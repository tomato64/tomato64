/*
 *
 * Copyright (C) 2023 - 2025 FreshTomato
 * https://freshtomato.org/
 *
 * For use with FreshTomato Firmware only.
 * No part of this file may be used without permission.
 *
 * Fixes/updates (C) 2023 - 2025 pedro
 *
 */


#include "rc.h"
#include "curve25519.h"

/* needed by logmsg() */
#define LOGMSG_DISABLE		DISABLE_SYSLOG_OSM
#define LOGMSG_NVDEBUG		"wireguard_debug"

#define BUF_SIZE		256
#define BUF_SIZE_8		8
#define BUF_SIZE_16		16
#define BUF_SIZE_32		32
#define BUF_SIZE_64		64
#define IF_SIZE			8
#define PEER_COUNT		3

/* uncomment to add default routing (also in patches/wireguard-tools/101-tomato-specific.patch line 412 - 414) after kernel fix */
#define KERNEL_WG_FIX

/* wireguard routing policy modes (rgwr) */
enum {
	WG_RGW_NONE = 0,
	WG_RGW_ALL,
	WG_RGW_POLICY,
	WG_RGW_POLICY_STRICT
};

char port[BUF_SIZE_8];
char fwmark[BUF_SIZE_16];


static int replace_in_file(const char *filename, const char *old_str, const char *new_str)
{
	FILE *fp_in = NULL;
	FILE *fp_out = NULL;
	char tmp_filename[FILENAME_MAX];
	char buf[4096];
	size_t old_len = strlen(old_str);
	size_t new_len = new_str ? strlen(new_str) : 0;

	if (!(fp_in = fopen(filename, "r"))) {
		logmsg(LOG_WARNING, "cannot open file for reading: %s (%s)", filename, strerror(errno));
		return -1;
	}

	memset(tmp_filename, 0, FILENAME_MAX);
	snprintf(tmp_filename, FILENAME_MAX, "/tmp/%s.tmp", filename);
	if (!(fp_out = fopen(tmp_filename, "w"))) {
		logmsg(LOG_WARNING, "could not create temporary file: %s (%s)", tmp_filename, strerror(errno));
		fclose(fp_in);
		return -1;
	}

	while (fgets(buf, sizeof(buf), fp_in)) {
		char *match = strstr(buf, old_str);
		if (match) {
			if (!new_str) /* delete line */
				continue;

			char *pos = buf;
			while ((match = strstr(pos, old_str))) {
				fwrite(pos, 1, match - pos, fp_out);
				fwrite(new_str, 1, new_len, fp_out);
				pos = match + old_len;
			}
			fputs(pos, fp_out);
		}
		else
			fputs(buf, fp_out);
	}

	fclose(fp_in);
	fclose(fp_out);

	if (rename(tmp_filename, filename) != 0) {
		logmsg(LOG_WARNING, "failed to overwrite %s: %s", filename, strerror(errno));
		eval("rm", "-rf", tmp_filename);
		return -1;
	}

	return 0;
}

static void wg_build_firewall(const int unit, const char *port) {
	FILE *fp;
	char buffer[BUF_SIZE_64];
	char tmp[BUF_SIZE_16];
	char *dns;
	int nvi;

	memset(buffer, 0, BUF_SIZE_64);
	snprintf(buffer, BUF_SIZE_64, WG_FW_DIR"/wg%d-fw.sh", unit);

	/* script with firewall rules (port, unit) */
	/* (..., open wireguard port, accept packets from wireguard internal subnet, set up forwarding) */
	if ((fp = fopen(buffer, "w"))) {
		chains_log_detection();

		fprintf(fp, "#!/bin/sh\n"
		            "\n# FW\n");

		nvi = atoi(getNVRAMVar("wg%d_fw", unit));

		/* Handle firewall rules if appropriate */
		memset(tmp, 0, BUF_SIZE_16);
		snprintf(tmp, BUF_SIZE_16, "wg%d_firewall", unit);
		if (atoi(getNVRAMVar("wg%d_com", unit)) == 3 && !nvram_contains_word(tmp, "custom")) { /* 'External - VPN Provider' & auto */
			fprintf(fp, "iptables -I INPUT -i wg%d -m state --state NEW -j %s\n"
			            "iptables -I FORWARD -i wg%d -m state --state NEW -j %s\n"
			            "iptables -I FORWARD -o wg%d -j ACCEPT\n"
			            "echo 1 > /proc/sys/net/ipv4/conf/all/src_valid_mark\n",
			            unit, (nvi ? chain_in_drop : chain_in_accept),
			            unit, (nvi ? "DROP" : "ACCEPT"),
			            unit);

			if (!nvram_get_int("ctf_disable")) /* bypass CTF if enabled */
				fprintf(fp, "iptables -t mangle -I PREROUTING -i wg%d -j MARK --set-mark 0x01/0x7\n", unit);

			/* masquerade all peer outbound traffic regardless of source subnet */
			if (atoi(getNVRAMVar("wg%d_nat", unit)) == 1)
				fprintf(fp, "iptables -t nat -I POSTROUTING -o wg%d -j MASQUERADE\n", unit);
		}
		else if (atoi(getNVRAMVar("wg%d_com", unit)) != 3) { /* other */
			fprintf(fp, "iptables -A INPUT -p udp --dport %s -j %s\n"
			            "iptables -A INPUT -i wg%d -j %s\n"
			            "iptables -A FORWARD -i wg%d -j ACCEPT\n",
			            port, chain_in_accept,
			            unit, chain_in_accept,
			            unit);

			if (!nvram_get_int("ctf_disable")) /* bypass CTF if enabled */
				fprintf(fp, "iptables -t mangle -I PREROUTING -i wg%d -j MARK --set-mark 0x01/0x7\n", unit);
		}

		dns = getNVRAMVar("wg%d_dns", unit);
		if (getNVRAMVar("wg%d_file", unit)[0] == '\0') { /* only if no optional config file has been added */
			/* script to add/remove fw rules for dns servers (unit, dns) */
			fprintf(fp, "\n# DNS\n"
			            "DNS_CHAIN=\"wg-ft-wg%d-dns\"\n"
			            "DNS_FILE=\"%s/wg%d.conf\"\n"
			            "STATUS=$?\n"
			            "\niptables -nL | grep \"$DNS_CHAIN\" && {\n"
			            " # remove rules\n"
			            " [ -r \"$DNS_FILE\" ] && {\n"
			            "  rm $DNS_FILE\n"
			            "  nohup service dnsmasq restart &\n"
			            "  iptables -D OUTPUT -j $DNS_CHAIN\n"
			            "  iptables -F $DNS_CHAIN\n"
			            "  iptables -X $DNS_CHAIN\n"
			            " }\n"
			            "} || {\n"
			            " # add rules\n"
			            " [ \"%s\" != \"\" ] && {\n"
			            "  > $DNS_FILE\n"
			            "  iptables -N $DNS_CHAIN\n"
			            "  for NAMESERVER in $(echo \"%s\" | tr \",\" \" \" ); do\n"
			            "   echo \"server=$NAMESERVER\" >> $DNS_FILE\n"
			            "   iptables -A $DNS_CHAIN -i wg%d -p tcp --dst $NAMESERVER/32 --dport 53 -j ACCEPT\n"
			            "   iptables -A $DNS_CHAIN -i wg%d -p udp --dst $NAMESERVER/32 --dport 53 -j ACCEPT\n"
			            "  done\n"
			            "  iptables -A OUTPUT -j $DNS_CHAIN\n"
			            "  [ $? -eq 0 ] && nohup service dnsmasq restart &\n"
			            " } || {\n"
			            "  exit $STATUS\n"
			            " }\n"
			            "}\n",
			            unit,
			            WG_DNS_DIR, unit,
			            dns,
			            dns,
			            unit,
			            unit);
		}
		fclose(fp);
		chmod(buffer, (S_IRUSR | S_IWUSR | S_IXUSR));
	}
}

