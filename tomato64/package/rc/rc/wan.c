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

	Modified for Tomato Firmware
	Portions, Copyright (C) 2006-2009 Jonathan Zarate

*/


#include "rc.h"

#include <sys/ioctl.h>
#include <arpa/inet.h>
#include <sys/sysinfo.h>
#include <time.h>
#include <bcmdevs.h>
#include <linux/sockios.h>
#include <linux/if_vlan.h>

/* needed by logmsg() */
#define LOGMSG_DISABLE	DISABLE_SYSLOG_OSM
#define LOGMSG_NVDEBUG	"wan_debug"


static int config_pppd(int wan_proto, int num, char *prefix)
{
	FILE *fp;
#ifdef TCONFIG_USB
	FILE *cfp;
#endif
	char *p;
	int demand;
	char tmp[100];

	mkdir("/tmp/ppp", 0777);
	symlink("/dev/null", "/tmp/ppp/connect-errors");
	symlink("/sbin/rc", "/tmp/ppp/ip-up");
	symlink("/sbin/rc", "/tmp/ppp/ip-down");
	symlink("/sbin/rc", "/tmp/ppp/ip-pre-up");
#ifdef TCONFIG_IPV6
	symlink("/sbin/rc", "/tmp/ppp/ipv6-up");
	symlink("/sbin/rc", "/tmp/ppp/ipv6-down");
#endif
	demand = nvram_get_int(strcat_r(prefix, "_ppp_demand", tmp));

	/* Generate options file */
	char ppp_optfile[256];
	memset(ppp_optfile, 0, 256);
	sprintf(ppp_optfile, "/tmp/ppp/%s_options", prefix);
	if ((fp = fopen(ppp_optfile, "w")) == NULL) {
		logerr(__FUNCTION__, __LINE__, ppp_optfile);
		return -1;
	}

#ifdef TCONFIG_USB
	if (wan_proto != WP_PPP3G)
#endif
		fprintf(fp, "user \"%s\"\n"		/* Don't rely on pap/chap secrets (useless) */
		            "password \"%s\"\n"		/* Don't rely on pap/chap secrets (useless) */
		            "lcp-echo-adaptive\n",	/* Suppress LCP echo-requests if traffic was received */
		            nvram_safe_get(strcat_r(prefix, "_ppp_username", tmp)),
		            nvram_safe_get(strcat_r(prefix, "_ppp_passwd", tmp)));

	fprintf(fp, "unit %d\n"			/* unit as WAN NUM, let's try to have persistent names */
	            "linkname %s\n"		/* link name for WAN ID */
	            "defaultroute\n"		/* Add a default route to the system routing tables, using the peer as the gateway */
	            "usepeerdns\n"		/* Ask the peer for up to 2 DNS server addresses */
	            "default-asyncmap\n"	/* Disable  asyncmap  negotiation */
	            "novj\n"			/* Disable Van Jacobson style TCP/IP header compression */
	            "nobsdcomp\n"		/* Disable BSD-Compress  compression */
	            "nodeflate\n"		/* Disable Deflate compression */
	            "noauth\n"			/* Do not authenticate peer */
	            "refuse-eap\n"		/* Do not use eap */
	            "maxfail 0\n"		/* Never give up */
	            "lcp-echo-interval %d\n"	/* Interval between LCP echo-requests */
	            "lcp-echo-failure %d\n"	/* Tolerance to unanswered echo-requests */
	            "%s",			/* Debug */
	            num,
	            prefix,
	            nvram_get_int(strcat_r(prefix, "_pppoe_lei", tmp)) ? : 10,
	            nvram_get_int(strcat_r(prefix, "_pppoe_lef", tmp)) ? : 5,
	            nvram_get_int("debug_ppp") ? "debug\n" : "");

#ifdef TCONFIG_USB
	if (wan_proto == WP_PPP3G && nvram_match(strcat_r(prefix, "_modem_dev", tmp), "ttyACM0")) {
		/* Don't write nopcomp and noaccomp options */
	}
	else
#endif
		fprintf(fp, "nopcomp\n"		/* Disable protocol field compression */
		            "noaccomp\n");	/* Disable Address/Control compression */

	if (wan_proto != WP_L2TP)
		fprintf(fp, "persist\n"
		            "holdoff %d\n",
		            demand ? 30 : (nvram_get_int(strcat_r(prefix, "_ppp_redialperiod", tmp)) ? : 30));

	switch (wan_proto) {
	case WP_PPTP:
		fprintf(fp, "plugin pptp.so\n"
		            "pptp_server %s\n"
		            "nomppe-stateful\n"
		            "require-mschap-v2\n"
		            "noauth\n" /* No authenticate peer (i dunno why it doesn't apply from shared params) */
		            "mtu %d\n",
		            nvram_safe_get(strcat_r(prefix, "_pptp_server_ip", tmp)),
		            (nvram_get_int(strcat_r(prefix, "_mtu_enable", tmp)) ? nvram_get_int(strcat_r(prefix, "_wan_mtu", tmp)) : 1400));
		break;
	case WP_PPPOE:
		fprintf(fp, "plugin rp-pppoe.so\n"
		            "nomppe nomppc\n"
		            "nic-%s\n"
		            "mru %d mtu %d\n",
		            nvram_safe_get(strcat_r(prefix, "_ifname", tmp)),
		            nvram_get_int(strcat_r(prefix, "_mtu", tmp)),
		            nvram_get_int(strcat_r(prefix, "_mtu", tmp)));
		if ((p = nvram_safe_get(strcat_r(prefix, "_ppp_service", tmp))) && (*p))
			fprintf(fp, "rp_pppoe_service '%s'\n", p);

		if ((p = nvram_safe_get(strcat_r(prefix, "_ppp_ac", tmp))) && (*p))
			fprintf(fp, "rp_pppoe_ac '%s'\n", p);

		if (nvram_match(strcat_r(prefix, "_ppp_mlppp", tmp), "1"))
			fprintf(fp, "mp\n");

		break;
#ifdef TCONFIG_USB
	case WP_PPP3G: ;
		/* Generate chat file */
		char ppp3g_chatfile[256];
		memset(ppp3g_chatfile, 0, 256);
		sprintf(ppp3g_chatfile, "/tmp/ppp/%s_connect.chat", prefix);

		if ((cfp = fopen(ppp3g_chatfile, "w")) == NULL) {
			logerr(__FUNCTION__, __LINE__, ppp3g_chatfile);
			return -1;
		}
		fprintf(cfp, "ABORT \"NO CARRIER\"\n"
		             "ABORT \"NO DIALTONE\"\n"
		             "ABORT \"NO DIAL TONE\"\n"
		             "ABORT \"NO ERROR\"\n"
		             "ABORT \"NO ANSWER\"\n"
		             "ABORT \"BUSY\"\n"
		             "ABORT \"VOICE\"\n"
		             "ABORT \"DELAYED\"\n"
		             "REPORT CONNECT\n"
		             "TIMEOUT 10\n"
		             "\"\" \"AT\"\n"
		             "OK \"AT&FE0V1X1&D2&C1S0=0\"\n"
		             "OK \"AT\"\n"
		             "OK \"ATS0=0\"\n"
		             "OK \"AT\"\n"
		             "OK \"AT&FE0V1X1&D2&C1S0=0\"\n"
		             "OK \"AT\"\n");

		/* Only send the AT+CGDCONT (define PDP context) command to set
		 * the APN if modem_apn is defined and non-empty.  Some ISPs
		 * (ex. BSNL EVDO in India) don't need this (the modem returns
		 * ERROR if issued).
		 */
		if ((p = nvram_safe_get(strcat_r(prefix, "_modem_apn", tmp))) && (*p))
			fprintf(cfp, "OK 'AT+CGDCONT=1,\"IP\",\"%s\"'\n", p);

		fprintf(cfp, "OK \"ATDT%s\"\n"
		             "TIMEOUT 30\n"
		             "CONNECT \\c\n",
		             nvram_safe_get(strcat_r(prefix, "_modem_init", tmp)));
		fclose(cfp);

		/* Add to options file */
		fprintf(fp, "%s\n"
		            "460800\n"
		            "connect \"/usr/sbin/chat %s -t 30 -f %s\"\n"
		            "noipdefault\n"
		            "lock\n"	/* Lock the modem device, to avoid another process trying to use it */
		            "crtscts\n"	/* Use hardware flow-control between the computer and the modem, to avoid data loss */
		            "modem\n"	/* Use the modem control lines */
		            "ipcp-accept-local\n",
		            nvram_safe_get(strcat_r(prefix, "_modem_dev", tmp)),
		            nvram_get_int("debug_ppp") ? "-v" : "-V",
		            ppp3g_chatfile);

		if (strlen(nvram_get(strcat_r(prefix, "_ppp_username", tmp))) > 0)
			fprintf(fp, "user \"%s\"\n", nvram_get(strcat_r(prefix, "_ppp_username", tmp)));
		if (strlen(nvram_get(strcat_r(prefix, "_ppp_passwd", tmp))) > 0)
			fprintf(fp, "password \"%s\"\n", nvram_get(strcat_r(prefix, "_ppp_passwd", tmp)));

		/* Clear old gateway */
		if (strlen(nvram_safe_get(strcat_r(prefix, "_gateway", tmp))) > 0)
			nvram_set(strcat_r(prefix, "_gateway", tmp), "");

		/* Detect 3G Modem */
		if (!nvram_get_int("g_upgrade") && !nvram_get_int("g_reboot"))
			xstart("switch3g", prefix);
		break;
#endif
	case WP_L2TP:
		fprintf(fp, "nomppe nomppc\n");		/* Disable MPPE, MPPC */
		if (nvram_get_int(strcat_r(prefix, "_mtu_enable", tmp)))
			fprintf(fp, "mtu %d\n", nvram_get_int(strcat_r(prefix, "_mtu", tmp)));
		break;
	}

	if (demand) {
		/* Demand mode */
		if (wan_proto != WP_L2TP)
			fprintf(fp, "demand\n"		/* Dial on demand */
			            "idle %d\n",	/* Disconnect if the link is idle for n seconds */
			            nvram_get_int(strcat_r(prefix, "_ppp_idletime", tmp)) * 60);

		fprintf(fp, "ipcp-accept-remote\n"	/* Force to accept the peer's idea of its (remote) IP address */
		            "ipcp-accept-local\n"	/* Force to accept the peer's idea of local IP address */
		            "noipdefault\n"		/* Disables the default behaviour when no local IP address is specified (request from peer) */
		            "ktune\n");			/* Set /proc/sys/net/ipv4/ip_dynaddr to 1 in demand mode if the local address changes */
	}

#ifdef TCONFIG_IPV6
	/* start/add IPv6 BUT only for "wan" (no multiwan support) */
	if (strcmp(prefix, "wan") == 0) { /* check for "wan" prefix */
		switch (get_ipv6_service()) {
		case IPV6_NATIVE:
		case IPV6_NATIVE_DHCP:
			fprintf(fp, "+ipv6\n");
			break;
		}
	}
#endif
	/* User specific options */
	fprintf(fp, "%s\n", nvram_safe_get(strcat_r(prefix, "_ppp_custom", tmp)));
	fclose(fp);

	return 0;
}

