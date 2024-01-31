/*
 *
 * Copyright (C) 2023 - 2024 FreshTomato
 * https://freshtomato.org/
 *
 * For use with FreshTomato Firmware only.
 * No part of this file may be used without permission.
 *
 * Fixes/updates (C) 2023 - 2024 pedro
 *
 */


#include "rc.h"
#include <dirent.h>
#include "curve25519.h"

/* needed by logmsg() */
#define LOGMSG_DISABLE		DISABLE_SYSLOG_OSM
#define LOGMSG_NVDEBUG		"wireguard_debug"

#define WG_DIR			"/etc/wireguard"
#define WG_DNS_DIR		WG_DIR"/dns"
#define WG_SCRIPTS_DIR		WG_DIR"/scripts"
#define WG_KEYS_DIR		WG_DIR"/keys"
#define WG_FW_DIR		WG_DIR"/fw"
#define WG_DEL_SCRIPT		"clear-fw-tmp.sh"
#define WG_DIR_DEL_SCRIPT	WG_FW_DIR"/"WG_DEL_SCRIPT

#define BUF_SIZE		256
#define BUF_SIZE_8		8
#define BUF_SIZE_16		16
#define BUF_SIZE_32		32
#define BUF_SIZE_64		64
#define IF_SIZE			8
#define PEER_COUNT		3

#define WG_INTERFACE_MAX	3


char port[BUF_SIZE_8];

static void wg_build_firewall(int unit, char *port, char *iface) {
	FILE *fp;
	char buffer[BUF_SIZE_64];
	char *dns;

	chains_log_detection();

	logmsg(LOG_DEBUG, "*** %s: building firewall scripts ...", __FUNCTION__);

	/* script with firewall rules (port, iface) */
	/* (..., open wireguard port, accept packets from wireguard internal subnet, set up forwarding) */
	memset(buffer, 0, BUF_SIZE_64);
	snprintf(buffer, BUF_SIZE_64, WG_FW_DIR"/%s-fw.sh", iface);
	if ((fp = fopen(buffer, "w"))) {
		fprintf(fp, "#!/bin/sh\n"
		            "\n# FW\n"
		            "iptables -A INPUT -p udp --dport %s -j %s\n"
		            "iptables -A INPUT -i %s -j %s\n"
		            "iptables -A FORWARD -i %s -j ACCEPT\n",
		            port, chain_in_accept,
		            iface, chain_in_accept,
		            iface);
#ifdef TCONFIG_BCMARM
		if (!nvram_get_int("ctf_disable")) /* bypass CTF if enabled */
			fprintf(fp, "iptables -t mangle -I PREROUTING -i %s -j MARK --set-mark 0x01/0x7\n", iface);
#endif /* TCONFIG_BCMARM */
		dns = getNVRAMVar("wg%d_dns", unit);
		if (getNVRAMVar("wg%d_file", unit)[0] == '\0') { /* only if no optional config file has been added */
			/* script to add/remove fw rules for dns servers (iface, dns) */
			fprintf(fp, "\n# DNS\n"
			            "DNS_CHAIN=\"wg-ft-%s-dns\"\n"
			            "DNS_FILE=\"%s/%s.conf\"\n"
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
			            "   iptables -A $DNS_CHAIN -i %s -p tcp --dst $NAMESERVER/32 --dport 53 -j ACCEPT\n"
			            "   iptables -A $DNS_CHAIN -i %s -p udp --dst $NAMESERVER/32 --dport 53 -j ACCEPT\n"
			            "  done\n"
			            "  iptables -A OUTPUT -j $DNS_CHAIN\n"
			            "  [ $? -eq 0 ] && nohup service dnsmasq restart &\n"
			            " } || {\n"
			            "  exit $STATUS\n"
			            " }\n"
			            "}\n",
			            iface,
			            WG_DNS_DIR, iface,
			            dns,
			            dns,
			            iface,
			            iface);
		}
		fclose(fp);
		chmod(buffer, (S_IRUSR | S_IWUSR | S_IXUSR));
	}

	/* script excerpt from wg-quick to route default (iface, route, fwmark) */
	if ((fp = fopen(WG_SCRIPTS_DIR"/route-default.sh", "w"))) {
		fprintf(fp, "#!/bin/sh\n"
		            "cmd() {\n"
		            "  echo \"[#] $*\" >&2\n"
		            "  \"$@\"\n"
		            "}\n\n"
		            "NL='\n"
		            "'\n"
		            "interface=\"${1}\"\n"
		            "route=\"${2}\"\n"
		            "table=\"${3}\"\n"
		            "case \"${route}\" in\n"
		            "  *:*)\n"
		            "    proto='-6'\n"
		            "    iptables='ip6tables'\n"
		            "    pf='ip6'\n"
		            "    ;;\n"
		            "  *)\n"
		            "    proto='-4'\n"
		            "    iptables='iptables'\n"
		            "    pf='ip'\n"
		            "    ;;\n"
		            "esac\n"
		            "cmd ip \"${proto}\" rule add not fwmark \"${table}\" table \"${table}\"\n"
		            "cmd ip \"${proto}\" rule add table main suppress_prefixlength 0\n"
		            "cmd ip \"${proto}\" route add \"${route}\" dev \"${interface}\" table \"${table}\"\n"
		            "restore=\"*raw${NL}\"\n"
		            "ip -o \"${proto}\" addr show dev \"${interface}\" 2>/dev/null | {\n"
		            "  match=''\n"
		            "  while read -r line; do\n"
		            "    match=\"$(\n"
		            "      printf %s \"${line}\" |\n"
		            "        sed -ne 's/^.*inet6\\? \\([0-9a-f:.]\\+\\)\\/[0-9]\\+.*$/\\1/; t P; b; : P; p'\n"
		            "    )\"\n"
		            "    [ -n \"${match}\" ] ||\n"
		            "      continue\n"
		            "    restore=\"${restore:+${restore}${NL}}-I PREROUTING ! -i ${interface} -d ${match} -m addrtype ! --src-type LOCAL -j DROP\"\n"
		            "  done\n"
		            "  restore=\"${restore:+${restore}${NL}}COMMIT${NL}*mangle${NL}-I POSTROUTING -m mark --mark ${table} -p udp -j CONNMARK --save-mark${NL}-I PREROUTING -p udp -j CONNMARK --restore-mark${NL}COMMIT\"\n"
		            "  ! [ \"${proto}\" = '-4' ] ||\n"
		            "    echo 1 > /proc/sys/net/ipv4/conf/all/src_valid_mark\n"
		            "  printf '%s\\n' \"${restore}\" |\n"
		            "    cmd \"${iptables}-restore\" -n\n"
		            "}\n", "%s", "%s");
		fclose(fp);
		chmod(WG_SCRIPTS_DIR"/route-default.sh", (S_IRUSR | S_IWUSR | S_IXUSR));
	}

	logmsg(LOG_DEBUG, "*** %s: Done", __FUNCTION__);
}

