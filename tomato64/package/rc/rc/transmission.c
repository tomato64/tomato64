/*
 * transmission.c
 *
 * Copyright (C) 2011 shibby
 *
 * Fixes/updates (C) 2018 - 2026 pedro
 * https://freshtomato.org/
 *
 */


#include "rc.h"

#include <sys/types.h>

#define tr_dir			"/etc/transmission"
#define tr_settings		tr_dir"/settings.json"
#define tr_fw_script		tr_dir"/tr-fw.sh"
#define tr_fw_del_script	tr_dir"/tr-clear-fw-tmp.sh"
#define tr_child_pid		tr_dir"/child.pid"

/* needed by logmsg() */
#define LOGMSG_DISABLE	DISABLE_SYSLOG_OSM
#define LOGMSG_NVDEBUG	"transmission_debug"


static void json_write_string(FILE *fp, const char *s1, const char *s2)
{
	const char *s;
	unsigned char c;
	int part;

	fputc('"', fp);

	for (part = 0; part < 2; ++part) {
		s = (part == 0) ? s1 : s2;
		if (!s)
			continue;

		while (*s) {
			c = (unsigned char)*s++;

			switch (c) {
			case '"':
				fputs("\\\"", fp);
				break;
			case '\\':
				fputs("\\\\", fp);
				break;
			case '\b':
				fputs("\\b", fp);
				break;
			case '\f':
				fputs("\\f", fp);
				break;
			case '\n':
				fputs("\\n", fp);
				break;
			case '\r':
				fputs("\\r", fp);
				break;
			case '\t':
				fputs("\\t", fp);
				break;
			default:
				if (c < 0x20)
					fprintf(fp, "\\u%04x", (unsigned int)c);
				else
					fputc(c, fp);
				break;
			}
		}
	}
	fputc('"', fp);
}

static void json_write_setting(FILE *fp, const char *key, const char *value1, const char *value2)
{
	fprintf(fp, "\"%s\": ", key);
	json_write_string(fp, value1, value2);
	fputs(",\n", fp);
}

static int json_custom_is_space(char c)
{
	return (c == ' ' || c == '\t' || c == '\n' || c == '\r');
}

static int json_write_custom_settings(FILE *fp, const char *custom)
{
	const char *start;
	const char *end;

	if (!custom)
		return 0;

	start = custom;
	while (*start && (json_custom_is_space(*start) || *start == ','))
		start++;

	end = start + strlen(start);
	while (end > start && (json_custom_is_space(*(end - 1)) || *(end - 1) == ','))
		end--;

	if (end <= start)
		return 0;

	fwrite(start, 1, (size_t)(end - start), fp);
	fputs(",\n", fp);

	return 1;
}

static void setup_tr_watchdog(void)
{
	FILE *fp;
	char buffer[64], buffer2[64];
	int nvi;

	if ((nvi = nvram_get_int("bt_check_time")) > 0) {
		snprintf(buffer, sizeof(buffer), tr_dir"/watchdog.sh");

		if ((fp = fopen(buffer, "w"))) {
			fprintf(fp, "#!/bin/sh\n"
			            "[ -z \"$(pidof transmission-daemon)\" -a \"$(nvram get g_upgrade)\" != \"1\" -a \"$(nvram get g_reboot)\" != \"1\" ] && {\n"
			            " logger -t transmission-watchdog transmission-daemon stopped? Starting...\n"
			            " service bittorrent restart\n"
			            "}\n");
			fclose(fp);
			chmod(buffer, (S_IRUSR | S_IWUSR | S_IXUSR));

			snprintf(buffer2, sizeof(buffer2), "*/%d * * * * %s", nvi, buffer);
			eval("cru", "a", "CheckTransmission", buffer2);
		}
	}
}

static void build_tr_firewall(void)
{
	FILE *p;

	/* create firewall script */
	if (!(p = fopen(tr_fw_script, "w"))) {
		logerr(__FUNCTION__, __LINE__, tr_fw_script);
		return;
	}

	chains_log_detection();

	/* open BT port */
	fprintf(p, "#!/bin/sh\n"
	           "iptables -A INPUT -p tcp --dport %d -j %s\n"
	           "iptables -A INPUT -p udp --dport %d -j %s\n",
	            nvram_get_int("bt_port"), chain_in_accept,
	            nvram_get_int("bt_port"), chain_in_accept);
#ifdef TCONFIG_IPV6
	if (ipv6_enabled())
		fprintf(p, "ip6tables -A INPUT -p tcp --dport %d -j %s\n"
		           "ip6tables -A INPUT -p udp --dport %d -j %s\n",
		           nvram_get_int("bt_port"), chain_in_accept,
		           nvram_get_int("bt_port"), chain_in_accept);
#endif

	/* GUI WAN access */
	if (nvram_get_int("bt_rpc_wan"))
		fprintf(p, "iptables -A INPUT -p tcp --dport %d -j %s\n"
		           "iptables -t nat -A WANPREROUTING -p tcp --dport %d -j DNAT --to-destination %s\n", /* nat table */
		            nvram_get_int("bt_port_gui"), chain_in_accept,
		            nvram_get_int("bt_port_gui"), nvram_safe_get("lan_ipaddr"));

	fclose(p);
	chmod(tr_fw_script, 0744);
}

