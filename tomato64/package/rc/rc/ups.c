/*
 * ups.c
 *
 * Copyright (C) 2011 shibby
 * Fixes/updates (C) 2018 - 2022 pedro
 *
 */


#include "rc.h"

#include <sys/stat.h>

#define APCUPSD_DATA	"/www/ext/cgi-bin/tomatodata.cgi"
#define APCUPSD_UPS	"/www/ext/cgi-bin/tomatoups.cgi"


void start_ups(void)
{
/*
 * Always copy and try to start service if USB support is enabled
 * If service will not find apc ups, then will turn off automaticaly
 */
	eval("cp", "/www/apcupsd/tomatodata.cgi", APCUPSD_DATA);
	eval("cp", "/www/apcupsd/tomatoups.cgi", APCUPSD_UPS);

	if (nvram_get_int("usb_apcupsd_custom")) /* use custom config file */
		xstart("apcupsd", "-f", "/etc/apcupsd.conf");
	else
		xstart("apcupsd");
}

void stop_ups(void)
{
	killall("apcupsd", SIGTERM);
	eval("rm", APCUPSD_DATA);
	eval("rm", APCUPSD_UPS);
}
