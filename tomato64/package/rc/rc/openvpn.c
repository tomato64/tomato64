/*
 *
 * Copyright (C) 2008-2010 Keith Moyer, tomatovpn@keithmoyer.com
 *
 * No part of this file may be used without permission.
 *
 * Fixes/updates (C) 2018 - 2022 pedro
 *
 */


#include "rc.h"

#include <sys/types.h>
#include <dirent.h>
#include <string.h>
#include <time.h>

#define OVPN_CLIENT_BASEIF	10
#define OVPN_SERVER_BASEIF	20

#define BUF_SIZE		256
#define IF_SIZE			8
#define OVPN_FW_STR		"s/-A/-D/g"
#define OVPN_DIR		"/etc/openvpn"
#define OVPN_DEL_SCRIPT		"clear-fw-tmp.sh"
#define OVPN_DIR_DEL_SCRIPT	OVPN_DIR"/fw/"OVPN_DEL_SCRIPT

/* OpenVPN clients/servers count */
#define OVPN_SERVER_MAX		2

#if defined(TCONFIG_BCMARM)
#define OVPN_CLIENT_MAX		3
#else
#define OVPN_CLIENT_MAX		2
#endif

/* needed by logmsg() */
#define LOGMSG_DISABLE	DISABLE_SYSLOG_OSM
#define LOGMSG_NVDEBUG	"openvpn_debug"

/* OpenVPN routing policy modes (rgw) */
enum {
	OVPN_RGW_NONE = 0,
	OVPN_RGW_ALL,
	OVPN_RGW_POLICY,
	OVPN_RGW_POLICY_STRICT
};

typedef enum ovpn_route
{
	NONE = 0,
	BRIDGE,
	NAT
} ovpn_route_t;

typedef enum ovpn_if
{
	OVPN_IF_TUN = 0,
	OVPN_IF_TAP
} ovpn_if_t;

typedef enum ovpn_auth
{
	OVPN_AUTH_STATIC = 0,
	OVPN_AUTH_TLS,
	OVPN_AUTH_CUSTOM
} ovpn_auth_t;

typedef enum ovpn_type
{
	OVPN_TYPE_SERVER = 0,
	OVPN_TYPE_CLIENT
} ovpn_type_t;

static int ovpn_setup_iface(char *iface, ovpn_if_t iface_type, ovpn_route_t route_mode, int unit, ovpn_type_t type) {
	char buffer[32];

	memset(buffer, 0, sizeof(buffer));
	snprintf(buffer, sizeof(buffer), "vpn_%s%d_br", (type == OVPN_TYPE_SERVER ? "server" : "client"), unit);

	/* Make sure module is loaded */
	modprobe("tun");
	f_wait_exists("/dev/net/tun", 5);

	/* Create tap/tun interface */
	if (eval("openvpn", "--mktun", "--dev", iface)) {
		logmsg(LOG_WARNING, "unable to create tunnel interface %s!", iface);
		return -1;
	}

	/* Bring interface up (TAP only) */
	if (iface_type == OVPN_IF_TAP) {
		if (route_mode == BRIDGE) {
			if (eval("brctl", "addif", nvram_safe_get(buffer), iface)) {
				logmsg(LOG_WARNING, "unable to add interface %s to bridge!", iface);
				return -1;
			}
		}

		if (eval("ifconfig", iface, "promisc", "up")) {
			logmsg(LOG_WARNING, "unable to bring tunnel interface %s up!", iface);
			return -1;
		}
	}

	return 0;
}

static void ovpn_remove_iface(ovpn_type_t type, int unit) {
	char buffer[8];
	int tmp = (type == OVPN_TYPE_CLIENT ? OVPN_CLIENT_BASEIF : OVPN_SERVER_BASEIF) + unit;

	/* NVRAM setting for device type could have changed, just try to remove both */
	memset(buffer, 0, sizeof(buffer));
	snprintf(buffer, sizeof(buffer), "tap%d", tmp);
	eval("openvpn", "--rmtun", "--dev", buffer);

	memset(buffer, 0, sizeof(buffer));
	snprintf(buffer, sizeof(buffer), "tun%d", tmp);
	eval("openvpn", "--rmtun", "--dev", buffer);
}

static void ovpn_setup_dirs(ovpn_type_t type, int unit) {
	char buffer[64];
	char *tmp = (type == OVPN_TYPE_SERVER ? "server" : "client");

	mkdir(OVPN_DIR, 0700);
	memset(buffer, 0, sizeof(buffer));
	snprintf(buffer, sizeof(buffer), OVPN_DIR"/%s%d", tmp, unit);
	mkdir(buffer, 0700);

	memset(buffer, 0, sizeof(buffer));
	snprintf(buffer, sizeof(buffer), OVPN_DIR"/vpn%s%d", tmp, unit);
	unlink(buffer);
	symlink("/usr/sbin/openvpn", buffer);

	if (type == OVPN_TYPE_CLIENT) {
		memset(buffer, 0, sizeof(buffer));
		snprintf(buffer, sizeof(buffer), OVPN_DIR"/client%d/updown-client.sh", unit);
		symlink("/usr/sbin/updown-client.sh", buffer);

		memset(buffer, 0, sizeof(buffer));
		snprintf(buffer, sizeof(buffer), OVPN_DIR"/client%d/vpnrouting.sh", unit);
		symlink("/usr/sbin/vpnrouting.sh", buffer);
	}
}

static void ovpn_cleanup_dirs(ovpn_type_t type, int unit) {
	char buffer[64];
	char *tmp = (type == OVPN_TYPE_SERVER ? "server" : "client");

	memset(buffer, 0, sizeof(buffer));
	snprintf(buffer, sizeof(buffer), OVPN_DIR"/%s%d", tmp, unit);
	eval("rm", "-rf", buffer);

	memset(buffer, 0, sizeof(buffer));
	snprintf(buffer, sizeof(buffer), OVPN_DIR"/vpn%s%d", tmp, unit);
	eval("rm", "-rf", buffer);

	memset(buffer, 0, sizeof(buffer));
	snprintf(buffer, sizeof(buffer), OVPN_DIR"/fw/%s%d-fw.sh", tmp, unit);
	eval("rm", "-rf", buffer);

	if (type == OVPN_TYPE_CLIENT) {
		memset(buffer, 0, sizeof(buffer));
		snprintf(buffer, sizeof(buffer), OVPN_DIR"/dns/client%d.resolv", unit);
		eval("rm", "-rf", buffer);

		rmdir(OVPN_DIR"/dns");
	}

	/* Attempt to remove directories. Will fail if not empty */
	rmdir(OVPN_DIR"/fw");
	rmdir(OVPN_DIR);
}

static void ovpn_setup_watchdog(ovpn_type_t type, const int unit)
{
	FILE *fp;
	char buffer[64], buffer2[64];
	char taskname[20];
	char *instanceType;
	int nvi;

	if (type == OVPN_TYPE_SERVER)
		instanceType = "server";
	else
		instanceType = "client";

	memset(buffer, 0, sizeof(buffer));
	snprintf(buffer, sizeof(buffer), "vpn_%s%d_poll", instanceType, unit);
	if ((nvi = nvram_get_int(buffer)) > 0) {
		memset(buffer, 0, sizeof(buffer));
		snprintf(buffer, sizeof(buffer), "/etc/openvpn/%s%d/watchdog.sh", instanceType, unit);

		if ((fp = fopen(buffer, "w"))) {
			fprintf(fp, "#!/bin/sh\n"
			            "[ -z \"$(pidof vpn%s%d)\" -a \"$(nvram get g_upgrade)\" != \"1\" -a \"$(nvram get g_reboot)\" != \"1\" ] && {\n"
			            " service vpn%s%d restart\n"
			            "}\n",
			            instanceType, unit,
			            instanceType, unit);
			fclose(fp);
			chmod(buffer, (S_IRUSR | S_IWUSR | S_IXUSR));

			memset(taskname, 0, sizeof(taskname));
			snprintf(taskname, sizeof(taskname),"CheckVPN%s%d", instanceType, unit);
			memset(buffer2, 0, sizeof(buffer2));
			snprintf(buffer2, sizeof(buffer2), "*/%d * * * * %s", nvi, buffer);
			eval("cru", "a", taskname, buffer2);
		}
	}
}