static int wg_quick_iface(char *iface, const char *file, const int up)
{
	char buffer[BUF_SIZE_32];
	char *up_down = (up == 1 ? "up" : "down");
	FILE *f;

	/* copy config to wg dir with proper name */
	memset(buffer, 0, BUF_SIZE_32);
	snprintf(buffer, BUF_SIZE_32, WG_DIR"/%s.conf", iface);
	if ((f = fopen(buffer, "w")) != NULL) {
		fappend(f, file);
		fclose(f);
	}
	else
		logmsg(LOG_WARNING, "unable to open wireguard configuration file %s for interface %s!", iface, file);

	/* set up/down wireguard IF */
	if (eval("wg-quick", up_down, buffer, iface, "--norestart")) {
		logmsg(LOG_WARNING, "unable to set %s wireguard interface %s from file %s!", up_down, iface, file);
		return -1;
	}
	else
		logmsg(LOG_DEBUG, "wireguard interface %s from file %s set %s", iface, file, up_down);

	stop_dnsmasq();
	start_dnsmasq();

	return 0;
}

static void wg_find_port(const int unit, char *port)
{
	char *b;

	b = getNVRAMVar("wg%d_port", unit);
	memset(port, 0, BUF_SIZE_8);
	if (b[0] == '\0')
		snprintf(port, BUF_SIZE_8, "%d", 51820 + unit);
	else
		snprintf(port, BUF_SIZE_8, "%s", b);
}

static void wg_find_fwmark(const int unit, char *port, char *fwmark)
{
	char *b;

	b = getNVRAMVar("wg%d_fwmark", unit);
	memset(fwmark, 0, BUF_SIZE_16);
	if (b[0] == '\0' || b[0] == '0')
		snprintf(fwmark, BUF_SIZE_16, "%s", port);
	else
		snprintf(fwmark, BUF_SIZE_16, "%s", b);
}

static int wg_if_exist(const char *ifname)
{
	return if_nametoindex(ifname) ? 1 : 0;
}

static void wg_setup_dirs(void) {
	/* main dir */
	if (mkdir_if_none(WG_DIR))
		chmod(WG_DIR, (S_IRUSR | S_IWUSR | S_IXUSR));

	/* script dir */
	if (mkdir_if_none(WG_SCRIPTS_DIR))
		chmod(WG_SCRIPTS_DIR, (S_IRUSR | S_IWUSR | S_IXUSR));

	/* keys dir */
	if (mkdir_if_none(WG_KEYS_DIR))
		chmod(WG_KEYS_DIR, (S_IRUSR | S_IWUSR | S_IXUSR));

	/* dns dir */
	if (mkdir_if_none(WG_DNS_DIR))
		chmod(WG_DNS_DIR, (S_IRUSR | S_IWUSR | S_IXUSR));

	/* FW dir */
	if (mkdir_if_none(WG_FW_DIR))
		chmod(WG_FW_DIR, (S_IRUSR | S_IWUSR | S_IXUSR));
}

static void wg_cleanup_dirs(void) {
	eval("rm", "-rf", WG_DIR);
}

