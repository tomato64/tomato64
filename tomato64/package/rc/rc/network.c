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
/*

	wificonf, OpenWRT
	Copyright (C) 2005 Felix Fietkau <nbd@vd-s.ath.cx>
	
*/
/*

	Modified for Tomato Firmware
	Portions, Copyright (C) 2006-2009 Jonathan Zarate

*/


#include "rc.h"

#ifndef UU_INT
typedef u_int64_t u64;
typedef u_int32_t u32;
typedef u_int16_t u16;
typedef u_int8_t u8;
#endif

#include <linux/types.h>
#include <linux/sockios.h>
#include <linux/ethtool.h>
#include <sys/ioctl.h>
#include <net/if_arp.h>
#include <arpa/inet.h>
#include <dirent.h>

#include <wlutils.h>
#include <bcmparams.h>
#include <wlioctl.h>

#ifndef WL_BSS_INFO_VERSION
#error WL_BSS_INFO_VERSION
#endif
#if WL_BSS_INFO_VERSION >= 108
#include <etioctl.h>
#else
#include <etsockio.h>
#endif
#ifdef TCONFIG_BCM714
#include <sys/reboot.h>
#endif

#ifdef TCONFIG_BCMWL6
#ifdef TCONFIG_BCMARM
#include <d11.h>
#endif
#include <wlif_utils.h>
#include <security_ipc.h>
#ifdef TCONFIG_BCMARM
#define WLCONF_PHYTYPE2STR(phy)	((phy) == PHY_TYPE_A ? "a" : \
				 (phy) == PHY_TYPE_B ? "b" : \
				 (phy) == PHY_TYPE_LP ? "l" : \
				 (phy) == PHY_TYPE_G ? "g" : \
				 (phy) == PHY_TYPE_SSN ? "s" : \
				 (phy) == PHY_TYPE_HT ? "h" : \
				 (phy) == PHY_TYPE_AC ? "v" : \
				 (phy) == PHY_TYPE_LCN ? "c" : "n")
#endif
#endif /* TCONFIG_BCMWL6 */

#define WL_MAX_ASSOC		128
#define STACHECK_CONNECT	30
#define STACHECK_DISCONNECT	5

/* needed by logmsg() */
#define LOGMSG_DISABLE	DISABLE_SYSLOG_OSM
#define LOGMSG_NVDEBUG	"network_debug"


void restart_wl(void);
void stop_lan_wl(void);
void start_lan_wl(void);
#ifdef TCONFIG_BCMWL6
int wl_sta_prepare(void);
void wl_sta_start(void);
void wl_sta_stop(void);
int wl_send_dif_event(const char *ifname, uint32 event);
#endif

enum {
	RADIO_OFF = 0,
	RADIO_ON = 1,
	RADIO_TOGGLE = 2
};

#ifdef TCONFIG_BCMARM
void wlconf_pre(void)
{
	int unit = 0;
	char word[128], *next;
	char tmp[128], prefix[] = "wlXXXXXXXXXX_";
	char buf[16] = {0};
	wlc_rev_info_t rev;
#ifdef TCONFIG_BCMBSD
	int smart_conn = nvram_get_int("smart_connect_x");
#endif

	foreach (word, nvram_safe_get("wl_ifnames"), next) {
		snprintf(prefix, sizeof(prefix), "wl%d_", unit);

#ifdef TCONFIG_BCMBSD
		nvram_set(strlcat_r(prefix, "probresp_sw", tmp, sizeof(tmp)), smart_conn ? "1" : "0"); /* turn On with wireless band steering otherwise Off */
#endif

		/* for TxBeamforming: get corerev for TxBF check */
		wl_ioctl(word, WLC_GET_REVINFO, &rev, sizeof(rev));
		snprintf(buf, sizeof(buf), "%d", rev.corerev);
		nvram_set(strlcat_r(prefix, "corerev", tmp, sizeof(tmp)), buf);

		if (rev.corerev < 40) { /* TxBF unsupported - turn off and hide options (at the GUI) */
			logmsg(LOG_DEBUG, "*** %s: TxBeamforming not supported for %s", __FUNCTION__, word);
			nvram_set(strlcat_r(prefix, "txbf_bfr_cap", tmp, sizeof(tmp)), "0"); /* off = 0 */
			nvram_set(strlcat_r(prefix, "txbf_bfe_cap", tmp, sizeof(tmp)), "0");
			nvram_set(strlcat_r(prefix, "txbf", tmp, sizeof(tmp)), "0");
			nvram_set(strlcat_r(prefix, "itxbf", tmp, sizeof(tmp)), "0");
			nvram_set(strlcat_r(prefix, "txbf_imp", tmp, sizeof(tmp)), "0");
		}
		else {
			/* nothing to do right now! - use default nvram config or desired user wlan setup */
			logmsg(LOG_DEBUG, "*** %s: TxBeamforming supported for %s - corerev: %s", __FUNCTION__, word, buf);
			logmsg(LOG_DEBUG, "*** %s: txbf_bfr_cap for %s = %s", __FUNCTION__, word, nvram_safe_get(strlcat_r(prefix, "txbf_bfr_cap", tmp, sizeof(tmp))));
			logmsg(LOG_DEBUG, "*** %s: txbf_bfe_cap for %s = %s", __FUNCTION__, word, nvram_safe_get(strlcat_r(prefix, "txbf_bfe_cap", tmp, sizeof(tmp))));
		}

		if (nvram_match(strlcat_r(prefix, "nband", tmp, sizeof(tmp)), "1") && /* only for wlX_nband == 1 for 5 GHz */
		    nvram_match(strlcat_r(prefix, "vreqd", tmp, sizeof(tmp)), "1") &&
		    nvram_match(strlcat_r(prefix, "nmode", tmp, sizeof(tmp)), "-1")) { /* only for mode AUTO == -1 */

#ifdef TCONFIG_BCM714
			if (nvram_match(strlcat_r(prefix, "turbo_qam", tmp, sizeof(tmp)), "1") ||
			    nvram_match(strlcat_r(prefix, "turbo_qam", tmp, sizeof(tmp)), "2")) { /* check turbo/nitro qam on or off ? (keep it simple) */
				logmsg(LOG_DEBUG, "*** %s: set vht_features 4 for %s", __FUNCTION__, word);
				eval("wl", "-i", word, "vht_features", "4");
			}
			else {
				logmsg(LOG_DEBUG, "*** %s: set vht_features 0 for %s", __FUNCTION__, word);
				eval("wl", "-i", word, "vht_features", "0");
			}
#endif /* TCONFIG_BCM714 */

			logmsg(LOG_DEBUG, "*** %s: set vhtmode 1 for %s", __FUNCTION__, word);
			eval("wl", "-i", word, "vhtmode", "1");
		}
		else if (nvram_match(strlcat_r(prefix, "nband", tmp, sizeof(tmp)), "2") && /* only for wlX_nband == 2 for 2,4 GHz */
		         nvram_match(strlcat_r(prefix, "vreqd", tmp, sizeof(tmp)), "1") &&
		         nvram_match(strlcat_r(prefix, "nmode", tmp, sizeof(tmp)), "-1")) { /* only for mode AUTO == -1 */


			if (nvram_match(strlcat_r(prefix, "turbo_qam", tmp, sizeof(tmp)), "1")) { /* check turbo qam on or off ? */
				logmsg(LOG_DEBUG, "*** %s: set vht_features 3 for %s", __FUNCTION__, word);
				eval("wl", "-i", word, "vht_features", "3");
			}
#ifdef TCONFIG_BCM714
			else if (nvram_match(strlcat_r(prefix, "turbo_qam", tmp, sizeof(tmp)), "2")) { /* check nitro qam on or off ? */
				logmsg(LOG_DEBUG, "*** %s: set vht_features 7 for %s", __FUNCTION__, word);
				eval("wl", "-i", word, "vht_features", "7");
			}
#endif /* TCONFIG_BCM714 */
			else {
				logmsg(LOG_DEBUG, "*** %s: set vht_features 0 for %s", __FUNCTION__, word);
				eval("wl", "-i", word, "vht_features", "0");
			}

			logmsg(LOG_DEBUG, "*** %s: set vhtmode 1 for %s", __FUNCTION__, word);
			eval("wl", "-i", word, "vhtmode", "1");

		}
		else {
			logmsg(LOG_DEBUG, "*** %s: set vhtmode 0 for %s", __FUNCTION__, word);
			eval("wl", "-i", word, "vht_features", "0");
			eval("wl", "-i", word, "vhtmode", "0");
		}
		unit++;
	}
}
#endif /* TCONFIG_BCMARM */

#if defined(TCONFIG_EBTABLES) && (defined(TCONFIG_BCMARM) || !defined(TCONFIG_BCMWL6)) /* for all branches, except SDK6 mips (RT-AC) */
static void bridges_flush_all_chains(void)
{
	/* every chain will be flushed */
	eval("ebtables", "-F");
}

/* Note: ebtables restarts MIPS RT-AC routers - on some commands - from version v132 @shibby and even earlier... */
static void bridges_block_all_ipv6(void)
{
	/* basic filter table configuration - block all IPv6 */
	eval("ebtables", "-I", "INPUT", "-p", "IPv6", "-j", "DROP");
	eval("ebtables", "-I", "FORWARD", "-p", "IPv6", "-j", "DROP");
	eval("ebtables", "-I", "OUTPUT", "-p", "IPv6", "-j", "DROP");
}
#endif /* defined(TCONFIG_EBTABLES) && (defined(TCONFIG_BCMARM) || !defined(TCONFIG_BCMWL6)) */

static void set_lan_hostname(const char *wan_hostname)
{
	const char *s;
	char *lan_hostname;
	char hostname[16];
	FILE *f;

	nvram_set("lan_hostname", wan_hostname);
	if ((wan_hostname == NULL) || (*wan_hostname == 0)) {
		/* derive from et0 mac address */
		s = nvram_get("lan_hwaddr");
		if (s && strlen(s) >= 17) {
			snprintf(hostname, sizeof(hostname), "FT-%c%c%c%c%c%c%c%c%c%c%c%c", s[0], s[1], s[3], s[4], s[6], s[7], s[9], s[10], s[12], s[13], s[15], s[16]);

			if ((f = fopen("/proc/sys/kernel/hostname", "w"))) {
				fputs(hostname, f);
				fclose(f);
			}
			nvram_set("lan_hostname", hostname);
		}
	}

	lan_hostname = nvram_safe_get("lan_hostname");
	if ((f = fopen("/etc/hosts", "w"))) {
		fprintf(f, "127.0.0.1 localhost\n");

		if ((s = nvram_get("lan_ipaddr")) && (*s))
			fprintf(f, "%s %s %s-lan\n", s, lan_hostname, lan_hostname);
		if ((s = nvram_get("lan1_ipaddr")) && (*s) && (strcmp(s, "") != 0))
			fprintf(f, "%s %s-lan1\n", s, lan_hostname);
		if ((s = nvram_get("lan2_ipaddr")) && (*s) && (strcmp(s, "") != 0))
			fprintf(f, "%s %s-lan2\n", s, lan_hostname);
		if ((s = nvram_get("lan3_ipaddr")) && (*s) && (strcmp(s, "") != 0))
			fprintf(f, "%s %s-lan3\n", s, lan_hostname);
#ifdef TCONFIG_IPV6
		if (ipv6_enabled()) {
			fprintf(f, "::1 localhost ip6-localhost ip6-loopback\n");
			s = ipv6_router_address(NULL);
			if (*s)
				fprintf(f, "%s %s\n", s, lan_hostname);
		}
#endif
		fclose(f);
	} else
		logerr(__FUNCTION__, __LINE__, "/etc/hosts");
}

