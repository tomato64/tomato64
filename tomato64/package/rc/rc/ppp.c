/*
 *
 * Copyright 2003, CyberTAN  Inc.  All Rights Reserved
 *
 * This is UNPUBLISHED PROPRIETARY SOURCE CODE of CyberTAN Inc.
 * the contents of this file may not be disclosed to third parties,
 * copied or duplicated in any form without the prior written
 * permission of CyberTAN Inc.
 *
 * This software should be used as a reference only, and it not
 * intended for production use!
 *
 *
 * THIS SOFTWARE IS OFFERED "AS IS", AND CYBERTAN GRANTS NO WARRANTIES OF ANY
 * KIND, EXPRESS OR IMPLIED, BY STATUTE, COMMUNICATION OR OTHERWISE.  CYBERTAN
 * SPECIFICALLY DISCLAIMS ANY IMPLIED WARRANTIES OF MERCHANTABILITY, FITNESS
 * FOR A SPECIFIC PURPOSE OR NONINFRINGEMENT CONCERNING THIS SOFTWARE
 */
/*
 *
 * Copyright 2005, Broadcom Corporation
 * All Rights Reserved.
 *
 * THIS SOFTWARE IS OFFERED "AS IS", AND BROADCOM GRANTS NO WARRANTIES OF ANY
 * KIND, EXPRESS OR IMPLIED, BY STATUTE, COMMUNICATION OR OTHERWISE. BROADCOM
 * SPECIFICALLY DISCLAIMS ANY IMPLIED WARRANTIES OF MERCHANTABILITY, FITNESS
 * FOR A SPECIFIC PURPOSE OR NONINFRINGEMENT CONCERNING THIS SOFTWARE.
 *
 */
/*
 * $Id: ppp.c,v 1.27 2005/03/29 02:00:06 honor Exp $
 */
/*
 *
 * Fixes/updates (C) 2018 - 2026 pedro
 * https://freshtomato.org/
 *
 */


#include "rc.h"

#include <sys/ioctl.h>

/* needed by logmsg() */
#define LOGMSG_DISABLE	DISABLE_SYSLOG_OS
#define LOGMSG_NVDEBUG	"ppp_debug"


/*
 * Called when ipv4 link comes up
 */
