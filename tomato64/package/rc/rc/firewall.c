/*

	Copyright 2003-2005, CyberTAN Inc.  All Rights Reserved

	This is UNPUBLISHED PROPRIETARY SOURCE CODE of CyberTAN Inc.
	the contents of this file may not be disclosed to third parties,
	copied or duplicated in any form without the prior written
	permission of CyberTAN Inc.

	This software should be used as a reference only, and it not
	intended for production use!

	THIS SOFTWARE IS OFFERED "AS IS", AND CYBERTAN GRANTS NO WARRANTIES OF ANY
	KIND, EXPRESS OR IMPLIED, BY STATUTE, COMMUNICATION OR OTHERWISE.  CYBERTAN
	SPECIFICALLY DISCLAIMS ANY IMPLIED WARRANTIES OF MERCHANTABILITY, FITNESS
	FOR A SPECIFIC PURPOSE OR NONINFRINGEMENT CONCERNING THIS SOFTWARE

*/
/*

	Modified for Tomato Firmware
	Portions, Copyright (C) 2006-2009 Jonathan Zarate

*/


#include "rc.h"

#include <stdarg.h>
#include <arpa/inet.h>
#include <dirent.h>

static int web_lanport;

wanface_list_t wanfaces;
wanface_list_t wan2faces;
#ifdef TCONFIG_MULTIWAN
wanface_list_t wan3faces;
wanface_list_t wan4faces;
#endif

char lanaddr[BRIDGE_COUNT][32];
char lanmask[BRIDGE_COUNT][32];
char lanface[BRIDGE_COUNT][IFNAMSIZ + 1];
#ifdef TCONFIG_IPV6
char wan6face[IFNAMSIZ + 1];
#endif

char dmz_ifname[IFNAMSIZ + 1];
static int can_enable_fastnat;

#ifdef DEBUG_IPTFILE
static int debug_only = 0;
#endif

static int remotemanage;

static int wanup;
static int wan2up;
#ifdef TCONFIG_MULTIWAN
static int wan3up;
static int wan4up;
#endif

const char chain_wan_prerouting[] = "WANPREROUTING";
const char ipt_fname[] = "/etc/iptables";
FILE *ipt_file;

#ifdef TCONFIG_IPV6
const char ip6t_fname[] = "/etc/ip6tables";
FILE *ip6t_file;

/* RFC-4890, sec. 4.3.1 */
const int allowed_icmpv6[] = { 1, 2, 3, 4, 128, 129 };
#endif

static const char *fastnat_run_dir = "/var/run/fastnat";


static int is_sta(int idx, int unit, int subunit, void *param)
{
	return (nvram_match(wl_nvname("mode", unit, subunit), "sta") && (nvram_match(wl_nvname("bss_enabled", unit, subunit), "1")));
}

#ifdef TCONFIG_BCMARM
void ip2class(char *lan_ip, char *netmask, char *buf)
{
	unsigned int val, ip;
	struct in_addr in;
	int i = 0;

	val = (unsigned int)inet_addr(netmask);
	ip = (unsigned int)inet_addr(lan_ip);

	in.s_addr = ip & val;

	for (val = ntohl(val); val; i++)
		val <<= 1;

	sprintf(buf, "%s/%d", inet_ntoa(in), i);
}
#endif

void allow_fastnat(const char *service, int allow)
{
	char p[64];

	snprintf(p, sizeof(p), "%s/%s", fastnat_run_dir, service);
	if (allow)
		unlink(p);
	else {
		mkdir_if_none(fastnat_run_dir);
		f_write_string(p, "", 0, 0);
	}
}

static inline int fastnat_allowed(void)
{
	DIR *dir;
	struct dirent *dp;
	int enabled;

	enabled = !nvram_get_int("qos_enable") && !nvram_get_int("fastnat_disable");

	if (enabled && (dir = opendir(fastnat_run_dir))) {
		while ((dp = readdir(dir))) {
			if ((strcmp(dp->d_name, ".") == 0) || (strcmp(dp->d_name, "..") == 0))
				continue;
			enabled = 0;
			break;
		}
		closedir(dir);
	}

	return (enabled);
}

void try_enabling_fastnat(void)
{
	f_write_procsysnet("ipv4/netfilter/ip_conntrack_fastnat", (fastnat_allowed() ? "1" : "0"));
}

void enable_ip_forward(void)
{
	f_write_procsysnet("ipv4/ip_forward", "1");
}

void enable_blackhole_detection(void)
{
	int enabled;

	enabled = nvram_get_int("fw_blackhole");
	f_write_procsysnet("ipv4/tcp_mtu_probing", (enabled ? "1" : "0"));
	f_write_procsysnet("ipv4/tcp_base_mss", (enabled ? "1024" : "512"));
}

void log_segfault(void)
{
	f_write_string("/proc/sys/kernel/print-fatal-signals", (nvram_get_int("debug_logsegfault") ? "1" : "0"), 0, 0);
}

static int dmz_dst(char *s)
{
	struct in_addr ia;
	char *p;

	if (!nvram_get_int("dmz_enable"))
		return 0;

	p = nvram_safe_get("dmz_ipaddr");

	/* check for valid IP */
	if (inet_pton(AF_INET, p, &ia) <= 0)
		return 0;

	if (s)
		strcpy(s, p);

	return 1;
}

void lan_ip(char *buffer, char *ret)
{
	char *nv, *p;
	char s[32];

	if ((nv = nvram_get(buffer)) != NULL) {
		strcpy(s, nv);
		if ((p = strrchr(s, '.')) != NULL) {
			*p = 0;
			strcpy(ret, s);
		}
	}
	else
		strcpy(ret, "");
}

void ipt_log_unresolved(const char *addr, const char *addrtype, const char *categ, const char *name)
{
	char *pre, *post;

	pre = (name && *name) ? " for \"" : "";
	post = (name && *name) ? "\"" : "";

	syslog(LOG_WARNING, "firewall: %s: not using %s%s%s%s (could not resolve as valid %s address)", categ, addr, pre, (name) ? : "", post, (addrtype) ? : "IP");
}

int ipt_addr(char *addr, int maxlen, const char *s, const char *dir, int af, int strict, const char *categ, const char *name)
{
	char p[INET6_ADDRSTRLEN * 2];
	int r = 0;

	if ((s) && (*s) && (*dir)) {
		if (sscanf(s, "%[0-9.]-%[0-9.]", p, p) == 2) {
			snprintf(addr, maxlen, "-m iprange --%s-range %s", dir, s);
			r = IPT_V4;
		}
#ifdef TCONFIG_IPV6
		else if (sscanf(s, "%[0-9A-Fa-f:]-%[0-9A-Fa-f:]", p, p) == 2) {
			snprintf(addr, maxlen, "-m iprange --%s-range %s", dir, s);
			r = IPT_V6;
		}
#endif
		else {
			snprintf(addr, maxlen, "-%c %s", dir[0], s);
			if (sscanf(s, "%[^/]/", p)) {
#ifdef TCONFIG_IPV6
				r = host_addrtypes(p, strict ? af : (IPT_V4 | IPT_V6));
#else
				r = host_addrtypes(p, IPT_V4);
#endif
			}
		}
	}
	else {
		*addr = 0;
		r = (IPT_V4 | IPT_V6);
	}

	if (((r == 0) || (strict && ((r & af) != af))) && (categ && *categ)) {
		ipt_log_unresolved(s, categ, name, (af & IPT_V4 & ~r) ? "IPv4" : ((af & IPT_V6 & ~r) ? "IPv6" : NULL));
	}

	return (r & af);
}

void ipt_write(const char *format, ...)
{
	va_list args;

	va_start(args, format);
	vfprintf(ipt_file, format, args);
	va_end(args);
}

void ip6t_write(const char *format, ...)
{
#ifdef TCONFIG_IPV6
	va_list args;

	va_start(args, format);
	vfprintf(ip6t_file, format, args);
	va_end(args);
#endif
}

static void foreach_wan_input(int wanXup, wanface_list_t wanXfaces)
{
	unsigned int br;
	int i;

	if ((nvram_get_int("nf_loopback") != 0) && (wanXup)) {
		for (i = 0; i < wanXfaces.count; ++i) {
			if (*(wanXfaces.iface[i].name)) {
				for (br = 0; br < BRIDGE_COUNT; br++) {
					if ((strcmp(lanface[br], "") != 0) || (br == 0)) {
						ipt_write("-A INPUT -i %s -d %s -j DROP\n", lanface[br], wanXfaces.iface[i].ip);
					}
				}
			}
		}
	}
}

static void foreach_wan_nat(int wanXup, wanface_list_t wanXfaces, char *p)
{
	int i;

	for (i = 0; i < wanXfaces.count; ++i) {
		if (*(wanXfaces.iface[i].name)) {
			if ((!wanXup) || (nvram_get_int("ne_snat") != 1))
				ipt_write("-A POSTROUTING %s -o %s -j MASQUERADE\n", p, wanXfaces.iface[i].name);
			else
				ipt_write("-A POSTROUTING %s -o %s -j SNAT --to-source %s\n", p, wanXfaces.iface[i].name, wanXfaces.iface[i].ip);
		}
	}
}

int ipt_dscp(const char *v, char *opt)
{
	unsigned int n;

	if (*v == 0) {
		*opt = 0;
		return 0;
	}

	n = strtoul(v, NULL, 0);
	if (n > 63)
		n = 63;

	sprintf(opt, " -m dscp --dscp 0x%02X", n);

	modprobe("xt_dscp");

	return 1;
}

int ipt_ipp2p(const char *v, char *opt)
{
	int n = atoi(v);

	if (n == 0) {
		*opt = 0;
		return 0;
	}

	strcpy(opt, " -m ipp2p ");
	if ((n & 0xFFF) == 0xFFF)
		strcat(opt, "--ipp2p");
	else {
		/* x12 */
		if (n & 0x0001) strcat(opt, "--apple ");
		if (n & 0x0002) strcat(opt, "--ares ");
		if (n & 0x0004) strcat(opt, "--bit ");
		if (n & 0x0008) strcat(opt, "--dc ");
		if (n & 0x0010) strcat(opt, "--edk ");
		if (n & 0x0020) strcat(opt, "--gnu ");
		if (n & 0x0040) strcat(opt, "--kazaa ");
		if (n & 0x0080) strcat(opt, "--mute ");
		if (n & 0x0100) strcat(opt, "--soul ");
		if (n & 0x0200) strcat(opt, "--waste ");
		if (n & 0x0400) strcat(opt, "--winmx ");
		if (n & 0x0800) strcat(opt, "--xdcc ");
		if (n & 0x1000) strcat(opt, "--pp ");
		if (n & 0x2000) strcat(opt, "--xunlei ");
	}

	modprobe("ipt_ipp2p");
	return 1;
}

char **layer7_in;

/* This L7 matches inbound traffic, caches the results, then the L7 outbound
 * should read the cached result and set the appropriate marks
 */