static void wg_setup_watchdog(const int unit)
{
	FILE *fp;
	char buffer[BUF_SIZE_64], buffer2[BUF_SIZE_64];
	char taskname[BUF_SIZE_32];
	int nvi;

	if ((nvi = atoi(getNVRAMVar("wg%d_poll", unit))) > 0) {
		memset(buffer, 0, BUF_SIZE_64);
		snprintf(buffer, BUF_SIZE_64, WG_SCRIPTS_DIR"/watchdog%d.sh", unit);

		if ((fp = fopen(buffer, "w"))) {
			fprintf(fp, "#!/bin/sh\n"
			            "pingme() {\n"
			            "[ \"3\" != \"%d\" ] && return 0\n"
			            " local i=1\n"
			            " while :; do\n"
			            "  ping -qc1 -W3 -I wg%d 1.1.1.1 &>/dev/null && return 0\n"
			            "  [ $((i++)) -ge 3 ] && break || sleep 5\n"
			            " done\n"
			            " return 1\n"
			            "}\n"
			            "ISUP=$(cat /sys/class/net/wg%d/operstate)\n"
			            "[ \"$(nvram get g_upgrade)\" != \"1\" -a \"$(nvram get g_reboot)\" != \"1\" ] && {\n"
			            " [ \"$ISUP\" == \"unknown\" -o \"$ISUP\" == \"up\" ] && pingme && exit 0\n"
			            " logger -t wg-watchdog wg%d stopped? Starting...\n"
			            " service wireguard%d restart\n"
			            "}\n",
			            atoi(getNVRAMVar("wg%d_com", unit)), /* only for 'External' (3) mode */
			            unit,
			            unit,
			            unit,
			            unit);
			fclose(fp);
			chmod(buffer, (S_IRUSR | S_IWUSR | S_IXUSR));

			memset(taskname, 0, BUF_SIZE_32);
			snprintf(taskname, BUF_SIZE_32,"CheckWireguard%d", unit);
			memset(buffer2, 0, BUF_SIZE_64);
			snprintf(buffer2, BUF_SIZE_64, "*/%d * * * * %s", nvi, buffer);
			eval("cru", "a", taskname, buffer2);
		}
	}
}

static int wg_create_iface(char *iface)
{
	/* Make sure module is loaded */
	modprobe("wireguard");
	f_wait_exists("/sys/module/wireguard", 5);

	/* Create wireguard interface */
	if (eval("ip", "link", "add", "dev", iface, "type", "wireguard")) {
		logmsg(LOG_WARNING, "unable to create wireguard interface %s!", iface);
		return -1;
	}
	else
		logmsg(LOG_DEBUG, "wireguard interface %s has been created", iface);

	return 0;
}

static int wg_set_iface_addr(char *iface, const char *addr)
{
	char *nv, *b;

	/* Flush all addresses from interface */
	if (eval("ip", "addr", "flush", "dev", iface)) {
		logmsg(LOG_WARNING, "unable to flush wireguard interface %s!", iface);
		return -1;
	}
	else
		logmsg(LOG_DEBUG, "successfully flushed wireguard interface %s", iface);

	/* Set wireguard interface address(es) */
	nv = strdup(addr);
	while ((b = strsep(&nv, ",")) != NULL) {
		if (eval("ip", "addr", "add", b, "dev", iface)) {
			logmsg(LOG_WARNING, "unable to add wireguard interface %s address of %s!", iface, b);
			return -1;
		}
		else
			logmsg(LOG_DEBUG, "wireguard interface %s has had address %s add to it", iface, b);
	}

	if (nv)
		free(nv);

	return 0;
}

static int wg_set_iface_port(char *iface, char *port)
{
	if (eval("wg", "set", iface, "listen-port", port)) {
		logmsg(LOG_WARNING, "unable to set wireguard interface %s port to %s!", iface, port);
		return -1;
	}
	else
		logmsg(LOG_DEBUG, "wireguard interface %s has had its port set to %s", iface, port);

	return 0;
}

static int wg_set_iface_privkey(char *iface, const char *privkey)
{
	FILE *fp;
	char buffer[BUF_SIZE];

	/* write private key to file */
	memset(buffer, 0, BUF_SIZE);
	snprintf(buffer, BUF_SIZE, WG_KEYS_DIR"/%s", iface);

	fp = fopen(buffer, "w");
	fprintf(fp, privkey);
	fclose(fp);

	chmod(buffer, (S_IRUSR | S_IWUSR));

	/* set interface private key */
	if (eval("wg", "set", iface, "private-key", buffer)) {
		logmsg(LOG_WARNING, "unable to set wireguard interface %s private key!", iface);
		return -1;
	}
	else
		logmsg(LOG_DEBUG, "wireguard interface %s has had its private key set", iface);

	/* remove file for security */
	remove(buffer);

	return 0;
}

static int wg_set_iface_fwmark(char *iface, char *fwmark)
{
	char buffer[BUF_SIZE_16];
	memset(buffer, 0, BUF_SIZE_16);

	if (fwmark[0] == '0' && fwmark[1] == '\0')
		snprintf(buffer, BUF_SIZE_16, "%s", fwmark);
	else
		snprintf(buffer, BUF_SIZE_16, "0x%s", fwmark);

	if (eval("wg", "set", iface, "fwmark", fwmark)) {
		logmsg(LOG_WARNING, "unable to set wireguard interface %s fwmark to %s!", iface, fwmark);
		return -1;
	}
	else
		logmsg(LOG_DEBUG, "wireguard interface %s has had its fwmark set to %s", iface, fwmark);

	return 0;
}

static int wg_set_iface_mtu(char *iface, char *mtu)
{
	if (eval("ip", "link", "set", "dev", iface, "mtu", mtu)) {
		logmsg(LOG_WARNING, "unable to set wireguard interface %s mtu to %s!", iface, mtu);
		return -1;
	}
	else
		logmsg(LOG_DEBUG, "wireguard interface %s has had its mtu set to %s", iface, mtu);

	return 0;
}

static int wg_set_iface_up(char *iface)
{
	int retry = 0;

	while (retry < 5) {
		if (!(eval("ip", "link", "set", "up", "dev", iface))) {
			logmsg(LOG_DEBUG, "wireguard interface %s has been brought up", iface);
			return 0;
		}
		else if (retry < 4) {
			logmsg(LOG_WARNING, "unable to bring up wireguard interface %s, retrying %d ...", iface, retry + 1);
			sleep(4);
		}
		retry += 1;
	}

	logmsg(LOG_WARNING, "unable to bring up wireguard interface %s!", iface);

	return -1;
}