static void ovpn_kill_switch(void)
{
	unsigned int i, br, rules_count;
	int policy_type;
	int wan_unit, mwan_num;
	char *enable, *type, *value, *kswitch;
	char *nv, *nvp, *b, *c;
	char wan_prefix[] = "wanXX";
	char buf[64], buf2[64], val[64], wan_if[16];

	mwan_num = nvram_get_int("mwan_num");
	if ((mwan_num < 1) || (mwan_num > MWAN_MAX))
		mwan_num = 1;

	for (i = 1; i <= OVPN_CLIENT_MAX; ++i) {
		rules_count = 0;
		nv = nvp = strdup(getNVRAMVar("vpn_client%d_routing_val", i));

		while (nvp && (b = strsep(&nvp, ">")) != NULL) {
			enable = type = value = kswitch = NULL;

			/* enable<type<domain_or_IP<kill_switch> */
			if ((vstrsep(b, "<", &enable, &type, &value, &kswitch)) < 4)
				continue;

			/* check if rule is enabled and kill switch is active and IP/domain is set */
			if ((atoi(enable) != 1) || (atoi(kswitch) != 1) || (*value == '\0'))
				continue;

			policy_type = atoi(type);
			rules_count++;

			/* check all active WANs */
			for (wan_unit = 1; wan_unit <= mwan_num; ++wan_unit) {
				get_wan_prefix(wan_unit, wan_prefix);

				/* find WAN IF */
				memset(wan_if, 0, sizeof(wan_if)); /* reset */
				snprintf(wan_if, sizeof(wan_if), "%s", get_wanface(wan_prefix));
				if ((!*wan_if) || (strcmp(wan_if, "") == 0))
					continue;

				memset(val, 0, sizeof(val)); /* reset */
				snprintf(val, sizeof(val), "%s", value); /* copy IP/domain to buffer */

				/* "From Source IP" */
				if (policy_type == 1) {
					/* find correct bridge for given IP */
					for (br = 0; br < BRIDGE_COUNT; br++) {
						memset(buf, 0, sizeof(buf)); /* reset */
						snprintf(buf, sizeof(buf), (br == 0 ? "lan_ipaddr" : "lan%d_ipaddr"), br);

						char *lan_ip = nvram_safe_get(buf);
						if (strcmp(lan_ip, "") != 0) { /* only for active */
							memset(buf, 0, sizeof(buf)); /* reset */
							snprintf(buf, sizeof(buf), "%s", val);
							if ((c = strchr(buf, '/')) != NULL)
								*c = 0; /* with mask? get IP */

							memset(buf, 0, sizeof(buf)); /* reset */
							snprintf(buf, sizeof(buf), "%s", val);
							if ((c = strrchr(buf, '.')) != NULL)
								*(c + 1) = 0; /* get first 3 octets from value */

							memset(buf2, 0, sizeof(buf2)); /* reset */
							snprintf(buf2, sizeof(buf2), "%s", lan_ip);
							if ((c = strrchr(buf2, '.')) != NULL)
								*(c + 1) = 0; /* get first 3 octets from lan IP */

							if (strcmp(buf, buf2) == 0) {
								memset(buf2, 0, sizeof(buf2)); /* reset */
								snprintf(buf2, sizeof(buf2), "br%d", br); /* copy brX to buffer */

								eval("iptables", "-I", "FORWARD", "-i", buf2, "-s", val, "-o", wan_if, "-j", "REJECT");
							}
						}
					}
				}
				/* "To Destination IP" / "To Domain" */
				else if ((policy_type == 2) || (policy_type == 3)) {
					memset(buf, 0, sizeof(buf)); /* reset */
					snprintf(buf, sizeof(buf), "tun1%d", i); /* find the appropriate tun IF */

					xstart("iptables", "-I", "FORWARD", "!", "-o", buf, "-d", val, "-j", "REJECT");
					xstart("iptables", "-I", "FORWARD", "-o", wan_if, "-d", val, "-j", "REJECT");
				}

			}
		}
		if (nv)
			free(nv);

		if (rules_count > 0)
			logmsg(LOG_INFO, "Kill-Switch: added %d rules to firewall for openvpn-client%d", rules_count, i);
	}
}