static void stop_ppp(char *prefix)
{
	char buffer[64];

	memset(buffer, 0, 64);
	sprintf(buffer, "/tmp/ppp/%s_link", prefix);
	unlink(buffer);
	/* Fix switching wan type from UI error (stale options file) */
	memset(buffer, 0, 64);
	sprintf(buffer, "/tmp/ppp/%s_options", prefix);
	unlink(buffer);

	memset(buffer, 0, 64);
	sprintf(buffer, "pppd%s", prefix);

	/* Race condition on start_pppoe in ip-up on boot, primary pp(poe/tp) wan will not reach start_wan_done on secondary ppp(oe/3g) start */
	//killall_tk_period_wait("ip-up", 50);
	//killall_tk_period_wait("ip-pre-up", 50);
	//killall_tk_period_wait("ip-down", 50);
#ifdef TCONFIG_IPV6
	//killall_tk_period_wait("ipv6-up", 50);
	//killall_tk_period_wait("ipv6-down", 50);
#endif
	/* FIXME: find a proper way to stop daemon */
	if (get_wanx_proto("wan") != WP_L2TP
	    && get_wanx_proto("wan2") != WP_L2TP
#ifdef TCONFIG_MULTIWAN
	    && get_wanx_proto("wan3") != WP_L2TP
	    && get_wanx_proto("wan4") != WP_L2TP
#endif
	)
		killall_tk_period_wait("xl2tpd", 50);

	//kill(nvram_get_int(strcat_r(prefix, "_pppd_pid", tmp)),1);
	killall_tk_period_wait((char *)buffer, 50);

	/* Don't kill other wans listeners, only this wan one
	 * its PID can be found in /var/run/listen-wan%d.pid
	 */
	//killall_tk_period_wait("listen", 50);

	/* WAN LED control */
	wan_led_off(prefix); /* LED OFF? */
}

static void run_pppd(char *prefix)
{
	char tmp[100];
	char pppd_path[256];
	char ppp_optfile[256];

	memset(pppd_path, 0, 256);
	sprintf(pppd_path, "/tmp/ppp/pppd%s", prefix);

	memset(ppp_optfile, 0, 256);
	sprintf(ppp_optfile, "/tmp/ppp/%s_options", prefix);

	symlink("/usr/sbin/pppd", pppd_path);
	eval(pppd_path, "file", ppp_optfile);

	if (nvram_get_int(strcat_r(prefix, "_ppp_demand", tmp))) { /* Demand mode */
		/*
		 * Fixed issue id 7887(or 7787):
		 * When DUT is PPTP Connect on Demand mode, it couldn't be trigger from LAN.
		*/
		stop_dnsmasq();
		dns_to_resolv();
		start_dnsmasq();

		/* Trigger Connect On Demand if user ping pptp server IP or WAN IP */
		eval("listen", nvram_safe_get("lan_ifname"), prefix);
	}
	else { /* Keepalive mode */
		start_redial(prefix);
	}
}

inline void stop_pptp(char *prefix)
{
	stop_ppp(prefix);
}

