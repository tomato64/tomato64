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
 * THIS SOFTWARE IS OFFERED "AS IS", AND CYBERTAN GRANTS NO WARRANTIES OF ANY
 * KIND, EXPRESS OR IMPLIED, BY STATUTE, COMMUNICATION OR OTHERWISE.  CYBERTAN
 * SPECIFICALLY DISCLAIMS ANY IMPLIED WARRANTIES OF MERCHANTABILITY, FITNESS
 * FOR A SPECIFIC PURPOSE OR NONINFRINGEMENT CONCERNING THIS SOFTWARE
 *
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
 *
 * Modified for Tomato Firmware
 * Portions, Copyright (C) 2006-2009 Jonathan Zarate
 * Fixes/updates (C) 2018 - 2023 pedro
 *
 */


#include "rc.h"

#include <sys/sysinfo.h>
#include <sys/ioctl.h>
#include <arpa/inet.h>

/* needed by logmsg() */
#define LOGMSG_DISABLE	DISABLE_SYSLOG_OS
#define LOGMSG_NVDEBUG	"dhcp_debug"

/* only for mips (incl. mips RT-AC) - no support yet for struct nvram_tuple; sync to nvram default values if changed! */
#ifndef TCONFIG_BCMARM
#define FT_LAN_IP_ADDR	"192.168.1.1"
#define FT_LAN_NETMASK	"255.255.255.0"
#define FT_LAN_GATEWAY	"0.0.0.0"
#endif /* !TCONFIG_BCMARM */

static void expires(unsigned int seconds, char *prefix)
{
	struct sysinfo info;
	char s[32];
	char buf[64];

	sysinfo(&info);
	memset(s, 0, sizeof(s));
	snprintf(s, sizeof(s), "%u", (unsigned int)info.uptime + seconds);

	memset(buf, 0, sizeof(buf));
	snprintf(buf, sizeof(buf), "/var/lib/misc/dhcpc-%s.expires", prefix);
	f_write_string(buf, s, 0, 0);
}

static void do_renew_file(unsigned int renew, char *prefix)
{
	char buf[64];

	memset(buf, 0, sizeof(buf));
	snprintf(buf, sizeof(buf), "/var/lib/misc/%s_dhcpc.renewing", prefix);

	if (renew)
		f_write(buf, NULL, 0, 0, 0);
	else
		unlink(buf);
}

void do_connect_file(unsigned int connect, char *prefix)
{
	char buf[64];

	memset(buf, 0, sizeof(buf));
	snprintf(buf, sizeof(buf), "/var/lib/misc/%s.connecting", prefix);

	if (connect)
		f_write(buf, NULL, 0, 0, 0);
	else
		unlink(buf);
}

static int env2nv_gateway(const char *nv)
{
	char *v, *g;
	char *b;
	int r = 0;

	if ((v = getenv("router")) != NULL) {
		if ((b = strdup(v)) != NULL) {
			if ((v = strchr(b, ' ')) != NULL) /* truncate multiple entries */
				*v = 0;
			if (!nvram_match((char *)nv, b)) {
				nvram_set(nv, b);
				r = 1;
			}
			free(b);
		}
	}
	else if ((v = getenv("staticroutes")) != NULL) {
		if ((b = strdup(v)) == NULL)
			return 0;

		v = b;
		while ((g = strsep(&v, " ")) != NULL) {
			if (strcmp(g, "0.0.0.0/0") == 0) {
				if ((g = strsep(&v, " ")) && *g) {
					if (!nvram_match((char *)nv, g)) {
						nvram_set(nv, g);
						r = 1;
					}
					break;
				}
			}
		}
		free(b);
	}

	return r;
}

static int deconfig(char *ifname, char *prefix)
{
	char tmp[32];

	ifconfig(ifname, IFUP, "0.0.0.0", NULL);

	if (using_dhcpc(prefix)) {
		nvram_set(strlcat_r(prefix, "_ipaddr", tmp, sizeof(tmp)), "0.0.0.0");
		nvram_set(strlcat_r(prefix, "_netmask", tmp, sizeof(tmp)), "0.0.0.0");
		nvram_set(strlcat_r(prefix, "_gateway", tmp, sizeof(tmp)), "0.0.0.0");
	}
	nvram_set(strlcat_r(prefix, "_lease", tmp, sizeof(tmp)), "0");
	nvram_set(strlcat_r(prefix, "_routes1", tmp, sizeof(tmp)), "");
	nvram_set(strlcat_r(prefix, "_routes2", tmp, sizeof(tmp)), "");
	expires(0, prefix);

	if ((get_wanx_proto(prefix) == WP_DHCP) || (get_wanx_proto(prefix) == WP_LTE)) {
		nvram_set(strlcat_r(prefix, "_netmask", tmp, sizeof(tmp)), "0.0.0.0");
		nvram_set(strlcat_r(prefix, "_gateway_get", tmp, sizeof(tmp)), "0.0.0.0");
		nvram_set(strlcat_r(prefix, "_get_dns", tmp, sizeof(tmp)), "");
	}

#ifdef TCONFIG_IPV6
	nvram_set("wan_6rd", "");
#endif

	return 0;
}

