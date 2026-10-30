/*
 *
 * Tomato Firmware
 * PPTP Server Support
 * Copyright (C) 2012 Augusto Bott
 *
 * Fixes/updates (C) 2018 - 2026 pedro
 * https://freshtomato.org/
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */


#include "tomato.h"

#ifndef IF_SIZE
 #define IF_SIZE 8
#endif


const char pptp_connected[] = "/etc/vpn/pptpd_connected";

void asp_pptpd_userol(int argc, char **argv)
{
	FILE *fp;
	char line[128];
	char clientusername[32 + 1];
	char clientlocalip[INET6_ADDRSTRLEN + 1];
	char clientremoteip[INET6_ADDRSTRLEN + 1];
	char interface[IF_SIZE + 1];
	int ppppid, clientuptime;
	char comma;
	char *js_if, *js_local, *js_remote, *js_user;

	web_puts("\n\npptpd_online=[");
	comma = ' ';

	fp = fopen(pptp_connected, "r");
	if (fp) {
		while (fgets(line, sizeof(line), fp) != NULL) {
			if (sscanf(line, "%d %8s %46s %46s %32s %d", &ppppid, interface, clientlocalip, clientremoteip, clientusername, &clientuptime) != 6)
				continue;

			js_if = js_string(interface);
			js_local = js_string(clientlocalip);
			js_remote = js_string(clientremoteip);
			js_user = js_string(clientusername);
			web_printf("%c['%d', '%s', '%s', '%s', '%s', '%d']", comma, ppppid, js_if ? js_if : "", js_local ? js_local : "", js_remote ? js_remote : "", js_user ? js_user : "", clientuptime);
			free(js_if);
			free(js_local);
			free(js_remote);
			free(js_user);
			comma = ',';
		}
		fclose(fp);
	}

	web_puts("];\n");
}

void wo_pptpdcmd(char *url)
{
	char *p;
	int n = 10;

	/* do we really need to output anything? */
	web_puts("\npptd_result = [\n");
	if ((p = webcgi_get("disconnect")) != NULL) {
		while ((kill(atoi(p), SIGTERM) == 0) && (n > 1)) {
			sleep(1);
			n--;
		}
	}
	web_puts("];\n");
}