int ipup_main(int argc, char **argv)
{
	char *wan_ifname;
	int proto, old_ck_pause = 0;
	int check_mwandog = nvram_get_int("mwan_cktime");
	char prefix[] = "wanXX";
	char tmp[100], tmp2[32];
	char ppplink_file[32];
	char buf[256];
	char *value;
	const char *p;

	if (!wait_action_idle(10))
		return -1;

	logmsg(LOG_DEBUG, "*** IN %s: IFNAME=%s DEVICE=%s LINKNAME=%s IPREMOTE=%s IPLOCAL=%s DNS1=%s DNS2=%s", __FUNCTION__, safe_getenv("IFNAME"), safe_getenv("DEVICE"), safe_getenv("LINKNAME"), safe_getenv("IPREMOTE"), safe_getenv("IPLOCAL"), safe_getenv("DNS1"), safe_getenv("DNS2"));

	wan_ifname = safe_getenv("IFNAME");
	strlcpy(prefix, safe_getenv("LINKNAME"), sizeof(prefix));
	logmsg(LOG_DEBUG, "*** %s: wan_ifname = %s, prefix = %s.", __FUNCTION__, wan_ifname, prefix);

	if ((!wan_ifname) || (!*wan_ifname))
		return -1;

	/* check mwwatchdog enabled - part 1 of 2 */
	if (check_mwandog) {
		snprintf(tmp2, sizeof(tmp2), "%s_ck_pause", prefix);
		old_ck_pause = nvram_get_int(tmp2); /* save old value */
		nvram_set(tmp2, "1"); /* skip checking on this WAN until start_wan_done() finished! */
		logmsg(LOG_DEBUG, "*** %s: set %s_ck_pause=1 to skip checking on this WAN (multiwan watchdog)", __FUNCTION__, prefix);
	}

	/* Note: User can set multi wan init state file with value "0" or "1" (default value)
	 * see function mwan_state_files(void)
	 * use nvram set mwan_state_init=0
	 * to set state file with value "0" instead of "1"
	 */
	
	prefix_nvram_set(prefix, "iface", wan_ifname, tmp, sizeof(tmp)); /* ppp# */
	prefix_nvram_set(prefix, "pppd_pid", safe_getenv("PPPD_PID"), tmp, sizeof(tmp));

	/* ipup receives six arguments:
	 *   <interface name>  <tty device>  <speed> <local IP address> <remote IP address> <ipparam>
	 *   ppp1              vlan1         0       71.135.98.32       151.164.184.87      0
	 */
	snprintf(ppplink_file, sizeof(ppplink_file), "/tmp/ppp/%s_link", prefix);
	f_write_string(ppplink_file, argv[1], 0, 0);

	if ((p = getenv("IPREMOTE"))) {
		prefix_nvram_set(prefix, "gateway_get", p, tmp, sizeof(tmp));
		logmsg(LOG_DEBUG, "*** %s: set %s_gateway_get=%s", __FUNCTION__, prefix, p);
	}

	if ((value = getenv("IPLOCAL"))) {
		proto = get_wanx_proto(prefix);

		switch (proto) { /* store last ip address for Web UI */
			case WP_PPPOE:
			case WP_PPP3G:
				if ((proto == WP_PPPOE) && using_dhcpc(prefix)) /* PPPoE with DHCP MAN */
					prefix_nvram_set(prefix, "ipaddr_buf", prefix_nvram_get(prefix, "ppp_get_ip", tmp2, sizeof(tmp2)), tmp, sizeof(tmp));
				else { /* PPPoE / 3G */
					prefix_nvram_set(prefix, "ipaddr_buf", prefix_nvram_get(prefix, "ipaddr", tmp2, sizeof(tmp2)), tmp, sizeof(tmp));
					prefix_nvram_set(prefix, "ipaddr", value, tmp, sizeof(tmp));
				}
				break;
			case WP_PPTP:
			case WP_L2TP:
				prefix_nvram_set(prefix, "ipaddr_buf", prefix_nvram_get(prefix, "ppp_get_ip", tmp2, sizeof(tmp2)), tmp, sizeof(tmp));
				break;
		}

		/* set netmask in nvram only if not already set (MAN) */
		if (prefix_nvram_match(prefix, "netmask", "0.0.0.0", tmp, sizeof(tmp)))
			prefix_nvram_set(prefix, "netmask", "255.255.255.255", tmp, sizeof(tmp));

		if (!prefix_nvram_match(prefix, "ppp_get_ip", value, tmp, sizeof(tmp))) {
			ifconfig(wan_ifname, IFUP, "0.0.0.0", NULL);
			prefix_nvram_set(prefix, "ppp_get_ip", value, tmp, sizeof(tmp));
		}

		_ifconfig(wan_ifname, IFUP, value, "255.255.255.255", (p && (*p)) ? p : NULL, 0);
	}

	buf[0] = 0;
	if ((p = getenv("DNS1")) != NULL)
		strlcpy(buf, p, sizeof(buf));
	if ((p = getenv("DNS2")) != NULL) {
		if (buf[0])
			strlcat(buf, " ", sizeof(buf));
		strlcat(buf, p, sizeof(buf));
	}
	prefix_nvram_set(prefix, "get_dns", buf, tmp, sizeof(tmp));

	if ((value = getenv("AC_NAME")))
		prefix_nvram_set(prefix, "ppp_get_ac", value, tmp, sizeof(tmp));
	if ((value = getenv("SRV_NAME")))
		prefix_nvram_set(prefix, "ppp_get_srv", value, tmp, sizeof(tmp));
	if ((value = getenv("MTU")))
		prefix_nvram_set(prefix, "run_mtu", value, tmp, sizeof(tmp));

	logmsg(LOG_DEBUG, "*** OUT %s: to start_wan_done, ifname=%s prefix=%s ...", __FUNCTION__, wan_ifname, prefix);
	start_wan_done(wan_ifname, prefix);

	/* check mwwatchdog enabled - part 2 of 2 */
	if (check_mwandog && !old_ck_pause) {
		snprintf(tmp2, sizeof(tmp2), "%s_ck_pause", prefix);
		nvram_set(tmp2, "0"); /* reset and check WAN XY with mwwatchdog again */
		logmsg(LOG_DEBUG, "*** %s: set %s_ck_pause=0 to check this WAN (multiwan watchdog)", __FUNCTION__, prefix);
	}

	return 0;
}