static int bound(char *ifname, int renew, char *prefix)
{
	char tmp [32], tmp2[32];
	char *netmask, *dns, *gw;
	int wan_proto = get_wanx_proto(prefix);

	do_renew_file(0, prefix);

	logmsg(LOG_DEBUG, "*** IN %s: interface=%s, wan_prefix=%s, renew=%d, proto=%d", __FUNCTION__, ifname, prefix, renew, wan_proto);

	nvram_set(strlcat_r(prefix, "_routes1", tmp, sizeof(tmp)), "");
	nvram_set(strlcat_r(prefix, "_routes2", tmp, sizeof(tmp)), "");
	env2nv("ip", strlcat_r(prefix, "_ipaddr", tmp, sizeof(tmp)));
	env2nv_gateway(strlcat_r(prefix, "_gateway", tmp, sizeof(tmp)));
	env2nv("dns", strlcat_r(prefix, "_get_dns", tmp, sizeof(tmp)));
	env2nv("domain", strlcat_r(prefix, "_get_domain", tmp, sizeof(tmp)));
	env2nv("lease", strlcat_r(prefix, "_lease", tmp, sizeof(tmp)));
	netmask = getenv("subnet") ? : "255.255.255.255";
	if ((wan_proto == WP_DHCP) || (wan_proto == WP_LTE) || (using_dhcpc(prefix))) { /* netmask for DHCP MAN */
		nvram_set(strlcat_r(prefix, "_netmask", tmp, sizeof(tmp)), netmask);
		nvram_set(strlcat_r(prefix, "_gateway_get", tmp, sizeof(tmp)), nvram_safe_get(strlcat_r(prefix, "_gateway", tmp2, sizeof(tmp2))));  /* tmp2 needed --> code evaluation left to right! */
	}

	/* RFC3442: If the DHCP server returns both a Classless Static Routes option
	 * and a Router option, the DHCP client MUST ignore the Router option.
	 * Similarly, if the DHCP server returns both a Classless Static Routes
	 * option and a Static Routes option, the DHCP client MUST ignore the
	 * Static Routes option.
	 * Ref: http://www.faqs.org/rfcs/rfc3442.html
	 */
	/* Classless Static Routes (option 121) */
	if (!env2nv("staticroutes", strlcat_r(prefix, "_routes1", tmp, sizeof(tmp))))
		/* or MS Classless Static Routes (option 249) */
		env2nv("msstaticroutes", strlcat_r(prefix, "_routes1", tmp2, sizeof(tmp2)));

	/* Static Routes (option 33) */
	env2nv("routes", strlcat_r(prefix, "_routes2", tmp, sizeof(tmp)));

	expires(atoi(safe_getenv("lease")), prefix);

#ifdef TCONFIG_IPV6
	env2nv("ip6rd", "wan_6rd");
#endif

	logmsg(LOG_DEBUG, "*** %s: %s_ipaddr=%s", __FUNCTION__, prefix, nvram_safe_get(strlcat_r(prefix, "_ipaddr", tmp, sizeof(tmp))));
	logmsg(LOG_DEBUG, "*** %s: %s_netmask=%s", __FUNCTION__, prefix, netmask);
	logmsg(LOG_DEBUG, "*** %s: %s_gateway=%s", __FUNCTION__, prefix, nvram_safe_get(strlcat_r(prefix, "_gateway", tmp, sizeof(tmp))));
	logmsg(LOG_DEBUG, "*** %s: %s_get_dns=%s", __FUNCTION__, prefix, nvram_safe_get(strlcat_r(prefix, "_get_dns", tmp, sizeof(tmp))));
	logmsg(LOG_DEBUG, "*** %s: %s_routes1=%s", __FUNCTION__, prefix, nvram_safe_get(strlcat_r(prefix, "_routes1", tmp, sizeof(tmp))));
	logmsg(LOG_DEBUG, "*** %s: %s_routes2=%s", __FUNCTION__, prefix, nvram_safe_get(strlcat_r(prefix, "_routes2", tmp, sizeof(tmp))));

	ifconfig(ifname, IFUP, "0.0.0.0", NULL);
	ifconfig(ifname, IFUP, nvram_safe_get(strlcat_r(prefix, "_ipaddr", tmp, sizeof(tmp))), netmask);

	if ((wan_proto != WP_DHCP) && (wan_proto != WP_LTE)) {

		/* setup dnsmasq and routes to dns / access servers */
		gw = nvram_safe_get(strlcat_r(prefix, "_gateway", tmp, sizeof(tmp)));
		if ((*gw) && (strcmp(gw, "0.0.0.0") != 0)) {
			logmsg(LOG_DEBUG, "*** %s: do preset_wan ... ifname=%s gateway=%s netmask=%s prefix=%s", __FUNCTION__, ifname, gw, netmask, prefix);
			preset_wan(ifname, gw, netmask, prefix);
		}
		else {
			logmsg(LOG_DEBUG, "*** %s: NO gateway! Just do DHCP DNS stuff ...", __FUNCTION__);
			dns_to_resolv();
		}
		/* don't clear dns servers for PPTP/L2TP wans, required for pptp/l2tp server name resolution */
		dns = nvram_safe_get(strlcat_r(prefix, "_get_dns", tmp, sizeof(tmp)));
		if (wan_proto != WP_PPTP && wan_proto != WP_L2TP) {
			nvram_set(strlcat_r(prefix, "_get_dns", tmp, sizeof(tmp)), renew ? dns : "");
			logmsg(LOG_DEBUG, "*** %s: clear / set dns to resolv.conf", __FUNCTION__);
		}
		switch (wan_proto) {
		case WP_PPTP:
			logmsg(LOG_DEBUG, "*** %s: start_pptp(%s) ...", __FUNCTION__, prefix);
			start_pptp(prefix);
			break;
		case WP_L2TP:
			logmsg(LOG_DEBUG, "*** %s: start_l2tp(%s) ...", __FUNCTION__, prefix);
			start_l2tp(prefix);
			break;

		case WP_PPPOE:
			logmsg(LOG_DEBUG, "*** %s: start_pppoe(%s) ...", __FUNCTION__, prefix);

			if (!strcmp(prefix, "wan"))
				start_pppoe(PPPOEWAN, prefix);
			else if (!strcmp(prefix, "wan2"))
				start_pppoe(PPPOEWAN2, prefix);
#ifdef TCONFIG_MULTIWAN
			else if (!strcmp(prefix, "wan3"))
				start_pppoe(PPPOEWAN3, prefix);
			else if (!strcmp(prefix, "wan4"))
				start_pppoe(PPPOEWAN4, prefix);
#endif
			break;
		}
	}
	else {
		logmsg(LOG_DEBUG, "*** OUT %s: to start_wan_done, ifname=%s prefix=%s ...", __FUNCTION__, ifname, prefix);
		start_wan_done(ifname,prefix);
	}

	return 0;
}