static void ipt_layer7_inbound(void)
{
	int en, i;
	char **p;

	if (!layer7_in) return;

	en = nvram_match("nf_l7in", "1");
	if (en) {
		ipt_write(":L7in - [0:0]\n");
		if (wanup) {
			for (i = 0; i < wanfaces.count; ++i) {
				if (*(wanfaces.iface[i].name))
					ipt_write("-A FORWARD -i %s -j L7in\n", wanfaces.iface[i].name);
			}
		}
		if (wan2up) {
			for (i = 0; i < wan2faces.count; ++i) {
				if (*(wan2faces.iface[i].name))
					ipt_write("-A FORWARD -i %s -j L7in\n", wan2faces.iface[i].name);
			}
		}
#ifdef TCONFIG_MULTIWAN
		if (wan3up) {
			for (i = 0; i < wan3faces.count; ++i) {
				if (*(wan3faces.iface[i].name))
					ipt_write("-A FORWARD -i %s -j L7in\n", wan3faces.iface[i].name);
			}
		}
		if (wan4up) {
			for (i = 0; i < wan4faces.count; ++i) {
				if (*(wan4faces.iface[i].name))
					ipt_write("-A FORWARD -i %s -j L7in\n", wan4faces.iface[i].name);
			}
		}
#endif
	}

	p = layer7_in;
	while (*p) {
		if (en) {
			ipt_write("-A L7in %s -j RETURN\n", *p);
			can_enable_fastnat = 0;
		}
		free(*p);
		++p;
	}
	free(layer7_in);
	layer7_in = NULL;
}

int ipt_layer7(const char *v, char *opt)
{
	char s[128];
	char *path;

	*opt = 0;
	if (*v == 0)
		return 0;

	if (strlen(v) > 32)
		return -1;

	path = "/etc/l7-extra";
	memset(s, 0, sizeof(s));
	snprintf(s, sizeof(s), "%s/%s.pat", path, v);
	if (!f_exists(s)) {
		path = "/etc/l7-protocols";
		memset(s, 0, sizeof(s));
		snprintf(s, sizeof(s), "%s/%s.pat", path, v);
		if (!f_exists(s)) {
			syslog(LOG_ERR, "L7 %s was not found", v);
			return -1;
		}
	}

	sprintf(opt, " -m layer7 --l7dir %s --l7proto %s", path, v);

	if (nvram_match("nf_l7in", "1")) {
		if (!layer7_in)
			layer7_in = calloc(51, sizeof(char *));

		if (layer7_in) {
			char **p;

			p = layer7_in;
			while (*p) {
				if (strcmp(*p, opt) == 0)
					return 1;

				++p;
			}
			if (((p - layer7_in) / sizeof(char *)) < 50)
				*p = strdup(opt);
		}
	}

	modprobe("xt_layer7");

	return 1;
}

static void ipt_account(void) {
	struct in_addr ipaddr, netmask, network;
	char lanN_ifname[] = "lanXX_ifname";
	char lanN_ipaddr[] = "lanXX_ipaddr";
	char lanN_netmask[] = "lanXX_netmask";
	char lanN[] = "lanXX";
	char netaddrnetmask[] = "255.255.255.255/255.255.255.255 ";
	char br;
	/* If the IP Address changes, the below rule will cause things to choke, and blocking rules don't get applied
	 * As a workaround, flush the entire FORWARD chain
	 */
	system("iptables -F FORWARD");

	for (br = 0 ; br < BRIDGE_COUNT; br++) {
		char bridge[2] = "0";
		if (br != 0)
			bridge[0] += br;
		else
			strcpy(bridge, "");

		snprintf(lanN_ifname, sizeof(lanN_ifname), "lan%s_ifname", bridge);

		if (strcmp(nvram_safe_get(lanN_ifname), "") != 0) {
			snprintf(lanN_ipaddr, sizeof(lanN_ipaddr), "lan%s_ipaddr", bridge);
			snprintf(lanN_netmask, sizeof(lanN_netmask), "lan%s_netmask", bridge);
			snprintf(lanN, sizeof(lanN), "lan%s", bridge);

			inet_aton(nvram_safe_get(lanN_ipaddr), &ipaddr);
			inet_aton(nvram_safe_get(lanN_netmask), &netmask);

			/* bitwise AND of ip and netmask gives the network */
			network.s_addr = ipaddr.s_addr & netmask.s_addr;

			snprintf(netaddrnetmask, sizeof(netaddrnetmask), "%s/%s", inet_ntoa(network), nvram_safe_get(lanN_netmask));

			/* ipv4 only */
			ipt_write("-A FORWARD -m account --aaddr %s --aname %s\n", netaddrnetmask, lanN);
		}
	}
}

static void save_webmon(void)
{
	eval("cp", "/proc/webmon_recent_domains", "/var/webmon/domain");
	eval("cp", "/proc/webmon_recent_searches", "/var/webmon/search");
}

