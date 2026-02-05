/*
 *
 * Tomato Firmware
 * Copyright (C) 2006-2009 Jonathan Zarate
 *
 * Fixes/updates (C) 2018 - 2026 pedro
 * https://freshtomato.org/
 *
 */


#include "tomato.h"

#include <bcmparams.h>

#ifndef MAX_NVPARSE
 #define MAX_NVPARSE		255
#endif

#if MWAN_MAX < 1 || MWAN_MAX > 8
 #error "Unsupported MWAN_MAX range"
#endif
#if BRIDGE_COUNT < 1 || BRIDGE_COUNT > 16
 #error "Unsupported BRIDGE_COUNT range"
#endif

#define DIG_ALL			2u
#define XIFS_PREFIX_LEN		13u /* "var xifs = [[" */
#define XIFS_SEP_LEN		3u  /* "],[" */
#define XIFS_SUFFIX_LEN		4u  /* "]];\n" */
#define XIFS_BUF_CAP ( \
	XIFS_PREFIX_LEN \
	+ (unsigned)(MWAN_MAX + BRIDGE_COUNT) * (6u + DIG_ALL) \
	+ XIFS_SEP_LEN \
	+ (unsigned)(MWAN_MAX + BRIDGE_COUNT) * (6u + DIG_ALL) \
	+ XIFS_SUFFIX_LEN \
	+ 1u \
)


static int in_arr(const char *k, const char *arr[], size_t n, int allow_prefix)
{
	size_t i, ls;
	const char *s;

	for (i = 0; i < n; ++i) {
		s = arr[i];
		if (!s)
			continue;

		if (allow_prefix) {
			ls = strlen(s);
			if (strncmp(s, k, ls) == 0)
				return 1;
		}
		else {
			if (strcmp(k, s) == 0)
				return 1;
		}
	}

	return 0;
}

static int print_wlnv(int idx, int unit, int subunit, void *param)
{
	char *k = param;
	char *nv;

	nv = wl_nvname(k + 3, unit, subunit);
	web_printf("\t'%s': '", nv); /* multiSSID */
	web_putj_utf8(nvram_safe_get(nv));
	web_puts("',\n");

	return 1;
}

/* static buffer */
static char g_xifs_buf[XIFS_BUF_CAP];