static int renew(char *ifname, char *prefix)
{
	char *a;
	int changed = 0, changed_dns = 0, routes_changed = 0;
	int wan_proto = get_wanx_proto(prefix);
	char tmp[32], tmp2[32];

	logmsg(LOG_DEBUG, "*** %s: interface=%s, wan_prefix=%s", __FUNCTION__, ifname, prefix);

	do_renew_file(0, prefix);

	/* check IP/Gateway/Netmask - change/new ? */
	if ((env2nv("ip", strlcat_r(prefix, "_ipaddr", tmp, sizeof(tmp)))) ||
	    (env2nv_gateway(strlcat_r(prefix, "_gateway", tmp, sizeof(tmp)))) ||
	    (wan_proto == WP_LTE && env2nv("subnet", strlcat_r(prefix, "_netmask", tmp, sizeof(tmp)))) ||
	    (wan_proto == WP_DHCP && env2nv("subnet", strlcat_r(prefix, "_netmask", tmp, sizeof(tmp))))) {
		/* WAN IP or gateway changed, restart/reconfigure everything */
		logmsg(LOG_DEBUG, "*** %s: WAN IP or gateway changed, restart/reconfigure everything", __FUNCTION__);

		return bound(ifname, 1, prefix);
	}

	if ((wan_proto == WP_DHCP) || (wan_proto == WP_LTE)) {
		changed |= env2nv("domain", strlcat_r(prefix, "_get_domain", tmp, sizeof(tmp))); /* check Domain - change/new ? */
		changed_dns |= env2nv("dns", strlcat_r(prefix, "_get_dns", tmp, sizeof(tmp))); /* check DNS - change/new ? */
	}

	nvram_set(strlcat_r(prefix, "_routes1_save", tmp, sizeof(tmp)), nvram_safe_get(strlcat_r(prefix, "_routes1", tmp2, sizeof(tmp2)))); /* backup */
	nvram_set(strlcat_r(prefix, "_routes2_save", tmp, sizeof(tmp)), nvram_safe_get(strlcat_r(prefix, "_routes2", tmp2, sizeof(tmp2)))); /* tmp2 needed --> code evaluation left to right! */

	/* Classless Static Routes (option 121) or MS Classless Static Routes (option 249) */
	if (getenv("staticroutes"))
		routes_changed |= env2nv("staticroutes", strlcat_r(prefix, "_routes1_save", tmp, sizeof(tmp))); /* check for changes */
	else
		routes_changed |= env2nv("msstaticroutes", strlcat_r(prefix, "_routes1_save", tmp, sizeof(tmp)));
	/* Static Routes (option 33) */
	routes_changed |= env2nv("routes", strlcat_r(prefix, "_routes2_save", tmp, sizeof(tmp)));

	if ((a = getenv("lease")) != NULL) {
		nvram_set(strlcat_r(prefix, "_lease", tmp, sizeof(tmp)), a);
		expires(atoi(a), prefix);
	}

	if (changed) { /* changed - part 1 of 2 */
		set_host_domain_name();
		stop_dnsmasq();
	}

	if (changed_dns) {
		logmsg(LOG_DEBUG, "*** %s: DNS changed", __FUNCTION__);
		dns_to_resolv(); /* dynamic update */
	}

	if (changed) { /* changed - part 2 of 2 */
		logmsg(LOG_DEBUG, "*** %s: Domain changed", __FUNCTION__);
		start_dnsmasq();
	}

	if (routes_changed) {
		do_wan_routes(ifname, 0, 0, prefix); /* route delete old */
		nvram_set(strlcat_r(prefix, "_routes1", tmp, sizeof(tmp)), nvram_safe_get(strlcat_r(prefix, "_routes1_save", tmp2, sizeof(tmp2)))); /* save changes and prepare for route add */
		nvram_set(strlcat_r(prefix, "_routes2", tmp, sizeof(tmp)), nvram_safe_get(strlcat_r(prefix, "_routes2_save", tmp2, sizeof(tmp2))));
		do_wan_routes(ifname, 0, 1, prefix); /* route add new */
	}

	nvram_unset(strlcat_r(prefix, "_routes1_save", tmp, sizeof(tmp))); /* remove backup */
	nvram_unset(strlcat_r(prefix, "_routes2_save", tmp, sizeof(tmp)));

	return 0;
}