void start_bittorrent(int force)
{
	FILE *fp;
	const char *pb, *pc, *pd, *pe, *pf, *ph, *pi, *pj, *pk, *pl, *pm, *pn, *po, *pp, *pr, *pt, *pu;
	const char *whitelistEnabled, *custom;
	char buf[256], buf2[64], settings_dir[256], log_path[256];
	int n;
	pid_t pidof_child = 0;
	pid_t child;

	/* only if enabled or forced */
	if (!nvram_get_int("bt_enable") && force == 0)
		return;

	if (serialize_restart("transmission-da", 1))
		return;

	memset(buf2, 0, sizeof(buf2));
	if (f_read_string(tr_child_pid, buf2, sizeof(buf2)) > 0 && atoi(buf2) > 0 && ppid(atoi(buf2)) > 0) { /* fork is still up */
		logmsg(LOG_WARNING, "%s: another process (PID: %s) still up, aborting ...", __FUNCTION__, buf2);
		return;
	}

	/* collecting data */
	if (nvram_get_int("bt_rpc_enable"))              { pb = "true"; } else { pb = "false"; }
	if (nvram_get_int("bt_dl_enable"))               { pc = "true"; } else { pc = "false"; }
	if (nvram_get_int("bt_ul_enable"))               { pd = "true"; } else { pd = "false"; }
	if (nvram_get_int("bt_incomplete"))              { pe = "true"; } else { pe = "false"; }
	if (nvram_get_int("bt_autoadd"))                 { pf = "true"; } else { pf = "false"; }
	if (nvram_get_int("bt_ratio_enable"))            { ph = "true"; } else { ph = "false"; }
	if (nvram_get_int("bt_dht"))                     { pi = "true"; } else { pi = "false"; }
	if (nvram_get_int("bt_pex"))                     { pj = "true"; } else { pj = "false"; }
	if (nvram_get_int("bt_blocklist"))               { pm = "true"; } else { pm = "false"; }
	if (nvram_get_int("bt_lpd"))                     { po = "true"; } else { po = "false"; }
	if (nvram_get_int("bt_utp"))                     { pp = "true"; } else { pp = "false"; }
	if (nvram_get_int("bt_ratio_idle_enable"))       { pr = "true"; } else { pr = "false"; }
	if (nvram_get_int("bt_dl_queue_enable"))         { pt = "true"; } else { pt = "false"; }
	if (nvram_get_int("bt_ul_queue_enable"))         { pu = "true"; } else { pu = "false"; }

	if      (nvram_match("bt_settings", "down_dir")) { pk = nvram_safe_get("bt_dir"); }
	else if (nvram_match("bt_settings", "custom"))   { pk = nvram_safe_get("bt_settings_custom"); }
	else                                             { pk = nvram_safe_get("bt_settings"); }

	if      (nvram_match("bt_binary", "internal"))   { pn = "/usr/bin"; }
	else if (nvram_match("bt_binary", "optware") )   { pn = "/opt/bin"; }
	else                                             { pn = nvram_safe_get("bt_binary_custom"); }

	if (nvram_get_int("bt_auth")) {
		pl = "true";
		whitelistEnabled = "false";
	}
	else {
		pl = "false";
		whitelistEnabled = "true";
	}

	custom = nvram_safe_get("bt_custom");

	/* writing data to file */
	mkdir_if_none(tr_dir);
	if (!(fp = fopen(tr_settings, "w"))) {
		logerr(__FUNCTION__, __LINE__, tr_settings);
		return;
	}

	fprintf(fp, "{\n"
	            "\"peer-port\": %d,\n"
	            "\"speed-limit-down-enabled\": %s,\n"
	            "\"speed-limit-up-enabled\": %s,\n"
	            "\"speed-limit-down\": %d,\n"
	            "\"speed-limit-up\": %d,\n"
	            "\"rpc-enabled\": %s,\n"
	            "\"rpc-port\": %d,\n"
	            "\"rpc-bind-address\": \"0.0.0.0\",\n"
	            "\"rpc-whitelist\": \"*\",\n"
	            "\"rpc-whitelist-enabled\": %s,\n"
	            "\"rpc-host-whitelist\": \"*\",\n"
	            "\"rpc-host-whitelist-enabled\": %s,\n",
	            nvram_get_int("bt_port"),
	            pc,
	            pd,
	            nvram_get_int("bt_dl"),
	            nvram_get_int("bt_ul"),
	            pb,
	            nvram_get_int("bt_port_gui"),
	            whitelistEnabled,
	            whitelistEnabled);

	json_write_setting(fp, "rpc-username", nvram_safe_get("bt_login"), NULL);
	json_write_setting(fp, "rpc-password", nvram_safe_get("bt_password"), NULL);
	json_write_setting(fp, "download-dir", nvram_safe_get("bt_dir"), NULL);

	fprintf(fp, "\"incomplete-dir-enabled\": %s,\n", pe);

	json_write_setting(fp, "incomplete-dir", nvram_safe_get("bt_dir"), "/.incomplete");
	json_write_setting(fp, "watch-dir", nvram_safe_get("bt_dir"), NULL);

	fprintf(fp, "\"watch-dir-enabled\": %s,\n"
	            "\"peer-limit-global\": %d,\n"
	            "\"peer-limit-per-torrent\": %d,\n"
	            "\"upload-slots-per-torrent\": %d,\n"
	            "\"dht-enabled\": %s,\n"
	            "\"pex-enabled\": %s,\n"
	            "\"lpd-enabled\": %s,\n"
	            "\"utp-enabled\": %s,\n"
	            "\"ratio-limit-enabled\": %s,\n"
	            "\"ratio-limit\": %s,\n"
	            "\"idle-seeding-limit-enabled\": %s,\n"
	            "\"idle-seeding-limit\": %d,\n"
	            "\"blocklist-enabled\": %s,\n",
	            pf,
	            nvram_get_int("bt_peer_limit_global"),
	            nvram_get_int("bt_peer_limit_per_torrent"),
	            nvram_get_int("bt_ul_slot_per_torrent"),
	            pi,
	            pj,
	            po,
	            pp,
	            ph,
	            nvram_safe_get("bt_ratio"),
	            pr,
	            nvram_get_int("bt_ratio_idle"),
	            pm);

	json_write_setting(fp, "blocklist-url", nvram_safe_get("bt_blocklist_url"), NULL);

	fprintf(fp, "\"download-queue-enabled\": %s,\n"
	            "\"download-queue-size\": %d,\n"
	            "\"seed-queue-enabled\": %s,\n"
	            "\"seed-queue-size\": %d,\n"
	            "\"message-level\": %d,\n",
	            pt,
	            nvram_get_int("bt_dl_queue_size"),
	            pu,
	            nvram_get_int("bt_ul_queue_size"),
	            nvram_get_int("bt_message"));

	json_write_custom_settings(fp, custom);

	fprintf(fp, "\"rpc-authentication-required\": %s\n"
	            "}\n",
	            pl);

	if (fclose(fp) != 0) {
		logerr(__FUNCTION__, __LINE__, tr_settings);
		return;
	}

	chmod(tr_settings, 0644);

	/* create firewall script */
	build_tr_firewall();

	/* fork new process */
	child = fork();
	if (child < 0) {
		logerr(__FUNCTION__, __LINE__, "fork");
		return;
	}
	if (child != 0) /* foreground process */
		return;

	pidof_child = getpid();

	/* write child pid to a file */
	snprintf(buf2, sizeof(buf2), "%d", pidof_child);
	f_write_string(tr_child_pid, buf2, 0, 0);

	/* tune buffers */
	f_write_procsysnet("core/rmem_max", "4194304");
	f_write_procsysnet("core/wmem_max", "2080768");
	memset(buf2, 0, sizeof(buf2));
	if (f_read_string("/proc/sys/net/ipv4/tcp_adv_win_scale", buf2, sizeof(buf2)) > 0 && atoi(buf2) > 0)
		f_write_procsysnet("ipv4/tcp_adv_win_scale", "4");

	/* wait a given time for partition to be mounted, etc */
	n = atoi(nvram_safe_get("bt_sleep"));
	if (n > 0 && n < 60) {
		logmsg(LOG_INFO, "transmission-daemon - delaying start by %d seconds ...", n);
		sleep(n);
	}

	/* only now prepare subdirs */
	mkdir_if_none(pk);
	snprintf(buf, sizeof(buf), "%s/.settings", pk);
	mkdir_if_none(buf);
	if (nvram_get_int("bt_incomplete")) {
		snprintf(buf, sizeof(buf), "%s/.incomplete", pk);
		mkdir_if_none(buf);
	}

	snprintf(buf, sizeof(buf), "%s/.settings", pk);
	eval("cp", tr_settings, buf);

	snprintf(buf, sizeof(buf), "%s/.settings/blocklists", pk);
	eval("rm", "-rf", buf);

	if (nvram_get_int("bt_blocklist")) {
		mkdir_if_none(buf);

		snprintf(buf, sizeof(buf), "%s/.settings/blocklists/level1.gz", pk);
#ifdef TCONFIG_STUBBY
		eval("wget", nvram_safe_get("bt_blocklist_url"), "-O", buf);
#else
		eval("wget", "--no-check-certificate", nvram_safe_get("bt_blocklist_url"), "-O", buf);
#endif
		eval("gunzip", buf);
	}

	run_bt_firewall_script();

	snprintf(buf, sizeof(buf), "%s/transmission-daemon", pn);
	snprintf(settings_dir, sizeof(settings_dir), "%s/.settings", pk);

	setenv("EVENT_NOEPOLL", "1", 1);
#ifdef TCONFIG_STUBBY
	setenv("CURL_CA_BUNDLE", "/etc/ssl/cert.pem", 1);
#endif

	if (nvram_get_int("bt_log")) {
		snprintf(log_path, sizeof(log_path), "%s/transmission.log", nvram_safe_get("bt_log_path"));
		eval(buf, "-g", settings_dir, "-e", log_path);
	}
	else
		eval(buf, "-g", settings_dir);

	sleep(1);

	if (pidof("transmission-da") > 0) {
		logmsg(LOG_INFO, "transmission-daemon started");
		setup_tr_watchdog();
		eval("rm", "-f", tr_child_pid);
	}
	else {
		logmsg(LOG_ERR, "starting transmission-daemon failed - check configuration ...");
		eval("rm", "-f", tr_child_pid);
		stop_bittorrent();
	}

	/* terminate the child */
	_exit(0);
}

