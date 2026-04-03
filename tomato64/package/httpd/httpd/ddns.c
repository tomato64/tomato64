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

#include <time.h>
#include <errno.h>
#include <sys/stat.h>

/* format a timestamp safely into buf (size buf_len).
 * returns 0 on success, -1 if localtime() fails.
 */
static int fmt_timestamp(time_t tt, char *buf, size_t buf_len)
{
	struct tm *tm_info;

	tm_info = localtime(&tt);
	if (tm_info == NULL)
		return -1;

	strftime(buf, buf_len, "%a, %d %b %Y %H:%M:%S %z: ", tm_info);
	return 0;
}

void asp_ddnsx(int argc, char **argv)
{
	char *p, *q;
	unsigned int i;
#if !defined(TCONFIG_NVRAM_32K) && !defined(TCONFIG_OPTIMIZE_SIZE)
	unsigned int clients_num = 4;
#else
	unsigned int clients_num = 2;
#endif
	char s[64], m[128], name[64], varname[128];
	time_t tt;
	struct stat st;

	web_puts("\nif (typeof nvram === 'undefined' || nvram.length == 0) nvram = { };");

	/* output WAN IP, DNS, and protocol for each WAN interface */
	for (i = 1; i <= MWAN_MAX; i++) {
		snprintf(name, sizeof(name), (i == 1 ? "wan" : "wan%u"), i);

		snprintf(varname, sizeof(varname), (i == 1 ? "ddnsx_wanip" : "ddnsx%u_wanip"), i);
		web_putj_nvram(varname, get_wanip(name));

		snprintf(s, sizeof(s), "%s_dns", name);
		snprintf(varname, sizeof(varname), "nvram.%s_dns", name);
		web_putj_nvram(varname, nvram_safe_get(s));

		snprintf(s, sizeof(s), "%s_proto", name);
		snprintf(varname, sizeof(varname), "nvram.%s_proto", name);
		web_putj_nvram(varname, nvram_safe_get(s));
	}

	web_putj_nvram("ddnsx0_ip_get", nvram_safe_get("ddnsx0_ip"));
	web_putj_nvram("ddnsx1_ip_get", nvram_safe_get("ddnsx1_ip"));
#if !defined(TCONFIG_NVRAM_32K) && !defined(TCONFIG_OPTIMIZE_SIZE)
	web_putj_nvram("ddnsx2_ip_get", nvram_safe_get("ddnsx2_ip"));
	web_putj_nvram("ddnsx3_ip_get", nvram_safe_get("ddnsx3_ip"));
#endif

	web_putj_nvram("nvram.dnscrypt_proxy", nvram_safe_get("dnscrypt_proxy"));
	web_putj_nvram("nvram.stubby_proxy", nvram_safe_get("stubby_proxy"));
	web_putj_nvram("nvram.dnscrypt_priority", nvram_safe_get("dnscrypt_priority"));
	web_putj_nvram("nvram.stubby_priority", nvram_safe_get("stubby_priority"));

	/* output DDNS status messages array */
	web_puts("\nddnsx_msg = [");

	for (i = 0; i < clients_num; ++i) {
		web_puts(i ? "','" : "'");
		snprintf(name, sizeof(name), "/var/lib/mdu/ddnsx%u.msg", i);
		f_read_string(name, m, sizeof(m)); /* null-terminated even on error */
		if (m[0] != 0) {
			if ((stat(name, &st) == 0) && (st.st_mtime > Y2K)) {
				if (fmt_timestamp(st.st_mtime, s, sizeof(s)) == 0)
					web_putj(s); /* escape timestamp - timezone may contain special chars */
			}
			web_putj(m);
		}
	}

	/* output last-updated timestamps array */
	web_puts("'];\nddnsx_last = [");

	for (i = 0; i < clients_num; ++i) {
		web_puts(i ? "','" : "'");
		snprintf(name, sizeof(name), "ddnsx%u", i);
		if (!nvram_match(name, "")) {
			snprintf(name, sizeof(name), "ddnsx%u_cache", i);
			if ((p = nvram_get(name)) == NULL)
				continue;

			errno = 0;
			tt = strtoul(p, &q, 10);
			if (errno || *q++ != ',')
				continue;

			if (tt > Y2K) {
				if (fmt_timestamp(tt, s, sizeof(s)) == 0)
					web_putj(s);
			}
			web_putj(q);
		}
	}

	web_puts("'];\n");
}
