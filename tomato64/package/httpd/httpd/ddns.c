/*
 *
 * Tomato Firmware
 * Copyright (C) 2007-2009 Jonathan Zarate
 * Fixes/updates (C) 2018 - 2024 pedro
 *
 */


#include "tomato.h"

#include <time.h>
#include <sys/stat.h>


void asp_ddnsx(int argc, char **argv)
{
	char *p, *q;
	int i;
#if !defined(TCONFIG_NVRAM_32K) && !defined(TCONFIG_OPTIMIZE_SIZE)
	int clients_num = 4;
#else
	int clients_num = 2;
#endif
	char s[64];
	char m[128];
	char name[64];
	time_t tt;
	struct stat st;

	web_printf("\nddnsx_wanip = '%s';", get_wanip("wan"));
	web_printf("\nddnsx2_wanip = '%s';", get_wanip("wan2"));
#ifdef TCONFIG_MULTIWAN
	web_printf("\nddnsx3_wanip = '%s';", get_wanip("wan3"));
	web_printf("\nddnsx4_wanip = '%s';", get_wanip("wan4"));
#endif
	web_printf("\nddnsx0_ip_get = '%s';", nvram_safe_get("ddnsx0_ip"));
	web_printf("\nddnsx1_ip_get = '%s';", nvram_safe_get("ddnsx1_ip"));
#if !defined(TCONFIG_NVRAM_32K) && !defined(TCONFIG_OPTIMIZE_SIZE)
	web_printf("\nddnsx2_ip_get = '%s';", nvram_safe_get("ddnsx2_ip"));
	web_printf("\nddnsx3_ip_get = '%s';", nvram_safe_get("ddnsx3_ip"));
#endif

	web_puts("\nif (typeof nvram === 'undefined' || nvram.length == 0) nvram = { };");

	web_printf("\nnvram.wan_dns = '%s';", nvram_safe_get("wan_dns"));
	web_printf("\nnvram.wan2_dns = '%s';", nvram_safe_get("wan2_dns"));
#ifdef TCONFIG_MULTIWAN
	web_printf("\nnvram.wan3_dns = '%s';", nvram_safe_get("wan3_dns"));
	web_printf("\nnvram.wan4_dns = '%s';", nvram_safe_get("wan4_dns"));
#endif
	web_printf("\nnvram.wan_proto = '%s';", nvram_safe_get("wan_proto"));
	web_printf("\nnvram.wan2_proto = '%s';", nvram_safe_get("wan2_proto"));
#ifdef TCONFIG_MULTIWAN
	web_printf("\nnvram.wan3_proto = '%s';", nvram_safe_get("wan3_proto"));
	web_printf("\nnvram.wan4_proto = '%s';", nvram_safe_get("wan4_proto"));
#endif
	web_printf("\nnvram.dnscrypt_proxy = '%s';", nvram_safe_get("dnscrypt_proxy"));
	web_printf("\nnvram.stubby_proxy = '%s';", nvram_safe_get("stubby_proxy"));
	web_printf("\nnvram.dnscrypt_priority = '%s';", nvram_safe_get("dnscrypt_priority"));
	web_printf("\nnvram.stubby_priority = '%s';", nvram_safe_get("stubby_priority"));

	web_puts("\nddnsx_msg = [");

	for (i = 0; i < clients_num; ++i) {
		web_puts(i ? "','" : "'");
		snprintf(name, sizeof(name), "/var/lib/mdu/ddnsx%d.msg", i);
		f_read_string(name, m, sizeof(m)); /* null term'd even on error */
		if (m[0] != 0) {
			if ((stat(name, &st) == 0) && (st.st_mtime > Y2K)) {
				strftime(s, sizeof(s), "%a, %d %b %Y %H:%M:%S %z: ", localtime(&st.st_mtime));
				web_puts(s);
			}
			web_putj(m);
		}
	}

	web_puts("'];\nddnsx_last = [");

	for (i = 0; i < clients_num; ++i) {
		web_puts(i ? "','" : "'");
		snprintf(name, sizeof(name), "ddnsx%d", i);
		if (!nvram_match(name, "")) {
			snprintf(name, sizeof(name), "ddnsx%d_cache", i);
			if ((p = nvram_get(name)) == NULL)
				continue;

			tt = strtoul(p, &q, 10);
			if (*q++ != ',')
				continue;

			if (tt > Y2K) {
				strftime(s, sizeof(s), "%a, %d %b %Y %H:%M:%S %z: ", localtime(&tt));
				web_puts(s);
			}
			web_putj(q);
		}
	}

	web_puts("'];\n");
}
