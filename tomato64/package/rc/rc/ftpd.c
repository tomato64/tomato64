/*
 * ftpd.c
 *
 * Portions, Copyright (C) 2006-2009 Jonathan Zarate
 * Fixes/updates (C) 2018 - 2023 pedro
 *
 */


#include "rc.h"

#define vsftpd_dir		"/etc/vsftpd"
#define vsftpd_conf		vsftpd_dir"/vsftpd.conf"
#define vsftpd_passwd		vsftpd_dir"/vsftpd.passwd"
#define vsftpd_users		vsftpd_dir"/vsftpd.users"
#define vsftpd_fw_script	vsftpd_dir"/vsftpd-fw.sh"
#define vsftpd_fw_del_script	vsftpd_dir"/vsftpd-clear-fw-tmp.sh"
#define vsftpd_run		"/var/run/vsftpd"

/* needed by logmsg() */
#define LOGMSG_DISABLE		DISABLE_SYSLOG_OSM
#define LOGMSG_NVDEBUG		"ftpd_debug"


static char *get_full_storage_path(char *val)
{
	static char buf[128];
	int len;

	memset(buf, 0, sizeof(buf));

	if (val[0] == '/')
		len = snprintf(buf, sizeof(buf), "%s", val);
	else
		len = snprintf(buf, sizeof(buf), "%s/%s", MOUNT_ROOT, val);

	if (len > 1 && buf[len - 1] == '/')
		buf[len - 1] = 0;

	return buf;
}

static char *nvram_storage_path(char *var)
{
	char *val = nvram_safe_get(var);

	return get_full_storage_path(val);
}

static void build_ftpd_firewall_script(void)
{
	FILE *p;
	char *u, *c;
	char *en, *sec, *hit;
	char t[512], s[64];

	/* for LAN & WAN */
	if (!nvram_match("ftp_enable", "1"))
		return;

	/* create firewall script */
	if (!(p = fopen(vsftpd_fw_script, "w"))) {
		logerr(__FUNCTION__, __LINE__, vsftpd_fw_script);
		return;
	}

	chains_log_detection();

	/* FTP WAN access */
	fprintf(p, "#!/bin/sh\n"
	           "iptables -t nat -A WANPREROUTING -p tcp --dport %s -j DNAT --to-destination %s\n",
	           nvram_safe_get("ftp_port"), nvram_safe_get("lan_ipaddr"));

	/* FTP conn limit */
	strlcpy(s, nvram_safe_get("ftp_limit"), sizeof(s));
	if ((vstrsep(s, ",", &en, &hit, &sec) == 3) && (atoi(en))) {
		modprobe("xt_recent");
		fprintf(p, "iptables -A INPUT -p tcp --dport %s -m state --state NEW -j ftplimit\n",
		           nvram_safe_get("ftp_port"));
#ifdef TCONFIG_IPV6
		if (ipv6_enabled())
			fprintf(p, "ip6tables -A INPUT -p tcp --dport %s -m state --state NEW -j ftplimit\n",
			           nvram_safe_get("ftp_port"));
#endif
	}

	/* FTP WAN access */
	strlcpy(t, nvram_safe_get("ftp_sip"), sizeof(t));
	u = t;
	do {
		if ((c = strchr(u, ',')) != NULL)
			*c = 0;

		if (ipt_source(u, s, "ftp", "remote access"))
			fprintf(p, "iptables -A INPUT -p tcp %s --dport %s -j %s\n",
			           s, nvram_safe_get("ftp_port"), chain_in_accept);
#ifdef TCONFIG_IPV6
		if (ipv6_enabled() && ip6t_source(u, s, "ftp", "remote access"))
			fprintf(p, "ip6tables -A INPUT -p tcp %s --dport %s -j %s\n",
			           s, nvram_safe_get("ftp_port"), chain_in_accept);
#endif
		if (!c)
			break;

		u = c + 1;
	} while (*u);

	fclose(p);
	chmod(vsftpd_fw_script, 0744);
}