int dhcpc_event_main(int argc, char **argv)
{
	char *ifname;
	ifname = getenv("interface");
	char prefix[] = "wanXX";

	if (nvram_match("wan_ifname", ifname))
		strlcpy(prefix, "wan", sizeof(prefix));
	else if (nvram_match("wan_iface", ifname))
		strlcpy(prefix, "wan", sizeof(prefix));
	else if (nvram_match("wan2_ifname", ifname))
		strlcpy(prefix, "wan2", sizeof(prefix));
	else if (nvram_match("wan2_iface", ifname))
		strlcpy(prefix, "wan2", sizeof(prefix));
#ifdef TCONFIG_MULTIWAN
	else if (nvram_match("wan3_ifname", ifname))
		strlcpy(prefix, "wan3", sizeof(prefix));
	else if (nvram_match("wan3_iface", ifname))
		strlcpy(prefix, "wan3", sizeof(prefix));
	else if (nvram_match("wan4_ifname", ifname))
		strlcpy(prefix, "wan4", sizeof(prefix));
	else if (nvram_match("wan4_iface", ifname))
		strlcpy(prefix, "wan4", sizeof(prefix));
#endif
	else
		strlcpy(prefix, "wan", sizeof(prefix));

	if (!wait_action_idle(10))
		return 0;

	logmsg(LOG_DEBUG, "*** %s: interface=%s wan_prefix=%s event=%s", __FUNCTION__, ifname, prefix, argv[1] ? : "");

	if ((!argv[1]) || (ifname == NULL))
		return EINVAL;
	else if (strstr(argv[1], "deconfig"))
		return deconfig(ifname, prefix);
	else if (strstr(argv[1], "bound"))
		return bound(ifname, 0, prefix);
	else if ((strstr(argv[1], "renew")) || (strstr(argv[1], "update")))
		return renew(ifname, prefix);

	return 0;
}

int dhcpc_release_main(int argc, char **argv)
{
	char prefix[] = "wanXX";
	char pid_file[64];

	if (argc > 1)
		strlcpy(prefix, argv[1], sizeof(prefix));
	else
		strlcpy(prefix, "wan", sizeof(prefix));

	logmsg(LOG_DEBUG, "*** %s: argc=%d wan_prefix=%s", __FUNCTION__, argc, prefix);

	mwan_table_del(prefix); /* for dual WAN and multi WAN */

	if (!using_dhcpc(prefix))
		return 1;

	memset(pid_file, 0, sizeof(pid_file));
	snprintf(pid_file, sizeof(pid_file), "/var/run/udhcpc-%s.pid", prefix);
	if (kill_pidfile_s(pid_file, SIGUSR2) == 0)
		sleep(2);

	do_renew_file(0, prefix);

	do_connect_file(0, prefix);

	mwan_load_balance(); /* for dual WAN and multi WAN */

	/* WAN LED control */
	wan_led_off(prefix);

	return 0;
}