void set_host_domain_name(void)
{
	const char *s;

	s = nvram_safe_get("wan_hostname");
	sethostname(s, strlen(s));
	set_lan_hostname(s);

	s = nvram_get("wan_domain");
	if ((s == NULL) || (*s == 0))
		s = nvram_safe_get("wan_get_domain");

	setdomainname(s, strlen(s));
}

static int wlconf(char *ifname, int unit, int subunit)
{
	int r = -1;
	char wl[24];

#ifdef TCONFIG_BCMARM
	int phytype;
	char buf[8] = {0};
	char tmp[128] = {0};
#endif
#ifdef TCONFIG_BCMBSD
	char word[128], *next;
	char prefix[] = "wlXXXXXXX_";
	char prefix2[] = "wlXXXXXXX_";
	char tmp2[128];
	int wlif_count = 0;
	int i = 0;
#endif

	/* Check interface - fail for non-wl interfaces */
	if ((unit < 0) || wl_probe(ifname))
		return r;

	logmsg(LOG_DEBUG, "*** %s: wlconf: ifname %s unit %d subunit %d", __FUNCTION__, ifname, unit, subunit);

#ifndef TCONFIG_BCMARM
	/* Tomato MIPS needs a little help here: wlconf() will not validate/restore all per-interface related variables right now; */
	snprintf(wl, sizeof(wl), "--wl%d", unit);
	eval("nvram", "validate", wl); /* sync wl_ and wlX ; (MIPS does not use nvram_tuple router_defaults; ARM branch does ... ) */
	memset(wl, 0, sizeof(wl)); /* reset */
#endif

#ifdef TCONFIG_BCMARM
	/* set phytype */
	if ((subunit == -1) && !wl_ioctl(ifname, WLC_GET_PHYTYPE, &phytype, sizeof(phytype))) {
		snprintf(wl, sizeof(wl), "wl%d_", unit);
		snprintf(buf, sizeof(buf), "%s", WLCONF_PHYTYPE2STR(phytype));
		nvram_set(strlcat_r(wl, "phytype", tmp, sizeof(tmp)), buf);
		logmsg(LOG_DEBUG, "*** %s: wlconf: %s = %s", __FUNCTION__, tmp, buf);
		memset(wl, 0, sizeof(wl)); /* reset */
	}
#endif
	
#ifdef TCONFIG_BCMBSD
	if (subunit == -1) {
		snprintf(prefix, sizeof(prefix), "wl%d_", unit);
		if ((unit == 0) && nvram_get_int("smart_connect_x") == 1) { /* band steering enabled --> sync wireless settings to first module */

			foreach(word, nvram_safe_get("wl_ifnames"), next)
				wlif_count++;

			for (i = unit + 1; i < wlif_count; i++) {
				snprintf(prefix2, sizeof(prefix2), "wl%d_", i);
				nvram_set(strlcat_r(prefix2, "ssid", tmp2, sizeof(tmp2)), nvram_safe_get(strlcat_r(prefix, "ssid", tmp, sizeof(tmp))));
				nvram_set(strlcat_r(prefix2, "key", tmp2, sizeof(tmp2)), nvram_safe_get(strlcat_r(prefix, "key", tmp, sizeof(tmp))));
				nvram_set(strlcat_r(prefix2, "key1", tmp2, sizeof(tmp2)), nvram_safe_get(strlcat_r(prefix, "key1", tmp, sizeof(tmp))));
				nvram_set(strlcat_r(prefix2, "key2", tmp2, sizeof(tmp2)), nvram_safe_get(strlcat_r(prefix, "key2", tmp, sizeof(tmp))));
				nvram_set(strlcat_r(prefix2, "key3", tmp2, sizeof(tmp2)), nvram_safe_get(strlcat_r(prefix, "key3", tmp, sizeof(tmp))));
				nvram_set(strlcat_r(prefix2, "key4", tmp2, sizeof(tmp2)), nvram_safe_get(strlcat_r(prefix, "key4", tmp, sizeof(tmp))));
				nvram_set(strlcat_r(prefix2, "crypto", tmp2, sizeof(tmp2)), nvram_safe_get(strlcat_r(prefix, "crypto", tmp, sizeof(tmp))));
				nvram_set(strlcat_r(prefix2, "wpa_psk", tmp2, sizeof(tmp2)), nvram_safe_get(strlcat_r(prefix, "wpa_psk", tmp, sizeof(tmp))));
				nvram_set(strlcat_r(prefix2, "radius_ipaddr", tmp2, sizeof(tmp2)), nvram_safe_get(strlcat_r(prefix, "radius_ipaddr", tmp, sizeof(tmp))));
				nvram_set(strlcat_r(prefix2, "radius_key", tmp2, sizeof(tmp2)), nvram_safe_get(strlcat_r(prefix, "radius_key", tmp, sizeof(tmp))));
				nvram_set(strlcat_r(prefix2, "radius_port", tmp2, sizeof(tmp2)), nvram_safe_get(strlcat_r(prefix, "radius_port", tmp, sizeof(tmp))));
				nvram_set(strlcat_r(prefix2, "closed", tmp2, sizeof(tmp2)), nvram_safe_get(strlcat_r(prefix, "closed", tmp, sizeof(tmp))));
			}
		}
	}
#endif /* TCONFIG_BCMBSD */

	r = eval("wlconf", ifname, "up");
	logmsg(LOG_DEBUG, "*** %s: wlconf %s = %d", __FUNCTION__, ifname, r);
	if (r == 0) {
		if (unit >= 0 && subunit <= 0) {
			/* setup primary wl interface */
			nvram_set("rrules_radio", "-1");

			eval("wl", "-i", ifname, "antdiv", nvram_safe_get(wl_nvname("antdiv", unit, 0)));
			eval("wl", "-i", ifname, "txant", nvram_safe_get(wl_nvname("txant", unit, 0)));
			eval("wl", "-i", ifname, "txpwr1", "-o", "-m", nvram_get_int(wl_nvname("txpwr", unit, 0)) ? nvram_safe_get(wl_nvname("txpwr", unit, 0)) : "-1");
#ifdef TCONFIG_BCMARM
			eval("wl", "-i", ifname, "interference", nvram_match(wl_nvname("phytype", unit, 0), "v") ? nvram_safe_get(wl_nvname("mitigation_ac", unit, 0)) : nvram_safe_get(wl_nvname("mitigation", unit, 0)));
#else
			eval("wl", "-i", ifname, "interference", nvram_safe_get(wl_nvname("mitigation", unit, 0)));
#endif
		}

		if (wl_client(unit, subunit)) {
			if (nvram_match(wl_nvname("mode", unit, subunit), "wet")) {
				ifconfig(ifname, IFUP | IFF_ALLMULTI, NULL, NULL);
			}
			if (nvram_get_int(wl_nvname("radio", unit, 0))) {
				snprintf(wl, sizeof(wl), "%d", unit);
				xstart("radio", "join", wl);
			}
		}
	}
	return r;
}

#ifdef TCONFIG_EMF
static void emf_mfdb_update(char *lan_ifname, char *lan_port_ifname, bool add)
{
	char word[256], *next;
	char *mgrp, *ifname;

	/* Add/Delete MFDB entries corresponding to new interface */
	foreach (word, nvram_safe_get("emf_entry"), next) {
		ifname = word;
		mgrp = strsep(&ifname, ":");

		if ((mgrp == NULL) || (ifname == NULL))
			continue;

		/* Add/Delete MFDB entry using the group addr and interface */
		if (lan_port_ifname == NULL || strcmp(lan_port_ifname, ifname) == 0) {
			eval("emf", ((add) ? "add" : "del"), "mfdb", lan_ifname, mgrp, ifname);
		}
	}
}

static void emf_uffp_update(char *lan_ifname, char *lan_port_ifname, bool add)
{
	char word[256], *next;
	char *ifname;

	/* Add/Delete UFFP entries corresponding to new interface */
	foreach (word, nvram_safe_get("emf_uffp_entry"), next) {
		ifname = word;

		if (ifname == NULL)
			continue;

		/* Add/Delete UFFP entry for the interface */
		if (lan_port_ifname == NULL || strcmp(lan_port_ifname, ifname) == 0) {
			eval("emf", ((add) ? "add" : "del"), "uffp", lan_ifname, ifname);
		}
	}
}

static void emf_rtport_update(char *lan_ifname, char *lan_port_ifname, bool add)
{
	char word[256], *next;
	char *ifname;

	/* Add/Delete RTPORT entries corresponding to the new interface */
	foreach (word, nvram_safe_get("emf_rtport_entry"), next) {
		ifname = word;

		if (ifname == NULL)
			continue;

		/* Add/Delete RTPORT entry for the interface */
		if (lan_port_ifname == NULL || strcmp(lan_port_ifname, ifname) == 0) {
			eval("emf", ((add) ? "add" : "del"), "rtport", lan_ifname, ifname);
		}
	}
}

static void start_emf(char *lan_ifname)
{
	int ret = 0;
	char tmp[32] = {0};
#ifdef TCONFIG_BCMARM
	char tmp_path[64] = {0};

	if ((lan_ifname == NULL) || (strcmp(lan_ifname, "") == 0))
		return;

	snprintf(tmp_path, sizeof(tmp_path), "/sys/class/net/%s/bridge/multicast_snooping", lan_ifname);

	/* make it possible to enable bridge multicast_snooping */
	if (nvram_get_int("br_mcast_snooping")) {
		f_write_string(tmp_path, "1", 0, 0);
	}
	else { /* DEFAULT: OFF - it can interfere with EMF */
		f_write_string(tmp_path, "0", 0, 0);
	}

	if (!nvram_get_int("emf_enable"))
		return;
#else /* MIPS */
	if (lan_ifname == NULL || !nvram_get_int("emf_enable") || (strcmp(lan_ifname, "") == 0))
		return;
#endif /* TCONFIG_BCMARM */

	/* Start EMF */
	ret = eval("emf", "start", lan_ifname);

	/* Add the static MFDB entries */
	emf_mfdb_update(lan_ifname, NULL, 1);

	/* Add the UFFP entries */
	emf_uffp_update(lan_ifname, NULL, 1);

	/* Add the RTPORT entries */
	emf_rtport_update(lan_ifname, NULL, 1);

	if (ret) {
		logmsg(LOG_INFO, "starting EMF for %s failed ...", lan_ifname);
	}
	else {
		logmsg(LOG_INFO, "EMF for %s is started", lan_ifname);
		nvram_set(strlcat_r(lan_ifname, "_emf_active", tmp, sizeof(tmp)), "1"); /* set active */
	}
}

static void stop_emf(char *lan_ifname)
{
	int ret = 0;
	char tmp[32] = {0};

	/* check if emf is active for lan_ifname */
	if (lan_ifname == NULL || !nvram_get_int(strlcat_r(lan_ifname, "_emf_active", tmp, sizeof(tmp))))
		return;

	/* Stop EMF for this LAN / brX */
	ret = eval("emf", "stop", lan_ifname);

	/* Remove bridge from igs */
	eval("igs", "del", "bridge", lan_ifname);
	eval("emf", "del", "bridge", lan_ifname);

	if (ret) {
		logmsg(LOG_INFO, "stopping EMF for %s failed ...", lan_ifname);
	}
	else {
		logmsg(LOG_INFO, "EMF for %s is stopped", lan_ifname);
		nvram_set(strlcat_r(lan_ifname, "_emf_active", tmp, sizeof(tmp)), "0"); /* set NOT active */
	}
}
#endif /* TCONFIG_EMF */