/*
 * Called when ipv4 link goes down
 */
int ipdown_main(int argc, char **argv)
{
	int proto;
	char prefix[] = "wanXX";
	char tmp[100], tmp2[32], tmp3[32];
	char ppplink_file[32];
	struct in_addr ipaddr;
	int mwan_num;

	mwan_num = mwan_active_num();

	if (!wait_action_idle(10))
		return -1;

	strlcpy(prefix, safe_getenv("LINKNAME"), sizeof(prefix));

	snprintf(ppplink_file, sizeof(ppplink_file), "/tmp/ppp/%s_link", prefix);
	unlink(ppplink_file);

	proto = get_wanx_proto(prefix);
	mwan_table_del(prefix);

	if ((proto == WP_L2TP) || (proto == WP_PPTP)) {
		/* clear dns from the resolv.conf */
		prefix_nvram_set(prefix, "get_dns", "", tmp, sizeof(tmp));
		dns_to_resolv();

		if (proto == WP_L2TP) {
			if (inet_pton(AF_INET, prefix_nvram_get(prefix, "l2tp_server_ip", tmp, sizeof(tmp)), &(ipaddr.s_addr))) {
				route_del(prefix_nvram_get(prefix, "ifname", tmp, sizeof(tmp)), 0, prefix_nvram_get(prefix, "l2tp_server_ip", tmp2, sizeof(tmp2)), prefix_nvram_get(prefix, "gateway", tmp3, sizeof(tmp3)), "255.255.255.255"); /* fixed routing problem in Israel */
				logmsg(LOG_DEBUG, "*** %s: route_del(%s, 0, %s, %s, 255.255.255.255)", __FUNCTION__, prefix_nvram_get(prefix, "ifname", tmp, sizeof(tmp)), prefix_nvram_get(prefix, "l2tp_server_ip", tmp2, sizeof(tmp2)), prefix_nvram_get(prefix, "gateway", tmp3, sizeof(tmp3)));
			}
		}

		if (proto == WP_PPTP) {
			if (inet_pton(AF_INET, prefix_nvram_get(prefix, "pptp_server_ip", tmp, sizeof(tmp)), &(ipaddr.s_addr))) {
				route_del(prefix_nvram_get(prefix, "ifname", tmp, sizeof(tmp)), 0, prefix_nvram_get(prefix, "pptp_server_ip", tmp2, sizeof(tmp2)), prefix_nvram_get(prefix, "gateway", tmp3, sizeof(tmp3)), "255.255.255.255");
				logmsg(LOG_DEBUG, "*** %s: route_del(%s, 0, %s, %s, 255.255.255.255)", __FUNCTION__, prefix_nvram_get(prefix, "ifname", tmp, sizeof(tmp)), prefix_nvram_get(prefix, "pptp_server_ip", tmp2, sizeof(tmp2)), prefix_nvram_get(prefix, "gateway", tmp3, sizeof(tmp3)));
			}
		}

		if (!prefix_nvram_get_int(prefix, "ppp_demand", tmp, sizeof(tmp))) { /* don't setup temp gateway for demand connections */
			/* restore the default gateway for WAN interface */
			prefix_nvram_set(prefix, "gateway_get", prefix_nvram_get(prefix, "gateway", tmp2, sizeof(tmp2)), tmp, sizeof(tmp));
			logmsg(LOG_DEBUG, "*** %s: restore default gateway: nvram_set(%s_gateway_get, %s)", __FUNCTION__, prefix, prefix_nvram_get(prefix, "gateway", tmp, sizeof(tmp)));

			if (mwan_num <= 1) {
				/* set default route to gateway if specified */
				route_del(prefix_nvram_get(prefix, "ifname", tmp, sizeof(tmp)), 0, "0.0.0.0", prefix_nvram_get(prefix, "gateway", tmp2, sizeof(tmp2)), "0.0.0.0");
				route_add(prefix_nvram_get(prefix, "ifname", tmp, sizeof(tmp)), 0, "0.0.0.0", prefix_nvram_get(prefix, "gateway", tmp2, sizeof(tmp2)), "0.0.0.0");
				logmsg(LOG_DEBUG, "*** %s: route_add(%s, 0, 0.0.0.0, %s, 0.0.0.0)", __FUNCTION__, prefix_nvram_get(prefix, "ifname", tmp, sizeof(tmp)), prefix_nvram_get(prefix, "gateway", tmp2, sizeof(tmp2)));
			}
		}

		/* unset received DNS entries (BAD for PPTP/L2TP here, it needs DNS on reconnect!) */
		//nvram_set(strlcat_r(prefix, "_get_dns", tmp, sizeof(tmp)), "");
	}

	/* don't kill all, only this wan listener!
	 * normally listen quits as link established
	 * and only one instance will run for a wan
	 */
	if (prefix_nvram_get_int(prefix, "ppp_demand", tmp, sizeof(tmp)))
		eval("listen", nvram_safe_get("lan_ifname"), prefix);

	mwan_load_balance();

	/* unset netmask in nvram only if equal to 255.255.255.255 (no MAN) */
	if (prefix_nvram_match(prefix, "netmask", "255.255.255.255", tmp, sizeof(tmp)))
		prefix_nvram_set(prefix, "netmask", "0.0.0.0", tmp, sizeof(tmp));

	/* don't clear active interface from nvram on disconnect. iface mandatory for mwan load balance */
	prefix_nvram_set(prefix, "pppd_pid", "", tmp, sizeof(tmp));

	/* WAN LED control */
	wan_led_off(prefix);

	return 1;
}