void start_ovpn_client(int unit)
{
	FILE *fp;
	ovpn_auth_t auth_mode;
	ovpn_route_t route_mode;
	ovpn_if_t if_type;
	char iface[IF_SIZE];
	char buffer[BUF_SIZE];
	char buffer2[32];
	int nvi;
	long int nvl;
	int userauth, useronly;
	int taskset_ret = 0;
#if defined(TCONFIG_BCMARM) && defined(TCONFIG_BCMSMP)
	char cpulist[2];
	int cpu_num = sysconf(_SC_NPROCESSORS_CONF) - 1;
#endif

	memset(buffer, 0, BUF_SIZE);
	snprintf(buffer, sizeof(buffer), "vpnclient%d", unit);
	if (serialize_restart(buffer, 1))
		return;

	/* Determine interface */
	memset(buffer, 0, BUF_SIZE);
	snprintf(buffer, sizeof(buffer), "vpn_client%d_if", unit);
	if (nvram_contains_word(buffer, "tap"))
		if_type = OVPN_IF_TAP;
	else if (nvram_contains_word(buffer, "tun"))
		if_type = OVPN_IF_TUN;
	else {
		logmsg(LOG_WARNING, "invalid interface type, %.3s", nvram_safe_get(buffer));
		return;
	}

	/* Build interface name */
	snprintf(iface, IF_SIZE, "%s%d", nvram_safe_get(buffer), (unit + OVPN_CLIENT_BASEIF));

	/* Determine encryption mode */
	memset(buffer, 0, BUF_SIZE);
	snprintf(buffer, sizeof(buffer), "vpn_client%d_crypt", unit);
	if (nvram_contains_word(buffer, "tls"))
		auth_mode = OVPN_AUTH_TLS;
	else if (nvram_contains_word(buffer, "secret"))
		auth_mode = OVPN_AUTH_STATIC;
	else if (nvram_contains_word(buffer, "custom"))
		auth_mode = OVPN_AUTH_CUSTOM;
	else {
		logmsg(LOG_WARNING, "invalid encryption mode, %.6s", nvram_safe_get(buffer));
		return;
	}

	/* Determine if we should bridge the tunnel */
	memset(buffer, 0, BUF_SIZE);
	snprintf(buffer, sizeof(buffer), "vpn_client%d_bridge", unit);
	if (if_type == OVPN_IF_TAP && nvram_get_int(buffer) == 1)
		route_mode = BRIDGE;

	/* Determine if we should NAT the tunnel */
	memset(buffer, 0, BUF_SIZE);
	snprintf(buffer, sizeof(buffer), "vpn_client%d_nat", unit);
	if (((if_type == OVPN_IF_TUN) || (route_mode != BRIDGE)) && nvram_get_int(buffer) == 1)
		route_mode = NAT;

	/* Setup directories and symlinks */
	ovpn_setup_dirs(OVPN_TYPE_CLIENT, unit);

	/* Setup interface */
	if (ovpn_setup_iface(iface, if_type, route_mode, unit, OVPN_TYPE_CLIENT)) {
		stop_ovpn_client(unit);
		return;
	}

	userauth = atoi(getNVRAMVar("vpn_client%d_userauth", unit));
	useronly = userauth && atoi(getNVRAMVar("vpn_client%d_useronly", unit));

	/* Build and write config file */
	memset(buffer, 0, BUF_SIZE);
	snprintf(buffer, sizeof(buffer), OVPN_DIR"/client%d/config.ovpn", unit);
	fp = fopen(buffer, "w");
	chmod(buffer, (S_IRUSR | S_IWUSR));

	fprintf(fp, "# Generated Configuration\n"
	            "daemon openvpn-client%d\n"
	            "dev %s\n"
	            "txqueuelen 1000\n"
	            "persist-key\n"
	            "persist-tun\n",
	            unit,
	            iface);

	if (auth_mode == OVPN_AUTH_TLS)
		fprintf(fp, "client\n");

	fprintf(fp, "proto %s\n"
	            "remote %s "
	            "%d\n",
	            getNVRAMVar("vpn_client%d_proto", unit),
	            getNVRAMVar("vpn_client%d_addr", unit),
	            atoi(getNVRAMVar("vpn_client%d_port", unit)));

	if (auth_mode == OVPN_AUTH_STATIC) {
		fprintf(fp, "ifconfig %s ", getNVRAMVar("vpn_client%d_local", unit));

		if (if_type == OVPN_IF_TUN)
			fprintf(fp, "%s\n", getNVRAMVar("vpn_client%d_remote", unit));
		else if (if_type == OVPN_IF_TAP)
			fprintf(fp, "%s\n", getNVRAMVar("vpn_client%d_nm", unit));
	}

	memset(buffer, 0, BUF_SIZE);
	snprintf(buffer, sizeof(buffer), "vpn_client%d_retry", unit);
	if ((nvi = nvram_get_int(buffer)) >= 0)
		fprintf(fp, "resolv-retry %d\n", nvi);
	else
		fprintf(fp, "resolv-retry infinite\n");

	if ((nvl = atol(getNVRAMVar("vpn_client%d_reneg", unit))) >= 0)
		fprintf(fp, "reneg-sec %ld\n", nvl);

	memset(buffer, 0, BUF_SIZE);
	snprintf(buffer, sizeof(buffer), "vpn_client%d_nobind", unit);
	if (nvram_get_int(buffer) > 0)
		fprintf(fp, "nobind\n");

	/* Compression */
	memset(buffer, 0, BUF_SIZE);
	strlcpy(buffer, getNVRAMVar("vpn_client%d_comp", unit), sizeof(buffer));
	if (strcmp(buffer, "-1")) {
#ifndef TCONFIG_OPTIMIZE_SIZE_MORE
		if ((!strcmp(buffer, "lz4")) || (!strcmp(buffer, "lz4-v2")))
			fprintf(fp, "compress %s\n", buffer);
		else
#endif
		     if (!strcmp(buffer, "yes"))
			fprintf(fp, "compress lzo\n");
		else if (!strcmp(buffer, "adaptive"))
			fprintf(fp, "comp-lzo adaptive\n");
#ifndef TCONFIG_OPTIMIZE_SIZE_MORE
		else if ((!strcmp(buffer, "stub")) || (!strcmp(buffer, "stub-v2")))
			fprintf(fp, "compress %s\n", buffer);
#endif
		else if (!strcmp(buffer, "no"))
			fprintf(fp, "compress\n");	/* Disable, but can be overriden */
	}

	/* Cipher */
	memset(buffer, 0, BUF_SIZE);
	strlcpy(buffer, getNVRAMVar("vpn_client%d_ncp_ciphers", unit), sizeof(buffer));
	if (auth_mode == OVPN_AUTH_TLS) {
		if (buffer[0] != '\0')
#ifndef TCONFIG_OPTIMIZE_SIZE_MORE
			fprintf(fp, "data-ciphers %s\n", buffer);
#else
			fprintf(fp, "ncp-ciphers %s\n", buffer);
#endif
	}
#ifndef TCONFIG_OPTIMIZE_SIZE_MORE
	else {	/* SECRET/CUSTOM */
#endif
		memset(buffer, 0, BUF_SIZE);
		snprintf(buffer, sizeof(buffer), "vpn_client%d_cipher", unit);
		if (!nvram_contains_word(buffer, "default"))
			fprintf(fp, "cipher %s\n", nvram_safe_get(buffer));
#ifndef TCONFIG_OPTIMIZE_SIZE_MORE
	}
#endif

	/* Digest */
	memset(buffer, 0, BUF_SIZE);
	snprintf(buffer, sizeof(buffer), "vpn_client%d_digest", unit);
	if (!nvram_contains_word(buffer, "default"))
		fprintf(fp, "auth %s\n", nvram_safe_get(buffer));

	/* Routing */
	memset(buffer, 0, BUF_SIZE);
	snprintf(buffer, sizeof(buffer), "vpn_client%d_rgw", unit);
	nvi = nvram_get_int(buffer);

	if (nvi == OVPN_RGW_ALL) {
		if (if_type == OVPN_IF_TAP && getNVRAMVar("vpn_client%d_gw", unit)[0] != '\0')
			fprintf(fp, "route-gateway %s\n", getNVRAMVar("vpn_client%d_gw", unit));
		fprintf(fp, "redirect-gateway def1\n");
	}
	else if (nvi >= OVPN_RGW_POLICY)
		fprintf(fp, "pull-filter ignore \"redirect-gateway\"\n"
		            "redirect-private def1\n");

	/* Selective routing */
	fprintf(fp, "script-security 2\n"
	            "up updown-client.sh\n"
	            "down updown-client.sh\n"
	            "route-delay 2\n"
	            "route-up vpnrouting.sh\n"
	            "route-pre-down vpnrouting.sh\n");

	if (auth_mode == OVPN_AUTH_TLS) {
		memset(buffer, 0, BUF_SIZE);
		snprintf(buffer, sizeof(buffer), "vpn_client%d_hmac", unit);
		nvi = nvram_get_int(buffer);
		memset(buffer, 0, BUF_SIZE);
		snprintf(buffer, sizeof(buffer), "vpn_client%d_static", unit);

		if (!nvram_is_empty(buffer) && nvi >= 0) {
			if (nvi == 3)
				fprintf(fp, "tls-crypt static.key");
#ifndef TCONFIG_OPTIMIZE_SIZE_MORE
			else if (nvi == 4)
				fprintf(fp, "tls-crypt-v2 static.key");
#endif
			else
				fprintf(fp, "tls-auth static.key");

			if (nvi < 2)
				fprintf(fp, " %d", nvi);
			fprintf(fp, "\n");
		}

		memset(buffer, 0, BUF_SIZE);
		snprintf(buffer, sizeof(buffer), "vpn_client%d_ca", unit);
		if (!nvram_is_empty(buffer))
			fprintf(fp, "ca ca.crt\n");

		if (!useronly) {
			memset(buffer, 0, BUF_SIZE);
			snprintf(buffer, sizeof(buffer), "vpn_client%d_crt", unit);
			if (!nvram_is_empty(buffer))
				fprintf(fp, "cert client.crt\n");

			memset(buffer, 0, BUF_SIZE);
			snprintf(buffer, sizeof(buffer), "vpn_client%d_key", unit);
			if (!nvram_is_empty(buffer))
				fprintf(fp, "key client.key\n");
		}

		memset(buffer, 0, BUF_SIZE);
		snprintf(buffer, sizeof(buffer), "vpn_client%d_tlsremote", unit);
		if (nvram_get_int(buffer))
			fprintf(fp, "remote-cert-tls server\n");

		if ((nvi = atoi(getNVRAMVar("vpn_client%d_tlsvername", unit))) > 0) {
			fprintf(fp, "verify-x509-name \"%s\" ", getNVRAMVar("vpn_client%d_cn", unit));
			if (nvi == 2)
				fprintf(fp, "name-prefix\n");
			else if (nvi == 3)
				fprintf(fp, "subject\n");
			else
				fprintf(fp, "name\n");
		}

		if (userauth)
			fprintf(fp, "auth-user-pass up\n");
	}
	else if (auth_mode == OVPN_AUTH_STATIC) {
		memset(buffer, 0, BUF_SIZE);
		snprintf(buffer, sizeof(buffer), "vpn_client%d_static", unit);

		if (!nvram_is_empty(buffer))
			fprintf(fp, "secret static.key\n");
	}
	fprintf(fp, "keepalive 15 60\n"
	            "verb 3\n"
	            "status-version 2\n"
	            "status status 10\n" /* Update status file every 10 sec */
	            "pull-filter ignore \"ifconfig-ipv6\"\n" /* IPv6 currently not supported */
	            "pull-filter ignore \"route-ipv6\"\n\n"
	            "# Custom Configuration\n"
	            "%s",
	            getNVRAMVar("vpn_client%d_custom", unit));

	fclose(fp);

	/* Write certification and key files */
	if (auth_mode == OVPN_AUTH_TLS) {
		memset(buffer, 0, BUF_SIZE);
		snprintf(buffer, sizeof(buffer), "vpn_client%d_ca", unit);
		if (!nvram_is_empty(buffer)) {
			memset(buffer, 0, BUF_SIZE);
			snprintf(buffer, sizeof(buffer), OVPN_DIR"/client%d/ca.crt", unit);
			fp = fopen(buffer, "w");
			chmod(buffer, (S_IRUSR | S_IWUSR));
			fprintf(fp, "%s", getNVRAMVar("vpn_client%d_ca", unit));
			fclose(fp);
		}

		if (!useronly) {
			memset(buffer, 0, BUF_SIZE);
			snprintf(buffer, sizeof(buffer), "vpn_client%d_key", unit);
			if (!nvram_is_empty(buffer)) {
				memset(buffer, 0, BUF_SIZE);
				snprintf(buffer, sizeof(buffer), OVPN_DIR"/client%d/client.key", unit);
				fp = fopen(buffer, "w");
				chmod(buffer, (S_IRUSR | S_IWUSR));
				fprintf(fp, "%s", getNVRAMVar("vpn_client%d_key", unit));
				fclose(fp);
			}

			memset(buffer, 0, BUF_SIZE);
			snprintf(buffer, sizeof(buffer), "vpn_client%d_crt", unit);
			if (!nvram_is_empty(buffer)) {
				memset(buffer, 0, BUF_SIZE);
				snprintf(buffer, sizeof(buffer), OVPN_DIR"/client%d/client.crt", unit);
				fp = fopen(buffer, "w");
				chmod(buffer, (S_IRUSR | S_IWUSR));
				fprintf(fp, "%s", getNVRAMVar("vpn_client%d_crt", unit));
				fclose(fp);
			}
		}
		if (userauth) {
			memset(buffer, 0, BUF_SIZE);
			snprintf(buffer, sizeof(buffer), OVPN_DIR"/client%d/up", unit);
			fp = fopen(buffer, "w");
			chmod(buffer, (S_IRUSR | S_IWUSR));
			fprintf(fp, "%s\n", getNVRAMVar("vpn_client%d_username", unit));
			fprintf(fp, "%s\n", getNVRAMVar("vpn_client%d_password", unit));
			fclose(fp);
		}
	}
	memset(buffer, 0, BUF_SIZE);
	snprintf(buffer, sizeof(buffer), "vpn_client%d_hmac", unit);
	if ((auth_mode == OVPN_AUTH_STATIC) || (auth_mode == OVPN_AUTH_TLS && nvram_get_int(buffer) >= 0)) {
		memset(buffer, 0, BUF_SIZE);
		snprintf(buffer, sizeof(buffer), "vpn_client%d_static", unit);
		if (!nvram_is_empty(buffer)) {
			memset(buffer, 0, BUF_SIZE);
			snprintf(buffer, sizeof(buffer), OVPN_DIR"/client%d/static.key", unit);
			fp = fopen(buffer, "w");
			chmod(buffer, (S_IRUSR | S_IWUSR));
			fprintf(fp, "%s", getNVRAMVar("vpn_client%d_static", unit));
			fclose(fp);
		}
	}

	/* Handle firewall rules if appropriate */
	memset(buffer, 0, BUF_SIZE);
	snprintf(buffer, sizeof(buffer), "vpn_client%d_firewall", unit);
	if (!nvram_contains_word(buffer, "custom")) {
		chains_log_detection();

		/* Create firewall rules */
		mkdir(OVPN_DIR"/fw", 0700);
		memset(buffer, 0, BUF_SIZE);
		snprintf(buffer, sizeof(buffer), OVPN_DIR"/fw/client%d-fw.sh", unit);
		fp = fopen(buffer, "w");
		chmod(buffer, (S_IRUSR | S_IWUSR | S_IXUSR));
		fprintf(fp, "#!/bin/sh\n");

		memset(buffer, 0, BUF_SIZE);
		snprintf(buffer, sizeof(buffer), "vpn_client%d_fw", unit);
		nvi = nvram_get_int(buffer);
		fprintf(fp, "iptables -I INPUT -i %s -m state --state NEW -j %s\n"
		            "iptables -I FORWARD -i %s -m state --state NEW -j %s\n"
		            "iptables -I FORWARD -o %s -j ACCEPT\n",
		            iface, (nvi ? chain_in_drop : chain_in_accept),
		            iface, (nvi ? "DROP" : "ACCEPT"),
		            iface);
#ifdef TCONFIG_BCMARM
		if (!nvram_get_int("ctf_disable")) /* bypass CTF if enabled */
			fprintf(fp, "iptables -t mangle -I PREROUTING -i %s -j MARK --set-mark 0x01/0x7\n", iface);
#endif /* TCONFIG_BCMARM */

		if (route_mode == NAT)
			/* masquerade all client outbound traffic regardless of source subnet */
			fprintf(fp, "iptables -t nat -I POSTROUTING -o %s -j MASQUERADE\n", iface);

		/* Create firewall rules for IPv6 */
#ifdef TCONFIG_IPV6
		if (ipv6_enabled()) {
			fprintf(fp, "ip6tables -I INPUT -i %s -m state --state NEW -j %s\n"
			            "ip6tables -I FORWARD -i %s -m state --state NEW -j %s\n"
			            "ip6tables -I FORWARD -o %s -j ACCEPT\n",
			            iface, (nvi ? chain_in_drop : chain_in_accept),
			            iface, (nvi ? "DROP" : "ACCEPT"),
			            iface);
		}
#endif

		memset(buffer, 0, BUF_SIZE);
		snprintf(buffer, sizeof(buffer), "vpn_client%d_rgw", unit);
		nvi = nvram_get_int(buffer);
		if (nvi >= OVPN_RGW_POLICY) {
			/* Disable rp_filter when in policy mode */
			fprintf(fp, "echo 0 > /proc/sys/net/ipv4/conf/%s/rp_filter\n"
			            "echo 0 > /proc/sys/net/ipv4/conf/all/rp_filter\n",
			            iface);

#if defined(TCONFIG_BCMARM)
			modprobe("xt_set");
			modprobe("ip_set");
			modprobe("ip_set_hash_ip");
#else
			modprobe("ipt_set");
			modprobe("ip_set");
			modprobe("ip_set_iphash");
#endif

		}

		fclose(fp);

		/* firewall rules */
		memset(buffer, 0, BUF_SIZE);
		snprintf(buffer, sizeof(buffer), OVPN_DIR"/fw/client%d-fw.sh", unit);

		/* first remove existing firewall rule(s) */
		run_del_firewall_script(buffer, OVPN_DIR_DEL_SCRIPT);

		/* then add firewall rule(s) */
		eval(buffer);
	}

	/* In case of openvpn unexpectedly dies and leaves it added - flush tun IF, otherwise openvpn will not re-start (required by iproute2) */
	eval("/usr/sbin/ip", "addr", "flush", "dev", iface);

	/* Start the VPN client */
	memset(buffer, 0, BUF_SIZE);
	snprintf(buffer, sizeof(buffer), OVPN_DIR"/vpnclient%d", unit);
	memset(buffer2, 0, sizeof(buffer2));
	snprintf(buffer2, sizeof(buffer2), OVPN_DIR"/client%d", unit);

#if defined(TCONFIG_BCMARM) && defined(TCONFIG_BCMSMP)
	/* Spread clients on cpu 1,0 or 1,2,3,0 (in that order) */
	cpu_num = sysconf(_SC_NPROCESSORS_CONF) - 1;
	if (cpu_num < 0)
		cpu_num = 0;

	snprintf(cpulist, sizeof(cpulist), "%d", (unit & cpu_num));
	taskset_ret = cpu_eval(NULL, cpulist, buffer, "--cd", buffer2, "--config", "config.ovpn");

	if (taskset_ret)
#endif
	taskset_ret = eval(buffer, "--cd", buffer2, "--config", "config.ovpn");

	if (taskset_ret) {
		logmsg(LOG_WARNING, "starting OpenVPN client%d failed - check configuration ...", unit);
		stop_ovpn_client(unit);
		return;
	}

	/* Set up cron job */
	ovpn_setup_watchdog(OVPN_TYPE_CLIENT, unit);

	memset(buffer, 0, BUF_SIZE);
	snprintf(buffer, sizeof(buffer), "vpn_client%d", unit);
	allow_fastnat(buffer, 0);
	try_enabling_fastnat();
}