void stop_bittorrent(void)
{
	pid_t pid;
	char buf[16];
	int m = atoi(nvram_safe_get("bt_sleep")) + 10;

	if (serialize_restart("transmission-da", 0))
		return;

	pid = pidof("transmission-da");

	eval("cru", "d", "CheckTransmission");

	/* wait for child of start_bittorrent to finish (if any) */
	memset(buf, 0, sizeof(buf));
	while (f_read_string(tr_child_pid, buf, sizeof(buf)) > 0 && atoi(buf) > 0 && ppid(atoi(buf)) > 0 && (m-- > 0)) {
		logmsg(LOG_DEBUG, "*** %s: waiting for child process of start_bittorrent to end, %d secs left ...", __FUNCTION__, m);
		sleep(1);
	}

	killall_and_waitfor("transmission-da", 10, 50);

	if (pid > 0)
		logmsg(LOG_INFO, "transmission-daemon stopped");

	simple_lock("firewall");
	run_del_firewall_script(tr_fw_script, tr_fw_del_script);

	/* clean-up */
	eval("rm", "-rf", tr_dir);
	simple_unlock("firewall");

	/* restore default buffers */
	memset(buf, 0, sizeof(buf));
	if (f_read_string("/proc/sys/net/core/rmem_default", buf, sizeof(buf)) > 0 && atoi(buf) > 0)
		f_write_procsysnet("core/rmem_max", buf);

	memset(buf, 0, sizeof(buf));
	if (f_read_string("/proc/sys/net/core/wmem_default", buf, sizeof(buf)) > 0 && atoi(buf) > 0)
		f_write_procsysnet("core/wmem_max", buf);

	memset(buf, 0, sizeof(buf));
	if (f_read_string("/proc/sys/net/ipv4/tcp_adv_win_scale", buf, sizeof(buf)) > 0 && atoi(buf) > 0)
		f_write_procsysnet("ipv4/tcp_adv_win_scale", "2");
}

void run_bt_firewall_script(void)
{
	FILE *fp;

	/* first remove existing firewall rule(s) */
	simple_lock("firewall");
	run_del_firewall_script(tr_fw_script, tr_fw_del_script);

	/* then (re-)add firewall rule(s) */
	if ((fp = fopen(tr_fw_script, "r"))) {
		fclose(fp);
		logmsg(LOG_DEBUG, "*** %s: running firewall script: %s", __FUNCTION__, tr_fw_script);
		eval(tr_fw_script);
		fix_chain_in_drop();
	}
	simple_unlock("firewall");
}