void start_pptp(char *prefix)
{
	int num = 0; /* wan */

	if (!strcmp(prefix,"wan2")) num = 1;
#ifdef TCONFIG_MULTIWAN
	else if (!strcmp(prefix,"wan3")) num = 2;
	else if (!strcmp(prefix,"wan4")) num = 3;
#endif

	if (!using_dhcpc(prefix))
		stop_dhcpc(prefix);

	stop_pptp(prefix);

	if (config_pppd(WP_PPTP, num, prefix) != 0)
		return;

	run_pppd(prefix);
}

/* Used by PPTP/L2TP mainly to setup host routes to DNS and access server */
void preset_wan(char *ifname, char *gw, char *netmask, char *prefix)
{
	struct in_addr ipaddr;
	char tmp[100];
	char word[100], *next;
	int i, ret;
	int proto;
	int mwan_num;

	mwan_num = nvram_get_int("mwan_num");
	if ((mwan_num < 1) || (mwan_num > MWAN_MAX))
		mwan_num = 1;

	if (mwan_num == 1) {
		/* Delete default route */
		route_del(ifname, 0, NULL, NULL, NULL);

		/* Set default route to gateway if specified */
		i = 5;
		while ((ret = route_add(ifname, 1, "0.0.0.0", gw, "0.0.0.0") != 0) && (i--)) {
			sleep(1);
		}
	}

	/* Try adding a host route to gateway first */
	route_add(ifname, 0, gw, NULL, "255.255.255.255");

	/* Add routes to dns servers as well for demand ppp to work */
	in_addr_t mask = inet_addr(netmask);
	foreach(word, nvram_safe_get(strcat_r(prefix, "_get_dns", tmp)), next) {
		if ((inet_addr(word) & mask) != (inet_addr(nvram_safe_get(strcat_r(prefix, "_ipaddr", tmp))) & mask))
			route_add(ifname, 0, word, gw, "255.255.255.255");
	}

	/* Add routes to PPTP/L2TP servers for load-balanced WAN setups will work.
	 * Without it, if there is other WAN active, xl2tpd / pptp tries to connect
	 * to l2tp/pptp server not via WAN's gateway, but setup route via existing
	 * Internet connection (other WAN gw or default) so never reached server.
	 * NB! l2/pptp_server_ip can be hostname also, so route will not be added
	*/
	proto = get_wanx_proto(prefix);
	if (proto == WP_PPTP) {
		if (inet_pton(AF_INET, nvram_safe_get(strcat_r(prefix, "_pptp_server_ip", tmp)), &(ipaddr.s_addr)))
			route_add(ifname, 0, nvram_safe_get(strcat_r(prefix, "_pptp_server_ip", tmp)), gw, "255.255.255.255");
	}
	if (proto == WP_L2TP) {
		if (inet_pton(AF_INET, nvram_safe_get(strcat_r(prefix, "_l2tp_server_ip", tmp)), &(ipaddr.s_addr)))
			route_add(ifname, 0, nvram_safe_get(strcat_r(prefix, "_l2tp_server_ip", tmp)), gw, "255.255.255.255");
	}

	/* FIXME! why only on primary WAN?
	 * All PPTP / L2TP need working DNS
	 * to resolve access server IP
	 */
	//if (!strcmp(prefix, "wan")) {
		stop_dnsmasq();
		dns_to_resolv();
		start_dnsmasq();
		sleep(1);
	/* issue with call from dhcp.c bound (it didn't do start_ppxxx on startup) on primary WAN! so disabled here */
		//start_firewall();
	//}
}

/* Get the IP, subnetmask, gaeway from WAN ppp interface in demand mode and set nvram */
static void start_tmp_ppp(int num, char *ifname, char *prefix)
{
	struct ifreq ifr;
	char tmp[100];
	int timeout;
	int s;

	/* wtf? only on primary wan? */
	//if (num != 0) return;

	/* Wait for ppp0 to be created */
	timeout = 15;
	while ((ifconfig(ifname, IFUP, NULL, NULL) != 0) && (timeout-- > 0)) {
		sleep(1);
	}

	if ((s = socket(AF_INET, SOCK_RAW, IPPROTO_RAW)) < 0)
		return;

	strlcpy(ifr.ifr_name, ifname, IFNAMSIZ);

	/* Set temporary IP address */
	timeout = 3;
	while (ioctl(s, SIOCGIFADDR, &ifr) && timeout--) {
		sleep(1);
	}

	if (!using_dhcpc(prefix)) {
		nvram_set(strcat_r(prefix, "_ipaddr", tmp), inet_ntoa(sin_addr(&(ifr.ifr_addr))));
		nvram_set(strcat_r(prefix, "_netmask", tmp), "255.255.255.255");
	}
	else	/* Don't overwrite netmask and set proper IP for PPP+DHCP wan */
		nvram_set(strcat_r(prefix, "_ppp_get_ip", tmp), inet_ntoa(sin_addr(&(ifr.ifr_addr))));

	/* Get temporary P-t-P address */
	timeout = 3;
	while (ioctl(s, SIOCGIFDSTADDR, &ifr) && timeout--) {
		sleep(1);
	}
	/* UI and listen / connect scripts se wan_gateway() which return gateway_get */
	if (!using_dhcpc(prefix)) {
		nvram_set(strcat_r(prefix, "_gateway", tmp), inet_ntoa(sin_addr(&(ifr.ifr_dstaddr))));
		nvram_set(strcat_r(prefix, "_gateway_get", tmp), inet_ntoa(sin_addr(&(ifr.ifr_dstaddr))));
	}
	else
		nvram_set(strcat_r(prefix, "_gateway_get", tmp), inet_ntoa(sin_addr(&(ifr.ifr_dstaddr))));

	close(s);

	start_wan_done(ifname, prefix);
}

void start_pppoe(int num, char *prefix)
{
	char pppname[8] = "";
	char tmp[100];

	if (num < 0 || num > 3)
		return;

	stop_pppoe(prefix);

#ifdef TCONFIG_USB
	if (nvram_match(strcat_r(prefix, "_proto", tmp), "ppp3g")) {
		if (nvram_match("usb_3g", "1")) {
			if (config_pppd(WP_PPP3G, num, prefix) != 0)
				return;
		}
		else /* USB support has been disabled */
			return;
	}
	else {
#endif
		if (config_pppd(WP_PPPOE, num, prefix) != 0)
			return;
#ifdef TCONFIG_USB
	}
#endif
	run_pppd(prefix);

	snprintf(pppname, sizeof(pppname), "ppp%d", num); /* Control by "unit" ppp options param */

	if (strcmp(pppname, "none") && strcmp(pppname, "")) { /* Got interface */
		/* Run in demand mode only if interface exists and not for L2TP wan */
		if (nvram_get_int(strcat_r(prefix, "_ppp_demand", tmp)) && !nvram_match(strcat_r(prefix, "_proto", tmp), "l2tp"))
			start_tmp_ppp(num, pppname, prefix);
		else
			ifconfig(pppname, IFUP, NULL, NULL);
	}
}

void stop_pppoe(char *prefix)
{
	stop_ppp(prefix);
}