void stop_ovpn_client(int unit)
{
	char buffer[BUF_SIZE];

	memset(buffer, 0, BUF_SIZE);
	snprintf(buffer, sizeof(buffer), "vpnclient%d", unit);
	if (serialize_restart(buffer, 0))
		return;

	/* Remove cron job */
	memset(buffer, 0, BUF_SIZE);
	snprintf(buffer, sizeof(buffer), "CheckVPNclient%d", unit);
	eval("cru", "d", buffer);

	/* Stop the VPN client */
	memset(buffer, 0, BUF_SIZE);
	snprintf(buffer, sizeof(buffer), "vpnclient%d", unit);
	killall_and_waitfor(buffer, 5, 50);

	ovpn_remove_iface(OVPN_TYPE_CLIENT, unit);

	/* Remove firewall rules after VPN exit */
	memset(buffer, 0, BUF_SIZE);
	snprintf(buffer, sizeof(buffer), OVPN_DIR"/fw/client%d-fw.sh", unit);
	run_del_firewall_script(buffer, OVPN_DIR_DEL_SCRIPT);

	/* Delete all files for this client */
	ovpn_cleanup_dirs(OVPN_TYPE_CLIENT, unit);

	memset(buffer, 0, BUF_SIZE);
	snprintf(buffer, sizeof(buffer), "vpn_client%d", unit);
	allow_fastnat(buffer, 1);
	try_enabling_fastnat();
}

