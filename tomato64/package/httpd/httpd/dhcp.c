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

#include <sys/sysinfo.h>
#include <sys/stat.h>
#include <sys/types.h>


void asp_dhcpc_time(int argc, char **argv)
{
	struct sysinfo si;
	char buf[32], expires_file[256];
	long exp, n;
	int r;
	unsigned int i;
	char prefix[] = "wanXX";

	for (i = 1; i <= MWAN_MAX; i++) {
		snprintf(prefix, sizeof(prefix), (i == 1 ? "wan" : "wan%u"), i);

		memset(expires_file, 0, sizeof(expires_file));
		snprintf(expires_file, sizeof(expires_file), "/var/lib/misc/dhcpc-%s.expires", prefix);

		if (using_dhcpc(prefix)) {
			exp = 0;
			r = f_read_string(expires_file, buf, sizeof(buf));
			if (r > 0) {
				n = atol(buf);
				if (n > 0) {
					sysinfo(&si);
					exp = n - si.uptime;
				}
			}

			web_printf("%s%s'", (i == 1 ? "'" : ",'"), reltime(exp, buf, sizeof(buf)));
		}
	}
}

void wo_dhcpc(char *url)
{
	char *p;
	char *argv[] = { NULL, NULL, NULL };
	pid_t pid;

	if ((p = webcgi_get("exec")) != NULL) {
		if (strcmp(p, "release") == 0)
			argv[0] = "dhcpc-release";
		else if (strcmp(p, "renew") == 0)
			argv[0] = "dhcpc-renew";

		argv[1] = webcgi_get("prefix");
		_eval(argv, NULL, 0, &pid);
	}

	common_redirect();
}

void wo_dhcpd(char *url)
{
	char *p, *w, *m;
	char *argv[5];
	pid_t pid;
#ifdef TOMATO64
	char buffer[128];
#endif /* TOMATO64 */

	if ((p = webcgi_get("remove")) != NULL) {
		f_write_string("/var/tmp/dhcp/delete", p, FW_CREATE | FW_NEWLINE, 0666);
		killall("dnsmasq", SIGUSR2);
		f_wait_notexists("/var/tmp/dhcp/delete", 5);
	}

	if (((w = webcgi_get("wl")) != NULL) && ((m = webcgi_get("mac")) != NULL)) {
#ifndef TOMATO64
		argv[0] = "wl";
		argv[1] = "-i";
		argv[2] = w;
		argv[3] = "deauthenticate";
		argv[4] = m;
		_eval(argv, NULL, 0, &pid);
#else
		snprintf(buffer, sizeof(buffer), "ubus call hostapd.%s del_client '{\"addr\":\"%s\", \"reason\":1, \"deauth\":true}'", w, m);
		system(buffer);
#endif /* TOMATO64 */
	}

	web_puts("{}");
}