int dhcpc_renew_main(int argc, char **argv)
{
	char prefix[] = "wanXX";
	char pid_file[64];

	if (argc > 1)
		strlcpy(prefix, argv[1], sizeof(prefix));
	else
		strlcpy(prefix, "wan", sizeof(prefix));

	logmsg(LOG_DEBUG, "*** %s: argc=%d wan_prefix=%s", __FUNCTION__, argc, prefix);

	mwan_table_add(prefix); /* for dual WAN and multi WAN */

	if (!using_dhcpc(prefix))
		return 1;

	memset(pid_file, 0, sizeof(pid_file));
	snprintf(pid_file, sizeof(pid_file), "/var/run/udhcpc-%s.pid", prefix);
	if (kill_pidfile_s(pid_file, SIGUSR1) == 0)
		do_renew_file(1, prefix);
	else {
		stop_dhcpc(prefix);
		start_dhcpc(prefix);
	}

	mwan_load_balance(); /* for dual WAN and multi WAN */

	return 0;
}

static void restart_basic_services(void) {
	stop_firewall();
	start_firewall();
	set_host_domain_name();
	stop_dnsmasq();
	dns_to_resolv();
	start_dnsmasq();
	stop_httpd();
	start_httpd();
}

static void restart_ntpd(void) {
	logmsg(LOG_DEBUG, "*** %s: restart ntpd", __FUNCTION__);
	stop_ntpd();
	sleep(1); /* wait a little bit before calling start_ntpd() */
	start_ntpd();
}

static int expires_lan(unsigned int in)
{
	time_t now;
	FILE *fp;
	char tmp[64];

	time(&now);
	snprintf(tmp, sizeof(tmp), "/tmp/dhcpc-lan.expires");
	if (!(fp = fopen(tmp, "w"))) {
		logerr(__FUNCTION__, __LINE__, tmp);
		return 1;
	}
	fprintf(fp, "%d", (unsigned int) now + in);
	fclose(fp);

	return 0;
}

static int deconfig_lan(void)
{
	char *lan_ifname = safe_getenv("interface");

	logmsg(LOG_DEBUG, "*** %s", __FUNCTION__);

#ifdef TCONFIG_BCMARM
	ifconfig(lan_ifname, IFUP | IFF_ALLMULTI, nvram_default_get("lan_ipaddr"), nvram_default_get("lan_netmask")); /* nvram (or FreshTomato) default values */
#else /* mips */
	ifconfig(lan_ifname, IFUP | IFF_ALLMULTI, FT_LAN_IP_ADDR, FT_LAN_NETMASK);
#endif

	expires_lan(0);

	/* Remove default route to gateway if specified */
	route_del(lan_ifname, 0, "0.0.0.0", nvram_safe_get("lan_gateway"), "0.0.0.0");

	/* clear resolv.conf */
	clear_resolv();

	/* completely clear old setup and bring back nvram (or FreshTomato) default values */
#ifdef TCONFIG_BCMARM
	nvram_set("lan_ipaddr", nvram_default_get("lan_ipaddr"));
	nvram_set("lan_netmask", nvram_default_get("lan_netmask"));
	nvram_set("lan_gateway", nvram_default_get("lan_gateway"));
#else /* mips */
	nvram_set("lan_ipaddr", FT_LAN_IP_ADDR);
	nvram_set("lan_netmask", FT_LAN_NETMASK);
	nvram_set("lan_gateway", FT_LAN_GATEWAY);
#endif
	nvram_set("wan_lease", "");
	nvram_set("wan_dns", "");

	/* (re)start firewall, dnsmasq and httpd */
	restart_basic_services();

	return 0;
}

static int bound_lan(void)
{
	char *lan_ifname = safe_getenv("interface");
	char *value;

	logmsg(LOG_DEBUG, "*** %s", __FUNCTION__);

	/* get IP/Gateway/Netmask */
	env2nv("ip", "lan_ipaddr");
	env2nv_gateway("lan_gateway");
	value = getenv("subnet") ? : "255.255.255.255";
	nvram_set("lan_netmask", value);

	if ((value = getenv("lease"))) {
		nvram_set("wan_lease", value); /* keep it easy - use wan variable! */
		expires_lan(atoi(value));
	}

	/* get DNS */
	env2nv("dns", "wan_dns"); /* keep it easy - use wan variable! (static dns) */

	ifconfig(lan_ifname, IFUP | IFF_ALLMULTI, nvram_safe_get("lan_ipaddr"), nvram_safe_get("lan_netmask"));

	/* Set default route to gateway if specified */
	route_add(lan_ifname, 0, "0.0.0.0", nvram_safe_get("lan_gateway"), "0.0.0.0");

	/* (re)start firewall, dnsmasq and httpd */
	restart_basic_services();

	/* get (sync) time */
	restart_ntpd();

	return 0;
}