void start_ovpn_server(int unit)
{
	FILE *fp;
	ovpn_auth_t auth_mode;
	ovpn_if_t if_type;
	char iface[IF_SIZE];
	char buffer[BUF_SIZE];
	char buffer2[32];
	int mwan_num, taskset_ret = 0;
	long int nvl;
#ifndef TCONFIG_OPTIMIZE_SIZE_MORE
	FILE *ccd;
	char *br_ipaddr, *br_netmask;
	char *chp, *route;
	int nvi, i, ip[4], nm[4];
	int c2c = 0;
	int dont_push_active = 0;
	int push_lan[4] = {0};
#endif
#if defined(TCONFIG_BCMARM) && defined(TCONFIG_BCMSMP)
	char cpulist[2];
	int cpu_num = sysconf(_SC_NPROCESSORS_CONF) - 1;
#endif

	memset(buffer, 0, BUF_SIZE);
	snprintf(buffer, sizeof(buffer), "vpnserver%d", unit);
	if (serialize_restart(buffer, 1))
		return;

	/* Determine interface */
	memset(buffer, 0, BUF_SIZE);
	snprintf(buffer, sizeof(buffer), "vpn_server%d_if", unit);
	if (nvram_contains_word(buffer, "tap"))
		if_type = OVPN_IF_TAP;
	else if (nvram_contains_word(buffer, "tun"))
		if_type = OVPN_IF_TUN;
	else {
		logmsg(LOG_WARNING, "invalid interface type, %.3s", nvram_safe_get(buffer));
		return;
	}

	/* Build interface name */
	snprintf(iface, IF_SIZE, "%s%d", nvram_safe_get(buffer), (unit + OVPN_SERVER_BASEIF));

	/* Determine encryption mode */
	memset(buffer, 0, BUF_SIZE);
	snprintf(buffer, sizeof(buffer), "vpn_server%d_crypt", unit);
	if (nvram_contains_word(buffer, "tls"))
		auth_mode = OVPN_AUTH_TLS;
	else if (nvram_contains_word(buffer, "secret"))
		auth_mode = OVPN_AUTH_STATIC;
	else if (nvram_contains_word(buffer, "custom"))
		auth_mode = OVPN_AUTH_CUSTOM;
	else {
		logmsg(LOG_WARNING, "invalid encryption mode, %.6s", nvram_safe_get(buffer));
		return;
	}

	if (is_intf_up(iface) > 0 && if_type == OVPN_IF_TAP)
		eval("brctl", "delif", getNVRAMVar("vpn_server%d_br", unit), iface);

	/* Setup directories and symlinks */
	ovpn_setup_dirs(OVPN_TYPE_SERVER, unit);

	/* Setup interface */
	if (ovpn_setup_iface(iface, if_type, 1, unit, OVPN_TYPE_SERVER)) {
		stop_ovpn_server(unit);
		return;
	}

	/* Build and write config files */
	memset(buffer, 0, BUF_SIZE);
	snprintf(buffer, sizeof(buffer), OVPN_DIR"/server%d/config.ovpn", unit);
	fp = fopen(buffer, "w");
	chmod(buffer, (S_IRUSR | S_IWUSR));

	memset(buffer, 0, BUF_SIZE);
	snprintf(buffer, sizeof(buffer), "vpn_server%d_port", unit);
	fprintf(fp, "# Generated Configuration\n"
	            "daemon openvpn-server%d\n"
	            "port %d\n"
	            "dev %s\n"
	            "txqueuelen 1000\n"
	            "keepalive 15 60\n"
	            "verb 3\n",
	            unit,
	            nvram_get_int(buffer),
	            iface);

#ifndef TCONFIG_OPTIMIZE_SIZE_MORE
	if (auth_mode == OVPN_AUTH_TLS) {
		if (if_type == OVPN_IF_TUN) {
			fprintf(fp, "topology subnet\n"
			            "server %s %s\n",
			            getNVRAMVar("vpn_server%d_sn", unit),
			            getNVRAMVar("vpn_server%d_nm", unit));
		}
		else if (if_type == OVPN_IF_TAP) {
			fprintf(fp, "server-bridge");
			memset(buffer, 0, BUF_SIZE);
			snprintf(buffer, sizeof(buffer), "vpn_server%d_dhcp", unit);
			if (nvram_get_int(buffer) == 0) {
				memset(buffer, 0, BUF_SIZE);
				snprintf(buffer, sizeof(buffer), "vpn_server%d_br", unit);
				if (nvram_contains_word(buffer, "br1")) {
					br_ipaddr = nvram_get("lan1_ipaddr");
					br_netmask = nvram_get("lan1_netmask");
				}
				else if (nvram_contains_word(buffer, "br2")) {
					br_ipaddr = nvram_get("lan2_ipaddr");
					br_netmask = nvram_get("lan2_netmask");
				}
				else if (nvram_contains_word(buffer, "br3")) {
					br_ipaddr = nvram_get("lan3_ipaddr");
					br_netmask = nvram_get("lan3_netmask");
				}
				else {
					br_ipaddr = nvram_get("lan_ipaddr");
					br_netmask = nvram_get("lan_netmask");
				}

				fprintf(fp, " %s %s %s %s",
				            br_ipaddr, br_netmask,
				            getNVRAMVar("vpn_server%d_r1", unit),
				            getNVRAMVar("vpn_server%d_r2", unit));
			}
			else {
				fprintf(fp, "\npush \"route 0.0.0.0 255.255.255.255 net_gateway\"");
			}
			fprintf(fp, "\n");
		}
	}
	else
#endif
	     if (auth_mode == OVPN_AUTH_STATIC) {
		if (if_type == OVPN_IF_TUN) {
			fprintf(fp, "ifconfig %s ", getNVRAMVar("vpn_server%d_local", unit));
			fprintf(fp, "%s\n", getNVRAMVar("vpn_server%d_remote", unit));
		}
	}

	/* Proto */
	mwan_num = nvram_get_int("mwan_num"); /* check active WANs num */
	if (mwan_num < 1)
		mwan_num = 1;

	memset(buffer, 0, BUF_SIZE);
	snprintf(buffer, sizeof(buffer), "vpn_server%d_proto", unit);
	fprintf(fp, "proto %s\n", nvram_safe_get(buffer)); /* full dual-stack functionality starting with OpenVPN 2.4.0 */

	if (nvram_contains_word(buffer, "udp") && mwan_num > 1) /* udp/udp4/udp6 - only if multiwan */
		fprintf(fp, "multihome\n");

	/* Cipher */
	strlcpy(buffer, getNVRAMVar("vpn_server%d_ncp_ciphers", unit), sizeof(buffer));
#ifndef TCONFIG_OPTIMIZE_SIZE_MORE
	if (auth_mode == OVPN_AUTH_TLS) {
		if (buffer[0] != '\0')
			fprintf(fp, "data-ciphers %s\n", buffer);
	}
	else
#endif
	     {	/* SECRET/CUSTOM */
		memset(buffer, 0, BUF_SIZE);
		snprintf(buffer, sizeof(buffer), "vpn_server%d_cipher", unit);
		if (!nvram_contains_word(buffer, "default"))
			fprintf(fp, "cipher %s\n", nvram_safe_get(buffer));
	}

	/* Digest */
	memset(buffer, 0, BUF_SIZE);
	snprintf(buffer, sizeof(buffer), "vpn_server%d_digest", unit);
	if (!nvram_contains_word(buffer, "default"))
		fprintf(fp, "auth %s\n", nvram_safe_get(buffer));

	/* Compression */
	memset(buffer, 0, BUF_SIZE);
	strlcpy(buffer, getNVRAMVar("vpn_server%d_comp", unit), sizeof(buffer));
	if (strcmp(buffer, "-1")) {
#ifndef TCONFIG_OPTIMIZE_SIZE_MORE
		if (!strcmp(buffer, "lz4") || !strcmp(buffer, "lz4-v2"))
			fprintf(fp, "compress %s\n", buffer);
		else
#endif
		     if (!strcmp(buffer, "yes"))
			fprintf(fp, "compress lzo\n");
		else if (!strcmp(buffer, "adaptive"))
			fprintf(fp, "comp-lzo adaptive\n");
		else if (!strcmp(buffer, "no"))
			fprintf(fp, "compress\n");	/* Disable, but client can override if desired */
	}

	if ((nvl = atol(getNVRAMVar("vpn_server%d_reneg", unit))) >= 0)
		fprintf(fp, "reneg-sec %ld\n", nvl);

#ifndef TCONFIG_OPTIMIZE_SIZE_MORE
	if (auth_mode == OVPN_AUTH_TLS) {
		if (if_type == OVPN_IF_TUN) {
			/* push LANs */
			for (i = 0; i < BRIDGE_COUNT; i++) {
				memset(buffer, 0, BUF_SIZE);
				snprintf(buffer, sizeof(buffer), (i == 0 ? "vpn_server%d_plan" : "vpn_server%d_plan%d"), unit, i);
				if (nvram_get_int(buffer)) {
					int ret3 = 0, ret4 = 0;

					ret3 = sscanf(getNVRAMVar((i == 0 ? "lan_ipaddr" : "lan%d_ipaddr"), i), "%d.%d.%d.%d", &ip[0], &ip[1], &ip[2], &ip[3]);
					ret4 = sscanf(getNVRAMVar((i == 0 ? "lan_netmask" : "lan%d_netmask"), i), "%d.%d.%d.%d", &nm[0], &nm[1], &nm[2], &nm[3]);
					if (ret3 == 4 && ret4 == 4) {
						fprintf(fp, "push \"route %d.%d.%d.%d %s\"\n", ip[0]&nm[0], ip[1]&nm[1], ip[2]&nm[2], ip[3]&nm[3], getNVRAMVar((i == 0 ? "lan_netmask" : "lan%d_netmask"), i));
						push_lan[i] = 1; /* IPv4 LANX will be pushed */
					}
				}
			}
		}

		memset(buffer, 0, BUF_SIZE);
		snprintf(buffer, sizeof(buffer), "vpn_server%d_ccd", unit);
		if (nvram_get_int(buffer)) {
			fprintf(fp, "client-config-dir ccd\n");

			memset(buffer, 0, BUF_SIZE);
			snprintf(buffer, sizeof(buffer), "vpn_server%d_c2c", unit);
			if ((c2c = nvram_get_int(buffer)))
				fprintf(fp, "client-to-client\n");

			memset(buffer, 0, BUF_SIZE);
			snprintf(buffer, sizeof(buffer), "vpn_server%d_ccd_excl", unit);
			if (nvram_get_int(buffer))
				fprintf(fp, "ccd-exclusive\n");

			memset(buffer, 0, BUF_SIZE);
			snprintf(buffer, sizeof(buffer), OVPN_DIR"/server%d/ccd", unit);
			mkdir(buffer, 0700);
			chdir(buffer);

			memset(buffer, 0, BUF_SIZE);
			snprintf(buffer, sizeof(buffer), "vpn_server%d_ccd_val", unit);
			strlcpy(buffer, nvram_safe_get(buffer), sizeof(buffer));
			chp = strtok(buffer, ">");
			while (chp != NULL) {
				nvi = strlen(chp);

				chp[strcspn(chp, "<")] = '\0';
				logmsg(LOG_DEBUG, "*** %s: CCD: enabled: %d", __FUNCTION__, atoi(chp));
				if (atoi(chp) == 1) {
					nvi -= strlen(chp)+1;
					chp += strlen(chp)+1;

					ccd = NULL;
					route = NULL;
					if (nvi > 0) {
						chp[strcspn(chp, "<")] = '\0';
						logmsg(LOG_DEBUG, "*** %s: CCD: Common name: %s", __FUNCTION__, chp);
						ccd = fopen(chp, "a");
						chmod(chp, (S_IRUSR | S_IWUSR));

						nvi -= strlen(chp) + 1;
						chp += strlen(chp) + 1;
					}
					if ((nvi > 0) && (ccd != NULL) && (strcspn(chp, "<") != strlen(chp))) {
						chp[strcspn(chp, "<")] = ' ';
						chp[strcspn(chp, "<")] = '\0';
						route = chp;
						logmsg(LOG_DEBUG, "*** %s: CCD: Route: %s", __FUNCTION__, chp);
						if (strlen(route) > 1) {
							fprintf(ccd, "iroute %s\n", route);
							fprintf(fp, "route %s\n", route);
						}

						nvi -= strlen(chp) + 1;
						chp += strlen(chp) + 1;
					}
					if (ccd != NULL)
						fclose(ccd);
					if ((nvi > 0) && (route != NULL)) {
						chp[strcspn(chp, "<")] = '\0';
						logmsg(LOG_DEBUG, "*** %s: CCD: Push: %d", __FUNCTION__, atoi(chp));
						if (c2c && atoi(chp) == 1 && strlen(route) > 1)
							fprintf(fp, "push \"route %s\"\n", route);

						nvi -= strlen(chp)+1;
						chp += strlen(chp)+1;
					}
					logmsg(LOG_DEBUG, "*** %s: CCD leftover: %d", __FUNCTION__, nvi + 1);
				}
				/* Advance to next entry */
				chp = strtok(NULL, ">");
			}
			logmsg(LOG_DEBUG, "*** %s: CCD processing complete", __FUNCTION__);
		}

		memset(buffer, 0, BUF_SIZE);
		snprintf(buffer, sizeof(buffer), "vpn_server%d_userpass", unit);
		if (nvram_get_int(buffer)) {
			fprintf(fp, "plugin /lib/openvpn_plugin_auth_nvram.so vpn_server%d_users_val\n"
			            "script-security 2\n",
			            unit);

			memset(buffer, 0, BUF_SIZE);
			snprintf(buffer, sizeof(buffer), "vpn_server%d_nocert", unit);
			if (nvram_get_int(buffer)) {
				fprintf(fp, "verify-client-cert optional\n"
				            "username-as-common-name\n");
			}
		}

		memset(buffer, 0, BUF_SIZE);
		snprintf(buffer, sizeof(buffer), "vpn_server%d_pdns", unit);
		if (nvram_get_int(buffer)) {
			if (nvram_safe_get("wan_domain")[0] != '\0')
				fprintf(fp, "push \"dhcp-option DOMAIN %s\"\n", nvram_safe_get("wan_domain"));
			if ((nvram_safe_get("wan_wins")[0] != '\0' && strcmp(nvram_safe_get("wan_wins"), "0.0.0.0") != 0))
				fprintf(fp, "push \"dhcp-option WINS %s\"\n", nvram_safe_get("wan_wins"));

			/* check if LANX will be pushed --> if YES, push the suitable DNS Server address */
			for (i = 0; i < BRIDGE_COUNT; i++) {
				if (push_lan[i] == 1) { /* push IPv4 LANx DNS */
					memset(buffer, 0, BUF_SIZE);
					snprintf(buffer, sizeof(buffer), (i == 0 ? "lan_ipaddr" : "lan%d_ipaddr"), i);
					fprintf(fp, "push \"dhcp-option DNS %s\"\n", nvram_safe_get(buffer));
					dont_push_active = 1;
				}
			}
			/* no LANx will be pushed, push only one active DNS */
			/* check what LAN is active before push DNS */
			if (dont_push_active == 0) {
				for (i = 0; i < BRIDGE_COUNT; i++) {
					memset(buffer, 0, BUF_SIZE);
					snprintf(buffer, sizeof(buffer), (i == 0 ? "lan_ipaddr" : "lan%d_ipaddr"), i);
					if (strcmp(nvram_safe_get(buffer), "") != 0) {
						fprintf(fp, "push \"dhcp-option DNS %s\"\n", nvram_safe_get(buffer));
						break;
					}
				}
			}
		}

		memset(buffer, 0, BUF_SIZE);
		snprintf(buffer, sizeof(buffer), "vpn_server%d_rgw", unit);
		if (nvram_get_int(buffer)) {
			if (if_type == OVPN_IF_TAP)
				fprintf(fp, "push \"route-gateway %s\"\n", nvram_safe_get("lan_ipaddr"));
			fprintf(fp, "push \"redirect-gateway def1\"\n");
		}

		nvi = atoi(getNVRAMVar("vpn_server%d_hmac", unit));
		memset(buffer, 0, BUF_SIZE);
		snprintf(buffer, sizeof(buffer), "vpn_server%d_static", unit);
		if (!nvram_is_empty(buffer) && nvi >= 0) {
			if (nvi == 3)
				fprintf(fp, "tls-crypt static.key");
			else if (nvi == 4)
				fprintf(fp, "tls-crypt-v2 static.key");
			else
				fprintf(fp, "tls-auth static.key");

			if (nvi < 2)
				fprintf(fp, " %d", nvi);
			fprintf(fp, "\n");
		}

		memset(buffer, 0, BUF_SIZE);
		snprintf(buffer, sizeof(buffer), "vpn_server%d_ca", unit);
		if (!nvram_is_empty(buffer))
			fprintf(fp, "ca ca.crt\n");
		memset(buffer, 0, BUF_SIZE);
		snprintf(buffer, sizeof(buffer), "vpn_server%d_dh", unit);
		if (!nvram_is_empty(buffer))
			fprintf(fp, "dh dh.pem\n");
		memset(buffer, 0, BUF_SIZE);
		snprintf(buffer, sizeof(buffer), "vpn_server%d_crt", unit);
		if (!nvram_is_empty(buffer))
			fprintf(fp, "cert server.crt\n");
		memset(buffer, 0, BUF_SIZE);
		snprintf(buffer, sizeof(buffer), "vpn_server%d_crl", unit);
		if (!nvram_is_empty(buffer))
			fprintf(fp, "crl-verify crl.pem\n");
		memset(buffer, 0, BUF_SIZE);
		snprintf(buffer, sizeof(buffer), "vpn_server%d_key", unit);
		if (!nvram_is_empty(buffer))
			fprintf(fp, "key server.key\n");
	}
	else
#endif
	     if (auth_mode == OVPN_AUTH_STATIC) {
		memset(buffer, 0, BUF_SIZE);
		snprintf(buffer, sizeof(buffer), "vpn_server%d_static", unit);
		if (!nvram_is_empty(buffer))
			fprintf(fp, "secret static.key\n");
	}
	fprintf(fp, "status-version 2\n"
	            "status status 10\n\n" /* Update status file every 10 sec */
	            "# Custom Configuration\n"
	            "%s",
	            getNVRAMVar("vpn_server%d_custom", unit));

	fclose(fp);

	/* Write certification and key files */
#ifndef TCONFIG_OPTIMIZE_SIZE_MORE
	if (auth_mode == OVPN_AUTH_TLS) {
		memset(buffer, 0, BUF_SIZE);
		snprintf(buffer, sizeof(buffer), "vpn_server%d_ca", unit);
		if (!nvram_is_empty(buffer)) {
			memset(buffer, 0, BUF_SIZE);
			snprintf(buffer, sizeof(buffer), OVPN_DIR"/server%d/ca.crt", unit);
			fp = fopen(buffer, "w");
			chmod(buffer, (S_IRUSR | S_IWUSR));
			fprintf(fp, "%s", getNVRAMVar("vpn_server%d_ca", unit));
			fclose(fp);
		}

		memset(buffer, 0, BUF_SIZE);
		snprintf(buffer, sizeof(buffer), "vpn_server%d_key", unit);
		if (!nvram_is_empty(buffer)) {
			memset(buffer, 0, BUF_SIZE);
			snprintf(buffer, sizeof(buffer), OVPN_DIR"/server%d/server.key", unit);
			fp = fopen(buffer, "w");
			chmod(buffer, (S_IRUSR | S_IWUSR));
			fprintf(fp, "%s", getNVRAMVar("vpn_server%d_key", unit));
			fclose(fp);
		}

		memset(buffer, 0, BUF_SIZE);
		snprintf(buffer, sizeof(buffer), "vpn_server%d_crt", unit);
		if (!nvram_is_empty(buffer)) {
			memset(buffer, 0, BUF_SIZE);
			snprintf(buffer, sizeof(buffer), OVPN_DIR"/server%d/server.crt", unit);
			fp = fopen(buffer, "w");
			chmod(buffer, (S_IRUSR | S_IWUSR));
			fprintf(fp, "%s", getNVRAMVar("vpn_server%d_crt", unit));
			fclose(fp);
		}

		memset(buffer, 0, BUF_SIZE);
		snprintf(buffer, sizeof(buffer), "vpn_server%d_crl", unit);
		if (!nvram_is_empty(buffer)) {
			memset(buffer, 0, BUF_SIZE);
			snprintf(buffer, sizeof(buffer), OVPN_DIR"/server%d/crl.pem", unit);
			fp = fopen(buffer, "w");
			chmod(buffer, (S_IRUSR | S_IWUSR));
			fprintf(fp, "%s", getNVRAMVar("vpn_server%d_crl", unit));
			fclose(fp);
		}

		memset(buffer, 0, BUF_SIZE);
		snprintf(buffer, sizeof(buffer), "vpn_server%d_dh", unit);
		if (!nvram_is_empty(buffer)) {
			memset(buffer, 0, BUF_SIZE);
			snprintf(buffer, sizeof(buffer), OVPN_DIR"/server%d/dh.pem", unit);
			fp = fopen(buffer, "w");
			chmod(buffer, (S_IRUSR | S_IWUSR));
			fprintf(fp, "%s", getNVRAMVar("vpn_server%d_dh", unit));
			fclose(fp);
		}
	}
#endif
	memset(buffer, 0, BUF_SIZE);
	snprintf(buffer, sizeof(buffer), "vpn_server%d_hmac", unit);
	if ((auth_mode == OVPN_AUTH_STATIC) || (auth_mode == OVPN_AUTH_TLS && nvram_get_int(buffer) >= 0)) {
		memset(buffer, 0, BUF_SIZE);
		snprintf(buffer, sizeof(buffer), "vpn_server%d_static", unit);
		if (!nvram_is_empty(buffer)) {
			memset(buffer, 0, BUF_SIZE);
			snprintf(buffer, sizeof(buffer), OVPN_DIR"/server%d/static.key", unit);
			fp = fopen(buffer, "w");
			chmod(buffer, (S_IRUSR | S_IWUSR));
			fprintf(fp, "%s", getNVRAMVar("vpn_server%d_static", unit));
			fclose(fp);
		}
	}

	/* Handle firewall rules if appropriate */
	memset(buffer, 0, BUF_SIZE);
	snprintf(buffer, sizeof(buffer), "vpn_server%d_firewall", unit);
	if (!nvram_contains_word(buffer, "custom")) {
		chains_log_detection();

		/* Create firewall rules */
		mkdir(OVPN_DIR"/fw", 0700);
		memset(buffer, 0, BUF_SIZE);
		snprintf(buffer, sizeof(buffer), OVPN_DIR"/fw/server%d-fw.sh", unit);
		fp = fopen(buffer, "w");
		chmod(buffer, (S_IRUSR | S_IWUSR | S_IXUSR));
		fprintf(fp, "#!/bin/sh\n");
		memset(buffer, 0, BUF_SIZE);
		strncpy(buffer, getNVRAMVar("vpn_server%d_proto", unit), BUF_SIZE);

		memset(buffer2, 0, sizeof(buffer2));
		if ((!strcmp(buffer, "udp")) || (!strcmp(buffer, "udp4")) || (!strcmp(buffer, "udp6")))
			snprintf(buffer2, sizeof(buffer2), "udp");
		else
			snprintf(buffer2, sizeof(buffer2), "tcp");

		fprintf(fp, "iptables -t nat -I PREROUTING -p %s ", buffer2);
		fprintf(fp, "--dport %d -j ACCEPT\n", atoi(getNVRAMVar("vpn_server%d_port", unit)));
		memset(buffer, 0, BUF_SIZE);
		strncpy(buffer, getNVRAMVar("vpn_server%d_proto", unit), BUF_SIZE);
		fprintf(fp, "iptables -I INPUT -p %s ", buffer2);
		memset(buffer, 0, BUF_SIZE);
		snprintf(buffer, sizeof(buffer), "vpn_server%d_port", unit);
		fprintf(fp, "--dport %d -j %s\n", nvram_get_int(buffer), chain_in_accept);
		memset(buffer, 0, BUF_SIZE);
		snprintf(buffer, sizeof(buffer), "vpn_server%d_firewall", unit);
		if (!nvram_contains_word(buffer, "external")) {
			fprintf(fp, "iptables -I INPUT -i %s -j %s\n"
			            "iptables -I FORWARD -i %s -j ACCEPT\n",
			            iface, chain_in_accept,
			            iface);
#ifdef TCONFIG_BCMARM
			if (!nvram_get_int("ctf_disable")) /* bypass CTF if enabled */
				fprintf(fp, "iptables -t mangle -I PREROUTING -i %s -j MARK --set-mark 0x01/0x7\n", iface);
#endif /* TCONFIG_BCMARM */
		}

		/* Create firewall rules for IPv6 */
#ifdef TCONFIG_IPV6
		if (ipv6_enabled()) {
			strncpy(buffer, getNVRAMVar("vpn_server%d_proto", unit), BUF_SIZE);
			fprintf(fp, "ip6tables -I INPUT -p %s ", buffer2);
			memset(buffer, 0, BUF_SIZE);
			snprintf(buffer, sizeof(buffer), "vpn_server%d_port", unit);
			fprintf(fp, "--dport %d -j %s\n", nvram_get_int(buffer), chain_in_accept);
			memset(buffer, 0, BUF_SIZE);
			snprintf(buffer, sizeof(buffer), "vpn_server%d_firewall", unit);
			if (!nvram_contains_word(buffer, "external")) {
				fprintf(fp, "ip6tables -I INPUT -i %s -j %s\n"
				            "ip6tables -I FORWARD -i %s -j ACCEPT\n",
				            iface, chain_in_accept,
				            iface);
			}
		}
#endif

		fclose(fp);

		/* firewall rules */
		memset(buffer, 0, BUF_SIZE);
		snprintf(buffer, sizeof(buffer), OVPN_DIR"/fw/server%d-fw.sh", unit);

		/* first remove existing firewall rule(s) */
		run_del_firewall_script(buffer, OVPN_DIR_DEL_SCRIPT);

		/* then add firewall rule(s) */
		eval(buffer);
	}

	/* Start the VPN server */
	memset(buffer, 0, BUF_SIZE);
	snprintf(buffer, sizeof(buffer), OVPN_DIR"/vpnserver%d", unit);
	memset(buffer2, 0, sizeof(buffer2));
	snprintf(buffer2, sizeof(buffer2), OVPN_DIR"/server%d", unit);

#if defined(TCONFIG_BCMARM) && defined(TCONFIG_BCMSMP)
	/* Spread servers on cpu 1,0 or 1,2 (in that order) */
	cpu_num = sysconf(_SC_NPROCESSORS_CONF) - 1;
	if (cpu_num < 0)
		cpu_num = 0;

	snprintf(cpulist, sizeof(cpulist), "%d", (unit & cpu_num));
	taskset_ret = cpu_eval(NULL, cpulist, buffer, "--cd", buffer2, "--config", "config.ovpn");

	if (taskset_ret)
#endif
	taskset_ret = eval(buffer, "--cd", buffer2, "--config", "config.ovpn");

	if (taskset_ret) {
		logmsg(LOG_WARNING, "starting OpenVPN server%d failed - check configuration ...", unit);
		stop_ovpn_server(unit);
		return;
	}

	/* Set up cron job */
	ovpn_setup_watchdog(OVPN_TYPE_SERVER, unit);

	memset(buffer, 0, BUF_SIZE);
	snprintf(buffer, sizeof(buffer), "vpn_server%d", unit);
	allow_fastnat(buffer, 0);
	try_enabling_fastnat();
}

