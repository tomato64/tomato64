/*
 *
 * Copyright (C) 2008-2010 Keith Moyer, tomatovpn@keithmoyer.com
 *
 * No part of this file may be used without permission.
 *
 * Fixes/updates (C) 2018 - 2026 pedro
 * https://freshtomato.org/
 *
 */


#include "rc.h"

#include <sys/types.h>
#ifdef TOMATO64
#include <sys/stat.h>
#endif /* TOMATO64 */

#define OVPN_CLIENT_BASEIF	10
#define OVPN_SERVER_BASEIF	20

#define IF_SIZE			8

/* needed by logmsg() */
#define LOGMSG_DISABLE	DISABLE_SYSLOG_OSM
#define LOGMSG_NVDEBUG	"openvpn_debug"

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
	char buffer[BUF_SIZE_16];

	memset(buffer, 0, BUF_SIZE_16);
	snprintf(buffer, BUF_SIZE_16, "vpn%s%d_br", (type == OVPN_TYPE_SERVER ? "s" : "c"), unit);

	/* Make sure module is loaded */
	modprobe("tun");
	f_wait_exists("/dev/net/tun", 5);

	/* Create tap/tun interface */
	if (eval("openvpn", "--mktun", "--dev", iface)) {
		logmsg(LOG_WARNING, "unable to create tunnel interface %s (%s)!", iface, strerror(errno));
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
	char buffer[BUF_SIZE_8];
	int tmp = (type == OVPN_TYPE_CLIENT ? OVPN_CLIENT_BASEIF : OVPN_SERVER_BASEIF) + unit;

	/* NVRAM setting for device type could have changed, just try to remove both */
	memset(buffer, 0, BUF_SIZE_8);
	snprintf(buffer, BUF_SIZE_8, "tap%d", tmp);
	eval("openvpn", "--rmtun", "--dev", buffer);

	memset(buffer, 0, BUF_SIZE_8);
	snprintf(buffer, BUF_SIZE_8, "tun%d", tmp);
	eval("openvpn", "--rmtun", "--dev", buffer);
}

static void ovpn_setup_dirs(ovpn_type_t type, int unit) {
	char buffer[BUF_SIZE_64];
	char *tmp = (type == OVPN_TYPE_SERVER ? "server" : "client");

	mkdir(OVPN_DIR, 0700);
	memset(buffer, 0, BUF_SIZE_64);
	snprintf(buffer, BUF_SIZE_64, OVPN_DIR"/%s%d", tmp, unit);
	mkdir(buffer, 0700);

	memset(buffer, 0, BUF_SIZE_64);
	snprintf(buffer, BUF_SIZE_64, OVPN_DIR"/vpn%s%d", tmp, unit);
	unlink(buffer);
	symlink("/usr/sbin/openvpn", buffer);

	if (type == OVPN_TYPE_CLIENT) {
		memset(buffer, 0, BUF_SIZE_64);
		snprintf(buffer, BUF_SIZE_64, OVPN_DIR"/client%d/updown-client.sh", unit);
		symlink("/usr/sbin/updown-client.sh", buffer);

		memset(buffer, 0, BUF_SIZE_64);
		snprintf(buffer, BUF_SIZE_64, OVPN_DIR"/client%d/vpnrouting.sh", unit);
		symlink("/usr/sbin/vpnrouting.sh", buffer);
	}
}

static void ovpn_cleanup_dirs(ovpn_type_t type, int unit) {
	char buffer[BUF_SIZE_64];
	char *tmp = (type == OVPN_TYPE_SERVER ? "server" : "client");

	memset(buffer, 0, BUF_SIZE_64);
	snprintf(buffer, BUF_SIZE_64, OVPN_DIR"/%s%d", tmp, unit);
	eval("rm", "-rf", buffer);

	memset(buffer, 0, BUF_SIZE_64);
	snprintf(buffer, BUF_SIZE_64, OVPN_DIR"/vpn%s%d", tmp, unit);
	eval("rm", "-rf", buffer);

	memset(buffer, 0, BUF_SIZE_64);
	snprintf(buffer, BUF_SIZE_64, OVPN_FW_DIR"/%s%d-fw.sh", tmp, unit);
	eval("rm", "-rf", buffer);

	if (type == OVPN_TYPE_CLIENT) {
		memset(buffer, 0, BUF_SIZE_64);
		snprintf(buffer, BUF_SIZE_64, OVPN_DNS_DIR"/client%d.resolv", unit);
		eval("rm", "-rf", buffer);
	}

	/* Attempt to remove directories. Will fail if not empty */
	rmdir(OVPN_FW_DIR);
	rmdir(OVPN_DIR);
}

static void ovpn_setup_watchdog(ovpn_type_t type, const int unit)
{
	FILE *fp;
	char buffer[BUF_SIZE_64], buffer2[BUF_SIZE_64];
	char taskname[BUF_SIZE_32];
	char *instanceType;
	int nvi;

	if (type == OVPN_TYPE_SERVER)
		instanceType = "server";
	else
		instanceType = "client";

	memset(buffer, 0, BUF_SIZE_64);
	snprintf(buffer, BUF_SIZE_64, "vpn%c%d_poll", *instanceType, unit); /* instanceType: 's' or 'c' only */
	if ((nvi = nvram_get_int(buffer)) > 0) {
		memset(buffer, 0, BUF_SIZE_64);
		snprintf(buffer, BUF_SIZE_64, "/etc/openvpn/%s%d/watchdog.sh", instanceType, unit);

		if ((fp = fopen(buffer, "w"))) {
			fprintf(fp, "#!/bin/sh\n"
			            "pingme() {\n"
			            "[ \"server\" = \"%s\" -o \"%d\" = \"0\" ] && return 0\n"
			            " local i=1\n"
			            " while :; do\n"
			            "  ping -qc1 -W3 -I %s%d %s &>/dev/null && return 0\n"
			            "  [ $((i++)) -ge 3 ] && break || sleep 5\n"
			            " done\n"
			            " return 1\n"
			            "}\n"
			            "[ \"$(nvram get g_upgrade)\" != \"1\" -a \"$(nvram get g_reboot)\" != \"1\" ] && {\n"
			            " pidof vpn%s%d &>/dev/null && pingme && exit 0\n"
			            " logger -t openvpn-watchdog vpn%s%d stopped? Starting...\n"
			            " service vpn%s%d restart\n"
			            "}\n",
			            instanceType, atoi(getNVRAMVar("vpnc%d_tchk", unit)),
			            getNVRAMVar("vpnc%d_if", unit), unit + (type == OVPN_TYPE_SERVER ? OVPN_SERVER_BASEIF : OVPN_CLIENT_BASEIF), nvram_safe_get("wan_checker"),
			            instanceType, unit,
			            instanceType, unit,
			            instanceType, unit);
			fclose(fp);
			chmod(buffer, (S_IRUSR | S_IWUSR | S_IXUSR));

			memset(taskname, 0, BUF_SIZE_32);
			snprintf(taskname, BUF_SIZE_32,"CheckVPN%s%d", instanceType, unit);
			memset(buffer2, 0, BUF_SIZE_64);
			snprintf(buffer2, BUF_SIZE_64, "*/%d * * * * %s", nvi, buffer);
			eval("cru", "a", taskname, buffer2);
		}
	}
}