static int renew_lan(void)
{
	char *value;
	char lan_gateway[32] = {0};
	char *lan_ifname = safe_getenv("interface");
	int changed = 0, changed_dns = 0;

	logmsg(LOG_DEBUG, "*** %s", __FUNCTION__);

	snprintf(lan_gateway, sizeof(lan_gateway),"%s", nvram_safe_get("lan_gateway")); /* copy current (old) nvram lan_gateway value first */

	/* check IP/Gateway/Netmask - change/new ? */
	changed = env2nv("ip", "lan_ipaddr");
	changed += env2nv_gateway("lan_gateway");
	changed += env2nv("subnet", "lan_netmask");

	if (changed) {
		/* IP or GATEWAY or NETMASK changed - reconfigure everything */
		logmsg(LOG_DEBUG, "*** %s: IP or GATEWAY or NETMASK changed - reconfigure everything", __FUNCTION__);

		/* Remove current (old) default route to gateway if specified */
		route_del(lan_ifname, 0, "0.0.0.0", lan_gateway, "0.0.0.0");

		return bound_lan();
	}

	if ((value = getenv("lease"))) {
		nvram_set("wan_lease", value); /* keep it easy - use wan variable! */
		expires_lan(atoi(value));
	}

	/* check DNS - change/new ? */
	changed_dns = env2nv("dns", "wan_dns"); /* keep it easy - use wan variable! (static dns) */

	if (changed_dns) {
		logmsg(LOG_DEBUG, "*** %s: DNS changed", __FUNCTION__);
		dns_to_resolv();
	}

	return 0;
}

int dhcpc_lan_main(int argc, char **argv)
{

	if (!wait_action_idle(10))
		return 0;

	logmsg(LOG_DEBUG, "*** %s: event=%s", __FUNCTION__, argv[1] ? : "");

	if (!argv[1])
		return EINVAL;
	else if (strstr(argv[1], "deconfig"))
		return deconfig_lan();
	else if (strstr(argv[1], "bound"))
		return bound_lan();
	else if ((strstr(argv[1], "renew")) || (strstr(argv[1], "update")))
		return renew_lan();

	return 0;
}

void start_dhcpc_lan(void)
{
	char pid_file[64];
	char cmd[256];
	char tmp[64];
	char *ifname = nvram_safe_get("lan_ifname");
	char *argv[128];
	int argc = 0;
	pid_t pid;
	int wan_proto = get_wan_proto(); /* 1. check wan disabled for AP / WET / Media Brige Mode */
	int lan_dhcp = nvram_get_int("lan_dhcp"); /* 2. check if DHCP Client for LAN (br0) is enabled! */

	stop_dhcpc_lan();

	/* check condidtions before we go on! */
	if ((wan_proto != WP_DISABLED) || (!lan_dhcp)) {
		logmsg(LOG_DEBUG, "*** %s: Not in AP / WET / MB Mode - skipping DHCP Client for Lan (br0)!", __FUNCTION__);
		return;
	}

	memset(pid_file, 0, sizeof(pid_file));
	snprintf(pid_file, sizeof(pid_file), "/var/run/udhcpc-lan.pid");

	memset(tmp, 0, sizeof(tmp));
	if (nvram_invmatch("wan_hostname", "")) {
		strlcpy(tmp, "-x hostname:", sizeof(tmp));
		strlcat(tmp, nvram_safe_get("wan_hostname"), sizeof(tmp));
	}

	memset(cmd, 0, sizeof(cmd));
	snprintf(cmd, sizeof(cmd), "/sbin/udhcpc -i %s -s /sbin/dhcpc-event-lan -p %s %s %s",
	                           ifname,
	                           pid_file,
	                           tmp,
	                           nvram_safe_get("dhcpc_custom")
	);

	logmsg(LOG_DEBUG, "*** %s: ifname=%s cmd=%s", __FUNCTION__, ifname, cmd);

	for (argv[argc = 0] = strtok(cmd, " "); argv[argc] != NULL; argv[++argc] = strtok(NULL, " "));
	_eval(argv, NULL, 0, &pid);
}

void stop_dhcpc_lan(void)
{
	char pid_file[64];

	killall("dhcpc-event-lan", SIGTERM);

	memset(pid_file, 0, sizeof(pid_file));
	snprintf(pid_file, sizeof(pid_file), "/var/run/udhcpc-lan.pid");
	kill_pidfile_s(pid_file, SIGUSR2);
	kill_pidfile_s(pid_file, SIGTERM);
	unlink(pid_file);
}