/* Set initial QoS mode for all et interfaces that are up. */
void set_et_qos_mode(void)
{
	int i, s, qos;
	struct ifreq ifr;
	struct ethtool_drvinfo info;

	if ((s = socket(AF_INET, SOCK_RAW, IPPROTO_RAW)) < 0)
		return;

	qos = (strcmp(nvram_safe_get("wl_wme"), "off") != 0);

	for (i = 1; i <= DEV_NUMIFS; i++) {
		ifr.ifr_ifindex = i;
		if (ioctl(s, SIOCGIFNAME, &ifr))
			continue;
		if (ioctl(s, SIOCGIFHWADDR, &ifr))
			continue;
		if (ifr.ifr_hwaddr.sa_family != ARPHRD_ETHER)
			continue;
		if (ioctl(s, SIOCGIFFLAGS, &ifr))
			continue;
		if (!(ifr.ifr_flags & IFF_UP))
			continue;

		/* Set QoS for et & bcm57xx devices */
		memset(&info, 0, sizeof(info));
		info.cmd = ETHTOOL_GDRVINFO;
		ifr.ifr_data = (caddr_t)&info;
		if (ioctl(s, SIOCETHTOOL, &ifr) < 0)
			continue;
		if ((strncmp(info.driver, "et", 2) != 0) && (strncmp(info.driver, "bcm57", 5) != 0))
			continue;

		ifr.ifr_data = (caddr_t)&qos;
		ioctl(s, SIOCSETCQOS, &ifr);
	}

	close(s);
}

void unload_wl(void)
{
#ifdef TCONFIG_DHDAP
	modprobe_r("dhd");
#else
	/* do not unload the wifi driver by default, it can cause problems for some router */
	if (nvram_match("wl_unload_enable", "1")) {
		modprobe_r("wl");
	}
#endif /* TCONFIG_DHDAP */
}

#ifdef TCONFIG_BCM714
/* Speed up here and use DEV_NUMIFS_SPEED_UP_714 instead of DEV_NUMIFS */
#define DEV_NUMIFS_SPEED_UP_714 	16

/* This function updates the nvram radio_dmode_X to NIC/DGL depending on driver mode */
static void wl_driver_mode_update(void)
{
	int unit = -1, maxunit = -1;
	int i = 0;
	char ifname[16] = {0};

	/* Search for existing wl devices with eth prefix and the max unit number used */
	for (i = 0; i <= DEV_NUMIFS_SPEED_UP_714 /* DEV_NUMIFS */; i++) {
		snprintf(ifname, sizeof(ifname), "eth%d", i);
		if (!wl_probe(ifname)) {
			int unit = -1;
			char mode_str[128];
			char *mode = "NIC";

#ifdef TCONFIG_DHDAP
			mode = dhd_probe(ifname) ? "NIC" : "DGL";
#endif

			if (!wl_ioctl(ifname, WLC_GET_INSTANCE, &unit, sizeof(unit))) {
				maxunit = (unit > maxunit) ? unit : maxunit + 1;
				sprintf(mode_str, "wlradio_dmode_%d", maxunit);
				if (strcmp(nvram_safe_get(mode_str), mode) != 0) {
					logmsg(LOG_INFO,"%s: Setting %s = %s\n", __FUNCTION__, mode_str, mode);
					nvram_set(mode_str, mode);
				}
			}
		}
	}

	/* Search for existing wl devices with wl prefix and the max unit number used */
	for (i = 0; i <= DEV_NUMIFS_SPEED_UP_714 /* DEV_NUMIFS */; i++) {
		snprintf(ifname, sizeof(ifname), "wl%d", i);
		if (!wl_probe(ifname)) {
			char mode_str[128];
			char *mode = "NIC";

#ifdef TCONFIG_DHDAP
			mode = dhd_probe(ifname) ? "NIC" : "DGL";
#endif

			if (!wl_ioctl(ifname, WLC_GET_INSTANCE, &unit, sizeof(unit))) {
				sprintf(mode_str, "wlradio_dmode_%d", i);
				if (strcmp(nvram_get(mode_str), mode) != 0) {
					logmsg(LOG_INFO,"%s: Setting %s = %s\n", __FUNCTION__, mode_str, mode);
					nvram_set(mode_str, mode);
				}
			}
		}
	}
}

void load_wl()
{
	char module[80], *modules, *next;
#ifdef TCONFIG_DPSTA
	modules = "dpsta dhd dhd24";
#else
	modules = "dhd dhd24";
#endif

	int i = 0, maxunit = -1;
	int unit;
	char ifname[16] = {0};
	char instance_base[128];
	int dhd_reboot = 0;

	if (!*nvram_safe_get("chipnum") || !*nvram_safe_get("chiprev")) {
		dhd_reboot = 1;
	}

	foreach(module, modules, next) {
		if (strcmp(module, "dhd") == 0 && nvram_get_int("dhd24"))
			continue;
		else if (strcmp(module, "dhd24") == 0 && nvram_get_int("dhd24"))
			eval("rmmod", "dhd");

		if (strcmp(module, "dhd") == 0 || strcmp(module, "dhd24") == 0) {
			/* Search for existing wl devices and the max unit number used */
			for (i = 1; i <= DEV_NUMIFS_SPEED_UP_714 /* DEV_NUMIFS */; i++) {
			snprintf(ifname, sizeof(ifname), "eth%d", i);
				if (!wl_probe(ifname)) {
					unit = -1;
					if (!wl_ioctl(ifname, WLC_GET_INSTANCE, &unit, sizeof(unit))) {
						maxunit = (unit > maxunit) ? unit : maxunit;
					}
				}
			}
			snprintf(instance_base, sizeof(instance_base), "instance_base=%d", maxunit + 1);

			if ((strcmp(module, "dhd") == 0) || (strcmp(module, "dhd24") == 0))
				snprintf(instance_base, sizeof(instance_base), "%s dhd_msg_level=%d", instance_base, nvram_get_int("dhd_msg_level"));

			eval("insmod", module, instance_base);
		} else {
			eval("insmod", module);
		}
	}

	wl_driver_mode_update();

	if (dhd_reboot) {
		for(i = 0; i < 5; ++i) {
			if (nvram_match("chipnum","0x4366") && *nvram_safe_get("chiprev")) {
					_dprintf("\nrebooting(dhd)...\n");
					reboot(RB_AUTOBOOT);
			}
			sleep(1);
		}
	}

}
#else
void load_wl(void)
{
#ifdef TCONFIG_DHDAP
	int i = 0, maxunit = -1;
	int unit = -1;
	char ifname[16] = {0};
	char instance_base[128];

	/* Search for existing wl devices and the max unit number used */
	for (i = 1; i <= DEV_NUMIFS; i++) {
		snprintf(ifname, sizeof(ifname), "eth%d", i);
		if (!wl_probe(ifname)) {
			if (!wl_ioctl(ifname, WLC_GET_INSTANCE, &unit, sizeof(unit))) {
				maxunit = (unit > maxunit) ? unit : maxunit;
			}
		}
	}
	snprintf(instance_base, sizeof(instance_base), "instance_base=%d", maxunit + 1);
#ifdef TCONFIG_BCM7
	snprintf(instance_base, sizeof(instance_base), "%s dhd_msg_level=%d", instance_base, nvram_get_int("dhd_msg_level"));
#endif
	eval("insmod", "dhd", instance_base);
#else /* TCONFIG_DHDAP */
	modprobe("wl");
#endif /* TCONFIG_DHDAP */
}
#endif /* TCONFIG_BCM714 */

int disabled_wl(int idx, int unit, int subunit, void *param)
{
	char *ifname;

	ifname = nvram_safe_get(wl_nvname("ifname", unit, subunit));

	/* skip disabled wl vifs */
	if (strncmp(ifname, "wl", 2) == 0 && strchr(ifname, '.') &&
	    !nvram_get_int(wl_nvname("bss_enabled", unit, subunit)))
		return 1;

	return 0;
}

static int set_wlmac(int idx, int unit, int subunit, void *param)
{
	char *ifname;

	ifname = nvram_safe_get(wl_nvname("ifname", unit, subunit));

	/* skip disabled wl vifs */
	if (strncmp(ifname, "wl", 2) == 0 && strchr(ifname, '.') &&
	    !nvram_get_int(wl_nvname("bss_enabled", unit, subunit)))
		return 0;

	if (strcmp(nvram_safe_get(wl_nvname("hwaddr", unit, subunit)), "") == 0) {
		set_mac(ifname, wl_nvname("hwaddr", unit, subunit), 2 + unit + ((subunit > 0) ? ((unit + 1) * 0x10 + subunit) : 0));
	}
	else {
		set_mac(ifname, wl_nvname("hwaddr", unit, subunit), 0);
	}

	return 1;
}

void start_wl(void)
{
	restart_wl();
}