static int config_l2tp(void) /* shared xl2tpd.conf for all WAN */
{

	FILE *fp;
	int i;
	int demand;
	char xl2tp_file[256];
	char tmp[100];
	const char *names[] = {
		"wan",
		"wan2",
#ifdef TCONFIG_MULTIWAN
		"wan3",
		"wan4",
#endif
		NULL
	};

	/* Generate XL2TPD configuration file */
	memset(xl2tp_file, 0, 256);
	sprintf(xl2tp_file, "/etc/xl2tpd.conf");
 	if ((fp = fopen(xl2tp_file, "w")) == NULL) {
		logerr(__FUNCTION__, __LINE__, xl2tp_file);
 		return -1;
	}
	/* GLOBAL */
	fprintf(fp, "[global]\n"
	            "access control = no\n"
	            "port = 1701\n"
	            "debug avp = no\n"
	            "debug network = no\n"
	            "debug packet = no\n"
	            "debug state = no\n"
	            "debug tunnel = no\n"
	            "\n");
	/* LACS */
	for (i = 0; names[i] != NULL; ++i) {
		if (!strcmp(nvram_safe_get(strcat_r(names[i], "_proto", tmp)), "l2tp")) {
			demand = nvram_get_int(strcat_r(names[i], "_ppp_demand", tmp));
			char ppp_optfile[256];
			memset(ppp_optfile, 0, 256);
			sprintf(ppp_optfile, "/tmp/ppp/%s_options", names[i]);
			fprintf(fp, "[lac %s]\n"
			            "lns = %s\n"
			            "bps = 1000000000\n" /* 1 gbit to both tx/rx */
			            "tunnel rws = 8\n"
			            "pppoptfile = %s\n"
			            "autodial = no\n"
			            "redial = %s\n"
			            "max redials = 32767\n"
			            "redial timeout = %d\n"
			            "ppp debug = %s\n"
			            "%s\n",
			            names[i], /* LAC name */
			            nvram_safe_get(strcat_r(names[i], "_l2tp_server_ip", tmp)),
			            ppp_optfile,
			            demand ? "no" : "yes",
			            nvram_get_int(strcat_r(names[i], "_ppp_redialperiod", tmp)) ? : 30,
			            nvram_get_int("debug_ppp") ? "yes" : "no",
			            nvram_safe_get(strcat_r(names[i], "_xl2tpd_custom", tmp)));

			memset(xl2tp_file, 0, 256);
			sprintf(xl2tp_file, "/etc/%s_xl2tpd.custom", names[i]);
			fappend(fp, xl2tp_file);
		}
	}

	fclose(fp);

	return 0;
}

inline void stop_l2tp(char *prefix)
{
	char l2tp_file[64];
	char dconnects[64];

	memset(l2tp_file, 0, 64);
	sprintf(l2tp_file, "/var/run/l2tp-control");
	memset(dconnects, 0, 64);
	sprintf(dconnects, "d %s", prefix);
	f_write_string(l2tp_file, dconnects, 0, 0);	/* Disconnect current session first */
	sleep(1);					/* Wait ip-down scripts to finish graceful */
	stop_ppp(prefix);				/* Unlink ppp files in /tmp/ppp (used by mwan.c) and stop daemon */
}

void start_l2tp(char *prefix)
{
	char tmp[100];
	int demand;

	if (!using_dhcpc(prefix)) /* As for PPTP */
		stop_dhcpc(prefix);

	//stop_l2tp(prefix);

	if (config_l2tp() != 0) /* Generate L2TP daemon config */
		return;

	int num = 0; /* wan */
	if (!strcmp(prefix,"wan2")) num = 1;
#ifdef TCONFIG_MULTIWAN
	else if (!strcmp(prefix,"wan3")) num = 2;
	else if (!strcmp(prefix,"wan4")) num = 3;
#endif
	if (config_pppd(WP_L2TP, num, prefix) != 0) /* ppp options */
		return;

	demand = nvram_get_int(strcat_r(prefix, "_ppp_demand", tmp));

	enable_ip_forward();

	eval("xl2tpd", "-c", "/etc/xl2tpd.conf");

	if (demand) /* Listen LAN for ping to l2tp server IP, see listen.c */
		eval("listen", nvram_safe_get("lan_ifname"), prefix);
	else
		force_to_dial(prefix); /* Force connect via l2tp-control */
}

char *wan_gateway(char *prefix)
{
	char tmp[100];
	char *gw = nvram_safe_get(strcat_r(prefix, "_gateway_get", tmp));

	if ((*gw == 0) || (strcmp(gw, "0.0.0.0") == 0))
		gw = nvram_safe_get(strcat_r(prefix, "_gateway", tmp));

	return gw;
}

/* Trigger connect on demand */
void force_to_dial(char *prefix)
{
	char l2tp_file[64];
	char connects[64];
	char dgw[64];

	memset(dgw, 0, 64);
	sprintf(dgw, "10.112.112.%d", 111 + get_wan_unit(prefix)); /* 10.112.112.112-115 for ppp0-3 */
	sleep(1);

	switch (get_wanx_proto(prefix)) {
	case WP_L2TP:
		memset(l2tp_file, 0, 64);
		sprintf(l2tp_file, "/var/run/l2tp-control");
		memset(connects, 0, 64);
		sprintf(connects, "c %s", prefix);
		f_write_string(l2tp_file, connects, 0, 0);
		break;
	case WP_PPTP:
		eval("ping", "-c", "2", dgw);
		break;
	case WP_DISABLED:
	case WP_STATIC:
		break;
	default:
		eval("ping", "-c", "2", (strcmp(wan_gateway(prefix),"") != 0 && strcmp(wan_gateway(prefix),"0.0.0.0") != 0) ? wan_gateway(prefix) : dgw);
		break;
	}
}

static void _do_wan_routes(char *ifname, char *nvname, int metric, int add)
{
	char *routes, *tmp;
	int bits;
	struct in_addr mask;
	char netmask[16];

	/* IP[/MASK] ROUTER IP2[/MASK2] ROUTER2 ... */
	tmp = routes = strdup(nvram_safe_get(nvname));
	while (tmp && *tmp) {
		char *ipaddr, *gateway, *nmask;

		ipaddr = nmask = strsep(&tmp, " ");
		strcpy(netmask, "255.255.255.255");

		if (nmask) {
			ipaddr = strsep(&nmask, "/");
			if (nmask && *nmask) {
				bits = strtol(nmask, &nmask, 10);
				if (bits >= 1 && bits <= 32) {
					mask.s_addr = htonl(0xffffffff << (32 - bits));
					strcpy(netmask, inet_ntoa(mask));
				}
			}
		}
		gateway = strsep(&tmp, " ");

		if (gateway && *gateway) {
			if (add)
				route_add(ifname, metric, ipaddr, gateway, netmask);
			else
				route_del(ifname, metric, ipaddr, gateway, netmask);
		}
	}
	free(routes);
}

void do_wan_routes(char *ifname, int metric, int add, char *prefix)
{
	if (nvram_get_int("dhcp_routes")) {
		char tmp[100];
		/* Static Routes:             IP ROUTER IP2 ROUTER2 ... */
		/* Classless Static Routes:   IP/MASK ROUTER IP2/MASK2 ROUTER2 ... */
		_do_wan_routes(ifname, strcat_r(prefix, "_routes1", tmp), metric, add);
		_do_wan_routes(ifname, strcat_r(prefix, "_routes2", tmp), metric, add);
	}
}