/* one time build */
static void build_xifs_into(char *buf, size_t buf_sz)
{
	char *p = buf;
	size_t n = buf_sz;
	int w;
	unsigned int i;

	#define APPEND(fmt, ...) \
		do { \
			w = snprintf(p, n, fmt, ##__VA_ARGS__); \
			if ((w < 0) || ((size_t)w >= n)) { \
				if (n) p[n - 1] = '\0'; \
					return; \
			} \
			p += w; \
			n -= (size_t)w; \
		} while (0)

	APPEND("var xifs = [[");

	for (i = 1; i <= MWAN_MAX; i++)
		APPEND((i == 1 ? "'wan'" : ",'wan%u'"), i);

	for (i = 0; i < BRIDGE_COUNT; i++)
		APPEND((i == 0 ? ",'lan'" : ",'lan%u'"), i);

	APPEND("],[");

	for (i = 1; i <= MWAN_MAX; i++)
		APPEND((i == 1 ? "'WAN%u'" : ",'WAN%u'"), (i - 1));

	for (i = 0; i < BRIDGE_COUNT; i++)
		APPEND(",'LAN%u'", i);

	APPEND("]];\n");
	#undef APPEND
}

static const char *xifs_once(void)
{
	static int built_xifs = 0;
	if (!built_xifs) {
		build_xifs_into(g_xifs_buf, sizeof(g_xifs_buf));
		built_xifs = 1;
	}

	return g_xifs_buf;
}

/*	<% jsdefaults(); %> ---> javascript defaults, defined in C */
void asp_jsdefaults(int argc, char **argv)
{
	/* global javascript variables */
	web_printf("\nMAX_BRIDGE_ID = %d;\nMAX_VLAN_ID = %d;\nMAXWAN_NUM = %d;\nMAX_PORT_ID = %d\n",
	           (BRIDGE_COUNT - 1), (TOMATO_VLANNUM - 1), MWAN_MAX, MAX_PORT_ID);

	web_puts(xifs_once());
}

static const char *chk_wan_vars[]  = { "wan_", "dr_wan_" };
static const char *skip_wan_vars[] = { "wan_dhcp_pass", "wan_domain", "wan_hostname", "wan_speed", "wan_wins" };

#ifndef TOMATO64
static const char *chk_lan_vars[]  = { "lan_", "dhcpd_", "udpxy_lan", "upnp_lan", "multicast_lan", "dr_lan_", "bwl_lan_", "dhcp_lease", "dnsmasq_pxelan", "vpn_server1_plan", "vpn_server2_plan" };
#endif /* TOMATO64 */
#ifdef TOMATO64
static const char *chk_lan_vars[]  = { "lan_", "dhcpd_", "udpxy_lan", "upnp_lan", "multicast_lan", "dr_lan_", "bwl_lan_", "dhcp_lease", "dnsmasq_pxelan", "vpn_server1_plan", "vpn_server2_plan", "vpn_server3_plan", "vpn_server4_plan" };
#endif /* TOMATO64 */
static const char *skip_lan_vars[] = { "lan_hwaddr", "lan_hwnames", "lan_dhcp", "lan_gateway", "lan_state", "lan_desc", "lan_invert", "lan_access", "dhcpd_static", "dhcpd_slt", "dhcpd_dmdns", "dhcpd_lmax", "dhcpd_gwmode" };

/* <% nvram("x,y,z"); %> ---> nvram = {'x': '1','y': '2','z': '3'};
 *
 * special cases:
 *   "wan_", "dr_wan_"
 *   "lan_", "dhcpd_", "udpxy_lan", "upnp_lan", "multicast_lan", "dr_lan_", "bwl_lan_", "dhcp_lease", "dnsmasq_pxelan", "vpn_server1_plan", "vpn_server2_plan"
 * - these variables only need the basic value entered in the nvram call in .asp scripts, without additional wanX/lanX
 *
 * WARNING! When you add another lan/wan related variable to nvram/asp files, and this is not so obvious,
 * so not simple "lan_" or "wan_" but ie. "something_lan_whatever", you must update asp_nvram() with this
 * and use only basic one (so "something_lan_whatever" is enough, no need for "something_lan1_whatever")
 *
 */
void asp_nvram(int argc, char **argv)
{
	char *list;
	char *p, *k;
	char buf[32];
	unsigned int i;

	if ((argc != 1) || ((list = strdup(argv[0])) == NULL))
		return;

	asp_jsdefaults(0, 0); /* to not have to add where nvram() already is */

	web_puts("\nnvram = {\n");
	p = list;
	while ((k = strsep(&p, ",")) != NULL) {
		if (*k == 0)
			continue;
		if (strcmp(k, "wl_unit") == 0)
			continue;

		web_printf("\t'%s': '", k);
		web_putj_utf8(nvram_safe_get(k));
		web_puts("',\n");

		/* wanX */
		if (in_arr(k, chk_wan_vars, ASIZE(chk_wan_vars), 1)) { /* check needed prefixes */
			if (in_arr(k, skip_wan_vars, ASIZE(skip_wan_vars), 0)) /* do not display these vars for other wans */
				continue;

			for (i = 2; i <= (MWAN_MAX < 4 ? 4 : MWAN_MAX); i++) {
			//for (i = 2; i <= MWAN_MAX; i++) { /* TODO: fix all .asp scripts (iteration by MAXWAN_NUM) to enable this */
				memset(buf, 0, sizeof(buf));
				if (strncmp(k, "wan_", 4) == 0)
					snprintf(buf, sizeof(buf), "wan%u%s", i, k + 3);
				else if (strncmp(k, "dr_wan_", 7) == 0)
					snprintf(buf, sizeof(buf), "dr_wan%u%s", i, k + 6);
				else
					continue;

				web_printf("\t'%s': '", buf);
				web_putj_utf8(nvram_safe_get(buf));
				web_puts("',\n");
			}
		}

		/* lanX */
		if (in_arr(k, chk_lan_vars, ASIZE(chk_lan_vars), 1)) { /* check needed prefixes */
			if (in_arr(k, skip_lan_vars, ASIZE(skip_lan_vars), 0)) /* do not display these vars for other lans */
				continue;

			for (i = 1; i < (BRIDGE_COUNT < 4 ? 4 : BRIDGE_COUNT); i++) {
			//for (i = 1; i < BRIDGE_COUNT; i++) { /* TODO: fix all .asp scripts (iteration by MAX_BRIDGE_ID) to enable this */
				memset(buf, 0, sizeof(buf));
				if (strncmp(k, "lan_", 4) == 0)
					snprintf(buf, sizeof(buf), "lan%u%s", i, k + 3);
				else if (strncmp(k, "udpxy_lan", 9) == 0)
					snprintf(buf, sizeof(buf), "udpxy_lan%u", i);
				else if (strncmp(k, "upnp_lan", 8) == 0)
					snprintf(buf, sizeof(buf), "upnp_lan%u", i);
				else if (strncmp(k, "multicast_lan", 13) == 0)
					snprintf(buf, sizeof(buf), "multicast_lan%u", i);
				else if (strncmp(k, "dr_lan_", 7) == 0)
					snprintf(buf, sizeof(buf), "dr_lan%u%s", i, k + 6);
				else if (strncmp(k, "bwl_lan_", 8) == 0)
					snprintf(buf, sizeof(buf), "bwl_lan%u%s", i, k + 7);
				else if (strncmp(k, "dhcpd_", 6) == 0)
					snprintf(buf, sizeof(buf), "dhcpd%u%s", i, k + 5);
				else if (strncmp(k, "dhcp_lease", 10) == 0)
					snprintf(buf, sizeof(buf), "dhcp%u%s", i, k + 4);
				else if (strncmp(k, "dnsmasq_pxelan", 14) == 0)
					snprintf(buf, sizeof(buf), "dnsmasq_pxelan%u", i);
				else if (strncmp(k, "vpn_server1_plan", 16) == 0)
					snprintf(buf, sizeof(buf), "vpn_server1_plan%u", i);
				else if (strncmp(k, "vpn_server2_plan", 16) == 0)
					snprintf(buf, sizeof(buf), "vpn_server2_plan%u", i);
#ifdef TOMATO64
				else if (strncmp(k, "vpn_server3_plan", 16) == 0)
					snprintf(buf, sizeof(buf), "vpn_server3_plan%u", i);
				else if (strncmp(k, "vpn_server4_plan", 16) == 0)
					snprintf(buf, sizeof(buf), "vpn_server4_plan%u", i);
#endif /* TOMATO64 */
				else
					continue;

				web_printf("\t'%s': '", buf);
				web_putj_utf8(nvram_safe_get(buf));
				web_puts("',\n");
			}
		}

		if (strncmp(k, "wl_", 3) == 0) {
			foreach_wif(1, k, print_wlnv);
		}
	}
	free(list);

	web_puts("\t'wl_unit': '"); /* multiSSID */
	web_putj(nvram_safe_get("wl_unit"));
	web_puts("',\n");

	web_puts("\t'http_id': '"); /* multiSSID */
	web_putj(nvram_safe_get("http_id"));
	web_puts("',\n");

	web_puts("\t'web_mx': '"); /* multiSSID */
	web_putj(nvram_safe_get("web_mx"));
	web_puts("',\n");

	web_puts("\t'web_pb': '"); /* multiSSID */
	web_putj(nvram_safe_get("web_pb"));
	web_puts("',\n");

	web_puts("\t'os_updated': '");
	web_putj((strcmp(nvram_safe_get("os_version_last"), tomato_shortver) ? "1" : "0"));
	web_puts("'};\n");
}

/* <% nvramseq('foo', 'bar%d', 5, 8); %> ---> foo = ['a','b','c']; */
void asp_nvramseq(int argc, char **argv)
{
	int i, e;
	char s[256];

	if (argc != 4)
		return;

	web_printf("\n%s = [\n", argv[0]);
	e = atoi(argv[3]);
	for (i = atoi(argv[2]); i <= e; ++i) {
		web_puts("'");
		snprintf(s, sizeof(s), argv[1], i);
		web_putj_utf8(nvram_safe_get(s));
		web_puts((i == e) ? "'" : "',");
	}
	web_puts("];\n");
}

void asp_nv(int argc, char **argv)
{
	if (argc == 1)
		web_puts(nvram_safe_get(argv[0]));
}

void asp_nvstat(int argc, char **argv)
{
	FILE *fp;
	struct nvram_header header;
	int part, size, used = 0;
	char s[20];

	if (mtd_getinfo("nvram", &part, &size)) {
		snprintf(s, sizeof(s), MTD_DEV(%dro), part);

		if ((fp = fopen(s, "r"))) {

#ifdef TCONFIG_BCMARM
#ifndef TCONFIG_NAND
			if (fseek(fp, size >= NVRAM_SPACE ? size - NVRAM_SPACE : 0, SEEK_SET) == 0)
#endif
				if ((fread(&header, sizeof(header), 1, fp) == 1) && (header.magic == NVRAM_MAGIC))
					used = header.len;
#else /* TCONFIG_BCMARM */
			if (nvram_match("boardtype", "0x052b") && nvram_match("boardrev", "02")) { /* Netgear 3500L v2 */
				if ((fread(&header, sizeof(header), 1, fp) == 1) && (header.magic == NVRAM_MAGIC))
					used = header.len;
			}
			else {
				if (fseek(fp, size >= NVRAM_SPACE ? size - NVRAM_SPACE : 0, SEEK_SET) == 0) {
					if ((fread(&header, sizeof(header), 1, fp) == 1) && (header.magic == NVRAM_MAGIC))
						used = header.len;
				}
			}
#endif /* TCONFIG_BCMARM */

			fclose(fp);
		}
	}

#if defined(TCONFIG_BCMARM) && (CONFIG_NVRAM_SIZE == 32) /* WORKAROUND for DIR868L to show 32 KB threshold at the GUI that should not be crossed right now! (you still can cross it...) */
	web_printf("\nnvstat = {size: %d,free: %d };\n", (CONFIG_NVRAM_SIZE * 0x0400), (CONFIG_NVRAM_SIZE * 0x0400) - used);
#else
	web_printf("\nnvstat = {size: %d,free: %d };\n", NVRAM_SPACE, NVRAM_SPACE - used);
#endif
}