static int wg_quick_iface(char *iface, char *file, int up)
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

static void find_port(int unit, char *port)
{
	char *b;

	b = getNVRAMVar("wg%d_port", unit);
	memset(port, 0, BUF_SIZE_8);
	if (b[0] == '\0')
		snprintf(port, BUF_SIZE_8, "%d", 51820 + unit);
	else
		snprintf(port, BUF_SIZE_8, "%s", b);
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

static int wg_set_iface_addr(char *iface, char *addr)
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
			logmsg(LOG_WARNING, "unable to add wireguard interface %s address of %s!", iface, addr);
			return -1;
		}
		else
			logmsg(LOG_DEBUG, "wireguard interface %s has had address %s add to it", iface, addr);
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

static int wg_set_iface_privkey(char *iface, char *privkey)
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
		if (!(eval("ifconfig", iface, "up"))) {
			logmsg(LOG_DEBUG, "wireguard interface %s has been brought up", iface);
			return 0;
		}
		else if (retry < 4) {
			logmsg(LOG_WARNING, "unable to bring up wireguard interface %s, retrying...", iface);
			sleep(4);
		}
		retry += 1;
	}

	logmsg(LOG_WARNING, "unable to bring up wireguard interface %s!", iface);

	return -1;
}

static int wg_iface_script(int unit, char *script_name)
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

		/* sed replace %i with interface */
		memset(buffer, 0, BUF_SIZE_32);
		snprintf(buffer, BUF_SIZE_32, "s/%%i/wg%d/g", unit);

		if (eval("sed", "-i", buffer, path)) {
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

static void wg_iface_pre_up(int unit)
{
	wg_iface_script(unit, "preup");
}

static void wg_iface_post_up(int unit)
{
	wg_iface_script(unit, "postup");
}

static void wg_iface_pre_down(int unit)
{
	wg_iface_script(unit, "predown");
}

static void wg_iface_post_down(int unit)
{
	wg_iface_script(unit, "postdown");
}

static int wg_set_peer_psk(char *iface, char *pubkey, char *presharedkey)
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

static int wg_set_peer_endpoint(char *iface, char *pubkey, char *endpoint, char *port)
{
	char buffer[BUF_SIZE_64];

	memset(buffer, 0, BUF_SIZE_64);
	snprintf(buffer, BUF_SIZE_64, "%s:%s", endpoint, port);

	if (eval("wg", "set", iface, "peer", pubkey, "endpoint", buffer)) {
		logmsg(LOG_WARNING, "unable to add endpoint of %s:%s to peer %s on wireguard interface %s!", endpoint, port, pubkey, iface);
		return -1;
	}
	else
		logmsg(LOG_DEBUG, "endpoint of %s:%s has been added to peer %s on wireguard interface %s", endpoint, port, pubkey, iface);

	return 0;
}

static int wg_route_peer(char *iface, char *route)
{
	if (eval("ip", "route", "add", route, "dev", iface)) {
		logmsg(LOG_WARNING, "unable to add route of %s for wireguard interface %s!", route, iface);
		return -1;
	}
	else
		logmsg(LOG_DEBUG, "wireguard interface %s has had a route added to it for %s", iface, route);

	return 0;
}
/*
static int wg_route_peer_default(char *iface, char *route, char *fwmark)
{
	if (eval(WG_SCRIPTS_DIR"/route-default.sh", iface, route, fwmark)) {
		logmsg(LOG_WARNING, "unable to add default route of %s to table %s for wireguard interface %s!", route, fwmark, iface);
		return -1;
	}
	else
		logmsg(LOG_WARNING, "wireguard interface %s has had a default route added to table %s for %s", iface, fwmark, route);

	return 0;
}
*/
static int wg_route_peer_custom(char *iface, char *route, char *table)
{
	if (eval("ip", "route", "add", route, "dev", iface, "table", table)) {
		logmsg(LOG_WARNING, "unable to add route of %s to table %s for wireguard interface %s!", route, table, iface);
		return -1;
	}
	else
		logmsg(LOG_DEBUG, "wireguard interface %s has had a route added to table %s for %s", iface, table, route);

	return 0;
}

static int wg_route_peer_allowed_ips(char *iface, char *allowed_ips, char *fwmark)
{
	char *aip, *b, *table, *rt, *tp, *ip, *nm;
	int route_type = 1, result = 0;
	char buffer[BUF_SIZE_32];

	/* check which routing type the user specified */
	memset(buffer, 0, BUF_SIZE_32);
	snprintf(buffer, BUF_SIZE_32, "%s_route", iface);
	tp = b = strdup(nvram_safe_get(buffer));
	if (tp) {
		if (vstrsep(b, "|", &rt, &table) < 3)
			route_type = atoi(rt);

		free(tp);
	}

	/* check which routing type the user specified */
	if (route_type >  0) {
		aip = strdup(allowed_ips);
		while ((b = strsep(&aip, ",")) != NULL) {
			if (vstrsep(b, "/", &ip, &nm) == 2) {
				if (atoi(nm) == 0) {
					/* uncomment to add default routing (also in router/wireguard-tools/src/wg-quick/posix.sh line 519 - 521) after kernel fix */
					//wg_route_peer_default(iface, b, fwmark);
				}
			}
			if (route_type == 1) { /* Auto */
				if (wg_route_peer(iface, b))
					result = -1;
			}
			else /* Custom Table */
				if (wg_route_peer_custom(iface, b, table))
					result = -1;
		}
		if (aip)
			free(aip);
	}

	return result;
}

static int wg_set_peer_allowed_ips(char *iface, char *pubkey, char *allowed_ips, char *fwmark)
{
	if (eval("wg", "set", iface, "peer", pubkey, "allowed-ips", allowed_ips)) {
		logmsg(LOG_WARNING, "unable to add peer %s to wireguard interface %s!", pubkey, iface);
		return -1;
	}
	else
		logmsg(LOG_DEBUG, "peer %s for wireguard interface %s has had its allowed ips set to %s", pubkey, iface, allowed_ips);

	return wg_route_peer_allowed_ips(iface, allowed_ips, fwmark);
}

static int wg_add_peer(char *iface, char *pubkey, char *allowed_ips, char *presharedkey, char *keepalive, char *endpoint, char *fwmark, char *port)
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
		wg_set_peer_endpoint(iface, pubkey, endpoint, port);

	return 0;
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

static void wg_pubkey(char *privkey, char *pubkey)
{
	uint8_t key[WG_KEY_LEN] __attribute__((aligned(sizeof(uintptr_t))));

	key_from_base64(key, privkey);
	curve25519_generate_public(key, key);
	key_to_base64(pubkey, key);
}

static int wg_add_peer_privkey(char *iface, char *privkey, char *allowed_ips, char *presharedkey, char *keepalive, char *endpoint, char *fwmark)
{
	char pubkey[64];

	memset(pubkey, 0, sizeof(pubkey));
	wg_pubkey(privkey, pubkey);

	return wg_add_peer(iface, pubkey, allowed_ips, presharedkey, keepalive, endpoint, fwmark, port);
}
/*
static int wg_remove_peer(char *iface, char *pubkey)
{
	if (eval("wg", "set", iface, "peer", pubkey, "remove")) {
		logmsg(LOG_WARNING, "unable to remove peer %s from wireguard interface %s!", iface, pubkey);
		return -1;
	}
	else
		logmsg(LOG_DEBUG, "peer %s has been removed from wireguard interface %s", iface, pubkey);

	return 0;
}

static int wg_remove_peer_privkey(char *iface, char *privkey)
{
	char pubkey[64];
	memset(pubkey, 0, sizeof(pubkey));

	wg_pubkey(privkey, pubkey);

	return wg_remove_peer(iface, pubkey);
}
*/
static int wg_remove_iface(char *iface)
{
	/* check if interface exists */
	if (eval("ip", "addr", "show", "dev", iface))
		logmsg(LOG_DEBUG, "no such interface: %s", iface);
	else {
		/* delete wireguard interface */
		if (eval("ip", "link", "delete", iface)) {
			logmsg(LOG_WARNING, "unable to delete wireguard interface %s!", iface);
			return -1;
		}
		else
			logmsg(LOG_DEBUG, "wireguard interface %s has been deleted", iface);
	}

	return 0;
}

void start_wg_eas(void)
{
	int unit;

	for (unit = 0; unit < WG_INTERFACE_MAX; unit++) {
		if (atoi(getNVRAMVar("wg%d_enable", unit)) == 1) {
			start_wireguard(unit);
		}
	}
}
/*
void stop_wg_eas(void)
{
	int unit;

	for (unit = 0; unit < WG_INTERFACE_MAX; unit++) {
		if (atoi(getNVRAMVar("wg%d_enable", unit)) == 1) {
			stop_wireguard(unit);
		}
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
		if (eval("ip", "addr", "show", "dev", iface))
			logmsg(LOG_DEBUG, "no such wg instance to stop: %s", iface);
		else
			stop_wireguard(unit);
	}
	wg_cleanup_dirs();

	/* remove tunnel interface module */
	modprobe_r("wireguard");
}