void stop_ovpn_server(int unit)
{
	char buffer[BUF_SIZE];

	memset(buffer, 0, BUF_SIZE);
	snprintf(buffer, sizeof(buffer), "vpnserver%d", unit);
	if (serialize_restart(buffer, 0))
		return;

	/* Remove cron job */
	memset(buffer, 0, BUF_SIZE);
	snprintf(buffer, sizeof(buffer), "CheckVPNserver%d", unit);
	eval("cru", "d", buffer);

	/* Stop the VPN server */
	memset(buffer, 0, BUF_SIZE);
	snprintf(buffer, sizeof(buffer), "vpnserver%d", unit);
	killall_and_waitfor(buffer, 5, 50);

	ovpn_remove_iface(OVPN_TYPE_SERVER, unit);

	/* Remove firewall rules */
	memset(buffer, 0, BUF_SIZE);
	snprintf(buffer, sizeof(buffer), OVPN_DIR"/fw/server%d-fw.sh", unit);
	run_del_firewall_script(buffer, OVPN_DIR_DEL_SCRIPT);

	/* Delete all files for this server */
	ovpn_cleanup_dirs(OVPN_TYPE_SERVER, unit);

	memset(buffer, 0, BUF_SIZE);
	snprintf(buffer, sizeof(buffer), "vpn_server%d", unit);
	allow_fastnat(buffer, 1);
	try_enabling_fastnat();
}