static int wg_iface_script(const int unit, const char *script_name)
{
	char *script;
	char buffer[BUF_SIZE_32];
	char path[BUF_SIZE_64];
	FILE *fp;

	memset(buffer, 0, BUF_SIZE_32);
	snprintf(buffer, BUF_SIZE_32, "wg%d_%s", unit, script_name);

	script = nvram_safe_get(buffer);

	if (strcmp(script, "") != 0) {
		/* build path */
		memset(path, 0, BUF_SIZE_64);
		snprintf(path, BUF_SIZE_64, WG_SCRIPTS_DIR"/wg%d-%s.sh", unit, script_name);

		if (!(fp = fopen(path, "w"))) {
			logmsg(LOG_WARNING, "unable to open %s for writing!", path);
			return -1;
		}
		fprintf(fp, "#!/bin/sh\n%s\n", script);
		fclose(fp);
		chmod(path, 0700);

		/* replace %i with interface */
		memset(buffer, 0, BUF_SIZE_32);
		snprintf(buffer, BUF_SIZE_32, "wg%d", unit);

		if (replace_in_file(path, "%i", buffer) != 0) {
			logmsg(LOG_WARNING, "unable to substitute interface name in %s script for wireguard interface wg%d!", script_name, unit);
			return -1;
		}
		else
			logmsg(LOG_DEBUG, "interface substitution in %s script for wireguard interface wg%d has executed successfully", script_name, unit);

		/* run script */
		if (eval(path)) {
			logmsg(LOG_WARNING, "unable to execute %s script for wireguard interface wg%d!", script_name, unit);
			return -1;
		}
		else
			logmsg(LOG_DEBUG, "%s script for wireguard interface wg%d has executed successfully", script_name, unit);
	}

	return 0;
}

static void wg_iface_pre_up(const int unit)
{
	wg_iface_script(unit, "preup");
}

static void wg_iface_post_up(const int unit)
{
	wg_iface_script(unit, "postup");
}

static void wg_iface_pre_down(const int unit)
{
	wg_iface_script(unit, "predown");
}

static void wg_iface_post_down(const int unit)
{
	wg_iface_script(unit, "postdown");
}

static int wg_set_peer_psk(char *iface, char *pubkey, const char *presharedkey)
{
	FILE *fp;
	char buffer[BUF_SIZE];
	int err = 0;

	/* write preshared key to file */
	memset(buffer, 0, BUF_SIZE);
	snprintf(buffer, BUF_SIZE, WG_KEYS_DIR"/%s.psk", iface);

	fp = fopen(buffer, "w");
	fprintf(fp, presharedkey);
	fclose(fp);

	if (eval("wg", "set", iface, "peer", pubkey, "preshared-key", buffer)) {
		logmsg(LOG_WARNING, "unable to add preshared key to peer %s on wireguard interface %s!", pubkey, iface);
		err = -1;
	}
	else
		logmsg(LOG_DEBUG, "preshared key has been added to peer %s on wireguard interface %s", pubkey, iface);

	/* remove file for security */
	remove(buffer);
	return err;
}

static int wg_set_peer_keepalive(char *iface, char *pubkey, char *keepalive)
{
	if (eval("wg", "set", iface, "peer", pubkey, "persistent-keepalive", keepalive)) {
		logmsg(LOG_WARNING, "unable to add persistent-keepalive of %s to peer %s on wireguard interface %s!", keepalive, pubkey, iface);
		return -1;
	}
	else
		logmsg(LOG_DEBUG, "persistent-keepalive of %s has been added to peer %s on wireguard interface %s", keepalive, pubkey, iface);

	return 0;
}

static int wg_set_peer_endpoint(const int unit, char *iface, char *pubkey, const char *endpoint, const char *port)
{
	char buffer[BUF_SIZE_64];

	memset(buffer, 0, BUF_SIZE_64);

	if (atoi(getNVRAMVar("wg%d_com", unit)) == 3) /* 'External - VPN Provider' */
		snprintf(buffer, BUF_SIZE_64, "%s", endpoint);
	else
		snprintf(buffer, BUF_SIZE_64, "%s:%s", endpoint, port);

	if (eval("wg", "set", iface, "peer", pubkey, "endpoint", buffer)) {
		logmsg(LOG_WARNING, "unable to add endpoint of %s to peer %s on wireguard interface %s!", buffer, pubkey, iface);
		return -1;
	}
	else
		logmsg(LOG_DEBUG, "endpoint of %s has been added to peer %s on wireguard interface %s", buffer, pubkey, iface);

	return 0;
}

static int wg_route_peer(char *iface, char *route, char *table, const int add)
{
	if (add == 1) {
		if (table != NULL) {
			if (eval("ip", "route", "add", route, "dev", iface, "table", table)) {
				logmsg(LOG_WARNING, "unable to add route of %s to table %s for wireguard interface %s! When using mask, check if the entry is correct (for the /24-31 mask the last IP octet must be 0, for the /9-16 mask the last two octets must be 0, etc.)", route, table, iface);
				return -1;
			}
			else
				logmsg(LOG_DEBUG, "wireguard interface %s has had a route added to table %s for %s", iface, table, route);
		}
		else {
			if (eval("ip", "route", "add", route, "dev", iface)) {
				logmsg(LOG_WARNING, "unable to add route of %s for wireguard interface %s! When using mask, check if the entry is correct (for the /24-31 mask the last IP octet must be 0, for the /9-16 mask the last two octets must be 0, etc.)", route, iface);
				return -1;
			}
			else
				logmsg(LOG_DEBUG, "wireguard interface %s has had a route added for %s", iface, route);
		}
	}
	else {
		if (table != NULL) {
			if (eval("ip", "route", "delete", route, "dev", iface, "table", table)) {
				logmsg(LOG_WARNING, "unable to remove route of %s to table %s for wireguard interface %s!", route, table, iface);
				return -1;
			}
			else
				logmsg(LOG_DEBUG, "wireguard interface %s has had a route removed to table %s for %s", iface, table, route);
		}
		else {
			if (eval("ip", "route", "delete", route, "dev", iface)) {
				logmsg(LOG_WARNING, "unable to remove route of %s for wireguard interface %s!", route, iface);
				return -1;
			}
			else
				logmsg(LOG_DEBUG, "wireguard interface %s has had a route removed for %s", iface, route);
		}
	}
	return 0;
}