/*
 * Called when interface comes up
 */
int ippreup_main(int argc, char **argv)
{
	/* nothing to do right now! */
	return 0;
}

/*
 * Called when ipv6 link comes up
 */
#ifdef TCONFIG_IPV6
int ip6up_main(int argc, char **argv)
{
	char *wan_ifname;
	char prefix[] = "wanXX";
	char tmp[32];
	char *value;

	if (!wait_action_idle(10))
		return -1;

	/* ToDo: check mwwatchdog enabled for case IPv6! missing so far! */
	
	wan_ifname = safe_getenv("IFNAME");
	strlcpy(prefix, safe_getenv("LINKNAME"), sizeof(prefix));
	logmsg(LOG_DEBUG, "*** %s: wan_ifname = %s, prefix = %s.", __FUNCTION__, wan_ifname, prefix);

	if ((!wan_ifname) || (!*wan_ifname))
		return -1;

	/* check nvram wan_iface for case "none" (re-connect) or NUL */
	if (prefix_nvram_match(prefix, "iface", "none", tmp, sizeof(tmp)) || prefix_nvram_match(prefix, "iface", "", tmp, sizeof(tmp)))
		prefix_nvram_set(prefix, "iface", wan_ifname, tmp, sizeof(tmp)); /* set interface pppX in case ipup_main() not yet (or later) called */

	if ((value = getenv("LLREMOTE")))
		nvram_set("ipv6_llremote", value); /* set ipv6 llremote address */

	start_wan6(wan_ifname);

	return 0;
}

/*
 * Called when ipv6 link goes down
 */
int ip6down_main(int argc, char **argv)
{
	char *wan_ifname;

	if (!wait_action_idle(10))
		return -1;

	wan_ifname = safe_getenv("IFNAME");
	if ((!wan_ifname) || (!*wan_ifname))
		return -1;

	nvram_set("ipv6_llremote", ""); /* clear ipv6 llremote address */

	stop_wan6();

	return 0;
}
#endif /* TCONFIG_IPV6 */

int pppevent_main(int argc, char **argv)
{
	char prefix[] = "wanXX";
	char ppplog_file[32];

	strlcpy(prefix, safe_getenv("LINKNAME"), sizeof(prefix));
	int i;

	for (i = 1; i < argc; ++i) {
		if (strcmp(argv[i], "-t") == 0) {
			if (++i >= argc)
				return 1;

			if ((strcmp(argv[i], "PAP_AUTH_FAIL") == 0) || (strcmp(argv[i], "CHAP_AUTH_FAIL") == 0)) {
				snprintf(ppplog_file, sizeof(ppplog_file), "/tmp/ppp/%s_log", prefix);
				f_write_string(ppplog_file, argv[i], 0, 0);
				notice_set(prefix, "Authentication failed");

				return 0;
			}
		}
	}

	return 1;
}