void start_ovpn_eas()
{
	char buffer[16], *cur;
	int nums[OVPN_CLIENT_MAX], i;

	if ((strlen(nvram_safe_get("vpn_server_eas")) == 0) && (strlen(nvram_safe_get("vpn_client_eas")) == 0))
		return;

	/* Parse and start servers */
	strlcpy(buffer, nvram_safe_get("vpn_server_eas"), sizeof(buffer));

	i = 0;
	for (cur = strtok(buffer, ","); (cur != NULL) && (i <= OVPN_SERVER_MAX); cur = strtok(NULL, ","))
		nums[i++] = atoi(cur);

	nums[i] = 0;
	for (i = 0; (nums[i] > 0) && (nums[i] <= OVPN_SERVER_MAX); i++) {
		memset(buffer, 0, sizeof(buffer));
		snprintf(buffer, sizeof(buffer), "vpnserver%d", nums[i]);

		if (pidof(buffer) > 0)
			stop_ovpn_server(nums[i]);

		start_ovpn_server(nums[i]);
	}

	/* Parse and start clients */
	strlcpy(buffer, nvram_safe_get("vpn_client_eas"), sizeof(buffer));

	i = 0;
	for (cur = strtok(buffer, ","); (cur != NULL) && (i <= OVPN_CLIENT_MAX); cur = strtok(NULL, ","))
		nums[i++] = atoi(cur);

	nums[i] = 0;
	for (i = 0; (nums[i] > 0) && (nums[i] <= OVPN_CLIENT_MAX); i++) {
		memset(buffer, 0, sizeof(buffer));
		snprintf(buffer, sizeof(buffer), "vpnclient%d", nums[i]);

		if (pidof(buffer) > 0)
			stop_ovpn_client(nums[i]);

		start_ovpn_client(nums[i]);
	}
}

