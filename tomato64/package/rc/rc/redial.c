/*

	Copyright 2003, CyberTAN  Inc.  All Rights Reserved

	THIS SOFTWARE IS OFFERED "AS IS", AND CYBERTAN GRANTS NO WARRANTIES OF ANY
	KIND, EXPRESS OR IMPLIED, BY STATUTE, COMMUNICATION OR OTHERWISE.  CYBERTAN
	SPECIFICALLY DISCLAIMS ANY IMPLIED WARRANTIES OF MERCHANTABILITY, FITNESS
	FOR A SPECIFIC PURPOSE OR NONINFRINGEMENT CONCERNING THIS SOFTWARE

*/
/*

	Copyright 2005, Broadcom Corporation
	All Rights Reserved.

	THIS SOFTWARE IS OFFERED "AS IS", AND BROADCOM GRANTS NO WARRANTIES OF ANY
	KIND, EXPRESS OR IMPLIED, BY STATUTE, COMMUNICATION OR OTHERWISE. BROADCOM
	SPECIFICALLY DISCLAIMS ANY IMPLIED WARRANTIES OF MERCHANTABILITY, FITNESS
	FOR A SPECIFIC PURPOSE OR NONINFRINGEMENT CONCERNING THIS SOFTWARE.

*/


#include "rc.h"

#include <sys/ioctl.h>
#include <wait.h>


/* used in keepalive mode (ppp_demand=0) */
void start_redial(char *prefix)
{
	stop_redial(prefix);
	xstart("/sbin/redial", prefix);
}

void stop_redial(char *prefix)
{
	char tmp[32];
	int pid;
	pid = nvram_get_int(strlcat_r(prefix, "_ppp_redialpid", tmp, sizeof(tmp)));
	if (pid > 1) {
		while (kill(pid, SIGKILL) == 0) {
			sleep(1);
		}
	}
}

int redial_main(int argc, char **argv)
{
	int tm;
	int proto;
	int mwan_num;
	char c_pid[10];
	char tmp[32], tmp2[16];
	memset(c_pid, 0, 10);
	sprintf(c_pid, "%d", getpid());
	char prefix[] = "wanXX";
	char prefix_mwan[] = "wanXX";

	if (argc > 1)
		strcpy(prefix, argv[1]);
	else
		strcpy(prefix, "wan");

	strcpy(prefix_mwan, prefix);

	mwan_num = nvram_get_int("mwan_num");
	if ((mwan_num < 1) || (mwan_num > MWAN_MAX))
		mwan_num = 1;

	proto = get_wanx_proto(prefix);
	if (proto == WP_PPPOE || proto == WP_PPP3G || proto == WP_PPTP || proto == WP_L2TP)
		if (nvram_get_int(strlcat_r(prefix, "_ppp_demand", tmp, sizeof(tmp))) != 0)
			return 0;

	nvram_set(strlcat_r(prefix, "_ppp_redialpid", tmp, sizeof(tmp)), c_pid);

	tm = nvram_get_int(strlcat_r(prefix, "_ppp_redialperiod", tmp, sizeof(tmp))) ? : 30;
	if (tm < 5)
		tm = 5;

	syslog(LOG_INFO, "Redial (%s) started, the check interval is %d seconds", prefix, tm);

	sleep(10);

	while (1) {
		while (1) {
			sleep(tm);

			if (!check_wanup(prefix))
				break;
		}

		if ((!wait_action_idle(10)) || (check_wanup(prefix)))
			continue;

		if (!strcmp(prefix, "wan") && mwan_num != 1) {
			strcpy(prefix_mwan, "wan1");
		}

		memset(tmp, 0, 32);
		sprintf(tmp, "%s-restart", prefix_mwan);
		memset(tmp2, 0, 16);
		sprintf(tmp2, "%s-restart-c", prefix_mwan);

		if (nvram_match("action_service", "wan-restart") || nvram_match("action_service", tmp) || nvram_match("action_service", "wan-restart-c") || nvram_match("action_service", tmp2))
			syslog(LOG_INFO, "Redial: %s DOWN. Reconnect is already in progress ...", prefix);
		else {
			syslog(LOG_INFO, "Redial: %s DOWN. Reconnecting ...", prefix);
			xstart("service", (char *)prefix_mwan, "restart");
			break;
		}
	}

	return 0;
}
