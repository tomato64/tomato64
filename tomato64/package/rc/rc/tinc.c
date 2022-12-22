/*
 *
 * Copyright (C) 2014-2021 Lance Fredrickson
 * lancethepants@gmail.com
 * Fixes/updates (C) 2018 - 2022 pedro
 *
*/


#include "rc.h"

#define BUF_SIZE 256
#define TINC_DIR		"/etc/tinc"
#define TINC_RSA_KEY		TINC_DIR"/rsa_key.priv"
#define TINC_PRIV_KEY		TINC_DIR"/ed25519_key.priv"
#define TINC_CONF		TINC_DIR"/tinc.conf"
#define TINC_HOSTS		TINC_DIR"/hosts"
#define TINC_UP_SCRIPT		TINC_DIR"/tinc-up"
#define TINC_DOWN_SCRIPT	TINC_DIR"/tinc-down"
#define TINC_FW_SCRIPT		TINC_DIR"/tinc-fw.sh"
#define TINC_FW_DEL_SCRIPT	TINC_DIR"/tinc-clear-fw-tmp.sh"
#define TINC_HOSTUP_SCRIPT	TINC_DIR"/host-up"
#define TINC_HOSTDOWN_SCRIPT	TINC_DIR"/host-down"
#define TINC_SUBNETUP_SCRIPT	TINC_DIR"/subnet-up"
#define TINC_SUBNETDOWN_SCRIPT	TINC_DIR"/subnet-down"

/* needed by logmsg() */
#define LOGMSG_DISABLE	DISABLE_SYSLOG_OSM
#define LOGMSG_NVDEBUG	"tinc_debug"


static void tinc_setup_watchdog(void)
{
	FILE *fp;
	char buffer[64], buffer2[64];
	int nvi;

	if ((nvi = nvram_get_int("tinc_poll")) > 0) {
		memset(buffer, 0, sizeof(buffer));
		snprintf(buffer, sizeof(buffer), TINC_DIR"/watchdog.sh");

		if ((fp = fopen(buffer, "w"))) {
			fprintf(fp, "#!/bin/sh\n"
			            "[ \"$(nvram get g_upgrade)\" != \"1\" -a \"$(nvram get g_reboot)\" != \"1\" ] && {\n"
			            " if [ -z \"$(pidof tincd)\" ]; then\n"
			            "  logger -t tinc tincd stopped? Starting...\n"
			            "  service tinc restart\n"
			            " elif [ $(tinc dump connections | grep -v localhost | wc -l ) -lt 1 ]; then\n"
			            "  logger -t tincd[\"$(pidof tincd)\"] Restarting process to due connectivity issue\n"
			            "  service tinc restart\n"
			            " fi\n"
			            "}\n");
			fclose(fp);
			chmod(buffer, (S_IRUSR | S_IWUSR | S_IXUSR));

			memset(buffer2, 0, sizeof(buffer2));
			snprintf(buffer2, sizeof(buffer2), "*/%d * * * * %s", nvi, buffer);
			eval("cru", "a", "CheckTincDaemon", buffer2);
		}
	}
}

static void build_tinc_firewall(const char *port)
{
	FILE *p;

	/* Create firewall script */
	if (!(p = fopen(TINC_FW_SCRIPT, "w"))) {
		logerr(__FUNCTION__, __LINE__, TINC_FW_SCRIPT);
		return;
	}

	chains_log_detection();

	fprintf(p, "#!/bin/sh\n");

	if (!nvram_match("tinc_manual_firewall", "2")) {
		if (strcmp(port, "") == 0)
			port = "655";

		fprintf(p, "iptables -I INPUT -p udp --dport %s -j %s\n"
		           "iptables -I INPUT -p tcp --dport %s -j %s\n"
		           "iptables -I INPUT -i tinc -j %s\n"
		           "iptables -I FORWARD -i tinc -j ACCEPT\n",
		           port, chain_in_accept,
		           port, chain_in_accept,
		           chain_in_accept);

#ifdef TCONFIG_IPV6
		if (ipv6_enabled())
			fprintf(p, "\n"
			           "ip6tables -I INPUT -p udp --dport %s -j %s\n"
			           "ip6tables -I INPUT -p tcp --dport %s -j %s\n"
			           "ip6tables -I INPUT -i tinc -j %s\n"
			           "ip6tables -I FORWARD -i tinc -j ACCEPT\n",
			           port, chain_in_accept,
			           port, chain_in_accept,
			           chain_in_accept);
#endif
	}

	if (!nvram_match("tinc_manual_firewall", "0")) {
		fprintf(p, "\n");
		fprintf(p, "%s\n", nvram_safe_get("tinc_firewall"));
	}

	fclose(p);
	chmod(TINC_FW_SCRIPT, 0744);
}