void restart_wl(void)
{
	char *lan_ifname, *lan_ifnames, *ifname, *p;
	int unit, subunit;

	char tmp[32];
	char br;
	char prefix[16] = {0};

#if defined(TCONFIG_BLINK) || defined(TCONFIG_BCMARM) /* RT-N+ */
	int wlan_cnt = 0;
	int wlan_5g_cnt = 0;
	char blink_wlan_ifname[32];
	char blink_wlan_5g_ifname[32];
#ifdef TCONFIG_AC3200
	int wlan_52g_cnt = 0;
	char blink_wlan_52g_ifname[32];
#endif /* TCONFIG_AC3200 */
#endif /* TCONFIG_BLINK || TCONFIG_BCMARM */
#ifdef TCONFIG_BCMARM
	/* get router model */
	int model = get_model();
#endif

#if defined(TCONFIG_EBTABLES) && (defined(TCONFIG_BCMARM) || !defined(TCONFIG_BCMWL6)) /* for all branches, except SDK6 mips (RT-AC) */
	/* check for wireless ethernet bridge mode (wet) and block IPv6 */
	if (foreach_wif(1, NULL, is_wet)) {
		logmsg(LOG_INFO, "No IPv6 support for wireless ethernet bridge mode");
		bridges_block_all_ipv6();
	}
#endif

	for (br = 0; br < BRIDGE_COUNT; br++) {
		char bridge[2] = "0";
		if (br != 0)
			bridge[0] += br;
		else
			strcpy(bridge, "");

		strcpy(tmp, "lan");
		strlcat(tmp, bridge, sizeof(tmp));
		strlcat(tmp, "_ifname", sizeof(tmp));
		lan_ifname = nvram_safe_get(tmp);

		if (strncmp(lan_ifname, "br", 2) == 0) {
			strcpy(tmp, "lan");
			strlcat(tmp, bridge, sizeof(tmp));
			strlcat(tmp, "_ifnames", sizeof(tmp));

			if ((lan_ifnames = strdup(nvram_safe_get(tmp))) != NULL) {
				p = lan_ifnames;
				while ((ifname = strsep(&p, " ")) != NULL) {
					while (*ifname == ' ')
						++ifname;
					if (*ifname == 0)
						continue;

					unit = -1; subunit = -1;

					/* ignore disabled wl vifs */
					if (strncmp(ifname, "wl", 2) == 0 && strchr(ifname, '.')) {
						char nv[40];
						snprintf(nv, sizeof(nv) - 1, "%s_bss_enabled", ifname);
						if (!nvram_get_int(nv))
							continue;
						if (get_ifname_unit(ifname, &unit, &subunit) < 0)
							continue;
					}
					/* get the instance number of the wl i/f */
					else if (wl_ioctl(ifname, WLC_GET_INSTANCE, &unit, sizeof(unit)))
						continue;

					memset(prefix, 0, sizeof(prefix));
					snprintf(prefix, sizeof(prefix), "wl%d_", unit);
					if (nvram_match(strlcat_r(prefix, "radio", tmp, sizeof(tmp)), "0")) {
						eval("wlconf", ifname, "down");
					}
					else {
						eval("wlconf", ifname, "start"); /* start wl iface */
					}

#if defined(TCONFIG_BLINK) || defined(TCONFIG_BCMARM) /* RT-N+ */
					/* Enable WLAN LEDs if wireless interface is enabled */
					if (nvram_get_int(wl_nvname("radio", unit, 0))) {
						if ((wlan_cnt == 0) && (wlan_5g_cnt == 0)
#ifdef TCONFIG_AC3200
						    && (wlan_52g_cnt == 0)
#endif
						) { /* kill all blink at first start up */
							killall("blink", SIGKILL);
							memset(blink_wlan_ifname, 0, sizeof(blink_wlan_ifname)); /* reset */
							memset(blink_wlan_5g_ifname, 0, sizeof(blink_wlan_5g_ifname));
#ifdef TCONFIG_AC3200
							memset(blink_wlan_52g_ifname, 0, sizeof(blink_wlan_52g_ifname));
#endif
						}
						if (unit == 0) {
							led(LED_WLAN, LED_ON); /* enable WLAN LED for 2.4 GHz */
							wlan_cnt++; /* count all wlan units / subunits */
							if (wlan_cnt < 2) strlcpy(blink_wlan_ifname, ifname, sizeof(blink_wlan_ifname));
						}
						else if (unit == 1) {
							led(LED_5G, LED_ON); /* enable WLAN LED for 5 GHz */
							wlan_5g_cnt++; /* count all 5g units / subunits */
							if (wlan_5g_cnt < 2) strlcpy(blink_wlan_5g_ifname, ifname, sizeof(blink_wlan_5g_ifname));
						}
#ifdef TCONFIG_AC3200
						else if (unit == 2) {
							led(LED_52G, LED_ON); /* enable WLAN LED for 2nd 5 GHz */
							wlan_52g_cnt++; /* count all 5g units / subunits */
							if (wlan_52g_cnt < 2) strlcpy(blink_wlan_52g_ifname, ifname, sizeof(blink_wlan_52g_ifname));
						}
#endif /* TCONFIG_AC3200 */
					}
#endif /* TCONFIG_BLINK || TCONFIG_BCMARM */
				}
				free(lan_ifnames);
			}
		}
		else if (strcmp(lan_ifname, "")) {
			/* specific non-bridged lan iface */
			eval("wlconf", lan_ifname, "start");
		}
	}

	killall("wldist", SIGTERM);
	eval("wldist");

#ifdef TCONFIG_BCMARM
	/* do some LED setup */
	if ((model == MODEL_WS880)
	    || (model == MODEL_R6400)
	    || (model == MODEL_R6400v2)
	    || (model == MODEL_R6700v1)
	    || (model == MODEL_R6700v3)
	    || (model == MODEL_R6900)
	    || (model == MODEL_R7000)
	    || (model == MODEL_XR300)
#ifdef TCONFIG_AC3200
	    || (model == MODEL_R8000)
#endif
	) {
		if (nvram_match("wl0_radio", "1") || nvram_match("wl1_radio", "1")
#ifdef TCONFIG_AC3200
		    || nvram_match("wl2_radio", "1")
#endif
		)
			led(LED_AOSS, LED_ON);
		else
			led(LED_AOSS, LED_OFF);
	}
#endif /* TCONFIG_BCMARM */

#if defined(TCONFIG_BLINK) || defined(TCONFIG_BCMARM) /* RT-N+ */
	/* Finally: start blink (traffic "control" of LED) if only one unit (for each wlan) is enabled [AND stealth mode is off - ARM] */
	if (nvram_get_int("blink_wl")
#ifdef TCONFIG_BCMARM
	    && nvram_match("stealth_mode", "0")
#endif
	) {
		if (wlan_cnt == 1)
			eval("blink", blink_wlan_ifname, "wlan", "10", "8192");
		if (wlan_5g_cnt == 1)
			eval("blink", blink_wlan_5g_ifname, "5g", "10", "8192");
#ifdef TCONFIG_AC3200
		if (wlan_52g_cnt == 1)
			eval("blink", blink_wlan_52g_ifname, "52g", "10", "8192");
#endif
	}
#endif /* TCONFIG_BLINK || TCONFIG_BCMARM */
}

void stop_lan_wl(void)
{
	char *p, *ifname;
	char *wl_ifnames;
	char *lan_ifname;
	int unit, subunit;
	char tmp[32];
	char br;

#if defined(TCONFIG_EBTABLES) && (defined(TCONFIG_BCMARM) || !defined(TCONFIG_BCMWL6)) /* for all branches, except SDK6 mips (RT-AC) */
	bridges_flush_all_chains(); /* ebtables clean-up */
#endif

	for (br = 0; br < BRIDGE_COUNT; br++) {
		char bridge[2] = "0";
		if (br !=0 )
			bridge[0] += br;
		else
			strcpy(bridge, "");

		strcpy(tmp, "lan");
		strlcat(tmp, bridge, sizeof(tmp));
		strlcat(tmp, "_ifname", sizeof(tmp));
		lan_ifname = nvram_safe_get(tmp);

		strcpy(tmp, "lan");
		strlcat(tmp, bridge, sizeof(tmp));
		strlcat(tmp, "_ifnames", sizeof(tmp));
		if ((wl_ifnames = strdup(nvram_safe_get(tmp))) != NULL) {
			p = wl_ifnames;
			while ((ifname = strsep(&p, " ")) != NULL) {
				while (*ifname == ' ')
					++ifname;
				if (*ifname == 0)
					continue;

				if (strncmp(ifname, "wl", 2) == 0 && strchr(ifname, '.')) {
					if (get_ifname_unit(ifname, &unit, &subunit) < 0)
						continue;
				}
				else if (wl_ioctl(ifname, WLC_GET_INSTANCE, &unit, sizeof(unit))) {
#ifdef TCONFIG_EMF
					if (nvram_get_int("emf_enable"))
						eval("emf", "del", "iface", lan_ifname, ifname);
#endif
					continue;
				}

				eval("wlconf", ifname, "down");
				ifconfig(ifname, 0, NULL, NULL);
				eval("brctl", "delif", lan_ifname, ifname);
#ifdef TCONFIG_EMF
				if (nvram_get_int("emf_enable"))
					eval("emf", "del", "iface", lan_ifname, ifname);
#endif

			}
			free(wl_ifnames);
		}
#ifdef TCONFIG_EMF
	stop_emf(lan_ifname);
#endif
	}

}

void start_lan_wl(void)
{
	char *lan_ifname;
	char *wl_ifnames, *ifname, *p;
	uint32 ip;
	int unit, subunit, sta;
	char tmp[32];
	char br;
#ifndef TCONFIG_BCM714
#ifdef TCONFIG_DHDAP
	int is_dhd;
#endif
#endif

	foreach_wif(0, NULL, set_wlmac);

	for (br = 0; br < BRIDGE_COUNT; br++) {
		char bridge[2] = "0";
		if (br != 0)
			bridge[0] += br;
		else
			strcpy(bridge, "");

		strcpy(tmp, "lan");
		strlcat(tmp, bridge, sizeof(tmp));
		strlcat(tmp, "_ifname", sizeof(tmp));
		lan_ifname = nvram_safe_get(tmp);

		if (strncmp(lan_ifname, "br", 2) == 0) {
#ifdef TCONFIG_EMF
			if (nvram_get_int("emf_enable")) {
				eval("emf", "add", "bridge", lan_ifname);
				eval("igs", "add", "bridge", lan_ifname);
			}
#endif
			strcpy(tmp, "lan");
			strlcat(tmp, bridge, sizeof(tmp));
			strlcat(tmp, "_ipaddr", sizeof(tmp));
			inet_aton(nvram_safe_get(tmp), (struct in_addr *)&ip);

			strcpy(tmp, "lan");
			strlcat(tmp, bridge, sizeof(tmp));
			strlcat(tmp, "_ifnames", sizeof(tmp));

			sta = 0;

			if ((wl_ifnames = strdup(nvram_safe_get(tmp))) != NULL) {
				p = wl_ifnames;
				while ((ifname = strsep(&p, " ")) != NULL) {
					while (*ifname == ' ')
						++ifname;
					if (*ifname == 0)
						continue;

					unit = -1; subunit = -1;

					/* ignore disabled wl vifs */
					if (strncmp(ifname, "wl", 2) == 0 && strchr(ifname, '.')) {
						char nv[40];
						snprintf(nv, sizeof(nv) - 1, "%s_bss_enabled", ifname);
						if (!nvram_get_int(nv))
							continue;
						if (get_ifname_unit(ifname, &unit, &subunit) < 0)
							continue;

						set_wlmac(0, unit, subunit, NULL);
					}
					else
						wl_ioctl(ifname, WLC_GET_INSTANCE, &unit, sizeof(unit));

					/* bring up interface */
					if (ifconfig(ifname, IFUP | IFF_ALLMULTI, NULL, NULL) != 0)
						continue;

					if (wlconf(ifname, unit, subunit) == 0) {
						const char *mode = nvram_safe_get(wl_nvname("mode", unit, subunit));

						if (strcmp(mode, "wet") == 0) {
							/* Enable host DHCP relay */
							if (nvram_get_int("dhcp_relay")) { /* only set "wet_host_ipv4" (again), "wet_host_mac" already set at start_lan() */
#if (!defined(TCONFIG_BCM7) && defined(TCONFIG_BCMSMP)) || defined(TCONFIG_BCM714) /* only for ARM dual-core SDK6 starting with ~ AiMesh 2.0 support / ~ October 2020 and SDK714 (with newer driver ~ 2020 and up!) */
								if (subunit > 0) { /* only for enabled subunits */
									wet_host_t wh;

									memset(&wh, 0, sizeof(wet_host_t));
									wh.bssidx = subunit;
									memcpy(&wh.buf, &ip, sizeof(ip)); /* struct for ip or mac */

									wl_iovar_set(ifname, "wet_host_ipv4", &wh, sizeof(wet_host_t));
								}
#else /* (!defined(TCONFIG_BCM7) && defined(TCONFIG_BCMSMP)) || defined(TCONFIG_BCM714) */
#ifdef TCONFIG_DHDAP
								is_dhd = !dhd_probe(ifname);
								if (is_dhd)
									dhd_iovar_setint(ifname, "wet_host_ipv4", ip);
								else
#endif /* TCONFIG_DHDAP */
									wl_iovar_setint(ifname, "wet_host_ipv4", ip);
#endif /* (!defined(TCONFIG_BCM7) && defined(TCONFIG_BCMSMP)) || defined(TCONFIG_BCM714) */
							}
						}

						sta |= (strcmp(mode, "sta") == 0);
						if ((strcmp(mode, "ap") != 0) && (strcmp(mode, "wet") != 0)
#ifdef TCONFIG_BCMWL6
						    && (strcmp(mode, "psta") != 0)
#endif
						)
							continue;
					}
					eval("brctl", "addif", lan_ifname, ifname);
#ifdef TCONFIG_EMF
					if (nvram_get_int("emf_enable"))
						eval("emf", "add", "iface", lan_ifname, ifname);
#endif
				}
				free(wl_ifnames);
			}
		}
#ifdef TCONFIG_EMF
		start_emf(lan_ifname);
#endif
	}
}

void stop_wireless(void) {
	char prefix[] = "wanXX";

	stop_nas();
	if (get_sta_wan_prefix(prefix)) { /* wl client will be down */
		logmsg(LOG_INFO, "wireless client WAN: stopping %s (WL down)", prefix);
		stop_wan_if(prefix);
	}
#ifdef TCONFIG_BCMWL6
	wl_sta_stop();
#endif
	stop_lan_wl();
}