void create_wanx_mac(char *prefix, int mac_plus)
{
	char buffer[32] = { 0 };
	char nvtmp[16];

	snprintf(buffer, sizeof(buffer), "%s", nvram_safe_get("lan_hwaddr"));	/* get LAN MAC address (usually et0) */
	inc_mac(buffer, mac_plus);						/* MAC + value for wanX */
	nvram_set(strcat_r(prefix, "_mac", nvtmp), buffer);			/* save it to nvram */
	logmsg(LOG_INFO, "Create and save wanX mac address - WAN: %s - Address: %s", prefix, buffer);
}

void store_wan_if_to_nvram(char *prefix)
{
	char *p = NULL;
	char *w = NULL;
	char buf[64];
	int wan_unit;
	int vid;
	int vid_map;
#if !defined(CONFIG_BCMWL6) && !defined(TCONFIG_BLINK) /* only mips RT branch */
	int vlan0tag = nvram_get_int("vlan0tag");
#endif
	char tmp[64];
	char *nvvar = NULL;

	wan_unit = get_wan_unit(prefix);

	if (strcmp(nvram_safe_get(strcat_r(prefix, "_sta", tmp)), "") == 0) {
		/* vlan ID mapping */
		p = nvram_safe_get(strcat_r(prefix, "_ifnameX", tmp));
		if (sscanf(p, "vlan%d", &vid) == 1) {
			snprintf(buf, sizeof(buf), "vlan%dvid", vid);
			vid_map = nvram_get_int(buf);
			if ((vid_map < 1) || (vid_map > 4094)) {
#if !defined(CONFIG_BCMWL6) && !defined(TCONFIG_BLINK) /* only mips RT branch */
				vid_map = vlan0tag | vid;
#else
				vid_map = vid;
#endif
			}
			snprintf(buf, sizeof(buf), "vlan%d", vid_map);
			p = buf;
		}
		/* Set wan mac on vlan (but not for wireless client) */
		nvvar = nvram_get(strcat_r(prefix, "_mac", tmp));

		/* check if we have a wanX mac? FT user could/can define one ... if not, create it (default);
		 * Increase mac address and keep distance to et0 mac --> We need it for working VLAN setups! (PPPoE);
		 * Set wanX macs after wl VIFs
		 * wan  --> et0 mac +16
		 * wan2 --> et0 mac +17
		 * wan3 --> et0 mac +18
		 * wan4 --> et0 mac +19
		 * Info: sync GUI def-mac - see www/advanced-mac.asp
		 */
		if ((nvvar == NULL) ||
		    (nvvar && !strlen(nvvar))) {
			create_wanx_mac(prefix, (wan_unit + 15));
		}
		set_mac(p, tmp, (wan_unit + 15));
	}
	else { /* Wireless client as wan */
		w = nvram_safe_get(strcat_r(prefix, "_sta", tmp));
		p = nvram_safe_get(strcat_r(w, "_ifname", tmp));
	}
	/* Store interface name to nvram */
	nvram_set(strcat_r(prefix, "_ifname", tmp), p);
	nvram_set(strcat_r(prefix, "_ifnames", tmp), p);
}

