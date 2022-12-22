/*

	Tomato Firmware
	Copyright (C) 2006-2009 Jonathan Zarate

*/


#include "rc.h"

#include <shared.h>


int get_lanports_status(int model)
{
	int r = 0;
	FILE *f;
	char s[128], a[16];

	if ((f = popen("/usr/sbin/robocfg showports", "r")) != NULL) {
		while (fgets(s, sizeof(s), f)) {
			/* LAN Ports: 0 1 2 3 */
			if (
#ifdef TCONFIG_BCMARM
#ifdef TCONFIG_BCM714
			    (model == MODEL_RTAC3100) ||
			    (model == MODEL_RTAC88U) ||
#endif /* TCONFIG_BCM714 */
			    (model == MODEL_RTAC56U)
#else
			    (model == MODEL_RTN15U)
#endif
			) {
				if ((sscanf(s, "Port 0: %s", a) == 1) ||
				    (sscanf(s, "Port 1: %s", a) == 1) ||
				    (sscanf(s, "Port 2: %s", a) == 1) ||
				    (sscanf(s, "Port 3: %s", a) == 1)) {
					if (strncmp(a, "DOWN", 4)) {
						r++;
					}
				}
			}
#ifdef TCONFIG_BCMARM
			/* LAN Ports: 1 2 3 4 */
			else if (
#ifdef TCONFIG_AC5300
				 (model == MODEL_RTAC5300) ||
#endif /* TCONFIG_AC5300 */
				 (model == MODEL_WS880) ||
			         (model == MODEL_RTN18U)) {
				if ((sscanf(s, "Port 1: %s", a) == 1) ||
				    (sscanf(s, "Port 2: %s", a) == 1) ||
				    (sscanf(s, "Port 3: %s", a) == 1) ||
				    (sscanf(s, "Port 4: %s", a) == 1)) {
					if (strncmp(a, "DOWN", 4)) {
						r++;
					}
				}
			}
#endif
		}
		pclose(f);
	}

	return r;
}

int blink_br_main(int argc, char *argv[])
{
	int model;

	/* Fork new process, run in the background (daemon) */
	if (fork() != 0)
		return 0;

	setsid();
	signal(SIGCHLD, chld_reap);

	/* get Router model */
	model = get_model();

	while(1) {
		if (
#ifdef TCONFIG_BCMARM
#ifdef TCONFIG_AC5300
		    (model == MODEL_RTAC5300) ||
#endif /* TCONFIG_AC5300 */
#ifdef TCONFIG_BCM714
		    (model == MODEL_RTAC3100) ||
		    (model == MODEL_RTAC88U) ||
#endif /* TCONFIG_BCM714 */
		    (model == MODEL_WS880) ||
		    (model == MODEL_RTN18U) ||
		    (model == MODEL_RTAC56U)
#else
		    (model == MODEL_RTN15U)
#endif
		) {
			if (get_lanports_status(model)) {
				led(LED_BRIDGE, LED_ON);
			}
			else {
				led(LED_BRIDGE, LED_OFF);
			}
		}
		else {
			/* nothing to do for this router --> exit / stop blink_br */
			exit(0);
		}
		/* sleep 3 sec before check again */
		sleep(3);
	}
}