void stop_ovpn_eas()
{
	char buffer[16], *cur;
	int nums[OVPN_CLIENT_MAX], i;

	/* Parse and stop servers */
	strlcpy(buffer, nvram_safe_get("vpn_server_eas"), sizeof(buffer));

	i = 0;
	for (cur = strtok(buffer, ","); (cur != NULL) && (i <= OVPN_SERVER_MAX); cur = strtok(NULL, ","))
		nums[i++] = atoi(cur);

	nums[i] = 0;
	for (i = 0; (nums[i] > 0) && (nums[i] <= OVPN_SERVER_MAX); i++) {
		memset(buffer, 0, sizeof(buffer));
		snprintf(buffer, sizeof(buffer), "vpnserver%d", nums[i]);

		if (pidof(buffer) > 0)
			stop_ovpn_server(nums[i]);
	}

	/* Parse and stop clients */
	strlcpy(buffer, nvram_safe_get("vpn_client_eas"), sizeof(buffer));

	i = 0;
	for (cur = strtok(buffer, ","); (cur != NULL) && (i <= OVPN_CLIENT_MAX); cur = strtok(NULL, ","))
		nums[i++] = atoi(cur);

	nums[i] = 0;
	for (i = 0; (nums[i] > 0) && (nums[i] <= OVPN_CLIENT_MAX); i++) {
		memset(buffer, 0, sizeof(buffer));
		snprintf(buffer, sizeof(buffer), "vpnclient%d", nums[i]);

		if (pidof(buffer) > 0)
			stop_ovpn_client(nums[i]);
	}
}

void stop_ovpn_all()
{
	char buffer[16];
	int i;

	/* Stop servers */
	for (i = 1; i <= OVPN_SERVER_MAX; i++) {
		memset(buffer, 0, sizeof(buffer));
		snprintf(buffer, sizeof(buffer), "vpnserver%d", i);
		if (pidof(buffer) > 0)
			stop_ovpn_server(i);
	}

	/* Stop clients */
	for (i = 1; i <= OVPN_CLIENT_MAX; i++) {
		memset(buffer, 0, sizeof(buffer));
		snprintf(buffer, sizeof(buffer), "vpnclient%d", i);
		if (pidof(buffer) > 0)
			stop_ovpn_client(i);
	}

	/* Remove tunnel interface module */
	modprobe_r("tun");
}

void run_ovpn_firewall_scripts(void)
{
	DIR *dir;
	struct stat fs;
	struct dirent *file;
	char *fa;
	char buf[64];

	ovpn_kill_switch();

	if (chdir(OVPN_DIR"/fw"))
		return;

	dir = opendir(OVPN_DIR"/fw");

	logmsg(LOG_DEBUG, "*** %s: beginning all firewall scripts...", __FUNCTION__);

	while ((file = readdir(dir)) != NULL) {
		fa = file->d_name;

		if ((fa[0] == '.') || (strcmp(fa, OVPN_DEL_SCRIPT) == 0))
			continue;

		memset(buf, 0, sizeof(buf));
		snprintf(buf, sizeof(buf), "%s/fw/", OVPN_DIR);
		strlcat(buf, fa, sizeof(buf));

		/* check exe permission (in case vpnrouting.sh is still working on routing file) */
		stat(buf, &fs);
		if (fs.st_mode & S_IXUSR) {
			/* first remove existing firewall rule(s) */
			run_del_firewall_script(buf, OVPN_DIR_DEL_SCRIPT);

			/* then (re-)add firewall rule(s) */
			logmsg(LOG_DEBUG, "*** %s: running firewall script: %s", __FUNCTION__, buf);
			eval(buf);
		}
		else
			logmsg(LOG_DEBUG, "*** %s: skipping firewall script (not executable): %s", __FUNCTION__, buf);
	}
	logmsg(LOG_DEBUG, "*** %s: done with all firewall scripts...", __FUNCTION__);

	closedir(dir);
}

void write_ovpn_dnsmasq_config(FILE* f)
{
	char nv[16];
	char buf[24];
	char *pos, *fn, ch;
	int cur;
	DIR *dir;
	struct dirent *file;

	strlcpy(buf, nvram_safe_get("vpn_server_dns"), sizeof(buf));
	for (pos = strtok(buf, ","); pos != NULL; pos = strtok(NULL, ",")) {
		cur = atoi(pos);
		if (cur) {
			logmsg(LOG_DEBUG, "*** %s: adding server %d interface to dns config", __FUNCTION__, cur);
			snprintf(nv, sizeof(nv), "vpn_server%d_if", cur);
			fprintf(f, "interface=%s%d\n", nvram_safe_get(nv), (OVPN_SERVER_BASEIF + cur));
		}
	}

	if ((dir = opendir(OVPN_DIR"/dns")) != NULL) {
		while ((file = readdir(dir)) != NULL) {
			fn = file->d_name;

			if (fn[0] == '.')
				continue;

			if (sscanf(fn, "client%d.resol%c", &cur, &ch) == 2) {
				logmsg(LOG_DEBUG, "*** %s: checking ADNS settings for client %d", __FUNCTION__, cur);
				snprintf(buf, sizeof(buf), "vpn_client%d_adns", cur);
				if (nvram_get_int(buf) == 2) {
					logmsg(LOG_INFO, "adding strict-order to dnsmasq config for client %d", cur);
					fprintf(f, "strict-order\n");
					break;
				}
			}

			if (sscanf(fn, "client%d.con%c", &cur, &ch) == 2) {
				logmsg(LOG_INFO, "adding Dnsmasq config from %s", fn);
				fappend(f, fn);
			}
		}
		closedir(dir);
	}
}

int write_ovpn_resolv(FILE* f)
{
	DIR *dir;
	struct dirent *file;
	char *fn, ch, num, buf[24];
	FILE *dnsf;
	int exclusive = 0;
	int adns = 0;

	if (chdir(OVPN_DIR"/dns"))
		return 0;

	dir = opendir(OVPN_DIR"/dns");

	while ((file = readdir(dir)) != NULL) {
		fn = file->d_name;

		if (fn[0] == '.')
			continue;

		if (sscanf(fn, "client%c.resol%c", &num, &ch) == 2) {
			snprintf(buf, sizeof(buf), "vpn_client%c_adns", num);
			adns = nvram_get_int(buf);
			if ((dnsf = fopen(fn, "r")) == NULL)
				continue;

			logmsg(LOG_INFO, "adding DNS entries from %s", fn);
			fappend(f, fn);

			if (adns == 3)
				exclusive = 1;
		}
	}
	closedir(dir);

	return exclusive;
}