static void wg_route_peer_default(char *iface, char *route, char *fwmark, int add)
{
#ifndef KERNEL_WG_FIX
	int i;
	char cmd[BUF_SIZE];
	char buffer[BUF_SIZE_32];
	char filename[BUF_SIZE_32];
	FILE *fp = NULL;
#endif

	if (add == 1) {
#ifdef KERNEL_WG_FIX
		wg_route_peer(iface, route, fwmark, 1);

		if (eval("ip", "rule", "add", "not", "fwmark", fwmark, "table", fwmark))
			logmsg(LOG_WARNING, "unable to filter fwmark %s for default route of %s on wireguard interface %s!", fwmark, route, iface);

		if (eval("ip", "rule", "add", "table", "main", "suppress_prefixlength", "0"))
			logmsg(LOG_WARNING, "unable to suppress prefix length of 0 for default route of %s on wireguard interface %s!", route, iface);
#else
		for (i = 0; i < BRIDGE_COUNT; i++) {
			memset(filename, 0, BUF_SIZE_32);
			snprintf(filename, BUF_SIZE_32, "/tmp/wg%d_route_tmp", i);
			memset(cmd, 0, BUF_SIZE);
			snprintf(cmd, BUF_SIZE, "ip route show dev br%d | cut -d' ' -f1 >%s", i, filename);
			system(cmd);
			if ((fp = fopen(filename, "r"))) {
				memset(buffer, 0, BUF_SIZE_32);
				fgets(buffer, BUF_SIZE_32, fp);
				buffer[strcspn(buffer, "\n")] = 0;
				if (strlen(buffer) > 1) {
					memset(cmd, 0, BUF_SIZE);
					snprintf(cmd, BUF_SIZE, "ip route add %s dev br%d table %s", buffer, i, fwmark);
					system(cmd);
				}
				fclose(fp);
			}
			unlink(filename);
		}

		wg_route_peer(iface, route, fwmark, 1);

		if (eval("ip", "rule", "add", "not", "fwmark", fwmark, "table", fwmark))
			logmsg(LOG_WARNING, "unable to filter fwmark %s for default route of %s on wireguard interface %s!", fwmark, route, iface);
#endif
	}
	else {
#ifdef KERNEL_WG_FIX
		if (eval("ip", "rule", "delete", "table", "main", "suppress_prefixlength", "0"))
			logmsg(LOG_WARNING, "unable to remove suppress prefix length of 0 for default route of %s on wireguard interface %s!", route, iface);

		if (eval("ip", "rule", "delete", "not", "from", "all", "fwmark", fwmark, "lookup", fwmark))
			logmsg(LOG_WARNING, "unable to remove filter fwmark %s for default route of %s on wireguard interface %s!", fwmark, route, iface);

		wg_route_peer(iface, route, fwmark, 0);
#else
		if (eval("ip", "rule", "delete", "not", "from", "all", "fwmark", fwmark, "lookup", fwmark))
			logmsg(LOG_WARNING, "unable to remove filter fwmark %s for default route of %s on wireguard interface %s!", fwmark, route, iface);

		wg_route_peer(iface, route, fwmark, 0);

		for (i = 0; i < BRIDGE_COUNT; i++) {
			memset(filename, 0, BUF_SIZE_32);
			snprintf(filename, BUF_SIZE_32, "/tmp/wg%d_route_tmp", i);
			memset(cmd, 0, BUF_SIZE);
			snprintf(cmd, BUF_SIZE, "ip route show dev br%d | cut -d' ' -f1 >%s", i, filename);
			system(cmd);
			if ((fp = fopen(filename, "r"))) {
				memset(buffer, 0, BUF_SIZE);
				fgets(buffer, BUF_SIZE, fp);
				buffer[strcspn(buffer, "\n")] = 0;
				if (strlen(buffer) > 1) {
					memset(cmd, 0, BUF_SIZE);
					snprintf(cmd, BUF_SIZE, "ip route delete %s dev br%d table %s", buffer, i, fwmark);
					system(cmd);
				}
				fclose(fp);
			}
			unlink(filename);
		}
#endif
	}
}

static void wg_route_peer_allowed_ips(const int unit, char *iface, const char *allowed_ips, const char *fwmark, int add)
{
	char *aip, *b, *table, *rt, *tp, *ip, *nm;
	int route_type = 1;
	char buffer[BUF_SIZE_32];

	tp = b = strdup(getNVRAMVar("wg%d_route", unit));
	if (tp) {
		if (vstrsep(b, "|", &rt, &table) < 3)
			route_type = atoi(rt);

		free(tp);
	}

	/* check which routing type the user specified */
	if (route_type > 0) { /* !off */
		aip = strdup(allowed_ips);
		while ((b = strsep(&aip, ",")) != NULL) {
			memset(buffer, 0, BUF_SIZE_32);
			snprintf(buffer, BUF_SIZE_32, "%s", b);

			if ((vstrsep(b, "/", &ip, &nm) == 2) && (atoi(nm) == 0)) { /* default route */
				logmsg(LOG_DEBUG, "*** %s: running wg_route_peer_default() iface=[%s] route=[%s] fwmark=[%s]", __FUNCTION__, iface, buffer, fwmark);
				wg_route_peer_default(iface, buffer, (char *)fwmark, add);
			}
			else { /* std route */
				logmsg(LOG_DEBUG, "*** %s: running wg_route_peer() iface=[%s] route=[%s] table=[%s]", __FUNCTION__, iface, buffer, table);
				wg_route_peer(iface, buffer, (route_type == 1 ? NULL : table), add);
			}
		}
		if (aip)
			free(aip);
	}
}