void start_wireless(void) {
#ifdef TCONFIG_BCMWL6
	int ret = 0;
#endif
	char prefix[] = "wanXX";

#ifdef TCONFIG_BCMWL6
	if ((ret = wl_sta_prepare()))
		wl_sta_start();
#endif
	start_lan_wl();
	start_nas();
	restart_wl();

	if (1 &&
#ifdef TCONFIG_BCMWL6
	    ret &&
#endif
	    get_sta_wan_prefix(prefix)) { /* wl client up again */
		logmsg(LOG_INFO, "wireless client WAN: starting %s (WL up)", prefix);
		start_wan_if(prefix);
		sleep(5);
		force_to_dial(prefix);
	}

}

void restart_wireless(void) {
	stop_wireless();
	start_wireless();
}

#ifdef TCONFIG_BCMWL6
void wl_sta_start(void)
{
	char *wlc = NULL;
	int unit = 0;
	int r = -1;

	wlc = nvram_safe_get("wlc_ifname");

	if (!strlen(wlc))
		return;

	/* Check wl interface */
	if (wl_probe(wlc))
		return;

	/* get the instance number of the wl */
	if (wl_ioctl(wlc, WLC_GET_INSTANCE, &unit, sizeof(unit)))
		return;

	/* bring up interface */
	if (ifconfig(wlc, IFUP | IFF_ALLMULTI, NULL, NULL) != 0)
		return;

	r = eval("wlconf", wlc, "up"); /* wl iface up! */
	if (r == 0) {
		if (strncmp(wlc, "eth", 3) == 0) { /* check for main radio units */
			eval("wl", "-i", wlc, "antdiv", nvram_safe_get(wl_nvname("antdiv", unit, 0)));
			eval("wl", "-i", wlc, "txant", nvram_safe_get(wl_nvname("txant", unit, 0)));
			eval("wl", "-i", wlc, "txpwr1", "-o", "-m", nvram_get_int(wl_nvname("txpwr", unit, 0)) ? nvram_safe_get(wl_nvname("txpwr", unit, 0)) : "-1");
#ifdef TCONFIG_BCMARM
			eval("wl", "-i", wlc, "interference", nvram_match(wl_nvname("phytype", unit, 0), "v") ? nvram_safe_get(wl_nvname("mitigation_ac", unit, 0)) : nvram_safe_get(wl_nvname("mitigation", unit, 0)));
#else
			eval("wl", "-i", wlc, "interference", nvram_safe_get(wl_nvname("mitigation", unit, 0)));
#endif /* TCONFIG_BCMARM */

			if (unit == 0) {
				led(LED_WLAN, LED_ON); /* enable WLAN LED for 2.4 GHz */
			}
			else if (unit == 1) {
				led(LED_5G, LED_ON); /* enable WLAN LED for 5 GHz */
			}
#ifdef TCONFIG_AC3200
			else if (unit == 2) {
				led(LED_52G, LED_ON); /* enable WLAN LED for 2nd 5 GHz */
			}
#endif /* TCONFIG_AC3200 */
		}

		xstart("radio", "join"); /* try connecting ... */
	}
	else
		return;

	eval("wlconf", wlc, "start"); /* start wl iface */
}

void wl_sta_stop(void)
{
	char *wlc = NULL;
	int unit = 0;

	wlc = nvram_safe_get("wlc_ifname");

	if (!strlen(wlc))
		return;

	/* Check wl interface */
	if (wl_probe(wlc))
		return;

	/* get the instance number of the wl */
	if (wl_ioctl(wlc, WLC_GET_INSTANCE, &unit, sizeof(unit)))
		return;

	eval("wlconf", wlc, "down"); /* stop wl iface */

	ifconfig(wlc, 0, NULL, NULL);

	if (strncmp(wlc, "eth", 3) == 0) { /* check for main radio units */
		if (unit == 0) {
			led(LED_WLAN, LED_OFF); /* disable WLAN LED for 2.4 GHz */
		}
		else if (unit == 1) {
			led(LED_5G, LED_OFF); /* disable WLAN LED for 5 GHz */
		}
#ifdef TCONFIG_AC3200
		else if (unit == 2) {
			led(LED_52G, LED_OFF); /* disable WLAN LED for 2nd 5 GHz */
		}
#endif /* TCONFIG_AC3200 */
	}
}

int wl_sta_prepare(void)
{
	int mwan_num;
	int wan_unit;
	char wan_prefix[] = "wanXX";
	char wl_prefix[8];
	int wl_unit = 0;
	int i;
	char buffer[32];
	char tmp[64];
	char *wl_sta = NULL;
	char *wlc = NULL;
	int sta = 0;
	char word[128], *next;

	/* quick pre-check for non-sta setups (help FT with CTF on!) */
	foreach (word, nvram_safe_get("wl_ifnames"), next) {
		snprintf(wl_prefix, sizeof(wl_prefix), "wl%d_", wl_unit);
		if (nvram_match(strlcat_r(wl_prefix, "mode", tmp, sizeof(tmp)), "sta")) {
			sta = 1; /* found! */
		}
		wl_unit++;
	}

	if (sta) { /* quick pre-check found sta */
		sta = 0; /* reset again - prepare for final check and nvram config (comes next) */
	}
	else { /* no sta setup found (anymore) */
		goto CLEANUP;
	}

	mwan_num = nvram_get_int("mwan_num");
	if (mwan_num < 1 || mwan_num > MWAN_MAX)
		mwan_num = 1;

	for (wan_unit = 1; wan_unit <= mwan_num; ++wan_unit) {
		get_wan_prefix(wan_unit, wan_prefix);

		store_wan_if_to_nvram(wan_prefix); /* prepare wan setup very early now! */

		wl_sta = nvram_safe_get(strlcat_r(wan_prefix, "_ifname", tmp, sizeof(tmp)));

		/* Check interface len */
		if (!strlen(wl_sta))
			continue;

		/* Check for wl interface */
		if (!wl_probe(wl_sta)) {

			/* get the instance number of the wl */
			if (wl_ioctl(wl_sta, WLC_GET_INSTANCE, &wl_unit, sizeof(wl_unit)))
				return 0;

			snprintf(wl_prefix, sizeof(wl_prefix), "wl%d_", wl_unit);

			if (nvram_match(strlcat_r(wl_prefix, "mode", tmp, sizeof(tmp)), "sta") &&
			    nvram_match(strlcat_r(wl_prefix, "radio", tmp, sizeof(tmp)), "1") &&
			    nvram_match(strlcat_r(wl_prefix, "bss_enabled", tmp, sizeof(tmp)), "1")) { /* check for sta interface */
				logmsg(LOG_INFO, "Wireless WAN found: %s - wl%d", wl_sta, wl_unit);
				sta = 1;
				break;
			}
		}
	}

	/* check bridges and remove sta interface from the interface list */
	if (sta) {
		for (i = 0; i < BRIDGE_COUNT; i++) {
			memset(buffer, 0, sizeof(buffer));
			snprintf(buffer, sizeof(buffer), (i == 0 ? "lan_ifname" : "lan%d_ifname"), i);
			if (strcmp(nvram_safe_get(buffer), "") != 0) { /* check brX */
				memset(buffer, 0, sizeof(buffer));
				memset(tmp, 0, sizeof(tmp));
				snprintf(buffer, sizeof(buffer), (i == 0 ? "lan_ifnames" : "lan%d_ifnames"), i);
				snprintf(tmp, sizeof(tmp), "%s", nvram_safe_get(buffer));

				if (!remove_from_list(wl_sta, tmp, sizeof(tmp))) {
					nvram_set(buffer, tmp); /* save lanX_ifnames back to nvram without sta interface */
					break;
				}

			}
		}
	}

CLEANUP:
	/* prepare & clean-up nvram */
	/* case 1: new setup/interface */
	if (sta && nvram_is_empty("wlc_ifname")) {
		nvram_set("wlc_ifname", wl_sta); /* if not yet set, save to nvram */
	}
	/* case 2: setup/interface changed */
	else if (sta && !nvram_match("wlc_ifname", wl_sta)) {
		wlc = nvram_safe_get("wlc_ifname");
		memset(tmp, 0, sizeof(tmp));
		snprintf(tmp, sizeof(tmp), "%s", nvram_safe_get("lan_ifnames"));

		if (!add_to_list(wlc, tmp, sizeof(tmp))) {
			nvram_set("lan_ifnames", tmp);
			nvram_set("wlc_ifname", wl_sta); /* save new client interface to nvram */
		}
	}
	/* case 3: no setup/interface change, restart */
	else if (sta && nvram_match("wlc_ifname", wl_sta)) {
		/* nothing to do so far! */
	}
	/* case 4: no client anymore */
	else if (!sta && strlen(nvram_safe_get("wlc_ifname"))) {
		wlc = nvram_safe_get("wlc_ifname");
		memset(tmp, 0, sizeof(tmp));
		snprintf(tmp, sizeof(tmp), "%s", nvram_safe_get("lan_ifnames"));

		if (!add_to_list(wlc, tmp, sizeof(tmp))) {
			logmsg(LOG_INFO, "Wireless WAN disabled: add wireless interface %s back to br0 (default)", wlc);
			nvram_set("lan_ifnames", tmp);
			nvram_unset("wlc_ifname"); /* remove client interface again! */
		}
	}
	/* case 5: no client */
	else {
		/* nothing to do so far! */
	}

	return sta;
}
#endif /* TCONFIG_BCMWL6 */

#ifdef TCONFIG_IPV6
void enable_ipv6(int enable)
{
	DIR *dir;
	struct dirent *dirent;
	char s[128];

	if ((dir = opendir("/proc/sys/net/ipv6/conf")) != NULL) {
		while ((dirent = readdir(dir)) != NULL) {
			/* Do not enable IPv6 on 'all', 'eth0', 'eth1', 'eth2' (ethX);
			 * IPv6 will live on the bridged instances --> This simplifies the routing table a little bit;
			 * 'default' is enabled so that new interfaces (brX, vlanX, ...) will get IPv6
			 */
			if ((strncmp("eth", dirent->d_name, 3) == 0) || (strncmp("all", dirent->d_name, 3) == 0)) {
				/* do nothing */
			}
			else {
				snprintf(s, sizeof(s), "ipv6/conf/%s/disable_ipv6", dirent->d_name);
				f_write_procsysnet(s, enable ? "0" : "1");
			}
		}
		closedir(dir);
	}
}

void accept_ra(const char *ifname)
{
	char s[128];

	/* possible values for accept_ra:
	   0 Do not accept Router Advertisements
	   1 Accept Router Advertisements if forwarding is disabled
	   2 Overrule forwarding behaviour. Accept Router Advertisements even if forwarding is enabled
	*/
	snprintf(s, sizeof(s), "ipv6/conf/%s/accept_ra", ifname);
	f_write_procsysnet(s, "2");
}

void accept_ra_reset(const char *ifname)
{
	char s[128];

	/* set accept_ra (back) to 1 (default) */
	snprintf(s, sizeof(s), "ipv6/conf/%s/accept_ra", ifname);
	f_write_procsysnet(s, "1");
}

void ipv6_forward(const char *ifname, int enable)
{
	char s[128];

	/* possible values for forwarding:
	   0 Forwarding disabled
	   1 Forwarding enabled
	*/
	snprintf(s, sizeof(s), "ipv6/conf/%s/forwarding", ifname);
	f_write_procsysnet(s, enable ? "1" : "0");
}