void start_wan_if(char *prefix)
{
	int wan_proto;
	char *wan_ifname;
	char *nvp;
	struct ifreq ifr;
	int sd;
	int max;
	int mtu;
	char buf[128];
	char tmp[128];
	int jumbo_enable = 0;
	struct vlan_ioctl_args ifv;

	do_connect_file(1, prefix);

	store_wan_if_to_nvram(prefix);

	/* Setup WAN interface name */
	wan_ifname = nvram_safe_get(strcat_r(prefix, "_ifname", tmp));
	if (wan_ifname[0] == 0) {
		wan_ifname = "none";
		nvram_set(strcat_r(prefix, "_ifname", tmp), wan_ifname);
	}

	if (strcmp(wan_ifname, "none") == 0) {
		nvram_set(strcat_r(prefix, "_proto", tmp), "disabled");
		logmsg(LOG_WARNING, "%s ifname is NONE, please check you vlan settings!", prefix);
	}

	/* Defined in shared.h, misc.c */
	wan_proto = get_wanx_proto(prefix);

	/* Set the default gateway for WAN interface */
	nvram_set(strcat_r(prefix, "_gateway_get", tmp), nvram_safe_get(strcat_r(prefix, "_gateway", tmp)));

	if (wan_proto == WP_DISABLED) {
		start_wan_done(wan_ifname, prefix);
		return;
	}

	if ((sd = socket(AF_INET, SOCK_RAW, IPPROTO_RAW)) < 0) {
		logerr(__FUNCTION__, __LINE__, "socket");
		return;
	}

	jumbo_enable = nvram_get_int("jumbo_frame_enable");

	/* MTU */
	switch (wan_proto) {
	case WP_PPPOE:
	case WP_PPP3G:
		max = 1492;
		break;
	case WP_PPTP:
	case WP_L2TP:
		max = 1460;
		break;
	default:
		max = 1500;
		break;
	}
	if (nvram_match(strcat_r(prefix, "_mtu_enable", tmp), "0"))
		mtu = max;
	else {
		/* KDB If we've big fat frames enabled then we *CAN* break the
		 * max MTU on PPP link
		 */
		mtu = nvram_get_int(strcat_r(prefix, "_mtu", tmp));
		if (!jumbo_enable && (mtu > max))
			mtu = max;
		else if (mtu < 576)
			mtu = 576;
	}
	memset(buf, 0, 128);
	sprintf(buf, "%d", mtu);
	nvram_set(strcat_r(prefix, "_mtu", tmp), buf);
	nvram_set(strcat_r(prefix, "_run_mtu", tmp), buf);

	/* Don't set the MTU on the port for PPP connections, it will be set on the link instead */
	if ((wan_proto == WP_PPTP) || (wan_proto == WP_L2TP) || (wan_proto == WP_PPPOE) || (wan_proto == WP_PPP3G))
		mtu = 0;

	/* Bring wan interface UP */
	_ifconfig(wan_ifname, IFUP, NULL, NULL, NULL, mtu);

	/* Try to increase WAN interface MTU to allow PPPoE MTU/MRU 1500 (default 1492, with 8 byte overhead) */
	mtu = nvram_get_int(strcat_r(prefix, "_mtu", tmp)); /* get mtu again */
	if ((jumbo_enable) && (mtu == 1500) && (wan_proto == WP_PPPOE)) {
		logmsg(LOG_INFO, "Try to increase WAN MTU up to 1500 for ISPs that support RFC 4638");

		/* set parent device --> should be "eth0" */
		strncpy(ifv.device1, wan_ifname, IFNAMSIZ);
		ifv.cmd = GET_VLAN_REALDEV_NAME_CMD;
		if (ioctl(sd, SIOCGIFVLAN, &ifv) >= 0) {
			strncpy(ifr.ifr_name, ifv.u.device2, IFNAMSIZ);
			ifr.ifr_mtu = mtu + 8;

			if (ioctl(sd, SIOCSIFMTU, &ifr)) {
				logerr(__FUNCTION__, __LINE__, wan_ifname);
				logmsg(LOG_INFO, "Error setting MTU on %s to %d", ifr.ifr_name, ifr.ifr_mtu);
			}
			else
				logmsg(LOG_INFO, "Increase MTU on %s to %d", ifr.ifr_name, ifr.ifr_mtu);
		}

		/* set wan device --> for example "vlan7" */
		strncpy(ifr.ifr_name, wan_ifname, IFNAMSIZ);
		ifr.ifr_mtu = mtu + 8;
		if (ioctl(sd, SIOCSIFMTU, &ifr)) {
			logerr(__FUNCTION__, __LINE__, wan_ifname);
			logmsg(LOG_INFO, "Error setting MTU on %s to %d", ifr.ifr_name, ifr.ifr_mtu);
		}
		else
			logmsg(LOG_INFO, "Increase MTU on %s to %d", ifr.ifr_name, ifr.ifr_mtu);

	}

	switch (wan_proto) {
	case WP_PPPOE:
	case WP_PPP3G:
		if (wan_proto == WP_PPPOE && using_dhcpc(prefix)) { /* PPPoE with DHCP MAN */
			stop_dhcpc(prefix);
			start_dhcpc(prefix);
		}
		else if (!strcmp(prefix, "wan"))  start_pppoe(PPPOEWAN, prefix);
		else if (!strcmp(prefix, "wan2")) start_pppoe(PPPOEWAN2, prefix);
#ifdef TCONFIG_MULTIWAN
		else if (!strcmp(prefix, "wan3")) start_pppoe(PPPOEWAN3, prefix);
		else if (!strcmp(prefix, "wan4")) start_pppoe(PPPOEWAN4, prefix);
#endif
		break;
	case WP_DHCP:
	case WP_LTE:
	case WP_L2TP:
	case WP_PPTP:
		if (wan_proto == WP_LTE) {
			/* Prepare LTE modem */
			if (!nvram_get_int("g_upgrade") && !nvram_get_int("g_reboot"))
				xstart("switch4g", prefix);
		}
		else if (using_dhcpc(prefix)) {
			stop_dhcpc(prefix);
			start_dhcpc(prefix);
		}
		else if (wan_proto != WP_DHCP && wan_proto != WP_LTE) {
			/* Set wan interface IP/mask */
			ifconfig(wan_ifname, IFUP, "0.0.0.0", NULL);
			ifconfig(wan_ifname, IFUP, nvram_safe_get(strcat_r(prefix, "_ipaddr", tmp)), nvram_safe_get(strcat_r(prefix, "_netmask", tmp)));

			nvp = nvram_safe_get(strcat_r(prefix, "_gateway", tmp));
			if ((strcmp(nvp, "0.0.0.0") != 0) && (*nvp))
				preset_wan(wan_ifname, nvp, nvram_safe_get(strcat_r(prefix, "_netmask", tmp)), prefix);

			switch (wan_proto) {
			case WP_PPTP:
				start_pptp(prefix);
				break;
			case WP_L2TP:
				start_l2tp(prefix);
				break;
			}
		}
		break;
	default: /* Static */
		nvram_set(strcat_r(prefix, "_iface", tmp), wan_ifname);
		ifconfig(wan_ifname, IFUP, nvram_safe_get(strcat_r(prefix, "_ipaddr", tmp)), nvram_safe_get(strcat_r(prefix, "_netmask", tmp)));
		int r = 10;
		while ((!check_wanup(prefix)) && (r-- > 0)) {
			sleep(1);
		}
		start_wan_done(wan_ifname, prefix);
		break;
	}

	/* Get current WAN hardware address */
	strlcpy(ifr.ifr_name, wan_ifname, IFNAMSIZ);
	if (ioctl(sd, SIOCGIFHWADDR, &ifr) == 0)
		nvram_set(strcat_r(prefix, "_hwaddr", tmp), ether_etoa((const unsigned char *) ifr.ifr_hwaddr.sa_data, buf));

	close(sd);

	/* Set initial QoS mode again now that WAN port is ready. */
	set_et_qos_mode();
}

void start_wan(void)
{
	int mwan_num;
	int wan_unit;
	char prefix[] = "wanXX";

	mwan_num = nvram_get_int("mwan_num");
	if ((mwan_num < 1) || (mwan_num > MWAN_MAX))
		mwan_num = 1;

	logmsg(LOG_INFO, "MultiWAN: MWAN is %d (max %d)", mwan_num, MWAN_MAX);

	for (wan_unit = 1; wan_unit <= mwan_num; ++wan_unit) {
		get_wan_prefix(wan_unit, prefix);
		start_wan_if(prefix);
		logmsg(LOG_DEBUG, "*** MultiWAN: %s: (unit: %d), prefix = %s", __FUNCTION__, wan_unit, prefix);
	}

	start_firewall();
	set_host_domain_name();

	/* only start mwanroute if it's not already up! */
	if (pidof("mwanroute") < 0) {
		logmsg(LOG_DEBUG, "*** %s: mwanroute not found, launch process", __FUNCTION__);
		xstart("mwanroute");
	}

	if (nvram_get_int("mwan_cktime") > 0) {
		logmsg(LOG_DEBUG, "*** %s: adding watchdog job", __FUNCTION__);
		xstart("watchdog", "add");
	}

	led(LED_DIAG, LED_OFF);
	led(LED_DMZ, nvram_match("dmz_enable", "1"));
}

#ifdef TCONFIG_IPV6
void start_wan6(const char *wan_ifname)
{
	struct in_addr addr4;
	struct in6_addr addr;
	static char addr6[INET6_ADDRSTRLEN];

	int service = get_ipv6_service();

	if (service != IPV6_DISABLED) {
		/* Check if "ipv6_accept_ra" (bit 0) for wan is enabled (via GUI, basic-ipv6.asp) */
		if ((nvram_get_int("ipv6_accept_ra") & 0x01) != 0)
			accept_ra(wan_ifname);
		else /* set default value */
			accept_ra_reset(wan_ifname);
	}

	switch (service) {
	case IPV6_NATIVE:
		snprintf(addr6, sizeof(addr6), "%s/%d", nvram_safe_get("ipv6_wan_addr"), nvram_get_int("ipv6_prefix_len_wan"));
		eval("ip", "-6", "addr", "add", addr6, "dev", (char *)wan_ifname);
		eval("ip", "-6", "route", "del", "::/0");
		eval("ip", "-6", "route", "add", nvram_safe_get("ipv6_isp_gw"), "dev", (char *)wan_ifname);
		eval("ip", "-6", "route", "add", "::/0", "via", nvram_safe_get("ipv6_isp_gw"), "dev", (char *)wan_ifname);
		break;
	case IPV6_NATIVE_DHCP:
		/* IPv6 RA (via WAN) will take care of adding the default route, so that ipv6_isp_opt should not be enabled/required!
		   BUT: some ISP's, Snap (NZ), Internode (AU) may need the default route / workaround --> Tomato User can decide
		   see also https://www.linksysinfo.org/index.php?threads/ipv6-and-comcast.38006/
		*/
		if (nvram_get_int("ipv6_isp_opt") == 1) {
			eval("ip", "route", "add", "::/0", "dev", (char *)wan_ifname);
		}
		stop_dhcp6c();
		start_dhcp6c();
		break;
	case IPV6_ANYCAST_6TO4:
	case IPV6_6IN4:
		stop_ipv6_tunnel();
		if (service == IPV6_ANYCAST_6TO4) {
			addr4.s_addr = 0;
			memset(&addr, 0, sizeof(addr));
			inet_aton(get_wanip("wan"), &addr4);
			addr.s6_addr16[0] = htons(0x2002);
			ipv6_mapaddr4(&addr, 16, &addr4, 0);
			addr.s6_addr16[3] = htons(0x0001);
			inet_ntop(AF_INET6, &addr, addr6, sizeof(addr6));
			nvram_set("ipv6_prefix", addr6);
		}
		start_ipv6_tunnel();
		/* FIXME: give it a few seconds for DAD completion */
		sleep(2);
		break;
	case IPV6_6RD:
	case IPV6_6RD_DHCP:
		stop_6rd_tunnel();
		start_6rd_tunnel();
		/* FIXME2?: give it a few seconds for DAD completion */
		sleep(2);
		break;
	}
}
void stop_wan6(void)
{
	stop_ipv6_tunnel();
	stop_dhcp6c();

	nvram_set("ipv6_get_dns", ""); /* clear dns */

	dns_to_resolv();
}
#endif /* #ifdef TCONFIG_IPV6 */

