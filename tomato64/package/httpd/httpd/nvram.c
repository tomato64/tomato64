/*
 *
 * Tomato Firmware
 * Copyright (C) 2006-2009 Jonathan Zarate
 * Fixes/updates (C) 2018 - 2025 pedro
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

/*	<% nvram("x,y,z"); %> ---> nvram = {'x': '1','y': '2','z': '3'}; */
void asp_nvram(int argc, char **argv)
{
	char *list;
	char *p, *k;

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

		web_printf("\t'%s': '", k); /* multiSSID */
		web_putj_utf8(nvram_safe_get(k));
		web_puts("',\n");

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
	web_printf("\nnvstat = { size: %d, free: %d };\n", (CONFIG_NVRAM_SIZE * 0x0400), (CONFIG_NVRAM_SIZE * 0x0400) - used);
#else
	web_printf("\nnvstat = { size: %d, free: %d };\n", NVRAM_SPACE, NVRAM_SPACE - used);
#endif
}