void ndp_proxy(const char *ifname, int enable)
{
	char s[128];

	snprintf(s, sizeof(s), "ipv6/conf/%s/proxy_ndp", ifname);
	f_write_procsysnet(s, enable ? "1" : "0");

}
#endif /* TCONFIG_IPV6 */

void start_lan(void)
{
	logmsg(LOG_DEBUG, "*** IN %s: %d", __FUNCTION__, __LINE__);

	char *lan_ifname;
	struct ifreq ifr;
	char *lan_ifnames, *ifname, *p;
	int sfd;
	uint32 ip;
	int unit, subunit, sta;
	int hwaddrset;
	char eabuf[32];
	char tmp[32];
	char tmp2[32];
	char br;
	int vid;
	int vid_map;
	char *iftmp;
	char nv[64];

#ifndef TCONFIG_BCM714
#ifdef TCONFIG_DHDAP
	int is_dhd;
#endif
#endif

#if !defined(TCONFIG_DHDAP) && !defined(TCONFIG_USBAP) /* load driver at init.c for USBAP/sdk7 */
	load_wl(); /* lets go! */
#endif

#ifdef TCONFIG_BCMARM
	wlconf_pre(); /* prepare a few wifi things */
#endif

	foreach_wif(0, NULL, set_wlmac);

#ifdef TCONFIG_BCMWL6
	if (wl_sta_prepare())
		wl_sta_start();
#endif

#ifdef TCONFIG_IPV6
	enable_ipv6(ipv6_enabled());  /* tell Kernel to disable/enable IPv6 for most interfaces */
#endif

#if !defined(TCONFIG_BLINK) && !defined(TCONFIG_BCMARM) /* RT only */
	int vlan0tag = nvram_get_int("vlan0tag");
#endif

	for (br = 0; br <= BRIDGE_COUNT; br++) {
		char bridge[2] = "0";
		if (br != 0)
			bridge[0] += br;
		else
			strcpy(bridge, "");

		if ((sfd = socket(AF_INET, SOCK_RAW, IPPROTO_RAW)) < 0)
			return;

		strcpy(tmp, "lan");
		strlcat(tmp, bridge, sizeof(tmp));
		strlcat(tmp, "_ifname", sizeof(tmp));
		lan_ifname = strdup(nvram_safe_get(tmp));

		if (strncmp(lan_ifname, "br", 2) == 0) {
			logmsg(LOG_DEBUG, "*** %s: setting up the bridge %s", __FUNCTION__, lan_ifname);

			eval("brctl", "addbr", lan_ifname);
			eval("brctl", "setfd", lan_ifname, "0");
			strcpy(tmp, "lan");
			strlcat(tmp, bridge, sizeof(tmp));
			strlcat(tmp, "_stp", sizeof(tmp));
			eval("brctl", "stp", lan_ifname, nvram_safe_get(tmp));

#ifdef TCONFIG_EMF
			if (nvram_get_int("emf_enable")) {
				eval("emf", "add", "bridge", lan_ifname);
				eval("igs", "add", "bridge", lan_ifname);
			}
#endif

			strcpy(tmp, "lan");
			strlcat(tmp, bridge, sizeof(tmp));
			strlcat(tmp, "_ipaddr", sizeof(tmp));
			inet_aton(nvram_safe_get(tmp), (struct in_addr *)&ip);

			hwaddrset = 0;
			sta = 0;

			strcpy(tmp, "lan");
			strlcat(tmp, bridge, sizeof(tmp));
			strlcat(tmp, "_ifnames", sizeof(tmp));
			if ((lan_ifnames = strdup(nvram_safe_get(tmp))) != NULL) {
				p = lan_ifnames;
				while ((iftmp = strsep(&p, " ")) != NULL) {
					while (*iftmp == ' ')
						++iftmp;
					if (*iftmp == 0)
						continue;
					ifname = iftmp;

					unit = -1; subunit = -1;

					/* ignore disabled wl vifs */
					if (strncmp(ifname, "wl", 2) == 0 && strchr(ifname, '.')) {
						snprintf(nv, sizeof(nv) - 1, "%s_bss_enabled", ifname);
						if (!nvram_get_int(nv))
							continue;
						if (get_ifname_unit(ifname, &unit, &subunit) < 0)
							continue;

						set_wlmac(0, unit, subunit, NULL);
					}
					else
						wl_ioctl(ifname, WLC_GET_INSTANCE, &unit, sizeof(unit));

					/* vlan ID mapping */
					if (strncmp(ifname, "vlan", 4) == 0) {
						if (sscanf(ifname, "vlan%d", &vid) == 1) {
							snprintf(tmp, sizeof(tmp), "vlan%dvid", vid);
							vid_map = nvram_get_int(tmp);
#if !defined(TCONFIG_BLINK) && !defined(TCONFIG_BCMARM) /* RT only */
							if ((vid_map < 1) || (vid_map > 4094)) vid_map = vlan0tag | vid;
#else
							if ((vid_map < 1) || (vid_map > 4094)) vid_map = vid;
#endif
							snprintf(tmp, sizeof(tmp), "vlan%d", vid_map);
							ifname = tmp;
						}
					}

					/* bring up interface */
					if (ifconfig(ifname, IFUP | IFF_ALLMULTI, NULL, NULL) != 0)
						continue;

					/* set the logical bridge address to that of the first interface OR Media Bridge interface address! */
					strlcpy(ifr.ifr_name, lan_ifname, IFNAMSIZ);
					if ((!hwaddrset) ||
#ifdef TCONFIG_BCMWL6
					    (is_psta_client(unit, subunit)) ||
#endif
					    (ioctl(sfd, SIOCGIFHWADDR, &ifr) == 0 && memcmp(ifr.ifr_hwaddr.sa_data, "\0\0\0\0\0\0", ETHER_ADDR_LEN) == 0)) {
						strlcpy(ifr.ifr_name, ifname, IFNAMSIZ);
						if (ioctl(sfd, SIOCGIFHWADDR, &ifr) == 0) {
							strlcpy(ifr.ifr_name, lan_ifname, IFNAMSIZ);
							ifr.ifr_hwaddr.sa_family = ARPHRD_ETHER;
#ifdef TCONFIG_DHDAP
							/* FIX me / Workaround for SDK7: mac addr value may be wrong for 2nd vif (after (re-) boot ??)  --> catch that case and adjust mac to our wl configuration */
							if (subunit > 0) {
								unsigned char hwaddr[ETHER_ADDR_LEN];
								if (ether_atoe(nvram_safe_get(wl_nvname("hwaddr", unit, subunit)), hwaddr) &&
								    memcmp(hwaddr, "\0\0\0\0\0\0", ETHER_ADDR_LEN) &&
								    memcmp(hwaddr, ifr.ifr_hwaddr.sa_data, ETHER_ADDR_LEN)) { /* compare and take nvram value if different */
									memcpy(ifr.ifr_hwaddr.sa_data, hwaddr, ETHER_ADDR_LEN);
								}
							}
#endif /* TCONFIG_DHDAP */
							logmsg(LOG_DEBUG, "*** %s: setting MAC of %s bridge to %s", __FUNCTION__, ifr.ifr_name, ether_etoa((const unsigned char *)ifr.ifr_hwaddr.sa_data, eabuf));
							ioctl(sfd, SIOCSIFHWADDR, &ifr);
							hwaddrset = 1;
						}
					}

					if (wlconf(ifname, unit, subunit) == 0) {
						const char *mode = nvram_safe_get(wl_nvname("mode", unit, subunit));

						if (strcmp(mode, "wet") == 0) {
							/* Enable host DHCP relay */
							if (nvram_get_int("dhcp_relay")) { /* set mac and ip */
#if (!defined(TCONFIG_BCM7) && defined(TCONFIG_BCMSMP)) || defined(TCONFIG_BCM714) /* only for ARM dual-core SDK6 starting with ~ AiMesh 2.0 support / ~ October 2020 and SDK714 (with newer driver ~ 2020 and up!) */
								if (subunit > 0) { /* only for enabled subunits */
									wet_host_t wh;

									memset(&wh, 0, sizeof(wet_host_t));
									wh.bssidx = subunit;
									memcpy(&wh.buf, ifr.ifr_hwaddr.sa_data, ETHER_ADDR_LEN); /* struct for ip or mac */

									wl_iovar_set(ifname, "wet_host_mac", &wh, ETHER_ADDR_LEN); /* set mac */

									memset(&wh, 0, sizeof(wet_host_t));
									wh.bssidx = subunit;
									memcpy(&wh.buf, &ip, sizeof(ip)); /* struct for ip or mac */

									wl_iovar_set(ifname, "wet_host_ipv4", &wh, sizeof(wet_host_t)); /* set ip */
								}
#else /* (!defined(TCONFIG_BCM7) && defined(TCONFIG_BCMSMP)) || defined(TCONFIG_BCM714) */
#ifdef TCONFIG_DHDAP
								is_dhd = !dhd_probe(ifname);
								if (is_dhd) {
									char macbuf[sizeof("wet_host_mac") + 1 + ETHER_ADDR_LEN];
									dhd_iovar_setbuf(ifname, "wet_host_mac", ifr.ifr_hwaddr.sa_data, ETHER_ADDR_LEN , macbuf, sizeof(macbuf)); /* set mac */
								}
								else
#endif /* TCONFIG_DHDAP */
									wl_iovar_set(ifname, "wet_host_mac", ifr.ifr_hwaddr.sa_data, ETHER_ADDR_LEN); /* set mac */
#ifdef TCONFIG_DHDAP
								is_dhd = !dhd_probe(ifname);
								if (is_dhd) {
									dhd_iovar_setint(ifname, "wet_host_ipv4", ip); /* set ip */
								}
								else
#endif /* TCONFIG_DHDAP */
									wl_iovar_setint(ifname, "wet_host_ipv4", ip); /* set ip */
#endif /* (!defined(TCONFIG_BCM7) && defined(TCONFIG_BCMSMP)) || defined(TCONFIG_BCM714) */
							}
						}

						sta |= (strcmp(mode, "sta") == 0);
						if ((strcmp(mode, "ap") != 0) && (strcmp(mode, "wet") != 0)
#ifdef TCONFIG_BCMWL6
						    && (strcmp(mode, "psta") != 0)
#endif
						)
							continue;
					}
					eval("brctl", "addif", lan_ifname, ifname);
#ifdef TCONFIG_EMF
					if (nvram_get_int("emf_enable"))
						eval("emf", "add", "iface", lan_ifname, ifname);
#endif
				}
				free(lan_ifnames);
			}
		}
		/* --- this shouldn't happen --- */
		else if (*lan_ifname) {
			ifconfig(lan_ifname, IFUP, NULL, NULL);
			wlconf(lan_ifname, -1, -1);
		}
		else {
			close(sfd);
			free(lan_ifname);
#ifdef TCONFIG_IPV6
			start_ipv6(); /* all work done at function start_lan(), lets call start_ipv6() finally (only once!) */
#endif
			return;
		}

		/* Get current LAN hardware address */
		strlcpy(ifr.ifr_name, lan_ifname, IFNAMSIZ);
		strcpy(tmp, "lan");
		strlcat(tmp, bridge, sizeof(tmp));
		strlcat(tmp, "_hwaddr", sizeof(tmp));
		if (ioctl(sfd, SIOCGIFHWADDR, &ifr) == 0) {
			nvram_set(tmp, ether_etoa((const unsigned char *)ifr.ifr_hwaddr.sa_data, eabuf));
		}

		close(sfd); /* close file descriptor */

		/* Set initial QoS mode for LAN ports */
		set_et_qos_mode();

		/* bring up and configure LAN interface */
		strcpy(tmp, "lan");
		strlcat(tmp, bridge, sizeof(tmp));
		strlcat(tmp, "_ipaddr", sizeof(tmp));
		strcpy(tmp2, "lan");
		strlcat(tmp2, bridge, sizeof(tmp2));
		strlcat(tmp2, "_netmask", sizeof(tmp2));
		ifconfig(lan_ifname, IFUP | IFF_ALLMULTI, nvram_safe_get(tmp), nvram_safe_get(tmp2));

		config_loopback();
		do_static_routes(1);

		if (br == 0)
			set_lan_hostname(nvram_safe_get("wan_hostname"));

		if ((get_wan_proto() == WP_DISABLED) && (br == 0)) {
			char *gateway = nvram_safe_get("lan_gateway") ;
			if ((*gateway) && (strcmp(gateway, "0.0.0.0") != 0)) {
				int tries = 5;
				while ((route_add(lan_ifname, 0, "0.0.0.0", gateway, "0.0.0.0") != 0) && (tries-- > 0))
					sleep(1);

				logmsg(LOG_DEBUG, "*** %s: add gateway=%s tries=%d", __FUNCTION__, gateway, tries);
			}
		}

#ifdef TCONFIG_EMF
		start_emf(lan_ifname);
#endif

		free(lan_ifname);

	} /* for-loop brX */

	logmsg(LOG_DEBUG, "*** OUT %s: %d", __FUNCTION__, __LINE__);
}