/* ppp_demand: 0=keep alive, 1=connect on demand (run 'listen')
 * wan_ifname: vlan1
 * wan_iface: ppp# (PPPOE, PPP3G, PPTP, L2TP), vlan1 (DHCP, HB, Static, LTE)
 */
void start_wan_done(char *wan_ifname, char *prefix)
{
	int proto;
	int n;
	char *gw;
	struct sysinfo si;
	int wanup;
	int mwan_num;

	char wantime_file[64];
	char tmp[100];

	int is_primary;
	char pw[] = "wanXX";
	int first_ntp_sync = 0;

	sysinfo(&si);

	mwan_num = nvram_get_int("mwan_num");
	if ((mwan_num < 1) || (mwan_num > MWAN_MAX))
		mwan_num = 1;

	memset(wantime_file, 0, 64);
	sprintf(wantime_file, "/var/lib/misc/%s_time", prefix);
	f_write(wantime_file, &si.uptime, sizeof(si.uptime), 0, 0);

	proto = get_wanx_proto(prefix);

	if (mwan_num == 1) {
		/* Delete default interface route */
		if (proto == WP_PPTP || proto == WP_L2TP || (proto == WP_PPPOE && using_dhcpc(prefix))) /* Delete MAN default route */
			route_del(NULL, 0, NULL, NULL, NULL);
		else
			route_del(wan_ifname, 0, NULL, NULL, NULL);
	}

	if (proto != WP_DISABLED) {
		/* Set default route to gateway if specified */
		gw = wan_gateway(prefix);

		if ((*gw != 0) && (strcmp(gw, "0.0.0.0") != 0)) {
			if (proto == WP_DHCP || proto == WP_STATIC || proto == WP_LTE)
				/* Possibly gateway is over the bridge, try adding a route to gateway first */
				route_add(wan_ifname, 0, gw, NULL, "255.255.255.255");

			if (mwan_num == 1) {
				n = 5;
				while ((route_add(wan_ifname, 0, "0.0.0.0", gw, "0.0.0.0") == 1) && (n--)) {
					sleep(1);
				}
			}

			/* hack: avoid routing cycles, when both peer and server have the same IP */
			if (proto == WP_PPTP || proto == WP_L2TP)
				/* Delete gateway route as it's no longer needed */
				route_del(wan_ifname, 0, gw, "0.0.0.0", "255.255.255.255");
		}

		/* Change GW from peer IP to PPTP/L2TP IP */
		if (proto == WP_PPTP || proto == WP_L2TP) {
			route_del(nvram_safe_get(strcat_r(prefix, "_iface", tmp)), 0, nvram_safe_get(strcat_r(prefix, "_gateway_get", tmp)), NULL, "255.255.255.255");
			route_add(nvram_safe_get(strcat_r(prefix, "_iface", tmp)), 0, nvram_safe_get(strcat_r(prefix, "_ppp_get_ip", tmp)), NULL, "255.255.255.255");
		}
	}

	stop_dnsmasq();
	dns_to_resolv();
	start_dnsmasq();
	start_firewall();
	start_qos(prefix);

	do_static_routes(1);
	/* And routes supplied via DHCP */
	do_wan_routes((using_dhcpc(prefix) ? nvram_safe_get(strcat_r(prefix, "_ifname",tmp)) : wan_ifname), 0, 1, prefix);

#ifdef TCONFIG_IPV6
	/* start IPv6 BUT only for "wan" (no multiwan support, no primary wan) */
	if (strcmp(prefix, "wan") == 0) { /* check for "wan" prefix */
		switch (get_ipv6_service()) {
		case IPV6_NATIVE:
		case IPV6_NATIVE_DHCP:
			if (strncmp(wan_ifname, "ppp", 3) == 0) { /* check for pppx */
				break;
			}
			/* fall through */
		default:
			start_wan6(get_wan6face());
			break;
		}
	}
#endif

	/*
	 * FIX boot with only secondary etc wan active (assume current wan is primary if previous is not up)
	 */
	get_wan_prefix(nvram_get_int("wan_primary"), pw); /* Get current primary wan name */
	if (!check_wanup(pw)) { /* If primary wan offline, set current as primary */
		memset(tmp, 0, 100);
		sprintf(tmp, "%d", get_wan_unit(prefix));
		nvram_set("wan_primary", tmp);
		memset(pw, 0, 6);
		strncpy(pw, prefix, sizeof(pw));
	}

	wanup = check_wanup(prefix); /* is wan up? */
	is_primary = (strcmp(prefix, pw) == 0); /* is this primary wan? */

	if (is_primary) {
#ifdef TCONFIG_ZEBRA
		stop_zebra();
		start_zebra();
#endif

		if ((wanup || (proto == WP_DISABLED)) && (!nvram_get_int("ntp_ready"))) {
			if ((proto == WP_DISABLED) && nvram_get_int("lan_dhcp")) { /* Case: AP / WET / MB Mode with DHCP client for Lan (br0) */
				/* nothing to do here! and start ntpd (only) with bound event */
				logmsg(LOG_DEBUG, "*** %s: start ntpd with bound event (DHCP client)", __FUNCTION__);
			}
			else { /* default */
				first_ntp_sync = 1;
				stop_ntpd();
				start_ntpd();
			}
		}

		if ((wanup) || (proto == WP_DISABLED)) {
			if (nvram_get_int("ntp_ready") && !first_ntp_sync) {
				stop_ddns();
				start_ddns();
			}
			stop_igmp_proxy();
			stop_udpxy();
			start_igmp_proxy();
			start_udpxy();
		}
	}

	if (nvram_get_int("ntp_ready") && !first_ntp_sync) {
		stop_sched();
		start_sched();
	}

	if (wanup) {
		char wan_unit_str[16];
		memset(wan_unit_str, 0, 16);
		sprintf(wan_unit_str, "%d", get_wan_unit(prefix));

		notice_set(prefix, "");
		run_nvscript("script_mwanup", wan_unit_str, 0);

		if (is_primary)
			run_nvscript("script_wanup", NULL, 0);
	}

	if (is_primary) {
		/* WAN LED control */
		if (wanup)
			wan_led(wanup); /* LED ON! */

#ifndef TCONFIG_BCMARM
		/* We don't need STP after wireless led is lighted */
		if (check_hw_type() == HW_BCM4702) {
			eval("brctl", "stp", nvram_safe_get("lan_ifname"), "0");
			if (nvram_match("lan_stp", "1")) 
				eval("brctl", "stp", nvram_safe_get("lan_ifname"), "1");
			if (strcmp(nvram_safe_get("lan1_ifname"),"") != 0) {
				eval("brctl", "stp", nvram_safe_get("lan1_ifname"), "0");
				if (nvram_match("lan1_stp", "1")) 
					eval("brctl", "stp", nvram_safe_get("lan1_ifname"), "1");
			}
			if (strcmp(nvram_safe_get("lan2_ifname"),"") != 0) {
				eval("brctl", "stp", nvram_safe_get("lan2_ifname"), "0");
				if (nvram_match("lan2_stp", "1")) 
					eval("brctl", "stp", nvram_safe_get("lan2_ifname"), "1");
			}
			if (strcmp(nvram_safe_get("lan3_ifname"),"") != 0) {
				eval("brctl", "stp", nvram_safe_get("lan3_ifname"), "0");
				if (nvram_match("lan3_stp", "1")) 
					eval("brctl", "stp", nvram_safe_get("lan3_ifname"), "1");
			}
		}
#endif

		if (wanup) {
#ifdef TCONFIG_OPENVPN
			if (nvram_get_int("ntp_ready") && !first_ntp_sync)
				start_ovpn_eas();
#endif
#ifdef TCONFIG_TINC
			start_tinc(0);
#endif
			start_pptp_client_eas();
			start_adblock(0);
#ifdef TCONFIG_SAMBASRV
			stop_samba();
			start_samba(0);
#endif
		}

		stop_upnp();
		start_upnp();
		start_bwlimit();
	}

	mwan_table_add(prefix);
	mwan_load_balance();

	do_connect_file(0, prefix);
}