void start_ovpn_client(int unit)
{
	FILE *fp;
	ovpn_auth_t auth_mode;
	ovpn_route_t route_mode = NONE;
	ovpn_if_t if_type;
	char iface[IF_SIZE];
	char buffer[BUF_SIZE];
	char buffer2[BUF_SIZE_32];
	int nvi;
	long int nvl;
	int userauth, useronly;
	int taskset_ret = 0;
#if defined(TCONFIG_BCMARM) && defined(TCONFIG_BCMSMP)
	char cpulist[2];
	int cpu_num = sysconf(_SC_NPROCESSORS_CONF) - 1;
	if (cpu_num < 0)
		cpu_num = 0;
#endif

	memset(buffer, 0, BUF_SIZE);
	snprintf(buffer, BUF_SIZE, "vpnc%d", unit);
	if (serialize_restart(buffer, 1))
		return;

	/* Determine interface */
	memset(buffer, 0, BUF_SIZE);
	snprintf(buffer, BUF_SIZE, "vpnc%d_if", unit);
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
	snprintf(buffer, BUF_SIZE, "vpnc%d_crypt", unit);
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
	if (if_type == OVPN_IF_TAP && atoi(getNVRAMVar("vpnc%d_bridge", unit)) == 1)
		route_mode = BRIDGE;

	/* Determine if we should NAT the tunnel */
	if (((if_type == OVPN_IF_TUN) || (route_mode != BRIDGE)) && atoi(getNVRAMVar("vpnc%d_nat", unit)) == 1)
		route_mode = NAT;

	/* Setup directories and symlinks */
	ovpn_setup_dirs(OVPN_TYPE_CLIENT, unit);

	/* Setup interface */
#ifdef TOMATO64
	/* DCO creates its own interface */
	if (!atoi(getNVRAMVar("vpnc%d_dco", unit))) {
#endif /* TOMATO64 */
	if (ovpn_setup_iface(iface, if_type, route_mode, unit, OVPN_TYPE_CLIENT)) {
		stop_ovpn_client(unit);
		return;
	}
#ifdef TOMATO64
	}
#endif /* TOMATO64 */

	userauth = atoi(getNVRAMVar("vpnc%d_userauth", unit));
	useronly = userauth && atoi(getNVRAMVar("vpnc%d_useronly", unit));

	/* Build and write config file */
	memset(buffer, 0, BUF_SIZE);
	snprintf(buffer, BUF_SIZE, OVPN_DIR"/client%d/config.ovpn", unit);
	fp = fopen(buffer, "w");
	if (!fp) {
		logmsg(LOG_ERR, "failed to create %s: (%s)", buffer, strerror(errno));
		stop_ovpn_client(unit);
		return;
	}
	chmod(buffer, (S_IRUSR | S_IWUSR));

	fprintf(fp, "# Generated Configuration\n"
	            "daemon openvpn-client%d\n"
	            "dev %s\n"
	            "txqueuelen 1000\n"
	            "persist-key\n"
	            "persist-tun\n",
	            unit,
	            iface);

#ifdef TOMATO64
	/* DCO - Data Channel Offload */
	if (atoi(getNVRAMVar("vpnc%d_dco", unit))) {
		modprobe("ovpn_dco_v2");
	}
	else {
		fprintf(fp, "disable-dco\n");
	}
#endif /* TOMATO64 */

	if (auth_mode == OVPN_AUTH_TLS)
		fprintf(fp, "client\n");

	fprintf(fp, "proto %s\n"
	            "remote %s "
	            "%d\n",
	            getNVRAMVar("vpnc%d_proto", unit),
	            getNVRAMVar("vpnc%d_addr", unit),
	            atoi(getNVRAMVar("vpnc%d_port", unit)));

	if (auth_mode == OVPN_AUTH_STATIC) {
		fprintf(fp, "ifconfig %s ", getNVRAMVar("vpnc%d_local", unit));

		if (if_type == OVPN_IF_TUN)
			fprintf(fp, "%s\n", getNVRAMVar("vpnc%d_remote", unit));
		else if (if_type == OVPN_IF_TAP)
			fprintf(fp, "%s\n", getNVRAMVar("vpnc%d_nm", unit));
	}

	if ((nvi = atoi(getNVRAMVar("vpnc%d_retry", unit))) >= 0)
		fprintf(fp, "resolv-retry %d\n", nvi);
	else
		fprintf(fp, "resolv-retry infinite\n");

	if ((nvl = atol(getNVRAMVar("vpnc%d_reneg", unit))) >= 0)
		fprintf(fp, "reneg-sec %ld\n", nvl);

	if (atoi(getNVRAMVar("vpnc%d_nobind", unit)) > 0)
		fprintf(fp, "nobind\n");

	/* Compression */
	memset(buffer, 0, BUF_SIZE);
	strlcpy(buffer, getNVRAMVar("vpnc%d_comp", unit), BUF_SIZE);
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
	strlcpy(buffer, getNVRAMVar("vpnc%d_ncp_ciphers", unit), BUF_SIZE);
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
		snprintf(buffer, BUF_SIZE, "vpnc%d_cipher", unit);
		if (!nvram_contains_word(buffer, "default"))
			fprintf(fp, "cipher %s\n", nvram_safe_get(buffer));
#ifndef TCONFIG_OPTIMIZE_SIZE_MORE
	}