void stop_lan(void)
{
	logmsg(LOG_DEBUG, "*** IN %s: %d", __FUNCTION__, __LINE__);

	char *lan_ifname;
	char *lan_ifnames, *p, *ifname;
	char tmp[32];
	char br;
	int vid, vid_map;
	char *iftmp;
#if !defined(TCONFIG_BLINK) && !defined(TCONFIG_BCMARM) /* RT only */
	int vlan0tag = nvram_get_int("vlan0tag");
#endif

#if defined(TCONFIG_EBTABLES) && (defined(TCONFIG_BCMARM) || !defined(TCONFIG_BCMWL6)) /* for all branches, except SDK6 mips (RT-AC) */
	bridges_flush_all_chains(); /* ebtables clean-up */
#endif

	ifconfig("lo", 0, NULL, NULL); /* Bring down loopback interface */

#ifdef TCONFIG_BCMWL6
	wl_sta_stop();
#endif

#ifdef TCONFIG_IPV6
	stop_ipv6(); /* stop IPv6 first! */
#endif

	for (br = 0; br < BRIDGE_COUNT; br++) {
		char bridge[2] = "0";
		if (br != 0)
			bridge[0] += br;
		else
			strcpy(bridge, "");

		strcpy(tmp, "lan");
		strlcat(tmp, bridge, sizeof(tmp));
		strlcat(tmp, "_ifname", sizeof(tmp));
		lan_ifname = nvram_safe_get(tmp);
		ifconfig(lan_ifname, 0, NULL, NULL);

		if (strncmp(lan_ifname, "br", 2) == 0) {
			strcpy(tmp, "lan");
			strlcat(tmp, bridge, sizeof(tmp));
			strlcat(tmp, "_ifnames", sizeof(tmp));
			if ((lan_ifnames = strdup(nvram_safe_get(tmp))) != NULL) {
				p = lan_ifnames;
				while ((iftmp = strsep(&p, " ")) != NULL) {
					while (*iftmp == ' ')
						++iftmp;
					if (*iftmp == 0)
						continue;

					ifname = iftmp;
					/* vlan ID mapping */
					if (strncmp(ifname, "vlan", 4) == 0) {
						if (sscanf(ifname, "vlan%d", &vid) == 1) {
							snprintf(tmp, sizeof(tmp), "vlan%dvid", vid);
							vid_map = nvram_get_int(tmp);
#if !defined(TCONFIG_BLINK) && !defined(TCONFIG_BCMARM) /* RT only */
							if ((vid_map < 1) || (vid_map > 4094)) vid_map = vlan0tag | vid;
#else
							if ((vid_map < 1) || (vid_map > 4094)) vid_map = vid;
#endif
							snprintf(tmp, sizeof(tmp), "vlan%d", vid_map);
							ifname = tmp;
						}
					}
					eval("wlconf", ifname, "down");
					ifconfig(ifname, 0, NULL, NULL);
					eval("brctl", "delif", lan_ifname, ifname);
#ifdef TCONFIG_EMF
					if (nvram_get_int("emf_enable"))
						eval("emf", "del", "iface", lan_ifname, ifname);
#endif
				}
				free(lan_ifnames);
			}
#ifdef TCONFIG_EMF
			stop_emf(lan_ifname);
#endif
			eval("brctl", "delbr", lan_ifname);
		}
		else if (*lan_ifname) {
			eval("wlconf", lan_ifname, "down");
		}
	}
#if !defined(TCONFIG_DHDAP) && !defined(TCONFIG_USBAP) /* do not unload driver for USBAP/sdk7 */
	unload_wl(); /* stop! */
#endif

	logmsg(LOG_DEBUG, "*** OUT %s: %d", __FUNCTION__, __LINE__);
}

static int is_sta(int idx, int unit, int subunit, void *param)
{
	return (nvram_match(wl_nvname("mode", unit, subunit), "sta") && (nvram_match(wl_nvname("bss_enabled", unit, subunit), "1")));
}

void do_static_routes(int add)
{
	char *buf;
	char *p, *q;
	char *dest, *mask, *gateway, *metric, *ifname;
	int r;

	if ((buf = strdup(nvram_safe_get(add ? "routes_static" : "routes_static_saved"))) == NULL)
		return;

	if (add)
		nvram_set("routes_static_saved", buf);
	else
		nvram_unset("routes_static_saved");

	p = buf;
	while ((q = strsep(&p, ">")) != NULL) {
		if (vstrsep(q, "<", &dest, &gateway, &mask, &metric, &ifname) < 5)
			continue;

		ifname = nvram_safe_get(((strcmp(ifname, "LAN") == 0) ? "lan_ifname" :
					((strcmp(ifname, "LAN1") == 0) ? "lan1_ifname" :
					((strcmp(ifname, "LAN2") == 0) ? "lan2_ifname" :
					((strcmp(ifname, "LAN3") == 0) ? "lan3_ifname" :
					((strcmp(ifname, "WAN2") == 0) ? "wan2_iface" :
					((strcmp(ifname, "WAN3") == 0) ? "wan3_iface" :
					((strcmp(ifname, "WAN4") == 0) ? "wan4_iface" :
					((strcmp(ifname, "MAN2") == 0) ? "wan2_ifname" :
					((strcmp(ifname, "MAN3") == 0) ? "wan3_ifname" :
					((strcmp(ifname, "MAN4") == 0) ? "wan4_ifname" :
					((strcmp(ifname, "WAN") == 0) ? "wan_iface" : "wan_ifname"))))))))))));
		logmsg(LOG_WARNING, "Static route %s: ifname=%s, metric=%s, dest=%s, gateway=%s, mask=%s", (add ? "added" : "deleted"), ifname, metric, dest, gateway, mask);

		if (add) {
			for (r = 3; r >= 0; --r) {
				if (route_add(ifname, atoi(metric), dest, gateway, mask) == 0)
					break;

				sleep(1);
			}
		}
		else {
			route_del(ifname, atoi(metric), dest, gateway, mask);
		}
	}
	free(buf);

	char *wan_modem_ipaddr;
	if ((nvram_match("wan_proto", "pppoe") || nvram_match("wan_proto", "dhcp") || nvram_match("wan_proto", "static"))
	    && (wan_modem_ipaddr = nvram_safe_get("wan_modem_ipaddr")) && *wan_modem_ipaddr && !nvram_match("wan_modem_ipaddr","0.0.0.0") 
	    && (!foreach_wif(1, NULL, is_sta))) {
		char ip[16];
		char *end = rindex(wan_modem_ipaddr,'.') + 1;
		unsigned char c = atoi(end);
		char *iface = nvram_safe_get("wan_ifname");

		snprintf(ip, sizeof(ip), "%.*s%hhu", end-wan_modem_ipaddr, wan_modem_ipaddr, (unsigned char)(c^1^((c&2)^((c&1)<<1))));
		eval("ip", "addr", add ?"add" : "del", ip, "peer", wan_modem_ipaddr, "dev", iface);
	}

	char *wan2_modem_ipaddr;
	if ((nvram_match("wan2_proto", "pppoe") || nvram_match("wan2_proto", "dhcp") || nvram_match("wan2_proto", "static"))
	    && (wan2_modem_ipaddr = nvram_safe_get("wan2_modem_ipaddr")) && *wan2_modem_ipaddr && !nvram_match("wan2_modem_ipaddr","0.0.0.0") 
	    && (!foreach_wif(1, NULL, is_sta))) {
		char ip[16];
		char *end = rindex(wan2_modem_ipaddr,'.') + 1;
		unsigned char c = atoi(end);
		char *iface = nvram_safe_get("wan2_ifname");

		snprintf(ip, sizeof(ip), "%.*s%hhu", end-wan2_modem_ipaddr, wan2_modem_ipaddr, (unsigned char)(c^1^((c&2)^((c&1)<<1))) );
		eval("ip", "addr", add ?"add" : "del", ip, "peer", wan2_modem_ipaddr, "dev", iface);
	}

#ifdef TCONFIG_MULTIWAN
	char *wan3_modem_ipaddr;
	if ((nvram_match("wan3_proto", "pppoe") || nvram_match("wan3_proto", "dhcp") || nvram_match("wan3_proto", "static"))
	    && (wan3_modem_ipaddr = nvram_safe_get("wan3_modem_ipaddr")) && *wan3_modem_ipaddr && !nvram_match("wan3_modem_ipaddr","0.0.0.0") 
	    && (!foreach_wif(1, NULL, is_sta))) {
		char ip[16];
		char *end = rindex(wan3_modem_ipaddr,'.') + 1;
		unsigned char c = atoi(end);
		char *iface = nvram_safe_get("wan3_ifname");

		snprintf(ip, sizeof(ip), "%.*s%hhu", end-wan3_modem_ipaddr, wan3_modem_ipaddr, (unsigned char)(c^1^((c&2)^((c&1)<<1))) );
		eval("ip", "addr", add ?"add" : "del", ip, "peer", wan3_modem_ipaddr, "dev", iface);
	}

	char *wan4_modem_ipaddr;
	if ((nvram_match("wan4_proto", "pppoe") || nvram_match("wan4_proto", "dhcp") || nvram_match("wan4_proto", "static"))
	    && (wan4_modem_ipaddr = nvram_safe_get("wan4_modem_ipaddr")) && *wan4_modem_ipaddr && !nvram_match("wan4_modem_ipaddr","0.0.0.0") 
	    && (!foreach_wif(1, NULL, is_sta))) {
		char ip[16];
		char *end = rindex(wan4_modem_ipaddr,'.') + 1;
		unsigned char c = atoi(end);
		char *iface = nvram_safe_get("wan4_ifname");

		snprintf(ip, sizeof(ip), "%.*s%hhu", end-wan4_modem_ipaddr, wan4_modem_ipaddr, (unsigned char)(c^1^((c&2)^((c&1)<<1))) );
		eval("ip", "addr", add ?"add" : "del", ip, "peer", wan4_modem_ipaddr, "dev", iface);
	}
#endif
}

