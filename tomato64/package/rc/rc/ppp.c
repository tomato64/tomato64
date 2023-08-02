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
	$Id: ppp.c,v 1.27 2005/03/29 02:00:06 honor Exp $
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
	int proto;
	char prefix[] = "wanXX";
	char tmp[100];
	char ppplink_file[32];
	char buf[256];
	char *value;
	const char *p;

	if (!wait_action_idle(10))
		return -1;

	logmsg(LOG_DEBUG, "*** IN %s: IFNAME=%s DEVICE=%s LINKNAME=%s IPREMOTE=%s IPLOCAL=%s DNS1=%s DNS2=%s", __FUNCTION__, getenv("IFNAME"), getenv("DEVICE"), getenv("LINKNAME"), getenv("IPREMOTE"), getenv("IPLOCAL"), getenv("DNS1"), getenv("DNS2"));

	wan_ifname = safe_getenv("IFNAME");
	strcpy(prefix, safe_getenv("LINKNAME"));
	logmsg(LOG_DEBUG, "*** %s: wan_ifname = %s, prefix = %s.", __FUNCTION__, wan_ifname, prefix);

	if ((!wan_ifname) || (!*wan_ifname))
		return -1;

	nvram_set(strlcat_r(prefix, "_iface", tmp, sizeof(tmp)), wan_ifname); /* ppp# */
	nvram_set(strlcat_r(prefix, "_pppd_pid", tmp, sizeof(tmp)), safe_getenv("PPPD_PID"));

	/* ipup receives six arguments:
	 *   <interface name>  <tty device>  <speed> <local IP address> <remote IP address> <ipparam>
	 *   ppp1              vlan1         0       71.135.98.32       151.164.184.87      0
	 */
	memset(ppplink_file, 0, 32);
	sprintf(ppplink_file, "/tmp/ppp/%s_link", prefix);
	f_write_string(ppplink_file, argv[1], 0, 0);

	if ((p = getenv("IPREMOTE"))) {
		nvram_set(strlcat_r(prefix, "_gateway_get", tmp, sizeof(tmp)), p);
		logmsg(LOG_DEBUG, "*** %s: set %s_gateway_get=%s", __FUNCTION__, prefix, p);
	}

	if ((value = getenv("IPLOCAL"))) {
		proto = get_wanx_proto(prefix);

		switch (proto) { /* store last ip address for Web UI */
			case WP_PPPOE:
			case WP_PPP3G:
				if ((proto == WP_PPPOE) && using_dhcpc(prefix)) /* PPPoE with DHCP MAN */
					nvram_set(strlcat_r(prefix, "_ipaddr_buf", tmp, sizeof(tmp)), nvram_safe_get(strlcat_r(prefix, "_ppp_get_ip", tmp, sizeof(tmp))));
				else { /* PPPoE / 3G */
					nvram_set(strlcat_r(prefix, "_ipaddr_buf", tmp, sizeof(tmp)), nvram_safe_get(strlcat_r(prefix, "_ipaddr", tmp, sizeof(tmp))));
					nvram_set(strlcat_r(prefix, "_ipaddr", tmp, sizeof(tmp)), value);
				}
				break;
			case WP_PPTP:
			case WP_L2TP:
				nvram_set(strlcat_r(prefix, "_ipaddr_buf", tmp, sizeof(tmp)), nvram_safe_get(strlcat_r(prefix, "_ppp_get_ip", tmp, sizeof(tmp))));
				break;
		}

		/* set netmask in nvram only if not already set (MAN) */
		if (nvram_match(strlcat_r(prefix, "_netmask", tmp, sizeof(tmp)), "0.0.0.0"))
			nvram_set(strlcat_r(prefix, "_netmask", tmp, sizeof(tmp)), "255.255.255.255");

		if (!nvram_match(strlcat_r(prefix, "_ppp_get_ip", tmp, sizeof(tmp)), value)) {
			ifconfig(wan_ifname, IFUP, "0.0.0.0", NULL);
			nvram_set(strlcat_r(prefix, "_ppp_get_ip", tmp, sizeof(tmp)), value);
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
	nvram_set(strlcat_r(prefix, "_get_dns", tmp, sizeof(tmp)), buf);

	if ((value = getenv("AC_NAME")))
		nvram_set(strlcat_r(prefix, "_ppp_get_ac", tmp, sizeof(tmp)), value);
	if ((value = getenv("SRV_NAME")))
		nvram_set(strlcat_r(prefix, "_ppp_get_srv", tmp, sizeof(tmp)), value);
	if ((value = getenv("MTU")))
		nvram_set(strlcat_r(prefix, "_run_mtu", tmp, sizeof(tmp)), value);

	logmsg(LOG_DEBUG, "*** OUT %s: to start_wan_done, ifname=%s prefix=%s ...", __FUNCTION__, wan_ifname, prefix);
	start_wan_done(wan_ifname, prefix);

	return 0;
}

/*
 * Called when ipv4 link goes down
 */
int ipdown_main(int argc, char **argv)
{
	int proto;
	char prefix[] = "wanXX";
	char tmp[100];
	char ppplink_file[32];
	struct in_addr ipaddr;
	int mwan_num;

	mwan_num = nvram_get_int("mwan_num");
	if ((mwan_num < 1) || (mwan_num > MWAN_MAX))
		mwan_num = 1;

	if (!wait_action_idle(10))
		return -1;

	strcpy(prefix, safe_getenv("LINKNAME"));

	memset(ppplink_file, 0, 32);
	sprintf(ppplink_file, "/tmp/ppp/%s_link", prefix);
	unlink(ppplink_file);

	proto = get_wanx_proto(prefix);
	mwan_table_del(prefix);

	if ((proto == WP_L2TP) || (proto == WP_PPTP)) {
		/* clear dns from the resolv.conf */
		nvram_set(strlcat_r(prefix, "_get_dns", tmp, sizeof(tmp)), "");
		dns_to_resolv();

		if (proto == WP_L2TP) {
			if (inet_pton(AF_INET, nvram_safe_get(strlcat_r(prefix, "_l2tp_server_ip", tmp, sizeof(tmp))), &(ipaddr.s_addr))) {
				route_del(nvram_safe_get(strlcat_r(prefix, "_ifname", tmp, sizeof(tmp))), 0, nvram_safe_get(strlcat_r(prefix, "_l2tp_server_ip", tmp, sizeof(tmp))), nvram_safe_get(strlcat_r(prefix, "_gateway", tmp, sizeof(tmp))), "255.255.255.255"); /* fixed routing problem in Israel */
				logmsg(LOG_DEBUG, "*** %s: route_del(%s, 0, %s, %s, 255.255.255.255)", __FUNCTION__, nvram_safe_get(strlcat_r(prefix, "_ifname", tmp, sizeof(tmp))), nvram_safe_get(strlcat_r(prefix, "_l2tp_server_ip", tmp, sizeof(tmp))), nvram_safe_get(strlcat_r(prefix, "_gateway", tmp, sizeof(tmp))));
			}
		}

		if (proto == WP_PPTP) {
			if (inet_pton(AF_INET, nvram_safe_get(strlcat_r(prefix, "_pptp_server_ip", tmp, sizeof(tmp))), &(ipaddr.s_addr))) {
				route_del(nvram_safe_get(strlcat_r(prefix, "_ifname", tmp, sizeof(tmp))), 0, nvram_safe_get(strlcat_r(prefix, "_pptp_server_ip", tmp, sizeof(tmp))), nvram_safe_get(strlcat_r(prefix, "_gateway", tmp, sizeof(tmp))), "255.255.255.255");
				logmsg(LOG_DEBUG, "*** %s: route_del(%s, 0, %s, %s, 255.255.255.255)", __FUNCTION__, nvram_safe_get(strlcat_r(prefix, "_ifname", tmp, sizeof(tmp))), nvram_safe_get(strlcat_r(prefix, "_pptp_server_ip", tmp, sizeof(tmp))), nvram_safe_get(strlcat_r(prefix, "_gateway", tmp, sizeof(tmp))));
			}
		}

		if (!nvram_get_int(strlcat_r(prefix, "_ppp_demand", tmp, sizeof(tmp)))) { /* don't setup temp gateway for demand connections */
			/* restore the default gateway for WAN interface */
			nvram_set(strlcat_r(prefix, "_gateway_get", tmp, sizeof(tmp)), nvram_safe_get(strlcat_r(prefix, "_gateway", tmp, sizeof(tmp))));
			logmsg(LOG_DEBUG, "*** %s: restore default gateway: nvram_set(%s_gateway_get, %s)", __FUNCTION__, prefix, nvram_safe_get(strlcat_r(prefix, "_gateway", tmp, sizeof(tmp))));

			if (mwan_num == 1) {
				/* set default route to gateway if specified */
				route_del(nvram_safe_get(strlcat_r(prefix, "_ifname", tmp, sizeof(tmp))), 0, "0.0.0.0", nvram_safe_get(strlcat_r(prefix, "_gateway", tmp, sizeof(tmp))), "0.0.0.0");
				route_add(nvram_safe_get(strlcat_r(prefix, "_ifname", tmp, sizeof(tmp))), 0, "0.0.0.0", nvram_safe_get(strlcat_r(prefix, "_gateway", tmp, sizeof(tmp))), "0.0.0.0");
				logmsg(LOG_DEBUG, "*** %s: route_add(%s, 0, 0.0.0.0, %s, 0.0.0.0)", __FUNCTION__, nvram_safe_get(strlcat_r(prefix, "_ifname", tmp, sizeof(tmp))), nvram_safe_get(strlcat_r(prefix, "_gateway", tmp, sizeof(tmp))));
			}
		}

		/* unset received DNS entries (BAD for PPTP/L2TP here, it needs DNS on reconnect!) */
		//nvram_set(strlcat_r(prefix, "_get_dns", tmp, sizeof(tmp)), "");
	}

	/* don't kill all, only this wan listener!
	 * normally listen quits as link established
	 * and only one instance will run for a wan
	 */
	if (nvram_get_int(strlcat_r(prefix, "_ppp_demand", tmp, sizeof(tmp))))
		eval("listen", nvram_safe_get("lan_ifname"), prefix);

	mwan_load_balance();

	/* unset netmask in nvram only if equal to 255.255.255.255 (no MAN) */
	if (nvram_match(strlcat_r(prefix, "_netmask", tmp, sizeof(tmp)), "255.255.255.255"))
		nvram_set(strlcat_r(prefix, "_netmask", tmp, sizeof(tmp)), "0.0.0.0");

	/* don't clear active interface from nvram on disconnect. iface mandatory for mwan load balance */
	nvram_set(strlcat_r(prefix, "_pppd_pid", tmp, sizeof(tmp)),"");

	/* WAN LED control */
	wan_led_off(prefix);

	return 1;
}

/*
 * Called when interface comes up
 */
int ippreup_main(int argc, char **argv)
{
	/* nothing to do righ now! */
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

	wan_ifname = safe_getenv("IFNAME");
	strcpy(prefix, safe_getenv("LINKNAME"));
	logmsg(LOG_DEBUG, "*** %s: wan_ifname = %s, prefix = %s.", __FUNCTION__, wan_ifname, prefix);

	if ((!wan_ifname) || (!*wan_ifname))
		return -1;

	/* check nvram wan_iface for case "none" (re-connect) or NUL */
	if (nvram_match(strlcat_r(prefix, "_iface", tmp, sizeof(tmp)), "none") || nvram_match(strlcat_r(prefix, "_iface", tmp, sizeof(tmp)), ""))
		nvram_set(strlcat_r(prefix, "_iface", tmp, sizeof(tmp)), wan_ifname); /* set interface pppX in case ipup_main() not yet (or later) called */

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

	strcpy(prefix, safe_getenv("LINKNAME"));
	int i;

	for (i = 1; i < argc; ++i) {
		if (strcmp(argv[i], "-t") == 0) {
			if (++i >= argc)
				return 1;

			if ((strcmp(argv[i], "PAP_AUTH_FAIL") == 0) || (strcmp(argv[i], "CHAP_AUTH_FAIL") == 0)) {
				memset(ppplog_file, 0, 32);
				sprintf(ppplog_file, "/tmp/ppp/%s_log", prefix);
				f_write_string(ppplog_file, argv[i], 0, 0);
				notice_set(prefix, "Authentication failed");

				return 0;
			}
		}
	}

	return 1;
}