#endif

	/* Digest */
	memset(buffer, 0, BUF_SIZE);
	snprintf(buffer, BUF_SIZE, "vpnc%d_digest", unit);
	if (!nvram_contains_word(buffer, "default"))
		fprintf(fp, "auth %s\n", nvram_safe_get(buffer));

	/* Routing */
	nvi = atoi(getNVRAMVar("vpnc%d_rgw", unit));

	if (nvi == VPN_RGW_ALL) {
		if (if_type == OVPN_IF_TAP && getNVRAMVar("vpnc%d_gw", unit)[0] != '\0')
			fprintf(fp, "route-gateway %s\n", getNVRAMVar("vpnc%d_gw", unit));
		fprintf(fp, "redirect-gateway def1\n");
	}
	else if (nvi >= VPN_RGW_POLICY)
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
		nvi = atoi(getNVRAMVar("vpnc%d_hmac", unit));

		memset(buffer, 0, BUF_SIZE);
		snprintf(buffer, BUF_SIZE, "vpnc%d_static", unit);

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
		snprintf(buffer, BUF_SIZE, "vpnc%d_ca", unit);
		if (!nvram_is_empty(buffer))
			fprintf(fp, "ca ca.crt\n");

		if (!useronly) {
			memset(buffer, 0, BUF_SIZE);
			snprintf(buffer, BUF_SIZE, "vpnc%d_crt", unit);
			if (!nvram_is_empty(buffer))
				fprintf(fp, "cert client.crt\n");

			memset(buffer, 0, BUF_SIZE);
			snprintf(buffer, BUF_SIZE, "vpnc%d_key", unit);
			if (!nvram_is_empty(buffer))
				fprintf(fp, "key client.key\n");
		}

		if (atoi(getNVRAMVar("vpnc%d_tlsremote", unit)))
			fprintf(fp, "remote-cert-tls server\n");

		if ((nvi = atoi(getNVRAMVar("vpnc%d_tlsvername", unit))) > 0) {
			fprintf(fp, "verify-x509-name \"%s\" ", getNVRAMVar("vpnc%d_cn", unit));
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
		snprintf(buffer, BUF_SIZE, "vpnc%d_static", unit);

		if (!nvram_is_empty(buffer))
			fprintf(fp, "secret static.key\n");
	}
	fprintf(fp, "keepalive 15 60\n"
	            "verb 3\n"
	            "status-version 2\n"
	            "status status 10\n" /* Update status file every 10 sec */
	            "# Custom Configuration\n"
	            "%s",
	            getNVRAMVar("vpnc%d_custom", unit));

	fclose(fp);

	/* Write certification and key files */
	if (auth_mode == OVPN_AUTH_TLS) {
		memset(buffer, 0, BUF_SIZE);
		snprintf(buffer, BUF_SIZE, "vpnc%d_ca", unit);
		if (!nvram_is_empty(buffer)) {
			memset(buffer, 0, BUF_SIZE);
			snprintf(buffer, BUF_SIZE, OVPN_DIR"/client%d/ca.crt", unit);
			fp = fopen(buffer, "w");
			if (!fp) {
				logmsg(LOG_ERR, "failed to create %s: (%s)", buffer, strerror(errno));
				stop_ovpn_client(unit);
				return;
			}
			chmod(buffer, (S_IRUSR | S_IWUSR));
			fprintf(fp, "%s", getNVRAMVar("vpnc%d_ca", unit));
			fclose(fp);
		}

		if (!useronly) {
			memset(buffer, 0, BUF_SIZE);
			snprintf(buffer, BUF_SIZE, "vpnc%d_key", unit);
			if (!nvram_is_empty(buffer)) {
				memset(buffer, 0, BUF_SIZE);
				snprintf(buffer, BUF_SIZE, OVPN_DIR"/client%d/client.key", unit);
				fp = fopen(buffer, "w");
				if (!fp) {
					logmsg(LOG_ERR, "failed to create %s: (%s)", buffer, strerror(errno));
					stop_ovpn_client(unit);
					return;
				}
				chmod(buffer, (S_IRUSR | S_IWUSR));
				fprintf(fp, "%s", getNVRAMVar("vpnc%d_key", unit));
				fclose(fp);
			}

			memset(buffer, 0, BUF_SIZE);
			snprintf(buffer, BUF_SIZE, "vpnc%d_crt", unit);
			if (!nvram_is_empty(buffer)) {
				memset(buffer, 0, BUF_SIZE);
				snprintf(buffer, BUF_SIZE, OVPN_DIR"/client%d/client.crt", unit);
				fp = fopen(buffer, "w");
				if (!fp) {
					logmsg(LOG_ERR, "failed to create %s: (%s)", buffer, strerror(errno));
					stop_ovpn_client(unit);
					return;
				}
				chmod(buffer, (S_IRUSR | S_IWUSR));
				fprintf(fp, "%s", getNVRAMVar("vpnc%d_crt", unit));
				fclose(fp);
			}
		}
		if (userauth) {
			memset(buffer, 0, BUF_SIZE);
			snprintf(buffer, BUF_SIZE, OVPN_DIR"/client%d/up", unit);
			fp = fopen(buffer, "w");
			if (!fp) {
				logmsg(LOG_ERR, "failed to create %s: (%s)", buffer, strerror(errno));
				stop_ovpn_client(unit);
				return;
			}
			chmod(buffer, (S_IRUSR | S_IWUSR));
			fprintf(fp, "%s\n", getNVRAMVar("vpnc%d_username", unit));
			fprintf(fp, "%s\n", getNVRAMVar("vpnc%d_password", unit));
			fclose(fp);
		}
	}

	if ((auth_mode == OVPN_AUTH_STATIC) || (auth_mode == OVPN_AUTH_TLS && atoi(getNVRAMVar("vpnc%d_hmac", unit)) >= 0)) {
		memset(buffer, 0, BUF_SIZE);
		snprintf(buffer, BUF_SIZE, "vpnc%d_static", unit);
		if (!nvram_is_empty(buffer)) {
			memset(buffer, 0, BUF_SIZE);
			snprintf(buffer, BUF_SIZE, OVPN_DIR"/client%d/static.key", unit);
			fp = fopen(buffer, "w");
			if (!fp) {
				logmsg(LOG_ERR, "failed to create %s: (%s)", buffer, strerror(errno));
				stop_ovpn_client(unit);
				return;
			}
			chmod(buffer, (S_IRUSR | S_IWUSR));
			fprintf(fp, "%s", getNVRAMVar("vpnc%d_static", unit));
			fclose(fp);
		}
	}

	/* Handle firewall rules if appropriate */
	memset(buffer, 0, BUF_SIZE);
	snprintf(buffer, BUF_SIZE, "vpnc%d_firewall", unit);
	if (!nvram_contains_word(buffer, "custom")) {
		chains_log_detection();

		/* Create firewall rules */
		mkdir(OVPN_FW_DIR, 0700);
		memset(buffer, 0, BUF_SIZE);
		snprintf(buffer, BUF_SIZE, OVPN_FW_DIR"/client%d-fw.sh", unit);
		fp = fopen(buffer, "w");
		if (!fp) {
			logmsg(LOG_ERR, "failed to create %s: (%s)", buffer, strerror(errno));
			stop_ovpn_client(unit);
			return;
		}

		nvi = atoi(getNVRAMVar("vpnc%d_fw", unit));

		fprintf(fp, "#!/bin/sh\n"
		            "iptables -I INPUT -i %s -m state --state NEW -j %s\n"
		            "iptables -I FORWARD -i %s -m state --state NEW -j %s\n"
		            "iptables -I FORWARD -o %s -j ACCEPT\n",
		            iface, (nvi ? chain_in_drop : chain_in_accept),
		            iface, (nvi ? "DROP" : "ACCEPT"),
		            iface);
#ifdef TCONFIG_BCMARM
		if (!nvram_get_int("ctf_disable")) { /* bypass CTF if enabled */
			fprintf(fp, "iptables -t mangle -I PREROUTING -i %s -j MARK --set-mark 0x01/0x7\n"
			            "iptables -t mangle -I POSTROUTING -o %s -j MARK --set-mark 0x01/0x7\n",
			            iface, iface);
#ifdef TCONFIG_IPV6
			if (ipv6_enabled()) {
				fprintf(fp, "ip6tables -t mangle -I PREROUTING -i %s -j MARK --set-mark 0x01/0x7\n"
				            "ip6tables -t mangle -I POSTROUTING -o %s -j MARK --set-mark 0x01/0x7\n",
				            iface, iface);
			}
#endif
		}
#endif /* TCONFIG_BCMARM */

		if (route_mode == NAT) {
			/* masquerade all client outbound traffic regardless of source subnet */
			fprintf(fp, "iptables -t nat -I POSTROUTING -o %s -j MASQUERADE\n", iface);
		}

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

		nvi = atoi(getNVRAMVar("vpnc%d_rgw", unit));
		if (nvi >= VPN_RGW_POLICY) {
			/* Disable rp_filter when in policy mode */
			fprintf(fp, "echo 0 > /proc/sys/net/ipv4/conf/%s/rp_filter\n"
			            "echo 0 > /proc/sys/net/ipv4/conf/all/rp_filter\n",
			            iface);

#if defined(TCONFIG_BCMARM)
			modprobe("ip_set");
			modprobe("xt_set");
			modprobe("ip_set_hash_ip");
#else
			modprobe("ip_set");
			modprobe("ipt_set");
			modprobe("ip_set_iphash");
#endif

		}

		fclose(fp);
		chmod(buffer, (S_IRUSR | S_IWUSR | S_IXUSR));

		/* firewall rules */
		memset(buffer, 0, BUF_SIZE);
		snprintf(buffer, BUF_SIZE, OVPN_FW_DIR"/client%d-fw.sh", unit);

		/* first remove existing firewall rule(s) */
		simple_lock("firewall");
		run_del_firewall_script(buffer, OVPN_DIR_DEL_SCRIPT);

		/* then add firewall rule(s) */
		eval(buffer);
		simple_unlock("firewall");
	}

	/* In case of openvpn unexpectedly dies and leaves it added - flush tun IF, otherwise openvpn will not re-start (required by iproute2) */
	eval("ip", "addr", "flush", "dev", iface);

	/* Start the VPN client */
	memset(buffer, 0, BUF_SIZE);
	snprintf(buffer, BUF_SIZE, OVPN_DIR"/vpnclient%d", unit);
	memset(buffer2, 0, BUF_SIZE_32);
	snprintf(buffer2, BUF_SIZE_32, OVPN_DIR"/client%d", unit);