void start_dhcpc(char *prefix)
{
	char pid_file[64];
	char cmd[512];
	char tmp[64];
	char *ifname;
	int proto;
	char *argv[128];
	int argc = 0;
	pid_t pid;

	nvram_set(strlcat_r(prefix, "_get_dns", tmp, sizeof(tmp)), "");

	do_renew_file(1, prefix);

	proto = get_wanx_proto(prefix);
	ifname = nvram_safe_get(strlcat_r(prefix, "_ifname", tmp, sizeof(tmp)));

	if ((proto == WP_DHCP) || (proto == WP_LTE))
		nvram_set(strlcat_r(prefix, "_iface", tmp, sizeof(tmp)), ifname);

	memset(pid_file, 0, sizeof(pid_file));
	snprintf(pid_file, sizeof(pid_file), "/var/run/udhcpc-%s.pid", prefix);

	memset(tmp, 0, sizeof(tmp));
	if (nvram_invmatch("wan_hostname", "")) {
		strlcpy(tmp, "-x hostname:", sizeof(tmp));
		strlcat(tmp, nvram_safe_get("wan_hostname"), sizeof(tmp));
	}

	memset(cmd, 0, sizeof(cmd));
	snprintf(cmd, sizeof(cmd), "/sbin/udhcpc -i %s -b -s /sbin/dhcpc-event -p %s %s %s %s %s %s %s",
	                           ifname,
	                           pid_file,
	                           tmp,
	                           nvram_get_int("dhcp_routes") ? "-O33 -O121 -O249" : "", /* routes/staticroutes/msstaticroutes */
	                           nvram_get_int("dhcpc_minpkt") ? "-m" : "",
	                           nvram_contains_word("log_events", "dhcpc") ? "-S" : "",
#ifdef TCONFIG_IPV6
	                           (get_ipv6_service() == IPV6_6RD_DHCP) ? "-O212 -O150" : "", /* ip6rd rfc/ip6rd comcast */
#else
	                           "",
#endif
	                           nvram_safe_get("dhcpc_custom")
	);

	logmsg(LOG_DEBUG, "*** %s: prefix=%s cmd=%s", __FUNCTION__, prefix, cmd);

	for (argv[argc = 0] = strtok(cmd, " "); argv[argc] != NULL; argv[++argc] = strtok(NULL, " "));
	_eval(argv, NULL, 0, &pid);
}

void stop_dhcpc(char *prefix)
{
	char pid_file[64];

	killall("dhcpc-event", SIGTERM);

	memset(pid_file, 0, sizeof(pid_file));
	snprintf(pid_file, sizeof(pid_file), "/var/run/udhcpc-%s.pid", prefix);
	if (kill_pidfile_s(pid_file, SIGUSR2) == 0) /* release */
		sleep(2);

	kill_pidfile_s(pid_file, SIGTERM);
	unlink(pid_file);

	do_renew_file(0, prefix);

	/* WAN LED control */
	wan_led_off(prefix);
}

#ifdef TCONFIG_IPV6
int dhcp6c_state_main(int argc, char **argv)
{
	const char *prefix;
	const char *lanif;
	char *reason;

	if (!wait_action_idle(10))
		return 1;

	/* check environment variable "REASON" which is passed to the client script when receiving a REPLY message
	 * Example: reason can be "REQUEST" or "RENEW" or "RELEASE" or ... see dhcp6c.8 manual
	 */
	if ((reason = getenv("REASON")) != NULL) {
		logmsg(LOG_DEBUG, "*** %s: REASON=%s", __FUNCTION__, reason);
	}

	lanif = getifaddr(nvram_safe_get("lan_ifname"), AF_INET6, 0);

	/* check IPv6 addr - change/new ? */
	if (!nvram_match("ipv6_rtr_addr", (char *) lanif)) {
		nvram_set("ipv6_rtr_addr", lanif);

		/* extract prefix from configured IPv6 address */
		prefix = ipv6_prefix(NULL);
		if (!nvram_match("ipv6_prefix", (char *) prefix))
			nvram_set("ipv6_prefix", prefix);

		/* (re)start dnsmasq and httpd */
		set_host_domain_name();
		stop_dnsmasq();
		start_dnsmasq();
		stop_httpd();
		start_httpd();
	}

	/* check DNS - change/new ? */
	if (env2nv("new_domain_name_servers", "ipv6_get_dns"))
		dns_to_resolv();

	return 0;
}