static void wg_set_peer_allowed_ips(char *iface, char *pubkey, char *allowed_ips, const char *fwmark)
{
	if (eval("wg", "set", iface, "peer", pubkey, "allowed-ips", allowed_ips))
		logmsg(LOG_WARNING, "unable to add allowed ips %s for peer %s to wireguard interface %s!", allowed_ips, pubkey, iface);
	else
		logmsg(LOG_DEBUG, "peer %s for wireguard interface %s has had its allowed ips set to %s", pubkey, iface, allowed_ips);
}

static void wg_add_peer(const int unit, char *iface, char *pubkey, char *allowed_ips, const char *presharedkey, char *keepalive, const char *endpoint, const char *fwmark, const char *port)
{
	/* set allowed ips / create peer */
	wg_set_peer_allowed_ips(iface, pubkey, allowed_ips, fwmark);

	/* set peer psk */
	if (presharedkey[0] != '\0')
		wg_set_peer_psk(iface, pubkey, presharedkey);

	/* set peer keepalive */
	if (atoi(keepalive) > 0)
		wg_set_peer_keepalive(iface, pubkey, keepalive);

	/* set peer endpoint */
	if (endpoint[0] != '\0')
		wg_set_peer_endpoint(unit, iface, pubkey, endpoint, port);

	/* add routes (also default route if any) */
	wg_route_peer_allowed_ips(unit, iface, allowed_ips, fwmark, 1); /* 1 = add */
}

static inline int decode_base64(const char src[static 4])
{
	int val = 0;
	unsigned int i;

	for (i = 0; i < 4; ++i)
		val |= (-1
			    + ((((('A' - 1) - src[i]) & (src[i] - ('Z' + 1))) >> 8) & (src[i] - 64))
			    + ((((('a' - 1) - src[i]) & (src[i] - ('z' + 1))) >> 8) & (src[i] - 70))
			    + ((((('0' - 1) - src[i]) & (src[i] - ('9' + 1))) >> 8) & (src[i] + 5))
			    + ((((('+' - 1) - src[i]) & (src[i] - ('+' + 1))) >> 8) & 63)
			    + ((((('/' - 1) - src[i]) & (src[i] - ('/' + 1))) >> 8) & 64)
			) << (18 - 6 * i);
	return val;
}

static inline void encode_base64(char dest[static 4], const uint8_t src[static 3])
{
	const uint8_t input[] = { (src[0] >> 2) & 63, ((src[0] << 4) | (src[1] >> 4)) & 63, ((src[1] << 2) | (src[2] >> 6)) & 63, src[2] & 63 };
	unsigned int i;

	for (i = 0; i < 4; ++i)
		dest[i] = input[i] + 'A'
			  + (((25 - input[i]) >> 8) & 6)
			  - (((51 - input[i]) >> 8) & 75)
			  - (((61 - input[i]) >> 8) & 15)
			  + (((62 - input[i]) >> 8) & 3);
}

static void key_to_base64(char base64[static WG_KEY_LEN_BASE64], const uint8_t key[static WG_KEY_LEN])
{
	unsigned int i;

	for (i = 0; i < WG_KEY_LEN / 3; ++i)
		encode_base64(&base64[i * 4], &key[i * 3]);

	encode_base64(&base64[i * 4], (const uint8_t[]){ key[i * 3 + 0], key[i * 3 + 1], 0 });
	base64[WG_KEY_LEN_BASE64 - 2] = '=';
	base64[WG_KEY_LEN_BASE64 - 1] = '\0';
}

static bool key_from_base64(uint8_t key[static WG_KEY_LEN], const char *base64)
{
	unsigned int i;
	volatile uint8_t ret = 0;
	int val;

	if (strlen(base64) != WG_KEY_LEN_BASE64 - 1 || base64[WG_KEY_LEN_BASE64 - 2] != '=')
		return FALSE;

	for (i = 0; i < WG_KEY_LEN / 3; ++i) {
		val = decode_base64(&base64[i * 4]);
		ret |= (uint32_t)val >> 31;
		key[i * 3 + 0] = (val >> 16) & 0xff;
		key[i * 3 + 1] = (val >> 8) & 0xff;
		key[i * 3 + 2] = val & 0xff;
	}
	val = decode_base64((const char[]){ base64[i * 4 + 0], base64[i * 4 + 1], base64[i * 4 + 2], 'A' });
	ret |= ((uint32_t)val >> 31) | (val & 0xff);
	key[i * 3 + 0] = (val >> 16) & 0xff;
	key[i * 3 + 1] = (val >> 8) & 0xff;

	return 1 & ((ret - 1) >> 8);
}

static void wg_pubkey(const char *privkey, char *pubkey)
{
	uint8_t key[WG_KEY_LEN] __attribute__((aligned(sizeof(uintptr_t))));

	key_from_base64(key, privkey);
	curve25519_generate_public(key, key);
	key_to_base64(pubkey, key);
}

static void wg_add_peer_privkey(const int unit, char *iface, const char *privkey, char *allowed_ips, const char *presharedkey, char *keepalive, const char *endpoint, const char *fwmark)
{
	char pubkey[BUF_SIZE_64];

	memset(pubkey, 0, BUF_SIZE_64);
	wg_pubkey(privkey, pubkey);

	wg_add_peer(unit, iface, pubkey, allowed_ips, presharedkey, keepalive, endpoint, fwmark, port);
}

static void wg_remove_peer(const int unit, char *iface, char *pubkey, char *allowed_ips, const char *fwmark)
{
	if (eval("wg", "set", iface, "peer", pubkey, "remove"))
		logmsg(LOG_WARNING, "unable to remove peer %s from wireguard interface %s!", iface, pubkey);
	else
		logmsg(LOG_DEBUG, "peer %s has been removed from wireguard interface %s", iface, pubkey);

	/* remove routes (also default route if any) */
	wg_route_peer_allowed_ips(unit, iface, allowed_ips, fwmark, 0); /* 0 = remove */
}