void start_wireguard(int unit)
{
	char *nv, *nvp, *rka, *b;
	char *priv, *name, *key, *psk, *ip, *ka, *aip, *ep;
	char iface[IF_SIZE];
	char buffer[BUF_SIZE];
	char fwmark[BUF_SIZE_16];

	/* determine interface */
	memset(iface, 0, IF_SIZE);
	snprintf(iface, IF_SIZE, "wg%d", unit);

	/* prepare port value */
	find_port(unit, port);

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
		b = getNVRAMVar("wg%d_fwmark", unit);
		memset(fwmark, 0, BUF_SIZE_16);
		if (b[0] == '\0' || b[0] == '0')
			snprintf(fwmark, BUF_SIZE_16, "%s", port);
		else
			snprintf(fwmark, BUF_SIZE_16, "%s", b);

		if (wg_set_iface_fwmark(iface, fwmark))
			goto out;

		/* set interface mtu */
		if (wg_set_iface_mtu(iface, getNVRAMVar("wg%d_mtu", unit)))
			goto out;

		/* check if keepalives are enabled from the router */
		if (getNVRAMVar("wg%d_ka", unit)[0] == '1')
			rka = "25";
		else
			rka = "0";

		/* bring up interface */
		wg_iface_pre_up(unit);
		if (wg_set_iface_up(iface))
			goto out;

		/* add stored peers */
		nvp = nv = strdup(getNVRAMVar("wg%d_peers", unit));
		if (nv) {
			while ((b = strsep(&nvp, ">")) != NULL) {
				if (vstrsep(b, "<", &priv, &name, &ep, &key, &psk, &ip, &aip, &ka) < 8)
					continue;

				/* build peer allowed ips */
				memset(buffer, 0, BUF_SIZE);
				if (aip[0] == '\0')
					snprintf(buffer, BUF_SIZE, "%s", ip);
				else
					snprintf(buffer, BUF_SIZE, "%s,%s", ip, aip);

				/* add peer to interface */
				if (priv[0] == '1')
					wg_add_peer_privkey(iface, key, buffer, psk, rka, ep, fwmark);
				else
					wg_add_peer(iface, key, buffer, psk, rka, ep, fwmark, port);
			}
		}
		if (nvp)
			free(nvp);

		/* run post up scripts */
		wg_iface_post_up(unit);
	}

	/* create firewall script */
	wg_build_firewall(unit, port, iface);

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

	logmsg(LOG_INFO, "wireguard (%s) started", iface);

	return;