void start_dhcp6c(void)
{
	FILE *f;
	int prefix_len;
	char *wan6face;
	char *argv[] = { "/usr/sbin/dhcp6c", "-T",
			 NULL,	/* LL | LLT */
			 NULL,	/* -D (Debug On) */
			 NULL,	/* -n (no prefix/address release on exit) */
			 NULL,	/* interface */
			 NULL };
	int argc;
	int ipv6_vlan = 0; /* bit 0 = IPv6 enabled for LAN1, bit 1 = IPv6 enabled for LAN2, bit 2 = IPv6 enabled for LAN3, 1 == TRUE, 0 == FALSE */
	int duid_type;

	/* Check if turned on */
	if (get_ipv6_service() != IPV6_NATIVE_DHCP)
		return;

	duid_type = nvram_get_int("ipv6_duid_type");

	/* check duid range */
	if (duid_type < 1 || duid_type > 4)
		duid_type = 3; /* default to DUID-LL */
	  
	argc = 2;
	switch (duid_type) {
		case 1: /* DUID-LLT */
			argv[argc] = "LLT";
			break;
		case 3: /* DUID-LL (default) and fall through */
		default:
			argv[argc] = "LL";
			break;
	}

	prefix_len = 64 - (nvram_get_int("ipv6_prefix_length") ? : 64);
	if (prefix_len < 0)
		prefix_len = 0;

	wan6face = nvram_safe_get("wan_iface");
	ipv6_vlan = nvram_get_int("ipv6_vlan");

	nvram_set("ipv6_get_dns", "");
	nvram_set("ipv6_rtr_addr", "");
	nvram_set("ipv6_prefix", "");

	nvram_set("ipv6_pd_pltime", "0"); /* reset preferred and valid lifetime */
	nvram_set("ipv6_pd_vltime", "0");

	/* Create dhcp6c.conf */
	unlink("/var/dhcp6c_duid");
	if ((f = fopen("/etc/dhcp6c.conf", "w"))) {
		fprintf(f, "interface %s {\n", wan6face);

		if (nvram_get_int("ipv6_pdonly") == 0)
			fprintf(f, " send ia-na 0;\n");

		fprintf(f, " send ia-pd 0;\n"
		           " request domain-name-servers;\n"
		           " script \"/sbin/dhcp6c-state\";\n"
		           "};\n"
		           "id-assoc pd 0 {\n"
		           " prefix ::/%d infinity;\n"
		           " prefix-interface %s {\n"
		           "  sla-id 0;\n"
		           "  sla-len %d;\n"
		           "  ifid 1;\n" /* override the default EUI-64 address selection and create a very userfriendly address --> ::1 */
		           " };\n",
		           nvram_get_int("ipv6_prefix_length"),
		           nvram_safe_get("lan_ifname"),
		           prefix_len);

		/* check IPv6 for LAN1 */
		if ((ipv6_vlan & 0x01) && (prefix_len >= 1) && (strcmp(nvram_safe_get("lan1_ipaddr"), "") != 0)) /* 2x IPv6 /64 networks possible --> for LAN and LAN1 */
			fprintf(f, " prefix-interface %s {\n"
			           "  sla-id 1;\n"
			           "  sla-len %d;\n"
			           "  ifid 1;\n" /* override the default EUI-64 address selection and create a very userfriendly address --> ::1 */
			           " };\n", nvram_safe_get("lan1_ifname"), prefix_len);

		/* check IPv6 for LAN2 */
		if ((ipv6_vlan & 0x02) && (prefix_len >= 2) && (strcmp(nvram_safe_get("lan2_ipaddr"), "") != 0)) /* 4x IPv6 /64 networks possible --> for LAN to LAN2 */
			fprintf(f, " prefix-interface %s {\n"
		                   "  sla-id 2;\n"
		                   "  sla-len %d;\n"
		                   "  ifid 1;\n" /* override the default EUI-64 address selection and create a very userfriendly address --> ::1 */
		                   " };\n", nvram_safe_get("lan2_ifname"), prefix_len);

		/* check IPv6 for LAN3 */
		if ((ipv6_vlan & 0x04) && (prefix_len >= 2) && (strcmp(nvram_safe_get("lan3_ipaddr"), "") != 0)) /* 4x IPv6 /64 networks possible --> for LAN to LAN3 */
			fprintf(f, " prefix-interface %s {\n"
			           "  sla-id 3;\n"
			           "  sla-len %d;\n"
			           "  ifid 1;\n" /* override the default EUI-64 address selection and create a very userfriendly address --> ::1 */
			           " };\n", nvram_safe_get("lan3_ifname"), prefix_len);

		fprintf(f, "};\n"
		           "id-assoc na 0 { };\n");

		fclose(f);
	}

	argc = 3;
#if defined(TCONFIG_BLINK) || defined(TCONFIG_BCMARM) /* RT-N+ */
	if (nvram_get_int("ipv6_debug"))
		argv[argc++] = "-D";
#endif

	if (nvram_get_int("ipv6_pd_norelease"))
		argv[argc++] = "-n";

	argv[argc++] = wan6face;
	argv[argc] = NULL;
	_eval(argv, NULL, 0, NULL);
}

void stop_dhcp6c(void)
{
	killall("dhcp6c-event", SIGTERM);
	killall_tk_period_wait("dhcp6c", 50);

	nvram_set("ipv6_pd_pltime", "0"); /* reset preferred and valid lifetime */
	nvram_set("ipv6_pd_vltime", "0");
}
#endif /* TCONFIG_IPV6 */