void start_tinc(int force)
{
	char *nv, *nvp, *b;
	const char *connecto, *name, *address, *port, *compression, *subnet, *rsa, *ed25519, *custom, *tinc_tmp_value;
	char buffer[BUF_SIZE];
	int ret;
	FILE *fp, *hp;

	/* only if enabled on wanup or forced */
	if (!nvram_get_int("tinc_enable") && force == 0)
		return;

	if (serialize_restart("tincd", 1))
		return;

	/* create tinc directories */
	mkdir(TINC_DIR, 0700);
	mkdir(TINC_HOSTS, 0700);

	/* write private rsa key */
	if (strcmp(tinc_tmp_value = nvram_safe_get("tinc_private_rsa"), "") != 0) {
		if (!(fp = fopen(TINC_RSA_KEY, "w"))) {
			logerr(__FUNCTION__, __LINE__, TINC_RSA_KEY);
			return;
		}
		fprintf(fp, "%s\n", tinc_tmp_value);
		fclose(fp);
		chmod(TINC_RSA_KEY, 0600);
	}

	/* write private ed25519 key */
	if (strcmp(tinc_tmp_value = nvram_safe_get("tinc_private_ed25519"), "") != 0) {
		if (!(fp = fopen(TINC_PRIV_KEY, "w"))) {
			logerr(__FUNCTION__, __LINE__, TINC_PRIV_KEY);
			return;
		}
		fprintf(fp, "%s\n", tinc_tmp_value);
		fclose(fp);
		chmod(TINC_PRIV_KEY, 0600);
	}

	/* create tinc.conf */
	if (!(fp = fopen(TINC_CONF, "w"))) {
		logerr(__FUNCTION__, __LINE__, TINC_CONF);
		return;
	}

	fprintf(fp, "Name = %s\n"
	            "Interface = tinc\n"
	            "DeviceType = %s\n",
	            nvram_safe_get("tinc_name"),
	            nvram_safe_get("tinc_devicetype"));

	if (nvram_match("tinc_devicetype", "tun"))
		fprintf(fp, "Mode = router\n");
	else if (nvram_match("tinc_devicetype", "tap"))
		fprintf(fp, "Mode = %s\n", nvram_safe_get("tinc_mode"));

	/* create tinc host files */
	nvp = nv = strdup(nvram_safe_get("tinc_hosts"));
	if (!nv)
		return;

	while ((b = strsep(&nvp, ">")) != NULL) {

		if (vstrsep(b, "<", &connecto, &name, &address, &port, &compression, &subnet, &rsa, &ed25519, &custom) < 9)
			continue;

		memset(buffer, 0, (BUF_SIZE));
		snprintf(buffer, sizeof(buffer), TINC_HOSTS"/%s", name);
		if (!(hp = fopen(buffer, "w"))) {
			logerr(__FUNCTION__, __LINE__, buffer);
			return;
		}

		/* write Connecto's to tinc.conf, excluding the host system if connecto is enabled */
		if ((strcmp(connecto, "1") == 0) && (strcmp(nvram_safe_get("tinc_name"), name) != 0))
			fprintf(fp, "ConnectTo = %s\n", name);

		if (strcmp(rsa, "") != 0)
			fprintf(hp, "%s\n", rsa);

		if (strcmp( ed25519, "") != 0)
			fprintf(hp, "%s\n", ed25519);

		if (strcmp(address, "") != 0)
			fprintf(hp, "Address = %s\n", address);

		if (strcmp(subnet, "") != 0)
			fprintf(hp, "Subnet = %s\n", subnet);

		if (strcmp(compression, "") != 0)
			fprintf(hp, "Compression = %s\n", compression);

		if (strcmp(port, "") != 0)
			fprintf(hp, "Port = %s\n", port);

		if (strcmp(custom, "") != 0)
			fprintf(hp, "%s\n", custom);

		fclose(hp);

		if (strcmp(nvram_safe_get("tinc_name"), name) == 0) {
			/* create tinc-up script if this is the host system */
			if (!(hp = fopen(TINC_UP_SCRIPT, "w"))) {
				logerr(__FUNCTION__, __LINE__, TINC_UP_SCRIPT);
				return;
			}

			fprintf(hp, "#!/bin/sh\n");

			/* Determine whether automatically generate tinc-up, or use manually supplied script */
			if (!nvram_match("tinc_manual_tinc_up", "1")) {

				/* those are removed by tinc itself on stop, no need for tinc-down script (interesting...) */
				if (nvram_match("tinc_devicetype", "tun"))
					fprintf(hp, "ifconfig $INTERFACE %s netmask %s\n", nvram_safe_get("lan_ipaddr"), nvram_safe_get("tinc_vpn_netmask"));
				else if (nvram_match("tinc_devicetype", "tap")) {
					fprintf(hp, "brctl addif %s $INTERFACE\n", nvram_safe_get("lan_ifname"));
					fprintf(hp, "ifconfig $INTERFACE 0.0.0.0 promisc up\n");
				}
			}
			else
				fprintf(hp, "%s\n", nvram_safe_get("tinc_tinc_up"));

			fclose(hp);
			chmod(TINC_UP_SCRIPT, 0744);

			/* create firewall script */
			build_tinc_firewall(port);
		}
	}

	/* Write tinc.conf custom configuration */
	if (strcmp(tinc_tmp_value = nvram_safe_get("tinc_custom"), "") != 0)
		fprintf(fp, "%s\n", tinc_tmp_value);

	fclose(fp);
	free(nv);

	/* write tinc-down script */
	if (strcmp(tinc_tmp_value = nvram_safe_get("tinc_tinc_down"), "") != 0) {
		if (!(fp = fopen(TINC_DOWN_SCRIPT, "w"))) {
			logerr(__FUNCTION__, __LINE__, TINC_DOWN_SCRIPT);
			return;
		}
		fprintf(fp, "#!/bin/sh\n");
		fprintf(fp, "%s\n", tinc_tmp_value);
		fclose(fp);
		chmod(TINC_DOWN_SCRIPT, 0744);
	}

	/* write host-up */
	if (strcmp(tinc_tmp_value = nvram_safe_get("tinc_host_up"), "") != 0) {
		if (!(fp = fopen(TINC_HOSTUP_SCRIPT, "w"))) {
			logerr(__FUNCTION__, __LINE__, TINC_HOSTUP_SCRIPT);
			return;
		}
		fprintf(fp, "#!/bin/sh\n" );
		fprintf(fp, "%s\n", tinc_tmp_value );
		fclose(fp);
		chmod(TINC_HOSTUP_SCRIPT, 0744);
	}

	/* write host-down */
	if (strcmp(tinc_tmp_value = nvram_safe_get("tinc_host_down"), "") != 0) {
		if (!(fp = fopen(TINC_HOSTDOWN_SCRIPT, "w"))) {
			logerr(__FUNCTION__, __LINE__, TINC_HOSTDOWN_SCRIPT);
			return;
		}
		fprintf(fp, "#!/bin/sh\n");
		fprintf(fp, "%s\n", tinc_tmp_value);
		fclose(fp);
		chmod(TINC_HOSTDOWN_SCRIPT, 0744);
	}

	/* write subnet-up */
	if (strcmp(tinc_tmp_value = nvram_safe_get("tinc_subnet_up"), "") != 0) {
		if (!(fp = fopen(TINC_SUBNETUP_SCRIPT, "w"))) {
			logerr(__FUNCTION__, __LINE__, TINC_SUBNETUP_SCRIPT);
			return;
		}
		fprintf(fp, "#!/bin/sh\n");
		fprintf(fp, "%s\n", tinc_tmp_value);
		fclose(fp);
		chmod(TINC_SUBNETUP_SCRIPT, 0744);
	}

	/* write subnet-down */
	if (strcmp(tinc_tmp_value = nvram_safe_get("tinc_subnet_down"), "") != 0) {
		if (!(fp = fopen(TINC_SUBNETDOWN_SCRIPT, "w"))) {
			logerr(__FUNCTION__, __LINE__, TINC_SUBNETDOWN_SCRIPT);
			return;
		}
		fprintf(fp, "#!/bin/sh\n");
		fprintf(fp, "%s\n", tinc_tmp_value);
		fclose(fp);
		chmod(TINC_SUBNETDOWN_SCRIPT, 0744);
	}

	/* make sure module is loaded */
	modprobe("tun");
	f_wait_exists("/dev/net/tun", 5);

	run_tinc_firewall_script();

	ret = eval("/usr/sbin/tinc", "start");

	if (ret) {
		logmsg(LOG_ERR, "starting tincd failed - check configuration ...");
		stop_tinc();
	}
	else {
		logmsg(LOG_INFO, "tincd started");
		tinc_setup_watchdog();
	}
}

void stop_tinc(void)
{
	if (serialize_restart("tincd", 0))
		return;

	eval("cru", "d", "CheckTincDaemon");

	if (pidof("tincd") > 0)
		killall_tk_period_wait("tincd", 50);

	run_del_firewall_script(TINC_FW_SCRIPT, TINC_FW_DEL_SCRIPT);

	system("/bin/rm -rf "TINC_DIR);
}

void run_tinc_firewall_script(void)
{
	FILE *fp;

	/* first remove existing firewall rule(s) */
	run_del_firewall_script(TINC_FW_SCRIPT, TINC_FW_DEL_SCRIPT);

	/* then (re-)add firewall rule(s) */
	if ((fp = fopen(TINC_FW_SCRIPT, "r"))) {
		fclose(fp);
		logmsg(LOG_DEBUG, "*** %s: running firewall script: %s", __FUNCTION__, TINC_FW_SCRIPT);
		eval(TINC_FW_SCRIPT);
	}
}