void start_ftpd(int force)
{
	char tmp[256];
	FILE *fp, *f;
	char *buf;
	char *p, *q;
	char *user, *pass, *rights, *root_dir;
	int i, ret;
#ifdef TCONFIG_HTTPS
	char t[32];
#endif

	/* only if enabled or forced */
	if (!nvram_get_int("ftp_enable") && force == 0)
		return;

	if (serialize_restart("vsftpd", 1))
		return;

	/* only if enabled */
	if (nvram_get_int("ftp_enable"))
		add_ftp_defaults(); /* backup: check nvram! */

	mkdir_if_none(vsftpd_users);
	mkdir_if_none(vsftpd_run);

	if ((fp = fopen(vsftpd_conf, "w")) == NULL) {
		logerr(__FUNCTION__, __LINE__, vsftpd_conf);
		return;
	}

	if (nvram_get_int("ftp_super")) {
		/* rights */
		memset(tmp, 0, sizeof(tmp));
		snprintf(tmp, sizeof(tmp), "%s/%s", vsftpd_users, "admin");
		if ((f = fopen(tmp, "w")) == NULL) {
			logerr(__FUNCTION__, __LINE__, tmp);
			fclose(fp);
			return;
		}

		fprintf(f, "dirlist_enable=yes\n"
		           "write_enable=yes\n"
		           "download_enable=yes\n");
		fclose(f);
	}

	if (nvram_invmatch("ftp_anonymous", "0")) {
		fprintf(fp, "anon_allow_writable_root=yes\n"
		            "anon_world_readable_only=no\n"
		            "anon_umask=022\n");

		/* rights */
		memset(tmp, 0, sizeof(tmp));
		snprintf(tmp, sizeof(tmp), "%s/ftp", vsftpd_users);
		if ((f = fopen(tmp, "w")) == NULL) {
			logerr(__FUNCTION__, __LINE__, tmp);
			fclose(fp);
			return;
		}

		if (nvram_match("ftp_dirlist", "0"))
			fprintf(f, "dirlist_enable=yes\n");
		if ((nvram_match("ftp_anonymous", "1")) || (nvram_match("ftp_anonymous", "3")))
			fprintf(f, "write_enable=yes\n");
		if ((nvram_match("ftp_anonymous", "1")) || (nvram_match("ftp_anonymous", "2")))
			fprintf(f, "download_enable=yes\n");
		fclose(f);

		if ((nvram_match("ftp_anonymous", "1")) || (nvram_match("ftp_anonymous", "3")))
			fprintf(fp, "anon_upload_enable=yes\n"
			            "anon_mkdir_write_enable=yes\n"
			            "anon_other_write_enable=yes\n");
	}
	else
		fprintf(fp, "anonymous_enable=no\n");

	fprintf(fp, "dirmessage_enable=yes\n"
	            "download_enable=no\n"
	            "dirlist_enable=no\n"
	            "hide_ids=yes\n"
	            "syslog_enable=yes\n"
	            "local_enable=yes\n"
	            "local_umask=022\n"
	            "chmod_enable=no\n"
	            "chroot_local_user=yes\n"
	            "check_shell=no\n"
	            "log_ftp_protocol=%s\n"
	            "user_config_dir=%s\n"
	            "passwd_file=%s\n"
	            "listen%s=yes\n"
	            "listen%s=no\n"
	            "listen_port=%s\n"
	            "background=yes\n"
#ifndef TCONFIG_BCMARM
	            "isolate=no\n"
#endif
	            "max_clients=%d\n"
	            "max_per_ip=%d\n"
	            "max_login_fails=1\n"
	            "idle_session_timeout=%s\n"
	            "use_sendfile=no\n"
	            "anon_max_rate=%d\n"
	            "local_max_rate=%d\n",
	            nvram_get_int("log_ftp") ? "yes" : "no",
	            vsftpd_users,
	            vsftpd_passwd,
#ifdef TCONFIG_IPV6
	            ipv6_enabled() ? "_ipv6" : "",
	            ipv6_enabled() ? "" : "_ipv6",
#else
	            "",
	            "_ipv6",
#endif
	            nvram_get("ftp_port") ? : "21",
	            nvram_get_int("ftp_max"),
	            nvram_get_int("ftp_ipmax"),
	            nvram_get("ftp_staytimeout") ? : "300",
	            nvram_get_int("ftp_anonrate") * 1024,
	            nvram_get_int("ftp_rate") * 1024);

#ifdef TCONFIG_HTTPS
	if (nvram_get_int("ftp_tls")) {
		fprintf(fp, "ssl_enable=YES\n"
		            "rsa_cert_file=/etc/cert.pem\n"
		            "rsa_private_key_file=/etc/key.pem\n"
		            "allow_anon_ssl=NO\n"
		            "force_local_data_ssl=YES\n"
		            "force_local_logins_ssl=YES\n"
		            "ssl_tlsv1=YES\n"
		            "ssl_sslv2=NO\n"
		            "ssl_sslv3=NO\n"
		            "require_ssl_reuse=NO\n"
		            "ssl_ciphers=HIGH\n");

		/* does a valid HTTPD cert exist? if not, generate one */
		if ((!f_exists("/etc/cert.pem")) || (!f_exists("/etc/key.pem"))) {
			gen_urandom(t, NULL, sizeof(t), 0);
			nvram_set("https_crt_gen", "1");
			nvram_set("https_crt_save", "1");
			eval("gencert.sh", t);
		}
	}
#endif /* TCONFIG_HTTPS */

	fprintf(fp, "ftpd_banner=Welcome to Tomato64 %s FTP service.\n"
	            "%s\n",
	            tomato_version,
	            nvram_safe_get("ftp_custom"));

	fclose(fp);

	/* prepare passwd file and default users */
	if ((fp = fopen(vsftpd_passwd, "w")) == NULL) {
		logerr(__FUNCTION__, __LINE__, vsftpd_passwd);
		return;
	}

	if ((user = nvram_safe_get("http_username")) && (!*user))
		user = "root";
	if ((pass = nvram_safe_get("http_passwd")) && (!*pass))
		pass = "admin";

	/* anonymous, admin, nobody */
	fprintf(fp, "ftp:x:0:0:ftp:%s:/sbin/nologin\n"
	            "%s:%s:0:0:root:/:/sbin/nologin\n"
	            "nobody:x:65534:65534:nobody:%s/:/sbin/nologin\n",
	            nvram_storage_path("ftp_anonroot"), user,
	            nvram_get_int("ftp_super") ? crypt(pass, "$1$") : "x",
	            MOUNT_ROOT);

	if ((buf = strdup(nvram_safe_get("ftp_users"))) && (*buf)) {
		/*
		 * username<password<rights[<root_dir>]
		 * rights:
		 *  Read/Write
		 *  Read Only
		 *  View Only
		 *  Private
		 */
		p = buf;
		while ((q = strsep(&p, ">")) != NULL) {
			i = vstrsep(q, "<", &user, &pass, &rights, &root_dir);
			if (i < 3)
				continue;
			if ((!user) || (!pass))
				continue;

			if ((i == 3) || (!root_dir) || (!(*root_dir)))
				root_dir = nvram_safe_get("ftp_pubroot");

			/* directory */
			memset(tmp, 0, sizeof(tmp));
			if (strncmp(rights, "Private", 7) == 0) {
				snprintf(tmp, sizeof(tmp), "%s/%s", nvram_storage_path("ftp_pvtroot"), user);
				mkdir_if_none(tmp);
			}
			else
				snprintf(tmp, sizeof(tmp), "%s", get_full_storage_path(root_dir));

			fprintf(fp, "%s:%s:0:0:%s:%s:/sbin/nologin\n", user, crypt(pass, "$1$"), user, tmp);

			/* rights */
			memset(tmp, 0, sizeof(tmp));
			snprintf(tmp, sizeof(tmp), "%s/%s", vsftpd_users, user);
			if ((f = fopen(tmp, "w")) == NULL) {
				logerr(__FUNCTION__, __LINE__, tmp);
				return;
			}

			tmp[0] = 0;
			if (nvram_invmatch("ftp_dirlist", "1"))
				strlcat(tmp, "dirlist_enable=yes\n", sizeof(tmp));
			if ((strstr(rights, "Read")) || (!strcmp(rights, "Private")))
				strlcat(tmp, "download_enable=yes\n", sizeof(tmp));
			if ((strstr(rights, "Write")) || (!strncmp(rights, "Private", 7)))
				strlcat(tmp, "write_enable=yes\n", sizeof(tmp));

			fputs(tmp, f);
			fclose(f);
		}
		free(buf);
	}

	fclose(fp);

	build_ftpd_firewall_script();

	run_ftpd_firewall_script();

	ret = eval("vsftpd", vsftpd_conf);
	if (ret) {
		logmsg(LOG_ERR, "starting vsftpd failed - check configuration ...");
		stop_ftpd();
	}
	else
		logmsg(LOG_INFO, "vsftpd is started");
}

void stop_ftpd(void)
{
	if (serialize_restart("vsftpd", 0))
		return;

	if (pidof("vsftpd") > 0) {
		killall_tk_period_wait("vsftpd", 50);
		logmsg(LOG_INFO, "vsftpd is stopped");
	}

	run_del_firewall_script(vsftpd_fw_script, vsftpd_fw_del_script);

	/* clean-up */
	unlink(vsftpd_passwd);
	unlink(vsftpd_conf);
	eval("rm", "-rf", vsftpd_fw_script);
	eval("rm", "-rf", vsftpd_users);
}

void run_ftpd_firewall_script(void)
{
	FILE *fp;

	/* first remove existing firewall rule(s) */
	run_del_firewall_script(vsftpd_fw_script, vsftpd_fw_del_script);

	/* then (re-)add firewall rule(s) */
	if ((fp = fopen(vsftpd_fw_script, "r"))) {
		fclose(fp);
		logmsg(LOG_DEBUG, "*** %s: running firewall script: %s", __FUNCTION__, vsftpd_fw_script);
		eval(vsftpd_fw_script);
		fix_chain_in_drop();
	}
}
