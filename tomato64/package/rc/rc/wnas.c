/*

	Copyright 2003, CyberTAN  Inc.  All Rights Reserved

	This is UNPUBLISHED PROPRIETARY SOURCE CODE of CyberTAN Inc.
	the contents of this file may not be disclosed to third parties,
	copied or duplicated in any form without the prior written
	permission of CyberTAN Inc.

	This software should be used as a reference only, and it not
	intended for production use!

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

#include <sys/sysinfo.h>
#include <sys/ioctl.h>
#include <bcmutils.h>
#include <wlutils.h>


/* ref: http://wiki.openwrt.org/OpenWrtDocs/nas */
// #define DEBUG_TIMING

/* needed by logmsg() */
#define LOGMSG_DISABLE	DISABLE_SYSLOG_OS
#define LOGMSG_NVDEBUG	"wnas_debug"


void notify_nas(const char *ifname);

static int security_on(int idx, int unit, int subunit, void *param)
{
	return nvram_get_int(wl_nvname("radio", unit, 0)) && (!nvram_match(wl_nvname("security_mode", unit, subunit), "disabled"));
}

static int is_wds(int idx, int unit, int subunit, void *param)
{
	return nvram_get_int(wl_nvname("radio", unit, 0)) && nvram_get_int(wl_nvname("wds_enable", unit, subunit));
}

int wds_enable(void)
{
	return foreach_wif(1, NULL, is_wds);
}

int wl_security_on(void)
{
	return foreach_wif(1, NULL, security_on);
}

void start_nas(void)
{
	FILE *fd;
	char *ifname, buf[256];

	if (!foreach_wif(1, NULL, security_on))
		return;

#ifdef DEBUG_TIMING
	struct sysinfo si;
	sysinfo(&si);
	logmsg(LOG_DEBUG, "*** %s: uptime=%ld", __FUNCTION__, si.uptime);
#else
	logmsg(LOG_DEBUG, "*** %s", __FUNCTION__);
#endif	/* DEBUG_TIMING */

	stop_nas();

	setenv("UDP_BIND_IP", "127.0.0.1", 1);
	eval("eapd");
	unsetenv("UDP_BIND_IP");
	eval("nas");

	if (wds_enable()) {	/* notify NAS of all wds up ifaces upon startup */
		if ((fd = fopen("/proc/net/dev", "r")) != NULL) {
			fgets(buf, sizeof(buf) - 1, fd);	/* header lines */
			fgets(buf, sizeof(buf) - 1, fd);
			while (fgets(buf, sizeof(buf) - 1, fd)) {
				if ((ifname = strchr(buf, ':')) == NULL)
					continue;

				*ifname = 0;
				if ((ifname = strrchr(buf, ' ')) == NULL)
					ifname = buf;
				else
					++ifname;

				if (strstr(ifname, "wds"))
					notify_nas(ifname);
			}
			fclose(fd);
		}
	}
}

void stop_nas(void)
{
#ifdef DEBUG_TIMING
	struct sysinfo si;
	sysinfo(&si);
	logmsg(LOG_DEBUG, "*** %s: uptime=%ld", __FUNCTION__, si.uptime);
#else
	logmsg(LOG_DEBUG, "*** %s", __FUNCTION__);
#endif	/* DEBUG_TIMING */

	killall_tk_period_wait("nas", 50);
	killall_tk_period_wait("eapd", 50);
}

void notify_nas(const char *ifname)
{
#ifdef DEBUG_TIMING
	struct sysinfo si;
	sysinfo(&si);
	logmsg(LOG_DEBUG, "*** %s: ifname=%s uptime=%ld", __FUNCTION__, ifname, si.uptime);
#else
	logmsg(LOG_DEBUG, "*** %s: ifname=%s", __FUNCTION__, ifname);
#endif	/* DEBUG_TIMING */

	/* Inform driver to send up new WDS link event */
	if (wl_iovar_setint((char *)ifname, "wds_enable", 1))
		logmsg(LOG_DEBUG, "*** %s: %s - set wds_enable failed", __FUNCTION__, ifname);
}