static void wg_remove_peer_privkey(const int unit, char *iface, char *privkey, char *allowed_ips, const char *fwmark)
{
	char pubkey[BUF_SIZE_64];
	memset(pubkey, 0, BUF_SIZE_64);

	wg_pubkey(privkey, pubkey);

	wg_remove_peer(unit, iface, pubkey, allowed_ips, fwmark);
}

static int wg_set_iface_down(char *iface)
{
	/* check if interface exists */
	if (wg_if_exist(iface)) {
		if (eval("ip", "link", "set", "down", "dev", iface)) {
			logmsg(LOG_WARNING, "failed to bring down wireGuard interface %s", iface);
			return -1;
		}
		else
			logmsg(LOG_DEBUG, "wireguard interface %s has been brought down", iface);
	}

	return 0;
}

static int wg_remove_iface(char *iface)
{
	/* check if interface exists */
	if (wg_if_exist(iface)) {
		/* delete wireguard interface */
		if (eval("ip", "link", "del", "dev", iface)) {
			logmsg(LOG_WARNING, "unable to delete wireguard interface %s!", iface);
			return -1;
		}
		else
			logmsg(LOG_DEBUG, "wireguard interface %s has been deleted", iface);
	}
	else
		logmsg(LOG_DEBUG, "no such interface: %s", iface);

	return 0;
}

void start_wg_eas(void)
{
	int unit;
	int externalall_mode = 0;

	for (unit = 0; unit < WG_INTERFACE_MAX; unit++) {
		if (atoi(getNVRAMVar("wg%d_enable", unit)) == 1) {
			if (atoi(getNVRAMVar("wg%d_com", unit)) == 3 && atoi(getNVRAMVar("wg%d_rgwr", unit)) == WG_RGW_ALL) { /* check for 'External - VPN Provider' mode with "Redirect Internet traffic" set to "All" on this unit */
				if (externalall_mode == 0) { /* no previous unit is in this mode - allow */
					start_wireguard(unit);
					externalall_mode++;
				}
				else
					logmsg(LOG_WARNING, "only one wireguard instance can be run in 'External - VPN Provider' mode with 'Redirect Internet traffic' set to 'All' (currently up: wg%d)! Aborting ...", unit);
			}
			else
				start_wireguard(unit);
		}
	}
}
/*
void stop_wg_eas(void)
{
	int unit;

	for (unit = 0; unit < WG_INTERFACE_MAX; unit++) {
		stop_wireguard(unit);
	}
}
*/
void stop_wg_all(void)
{
	int unit;
	char iface[IF_SIZE];

	for (unit = 0; unit < WG_INTERFACE_MAX; unit++) {
		memset(iface, 0, IF_SIZE);
		snprintf(iface, IF_SIZE, "wg%d", unit);
		if (wg_if_exist(iface))
			stop_wireguard(unit);
		else
			logmsg(LOG_DEBUG, "no such wg instance to stop: %s", iface);
	}
	wg_cleanup_dirs();

	/* remove tunnel interface module */
	modprobe_r("wireguard");
}

void start_wireguard(const int unit)
{
	char *nv, *nvp, *rka, *b;
	char *priv, *name, *key, *psk, *ip, *ka, *aip, *ep;
	char iface[IF_SIZE];
	char buffer[BUF_SIZE];
	int mode;

	memset(buffer, 0, BUF_SIZE);
	snprintf(buffer, BUF_SIZE, "wireguard%d", unit);
	if (serialize_restart(buffer, 1))
		return;

	/* determine interface */
	memset(iface, 0, IF_SIZE);
	snprintf(iface, IF_SIZE, "wg%d", unit);

	/* prepare port value */
	wg_find_port(unit, port);

	/* set up directories for later use */
	wg_setup_dirs();

	/* check if file is specified */
	if (getNVRAMVar("wg%d_file", unit)[0] != '\0') {
		if (wg_quick_iface(iface, getNVRAMVar("wg%d_file", unit), 1))
			goto out;
	}
	else {
		/* create interface */
		if (wg_create_iface(iface))
			goto out;

		/* set interface address */
		if (wg_set_iface_addr(iface, getNVRAMVar("wg%d_ip", unit)))
			goto out;

		/* set interface port */
		if (wg_set_iface_port(iface, port))
			goto out;

		/* set interface private key */
		if (wg_set_iface_privkey(iface, getNVRAMVar("wg%d_key", unit)))
			goto out;

		/* set interface fwmark */
		wg_find_fwmark(unit, port, fwmark);
		if (wg_set_iface_fwmark(iface, fwmark))
			goto out;

		/* set interface mtu */
		if (wg_set_iface_mtu(iface, getNVRAMVar("wg%d_mtu", unit)))
			goto out;

		/* check for keepalives from the router */
		rka = getNVRAMVar("wg%d_ka", unit);

		/* bring up interface */
		wg_iface_pre_up(unit);
		if (wg_set_iface_up(iface))
			goto out;

		/* add stored peers */
		nvp = nv = strdup(getNVRAMVar("wg%d_peers", unit));
		if (nv) {
			mode = atoi(getNVRAMVar("wg%d_com", unit));

			while ((b = strsep(&nvp, ">")) != NULL) {
				if (vstrsep(b, "<", &priv, &name, &ep, &key, &psk, &ip, &aip, &ka) < 8)
					continue;

				/* build peer allowed ips */
				memset(buffer, 0, BUF_SIZE);
				if (aip[0] == '\0')
					snprintf(buffer, BUF_SIZE, "%s", ip);
				else if (ip[0] == '\0')
					snprintf(buffer, BUF_SIZE, "%s", aip);
				else
					snprintf(buffer, BUF_SIZE, "%s,%s", ip, aip);

				/* add peer to interface (and route) */
				if (priv[0] == '1') /* peer has private key? */
					wg_add_peer_privkey(unit, iface, key, buffer, psk, (mode == 3 ? ka : rka), ep, fwmark);
				else
					wg_add_peer(unit, iface, key, buffer, psk, (mode == 3 ? ka : rka), ep, fwmark, port);
			}
		}
		if (nvp)
			free(nvp);

		eval("ip", "route", "flush", "cache");

		/* run post up scripts */
		wg_iface_post_up(unit);
	}

	/* create firewall script & DNS rules */
	wg_build_firewall(unit, port);

	/* firewall + dns rules */
	memset(buffer, 0, BUF_SIZE);
	snprintf(buffer, BUF_SIZE, WG_FW_DIR"/%s-fw.sh", iface);

	/* first remove existing firewall rule(s) */
	run_del_firewall_script(buffer, WG_DIR_DEL_SCRIPT);

	/* then add firewall rule(s) */
	if (eval(buffer))
		logmsg(LOG_WARNING, "unable to add iptable rules for wireguard interface %s on port %s!", iface, port);
	else
		logmsg(LOG_DEBUG, "iptable rules have been added for wireguard interface %s on port %s", iface, port);

	wg_setup_watchdog(unit);

	logmsg(LOG_INFO, "wireguard (%s) started", iface);

	return;

out:
	stop_wireguard(unit);
}