void hotplug_net(void)
{
	char *interface, *action;
	char *lan_ifname = nvram_safe_get("lan_ifname");
#ifdef TCONFIG_BCMWL6
	int psta;
#endif

	if (((interface = getenv("INTERFACE")) == NULL) || ((action = getenv("ACTION")) == NULL))
		return;

	logmsg(LOG_DEBUG, "*** %s: INTERFACE=%s ACTION=%s", __FUNCTION__, interface, action);

#ifdef TCONFIG_BCMWL6
	psta = wl_wlif_is_psta(interface);
#endif

	if (((strncmp(interface, "wds", 3) == 0)
#ifdef TCONFIG_BCMWL6
	     || psta
#endif
	     ) &&
	    (strcmp(action, "register") == 0 || strcmp(action, "add") == 0)) {

		/* interface up! */
		ifconfig(interface, IFUP, NULL, NULL);

#ifdef TCONFIG_EMF
		if (nvram_get_int("emf_enable")) {
			eval("emf", "add", "iface", lan_ifname, interface);
			emf_mfdb_update(lan_ifname, interface, 1);
			emf_uffp_update(lan_ifname, interface, 1);
			emf_rtport_update(lan_ifname, interface, 1);
		}
#endif
#ifdef TCONFIG_BCMWL6
		/* Indicate interface create event to eapd */
		if (psta) {
			logmsg(LOG_DEBUG, "*** %s: send dif event to %s", __FUNCTION__, interface);
			wl_send_dif_event(interface, 0);
			return;
		}
#endif
		if (strncmp(lan_ifname, "br", 2) == 0) {
			eval("brctl", "addif", lan_ifname, interface);
			notify_nas(interface);
		}

		return;
	}

#ifdef TCONFIG_BCMWL6
	if (strcmp(action, "unregister") == 0 || strcmp(action, "remove") == 0) {
		/* Indicate interface delete event to eapd */
		logmsg(LOG_DEBUG, "*** %s: send dif event (delete) to %s", __FUNCTION__, interface);
		wl_send_dif_event(interface, 1);

#ifdef TCONFIG_EMF
		if (nvram_get_int("emf_enable"))
			eval("emf", "del", "iface", lan_ifname, interface);
#endif /* TCONFIG_EMF */
	}
#endif
}

#ifdef TCONFIG_BCMWL6
int wl_send_dif_event(const char *ifname, uint32 event)
{
	static int s = -1;
	int len, n;
	struct sockaddr_in to;
	char data[IFNAMSIZ + sizeof(uint32)];

	if (ifname == NULL)
		return -1;

	/* create a socket to receive dynamic i/f events */
	if (s < 0) {
		s = socket(AF_INET, SOCK_DGRAM, 0);
		if (s < 0) {
			logerr(__FUNCTION__, __LINE__, "socket");
			return -1;
		}
	}

	/* Init the message contents to send to eapd. Specify the interface
	 * and the event that occured on the interface.
	 */
	strncpy(data, ifname, IFNAMSIZ);
	*(uint32 *)(data + IFNAMSIZ) = event;
	len = IFNAMSIZ + sizeof(uint32);

	/* send to eapd */
	to.sin_addr.s_addr = inet_addr(EAPD_WKSP_UDP_ADDR);
	to.sin_family = AF_INET;
	to.sin_port = htons(EAPD_WKSP_DIF_UDP_PORT);

	n = sendto(s, data, len, 0, (struct sockaddr *)&to,
		sizeof(struct sockaddr_in));

	if (n != len) {
		logerr(__FUNCTION__, __LINE__, "udp");
		return -1;
	}

	logmsg(LOG_DEBUG, "*** %s: sent event %d", __FUNCTION__, event);

	return n;
}
#endif /* TCONFIG_BCMWL6 */

static int is_same_addr(struct ether_addr *addr1, struct ether_addr *addr2)
{
	unsigned int i;

	for (i = 0; i < 6; i++) {
		if (addr1->octet[i] != addr2->octet[i])
			return 0;
	}

	return 1;
}

static int check_wl_client(char *ifname, int unit, int subunit)
{
	struct ether_addr bssid;
#ifdef TCONFIG_BCMWL6
	char macaddr[32];
#endif /* TCONFIG_BCMWL6 */
	wl_bss_info_t *bi;
	char buf[WLC_IOCTL_MAXLEN];
	struct maclist *mlist;
	unsigned int i;
	int mlsize;
	int associated, authorized;

	*(uint32 *)buf = WLC_IOCTL_MAXLEN;
	if (wl_ioctl(ifname, WLC_GET_BSSID, &bssid, ETHER_ADDR_LEN) < 0 ||
	    wl_ioctl(ifname, WLC_GET_BSS_INFO, buf, WLC_IOCTL_MAXLEN) < 0)
		return 0;

	bi = (wl_bss_info_t *)(buf + 4);
	if ((bi->SSID_len == 0) ||
	    (bi->BSSID.octet[0] + bi->BSSID.octet[1] + bi->BSSID.octet[2] +
	     bi->BSSID.octet[3] + bi->BSSID.octet[4] + bi->BSSID.octet[5] == 0))
		return 0;

	associated = 0;
	authorized = strstr(nvram_safe_get(wl_nvname("akm", unit, subunit)), "psk") == 0;

	mlsize = sizeof(struct maclist) + (WL_MAX_ASSOC * sizeof(struct ether_addr));
	if ((mlist = malloc(mlsize)) != NULL) {
		mlist->count = WL_MAX_ASSOC;
		if (wl_ioctl(ifname, WLC_GET_ASSOCLIST, mlist, mlsize) == 0) {
			for (i = 0; i < mlist->count; ++i) {
				if (is_same_addr(&mlist->ea[i], &bi->BSSID)) {
					associated = 1;
					break;
				}
			}
		}

		if (associated && !authorized) {
			memset(mlist, 0, mlsize);
			mlist->count = WL_MAX_ASSOC;
			strlcpy((char*)mlist, "autho_sta_list", mlsize);
			if (wl_ioctl(ifname, WLC_GET_VAR, mlist, mlsize) == 0) {
				for (i = 0; i < mlist->count; ++i) {
					if (is_same_addr(&mlist->ea[i], &bi->BSSID)) {
						authorized = 1;
						break;
					}
				}
			}
		}
		free(mlist);
	}

#ifdef TCONFIG_BCMWL6
	if (associated && authorized && !strcmp(nvram_safe_get(wl_nvname("mode", unit, subunit)), "psta")) {
		ether_etoa((const unsigned char *) &bssid, macaddr);
		logmsg(LOG_DEBUG, "*** %s: %s send keepalive nulldata to %s", __FUNCTION__, ifname, macaddr);
		eval("wl", "-i", ifname, "send_nulldata", macaddr);
	}
#endif /* TCONFIG_BCMWL6 */

	return (associated && authorized);
}

static int radio_join(int idx, int unit, int subunit, void *param)
{
	int i, stacheck_connect, stacheck;
	char s[32], f[64];
	char ifname[16];
	char *amode, sec[16];

	int *unit_filter = param;
	if (*unit_filter >= 0 && *unit_filter != unit)
		return 0;

	if (!nvram_get_int(wl_nvname("radio", unit, 0)) || !wl_client(unit, subunit))
		return 0;

	snprintf(ifname, sizeof(ifname), "%s", nvram_safe_get(wl_nvname("ifname", unit, subunit)));

	/* skip disabled wl vifs */
	if (strncmp(ifname, "wl", 2) == 0 && strchr(ifname, '.') && !nvram_get_int(wl_nvname("bss_enabled", unit, subunit)))
		return 0;

	snprintf(f, sizeof(f), "/var/run/radio.%d.%d.pid", unit, subunit < 0 ? 0 : subunit);
	if (f_read_string(f, s, sizeof(s)) > 0) {
		if ((i = atoi(s)) > 1) {
			kill(i, SIGTERM);
			sleep(1);
		}
	}

	if (fork() == 0) {
		snprintf(s, sizeof(s), "%d", getpid());
		f_write(f, s, sizeof(s), 0, 0644);

		stacheck_connect = nvram_get_int("sta_chkint");
		if (stacheck_connect <= 0)
			stacheck_connect = STACHECK_CONNECT;

		while (get_radio(unit) && wl_client(unit, subunit)) {

			if (check_wl_client(ifname, unit, subunit)) {
				stacheck = stacheck_connect;
			}
			else {
				eval("wl", "-i", ifname, "disassoc");

				if (!strlen(nvram_safe_get(wl_nvname("ssid", unit, subunit))))
					break;

				snprintf(sec, sizeof(sec), "%s", nvram_safe_get(wl_nvname("akm", unit, subunit)));

				if (strstr(sec, "psk2"))
					amode = "wpa2psk";
				else if (strstr(sec, "psk"))
					amode = "wpapsk";
				else if (strstr(sec, "wpa2"))
					amode = "wpa2";
				else if (strstr(sec, "wpa"))
					amode = "wpa";
				else if (nvram_get_int(wl_nvname("auth", unit, subunit)))
					amode = "shared";
				else
					amode = "open";

				eval("wl", "-i", ifname, "join", nvram_safe_get(wl_nvname("ssid", unit, subunit)), "imode", "bss", "amode", amode);

				stacheck = STACHECK_DISCONNECT;
			}
			sleep(stacheck);
		}
		unlink(f);
	}

	return 1;
}

static int radio_toggle(int idx, int unit, int subunit, void *param)
{
	if (!nvram_get_int(wl_nvname("radio", unit, 0)))
		return 0;

	int *op = param;

	if (*op == RADIO_TOGGLE)
		*op = get_radio(unit) ? RADIO_OFF : RADIO_ON;

	set_radio(*op, unit);

	return *op;
}

int radio_main(int argc, char *argv[])
{
	int op = RADIO_OFF;
	int unit;

	if (argc < 2) {
HELP:
		usage_exit(argv[0], "on|off|toggle|join [N]\n");
	}
	unit = (argc == 3) ? atoi(argv[2]) : -1;

	if (strcmp(argv[1], "toggle") == 0)
		op = RADIO_TOGGLE;
	else if (strcmp(argv[1], "off") == 0)
		op = RADIO_OFF;
	else if (strcmp(argv[1], "on") == 0)
		op = RADIO_ON;
	else if (strcmp(argv[1], "join") == 0)
		goto JOIN;
	else
		goto HELP;

	if (unit >= 0)
		op = radio_toggle(0, unit, 0, &op);
	else
		op = foreach_wif(0, &op, radio_toggle);

	if (!op) {
		led(LED_DIAG, LED_OFF);
		return 0;
	}
JOIN:
	foreach_wif(1, &unit, radio_join);

	return 0;
}

static int get_wldist(int idx, int unit, int subunit, void *param)
{
	int n;

	char *p = nvram_safe_get(wl_nvname("distance", unit, 0));
	if ((*p == 0) || ((n = atoi(p)) < 0))
		return 0;

	return (9 + (n / 150) + ((n % 150) ? 1 : 0));
}

static int wldist(int idx, int unit, int subunit, void *param)
{
	rw_reg_t r;
	uint32 s;
	char *p;
	int n;

	n = get_wldist(idx, unit, subunit, param);
	if (n > 0) {
		s = 0x10 | (n << 16);
		p = nvram_safe_get(wl_nvname("ifname", unit, 0));
		wl_ioctl(p, 197, &s, sizeof(s));

		r.byteoff = 0x684;
		r.val = n + 510;
		r.size = 2;
		wl_ioctl(p, 102, &r, sizeof(r));
	}

	return 0;
}

/* ref: wificonf.c */
int wldist_main(int argc, char *argv[])
{
	if (fork() == 0) {
		if (foreach_wif(0, NULL, get_wldist) == 0)
			return 0;

		while (1) {
			foreach_wif(0, NULL, wldist);
			sleep(2);
		}
	}

	return 0;
}