void stop_wan_if(char *prefix)
{
	char name[80];
	char *next;
	int wan_proto;

	char tmp[100];
	char wannotice_file[64];

	int mwan_num = nvram_get_int("mwan_num");

	if ((mwan_num < 1) || (mwan_num > MWAN_MAX))
		mwan_num = 1;

	if ((strcmp(prefix, "wan") == 0) && (mwan_num == 1)) { /* check for "wan" prefix AND only 1x wan (single-wan) */
		stop_upnp();
		logmsg(LOG_DEBUG, "*** %s: stop miniupnp (Case: Single-WAN)", __FUNCTION__);
	}

	mwan_table_del(prefix);

	stop_qos(prefix);

	/* stop IPv6 BUT only for "wan" (no multiwan support, no primary wan) */
#ifdef TCONFIG_IPV6
	if (strcmp(prefix, "wan") == 0) { /* check for "wan" prefix */
		stop_wan6();
	}
#endif

	/* Kill any WAN client daemons or callbacks */
	stop_redial(prefix);
	stop_l2tp(prefix); /* Special case for xl2tpd */
	stop_ppp(prefix); /* One for all */
	stop_dhcpc(prefix);

	wan_proto = get_wanx_proto(prefix);

	if (wan_proto == WP_PPP3G)
		killall_tk_period_wait("switch3g", 50); /* Kill switch3g script if running */

	if (wan_proto == WP_LTE) {
		killall_tk_period_wait("switch4g", 50); /* Kill switch4g script if running */
		xstart("switch4g", prefix, "disconnect");
		if (!nvram_get_int("g_upgrade"))
			sleep(3); /* Wait a litle for disconnect */
	}

	/* Bring down WAN interfaces */
	foreach(name, nvram_safe_get(strcat_r(prefix, "_ifnames", tmp)), next)
		ifconfig(name, 0, "0.0.0.0", NULL);

	memset(wannotice_file, 0, 64);
	sprintf(wannotice_file, "/var/notice/%s", prefix);
	unlink(wannotice_file);

	do_connect_file(0, prefix);

	mwan_load_balance();

	/* Clear old IP params from nvram on if stop
	 * but only if WAN is not as STATIC - shibby
	 * TBD: check mwan compat on empty gw/ip etc
	 */
	if (wan_proto != WP_STATIC && using_dhcpc(prefix)) { /* Not STATIC or PPP wan with manual IP params */
		nvram_set(strcat_r(prefix, "_ipaddr", tmp), "0.0.0.0");
		nvram_set(strcat_r(prefix, "_netmask", tmp), "0.0.0.0");
		nvram_set(strcat_r(prefix, "_gateway", tmp), "0.0.0.0");
	}
	if (wan_proto == WP_DHCP || wan_proto == WP_LTE || using_dhcpc(prefix)) { /* DHCP/LTE/PPP+DHCP wan */
		nvram_set(strcat_r(prefix, "_gateway_get", tmp), "0.0.0.0");
		nvram_set(strcat_r(prefix, "_get_dns", tmp), "");
	}
	if (wan_proto == WP_PPTP || wan_proto == WP_L2TP || wan_proto == WP_PPPOE || wan_proto == WP_PPP3G) {
		nvram_set(strcat_r(prefix, "_ppp_get_ip", tmp), "0.0.0.0");
		nvram_set(strcat_r(prefix, "_get_dns", tmp), "");
		/* For debug. Don't fool watchdog / mwan scripts with old obsolete interface, it can be actually used on other wan (ex: ppp0) */
		nvram_set(strcat_r(prefix, "_iface", tmp), "none");
	}
}

void stop_wan(void)
{
	logmsg(LOG_DEBUG, "*** IN: %s", __FUNCTION__);

	logmsg(LOG_DEBUG, "*** %s: removing watchdog job", __FUNCTION__);
	xstart("watchdog", "del");

#ifdef TCONFIG_TINC
	stop_tinc();
#endif
	stop_bwlimit();
	stop_upnp(); /* miniupnp - case: stop always (x wan(s)/restart) */

#ifdef TCONFIG_OPENVPN
	stop_ovpn_all();
#endif
	stop_pptp_client_eas();
	stop_igmp_proxy();
	stop_udpxy();
#ifdef TCONFIG_IPV6
	stop_ipv6_tunnel();
	stop_dhcp6c();
	nvram_set("ipv6_get_dns", "");
#endif
	stop_firewall();
	stop_adblock();
	clear_resolv();
	stop_wan_if("wan");
	stop_wan_if("wan2");
#ifdef TCONFIG_MULTIWAN
	stop_wan_if("wan3");
	stop_wan_if("wan4");
#endif

	logmsg(LOG_DEBUG, "*** OUT: %s", __FUNCTION__);
}