void stop_wireguard(const int unit)
{
	char *nv, *nvp, *b;
	char *priv, *name, *key, *psk, *ip, *ka, *aip, *ep;
	char iface[IF_SIZE];
	char buffer[BUF_SIZE];
	int is_dev;

	memset(buffer, 0, BUF_SIZE);
	snprintf(buffer, BUF_SIZE, "wireguard%d", unit);
	if (serialize_restart(buffer, 0))
		return;

	/* remove cron job */
	memset(buffer, 0, BUF_SIZE);
	snprintf(buffer, BUF_SIZE, "CheckWireguard%d", unit);
	eval("cru", "d", buffer);

	/* remove watchdog file */
	memset(buffer, 0, BUF_SIZE);
	snprintf(buffer, BUF_SIZE, WG_SCRIPTS_DIR"/watchdog%d.sh", unit);
	eval("rm", "-rf", buffer);

	/* determine interface */
	memset(iface, 0, IF_SIZE);
	snprintf(iface, IF_SIZE, "wg%d", unit);

	is_dev = wg_if_exist(iface);

	if (getNVRAMVar("wg%d_file", unit)[0] != '\0')
		wg_quick_iface(iface, getNVRAMVar("wg%d_file", unit), 0);
	else {
		/* prepare port value */
		wg_find_port(unit, port);

		/* prepare fwmark value */
		wg_find_fwmark(unit, port, fwmark);

		wg_iface_pre_down(unit);

		/* remove peers */
		nvp = nv = strdup(getNVRAMVar("wg%d_peers", unit));
		if (nv) {
			while ((b = strsep(&nvp, ">")) != NULL) {
				if (vstrsep(b, "<", &priv, &name, &ep, &key, &psk, &ip, &aip, &ka) < 8)
					continue;

				/* build peer allowed ips */
				memset(buffer, 0, BUF_SIZE);
				if (aip[0] == '\0')
					snprintf(buffer, BUF_SIZE, "%s", ip);
				else if (ip[0] == '\0')
					snprintf(buffer, BUF_SIZE, "%s", aip);
				else
					snprintf(buffer, BUF_SIZE, "%s,%s", ip, aip);

				/* remove peer from interface (and route) */
				if (priv[0] == '1') /* peer has private key? */
					wg_remove_peer_privkey(unit, iface, key, buffer, fwmark);
				else
					wg_remove_peer(unit, iface, key, buffer, fwmark);
			}
		}
		if (nvp)
			free(nvp);

		eval("ip", "rule", "delete", "table", fwmark, "fwmark", fwmark);
		eval("ip", "route", "flush", "table", fwmark);
		eval("ip", "route", "flush", "cache");

		/* remove interface */
		wg_set_iface_down(iface);
		wg_remove_iface(iface);
		wg_iface_post_down(unit);
	}

	/* remove firewall rules */
	memset(buffer, 0, BUF_SIZE);
	snprintf(buffer, BUF_SIZE, WG_FW_DIR"/%s-fw.sh", iface);
	run_del_firewall_script(buffer, WG_DIR_DEL_SCRIPT);
	eval("rm", "-rf", buffer);

	if (is_dev)
		logmsg(LOG_INFO, "wireguard (%s) stopped", iface);
}

void write_wg_dnsmasq_config(FILE* f)
{
	char buf[BUF_SIZE], device[BUF_SIZE_32];
	char *pos, *fn;
	DIR *dir;
	struct dirent *file;
	int cur;

	/* add interface(s) to dns config */
	strlcpy(buf, nvram_safe_get("wg_adns"), BUF_SIZE);
	for (pos = strtok(buf, ","); pos != NULL; pos = strtok(NULL, ",")) {
		cur = atoi(pos);
		if (cur || cur == 0) {
			logmsg(LOG_DEBUG, "*** %s: adding server wg%d interface to Dnsmasq config", __FUNCTION__, cur);
			fprintf(f, "interface=wg%d\n", cur);
		}
	}

	if ((dir = opendir(WG_DNS_DIR)) != NULL) {
		while ((file = readdir(dir)) != NULL) {
			fn = file->d_name;

			if (fn[0] == '.')
				continue;

			if (sscanf(fn, "%s.conf", device) == 1) {
				logmsg(LOG_DEBUG, "*** %s: adding Dnsmasq config from %s", __FUNCTION__, fn);
				memset(buf, 0, BUF_SIZE);
				snprintf(buf, BUF_SIZE, WG_DNS_DIR"/%s", fn);
				fappend(f, buf);
			}
		}
		closedir(dir);
	}
}
