/*
 *
 * FreshTomato Firmware
 *
 * Fixes/updates (C) 2018 - 2025 pedro
 * https://freshtomato.org/
 *
 */
 
 
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <bcmnvram.h>

#include "httpd.h"


void wo_wwansignal(char *url)
{
	char wancmd[32];
	int desired_wan = atoi(webcgi_safeget("mwan_num", "1"));

	if (desired_wan == 1)
		snprintf(wancmd, sizeof(wancmd), "wwansignal wan -stdout");
	else
		snprintf(wancmd, sizeof(wancmd), "wwansignal wan%d -stdout", desired_wan);

	web_puts("\nwwanstatus = '");
	web_pipecmd(wancmd, WOF_JAVASCRIPT);
	web_puts("';");
}

static char *getModemDiagPort(const char *wannum)
{
	char tmp[32];

	snprintf(tmp, sizeof(tmp), "%s_proto", wannum);
	if (nvram_match(tmp, "ppp3g")) {
		snprintf(tmp, sizeof(tmp), "%s_modem_dev", wannum);
		return nvram_safe_get(tmp);
	}
	else if (nvram_match(tmp, "lte")) {
		snprintf(tmp, sizeof(tmp), "%s_modem_type", wannum);
		if (nvram_match(tmp, "non-hilink") || nvram_match(tmp, "huawei-non-hilink") || nvram_match(tmp, "hw-ether")) {
			snprintf(tmp, sizeof(tmp), "%s_modem_dev", wannum);
			return nvram_safe_get(tmp);
		}
		else { /* QMI or HiLink */
			web_puts("\nwwansms_error = 'WWAN is not supported!'");
			return NULL;
		}
	}
	else {
		web_puts("\nwwansms_error = 'WAN is not WWAN!'"); /* TODO: SMS support for QMI */
		return NULL;
	}
}

void wo_wwansms(char *url)
{
	char smscmd[72], wannum[8];
	char *wwan_devId = NULL;
	int desired_wan = atoi(webcgi_safeget("mwan_num", "1"));

	snprintf(wannum, sizeof(wannum), "wan%c", (desired_wan == 1 ? '\0' : (char)desired_wan + 48));

	wwan_devId = getModemDiagPort(wannum);

	if (wwan_devId != NULL) {
		snprintf(smscmd, sizeof(smscmd), "gcom -d %s -s /etc/gcom/getsmses_plain.gcom", wwan_devId);

		web_puts("\n");
		web_puts("\nwwansms = '");
		web_pipecmd(smscmd, WOF_JAVASCRIPT);
		web_puts("';");
	}
}

void wo_wwansms_delete(char *url)
{
	char smscmd[150], wannum[8];
	char *wwan_devId = NULL;
	int desired_wan;
	const char *smsToRemove_str = webcgi_safeget("sms_num", "");
	const char *desired_wan_str = webcgi_safeget("mwan_num", "");

	if (!*smsToRemove_str) {
		web_puts("\nwwansms_error = 'sms_num is empty!'");
		return;
	}
	if (!*desired_wan_str) {
		web_puts("\nwwansms_error = 'mwan_num is empty!'");
		return;
	}
	desired_wan = atoi(desired_wan_str);
	snprintf(wannum, sizeof(wannum), "wan%c", (desired_wan == 1 ? '\0' : (char)desired_wan + 48));

	wwan_devId = getModemDiagPort(wannum);

	if (wwan_devId != NULL) {
		snprintf(smscmd, sizeof(smscmd), "MODE=\"AT+CMGD=%d\" gcom -d %s -s /etc/gcom/setverbose.gcom", atoi(smsToRemove_str), wwan_devId);

		web_puts("\n");
		web_puts("\nwwansms_delete = '");
		web_pipecmd(smscmd, WOF_JAVASCRIPT);
		web_puts("';");
	}
}
