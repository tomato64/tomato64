/*
 *
 * Tomato Firmware
 * Copyright (C) 2007-2009 Jonathan Zarate
 * Fixes/updates (C) 2018 - 2023 pedro
 *
 */


#include "tomato.h"

#include <time.h>
#include <sys/stat.h>


void asp_ddnsx(int argc, char **argv)
{
	char *p, *q;
	int i;
	char s[64];
	char m[128];
	char name[64];
	time_t tt;
	struct stat st;

	web_printf("\nddnsx_ip = '%s';", get_wanip("wan"));
	web_printf("\nddnsx2_ip = '%s';", get_wanip("wan2"));
#ifdef TCONFIG_MULTIWAN
	web_printf("\nddnsx3_ip = '%s';", get_wanip("wan3"));
	web_printf("\nddnsx4_ip = '%s';", get_wanip("wan4"));
#endif

	web_printf("\nddnsx_ip_nvram = '%s';", nvram_safe_get("ddnsx_ip"));
	web_printf("\nwan_dns_nvram = '%s';", nvram_safe_get("wan_dns"));
	web_printf("\nwan_get_dns_nvram = '%s';", nvram_safe_get("wan_get_dns"));
	web_printf("\ndns_addget_nvram = '%s';", nvram_safe_get("dns_addget"));

	web_puts("\nddnsx_msg = [");

	for (i = 0; i < 2; ++i) {
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

	for (i = 0; i < 2; ++i) {
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