#if defined(TCONFIG_BCMARM) && defined(TCONFIG_BCMSMP)
	/* Spread clients on cpu 1,0 or 1,2,3,0 (in that order) */
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
	snprintf(buffer, BUF_SIZE, "vpn_client%d", unit);
	allow_fastnat(buffer, 0);
	try_enabling_fastnat();
}

void stop_ovpn_client(int unit)
{
	char buffer[BUF_SIZE];

	memset(buffer, 0, BUF_SIZE);
	snprintf(buffer, BUF_SIZE, "vpnclient%d", unit);
	if (serialize_restart(buffer, 0))
		return;

	/* Remove cron job */
	memset(buffer, 0, BUF_SIZE);
	snprintf(buffer, BUF_SIZE, "CheckVPNclient%d", unit);
	eval("cru", "d", buffer);

	/* Stop the VPN client */
	memset(buffer, 0, BUF_SIZE);
	snprintf(buffer, BUF_SIZE, "vpnclient%d", unit);
	killall_and_waitfor(buffer, 5, 50);

	ovpn_remove_iface(OVPN_TYPE_CLIENT, unit);

	/* Remove firewall rules after VPN exit */
	memset(buffer, 0, BUF_SIZE);
	snprintf(buffer, BUF_SIZE, OVPN_FW_DIR"/client%d-fw.sh", unit);

	simple_lock("firewall");
	run_del_firewall_script(buffer, OVPN_DIR_DEL_SCRIPT);

	/* Delete all files for this client */
	ovpn_cleanup_dirs(OVPN_TYPE_CLIENT, unit);
	simple_unlock("firewall");

	memset(buffer, 0, BUF_SIZE);
	snprintf(buffer, BUF_SIZE, "vpn_client%d", unit);
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
	char buffer2[BUF_SIZE_32];
	int mwan_num, taskset_ret = 0;
	long int nvl;
#ifndef TCONFIG_OPTIMIZE_SIZE_MORE
	FILE *ccd;
	char *br_ipaddr, *br_netmask;
	char *chp, *route;
	int nvi, i, ip[4], nm[4];
	int c2c = 0;
	int dont_push_active = 0;
	int push_lan[BRIDGE_COUNT] = {0};
#endif
#if defined(TCONFIG_BCMARM) && defined(TCONFIG_BCMSMP)
	char cpulist[2];
	int cpu_num = sysconf(_SC_NPROCESSORS_CONF) - 1;
	if (cpu_num < 0)
		cpu_num = 0;
#endif

	memset(buffer, 0, BUF_SIZE);
	snprintf(buffer, BUF_SIZE, "vpnserver%d", unit);
	if (serialize_restart(buffer, 1))
		return;

	/* Determine interface */
	memset(buffer, 0, BUF_SIZE);
	snprintf(buffer, BUF_SIZE, "vpns%d_if", unit);
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
	snprintf(buffer, BUF_SIZE, "vpns%d_crypt", unit);
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
		eval("brctl", "delif", getNVRAMVar("vpns%d_br", unit), iface);

	/* Setup directories and symlinks */
	ovpn_setup_dirs(OVPN_TYPE_SERVER, unit);

	/* Setup interface */
#ifdef TOMATO64
	/* DCO creates its own interface */
	if (!atoi(getNVRAMVar("vpns%d_dco", unit))) {
#endif /* TOMATO64 */
	if (ovpn_setup_iface(iface, if_type, 1, unit, OVPN_TYPE_SERVER)) {
		stop_ovpn_server(unit);
		return;
	}
#ifdef TOMATO64
	}
#endif /* TOMATO64 */

	/* Build and write config files */
	memset(buffer, 0, BUF_SIZE);
	snprintf(buffer, BUF_SIZE, OVPN_DIR"/server%d/config.ovpn", unit);
	fp = fopen(buffer, "w");
	if (!fp) {
		logmsg(LOG_ERR, "failed to create %s: (%s)", buffer, strerror(errno));
		stop_ovpn_server(unit);
		return;
	}
	chmod(buffer, (S_IRUSR | S_IWUSR));

	fprintf(fp, "# Generated Configuration\n"
	            "daemon openvpn-server%d\n"
	            "port %d\n"
	            "dev %s\n"
	            "txqueuelen 1000\n"
	            "keepalive 15 60\n"
	            "verb 3\n",
	            unit,
	            atoi(getNVRAMVar("vpns%d_port", unit)),
	            iface);

#ifdef TOMATO64
	/* DCO - Data Channel Offload */
	if (atoi(getNVRAMVar("vpns%d_dco", unit))) {
		modprobe("ovpn_dco_v2");
	}
	else {
		fprintf(fp, "disable-dco\n");
	}
#endif /* TOMATO64 */

#ifndef TCONFIG_OPTIMIZE_SIZE_MORE
	if (auth_mode == OVPN_AUTH_TLS) {
		if (if_type == OVPN_IF_TUN) {
			fprintf(fp, "topology subnet\n"
			            "server %s %s\n",
			            getNVRAMVar("vpns%d_sn", unit),
			            getNVRAMVar("vpns%d_nm", unit));
		}
		else if (if_type == OVPN_IF_TAP) {
			fprintf(fp, "server-bridge");

			if (atoi(getNVRAMVar("vpns%d_dhcp", unit)) == 0) {
				br_ipaddr = nvram_get("lan_ipaddr"); /* default */
				br_netmask = nvram_get("lan_netmask");

				memset(buffer, 0, BUF_SIZE);
				snprintf(buffer, BUF_SIZE, "vpns%d_br", unit);
				for (i = 1; i < BRIDGE_COUNT; i++) {
					memset(buffer2, 0, BUF_SIZE_32);
					snprintf(buffer2, BUF_SIZE_32, "br%d", i);
					if (nvram_contains_word(buffer, buffer2)) {
						memset(buffer2, 0, BUF_SIZE_32);
						snprintf(buffer2, BUF_SIZE_32, "lan%d_ipaddr", i);
						br_ipaddr = nvram_get(buffer2);
						memset(buffer2, 0, BUF_SIZE_32);
						snprintf(buffer2, BUF_SIZE_32, "lan%d_netmask", i);
						br_netmask = nvram_get(buffer2);
						break;
					}
				}

				fprintf(fp, " %s %s %s %s",
				            br_ipaddr, br_netmask,
				            getNVRAMVar("vpns%d_r1", unit),
				            getNVRAMVar("vpns%d_r2", unit));
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
			fprintf(fp, "ifconfig %s ", getNVRAMVar("vpns%d_local", unit));
			fprintf(fp, "%s\n", getNVRAMVar("vpns%d_remote", unit));
		}
	}

	/* Proto */
	mwan_num = nvram_get_int("mwan_num"); /* check active WANs num */
	if (mwan_num < 1)
		mwan_num = 1;

	memset(buffer, 0, BUF_SIZE);
	snprintf(buffer, BUF_SIZE, "vpns%d_proto", unit);
	fprintf(fp, "proto %s\n", nvram_safe_get(buffer)); /* full dual-stack functionality starting with OpenVPN 2.4.0 */

	if (nvram_contains_word(buffer, "udp") && mwan_num > 1) /* udp/udp4/udp6 - only if multiwan */
		fprintf(fp, "multihome\n");

	/* Cipher */
	strlcpy(buffer, getNVRAMVar("vpns%d_ncp_ciphers", unit), BUF_SIZE);
#ifndef TCONFIG_OPTIMIZE_SIZE_MORE
	if (auth_mode == OVPN_AUTH_TLS) {
		if (buffer[0] != '\0')
			fprintf(fp, "data-ciphers %s\n", buffer);
	}
	else
#endif
	     {	/* SECRET/CUSTOM */
		memset(buffer, 0, BUF_SIZE);
		snprintf(buffer, BUF_SIZE, "vpns%d_cipher", unit);
		if (!nvram_contains_word(buffer, "default"))
			fprintf(fp, "cipher %s\n", nvram_safe_get(buffer));
	}

	/* Digest */
	memset(buffer, 0, BUF_SIZE);
	snprintf(buffer, BUF_SIZE, "vpns%d_digest", unit);
	if (!nvram_contains_word(buffer, "default"))
		fprintf(fp, "auth %s\n", nvram_safe_get(buffer));

	/* Compression */
	memset(buffer, 0, BUF_SIZE);
	strlcpy(buffer, getNVRAMVar("vpns%d_comp", unit), BUF_SIZE);
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

	if ((nvl = atol(getNVRAMVar("vpns%d_reneg", unit))) >= 0)
		fprintf(fp, "reneg-sec %ld\n", nvl);

#ifndef TCONFIG_OPTIMIZE_SIZE_MORE
	if (auth_mode == OVPN_AUTH_TLS) {
		if (if_type == OVPN_IF_TUN) {
			/* push LANs */
			for (i = 0; i < BRIDGE_COUNT; i++) {
				memset(buffer, 0, BUF_SIZE);
				snprintf(buffer, BUF_SIZE, (i == 0 ? "vpns%d_plan" : "vpns%d_plan%d"), unit, i);
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

		if (atoi(getNVRAMVar("vpns%d_ccd", unit))) {
			fprintf(fp, "client-config-dir ccd\n");

			if ((c2c = atoi(getNVRAMVar("vpns%d_c2c", unit))))
				fprintf(fp, "client-to-client\n");

			if (atoi(getNVRAMVar("vpns%d_ccd_excl", unit)))
				fprintf(fp, "ccd-exclusive\n");

			memset(buffer, 0, BUF_SIZE);
			snprintf(buffer, BUF_SIZE, OVPN_DIR"/server%d/ccd", unit);
			mkdir(buffer, 0700);
			if (chdir(buffer) != 0) {
				logmsg(LOG_WARNING, "chdir to %s failed (%s)", buffer, strerror(errno));
				stop_ovpn_server(unit);
				return;
			}

			memset(buffer, 0, BUF_SIZE);
			snprintf(buffer, BUF_SIZE, "vpns%d_ccd_val", unit);
			strlcpy(buffer, nvram_safe_get(buffer), BUF_SIZE);
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
						if (!ccd) {
							logmsg(LOG_ERR, "failed to create %s: (%s)", chp, strerror(errno));
							stop_ovpn_server(unit);
							return;
						}
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

		if (atoi(getNVRAMVar("vpns%d_userpass", unit))) {
			fprintf(fp, "plugin /lib/openvpn_plugin_auth_nvram.so vpns%d_users_val\n"
			            "script-security 2\n",
			            unit);

			if (atoi(getNVRAMVar("vpns%d_nocert", unit))) {
				fprintf(fp, "verify-client-cert optional\n"
				            "username-as-common-name\n");
			}
		}

		if (atoi(getNVRAMVar("vpns%d_pdns", unit))) {
			if (nvram_safe_get("wan_domain")[0] != '\0')
				fprintf(fp, "push \"dhcp-option DOMAIN %s\"\n", nvram_safe_get("wan_domain"));
			if ((nvram_safe_get("wan_wins")[0] != '\0' && strcmp(nvram_safe_get("wan_wins"), "0.0.0.0") != 0))
				fprintf(fp, "push \"dhcp-option WINS %s\"\n", nvram_safe_get("wan_wins"));

			/* check if LANX will be pushed --> if YES, push the suitable DNS Server address */
			for (i = 0; i < BRIDGE_COUNT; i++) {
				if (push_lan[i] == 1) { /* push IPv4 LANx DNS */
					memset(buffer, 0, BUF_SIZE);
					snprintf(buffer, BUF_SIZE, (i == 0 ? "lan_ipaddr" : "lan%d_ipaddr"), i);
					fprintf(fp, "push \"dhcp-option DNS %s\"\n", nvram_safe_get(buffer));
					dont_push_active = 1;
				}
			}
			/* no LANx will be pushed, push only one active DNS */
			/* check what LAN is active before push DNS */
			if (dont_push_active == 0) {
				for (i = 0; i < BRIDGE_COUNT; i++) {
					memset(buffer, 0, BUF_SIZE);
					snprintf(buffer, BUF_SIZE, (i == 0 ? "lan_ipaddr" : "lan%d_ipaddr"), i);
					if (strcmp(nvram_safe_get(buffer), "") != 0) {
						fprintf(fp, "push \"dhcp-option DNS %s\"\n", nvram_safe_get(buffer));
						break;
					}
				}
			}
		}

		if (atoi(getNVRAMVar("vpns%d_rgw", unit))) {
			if (if_type == OVPN_IF_TAP)
				fprintf(fp, "push \"route-gateway %s\"\n", nvram_safe_get("lan_ipaddr"));
			fprintf(fp, "push \"redirect-gateway def1\"\n");
		}

		nvi = atoi(getNVRAMVar("vpns%d_hmac", unit));
		memset(buffer, 0, BUF_SIZE);
		snprintf(buffer, BUF_SIZE, "vpns%d_static", unit);
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
		snprintf(buffer, BUF_SIZE, "vpns%d_ca", unit);
		if (!nvram_is_empty(buffer))
			fprintf(fp, "ca ca.crt\n");

		nvi = atoi(getNVRAMVar("vpns%d_ecdh", unit));
		memset(buffer, 0, BUF_SIZE);
		snprintf(buffer, BUF_SIZE, "vpns%d_dh", unit);
		if (!nvram_is_empty(buffer) && nvi == 0)
			fprintf(fp, "dh dh.pem\n");
		else
			fprintf(fp, "dh none\n");

		memset(buffer, 0, BUF_SIZE);
		snprintf(buffer, BUF_SIZE, "vpns%d_crt", unit);
		if (!nvram_is_empty(buffer))
			fprintf(fp, "cert server.crt\n");
		memset(buffer, 0, BUF_SIZE);
		snprintf(buffer, BUF_SIZE, "vpns%d_crl", unit);
		if (!nvram_is_empty(buffer))
			fprintf(fp, "crl-verify crl.pem\n");
		memset(buffer, 0, BUF_SIZE);
		snprintf(buffer, BUF_SIZE, "vpns%d_key", unit);
		if (!nvram_is_empty(buffer))
			fprintf(fp, "key server.key\n");
	}
	else
#endif
	     if (auth_mode == OVPN_AUTH_STATIC) {
		memset(buffer, 0, BUF_SIZE);
		snprintf(buffer, BUF_SIZE, "vpns%d_static", unit);
		if (!nvram_is_empty(buffer))
			fprintf(fp, "secret static.key\n");
	}
	fprintf(fp, "status-version 2\n"
	            "status status 10\n\n" /* Update status file every 10 sec */
	            "# Custom Configuration\n"
	            "%s",
	            getNVRAMVar("vpns%d_custom", unit));

	fclose(fp);

	/* Write certification and key files */
#ifndef TCONFIG_OPTIMIZE_SIZE_MORE
	if (auth_mode == OVPN_AUTH_TLS) {
		memset(buffer, 0, BUF_SIZE);
		snprintf(buffer, BUF_SIZE, "vpns%d_ca", unit);
		if (!nvram_is_empty(buffer)) {
			memset(buffer, 0, BUF_SIZE);
			snprintf(buffer, BUF_SIZE, OVPN_DIR"/server%d/ca.crt", unit);
			fp = fopen(buffer, "w");
			if (!fp) {
				logmsg(LOG_ERR, "failed to create %s: (%s)", buffer, strerror(errno));
				stop_ovpn_server(unit);
				return;
			}
			chmod(buffer, (S_IRUSR | S_IWUSR));
			fprintf(fp, "%s", getNVRAMVar("vpns%d_ca", unit));
			fclose(fp);
		}

		memset(buffer, 0, BUF_SIZE);
		snprintf(buffer, BUF_SIZE, "vpns%d_key", unit);
		if (!nvram_is_empty(buffer)) {
			memset(buffer, 0, BUF_SIZE);
			snprintf(buffer, BUF_SIZE, OVPN_DIR"/server%d/server.key", unit);
			fp = fopen(buffer, "w");
			if (!fp) {
				logmsg(LOG_ERR, "failed to create %s: (%s)", buffer, strerror(errno));
				stop_ovpn_server(unit);
				return;
			}
			chmod(buffer, (S_IRUSR | S_IWUSR));
			fprintf(fp, "%s", getNVRAMVar("vpns%d_key", unit));
			fclose(fp);
		}

		memset(buffer, 0, BUF_SIZE);
		snprintf(buffer, BUF_SIZE, "vpns%d_crt", unit);
		if (!nvram_is_empty(buffer)) {
			memset(buffer, 0, BUF_SIZE);
			snprintf(buffer, BUF_SIZE, OVPN_DIR"/server%d/server.crt", unit);
			fp = fopen(buffer, "w");
			if (!fp) {
				logmsg(LOG_ERR, "failed to create %s: (%s)", buffer, strerror(errno));
				stop_ovpn_server(unit);
				return;
			}
			chmod(buffer, (S_IRUSR | S_IWUSR));
			fprintf(fp, "%s", getNVRAMVar("vpns%d_crt", unit));
			fclose(fp);
		}

		memset(buffer, 0, BUF_SIZE);
		snprintf(buffer, BUF_SIZE, "vpns%d_crl", unit);
		if (!nvram_is_empty(buffer)) {
			memset(buffer, 0, BUF_SIZE);
			snprintf(buffer, BUF_SIZE, OVPN_DIR"/server%d/crl.pem", unit);
			fp = fopen(buffer, "w");
			if (!fp) {
				logmsg(LOG_ERR, "failed to create %s: (%s)", buffer, strerror(errno));
				stop_ovpn_server(unit);
				return;
			}
			chmod(buffer, (S_IRUSR | S_IWUSR));
			fprintf(fp, "%s", getNVRAMVar("vpns%d_crl", unit));
			fclose(fp);
		}

		memset(buffer, 0, BUF_SIZE);
		snprintf(buffer, BUF_SIZE, "vpns%d_dh", unit);
		if (!nvram_is_empty(buffer)) {
			memset(buffer, 0, BUF_SIZE);
			snprintf(buffer, BUF_SIZE, OVPN_DIR"/server%d/dh.pem", unit);
			fp = fopen(buffer, "w");
			if (!fp) {
				logmsg(LOG_ERR, "failed to create %s: (%s)", buffer, strerror(errno));
				stop_ovpn_server(unit);
				return;
			}
			chmod(buffer, (S_IRUSR | S_IWUSR));
			fprintf(fp, "%s", getNVRAMVar("vpns%d_dh", unit));
			fclose(fp);
		}
	}
#endif
	if ((auth_mode == OVPN_AUTH_STATIC) || (auth_mode == OVPN_AUTH_TLS && atoi(getNVRAMVar("vpns%d_hmac", unit)) >= 0)) {
		memset(buffer, 0, BUF_SIZE);
		snprintf(buffer, BUF_SIZE, "vpns%d_static", unit);
		if (!nvram_is_empty(buffer)) {
			memset(buffer, 0, BUF_SIZE);
			snprintf(buffer, BUF_SIZE, OVPN_DIR"/server%d/static.key", unit);
			fp = fopen(buffer, "w");
			if (!fp) {
				logmsg(LOG_ERR, "failed to create %s: (%s)", buffer, strerror(errno));
				stop_ovpn_server(unit);
				return;
			}
			chmod(buffer, (S_IRUSR | S_IWUSR));
			fprintf(fp, "%s", getNVRAMVar("vpns%d_static", unit));
			fclose(fp);
		}
	}

	/* Handle firewall rules if appropriate */
	memset(buffer, 0, BUF_SIZE);
	snprintf(buffer, BUF_SIZE, "vpns%d_firewall", unit);
	if (!nvram_contains_word(buffer, "custom")) {
		chains_log_detection();

		/* Create firewall rules */
		mkdir(OVPN_FW_DIR, 0700);
		memset(buffer, 0, BUF_SIZE);
		snprintf(buffer, BUF_SIZE, OVPN_FW_DIR"/server%d-fw.sh", unit);
		fp = fopen(buffer, "w");
		if (!fp) {
			logmsg(LOG_ERR, "failed to create %s: (%s)", buffer, strerror(errno));
			stop_ovpn_server(unit);
			return;
		}
		chmod(buffer, (S_IRUSR | S_IWUSR | S_IXUSR));

		memset(buffer, 0, BUF_SIZE);
		strncpy(buffer, getNVRAMVar("vpns%d_proto", unit), BUF_SIZE);
		memset(buffer2, 0, BUF_SIZE_32);
		if ((!strcmp(buffer, "udp")) || (!strcmp(buffer, "udp4")) || (!strcmp(buffer, "udp6")))
			snprintf(buffer2, BUF_SIZE_32, "udp");
		else
			snprintf(buffer2, BUF_SIZE_32, "tcp");

		fprintf(fp, "#!/bin/sh\n"
		            "iptables -t nat -I PREROUTING -p %s --dport %d -j ACCEPT\n",
		            buffer2, atoi(getNVRAMVar("vpns%d_port", unit)));

		memset(buffer, 0, BUF_SIZE);
		strncpy(buffer, getNVRAMVar("vpns%d_proto", unit), BUF_SIZE);
		fprintf(fp, "iptables -I INPUT -p %s --dport %d -j %s\n",
		            buffer2, atoi(getNVRAMVar("vpns%d_port", unit)), chain_in_accept);

		memset(buffer, 0, BUF_SIZE);
		snprintf(buffer, BUF_SIZE, "vpns%d_firewall", unit);
		if (!nvram_contains_word(buffer, "external")) {
			fprintf(fp, "iptables -I INPUT -i %s -j %s\n"
			            "iptables -I FORWARD -i %s -j ACCEPT\n",
			            iface, chain_in_accept,
			            iface);
#ifdef TCONFIG_BCMARM
			if (!nvram_get_int("ctf_disable")) { /* bypass CTF if enabled */
				fprintf(fp, "iptables -t mangle -I PREROUTING -i %s -j MARK --set-mark 0x01/0x7\n"
				            "iptables -t mangle -I POSTROUTING -o %s -j MARK --set-mark 0x01/0x7\n",
				            iface, iface);
#ifdef TCONFIG_IPV6
				if (ipv6_enabled()) {
					fprintf(fp, "ip6tables -t mangle -I PREROUTING -i %s -j MARK --set-mark 0x01/0x7\n"
					            "ip6tables -t mangle -I POSTROUTING -o %s -j MARK --set-mark 0x01/0x7\n",
					            iface, iface);
				}
#endif
			}
#endif /* TCONFIG_BCMARM */
		}

		/* Create firewall rules for IPv6 */
#ifdef TCONFIG_IPV6
		if (ipv6_enabled()) {
			strncpy(buffer, getNVRAMVar("vpns%d_proto", unit), BUF_SIZE);
			fprintf(fp, "ip6tables -I INPUT -p %s --dport %d -j %s\n",
			            buffer2, atoi(getNVRAMVar("vpns%d_port", unit)), chain_in_accept);

			memset(buffer, 0, BUF_SIZE);
			snprintf(buffer, BUF_SIZE, "vpns%d_firewall", unit);
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
		snprintf(buffer, BUF_SIZE, OVPN_FW_DIR"/server%d-fw.sh", unit);

		/* first remove existing firewall rule(s) */
		simple_lock("firewall");
		run_del_firewall_script(buffer, OVPN_DIR_DEL_SCRIPT);

		/* then add firewall rule(s) */
		eval(buffer);
		simple_unlock("firewall");
	}

	/* Start the VPN server */
	memset(buffer, 0, BUF_SIZE);
	snprintf(buffer, BUF_SIZE, OVPN_DIR"/vpnserver%d", unit);
	memset(buffer2, 0, BUF_SIZE_32);
	snprintf(buffer2, BUF_SIZE_32, OVPN_DIR"/server%d", unit);

#if defined(TCONFIG_BCMARM) && defined(TCONFIG_BCMSMP)
	/* Spread servers on cpu 1,0 or 1,2 (in that order) */
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
	snprintf(buffer, BUF_SIZE, "vpn_server%d", unit);
	allow_fastnat(buffer, 0);
	try_enabling_fastnat();
}

void stop_ovpn_server(int unit)
{
	char buffer[BUF_SIZE];

	memset(buffer, 0, BUF_SIZE);
	snprintf(buffer, BUF_SIZE, "vpnserver%d", unit);
	if (serialize_restart(buffer, 0))
		return;

	/* Remove cron job */
	memset(buffer, 0, BUF_SIZE);
	snprintf(buffer, BUF_SIZE, "CheckVPNserver%d", unit);
	eval("cru", "d", buffer);

	/* Stop the VPN server */
	memset(buffer, 0, BUF_SIZE);
	snprintf(buffer, BUF_SIZE, "vpnserver%d", unit);
	killall_and_waitfor(buffer, 5, 50);

	ovpn_remove_iface(OVPN_TYPE_SERVER, unit);

	/* Remove firewall rules */
	memset(buffer, 0, BUF_SIZE);
	snprintf(buffer, BUF_SIZE, OVPN_FW_DIR"/server%d-fw.sh", unit);

	simple_lock("firewall");
	run_del_firewall_script(buffer, OVPN_DIR_DEL_SCRIPT);

	/* Delete all files for this server */
	ovpn_cleanup_dirs(OVPN_TYPE_SERVER, unit);
	simple_unlock("firewall");

	memset(buffer, 0, BUF_SIZE);
	snprintf(buffer, BUF_SIZE, "vpn_server%d", unit);
	allow_fastnat(buffer, 1);
	try_enabling_fastnat();
}

void start_ovpn_eas()
{
	char buffer[BUF_SIZE_16], *cur;
#ifdef TOMATO64
        int nums[OVPN_SERVER_MAX + 1], i;
#else
	int nums[OVPN_CLIENT_MAX + 1], i;
#endif /* TOMATO64 */

	/* OVPN_CLIENT_MAX is always bigger than OVPN_SERVER_MAX */

	if ((strlen(nvram_safe_get("vpns_eas")) == 0) && (strlen(nvram_safe_get("vpnc_eas")) == 0))
		return;

	/* Parse and start servers */
	strlcpy(buffer, nvram_safe_get("vpns_eas"), BUF_SIZE_16);

	i = 0;
	for (cur = strtok(buffer, ","); (cur != NULL) && (i < OVPN_SERVER_MAX); cur = strtok(NULL, ","))
		nums[i++] = atoi(cur);

	nums[i] = 0;
	for (i = 0; (nums[i] > 0) && (nums[i] <= OVPN_SERVER_MAX); i++) {
		memset(buffer, 0, BUF_SIZE_16);
		snprintf(buffer, BUF_SIZE_16, "vpnserver%d", nums[i]);

		if (pidof(buffer) > 0)
			stop_ovpn_server(nums[i]);

		start_ovpn_server(nums[i]);
	}

	/* Parse and start clients */
	strlcpy(buffer, nvram_safe_get("vpnc_eas"), BUF_SIZE_16);

	i = 0;
	for (cur = strtok(buffer, ","); (cur != NULL) && (i < OVPN_CLIENT_MAX); cur = strtok(NULL, ","))
		nums[i++] = atoi(cur);

	nums[i] = 0;
	for (i = 0; (nums[i] > 0) && (nums[i] <= OVPN_CLIENT_MAX); i++) {
		memset(buffer, 0, BUF_SIZE_16);
		snprintf(buffer, BUF_SIZE_16, "vpnclient%d", nums[i]);

		if (pidof(buffer) > 0)
			stop_ovpn_client(nums[i]);

		start_ovpn_client(nums[i]);
	}
}
/*
void stop_ovpn_eas()
{
	char buffer[BUF_SIZE_16], *cur;
	int nums[OVPN_CLIENT_MAX + 1], i;
	// OVPN_CLIENT_MAX is always bigger than OVPN_SERVER_MAX

	// Parse and stop servers
	strlcpy(buffer, nvram_safe_get("vpns_eas"), BUF_SIZE_16);

	i = 0;
	for (cur = strtok(buffer, ","); (cur != NULL) && (i < OVPN_SERVER_MAX); cur = strtok(NULL, ","))
		nums[i++] = atoi(cur);

	nums[i] = 0;
	for (i = 0; (nums[i] > 0) && (nums[i] <= OVPN_SERVER_MAX); i++) {
		memset(buffer, 0, BUF_SIZE_16);
		snprintf(buffer, BUF_SIZE_16, "vpnserver%d", nums[i]);

		if (pidof(buffer) > 0)
			stop_ovpn_server(nums[i]);
	}

	// Parse and stop clients
	strlcpy(buffer, nvram_safe_get("vpnc_eas"), BUF_SIZE_16);

	i = 0;
	for (cur = strtok(buffer, ","); (cur != NULL) && (i < OVPN_CLIENT_MAX); cur = strtok(NULL, ","))
		nums[i++] = atoi(cur);

	nums[i] = 0;
	for (i = 0; (nums[i] > 0) && (nums[i] <= OVPN_CLIENT_MAX); i++) {
		memset(buffer, 0, BUF_SIZE_16);
		snprintf(buffer, BUF_SIZE_16, "vpnclient%d", nums[i]);

		if (pidof(buffer) > 0)
			stop_ovpn_client(nums[i]);
	}
}
*/
void stop_ovpn_all()
{
	char buffer[BUF_SIZE_16];
	int i;

	/* Stop servers */
	for (i = 1; i <= OVPN_SERVER_MAX; i++) {
		memset(buffer, 0, BUF_SIZE_16);
		snprintf(buffer, BUF_SIZE_16, "vpnserver%d", i);
		if (pidof(buffer) > 0)
			stop_ovpn_server(i);
	}

	/* Stop clients */
	for (i = 1; i <= OVPN_CLIENT_MAX; i++) {
		memset(buffer, 0, BUF_SIZE_16);
		snprintf(buffer, BUF_SIZE_16, "vpnclient%d", i);
		if (pidof(buffer) > 0)
			stop_ovpn_client(i);
	}

	/* Remove tunnel interface module */
	modprobe_r("tun");
}

void write_ovpn_dnsmasq_config(FILE *f)
{
	DIR *dir;
	struct dirent *file;
	char nv[BUF_SIZE_16];
	char buf[BUF_SIZE];
	char *pos, *fn, ch;
	int num;

	/* add server interfaces to DNS config */
	strlcpy(buf, nvram_safe_get("vpns_dns"), BUF_SIZE);
	for (pos = strtok(buf, ","); pos != NULL; pos = strtok(NULL, ",")) {
		num = atoi(pos);
		if (num) {
			logmsg(LOG_DEBUG, "%s: adding server %d interface to dns config", __FUNCTION__, num);
			snprintf(nv, BUF_SIZE_16, "vpns%d_if", num);
			fprintf(f, "interface=%s%d\n", nvram_safe_get(nv), OVPN_SERVER_BASEIF + num);
		}
	}

	/* open DNS directory */
	dir = opendir(OVPN_DNS_DIR);
	if (!dir)
		return;

	while ((file = readdir(dir)) != NULL) {
		fn = file->d_name;

		if (fn[0] == '.')
			continue;

		/* check for .resolv files */
		if (sscanf(fn, "client%d.resol%c", &num, &ch) == 2 && ch == 'v') {
			logmsg(LOG_DEBUG, "%s: checking ADNS settings for client %d", __FUNCTION__, num);
			snprintf(buf, BUF_SIZE, "vpnc%d_adns", num);
			if (nvram_get_int(buf) == 2) {
				logmsg(LOG_INFO, "adding strict-order to dnsmasq config for client %d", num);
				fprintf(f, "strict-order\n");
				break;
			}
		}

		/* check for .conf files */
		if (sscanf(fn, "client%d.con%c", &num, &ch) == 2) {
			memset(buf, 0, BUF_SIZE);
			snprintf(buf, BUF_SIZE, "%s/%s", OVPN_DNS_DIR, fn);
			if (fappend(f, buf) == -1) {
				logmsg(LOG_WARNING, "fappend failed for %s (%s)", buf, strerror(errno));
				continue;
			}

			logmsg(LOG_INFO, "adding Dnsmasq config from %s", fn);
		}
	}
	closedir(dir);
}

int write_ovpn_resolv(FILE *f)
{
	DIR *dir;
	struct dirent *file;
	char buf[BUF_SIZE];
	char *fn, ch;
	int num, exclusive = 0;

	/* open DNS directory */
	dir = opendir(OVPN_DNS_DIR);
	if (!dir)
		return 0;

	while ((file = readdir(dir)) != NULL) {
		fn = file->d_name;

		if (fn[0] == '.')
			continue;

		if (sscanf(fn, "client%d.resol%c", &num, &ch) == 2 && ch == 'v') {
			memset(buf, 0, BUF_SIZE);
			snprintf(buf, BUF_SIZE, "%s/%s", OVPN_DNS_DIR, fn);
			if (fappend(f, buf) == -1) {
				logmsg(LOG_WARNING, "fappend failed for %s (%s)", buf, strerror(errno));
				continue;
			}

			logmsg(LOG_INFO, "%s: adding DNS entries from %s", __FUNCTION__, fn);

			snprintf(buf, BUF_SIZE, "vpnc%d_adns", num);
			if (nvram_get_int(buf) == 3)
				exclusive = 1;
		}
	}
	closedir(dir);

	return exclusive;
}