out:
	stop_wireguard(unit);
}

void stop_wireguard(int unit)
{
	char iface[IF_SIZE];
	char buffer[BUF_SIZE];
	int is_dev;

	/* determine interface */
	memset(iface, 0, IF_SIZE);
	snprintf(iface, IF_SIZE, "wg%d", unit);

	is_dev = eval("ip", "addr", "show", "dev", iface);

	if (getNVRAMVar("wg%d_file", unit)[0] != '\0')
		wg_quick_iface(iface, getNVRAMVar("wg%d_file", unit), 0);
	else {
		/* remove interface */
		wg_iface_pre_down(unit);
		wg_remove_iface(iface);
		wg_iface_post_down(unit);
	}

	/* remove firewall rules */
	memset(buffer, 0, BUF_SIZE);
	snprintf(buffer, BUF_SIZE, WG_FW_DIR"/%s-fw.sh", iface);
	run_del_firewall_script(buffer, WG_DIR_DEL_SCRIPT);

	if (!is_dev)
		logmsg(LOG_INFO, "wireguard (%s) stopped", iface);
}

void run_wg_firewall_scripts(void)
{
	DIR *dir;
	struct stat fs;
	struct dirent *file;
	char *fa;
	char buffer[BUF_SIZE_64];

	if (chdir(WG_FW_DIR))
		return;

	dir = opendir(WG_FW_DIR);

	logmsg(LOG_DEBUG, "*** %s: beginning all firewall scripts...", __FUNCTION__);

	while ((file = readdir(dir)) != NULL) {
		fa = file->d_name;

		if ((fa[0] == '.') || (strcmp(fa, WG_DEL_SCRIPT) == 0))
			continue;

		memset(buffer, 0, BUF_SIZE_64);
		snprintf(buffer, BUF_SIZE_64, "%s/", WG_FW_DIR);
		strlcat(buffer, fa, BUF_SIZE_64);

		/* check exe permission */
		stat(buffer, &fs);
		if (fs.st_mode & S_IXUSR) {
			/* first remove existing firewall rule(s) */
			run_del_firewall_script(buffer, WG_DIR_DEL_SCRIPT);

			/* then (re-)add firewall rule(s) */
			logmsg(LOG_DEBUG, "*** %s: running firewall script: %s", __FUNCTION__, buffer);
			eval(buffer);
		}
		else
			logmsg(LOG_DEBUG, "*** %s: skipping firewall script (not executable): %s", __FUNCTION__, buffer);
	}
	logmsg(LOG_DEBUG, "*** %s: done with all firewall scripts...", __FUNCTION__);

	closedir(dir);
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