static void ipt_webmon()
{
	int wmtype, clear, i;
	char t[512];
	char src[128];
	char *p, *c;
	int ok;

	if (!nvram_get_int("log_wm"))
		return;

	can_enable_fastnat = 0;
	wmtype = nvram_get_int("log_wmtype");
	clear = nvram_get_int("log_wmclear");

#ifdef TCONFIG_BCMARM
	/* disable webmon for ip6tables (ARM) - FIXME */
	ipt_write(":monitor - [0:0]\n");
#else
	ip46t_write(":monitor - [0:0]\n");
#endif
	/* include IPs */
	strlcpy(t, wmtype == 1 ? nvram_safe_get("log_wmip") : "", sizeof(t));
	p = t;
	do {
		if ((c = strchr(p, ',')) != NULL) *c = 0;

		if ((ok = ipt_addr(src, sizeof(src), p, "src", (IPT_V4 | IPT_V6), 0, "webmon", NULL))) {

/* disable webmon for ip6tables (ARM) - FIXME */
#ifndef TCONFIG_BCMARM
#ifdef TCONFIG_IPV6
			if (*wan6face && (ok & IPT_V6))
				ip6t_write("-A FORWARD -o %s %s -j monitor\n", wan6face, src);
#endif
#endif
			if (ok & IPT_V4) {
				for (i = 0; i < wanfaces.count; ++i) {
					if (*(wanfaces.iface[i].name))
						ipt_write("-A FORWARD -o %s %s -j monitor\n", wanfaces.iface[i].name, src);
				}
				for (i = 0; i < wan2faces.count; ++i) {
					if (*(wan2faces.iface[i].name))
						ipt_write("-A FORWARD -o %s %s -j monitor\n", wan2faces.iface[i].name, src);
				}
#ifdef TCONFIG_MULTIWAN
				for (i = 0; i < wan3faces.count; ++i) {
					if (*(wan3faces.iface[i].name))
						ipt_write("-A FORWARD -o %s %s -j monitor\n", wan3faces.iface[i].name, src);
				}
				for (i = 0; i < wan4faces.count; ++i) {
					if (*(wan4faces.iface[i].name))
						ipt_write("-A FORWARD -o %s %s -j monitor\n", wan4faces.iface[i].name, src);
				}
#endif
			}
		}

		if (!c) break;
		p = c + 1;
	} while (*p);

	/* exclude IPs */
	if (wmtype == 2) {
		strlcpy(t, nvram_safe_get("log_wmip"), sizeof(t));
		p = t;
		do {
			if ((c = strchr(p, ',')) != NULL)
				*c = 0;
			if ((ok = ipt_addr(src, sizeof(src), p, "src", (IPT_V4 | IPT_V6), 0, "webmon", NULL))) {
				if (*src)
#ifdef TCONFIG_BCMARM
					/* disable webmon for ip6tables (ARM) - FIXME */
					ipt_flagged_write(ok, "-A monitor %s -j RETURN\n", src);
#else
					ip46t_flagged_write(ok, "-A monitor %s -j RETURN\n", src);
#endif
			}
			if (!c) break;
			p = c + 1;
		} while (*p);
	}

	char webdomain[100];
	char websearch[100];

	memset(webdomain, 0, 100);
	memset(websearch, 0, 100);
	if (nvram_match("webmon_bkp", "1")) {
		xstart( "/usr/sbin/webmon_bkp", "add" ); /* add jobs to cru */

		snprintf(webdomain, sizeof(webdomain), "--domain_load_file %s/webmon_recent_domains", nvram_safe_get("webmon_dir"));
		snprintf(websearch, sizeof(websearch), "--search_load_file %s/webmon_recent_searches", nvram_safe_get("webmon_dir"));
	}
	else {
		snprintf(webdomain, sizeof(webdomain), "--domain_load_file /var/webmon/domain");
		snprintf(websearch, sizeof(websearch), "--search_load_file /var/webmon/search");
	}

#ifdef TCONFIG_BCMARM
	/* disable webmon for ip6tables - FIXME */
	ipt_write(
#else
	ip46t_write(
#endif
	            "-A monitor -p tcp -m webmon --max_domains %d --max_searches %d %s %s -j RETURN\n",
	            nvram_get_int("log_wmdmax") ? : 1, nvram_get_int("log_wmsmax") ? : 1, (clear & 1) == 0 ? webdomain : "--clear_domain", (clear & 2) == 0 ? websearch : "--clear_search");

	if (nvram_match("webmon_bkp", "1"))
		xstart("/usr/sbin/webmon_bkp", "hourly"); /* make a copy immediately */

#ifdef TCONFIG_BCMARM
	modprobe("ipt_webmon");
#else
	modprobe("xt_webmon");
#endif
}

static void mangle_table(void)
{
	int ttl;
#ifdef TCONFIG_BCMARM
	char lan_class[32];
	int i, n;
#endif	/* TCONFIG_BCMARM */

	char *p, *wanface, *wan2face;
#ifdef TCONFIG_MULTIWAN
	char *wan3face, *wan4face;
#endif

	ip46t_write("*mangle\n"
	            ":PREROUTING ACCEPT [0:0]\n"
	            ":OUTPUT ACCEPT [0:0]\n");

	if (wanup || wan2up
#ifdef TCONFIG_MULTIWAN
	|| wan3up || wan4up
#endif
	) {
		ipt_qos();
		/* 1 for mangle */
		ipt_bwlimit(1);

		p = nvram_safe_get("nf_ttl");
		if (strncmp(p, "c:", 2) == 0) {
			p += 2;
			ttl = atoi(p);
			p = (ttl >= 0 && ttl <= 255) ? "set" : NULL;
		}
		else if ((ttl = atoi(p)) != 0) {
			if (ttl > 0)
				p = "inc";
			else {
				ttl = -ttl;
				p = "dec";
			}
			if (ttl > 255)
				p = NULL;
		}
		else
			p = NULL;

		wanface = wanfaces.iface[0].name;
		wan2face = wan2faces.iface[0].name;
#ifdef TCONFIG_MULTIWAN
		wan3face = wan3faces.iface[0].name;
		wan4face = wan4faces.iface[0].name;
#endif

		if (p) {
			modprobe("xt_HL");

			if (wanup && *wanface) {
				/* set TTL on primary WAN iface only */
				ipt_write("-I PREROUTING -i %s -j TTL --ttl-%s %d\n"
				          "-I POSTROUTING -o %s -j TTL --ttl-%s %d\n",
				          wanface, p, ttl,
				          wanface, p, ttl);
			}
			if (wan2up && *wan2face) {
				/* set TTL on primary WAN2 iface only */
				ipt_write("-I PREROUTING -i %s -j TTL --ttl-%s %d\n"
				          "-I POSTROUTING -o %s -j TTL --ttl-%s %d\n",
				          wan2face, p, ttl,
				          wan2face, p, ttl);
			}
#ifdef TCONFIG_MULTIWAN
			if (wan3up && *wan3face) {
				/* set TTL on primary WAN3 iface only */
				ipt_write("-I PREROUTING -i %s -j TTL --ttl-%s %d\n"
				          "-I POSTROUTING -o %s -j TTL --ttl-%s %d\n",
				          wan3face, p, ttl,
				          wan3face, p, ttl);
				}
			if (wan4up && *wan4face) {
				/* set TTL on primary WAN4 iface only */
				ipt_write("-I PREROUTING -i %s -j TTL --ttl-%s %d\n"
				          "-I POSTROUTING -o %s -j TTL --ttl-%s %d\n",
				          wan4face, p, ttl,
				          wan4face, p, ttl);
			}
#endif

#ifdef TCONFIG_IPV6
/* FIXME: IPv6 HL should be configurable separately from TTL.
 *        disable it until GUI setting is implemented.
 */
 #if 0
			ip6t_write("-I PREROUTING -i %s -j HL --hl-%s %d\n"
			           "-I POSTROUTING -o %s -j HL --hl-%s %d\n",
			           wan6face, p, ttl,
			           wan6face, p, ttl);
 #endif
#endif
		}

		/* Reset Incoming DSCP to 0x00 */
		if (nvram_match("DSCP_fix_enable", "1")) {
			modprobe("xt_DSCP");

			if (wanup && *wanface)
				ipt_write("-I PREROUTING -i %s -j DSCP --set-dscp 0\n", wanface);

			if (wan2up && *wan2face)
				ipt_write("-I PREROUTING -i %s -j DSCP --set-dscp 0\n", wan2face);

#ifdef TCONFIG_MULTIWAN
			if (wan3up && *wan3face)
				ipt_write("-I PREROUTING -i %s -j DSCP --set-dscp 0\n", wan3face);

			if (wan4up && *wan4face)
				ipt_write("-I PREROUTING -i %s -j DSCP --set-dscp 0\n", wan4face);
#endif
		}

	}

	/* Clamp TCP MSS to PMTU of WAN interface (IPv4 & IPv6) */
	if (!nvram_get_int("tcp_clamp_disable"))
		ip46t_write("-I FORWARD -p tcp --tcp-flags SYN,RST SYN -j TCPMSS --clamp-mss-to-pmtu\n");
	else
		syslog(LOG_INFO, "Firewall: No Clamping of TCP MSS to PMTU of WAN interface"); /* Ex.: case MTU 1500 for ISPs that support RFC 4638 */

#ifdef TCONFIG_BCMARM
	/* set mark for NAT loopback to work if CTF is enabled! (bypass) */
	if (!nvram_get_int("ctf_disable")) {
		for (i = 0; i < BRIDGE_COUNT; i++) {
			if ((strcmp(lanface[i], "") != 0) && (strcmp(lanaddr[i], "") != 0)) { /* check LAN setup */
				ip2class(lanaddr[i], lanmask[i], lan_class);
				ipt_write("-A FORWARD -o %s -s %s -d %s -j MARK --set-mark 0x01/0x7\n", lanface[i], lan_class, lan_class);
			}
		}
	}
#endif /* TCONFIG_BCMARM */

#ifdef TCONFIG_BCMARM
	for (i = 0; i < wanfaces.count; ++i) {
		if ((*(wanfaces.iface[i].name)) && (wanup)) {
			/* Drop incoming packets which destination IP address is to our LAN side directly */
			for (n = 0; n < BRIDGE_COUNT; n++) {
				if ((strcmp(lanaddr[n], "") != 0 && strcmp(lanmask[n], "") != 0) || (n == 0)) /* note: ipt will correct lanaddr[0] */
					ipt_write("-A PREROUTING -i %s -d %s/%s -j DROP\n", wanfaces.iface[i].name, lanaddr[n], lanmask[n]);
			}
		}
	}

	for (i = 0; i < wan2faces.count; ++i) {
		if ((*(wan2faces.iface[i].name)) && (wan2up)) {
			/* Drop incoming packets which destination IP address is to our LAN side directly */
			for (n = 0; n < BRIDGE_COUNT; n++) {
				if ((strcmp(lanaddr[n], "") != 0 && strcmp(lanmask[n], "") != 0) || (n == 0)) /* note: ipt will correct lanaddr[0] */
					ipt_write("-A PREROUTING -i %s -d %s/%s -j DROP\n", wan2faces.iface[i].name, lanaddr[n], lanmask[n]);
			}
		}
	}
#ifdef TCONFIG_MULTIWAN
	for (i = 0; i < wan3faces.count; ++i) {
		if ((*(wan3faces.iface[i].name)) && (wan3up)) {
			/* Drop incoming packets which destination IP address is to our LAN side directly */
			for (n = 0; n < BRIDGE_COUNT; n++) {
				if ((strcmp(lanaddr[n], "") != 0 && strcmp(lanmask[n], "") != 0) || (n == 0)) /* note: ipt will correct lanaddr[0] */
					ipt_write("-A PREROUTING -i %s -d %s/%s -j DROP\n", wan3faces.iface[i].name, lanaddr[n], lanmask[n]);
			}
		}
	}

	for (i = 0; i < wan4faces.count; ++i) {
		if ((*(wan4faces.iface[i].name)) && (wan4up)) {
			/* Drop incoming packets which destination IP address is to our LAN side directly */
			for (n = 0; n < BRIDGE_COUNT; n++) {
				if ((strcmp(lanaddr[n], "") != 0 && strcmp(lanmask[n], "") != 0) || (n == 0)) /* note: ipt will correct lanaddr[0] */
					ipt_write("-A PREROUTING -i %s -d %s/%s -j DROP\n", wan4faces.iface[i].name, lanaddr[n], lanmask[n]);
			}
		}
	}
#endif
#endif /* TCONFIG_BCMARM */

	ipt_routerpolicy();

	ip46t_write("COMMIT\n");
}

static void nat_table(void)
{
	char dst[64];
	char src[64];
	char t[512];
	char *p, *c, *b;
	int i;
#ifndef TCONFIG_BCMARM
	int n;
#endif /* !TCONFIG_BCMARM */

	ipt_write("*nat\n"
	          ":PREROUTING ACCEPT [0:0]\n"
	          ":POSTROUTING ACCEPT [0:0]\n"
	          ":OUTPUT ACCEPT [0:0]\n"
	          ":%s - [0:0]\n",
	          chain_wan_prerouting);

	/* 2 for nat */
	ipt_bwlimit(2);

	for (i = 0; i < wanfaces.count; ++i) {
		if (*(wanfaces.iface[i].name)) {
			/* chain_wan_prerouting */
			if (wanup)
				ipt_write("-A PREROUTING -d %s -j %s\n", wanfaces.iface[i].ip, chain_wan_prerouting);
#ifndef TCONFIG_BCMARM
			/* Drop incoming packets which destination IP address is to our LAN side directly */
			for (n = 0; n < BRIDGE_COUNT; n++) {
				if ((strcmp(lanaddr[n], "") != 0 && strcmp(lanmask[n], "") != 0) || (n == 0)) /* note: ipt will correct lanaddr[0] */
					ipt_write("-A PREROUTING -i %s -d %s/%s -j DROP\n", wanfaces.iface[i].name, lanaddr[n], lanmask[n]);
			}
#endif /* !TCONFIG_BCMARM */
		}
	}
	for (i = 0; i < wan2faces.count; ++i) {
		if (*(wan2faces.iface[i].name)) {
			/* chain_wan_prerouting */
			if (wan2up)
				ipt_write("-A PREROUTING -d %s -j %s\n", wan2faces.iface[i].ip, chain_wan_prerouting);
#ifndef TCONFIG_BCMARM
			/* Drop incoming packets which destination IP address is to our LAN side directly */
			for (n = 0; n < BRIDGE_COUNT; n++) {
				if ((strcmp(lanaddr[n], "") != 0 && strcmp(lanmask[n], "") != 0) || (n == 0)) /* note: ipt will correct lanaddr[0] */
					ipt_write("-A PREROUTING -i %s -d %s/%s -j DROP\n", wan2faces.iface[i].name, lanaddr[n], lanmask[n]);
			}
#endif /* !TCONFIG_BCMARM */
		}
	}
#ifdef TCONFIG_MULTIWAN
	for (i = 0; i < wan3faces.count; ++i) {
		if (*(wan3faces.iface[i].name)) {
			/* chain_wan_prerouting */
			if (wan3up)
				ipt_write("-A PREROUTING -d %s -j %s\n", wan3faces.iface[i].ip, chain_wan_prerouting);
#ifndef TCONFIG_BCMARM
			/* Drop incoming packets which destination IP address is to our LAN side directly */
			for (n = 0; n < BRIDGE_COUNT; n++) {
				if ((strcmp(lanaddr[n], "") != 0 && strcmp(lanmask[n], "") != 0) || (n == 0)) /* note: ipt will correct lanaddr[0] */
					ipt_write("-A PREROUTING -i %s -d %s/%s -j DROP\n", wan3faces.iface[i].name, lanaddr[n], lanmask[n]);
			}
#endif /* !TCONFIG_BCMARM */
		}
	}
	for (i = 0; i < wan4faces.count; ++i) {
		if (*(wan4faces.iface[i].name)) {
			/* chain_wan_prerouting */
			if (wan4up)
				ipt_write("-A PREROUTING -d %s -j %s\n", wan4faces.iface[i].ip, chain_wan_prerouting);
#ifndef TCONFIG_BCMARM
			/* Drop incoming packets which destination IP address is to our LAN side directly */
			for (n = 0; n < BRIDGE_COUNT; n++) {
				if ((strcmp(lanaddr[n], "") != 0 && strcmp(lanmask[n], "") != 0) || (n == 0)) /* note: ipt will correct lanaddr[0] */
					ipt_write("-A PREROUTING -i %s -d %s/%s -j DROP\n", wan4faces.iface[i].name, lanaddr[n], lanmask[n]);
			}
#endif /* !TCONFIG_BCMARM */
		}
	}
#endif /* TCONFIG_MULTIWAN */

	if (wanup || wan2up
#ifdef TCONFIG_MULTIWAN
	    || wan3up || wan4up
#endif
	) {
		if (nvram_match("dns_intcpt", "1")) {
			/* Need to intercept both TCP and UDP DNS requests for all lan interfaces */
			modprobe("ipt_REDIRECT");
			ipt_write("-A PREROUTING -i br+ -p tcp -m tcp --dport 53 -j REDIRECT\n");
			ipt_write("-A PREROUTING -i br+ -p udp -m udp --dport 53 -j REDIRECT\n");
		}

		/* NTP server redir */
		if (nvram_get_int("ntpd_enable") && nvram_get_int("ntpd_server_redir")) {
			modprobe("ipt_REDIRECT");
			ipt_write("-A PREROUTING -i br+ -p udp -m udp --dport 123 -j REDIRECT\n");
		}

		/* ICMP packets are always redirected to INPUT chains */
		ipt_write("-A %s -p icmp -j DNAT --to-destination %s\n", chain_wan_prerouting, lanaddr[0]);

		/* force remote access to the router if DMZ is enabled */
		if ((nvram_match("dmz_enable", "1")) && (nvram_match("dmz_ra", "1"))) {
			strlcpy(t, nvram_safe_get("rmgt_sip"), sizeof(t));
			p = t;
			do {
				if ((c = strchr(p, ',')) != NULL)
					*c = 0;

				ipt_source(p, src, "ra", NULL);

				if (remotemanage)
					ipt_write("-A %s -p tcp -m tcp %s --dport %s -j DNAT --to-destination %s:%d\n", chain_wan_prerouting, src, nvram_safe_get("http_wanport"), lanaddr[0], web_lanport);

				if (nvram_get_int("sshd_remote"))
					ipt_write("-A %s %s -p tcp -m tcp --dport %s -j DNAT --to-destination %s:%s\n", chain_wan_prerouting, src, nvram_safe_get("sshd_rport"), lanaddr[0], nvram_safe_get("sshd_port"));

				if (!c)
					break;

				p = c + 1;
			} while (*p);
		}
		ipt_forward(IPT_TABLE_NAT);
		ipt_triggered(IPT_TABLE_NAT);
	}

	if (nvram_get_int("upnp_enable") & 3) {
		ipt_write(":upnp - [0:0]\n"
		          ":pupnp - [0:0]\n");

		for (i = 0; i < wanfaces.count; ++i) {
			if (*(wanfaces.iface[i].name)) {
				if (wanup)
					/* ! for loopback (all) to work */
					ipt_write("-A PREROUTING -d %s -j upnp\n", wanfaces.iface[i].ip);
				else
					ipt_write("-A PREROUTING -i %s -j upnp\n", wanfaces.iface[i].name);
			}
		}

		for (i = 0; i < wan2faces.count; ++i) {
			if (*(wan2faces.iface[i].name)) {
				if (wan2up)
					/* ! for loopback (all) to work */
					ipt_write("-A PREROUTING -d %s -j upnp\n", wan2faces.iface[i].ip);
				else
					ipt_write("-A PREROUTING -i %s -j upnp\n", wan2faces.iface[i].name);
			}
		}
#ifdef TCONFIG_MULTIWAN
		for (i = 0; i < wan3faces.count; ++i) {
			if (*(wan3faces.iface[i].name)) {
				if (wan3up)
					/* ! for loopback (all) to work */
					ipt_write("-A PREROUTING -d %s -j upnp\n", wan3faces.iface[i].ip);
				else
					ipt_write("-A PREROUTING -i %s -j upnp\n", wan3faces.iface[i].name);
			}
		}

		for (i = 0; i < wan4faces.count; ++i) {
			if (*(wan4faces.iface[i].name)) {
				if (wan4up)
					/* ! for loopback (all) to work */
					ipt_write("-A PREROUTING -d %s -j upnp\n", wan4faces.iface[i].ip);
				else
					ipt_write("-A PREROUTING -i %s -j upnp\n", wan4faces.iface[i].name);
			}
		}
#endif
	}

#ifdef TCONFIG_TOR
	/* TOR */
	if (nvram_match("tor_enable", "1") && nvram_match("tor_solve_only", "0")) {
		char *torports;
		char *toriface = nvram_safe_get("tor_iface");
		char *tortrans = nvram_safe_get("tor_transport");
		char buf[8];
		int done = 0;

		if (nvram_match("tor_ports", "custom"))
			torports = nvram_safe_get("tor_ports_custom");
		else
			torports = nvram_safe_get("tor_ports");

		for (i = 0; i < BRIDGE_COUNT; i++) {
			memset(buf, 0, sizeof(buf));
			snprintf(buf, sizeof(buf), "br%d", i);

			if (done == 0 && nvram_match("tor_iface", buf) && ((strcmp(lanaddr[i], "") != 0) || (i == 0))) {
				ipt_write("-A PREROUTING -i %s -p tcp -m multiport --dport %s ! -d %s -j DNAT --to-destination %s:%s\n",
				           toriface, torports, lanaddr[i], lanaddr[i], tortrans);
				done = 1;
			}
		}
		if (done == 0) {
			strlcpy(t, nvram_safe_get("tor_users"), sizeof(t));
			p = t;
			do {
				if ((c = strchr(p, ',')) != NULL)
					*c = 0;

				if (ipt_source_strict(p, src, "tor", NULL))
					ipt_write("-A PREROUTING %s -p tcp -m multiport --dport %s ! -d %s -j DNAT --to-destination %s:%s\n",
					           src, torports, lanaddr[0], lanaddr[0], tortrans);

				if (!c)
					break;

				p = c + 1;
			} while (*p);
		}
	}
#endif

#ifdef TCONFIG_SNMP
	if (nvram_match("snmp_enable", "1") && nvram_match("snmp_remote", "1"))
		ipt_write("-A WANPREROUTING -p tcp --dport %s -j DNAT --to-destination %s\n", nvram_safe_get("snmp_port"), lanaddr[0]);
#endif

	if (wanup || wan2up
#ifdef TCONFIG_MULTIWAN
	    || wan3up || wan4up
#endif
	) {
		memset(dst, 0, sizeof(dst));
		if (dmz_dst(dst)) {
			strlcpy(t, nvram_safe_get("dmz_sip"), sizeof(t));
			p = t;
			do {
				if ((c = strchr(p, ',')) != NULL)
					*c = 0;

				if (ipt_source_strict(p, src, "dmz", NULL))
					ipt_write("-A %s %s -j DNAT --to-destination %s\n", chain_wan_prerouting, src, dst);

				if (!c)
					break;

				p = c + 1;
			} while (*p);
		}
	}

	p = "";
#ifdef TCONFIG_IPV6
	switch (get_ipv6_service()) {
	case IPV6_6IN4:
		/* avoid NATing proto-41 packets when using 6in4 tunnel */
		p = "! -p 41";
		break;
	}
#endif

	foreach_wan_nat(wanup, wanfaces, p);
	foreach_wan_nat(wan2up, wan2faces, p);
#ifdef TCONFIG_MULTIWAN
	foreach_wan_nat(wan3up, wan3faces, p);
	foreach_wan_nat(wan4up, wan4faces, p);
#endif

#ifdef TCONFIG_PPTPD
	/* PPTP Client NAT */
	pptp_client_firewall("POSTROUTING", p, ipt_write);
#endif
	if ((nvram_match("wan_proto", "pppoe") || nvram_match("wan_proto", "dhcp") || nvram_match("wan_proto", "static"))
	    && (b = nvram_safe_get("wan_modem_ipaddr")) && (*b) && (!nvram_match("wan_modem_ipaddr", "0.0.0.0")) && (!foreach_wif(1, NULL, is_sta)))
		ipt_write("-A POSTROUTING -o %s -d %s -j MASQUERADE\n", nvram_safe_get("wan_ifname"), b);

	if ((nvram_match("wan2_proto", "pppoe") || nvram_match("wan2_proto", "dhcp") || nvram_match("wan2_proto", "static"))
	    && (b = nvram_safe_get("wan2_modem_ipaddr")) && (*b) && (!nvram_match("wan2_modem_ipaddr", "0.0.0.0")) && (!foreach_wif(1, NULL, is_sta)))
		ipt_write("-A POSTROUTING -o %s -d %s -j MASQUERADE\n", nvram_safe_get("wan2_ifname"), b);

#ifdef TCONFIG_MULTIWAN
	if ((nvram_match("wan3_proto", "pppoe") || nvram_match("wan3_proto", "dhcp") || nvram_match("wan3_proto", "static"))
	    && (b = nvram_safe_get("wan3_modem_ipaddr")) && (*b) && (!nvram_match("wan3_modem_ipaddr", "0.0.0.0")) && (!foreach_wif(1, NULL, is_sta)))
		ipt_write("-A POSTROUTING -o %s -d %s -j MASQUERADE\n", nvram_safe_get("wan3_ifname"), b);

	if ((nvram_match("wan4_proto", "pppoe") || nvram_match("wan4_proto", "dhcp") || nvram_match("wan4_proto", "static"))
	    && (b = nvram_safe_get("wan4_modem_ipaddr")) && (*b) && (!nvram_match("wan4_modem_ipaddr", "0.0.0.0")) && (!foreach_wif(1, NULL, is_sta)))
		ipt_write("-A POSTROUTING -o %s -d %s -j MASQUERADE\n", nvram_safe_get("wan4_ifname"), b);
#endif

	switch (nvram_get_int("nf_loopback")) {
		case 1: /* 1 = forwarded-only */
		case 2: /* 2 = disable */
		break;
		default: /* 0 = all (same as block_loopback=0) */
			for (i = 0; i < BRIDGE_COUNT; i++) {
				if ((strcmp(lanface[i], "") != 0 && strcmp(lanmask[i], "") != 0) || (i == 0))
					ipt_write("-A POSTROUTING -o %s -s %s/%s -d %s/%s -j SNAT --to-source %s\n", lanface[i], lanaddr[i], lanmask[i], lanaddr[i], lanmask[i], lanaddr[i]);
			}
		break;
	}

	ipt_write("COMMIT\n");
}

static void filter_input(void)
{
	char s[64];
	char t[512];
	char *en;
	char *sec;
	char *hit;
	int n;
	char *p, *c;

#ifdef TCONFIG_BCMARM
	/* 3 for filter */
	ipt_bwlimit(3);
#endif

	foreach_wan_input(wanup, wanfaces);
	foreach_wan_input(wan2up, wan2faces);
#ifdef TCONFIG_MULTIWAN
	foreach_wan_input(wan3up, wan3faces);
	foreach_wan_input(wan4up, wan4faces);
#endif

	ipt_write("-A INPUT -m state --state INVALID -j DROP\n"
	          "-A INPUT -m state --state RELATED,ESTABLISHED -j ACCEPT\n");

	strlcpy(s, nvram_safe_get("ne_shlimit"), sizeof(s));

	if ((vstrsep(s, ",", &en, &hit, &sec) == 3) && ((n = atoi(en) & 3) != 0)) {
		modprobe("xt_recent");

		ipt_write("-N shlimit\n"
		          "-A shlimit -m recent --set --name shlimit\n"
		          "-A shlimit -m recent --update --hitcount %d --seconds %s --name shlimit -j %s\n",
		          atoi(hit) + 1, sec, chain_in_drop);

		if (n & 1) {
			ipt_write("-A INPUT -p tcp --dport %s -m state --state NEW -j shlimit\n", nvram_safe_get("sshd_port"));
			if (nvram_get_int("sshd_remote") && nvram_invmatch("sshd_rport", nvram_safe_get("sshd_port"))) {
				ipt_write("-A INPUT -p tcp --dport %s -m state --state NEW -j shlimit\n", nvram_safe_get("sshd_rport"));
			}
		}
		if (n & 2)
			ipt_write("-A INPUT -p tcp --dport %s -m state --state NEW -j shlimit\n", nvram_safe_get("telnetd_port"));
	}

	/* Protect against brute force on port defined for remote GUI access */
	if (remotemanage && nvram_get_int("http_wanport_bfm")) {
		modprobe("xt_recent");

		ipt_write("-N wwwlimit\n"
		          "-A wwwlimit -m recent --set --name www\n"
		          "-A wwwlimit -m recent --update --hitcount 15 --seconds 5 --name www -j %s\n",
		          chain_in_drop);
		ipt_write("-A INPUT -p tcp --dport %s -m state --state NEW -j wwwlimit\n", nvram_safe_get("http_wanport"));
	}

#ifdef TCONFIG_FTP
	strlcpy(s, nvram_safe_get("ftp_limit"), sizeof(s));
	if ((vstrsep(s, ",", &en, &hit, &sec) == 3) && (atoi(en)) && (nvram_get_int("ftp_enable") == 1)) {
		modprobe("xt_recent");

		ipt_write("-N ftplimit\n"
		          "-A ftplimit -m recent --set --name ftp\n"
		          "-A ftplimit -m recent --update --hitcount %d --seconds %s --name ftp -j %s\n",
		          atoi(hit) + 1, sec, chain_in_drop);
	}
#endif

	ipt_write("-A INPUT -i lo -j ACCEPT\n"
	          "-A INPUT -i %s -j ACCEPT\n",
	          lanface[0]);
	if (strcmp(lanface[1], "") != 0)
		ipt_write("-A INPUT -i %s -j ACCEPT\n", lanface[1]);
	if (strcmp(lanface[2], "") != 0)
		ipt_write("-A INPUT -i %s -j ACCEPT\n", lanface[2]);
	if (strcmp(lanface[3], "") != 0)
		ipt_write("-A INPUT -i %s -j ACCEPT\n", lanface[3]);

#ifdef TCONFIG_IPV6
	n = get_ipv6_service();
	switch (n) {
	case IPV6_ANYCAST_6TO4:
	case IPV6_6IN4:
		memset(s, 0, 64);
		/* Accept ICMP requests from the remote tunnel endpoint */
		if (n == IPV6_ANYCAST_6TO4)
			snprintf(s, sizeof(s), "192.88.99.%d", nvram_get_int("ipv6_relay"));
		else
			strlcpy(s, nvram_safe_get("ipv6_tun_v4end"), sizeof(s));

		if (*s && strcmp(s, "0.0.0.0") != 0)
			ipt_write("-A INPUT -p icmp -s %s -j %s\n", s, chain_in_accept);

		ipt_write("-A INPUT -p 41 -j %s\n", chain_in_accept);
		break;
	}
#endif

	/* ICMP request from WAN interface */
	if (nvram_match("block_wan", "0")) {
		if (nvram_match("block_wan_limit", "0")) {
			/* allow ICMP echo/traceroute packets to be received */
			ipt_write("-A INPUT -p icmp --icmp-type 8 -m state --state NEW,ESTABLISHED,RELATED -j %s\n", chain_in_accept);
			ipt_write("-A INPUT -p icmp --icmp-type 30 -m state --state NEW,ESTABLISHED,RELATED -j %s\n", chain_in_accept);
			/* allow udp traceroute packets */
			ipt_write("-A INPUT -p udp --dport 33434:33534 -j %s\n", chain_in_accept);
		}
		else {
			/* allow ICMP echo/traceroute packets to be received, but restrict the flow to avoid ping flood attacks */
			ipt_write("-A INPUT -p icmp --icmp-type 8 -m state --state NEW,ESTABLISHED,RELATED -m limit --limit %d/second -j %s\n", nvram_get_int("block_wan_limit_icmp"), chain_in_accept);
			ipt_write("-A INPUT -p icmp --icmp-type 30 -m state --state NEW,ESTABLISHED,RELATED -m limit --limit %d/second -j %s\n", nvram_get_int("block_wan_limit_icmp"), chain_in_accept);
			/* allow udp traceroute packets, but restrict the flow to avoid ping flood attacks */
			ipt_write("-A INPUT -p udp --dport 33434:33534 -m limit --limit %d/second -j %s\n", nvram_get_int("block_wan_limit_icmp"), chain_in_accept);
		}
	}

	/* Accept incoming packets from broken dhcp servers, which are sending replies
	 * from addresses other than used for query. This could lead to a lower level
	 * of security, so allow to disable it via nvram variable.
	 */
	if (nvram_invmatch("wan_dhcp_pass", "0") && (using_dhcpc("wan") || using_dhcpc("wan2")
#ifdef TCONFIG_MULTIWAN
	|| using_dhcpc("wan3") || using_dhcpc("wan4")
#endif
	)) {
		ipt_write("-A INPUT -p udp --sport 67 --dport 68 -j %s\n", chain_in_accept);
	}

	strlcpy(t, nvram_safe_get("rmgt_sip"), sizeof(t));
	p = t;
	do {
		if ((c = strchr(p, ',')) != NULL)
			*c = 0;

		if (ipt_source(p, s, "remote management", NULL)) {
			if (remotemanage)
				ipt_write("-A INPUT -p tcp %s --dport %s -j %s\n", s, nvram_safe_get("http_wanport"), chain_in_accept);

			if (nvram_get_int("sshd_remote"))
				ipt_write("-A INPUT -p tcp %s --dport %s -j %s\n", s, nvram_safe_get("sshd_rport"), chain_in_accept);
		}

		if (!c)
			break;

		p = c + 1;
	} while (*p);

#ifdef TCONFIG_SNMP
	if (nvram_match("snmp_enable", "1") && nvram_match("snmp_remote", "1")) {
		strlcpy(t, nvram_safe_get("snmp_remote_sip"), sizeof(t));
		p = t;
		do {
			if ((c = strchr(p, ',')) != NULL)
				*c = 0;

			if (ipt_source(p, s, "snmp", "remote"))
				ipt_write("-A INPUT -p udp %s --dport %s -j %s\n", s, nvram_safe_get("snmp_port"), chain_in_accept);

			if (!c)
				break;

			p = c + 1;
		} while (*p);
	}
#endif

	/* IGMP query from WAN interface */
	if ((nvram_match("multicast_pass", "1")) || (nvram_match("udpxy_enable", "1")))
		ipt_write("-A INPUT -p igmp -d 224.0.0.0/4 -j ACCEPT\n"
		          "-A INPUT -p udp -d 224.0.0.0/4 ! --dport 1900 -j ACCEPT\n");

	/* Routing protocol, RIP, accept */
	if (nvram_invmatch("dr_wan_rx", "0"))
		ipt_write("-A INPUT -p udp --dport 520 -j ACCEPT\n");

#ifdef TCONFIG_PPTPD
	/* Add for pptp client */
	pptp_client_firewall("INPUT", "", ipt_write);
#endif

	/* if logging */
	if (*chain_in_drop == 'l')
		ipt_write("-A INPUT -j %s\n", chain_in_drop);

	/* NTP server LAN & WAN */
	if (nvram_get_int("ntpd_enable") == 2)
		ipt_write("-A INPUT -p udp -m udp --dport 123 -j %s\n", chain_in_accept);

	/* default policy: DROP */
}

static void filter_forward(void)
{
	char dst[64];
	char src[64];
	char buffer[512], dmz1[32], dmz2[32];
	char *p, *c;
	char br, br2;
	char lanN_ifname[] = "lanXX_ifname";
	char lanN_ifname2[] = "lanXX_ifname";
	unsigned int i;

#ifdef TCONFIG_IPV6
	ip6t_write("-A FORWARD -m rt --rt-type 0 -j DROP\n");
#endif

	if (nvram_match("cstats_enable", "1"))
		ipt_account();

	for (i = 0; i < BRIDGE_COUNT; i++) {
		if ((strcmp(lanface[i], "") != 0) || (i == 0))
			ip46t_write("-A FORWARD -i %s -o %s -j ACCEPT\n", lanface[i], lanface[i]); /* accept all lan to lan */
	}

	char lanAccess[17] = "0000000000000000";

	const char *d, *sbr, *saddr, *dbr, *daddr, *desc;
	char *nv, *nvp, *b;
	nvp = nv = strdup(nvram_safe_get("lan_access"));
	if (nv) {
		while ((b = strsep(&nvp, ">")) != NULL) {
			/*
			 * 1<0<1.2.3.4<1<5.6.7.8<30,45-50<desc
			 *
			 * 1 = enabled
			 * 0 = src bridge
			 * 1.2.3.4 = src addr
			 * 1 = dst bridge
			 * 5.6.7.8 = dst addr
			 * desc = desc
			 */
			if ((vstrsep(b, "<", &d, &sbr, &saddr, &dbr, &daddr, &desc) < 6) || (*d != '1'))
				continue;
			if (!ipt_addr(src, sizeof(src), saddr, "src", (IPT_V4 | IPT_V6), 0, "LAN access", desc))
				continue;
			if (!ipt_addr(dst, sizeof(dst), daddr, "dst", (IPT_V4 | IPT_V6), 0, "LAN access", desc))
				continue;

			/* ipv4 only */
			ipt_write("-A FORWARD -i %s%s -o %s%s %s %s -j ACCEPT\n", "br", sbr, "br", dbr, src, dst);

			if ((strcmp(src, "") == 0) && (strcmp(dst, "") == 0))
				lanAccess[((*sbr - 48) + (*dbr - 48) * 4)] = '1';

		}
	}
	free(nv);

	ip46t_write("-A FORWARD -m state --state INVALID -j DROP\n"); /* drop if INVALID state */

	if (wanup || wan2up
#ifdef TCONFIG_MULTIWAN
	|| wan3up || wan4up
#endif
	) {
		ipt_restrictions();

		ipt_layer7_inbound();
	}

	ipt_webmon();

	ip46t_write(":wanin - [0:0]\n"
	            ":wanout - [0:0]\n"
	            "-A FORWARD -m state --state RELATED,ESTABLISHED -j ACCEPT\n"); /* already established or related (via helper) */

	/* IPv4 IPSec */
	if (nvram_match("ipsec_pass", "1") || nvram_match("ipsec_pass", "3")) {
		for (i = 0; i < (unsigned int) wanfaces.count; ++i) {
			if (*(wanfaces.iface[i].name))
				ipt_write("-A FORWARD -i %s -p esp -j ACCEPT\n"				/* ESP */
				          "-A FORWARD -i %s -p ah -j ACCEPT\n"				/* AH */
				          "-A FORWARD -i %s -p udp --dport 500 -j ACCEPT\n"		/* IKE */
				          "-A FORWARD -i %s -p udp --dport 4500 -j ACCEPT\n",		/* NAT-T */
				          wanfaces.iface[i].name, wanfaces.iface[i].name, wanfaces.iface[i].name, wanfaces.iface[i].name);
		}
		for (i = 0; i < (unsigned int) wan2faces.count; ++i) {
			if (*(wan2faces.iface[i].name))
				ipt_write("-A FORWARD -i %s -p esp -j ACCEPT\n"				/* ESP */
				          "-A FORWARD -i %s -p ah -j ACCEPT\n"				/* AH */
				          "-A FORWARD -i %s -p udp --dport 500 -j ACCEPT\n"		/* IKE */
				          "-A FORWARD -i %s -p udp --dport 4500 -j ACCEPT\n",		/* NAT-T */
				          wan2faces.iface[i].name, wan2faces.iface[i].name, wan2faces.iface[i].name, wan2faces.iface[i].name);
		}
#ifdef TCONFIG_MULTIWAN
		for (i = 0; i < (unsigned int) wan3faces.count; ++i) {
			if (*(wan3faces.iface[i].name))
				ipt_write("-A FORWARD -i %s -p esp -j ACCEPT\n"				/* ESP */
				          "-A FORWARD -i %s -p ah -j ACCEPT\n"				/* AH */
				          "-A FORWARD -i %s -p udp --dport 500 -j ACCEPT\n"		/* IKE */
				          "-A FORWARD -i %s -p udp --dport 4500 -j ACCEPT\n",		/* NAT-T */
				          wan3faces.iface[i].name, wan3faces.iface[i].name, wan3faces.iface[i].name, wan3faces.iface[i].name);
		}
		for (i = 0; i < (unsigned int) wan4faces.count; ++i) {
			if (*(wan4faces.iface[i].name))
				ipt_write("-A FORWARD -i %s -p esp -j ACCEPT\n"				/* ESP */
				          "-A FORWARD -i %s -p ah -j ACCEPT\n"				/* AH */
				          "-A FORWARD -i %s -p udp --dport 500 -j ACCEPT\n"		/* IKE */
				          "-A FORWARD -i %s -p udp --dport 4500 -j ACCEPT\n",		/* NAT-T */
				          wan4faces.iface[i].name, wan4faces.iface[i].name, wan4faces.iface[i].name, wan4faces.iface[i].name);
		}
#endif /* TCONFIG_MULTIWAN */
	}

	for (br = 0; br < BRIDGE_COUNT; br++) {
		char bridge[2] = "0";
		if (br != 0)
			bridge[0] += br;
		else
			strcpy(bridge, "");

		snprintf(lanN_ifname, sizeof(lanN_ifname), "lan%s_ifname", bridge);
		if (strncmp(nvram_safe_get(lanN_ifname), "br", 2) == 0) {
			for (br2 = 0; br2 < BRIDGE_COUNT; br2++) {
				if (br == br2)
					continue;

				if (lanAccess[((br)+(br2)*4)] == '1')
					continue;

				char bridge2[2] = "0";
				if (br2 != 0)
					bridge2[0] += br2;
				else
					strcpy(bridge2, "");

				snprintf(lanN_ifname2, sizeof(lanN_ifname2), "lan%s_ifname", bridge2);

				if (strncmp(nvram_safe_get(lanN_ifname2), "br", 2) == 0)
					ipt_write("-A FORWARD -i %s -o %s -j DROP\n", nvram_safe_get(lanN_ifname), nvram_safe_get(lanN_ifname2));
			}
//			ip46t_write("-A FORWARD -i %s -j %s\n", nvram_safe_get(lanN_ifname), chain_out_accept);
		}
	}

#ifdef TCONFIG_IPV6
	/* Filter out invalid WAN->WAN connections */
	if (*wan6face)
//		ip6t_write("-A FORWARD -o %s ! -i %s -j %s\n", wan6face, lanface[0], chain_in_drop); /* we can't drop connections from WAN to LAN1-3 */
		ip6t_write("-A FORWARD -o %s -i %s -j %s\n", wan6face, wan6face, chain_in_drop); /* drop connection from WAN -> WAN only */

	modprobe("xt_length");
	ip6t_write("-A FORWARD -p ipv6-nonxt -m length --length 40 -j ACCEPT\n");

	/* ICMPv6 rules */
	for (i = 0; i < sizeof(allowed_icmpv6)/sizeof(int); ++i) {
		if ((allowed_icmpv6[i] == 128) && !nvram_get_int("block_wan")) /* ratelimit ipv6 ping */
			ip6t_write("-A FORWARD -p ipv6-icmp --icmpv6-type %i -m limit --limit 5/s -j %s\n", allowed_icmpv6[i], chain_in_accept);
		else
			ip6t_write("-A FORWARD -p ipv6-icmp --icmpv6-type %i -j %s\n", allowed_icmpv6[i], chain_in_accept);
	}

	/* IPv6 IPSec - RFC 6092 */
	if (*wan6face && (nvram_match("ipsec_pass", "1") || nvram_match("ipsec_pass", "2")))
		ip6t_write("-A FORWARD -i %s -p esp -j ACCEPT\n"				/* ESP */
		           "-A FORWARD -i %s -p udp --dport 500 -j ACCEPT\n",			/* IKE */
		           wan6face, wan6face);

	/* IPv6 */
	if (*wan6face)
		ip6t_write("-A FORWARD -i %s -j wanin\n"			/* generic from wan */
		           "-A FORWARD -o %s -j wanout\n",			/* generic to wan */
		           wan6face, wan6face);
#endif

	/* IPv4 */
	for (i = 0; i < (unsigned int) wanfaces.count; ++i) {
		if (*(wanfaces.iface[i].name))
			ipt_write("-A FORWARD -i %s -j wanin\n"			/* generic from wan */
			          "-A FORWARD -o %s -j wanout\n",		/* generic to wan */
			          wanfaces.iface[i].name, wanfaces.iface[i].name);
	}

	for (i = 0; i < (unsigned int) wan2faces.count; ++i) {
		if (*(wan2faces.iface[i].name))
			ipt_write("-A FORWARD -i %s -j wanin\n"			/* generic from wan */
			          "-A FORWARD -o %s -j wanout\n",		/* generic to wan */
			          wan2faces.iface[i].name, wan2faces.iface[i].name);
	}

#ifdef TCONFIG_MULTIWAN
	for (i = 0; i < (unsigned int) wan3faces.count; ++i) {
		if (*(wan3faces.iface[i].name))
			ipt_write("-A FORWARD -i %s -j wanin\n"			/* generic from wan */
			          "-A FORWARD -o %s -j wanout\n",		/* generic to wan */
			          wan3faces.iface[i].name, wan3faces.iface[i].name);
	}

	for (i = 0; i < (unsigned int) wan4faces.count; ++i) {
		if (*(wan4faces.iface[i].name))
			ipt_write("-A FORWARD -i %s -j wanin\n"			/* generic from wan */
			          "-A FORWARD -o %s -j wanout\n",		/* generic to wan */
			          wan4faces.iface[i].name, wan4faces.iface[i].name);
	}
#endif

#ifdef TCONFIG_PPTPD
	/* Add for pptp client */
	pptp_client_firewall("FORWARD", "", ipt_write);
#endif

	for (br = 0; br < BRIDGE_COUNT; br++) {
		char bridge[2] = "0";
		if (br != 0)
			bridge[0] += br;
		else
			strcpy(bridge, "");

		snprintf(lanN_ifname, sizeof(lanN_ifname), "lan%s_ifname", bridge);
		if (strncmp(nvram_safe_get(lanN_ifname), "br", 2) == 0)
			ip46t_write("-A FORWARD -i %s -j %s\n", nvram_safe_get(lanN_ifname), chain_out_accept);
	}

#ifdef TCONFIG_IPV6
	/* IPv6 forward LAN->WAN accept */
	for (i = 0; i < BRIDGE_COUNT; i++) {
		if (*wan6face && ((strcmp(lanface[i], "") != 0) || (i == 0)))
			ip6t_write("-A FORWARD -i %s -o %s -j %s\n", lanface[i], wan6face, chain_out_accept);
	}
#endif

	/* IPv4 only */
	if (nvram_get_int("upnp_enable") & 3) {
		ipt_write(":upnp - [0:0]\n");
		for (i = 0; i < (unsigned int) wanfaces.count; ++i) {
			if (*(wanfaces.iface[i].name))
				ipt_write("-A FORWARD -i %s -j upnp\n", wanfaces.iface[i].name);
		}
	}

	if (wanup || wan2up
#ifdef TCONFIG_MULTIWAN
	|| wan3up || wan4up
#endif
	) {
		if ((nvram_match("multicast_pass", "1")) || (nvram_match("udpxy_enable", "1")))
			ipt_write("-A wanin -p udp -d 224.0.0.0/4 -j %s\n", chain_in_accept);

		ipt_triggered(IPT_TABLE_FILTER);
		ipt_forward(IPT_TABLE_FILTER);
#ifdef TCONFIG_IPV6
		ip6t_forward();
#endif
		memset(dst, 0, sizeof(dst));
		if (dmz_dst(dst)) {
			memset(dmz_ifname, 0, sizeof(dmz_ifname));
			for (i = 0; i < BRIDGE_COUNT; i++) {
				if (strcmp(lanface[i], "") != 0) { /* LAN is enabled */
					memset(buffer, 0, sizeof(buffer));
					snprintf(buffer, sizeof(buffer), (i == 0 ? "lan_ipaddr" : "lan%d_ipaddr"), i);
					lan_ip(buffer, dmz1);
					lan_ip("dmz_ipaddr", dmz2);

					if (strcmp(dmz1, dmz2) == 0 && strcmp(lanface[i], "") != 0) {
						strlcpy(dmz_ifname, lanface[i], sizeof(dmz_ifname));
						break;
					}
				}
			}
			if (strcmp(dmz_ifname, "") == 0)
				strlcpy(dmz_ifname, lanface[0], sizeof(dmz_ifname)); /* empty? set default (primary) */

			memset(buffer, 0, sizeof(buffer));
			strlcpy(buffer, nvram_safe_get("dmz_sip"), sizeof(buffer));
			p = buffer;
			do {
				if ((c = strchr(p, ',')) != NULL)
					*c = 0;

				if (ipt_source(p, src, "dmz", NULL))
					ipt_write("-A FORWARD -o %s %s -d %s -j %s\n", dmz_ifname, src, dst, chain_in_accept);

				if (!c)
					break;

				p = c + 1;
			} while (*p);
		}
	}
	/* default policy: DROP */
}

static void filter_log(void)
{
	int n;
	char limit[128];

	n = nvram_get_int("log_limit");
	memset(limit, 0, 128);
	if ((n >= 1) && (n <= 9999))
		snprintf(limit, sizeof(limit), "-m limit --limit %d/m", n);
	else
		limit[0] = 0;

#ifdef TCONFIG_IPV6
	modprobe("ip6t_LOG");
#endif
	if ((*chain_in_drop == 'l') || (*chain_out_drop == 'l'))
		ip46t_write(":logdrop - [0:0]\n"
		            "-A logdrop -m state --state NEW %s -j LOG --log-prefix \"DROP \" --log-macdecode --log-tcp-sequence --log-tcp-options --log-ip-options\n"
		            "-A logdrop -j DROP\n"
		            ":logreject - [0:0]\n"
		            "-A logreject %s -j LOG --log-prefix \"REJECT \" --log-macdecode --log-tcp-sequence --log-tcp-options --log-ip-options\n"
		            "-A logreject -p tcp -j REJECT --reject-with tcp-reset\n",
		            limit, limit);

	if ((*chain_in_accept == 'l') || (*chain_out_accept == 'l'))
		ip46t_write(":logaccept - [0:0]\n"
		            "-A logaccept -m state --state NEW %s -j LOG --log-prefix \"ACCEPT \" --log-macdecode --log-tcp-sequence --log-tcp-options --log-ip-options\n"
		            "-A logaccept -j ACCEPT\n",
		            limit);
}

#ifdef TCONFIG_IPV6
static void filter6_input(void)
{
	char s[128];
	char t[512];
	char *en;
	char *sec;
	char *hit;
	unsigned int n;
	char *p, *c;

	/* RFC-4890, sec. 4.4.1 */
	const int allowed_local_icmpv6[] = { 130, 131, 132, 133, 134, 135, 136, 141, 142, 143, 148, 149, 151, 152, 153 };

	ip6t_write("-A INPUT -m rt --rt-type 0 -j %s\n"
	           /* "-A INPUT -m state --state INVALID -j DROP\n" */
	           "-A INPUT -m state --state RELATED,ESTABLISHED -j ACCEPT\n",
	           chain_in_drop);

	modprobe("xt_length");
	ip6t_write("-A INPUT -p ipv6-nonxt -m length --length 40 -j ACCEPT\n");

	strlcpy(s, nvram_safe_get("ne_shlimit"), sizeof(s));
	if ((vstrsep(s, ",", &en, &hit, &sec) == 3) && ((n = atoi(en) & 3) != 0)) {
		modprobe("xt_recent");

		ip6t_write("-N shlimit\n"
		           "-A shlimit -m recent --set --name shlimit\n"
		           "-A shlimit -m recent --update --hitcount %d --seconds %s --name shlimit -j %s\n",
		           atoi(hit) + 1, sec, chain_in_drop);

		if (n & 1) {
			ip6t_write("-A INPUT -p tcp --dport %s -m state --state NEW -j shlimit\n", nvram_safe_get("sshd_port"));
			if (nvram_get_int("sshd_remote") && nvram_invmatch("sshd_rport", nvram_safe_get("sshd_port")))
				ip6t_write("-A INPUT -p tcp --dport %s -m state --state NEW -j shlimit\n", nvram_safe_get("sshd_rport"));
		}
		if (n & 2)
			ip6t_write("-A INPUT -p tcp --dport %s -m state --state NEW -j shlimit\n", nvram_safe_get("telnetd_port"));
	}

	/* Protect against brute force on port defined for remote GUI access */
	if (remotemanage && nvram_get_int("http_wanport_bfm")) {
		modprobe("xt_recent");

		ip6t_write("-N wwwlimit\n"
		           "-A wwwlimit -m recent --set --name www\n"
		           "-A wwwlimit -m recent --update --hitcount 15 --seconds 5 --name www -j %s\n",
		           chain_in_drop);
		ip6t_write("-A INPUT -p tcp --dport %s -m state --state NEW -j wwwlimit\n", nvram_safe_get("http_wanport"));
	}

#ifdef TCONFIG_FTP
	strlcpy(s, nvram_safe_get("ftp_limit"), sizeof(s));
	if ((vstrsep(s, ",", &en, &hit, &sec) == 3) && (atoi(en)) && (nvram_get_int("ftp_enable") == 1)) {
		modprobe("xt_recent");

		ip6t_write("-N ftplimit\n"
		           "-A ftplimit -m recent --set --name ftp\n"
		           "-A ftplimit -m recent --update --hitcount %d --seconds %s --name ftp -j %s\n",
		           atoi(hit) + 1, sec, chain_in_drop);
	}
#endif /* TCONFIG_FTP */

	ip6t_write("-A INPUT -i lo -j ACCEPT\n"
	           "-A INPUT -i %s -j ACCEPT\n", /* anything coming from LAN */
	           lanface[0]);
	if (strcmp(lanface[1], "") != 0)
		ip6t_write("-A INPUT -i %s -j ACCEPT\n", lanface[1]);
	if (strcmp(lanface[2], "") != 0)
		ip6t_write("-A INPUT -i %s -j ACCEPT\n", lanface[2]);
	if (strcmp(lanface[3], "") != 0)
		ip6t_write("-A INPUT -i %s -j ACCEPT\n", lanface[3]);

	switch (get_ipv6_service()) {
	case IPV6_ANYCAST_6TO4:
	case IPV6_NATIVE_DHCP:
		/* allow responses from the dhcpv6 server (Port 547) to the client (Port 546) */
		ip6t_write("-A INPUT -p udp --sport 547 --dport 546 -j %s\n", chain_in_accept);
		break;
	}

	/* ICMPv6 rules */
	for (n = 0; n < sizeof(allowed_icmpv6)/sizeof(int); n++) {
		if ((allowed_icmpv6[n] == 128) && !nvram_get_int("block_wan")) /* ratelimit ipv6 ping */
			ip6t_write("-A INPUT -p ipv6-icmp --icmpv6-type %i -m limit --limit 5/s -j %s\n", allowed_icmpv6[n], chain_in_accept);
		else
			ip6t_write("-A INPUT -p ipv6-icmp --icmpv6-type %i -j %s\n", allowed_icmpv6[n], chain_in_accept);
	}
	for (n = 0; n < sizeof(allowed_local_icmpv6)/sizeof(int); n++) {
		ip6t_write("-A INPUT -p ipv6-icmp --icmpv6-type %i -j %s\n", allowed_local_icmpv6[n], chain_in_accept);
	}

	/* Remote Managment */
	strlcpy(t, nvram_safe_get("rmgt_sip"), sizeof(t));
	p = t;
	do {
		if ((c = strchr(p, ',')) != NULL)
			*c = 0;

		if (ip6t_source(p, s, "remote management", NULL)) {

			if (remotemanage)
				ip6t_write("-A INPUT -p tcp %s --dport %s -j %s\n", s, nvram_safe_get("http_wanport"), chain_in_accept);

			if (nvram_get_int("sshd_remote"))
				ip6t_write("-A INPUT -p tcp %s --dport %s -j %s\n", s, nvram_safe_get("sshd_rport"), chain_in_accept);
		}

		if (!c) break;
		p = c + 1;
	} while (*p);

	/* if logging */
	if (*chain_in_drop == 'l')
		ip6t_write( "-A INPUT -j %s\n", chain_in_drop);

	/* default policy: DROP */
}
#endif /* TCONFIG_IPV6 */

static void filter_table(void)
{
	ip46t_write("*filter\n"
	            ":INPUT DROP [0:0]\n"
	            ":OUTPUT ACCEPT [0:0]\n");

	filter_log();

	filter_input();
#ifdef TCONFIG_IPV6
	filter6_input();
	ip6t_write("-A OUTPUT -m rt --rt-type 0 -j %s\n", chain_in_drop);
#endif

#ifdef TCONFIG_PPTPD
	/* Add for pptp client */
	pptp_client_firewall("OUTPUT", "", ipt_write);
#endif

	ip46t_write(":FORWARD DROP [0:0]\n");
	filter_forward();
	ip46t_write("COMMIT\n");
}

int start_firewall(void)
{
	DIR *dir;
	struct dirent *dirent;
	char s[64];
	char buf1[16];
	char buf2[16];
	char *c;
	char *wanface, *wan2face;
#ifdef TCONFIG_MULTIWAN
	char *wan3face, *wan4face;
#endif
	int n;
	int wanproto;
	char *iptrestore_argv[] = { "iptables-restore", (char *)ipt_fname, NULL };
#ifdef TCONFIG_IPV6
	char *ip6trestore_argv[] = { "ip6tables-restore", (char *)ip6t_fname, NULL };
#endif

	simple_lock("firewall");
	simple_lock("restrictions");

	wanup = check_wanup("wan");
	wan2up = check_wanup("wan2");
#ifdef TCONFIG_MULTIWAN
	wan3up = check_wanup("wan3");
	wan4up = check_wanup("wan4");
#endif

	log_segfault();

	/* NAT performance tweaks
	 * These values can be overriden later if needed via firewall script
	 */
	f_write_procsysnet("core/netdev_max_backlog", "2048");
	f_write_procsysnet("core/somaxconn", "1024");
	f_write_procsysnet("ipv4/tcp_max_syn_backlog", "1024");
	f_write_procsysnet("ipv4/neigh/default/gc_thresh1", "1");
	f_write_procsysnet("ipv4/neigh/default/gc_thresh2", "2048");
	f_write_procsysnet("ipv4/neigh/default/gc_thresh3", "4096");
#ifdef TCONFIG_IPV6
	f_write_procsysnet("ipv6/neigh/default/gc_thresh1", "1");
	f_write_procsysnet("ipv6/neigh/default/gc_thresh2", "2048");
	f_write_procsysnet("ipv6/neigh/default/gc_thresh3", "4096");
#endif

	if (nvram_get_int("fw_nat_tuning") > 0) {
		f_write_procsysnet("core/netdev_max_backlog", "3072");
		f_write_procsysnet("core/somaxconn", "3072");
	}

	/* Medium buffers */
	if (nvram_get_int("fw_nat_tuning") == 1) {
		f_write_procsysnet("ipv4/tcp_max_syn_backlog", "4096");
		f_write_procsysnet("core/rmem_default", "262144");
		f_write_procsysnet("core/rmem_max", "262144");
		f_write_procsysnet("core/wmem_default", "262144");
		f_write_procsysnet("core/wmem_max", "262144");
		f_write_procsysnet("ipv4/tcp_rmem", "4096 131072 262144");
		f_write_procsysnet("ipv4/tcp_wmem", "4096 131072 262144");
		f_write_procsysnet("ipv4/udp_mem", "32768 131072 262144");
	}

	/* Large buffers */
	if (nvram_get_int("fw_nat_tuning") == 2) {
		f_write_procsysnet("ipv4/tcp_max_syn_backlog", "8192");
		f_write_procsysnet("core/rmem_default", "1040384");
		f_write_procsysnet("core/rmem_max", "1040384");
		f_write_procsysnet("core/wmem_default", "1040384");
		f_write_procsysnet("core/wmem_max", "1040384");
		f_write_procsysnet("ipv4/tcp_rmem", "8192 131072 992000");
		f_write_procsysnet("ipv4/tcp_wmem", "8192 131072 992000");
		f_write_procsysnet("ipv4/udp_mem", "122880 520192 992000");
	}

	f_write_procsysnet("ipv4/tcp_fin_timeout", "30");
	f_write_procsysnet("ipv4/tcp_keepalive_time", "1800");
	f_write_procsysnet("ipv4/tcp_keepalive_intvl", "24");
	f_write_procsysnet("ipv4/tcp_keepalive_probes", "3");
	f_write_procsysnet("ipv4/tcp_retries2", "5");
	f_write_procsysnet("ipv4/tcp_syn_retries", "3");
	f_write_procsysnet("ipv4/tcp_synack_retries", "3");
#if defined(TCONFIG_BCMARM)
	f_write_procsysnet("ipv4/tcp_tw_recycle", "0");
#else
	f_write_procsysnet("ipv4/tcp_tw_recycle", "1");
#endif
	f_write_procsysnet("ipv4/tcp_tw_reuse", "1");

	/* DoS-related tweaks */
	f_write_procsysnet("ipv4/icmp_ignore_bogus_error_responses", "1");
	f_write_procsysnet("ipv4/icmp_echo_ignore_broadcasts", "1");
	f_write_procsysnet("ipv4/tcp_rfc1337", "1");
	f_write_procsysnet("ipv4/ip_local_port_range", "1024 65535");
	f_write_procsysnet("ipv4/tcp_syncookies", (nvram_get_int("ne_syncookies") ? "1" : "0"));

	wanproto = get_wan_proto();
	f_write_procsysnet("ipv4/ip_dynaddr", ((wanproto == WP_DISABLED) || (wanproto == WP_STATIC)) ? "0" : "1");

	/* Force IGMPv2 if enabled via GUI (advanced-routing.asp) */
	/* 0 - (default) No enforcement of a IGMP version, IGMPv1/v2 fallback allowed. Will back to IGMPv3 mode again if all IGMPv1/v2 Querier Present timer expires. */
	f_write_procsysnet("ipv4/conf/default/force_igmp_version", (nvram_match("force_igmpv2", "1") ? "2" : "0"));
	f_write_procsysnet("ipv4/conf/all/force_igmp_version", (nvram_match("force_igmpv2", "1") ? "2" : "0"));

	/* Reduce and flush the route cache to ensure a more synchronous load balancing across multiwan */
	if (nvram_get_int("mwan_tune_gc")) {
		f_write_procsysnet("ipv4/route/flush", "1"); /* flush routing cache */
		f_write_procsysnet("ipv4/route/gc_elasticity", "1");
		f_write_procsysnet("ipv4/route/gc_interval", "1");
		f_write_procsysnet("ipv4/route/gc_min_interval_ms", "20");
		f_write_procsysnet("ipv4/route/gc_thresh", "1");
		f_write_procsysnet("ipv4/route/gc_timeout", "1");
#ifndef TCONFIG_BCMARM
		f_write_procsysnet("ipv4/route/max_delay", "1");
		f_write_procsysnet("ipv4/route/min_delay", "0");
		f_write_procsysnet("ipv4/route/secret_interval", "1");
#endif
	}
	else { /* back to std values */
		f_write_procsysnet("ipv4/route/gc_elasticity", "8");
		f_write_procsysnet("ipv4/route/gc_interval", "60");
		f_write_procsysnet("ipv4/route/gc_min_interval_ms", "500");
		f_write_procsysnet("ipv4/route/gc_timeout", "300");
#ifdef TCONFIG_BCMARM
		f_write_procsysnet("ipv4/route/gc_thresh", "2048");
#else
		f_write_procsysnet("ipv4/route/gc_thresh", "1024");
		f_write_procsysnet("ipv4/route/max_delay", "10");
		f_write_procsysnet("ipv4/route/min_delay", "2");
		f_write_procsysnet("ipv4/route/secret_interval", "600");
#endif
	}

	enable_blackhole_detection();

	chains_log_detection();

	for (n = 0; n < BRIDGE_COUNT; n++) {
		memset(buf1, 0, sizeof(buf1));
		snprintf(buf1, sizeof(buf1), (n == 0 ? "lan_ifname" : "lan%d_ifname"), n);
		strlcpy(lanface[n], nvram_safe_get(buf1), sizeof(lanface[n]));

		memset(buf1, 0, sizeof(buf1));
		snprintf(buf1, sizeof(buf1), (n == 0 ? "lan_ipaddr" : "lan%d_ipaddr"), n);
		strlcpy(lanaddr[n], nvram_safe_get(buf1), sizeof(lanaddr[n]));

		memset(buf1, 0, sizeof(buf1));
		snprintf(buf1, sizeof(buf1), (n == 0 ? "lan_netmask" : "lan%d_netmask"), n);
		strlcpy(lanmask[n], nvram_safe_get(buf1), sizeof(lanmask[n]));
	}

	memcpy(&wanfaces, get_wanfaces("wan"), sizeof(wanfaces));
	memcpy(&wan2faces, get_wanfaces("wan2"), sizeof(wan2faces));
#ifdef TCONFIG_MULTIWAN
	memcpy(&wan3faces, get_wanfaces("wan3"), sizeof(wan3faces));
	memcpy(&wan4faces, get_wanfaces("wan4"), sizeof(wan4faces));
#endif
	wanface = wanfaces.iface[0].name;
	wan2face = wan2faces.iface[0].name;
#ifdef TCONFIG_MULTIWAN
	wan3face = wan3faces.iface[0].name;
	wan4face = wan4faces.iface[0].name;
#endif
#ifdef TCONFIG_IPV6
	strlcpy(wan6face, get_wan6face(), sizeof(wan6face));
#endif

	can_enable_fastnat = 1;

	/*
		block obviously spoofed IP addresses

		rp_filter - BOOLEAN
			1 - do source validation by reversed path, as specified in RFC1812
			    Recommended option for single homed hosts and stub network
			    routers. Could cause troubles for complicated (not loop free)
			    networks running a slow unreliable protocol (sort of RIP),
			    or using static routes.
			0 - No source validation.
	*/

#ifdef TCONFIG_MULTIWAN
	const char* multiwan_wanfaces[] = { wanface, wan2face, wan3face, wan4face };
	const int multiwan_wanfaces_count = 4;
#else
	const char* multiwan_wanfaces[] = { wanface, wan2face };
	const int multiwan_wanfaces_count = 2;
#endif

	if ((dir = opendir("/proc/sys/net/ipv4/conf")) != NULL) {
		while ((dirent = readdir(dir)) != NULL) {
			if ((strcmp(dirent->d_name, ".") == 0) || (strcmp(dirent->d_name, "..") == 0))
				continue;

			memset(s, 0, 64);
			snprintf(s, sizeof(s), "/proc/sys/net/ipv4/conf/%s/rp_filter", dirent->d_name);
			bool enable_rp_filter = 1;

			for (n = 1; n <= multiwan_wanfaces_count; n++) {
				memset(buf1, 0, sizeof(buf1));
				snprintf(buf1, sizeof(buf1), "%d", n);
				memset(buf2, 0, sizeof(buf2));
				snprintf(buf2, sizeof(buf2), "wan%s_ifname", (n == 1 ? "" : buf1));
				c = nvram_safe_get(buf2);

				/* mcast needs rp filter to be turned off only for non default iface */
				if (!(nvram_match("multicast_pass", "1")) || !(nvram_match("udpxy_enable", "1")) || (strcmp(multiwan_wanfaces[n - 1], c) == 0))
					c = NULL;

				/* in gateway mode, rp_filter blocks pbr */
				if ((c != NULL && strcmp(dirent->d_name, c) == 0) || (strcmp(dirent->d_name, multiwan_wanfaces[n - 1]) == 0)) {
					enable_rp_filter = 0;
					break;
				}
			}

			f_write_string(s, (enable_rp_filter == 1 ? "1" : "0"), 0, 0);
		}
		closedir(dir);
	}
	f_write_procsysnet("ipv4/conf/default/rp_filter", "1");
	f_write_procsysnet("ipv4/conf/all/rp_filter", "0");

	/* Remote management */
	remotemanage = 0;

	if (nvram_match("remote_management", "1") && nvram_invmatch("http_wanport", "") && nvram_invmatch("http_wanport", "0"))
		remotemanage = 1;

	if (nvram_match("remote_mgt_https", "1")) {
		web_lanport = nvram_get_int("https_lanport");
		if (web_lanport <= 0)
			web_lanport = 443;
	}
	else {
		web_lanport = nvram_get_int("http_lanport");
		if (web_lanport <= 0)
			web_lanport = 80;
	}

	if ((ipt_file = fopen(ipt_fname, "w")) == NULL) {
		notice_set("iptables", "Unable to create iptables restore file");
		simple_unlock("firewall");
		return 0;
	}

#ifdef TCONFIG_IPV6
	if ((ip6t_file = fopen(ip6t_fname, "w")) == NULL) {
		notice_set("ip6tables", "Unable to create ip6tables restore file");
		simple_unlock("firewall");
		return 0;
	}
	modprobe("nf_conntrack_ipv6");
	modprobe("ip6t_REJECT");
#endif

#ifdef TCONFIG_BCMARM
	/* do not load IMQ modules */
#else
	modprobe("imq");
	modprobe("xt_IMQ");
#endif

	mangle_table();
	nat_table();
	filter_table();

	fclose(ipt_file);
	ipt_file = NULL;

#ifdef TCONFIG_IPV6
	fclose(ip6t_file);
	ip6t_file = NULL;
#endif

#ifdef DEBUG_IPTFILE
	if (debug_only) {
		simple_unlock("firewall");
		simple_unlock("restrictions");
		return 0;
	}
#endif

	save_webmon();

	if (nvram_get_int("upnp_enable") & 3) {
		f_write("/etc/upnp/save", NULL, 0, 0, 0);
		if (killall("miniupnpd", SIGUSR2) == 0)
			f_wait_notexists("/etc/upnp/save", 5);
	}

	/* Quite a few functions will blindly attempt to manipulate iptables, colliding with us.
	 * Retry a few times with increasing wait time to resolve collision.
	 */
	notice_set("iptables", "");
	for (n = 1; n < 4; n++) {
		if (_eval(iptrestore_argv, ">/var/notice/iptables", 0, NULL) == 0) {
			led(LED_DIAG, LED_OFF);
			notice_set("iptables", "");
			n = 4;
		}
		else {
			syslog(LOG_INFO, "iptables-restore failed - retrying in %d secs...", n*n);
			sleep(n*n);
		}
	}
	if (n < 5) {
		memset(s, 0, 64);
		snprintf(s, sizeof(s), "%s.error", ipt_fname);
		rename(ipt_fname, s);
		syslog(LOG_CRIT, "Error while loading rules. See %s file.", s);
		led(LED_DIAG, LED_ON);
	}

#ifdef TCONFIG_IPV6
	if (ipv6_enabled()) {
		/* Quite a few functions will blindly attempt to manipulate iptables, colliding with us.
		 * Retry a few times with increasing wait time to resolve collision.
		 */
		notice_set("ip6tables", "");
		for (n = 1; n < 4; n++) {
			if (_eval(ip6trestore_argv, ">/var/notice/ip6tables", 0, NULL) == 0) {
				notice_set("ip6tables", "");
				n = 4;
			}
			else {
				syslog(LOG_INFO, "ip6tables-restore failed - retrying in %d secs...", n*n);
				sleep(n*n);
			}
		}
		if (n < 5) {
			memset(s, 0, 64);
			snprintf(s, sizeof(s), "%s.error", ip6t_fname);
			rename(ip6t_fname, s);
			syslog(LOG_CRIT, "Error while loading rules. See %s file.", s);
			led(LED_DIAG, LED_ON);
		}
	}
	else {
		eval("ip6tables", "-F");
		eval("ip6tables", "-t", "mangle", "-F");
	}
#endif

	if (nvram_get_int("upnp_enable") & 3) {
		f_write("/etc/upnp/load", NULL, 0, 0, 0);
		killall("miniupnpd", SIGUSR2);
	}

	simple_unlock("restrictions");
	sched_restrictions();
	enable_ip_forward();

	led(LED_DMZ, dmz_dst(NULL));

#ifdef TCONFIG_IPV6
	modprobe_r("nf_conntrack_ipv6");
	modprobe_r("ip6t_LOG");
	modprobe_r("ip6t_REJECT");
#endif

	modprobe_r("xt_layer7");
	modprobe_r("xt_recent");
	modprobe_r("xt_HL");
	modprobe_r("xt_length");
#ifdef TCONFIG_BCMARM
	modprobe_r("ipt_web");
	modprobe_r("ipt_webmon");
#else
	modprobe_r("xt_web");
	modprobe_r("xt_webmon");
#endif
	modprobe_r("xt_dscp");
	modprobe_r("ipt_ipp2p");

	unlink("/var/webmon/domain");
	unlink("/var/webmon/search");

#ifdef TCONFIG_FTP
	run_ftpd_firewall_script();
#endif

#ifdef TCONFIG_PPTPD
	run_pptpd_firewall_script();
#endif

#ifdef TCONFIG_NGINX
	/* Web Server WAN access */
	run_nginx_firewall_script();
#endif

#ifdef TCONFIG_BT
	/* Open BT port/GUI WAN access */
	run_bt_firewall_script();
#endif

#ifdef TCONFIG_OPENVPN
	run_ovpn_firewall_scripts();
#endif

#ifdef TCONFIG_TINC
	run_tinc_firewall_script();
#endif

	run_nvscript("script_fire", NULL, 1);

	allow_fastnat("firewall", can_enable_fastnat);
	try_enabling_fastnat();

	simple_unlock("firewall");

	return 0;
}

int stop_firewall(void)
{
	led(LED_DMZ, LED_OFF);
	return 0;
}

#ifdef DEBUG_IPTFILE
void create_test_iptfile(void)
{
	debug_only = 1;
	start_firewall();
	debug_only = 0;
}
#endif
