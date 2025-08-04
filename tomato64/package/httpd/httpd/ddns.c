/*
 *
 * Tomato Firmware
 * Copyright (C) 2007-2009 Jonathan Zarate
 * Fixes/updates (C) 2018 - 2025 pedro
 *
 */


#include "tomato.h"

#include <time.h>
#include <sys/stat.h>


void asp_ddnsx(int argc, char **argv)
{
	char *p, *q;
	unsigned int i;
#if !defined(TCONFIG_NVRAM_32K) && !defined(TCONFIG_OPTIMIZE_SIZE)
	unsigned int clients_num = 4;
#else
	unsigned int clients_num = 2;
#endif
	char s[64];
	char m[128];
	char name[64];
	time_t tt;
	struct stat st;

	web_puts("\nif (typeof nvram === 'undefined' || nvram.length == 0) nvram = { };");

	for (i = 1; i <= MWAN_MAX; i++) {
		memset(name, 0, sizeof(name));
		snprintf(name, sizeof(name), (i == 1 ? "wan" : "wan%u"), i);

		memset(s, 0, sizeof(s));
		snprintf(s, sizeof(s), (i == 1 ? "ddnsx_wanip" : "ddnsx%u_wanip"), i);
		web_printf("\n%s = '%s';", s, get_wanip(name));
		snprintf(s, sizeof(s), "%s_dns", name);
		snprintf(m, sizeof(m), "nvram.%s_dns", name);
		web_printf("\n%s = '%s';", m, nvram_safe_get(s));
		snprintf(s, sizeof(s), "%s_proto", name);
		snprintf(m, sizeof(m), "nvram.%s_proto", name);
		web_printf("\n%s = '%s';", m, nvram_safe_get(s));
	}

	web_printf("\nddnsx0_ip_get = '%s';", nvram_safe_get("ddnsx0_ip"));
	web_printf("\nddnsx1_ip_get = '%s';", nvram_safe_get("ddnsx1_ip"));
#if !defined(TCONFIG_NVRAM_32K) && !defined(TCONFIG_OPTIMIZE_SIZE)
	web_printf("\nddnsx2_ip_get = '%s';", nvram_safe_get("ddnsx2_ip"));
	web_printf("\nddnsx3_ip_get = '%s';", nvram_safe_get("ddnsx3_ip"));
#endif

	web_printf("\nnvram.dnscrypt_proxy = '%s';", nvram_safe_get("dnscrypt_proxy"));
	web_printf("\nnvram.stubby_proxy = '%s';", nvram_safe_get("stubby_proxy"));
	web_printf("\nnvram.dnscrypt_priority = '%s';", nvram_safe_get("dnscrypt_priority"));
	web_printf("\nnvram.stubby_priority = '%s';", nvram_safe_get("stubby_priority"));

	web_puts("\nddnsx_msg = [");

	for (i = 0; i < clients_num; ++i) {
		web_puts(i ? "','" : "'");
		snprintf(name, sizeof(name), "/var/lib/mdu/ddnsx%u.msg", i);
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
		snprintf(name, sizeof(name), "ddnsx%u", i);
		if (!nvram_match(name, "")) {
			snprintf(name, sizeof(name), "ddnsx%u_cache", i);
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
