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
 * Fixes/updates (C) 2018 - 2025 pedro
 * https://freshtomato.org/
 *
 */


#include "rc.h"

#ifndef TCONFIG_BCMARM
 #include <sys/time.h>
#endif
#include <wlutils.h>

#ifdef TOMATO64
#include <sys/time.h>
#include <sys/stat.h>
#endif /* TOMATO64 */

/* needed by logmsg() */
#define LOGMSG_DISABLE		DISABLE_SYSLOG_OSM
#define LOGMSG_NVDEBUG		"services_debug"


const char adblockexe[] = "/usr/sbin/adblock";
const char upnppath[] = "/etc/upnp";
const char upnpcfg[] = "/etc/upnp/config";
const char upnpcfgalt[] = "/etc/upnp/config.alt";
const char upnpcfgcustom[] = "/etc/upnp/config.custom";
const char igmpcfg[] = "/etc/igmp.conf";
#ifdef TCONFIG_ZEBRA
const char zebracfg[] = "/etc/zebra.conf";
const char ripdcfg[] = "/etc/ripd.conf";
#endif
#ifdef TCONFIG_MDNS
const char avahicfgpath[] = "/etc/avahi";
const char avahicfg[] = "avahi-daemon.conf";
const char avahisrvpath[] = "/etc/avahi/services";
const char avahicfgalt[] = "/etc/avahi/avahi-daemon_alt.conf";
#endif /* TCONFIG_MDNS */

#ifdef TCONFIG_BCMARM
 extern struct nvram_tuple rstats_defaults[];
#endif /* TCONFIG_BCMARM */
#ifdef TCONFIG_BCMARM
 extern struct nvram_tuple cstats_defaults[];
#endif /* TCONFIG_BCMARM */
#if defined(TCONFIG_FTP) && defined(TCONFIG_BCMARM)
 extern struct nvram_tuple ftp_defaults[];
#endif /* TCONFIG_FTP && TCONFIG_BCMARM */
#if defined(TCONFIG_SNMP) && defined(TCONFIG_BCMARM)
 extern struct nvram_tuple snmp_defaults[];
#endif /* TCONFIG_SNMP && TCONFIG_BCMARM */
#ifdef TCONFIG_BCMARM
 extern struct nvram_tuple upnp_defaults[];
#endif /* TCONFIG_BCMARM */
#ifdef TCONFIG_BCMBSD
 extern struct nvram_tuple bsd_defaults[];
#endif /* TCONFIG_BCMBSD */

/* Pop an alarm to recheck pids in 500 msec */
static const struct itimerval pop_tv = { {0, 0}, {0, 500 * 1000} };
/* Pop an alarm to reap zombies */
static const struct itimerval zombie_tv = { {0, 0}, {307, 0} };
static pid_t pid_crond = -1;
static pid_t pid_hotplug2 = -1;
static pid_t pid_igmp = -1;
static pid_t pid_ntpd = -1;
#ifdef TCONFIG_FANCTRL
static pid_t pid_phy_tempsense = -1;
#endif

void add_rstats_defaults(void)
{
#ifdef TCONFIG_BCMARM
	struct nvram_tuple *t;

	/* Restore defaults if necessary */
	for (t = rstats_defaults; t->name; t++) {
		if (!nvram_get(t->name)) { /* check existence */
			nvram_set(t->name, t->value);
		}
	}
#else
	eval("nvram", "rstats_defaults", "--add");
#endif /* TCONFIG_BCMARM */
}

void del_rstats_defaults(void)
{
#ifdef TCONFIG_BCMARM
	if (nvram_match("rstats_enable", "0")) {
		struct nvram_tuple *t;

		/* remove defaults if NOT necessary (only keep "xyz_enable" nv var.) */
		for (t = rstats_defaults; t->name; t++) {
			nvram_unset(t->name);
		}
	}
#else
	eval("nvram", "rstats_defaults", "--del");
#endif /* TCONFIG_BCMARM */
}

void add_cstats_defaults(void)
{
#ifdef TCONFIG_BCMARM
	struct nvram_tuple *t;

	/* Restore defaults if necessary */
	for (t = cstats_defaults; t->name; t++) {
		if (!nvram_get(t->name)) { /* check existence */
			nvram_set(t->name, t->value);
		}
	}
#else
	eval("nvram", "cstats_defaults", "--add");
#endif /* TCONFIG_BCMARM */
}

void del_cstats_defaults(void)
{
#ifdef TCONFIG_BCMARM
	if (nvram_match("cstats_enable", "0")) {
		struct nvram_tuple *t;

		/* remove defaults if NOT necessary (only keep "xyz_enable" nv var.) */
		for (t = cstats_defaults; t->name; t++) {
			nvram_unset(t->name);
		}
	}
#else
	eval("nvram", "cstats_defaults", "--del");
#endif /* TCONFIG_BCMARM */
}

#ifdef TCONFIG_FTP
void add_ftp_defaults(void)
{
#ifdef TCONFIG_BCMARM
	struct nvram_tuple *t;

	/* Restore defaults if necessary */
	for (t = ftp_defaults; t->name; t++) {
		if (!nvram_get(t->name)) { /* check existence */
			nvram_set(t->name, t->value);
		}
	}
#else
	eval("nvram", "ftp_defaults", "--add");
#endif /* TCONFIG_BCMARM */
}

void del_ftp_defaults(void)
{
#ifdef TCONFIG_BCMARM
	if (nvram_match("ftp_enable", "0")) {
		struct nvram_tuple *t;

		/* remove defaults if NOT necessary (only keep "xyz_enable" nv var.) */
		for (t = ftp_defaults; t->name; t++) {
			nvram_unset(t->name);
		}
	}
#else
	eval("nvram", "ftp_defaults", "--del");
#endif /* TCONFIG_BCMARM */
}
#endif /* TCONFIG_FTP */

#ifdef TCONFIG_SNMP
void add_snmp_defaults(void)
{
#ifdef TCONFIG_BCMARM
	struct nvram_tuple *t;

	/* Restore defaults if necessary */
	for (t = snmp_defaults; t->name; t++) {
		if (!nvram_get(t->name)) { /* check existence */
			nvram_set(t->name, t->value);
		}
	}
#else
	eval("nvram", "snmp_defaults", "--add");
#endif /* TCONFIG_BCMARM */
}

void del_snmp_defaults(void)
{
#ifdef TCONFIG_BCMARM
	if (nvram_match("snmp_enable", "0")) {
		struct nvram_tuple *t;

		/* remove defaults if NOT necessary (only keep "xyz_enable" nv var.) */
		for (t = snmp_defaults; t->name; t++) {
			nvram_unset(t->name);
		}
	}
#else
	eval("nvram", "snmp_defaults", "--del");
#endif /* TCONFIG_BCMARM */
}
#endif /* TCONFIG_SNMP */

void add_upnp_defaults(void)
{
#ifdef TCONFIG_BCMARM
	struct nvram_tuple *t;

	/* Restore defaults if necessary */
	for (t = upnp_defaults; t->name; t++) {
		if (!nvram_get(t->name)) { /* check existence */
			nvram_set(t->name, t->value);
		}
	}
#else
	eval("nvram", "upnp_defaults", "--add");
#endif /* TCONFIG_BCMARM */
}

void del_upnp_defaults(void)
{
#ifdef TCONFIG_BCMARM
	if (nvram_match("upnp_enable", "0")) {
		struct nvram_tuple *t;

		/* remove defaults if NOT necessary (only keep "xyz_enable" nv var.) */
		for (t = upnp_defaults; t->name; t++) {
			nvram_unset(t->name);
		}
	}
#else
	eval("nvram", "upnp_defaults", "--del");
#endif /* TCONFIG_BCMARM */
}

#ifdef TCONFIG_BCMBSD
void add_bsd_defaults(void)
{
	struct nvram_tuple *t;

	/* Restore defaults if necessary */
	for (t = bsd_defaults; t->name; t++) {
		if (!nvram_get(t->name)) { /* check existence */
			nvram_set(t->name, t->value);
		}
	}
}

void del_bsd_defaults(void)
{
	if (nvram_match("smart_connect_x", "0")) {
		struct nvram_tuple *t;

		/* remove defaults if NOT necessary (only keep "xyz_enable" nv var.) */
		for (t = bsd_defaults; t->name; t++) {
			nvram_unset(t->name);
		}
	}
}
#endif /* TCONFIG_BCMBSD */

void restart_firewall(void)
{
	stop_firewall();
	start_firewall();
}

#ifdef TCONFIG_DNSCRYPT
void start_dnscrypt(void)
{
	const static char *dnscrypt_resolv = "/etc/dnscrypt-resolvers.csv";
	const static char *dnscrypt_resolv_alt = "/etc/dnscrypt-resolvers-alt.csv";
	char dnscrypt_local[30];
	char *dnscrypt_ekeys;
	char *edns1, *edns2;

	if (!nvram_get_int("dnscrypt_proxy"))
		return;

	if (serialize_restart("dnscrypt-proxy", 1))
		return;

	memset(dnscrypt_local, 0, sizeof(dnscrypt_local));
	snprintf(dnscrypt_local, sizeof(dnscrypt_local), "127.0.0.1:%s", nvram_safe_get("dnscrypt_port"));

	dnscrypt_ekeys = nvram_get_int("dnscrypt_ephemeral_keys") ? "-E" : "";
	edns1 = nvram_get_int("dnsmasq_edns_size") < 1252 ? "-e" : ""; /* in case of EDNS packet size is set lower than 1252 in dnsmasq, set it also for dnscrypt-proxy */
	edns2 = nvram_get_int("dnsmasq_edns_size") < 1252 ? nvram_safe_get("dnsmasq_edns_size") : "";

	if (nvram_get_int("dnscrypt_manual"))
		eval("dnscrypt-proxy", "-d", dnscrypt_ekeys,
		     "-a", dnscrypt_local,
		     "-m", nvram_safe_get("dnscrypt_log"),
		     "-N", nvram_safe_get("dnscrypt_provider_name"),
		     "-k", nvram_safe_get("dnscrypt_provider_key"),
		     "-r", nvram_safe_get("dnscrypt_resolver_address"),
		     edns1, edns2);
	else
		eval("dnscrypt-proxy", "-d", dnscrypt_ekeys,
		     "-a", dnscrypt_local,
		     "-m", nvram_safe_get("dnscrypt_log"),
		     "-R", nvram_safe_get("dnscrypt_resolver"),
		     edns1, edns2,
		     "-L", f_exists(dnscrypt_resolv_alt) ? (char *) dnscrypt_resolv_alt : (char *) dnscrypt_resolv);
#ifdef TCONFIG_IPV6
	if (get_ipv6_service()) { /* when ipv6 enabled */
		memset(dnscrypt_local, 0, sizeof(dnscrypt_local));
		snprintf(dnscrypt_local, sizeof(dnscrypt_local), "::1:%s", nvram_safe_get("dnscrypt_port"));

		if (nvram_get_int("dnscrypt_manual"))
			eval("dnscrypt-proxy", "-d", dnscrypt_ekeys,
			     "-a", dnscrypt_local,
			     "-m", nvram_safe_get("dnscrypt_log"),
			     "-N", nvram_safe_get("dnscrypt_provider_name"),
			     "-k", nvram_safe_get("dnscrypt_provider_key"),
			     "-r", nvram_safe_get("dnscrypt_resolver_address"),
			     edns1, edns2);
		else
			eval("dnscrypt-proxy", "-d", dnscrypt_ekeys,
			     "-a", dnscrypt_local,
			     "-m", nvram_safe_get("dnscrypt_log"),
			     "-R", nvram_safe_get("dnscrypt_resolver"),
			     edns1, edns2,
			     "-L", f_exists(dnscrypt_resolv_alt) ? (char *) dnscrypt_resolv_alt : (char *) dnscrypt_resolv);
	}
#endif
}

void stop_dnscrypt(void)
{
	if (serialize_restart("dnscrypt-proxy", 0))
		return;

	killall_tk_period_wait("dnscrypt-proxy", 50);
}
#endif /* TCONFIG_DNSCRYPT */

#ifdef TCONFIG_STUBBY
void start_stubby(void)
{
	const static char *stubby_conf = "/etc/stubby/stubby.yml";
	const static char *stubby_conf_alt = "/etc/stubby/stubby_alt.yml";
	const static char *stubby_conf_custom = "/etc/stubby/stubby_custom.yml";
	FILE *fp, *fc;
	char *nv, *nvp, *b;
	char *server, *tlsport, *hostname, *spkipin, *digest;
	int ntp_ready, port, dnssec, ret;
	union {
		struct in_addr addr4;
#ifdef TCONFIG_IPV6
		struct in6_addr addr6;
#endif
	} addr;

	if (!nvram_get_int("stubby_proxy"))
		return;

	if (serialize_restart("stubby", 1))
		return;

	mkdir_if_none("/etc/stubby");

	/* alternative (user) configuration file */
	if (f_exists(stubby_conf_alt)) {
		eval("stubby", "-g", "-C", (char *)stubby_conf_alt);
		return;
	}

	/* custom configuration: use only this one, omit the other settings */
	if (strlen(nvram_safe_get("stubby_custom")) > 0) {
		if ((fc = fopen(stubby_conf_custom, "w")) == NULL) {
			logerr(__FUNCTION__, __LINE__, stubby_conf_custom);
			return;
		}
		fprintf(fc, "%s\n", nvram_safe_get("stubby_custom"));
		fclose(fc);

		eval("stubby", "-g", "-C", (char *)stubby_conf_custom);
		return;
	}

	if ((fp = fopen(stubby_conf, "w")) == NULL) {
		logerr(__FUNCTION__, __LINE__, stubby_conf);
		return;
	}

	ntp_ready = nvram_get_int("ntp_ready");
	dnssec = (nvram_get_int("dnssec_enable") && nvram_match("dnssec_method", "1"));

	/* basic & privacy settings */
	fprintf(fp, "appdata_dir: \"/var/lib/misc\"\n"
	            "resolution_type: GETDNS_RESOLUTION_STUB\n"
	            "dns_transport_list:\n"
	            "%s"
	            "tls_authentication: %s\n"
	            "tls_query_padding_blocksize: 128\n"
	            "edns_client_subnet_private: 1\n"
	            "%s"
	/* connection settings */
	            "idle_timeout: 5000\n"
	            "tls_connection_retries: 5\n"
	            "tls_backoff_time: 900\n"
	            "timeout: 2000\n"
	            "round_robin_upstreams: 1\n"
	            "tls_min_version: %s\n"
	/* listen address */
	            "listen_addresses:\n"
	            "  - 127.0.0.1@%s\n",
	            ntp_ready ? "  - GETDNS_TRANSPORT_TLS\n" : "  - GETDNS_TRANSPORT_UDP\n  - GETDNS_TRANSPORT_TCP\n",
	            ntp_ready ? "GETDNS_AUTHENTICATION_REQUIRED" : "GETDNS_AUTHENTICATION_NONE",
	            (ntp_ready && dnssec) ? "dnssec: GETDNS_EXTENSION_TRUE\n" : "",
	            nvram_get_int("stubby_force_tls13") ? "GETDNS_TLS1_3" : "GETDNS_TLS1_2",
	            nvram_safe_get("stubby_port"));
#ifdef TCONFIG_IPV6
	if (get_ipv6_service()) /* when ipv6 enabled */
		fprintf(fp, "  - 0::1@%s\n", nvram_safe_get("stubby_port"));
#endif
	/* upstreams */
	fprintf(fp, "upstream_recursive_servers:\n");

	nv = nvp = strdup(nvram_safe_get("stubby_resolvers"));
	while (nvp && (b = strsep(&nvp, "<")) != NULL) {
		server = tlsport = hostname = spkipin = NULL;

		/* <server>port>hostname>[digest:]spkipin */
		if ((vstrsep(b, ">", &server, &tlsport, &hostname, &spkipin)) < 4)
			continue;

		/* check server, can be IPv4/IPv6 address */
		if (*server == '\0')
			continue;
		else if (inet_pton(AF_INET, server, &addr) <= 0
#ifdef TCONFIG_IPV6
		         && ((inet_pton(AF_INET6, server, &addr) <= 0) || (!ipv6_enabled()))
#endif
		)
			continue;

		/* check port, if specified */
		port = (*tlsport ? atoi(tlsport) : 0);
		if ((port < 0) || (port > 65535))
			continue;

		/* add server */
		fprintf(fp, "  - address_data: %s\n", server);
		if (port)
			fprintf(fp, "    tls_port: %d\n", port);
		if (*hostname)
			fprintf(fp, "    tls_auth_name: \"%s\"\n", hostname);
		if (*spkipin) {
			digest = strchr(spkipin, ':') ? strsep(&spkipin, ":") : "sha256";
			fprintf(fp, "    tls_pubkey_pinset:\n"
			            "      - digest: \"%s\"\n"
			            "        value: %s\n", digest, spkipin);
		}
	}
	if (nv)
		free(nv);

	fclose(fp);

	if (dnssec) {
		if (ntp_ready)
			logmsg(LOG_INFO, "stubby: DNSSEC enabled");
		else
			logmsg(LOG_INFO, "stubby: DNSSEC pending ntp sync");
	}

	ret = eval("stubby", "-g", "-v", nvram_safe_get("stubby_log"), "-C", (char *)stubby_conf);

	if (ret)
		logmsg(LOG_ERR, "starting stubby failed ...");
}

void stop_stubby(void)
{
	if (serialize_restart("stubby", 0))
		return;

	killall_tk_period_wait("stubby", 70);
	eval("rm", "-f", "/var/run/stubby.pid");
}
#endif /* TCONFIG_STUBBY */

#ifdef TCONFIG_MDNS
void generate_mdns_config(void)
{
	FILE *fp;
	char avahi_config[80], tmp[8];
	unsigned int i;

	snprintf(avahi_config, sizeof(avahi_config), "%s/%s", avahicfgpath, avahicfg);

	/* generate avahi configuration file */
	if (!(fp = fopen(avahi_config, "w"))) {
		logerr(__FUNCTION__, __LINE__, avahi_config);
		return;
	}

	/* set [server] configuration */
	fprintf(fp, "[Server]\n"
	            "use-ipv4=yes\n"
	            "use-ipv6=%s\n"
	            "deny-interfaces=",
	            ipv6_enabled() ? "yes" : "no");

	for (i = 1; i <= MWAN_MAX; i++) {
		memset(tmp, 0, sizeof(tmp));
		snprintf(tmp, sizeof(tmp), (i == 1 ? "wan" : "wan%d"), i);
		if ((check_wanup(tmp)) || (i == 1))
			fprintf(fp, "%s%s", (i == 1 ? "" : ","), get_wanface(tmp));
	}

	fprintf(fp, "\n"
	            "ratelimit-interval-usec=1000000\n"
	            "ratelimit-burst=1000\n");

	/* set [publish] configuration */
	fprintf(fp, "\n[publish]\n"
	            "publish-hinfo=yes\n"
	            "publish-a-on-ipv6=no\n"
	            "publish-aaaa-on-ipv4=%s\n",
	            ipv6_enabled() ? "yes" : "no");

	/* set [reflector] configuration */
	fprintf(fp, "\n[reflector]\n");
	if (nvram_get_int("mdns_reflector"))
		fprintf(fp, "enable-reflector=yes\n");

	/* set [rlimits] configuration */
	fprintf(fp, "\n[rlimits]\n"
	            "rlimit-core=0\n"
	            "rlimit-data=4194304\n"
	            "rlimit-fsize=0\n"
	            "rlimit-nofile=256\n"
	            "rlimit-stack=4194304\n"
	            "rlimit-nproc=3\n");

	fclose(fp);
}

void start_mdns(void)
{
	if (nvram_get_int("g_upgrade") || nvram_get_int("g_reboot"))
		return;

	if (!nvram_get_int("mdns_enable"))
		return;

	if (serialize_restart("avahi-daemon", 1))
		return;

	mkdir_if_none(avahicfgpath);
	mkdir_if_none(avahisrvpath);

	/* alternative (user) configuration file */
	if (f_exists(avahicfgalt))
		eval("avahi-daemon", "-D", "-f", (char *)avahicfgalt, (nvram_get_int("mdns_debug") ? "--debug" : NULL));
	else {
		generate_mdns_config();
		eval("avahi-daemon", "-D", (nvram_get_int("mdns_debug") ? "--debug" : NULL));
	}
}

void stop_mdns(void)
{
	if (serialize_restart("avahi-daemon", 0))
		return;

	killall_tk_period_wait("avahi-daemon", 50);
}
#endif /* TCONFIG_MDNS */

#ifdef TCONFIG_IRQBALANCE
void stop_irqbalance(void)
{
	if (serialize_restart("irqbalance", 0))
		return;

	if (pidof("irqbalance") > 0) {
		killall_tk_period_wait("irqbalance", 50);
		logmsg(LOG_INFO, "irqbalance is stopped");
	}
}

void start_irqbalance(void)
{
	int ret;

	if (serialize_restart("irqbalance", 1))
		return;

	ret = eval("irqbalance", "-t", "10", "-s", "/var/run/irqbalance.pid");

	if (ret)
		logmsg(LOG_ERR, "starting irqbalance failed ...");
	else
		logmsg(LOG_INFO, "irqbalance is started");
}
#endif /* TCONFIG_IRQBALANCE */

#ifdef TCONFIG_FANCTRL
void start_phy_tempsense()
{
	stop_phy_tempsense();
	/* renice to high priority (10) - avoid revs fluctuations on high CPU load */
	char *phy_tempsense_argv[] = { "nice", "-n", "-10", "phy_tempsense", NULL };
	_eval(phy_tempsense_argv, NULL, 0, &pid_phy_tempsense);
}

void stop_phy_tempsense()
{
	pid_phy_tempsense = -1;
	killall_tk_period_wait("phy_tempsense", 50);
}
#endif /* TCONFIG_FANCTRL */

void start_adblock(int update)
{
	if (nvram_get_int("g_upgrade") || nvram_get_int("g_reboot"))
		return;

	if (!nvram_get_int("adblock_enable"))
		return;

	killall("adblock", SIGTERM);
	sleep(1);
	if (update)
		xstart(adblockexe, "update");
	else
		xstart(adblockexe);
}

void stop_adblock()
{
	xstart(adblockexe, "stop");
}

#ifdef TCONFIG_IPV6
static int write_ipv6_dns_servers(FILE *f, const char *prefix, char *dns, const char *suffix, int once)
{
	char p[INET6_ADDRSTRLEN + 1], *next = NULL;
	struct in6_addr addr;
	int cnt = 0;

	foreach(p, dns, next) {
		/* verify that this is a valid IPv6 address */
		if (inet_pton(AF_INET6, p, &addr) == 1) {
			fprintf(f, "%s%s%s", (once && cnt) ? "" : prefix, p, suffix);
			++cnt;
		}
	}

	return cnt;
}
#endif

void dns_to_resolv(void)
{
	FILE *f;
	const dns_list_t *dns;
	char *trig_ip;
	int i;
	mode_t m;
	char wan_prefix[] = "wanXX";
	int wan_unit, mwan_num;
	int append = 0;
	int exclusive = 0;
	char tmp[64];

	mwan_num = nvram_get_int("mwan_num");
	if ((mwan_num < 1) || (mwan_num > MWAN_MAX))
		mwan_num = 1;

	for (wan_unit = 1; wan_unit <= mwan_num; ++wan_unit) {
		get_wan_prefix(wan_unit, wan_prefix);

		/* skip inactive WAN connections */
		if ((check_wanup(wan_prefix) == 0) &&
		    get_wanx_proto(wan_prefix) != WP_DISABLED &&
		    get_wanx_proto(wan_prefix) != WP_PPTP &&
		    get_wanx_proto(wan_prefix) != WP_L2TP &&
		    !nvram_get_int(strlcat_r(wan_prefix, "_ppp_demand", tmp, sizeof(tmp))))
		{
			logmsg(LOG_DEBUG, "*** %s: %s (proto:%d) is not UP, not P-t-P or On Demand, SKIP ADD", __FUNCTION__, wan_prefix, get_wanx_proto(wan_prefix));
			continue;
		}
		else {
			logmsg(LOG_DEBUG, "*** %s: %s (proto:%d) is OK to ADD", __FUNCTION__, wan_prefix, get_wanx_proto(wan_prefix));
			append++;
		}
		m = umask(022); /* 077 from pppoecd */
		if ((f = fopen(dmresolv, (append == 1) ? "w" : "a")) != NULL) { /* write / append */
			if (append == 1)
				/* check for VPN DNS entries */
				exclusive = (write_pptpc_resolv(f)
#ifdef TCONFIG_OPENVPN
				             || write_ovpn_resolv(f)
#endif
				);

			logmsg(LOG_DEBUG, "*** %s: exclusive: %d", __FUNCTION__, exclusive);
			if (!exclusive) { /* exclusive check */
#ifdef TCONFIG_IPV6
				if ((write_ipv6_dns_servers(f, "nameserver ", nvram_safe_get("ipv6_dns"), "\n", 0) == 0) || (nvram_get_int("wan_addget"))) { /* addget only for the first WAN */
					if (append == 1) /* only once */
						write_ipv6_dns_servers(f, "nameserver ", nvram_safe_get("ipv6_get_dns"), "\n", 0);
				}
#endif
				dns = get_dns(wan_prefix); /* static buffer */
				if (dns->count == 0) {
					/* put a pseudo DNS IP to trigger Connect On Demand */
					if (nvram_match(strlcat_r(wan_prefix, "_ppp_demand", tmp, sizeof(tmp)), "1")) {
						switch (get_wanx_proto(wan_prefix)) {
							case WP_PPPOE:
							case WP_PPP3G:
							case WP_PPTP:
							case WP_L2TP:
								/* The nameserver IP specified below used to be 1.1.1.1, however this became an legit IP address of a public recursive DNS server,
								 * defeating the purpose of specifying a bogus DNS server in order to trigger Connect On Demand.
								 * An IP address from TEST-NET-2 block was chosen here, as RFC 5737 explicitly states this address block
								 * should be non-routable over the public internet. In effect since January 2010.
								 * Further info: https://linksysinfo.org/index.php?threads/tomato-using-1-1-1-1-for-pppoe-connect-on-demand.74102/
								 * Also add possibility to change that IP (198.51.100.1) in GUI by the user
								 */
								trig_ip = nvram_safe_get(strlcat_r(wan_prefix, "_ppp_demand_dnsip", tmp, sizeof(tmp)));
								logmsg(LOG_DEBUG, "*** %s: no servers for %s: put a pseudo DNS (non-routable on public internet) IP %s to trigger Connect On Demand", __FUNCTION__, wan_prefix, trig_ip);
								fprintf(f, "nameserver %s\n", trig_ip);
								break;
						}
					}
				}
				else {
					fprintf(f, "# dns for %s:\n", wan_prefix);
					for (i = 0; i < dns->count; i++) {
						if (dns->dns[i].port == 53) { /* resolv.conf doesn't allow for an alternate port */
							fprintf(f, "nameserver %s\n", inet_ntoa(dns->dns[i].addr));
							logmsg(LOG_DEBUG, "*** %s: %s DNS %s to %s [%s]", ((append == 1) ? "write" : "append"), __FUNCTION__, inet_ntoa(dns->dns[i].addr), dmresolv, wan_prefix);
						}
					}
				}
			}
			fclose(f);
		}
		else {
			logerr(__FUNCTION__, __LINE__, dmresolv);
			return;
		}
		umask(m);

	} /* end for (wan_unit = 1; wan_unit <= mwan_num; ++wan_unit) */
}

void start_httpd(void)
{
	int ret;

	if (serialize_restart("httpd", 1))
		return;

	if (nvram_match("web_css", "online"))
		xstart("/usr/sbin/ttb");

	/* set www dir */
	if (nvram_match("web_dir", "jffs"))
		chdir("/jffs/www");
	else if (nvram_match("web_dir", "opt"))
		chdir("/opt/www");
	else if (nvram_match("web_dir", "tmp"))
		chdir("/tmp/www");
	else
		chdir("/www");

	sleep(1);
	ret = eval("httpd", (nvram_get_int("http_nocache") ? "-N" : ""));
	chdir("/");

	if (ret)
		logmsg(LOG_ERR, "starting httpd failed ...");
	else
		logmsg(LOG_INFO, "httpd is started");

#ifdef TOMATO64
	if (nvram_match("http_enable", "1"))
		eval("/usr/bin/start_ttyd", "1");

	if (nvram_match("https_enable", "1"))
		eval("/usr/bin/start_ttyd", "2");
#endif /* TOMATO64 */
}

void stop_httpd(void)
{
	if (serialize_restart("httpd", 0))
		return;

	if (pidof("httpd") > 0) {
		killall_tk_period_wait("httpd", 50);
		logmsg(LOG_INFO, "httpd is stopped");
	}
#ifdef TOMATO64
	killall("ttyd", SIGTERM);
#endif /* TOMATO64 */
}

#ifdef TCONFIG_IPV6
static void add_ip6_lanaddr(void)
{
	char ip[INET6_ADDRSTRLEN + 4];
	const char *p;

	p = ipv6_router_address(NULL);
	if (*p) {
		snprintf(ip, sizeof(ip), "%s/%d", p, nvram_get_int("ipv6_prefix_length") ? : 64);

		eval("ip", "-6", "addr", "add", ip, "dev", nvram_safe_get("lan_ifname"));
	}
}

void start_ipv6_tunnel(void)
{
	char ip[INET6_ADDRSTRLEN + 4];
	char ip_tmp[INET6_ADDRSTRLEN + 4];
	struct in_addr addr4;
	struct in6_addr addr;
	char *wanip, *mtu, *tun_dev;
	int service;
	char wan_prefix[] = "wanXX";
	int wan_unit, mwan_num;

	mwan_num = nvram_get_int("mwan_num");
	if ((mwan_num < 1) || (mwan_num > MWAN_MAX))
		mwan_num = 1;

	for (wan_unit = 1; wan_unit <= mwan_num; ++wan_unit) {
		get_wan_prefix(wan_unit, wan_prefix);
		if (check_wanup(wan_prefix))
			break;
	}

	service = get_ipv6_service();
	tun_dev = (char *)get_wan6face();
	wanip = (char *)get_wanip(wan_prefix);

	mtu = (nvram_get_int("ipv6_tun_mtu") > 0) ? nvram_safe_get("ipv6_tun_mtu") : "1480";

	modprobe("sit");

	eval("ip", "tunnel", "add", tun_dev, "mode", "sit", "remote", (service == IPV6_ANYCAST_6TO4) ? "any" : nvram_safe_get("ipv6_tun_v4end"), "local", wanip, "ttl", nvram_safe_get("ipv6_tun_ttl"));
	eval("ip", "link", "set", tun_dev, "mtu", mtu, "up");

	nvram_set("ipv6_ifname", tun_dev);

	if (service == IPV6_ANYCAST_6TO4) {
		int prefixlen = 16;
		int mask4size = 0;

		addr4.s_addr = 0;
		memset(&addr, 0, sizeof(addr));
		inet_aton(wanip, &addr4);
		addr.s6_addr16[0] = htons(0x2002);
		ipv6_mapaddr4(&addr, prefixlen, &addr4, mask4size);
		addr.s6_addr16[7] = htons(0x0001);
		inet_ntop(AF_INET6, &addr, ip_tmp, sizeof(ip_tmp));
		snprintf(ip, sizeof(ip), "%s/%d", ip_tmp, prefixlen);
		add_ip6_lanaddr();
	}
	/* static tunnel 6to4 */
	else
		snprintf(ip, sizeof(ip), "%s/%d", nvram_safe_get("ipv6_tun_addr"), nvram_get_int("ipv6_tun_addrlen") ? : 64);

	eval("ip", "-6", "addr", "add", ip, "dev", tun_dev);

	if (service == IPV6_ANYCAST_6TO4) {
		snprintf(ip, sizeof(ip), "::192.88.99.%d", nvram_get_int("ipv6_relay"));
		eval("ip", "-6", "route", "add", "2000::/3", "via", ip, "dev", tun_dev, "metric", "1");
	}
	else
		eval("ip", "-6", "route", "add", "::/0", "dev", tun_dev, "metric", "1");

	/* (re)start dnsmasq */
	if (service == IPV6_ANYCAST_6TO4) {
		stop_dnsmasq();
		start_dnsmasq();
	}
}

void stop_ipv6_tunnel(void)
{
	eval("ip", "tunnel", "del", (char *)get_wan6face());

	if (get_ipv6_service() == IPV6_ANYCAST_6TO4)
		/* get rid of old IPv6 address from lan iface */
		eval("ip", "-6", "addr", "flush", "dev", nvram_safe_get("lan_ifname"), "scope", "global");

	modprobe_r("sit");
}

void start_6rd_tunnel(void)
{
	const char *tun_dev, *wanip;
	int service, mask_len, prefix_len, local_prefix_len;
	char mtu[10], prefix[INET6_ADDRSTRLEN], relay[INET_ADDRSTRLEN];
	struct in_addr netmask_addr, relay_addr, relay_prefix_addr, wanip_addr;
	struct in6_addr prefix_addr, local_prefix_addr;
	char local_prefix[INET6_ADDRSTRLEN];
	char tmp_ipv6[INET6_ADDRSTRLEN + 4], tmp_ipv4[INET_ADDRSTRLEN + 4];
	char tmp[256];
	FILE *f;
	char wan_prefix[] = "wanXX";
	int wan_unit, mwan_num;

	mwan_num = nvram_get_int("mwan_num");
	if ((mwan_num < 1) || (mwan_num > MWAN_MAX))
		mwan_num = 1;

	for (wan_unit = 1; wan_unit <= mwan_num; ++wan_unit) {
		get_wan_prefix(wan_unit, wan_prefix);
		if (check_wanup(wan_prefix))
			break;
	}

	service = get_ipv6_service();
	wanip = get_wanip(wan_prefix);
	tun_dev = get_wan6face();
	memset(mtu, 0, sizeof(mtu));
	snprintf(mtu, sizeof(mtu), "%d", (nvram_get_int("wan_mtu") > 0) ? (nvram_get_int("wan_mtu") - 20) : 1280);

	/* maybe we can merge the ipv6_6rd_* variables into a single ipv_6rd_string (ala wan_6rd) to save nvram space? */
	if (service == IPV6_6RD) {
		logmsg(LOG_DEBUG, "*** %s: starting 6rd tunnel using manual settings", __FUNCTION__);
		mask_len = nvram_get_int("ipv6_6rd_ipv4masklen");
		prefix_len = nvram_get_int("ipv6_6rd_prefix_length");
		strlcpy(prefix, nvram_safe_get("ipv6_6rd_prefix"), sizeof(prefix));
		strlcpy(relay, nvram_safe_get("ipv6_6rd_borderrelay"), sizeof(relay));
	}
	else {
		logmsg(LOG_DEBUG, "*** %s: starting 6rd tunnel using automatic settings", __FUNCTION__);
		char *wan_6rd = nvram_safe_get("wan_6rd");
		if (sscanf(wan_6rd, "%d %d %s %s", &mask_len,  &prefix_len, prefix, relay) < 4) {
			logmsg(LOG_DEBUG, "*** %s: wan_6rd string is missing or invalid (%s)", __FUNCTION__, wan_6rd);
			return;
		}
	}

	/* validate values that were passed */
	if ((mask_len < 0) || (mask_len > 32)) {
		logmsg(LOG_DEBUG, "*** %s: invalid mask_len value (%d)", __FUNCTION__, mask_len);
		return;
	}
	if ((prefix_len < 0) || (prefix_len > 128)) {
		logmsg(LOG_DEBUG, "*** %s: invalid prefix_len value (%d)", __FUNCTION__, prefix_len);
		return;
	}
	if (((32 - mask_len) + prefix_len) > 128) {
		logmsg(LOG_DEBUG, "*** %s: invalid combination of mask_len and prefix_len!", __FUNCTION__);
		return;
	}

	memset(tmp, 0, sizeof(tmp));
	snprintf(tmp, sizeof(tmp), "ping -q -c 2 %s | grep packet", relay);
	if ((f = popen(tmp, "r")) == NULL) {
		logmsg(LOG_DEBUG, "*** %s: error obtaining data", __FUNCTION__);
		return;
	}
	fgets(tmp, sizeof(tmp), f);
	pclose(f);

	if (strstr(tmp, " 0% packet loss") == NULL) {
		logmsg(LOG_DEBUG, "*** %s: failed to ping border relay", __FUNCTION__);
		return;
	}

	/* get relay prefix from border relay address and mask */
	netmask_addr.s_addr = htonl(0xffffffff << (32 - mask_len));
	inet_aton(relay, &relay_addr);
	relay_prefix_addr.s_addr = relay_addr.s_addr & netmask_addr.s_addr;

	/* calculate the local prefix */
	inet_pton(AF_INET6, prefix, &prefix_addr);
	inet_pton(AF_INET, wanip, &wanip_addr);
	if (calc_6rd_local_prefix(&prefix_addr, prefix_len, mask_len, &wanip_addr, &local_prefix_addr, &local_prefix_len) == 0) {
		logmsg(LOG_DEBUG, "*** %s: error calculating local prefix", __FUNCTION__);
		return;
	}
	inet_ntop(AF_INET6, &local_prefix_addr, local_prefix, sizeof(local_prefix));

	snprintf(tmp_ipv6, sizeof(tmp_ipv6), "%s1", local_prefix);
	nvram_set("ipv6_rtr_addr", tmp_ipv6);
	nvram_set("ipv6_prefix", local_prefix);

	/* load sit module needed for the 6rd tunnel */
	modprobe("sit");

	/* create the 6rd tunnel */
	eval("ip", "tunnel", "add", (char *)tun_dev, "mode", "sit", "local", (char *)wanip, "ttl", nvram_safe_get("ipv6_tun_ttl"));

	snprintf(tmp_ipv6, sizeof(tmp_ipv6), "%s/%d", prefix, prefix_len);
	snprintf(tmp_ipv4, sizeof(tmp_ipv4), "%s/%d", inet_ntoa(relay_prefix_addr), mask_len);
	eval("ip", "tunnel" "6rd", "dev", (char *)tun_dev, "6rd-prefix", tmp_ipv6, "6rd-relay_prefix", tmp_ipv4);

	/* bring up the link */
	eval("ip", "link", "set", "dev", (char *)tun_dev, "mtu", (char *)mtu, "up");

	/* set the WAN address Note: IPv6 WAN CIDR should be: ((32 - ip6rd_ipv4masklen) + ip6rd_prefixlen) */
	snprintf(tmp_ipv6, sizeof(tmp_ipv6), "%s1/%d", local_prefix, local_prefix_len);
	eval("ip", "-6", "addr", "add", tmp_ipv6, "dev", (char *)tun_dev);

	/* set the LAN address Note: IPv6 LAN CIDR should be 64 */
	snprintf(tmp_ipv6, sizeof(tmp_ipv6), "%s1/%d", local_prefix, nvram_get_int("ipv6_prefix_length") ? : 64);
	eval("ip", "-6", "addr", "add", tmp_ipv6, "dev", nvram_safe_get("lan_ifname"));

	/* add default route via the border relay */
	snprintf(tmp_ipv6, sizeof(tmp_ipv6), "::%s", relay);
	eval("ip", "-6", "route", "add", "::/0", "via", tmp_ipv6, "dev", (char *)tun_dev);

	nvram_set("ipv6_ifname", (char *)tun_dev);

	/* (re)start dnsmasq */
	stop_dnsmasq();
	start_dnsmasq();
}

void stop_6rd_tunnel(void)
{
	eval("ip", "tunnel", "del", (char *)get_wan6face());
	eval("ip", "-6", "addr", "flush", "dev", nvram_safe_get("lan_ifname"), "scope", "global");

	modprobe_r("sit");
}

void start_ipv6(void)
{
	int service, i;
	char buffer[16];

	service = get_ipv6_service();

	/* check if turned on */
	if (service != IPV6_DISABLED) {

		ipv6_forward("default", 1); /* enable it for default */
		ipv6_forward("all", 1); /* enable it for all */
		ndp_proxy("default", 1);
		ndp_proxy("all", 1);

		/* check if "ipv6_accept_ra" (bit 1) for lan is enabled (via GUI, basic-ipv6.asp) and "ipv6_radvd" AND "ipv6_dhcpd" (SLAAC and/or DHCP with dnsmasq) is disabled (via GUI, advanced-dhcpdns.asp) */
		/* HINT: "ipv6_accept_ra" bit 0 ==> used for wan, "ipv6_accept_ra" bit 1 ==> used for lan interfaces (br0...br3) */
		/* check lanX / brX if available */
		for (i = 0; i < BRIDGE_COUNT; i++) {
			memset(buffer, 0, sizeof(buffer));
			snprintf(buffer, sizeof(buffer), (i == 0 ? "lan_ipaddr" : "lan%d_ipaddr"), i);
			if (strcmp(nvram_safe_get(buffer), "") != 0) {
				memset(buffer, 0, sizeof(buffer));
				snprintf(buffer, sizeof(buffer), (i == 0 ? "lan_ifname" : "lan%d_ifname"), i);
				if (((nvram_get_int("ipv6_accept_ra") & 0x02) != 0) && !nvram_get_int("ipv6_radvd") && !nvram_get_int("ipv6_dhcpd"))
					/* accept_ra for brX */
					accept_ra(nvram_safe_get(buffer));
				else
					/* accept_ra default value for brX */
					accept_ra_reset(nvram_safe_get(buffer));
			}
		}
	}

	switch (service) {
		case IPV6_NATIVE:
		case IPV6_6IN4:
		case IPV6_MANUAL:
			add_ip6_lanaddr();
			break;
		case IPV6_NATIVE_DHCP:
		case IPV6_ANYCAST_6TO4:
			nvram_set("ipv6_rtr_addr", "");
			nvram_set("ipv6_prefix", "");
			break;
	}
}

void stop_ipv6(void)
{
	stop_ipv6_tunnel();
	stop_dhcp6c();

	ipv6_forward("default", 0); /* disable it for default */
	ipv6_forward("all", 0); /* disable it for all */
	ndp_proxy("default", 0);
	ndp_proxy("all", 0);

	eval("ip", "-6", "addr", "flush", "scope", "global");
	eval("ip", "-6", "route", "flush", "scope", "global");
}
#endif /* TCONFIG_IPV6 */

void start_upnp(void)
{
	FILE *f;
	int enable, upnp_port, https;
	int ports[4];
	char uuid[45];
	char lanN_ipaddr[] = "lanXX_ipaddr";
	char lanN_netmask[] = "lanXX_netmask";
	char lanN_ifname[] = "lanXX_ifname";
	char upnp_lanN[] = "upnp_lanXX";
	char tmp[8];
	char *lanip, *lanmask, *lanifname;
	char br;
	unsigned int i;

	enable = nvram_get_int("upnp_enable");

	/* only if enabled and proto not disabled */
	if ((enable == 0) || (get_wan_proto() == WP_DISABLED))
		return;

	if (serialize_restart("miniupnpd", 1))
		return;

	add_upnp_defaults(); /* backup: check nvram! */

	mkdir(upnppath, 0777);

	/* alternative configuration file */
	if (f_exists(upnpcfgalt)) {
		xstart("miniupnpd", "-f", upnpcfgalt);
		return;
	}

	if ((f = fopen(upnpcfg, "w")) == NULL) {
		logerr(__FUNCTION__, __LINE__, upnpcfg);
		return;
	}

	/* GUI configuration */

	/* TODO: not implemented in GUI */
	upnp_port = nvram_get_int("upnp_port");
	if ((upnp_port < 0) || (upnp_port >= 0xFFFF))
		upnp_port = 0;

	for (i = 1; i <= MWAN_MAX; i++) {
		memset(tmp, 0, sizeof(tmp));
		snprintf(tmp, sizeof(tmp), (i == 1 ? "wan" : "wan%d"), i);
		if ((check_wanup(tmp)) || (i == 1))
			fprintf(f, "ext_ifname=%s\n", get_wanface(tmp));
	}
	fprintf(f, "port=%d\n"
	           "enable_upnp=%s\n"
	           "enable_pcp_pmp=%s\n"
	           "force_igd_desc_v1=yes\n"
	           "secure_mode=%s\n"
	           "pcp_allow_thirdparty=%s\n"
	           "upnp_forward_chain=upnp\n"
	           "upnp_nat_chain=upnp\n"
	           "upnp_nat_postrouting_chain=pupnp\n"
	           "notify_interval=%d\n"
	           "system_uptime=yes\n"
	           "friendly_name=Tomato64 UPnP IGD &amp; PCP\n"
	           "model_name=%s\n"
	           "model_url=https://tomato64.org/\n"
	           "manufacturer_name=Tomato64 Firmware\n"
	           "manufacturer_url=https://tomato64.org/\n"
	           /* Empty strings so that 1 and 00000000 are not reported */
	           "model_number=\n"
	           "serial=\n"
	           "\n",
	           upnp_port,
	           (enable & 1) ? "yes" : "no",			/* upnp enable */
	           (enable & 2) ? "yes" : "no",			/* pcp_pmp enable */
	           nvram_get_int("upnp_secure") ? "yes" : "no",	/* secure_mode (only forward to self) */
	           nvram_get_int("upnp_secure") ? "no" : "yes",	/* allow third party */
	           nvram_get_int("upnp_ssdp_interval"),
	           nvram_safe_get("t_model_name"));

	https = nvram_get_int("https_enable");
	fprintf(f, "presentation_url=http%s://%s:%s/forward-upnp.asp\n", (https ? "s" : ""), nvram_safe_get("lan_ipaddr"), nvram_safe_get(https ? "https_lanport" : "http_lanport"));

	f_read_string("/proc/sys/kernel/random/uuid", uuid, sizeof(uuid));
	fprintf(f, "uuid=%s\n", uuid);

	/* move custom configuration before "allow" statements */
	/* discussion: http://www.linksysinfo.org/index.php?threads/miniupnpd-custom-config-syntax.70863/#post-256291 */
	fappend(f, upnpcfgcustom);
	fprintf(f, "%s\n", nvram_safe_get("upnp_custom"));

	for (br = 0; br < BRIDGE_COUNT; br++) {
		char bridge[2] = "0";
		if (br != 0)
			bridge[0] += br;
		else
			memset(bridge, 0, sizeof(bridge));

		snprintf(lanN_ipaddr, sizeof(lanN_ipaddr), "lan%s_ipaddr", bridge);
		snprintf(lanN_netmask, sizeof(lanN_netmask), "lan%s_netmask", bridge);
		snprintf(lanN_ifname, sizeof(lanN_ifname), "lan%s_ifname", bridge);
		snprintf(upnp_lanN, sizeof(upnp_lanN), "upnp_lan%s", bridge);

		lanip = nvram_safe_get(lanN_ipaddr);
		lanmask = nvram_safe_get(lanN_netmask);
		lanifname = nvram_safe_get(lanN_ifname);

		if ((strcmp(nvram_safe_get(upnp_lanN), "1") == 0) && (strcmp(lanifname, "") != 0)) {
			fprintf(f, "listening_ip=%s\n", lanifname);

			/* not implemented in GUI */
			if ((ports[0] = nvram_get_int("upnp_min_port_ext")) > 0 &&
			    (ports[1] = nvram_get_int("upnp_max_port_ext")) > 0 &&
			    (ports[2] = nvram_get_int("upnp_min_port_int")) > 0 &&
			    (ports[3] = nvram_get_int("upnp_max_port_int")) > 0)
				fprintf(f, "allow %d-%d %s/%s %d-%d\n", ports[0], ports[1], lanip, lanmask, ports[2], ports[3]);
			else
				/* by default allow only redirection of ports above 1024 */
				fprintf(f, "allow 1024-65535 %s/%s 1024-65535\n", lanip, lanmask);
		}
	}
	fprintf(f, "\ndeny 0-65535 0.0.0.0/0 0-65535\n");

	fclose(f);

	xstart("miniupnpd", "-f", (char *)upnpcfg);
}

void stop_upnp(void)
{
	if (serialize_restart("miniupnpd", 0))
		return;

	killall_tk_period_wait("miniupnpd", 50);

	/* clean-up */
	eval("rm", "-f", (char *)upnpcfg);
}

void start_cron(void)
{
	stop_cron();

	eval("crond", (nvram_contains_word("log_events", "crond") ? NULL : "-l"), "9");

	if (!nvram_contains_word("debug_norestart", "crond"))
		pid_crond = -2;
}

void stop_cron(void)
{
	pid_crond = -1;
	killall_tk_period_wait("crond", 50);
}

void start_hotplug2(void)
{
	stop_hotplug2();

	f_write_string("/proc/sys/kernel/hotplug", "", FW_NEWLINE, 0);
	xstart("hotplug2", "--persistent", "--no-coldplug");

	/* FIXME: Don't remember exactly why I put "sleep" here - but it was not for a race with check_services()... */
	sleep(1);

	if (!nvram_contains_word("debug_norestart", "hotplug2"))
		pid_hotplug2 = -2;
}

void stop_hotplug2(void)
{
	pid_hotplug2 = -1;
	killall_tk_period_wait("hotplug2", 50);
}

#ifdef TCONFIG_ZEBRA
void start_zebra(void)
{
	FILE *fp;

	char *lan_tx = nvram_safe_get("dr_lan_tx");
	char *lan_rx = nvram_safe_get("dr_lan_rx");
	char *lan1_tx = nvram_safe_get("dr_lan1_tx");
	char *lan1_rx = nvram_safe_get("dr_lan1_rx");
	char *lan2_tx = nvram_safe_get("dr_lan2_tx");
	char *lan2_rx = nvram_safe_get("dr_lan2_rx");
	char *lan3_tx = nvram_safe_get("dr_lan3_tx");
	char *lan3_rx = nvram_safe_get("dr_lan3_rx");
	char *wan_tx = nvram_safe_get("dr_wan_tx");
	char *wan_rx = nvram_safe_get("dr_wan_rx");

	if (serialize_restart("zebra", 1))
		return;

	if ((*lan_tx == '0') && (*lan_rx == '0') &&
	    (*lan1_tx == '0') && (*lan1_rx == '0') &&
	    (*lan2_tx == '0') && (*lan2_rx == '0') &&
	    (*lan3_tx == '0') && (*lan3_rx == '0') &&
	    (*wan_tx == '0') && (*wan_rx == '0')) {
		return;
	}

	f_write(zebracfg, NULL, 0, 0, 0); /* blank */

	if ((fp = fopen(ripdcfg, "w")) == NULL) {
		logerr(__FUNCTION__, __LINE__, ripdcfg);
		return;
	}

	char *lan_ifname = nvram_safe_get("lan_ifname");
	char *lan1_ifname = nvram_safe_get("lan1_ifname");
	char *lan2_ifname = nvram_safe_get("lan2_ifname");
	char *lan3_ifname = nvram_safe_get("lan3_ifname");
	char *wan_ifname = nvram_safe_get("wan_ifname");

	fprintf(fp, "router rip\n");

	if (strcmp(lan_ifname, "") != 0)
		fprintf(fp, "network %s\n", lan_ifname);
	if (strcmp(lan1_ifname, "") != 0)
		fprintf(fp, "network %s\n", lan1_ifname);
	if (strcmp(lan2_ifname, "") != 0)
		fprintf(fp, "network %s\n", lan2_ifname);
	if (strcmp(lan3_ifname, "") != 0)
		fprintf(fp, "network %s\n", lan3_ifname);

	fprintf(fp, "network %s\n", wan_ifname);
	fprintf(fp, "redistribute connected\n");

	if (strcmp(lan_ifname, "") != 0) {
		fprintf(fp, "interface %s\n", lan_ifname);
		if (*lan_tx != '0')
			fprintf(fp, "ip rip send version %s\n", lan_tx);
		if (*lan_rx != '0')
			fprintf(fp, "ip rip receive version %s\n", lan_rx);
	}
	if (strcmp(lan1_ifname, "") != 0) {
		fprintf(fp, "interface %s\n", lan1_ifname);
		if (*lan1_tx != '0')
			fprintf(fp, "ip rip send version %s\n", lan1_tx);
		if (*lan1_rx != '0')
			fprintf(fp, "ip rip receive version %s\n", lan1_rx);
	}
	if (strcmp(lan2_ifname, "") != 0) {
		fprintf(fp, "interface %s\n", lan2_ifname);
		if (*lan2_tx != '0')
			fprintf(fp, "ip rip send version %s\n", lan2_tx);
		if (*lan2_rx != '0')
			fprintf(fp, "ip rip receive version %s\n", lan2_rx);
	}
	if (strcmp(lan3_ifname, "") != 0) {
		fprintf(fp, "interface %s\n", lan3_ifname);
		if (*lan3_tx != '0')
			fprintf(fp, "ip rip send version %s\n", lan3_tx);
		if (*lan3_rx != '0')
			fprintf(fp, "ip rip receive version %s\n", lan3_rx);
	}

	fprintf(fp, "interface %s\n", wan_ifname);

	if (*wan_tx != '0')
		fprintf(fp, "ip rip send version %s\n", wan_tx);
	if (*wan_rx != '0')
		fprintf(fp, "ip rip receive version %s\n", wan_rx);

	fprintf(fp, "router rip\n");

	if (strcmp(lan_ifname, "") != 0) {
		if (*lan_tx == '0')
			fprintf(fp, "distribute-list private out %s\n", lan_ifname);
		if (*lan_rx == '0')
			fprintf(fp, "distribute-list private in %s\n", lan_ifname);
	}
	if (strcmp(lan1_ifname, "") != 0) {
		if (*lan1_tx == '0')
			fprintf(fp, "distribute-list private out %s\n", lan1_ifname);
		if (*lan1_rx == '0')
			fprintf(fp, "distribute-list private in %s\n", lan1_ifname);
	}
	if (strcmp(lan2_ifname, "") != 0) {
		if (*lan2_tx == '0')
			fprintf(fp, "distribute-list private out %s\n", lan2_ifname);
		if (*lan2_rx == '0')
			fprintf(fp, "distribute-list private in %s\n", lan2_ifname);
	}
	if (strcmp(lan3_ifname, "") != 0) {
		if (*lan3_tx == '0')
			fprintf(fp, "distribute-list private out %s\n", lan3_ifname);
		if (*lan3_rx == '0')
			fprintf(fp, "distribute-list private in %s\n", lan3_ifname);
	}
	if (*wan_tx == '0')
		fprintf(fp, "distribute-list private out %s\n", wan_ifname);
	if (*wan_rx == '0')
		fprintf(fp, "distribute-list private in %s\n", wan_ifname);

	fprintf(fp, "access-list private deny any\n");

	//fprintf(fp, "debug rip events\n");
	//fprintf(fp, "log file /etc/ripd.log\n");
	fclose(fp);

	xstart("zebra", "-d");
	xstart("ripd",  "-d");
}

void stop_zebra(void)
{
	if (serialize_restart("zebra", 0))
		return;

	killall("zebra", SIGTERM);
	killall("ripd", SIGTERM);

	unlink(zebracfg);
	unlink(ripdcfg);
}
#endif /* #ifdef TCONFIG_ZEBRA */

void start_syslog(void)
{
	char *argv[20];
	int argc;
	char *nv;
	char *b_opt = "";
	char rem[256];
	int n;
	char s[64];
	char cfg[256];
	char *rot_siz = "50";
	char *rot_keep = "1";
	char *log_file_path;
	char log_default[] = "/var/log/messages";
	char *log_min_level;

	argv[0] = "syslogd";
	argc = 1;

	if (nvram_get_int("log_dropdups"))
		argv[argc++] = "-D";

	if (nvram_get_int("log_remote")) {
		nv = nvram_safe_get("log_remoteip");
		if (*nv) {
			snprintf(rem, sizeof(rem), "%s:%s", nv, nvram_safe_get("log_remoteport"));
			argv[argc++] = "-R";
			argv[argc++] = rem;
		}
	}

	if (nvram_get_int("log_file")) {
		argv[argc++] = "-L";

		if (strcmp(nvram_safe_get("log_file_size"), "") != 0)
			rot_siz = nvram_safe_get("log_file_size");

		if (nvram_get_int("log_file_size") > 0)
			rot_keep = nvram_safe_get("log_file_keep");

		/* log to custom path */
		if (nvram_get_int("log_file_custom")) {
			log_file_path = nvram_safe_get("log_file_path");
			argv[argc++] = "-s";
			argv[argc++] = rot_siz;
			argv[argc++] = "-O";
			argv[argc++] = log_file_path;
			if (strcmp(nvram_safe_get("log_file_path"), log_default) != 0) {
				remove(log_default);
				symlink(log_file_path, log_default);
			}
		}
		else {
			/* Read options:    rotate_size(kb)    num_backups    logfilename.
			 * Ignore these settings and use defaults if the logfile cannot be written to.
			 */
			if (f_read_string("/etc/syslogd.cfg", cfg, sizeof(cfg)) > 0) {
				if ((nv = strchr(cfg, '\n')))
					*nv = 0;

				if ((nv = strtok(cfg, " \t"))) {
					if (isdigit(*nv))
						rot_siz = nv;
				}

				if ((nv = strtok(NULL, " \t")))
					b_opt = nv;

				if ((nv = strtok(NULL, " \t")) && *nv == '/') {
					if (f_write(nv, cfg, 0, FW_APPEND, 0) >= 0) {
						argv[argc++] = "-O";
						argv[argc++] = nv;
					}
					else {
						rot_siz = "50";
						b_opt = "";
					}
				}
			}
		}

		if (!(nvram_get_int("log_file_custom"))) {
			argv[argc++] = "-s";
			argv[argc++] = rot_siz;
			struct stat sb;
			if (lstat(log_default, &sb) != -1)
				if (S_ISLNK(sb.st_mode))
					remove(log_default);
		}

		if (isdigit(*b_opt)) {
			argv[argc++] = "-b";
			argv[argc++] = b_opt;
		}
		else if (nvram_get_int("log_file_size") > 0) {
			argv[argc++] = "-b";
			argv[argc++] = rot_keep;
		}

		log_min_level = nvram_safe_get("log_min_level");
		argv[argc++] = "-l";
		argv[argc++] = log_min_level;
	}

	if (argc > 1) {
		argv[argc] = NULL;
		_eval(argv, NULL, 0, NULL);

		argv[0] = "klogd";
		argv[1] = NULL;
		_eval(argv, NULL, 0, NULL);

		/* used to be available in syslogd -m */
		n = nvram_get_int("log_mark");
		if (n > 0) {
			memset(rem, 0, sizeof(rem));
			/* n is in minutes */
			if (n < 60)
				snprintf(rem, sizeof(rem), "*/%d * * * *", n);
			else if (n < 60 * 24)
				snprintf(rem, sizeof(rem), "0 */%d * * *", n / 60);
			else
				snprintf(rem, sizeof(rem), "0 0 */%d * *", n / (60 * 24));

			memset(s, 0, sizeof(s));
			snprintf(s, sizeof(s), "%s logger -p syslog.info -- -- MARK --", rem);
			eval("cru", "a", "syslogdmark", s);
		}
		else {
			eval("cru", "d", "syslogdmark");
		}
	}
}

void stop_syslog(void)
{
	killall("klogd", SIGTERM);
	killall("syslogd", SIGTERM);
}

void start_igmp_proxy(void)
{
	FILE *fp;
	const char *nv;
	char igmp_buffer[32];
	char wan_prefix[] = "wanXX";
	int wan_unit, mwan_num, count = 0;
	int ret = 1;
	int i, enabled_interface;

	mwan_num = nvram_get_int("mwan_num");
	if ((mwan_num < 1) || (mwan_num > MWAN_MAX))
		mwan_num = 1;

	/* only if enabled */
	if (!nvram_get_int("multicast_pass"))
		return;

	/* custom configuration file */
	if (f_exists("/etc/igmp.alt"))
		ret = eval("igmpproxy", "/etc/igmp.alt");
	/* GUI configuration */
	else if ((fp = fopen(igmpcfg, "w")) != NULL) {
		fprintf(fp, "user nobody\n"); /* drop privileges */

		/* check that lan, lan1, lan2 and lan3 are not selected and use custom config */
		/* The configuration file must define one (or more) upstream interface(s) and one or more downstream interfaces,
		 * see https://github.com/pali/igmpproxy/commit/b55e0125c79fc9dbc95c6d6ab1121570f0c6f80f and
		 * see https://github.com/pali/igmpproxy/blob/master/igmpproxy.conf
		 */
		enabled_interface=0;
		for (i = 0; i < BRIDGE_COUNT; i++) {
			snprintf(igmp_buffer, sizeof(igmp_buffer), (i == 0 ? "multicast_lan" : "multicast_lan%d"), i);
			enabled_interface += nvram_get_int(igmp_buffer);
		}
		if (!enabled_interface) {
			fprintf(fp, "%s\n", nvram_safe_get("multicast_custom"));
			fclose(fp);
			ret = eval("igmpproxy", (char *)igmpcfg);
		}
		/* create default config for upstream/downstream interface(s) */
		else {
			if (nvram_get_int("multicast_quickleave"))
				fprintf(fp, "quickleave\n");

			for (wan_unit = 1; wan_unit <= mwan_num; ++wan_unit) {
				get_wan_prefix(wan_unit, wan_prefix);
				if ((check_wanup(wan_prefix)) && (get_wanx_proto(wan_prefix) != WP_DISABLED)) {
					count++;
					/*
					 * Configuration for Upstream Interface
					 * Example:
					 * phyint ppp0 upstream ratelimit 0 threshold 1
					 * altnet 193.158.35.0/24
					 */
					fprintf(fp, "phyint %s upstream ratelimit 0 threshold 1\n", get_wanface(wan_prefix));
					/* check for allowed remote network address, see note at GUI advanced-firewall.asp */
					if ((nvram_get("multicast_altnet_1") != NULL) || (nvram_get("multicast_altnet_2") != NULL) || (nvram_get("multicast_altnet_3") != NULL)) {
						if (((nv = nvram_get("multicast_altnet_1")) != NULL) && (*nv)) {
							memset(igmp_buffer, 0, sizeof(igmp_buffer)); /* reset */
							snprintf(igmp_buffer, sizeof(igmp_buffer),"%s", nv); /* copy to buffer */
							fprintf(fp, "\taltnet %s\n", igmp_buffer); /* with the following format: a.b.c.d/n - Example: altnet 10.0.0.0/16 */
							logmsg(LOG_INFO, "igmpproxy: multicast_altnet_1 = %s", igmp_buffer);
						}

						if (((nv = nvram_get("multicast_altnet_2")) != NULL) && (*nv)) {
							memset(igmp_buffer, 0, sizeof(igmp_buffer)); /* reset */
							snprintf(igmp_buffer, sizeof(igmp_buffer),"%s", nv); /* copy to buffer */
							fprintf(fp, "\taltnet %s\n", igmp_buffer); /* with the following format: a.b.c.d/n - Example: altnet 10.0.0.0/16 */
							logmsg(LOG_INFO, "igmpproxy: multicast_altnet_2 = %s", igmp_buffer);
						}

						if (((nv = nvram_get("multicast_altnet_3")) != NULL) && (*nv)) {
							memset(igmp_buffer, 0, sizeof(igmp_buffer)); /* reset */
							snprintf(igmp_buffer, sizeof(igmp_buffer),"%s", nv); /* copy to buffer */
							fprintf(fp, "\taltnet %s\n", igmp_buffer); /* with the following format: a.b.c.d/n - Example: altnet 10.0.0.0/16 */
							logmsg(LOG_INFO, "igmpproxy: multicast_altnet_3 = %s", igmp_buffer);
						}
					}
					else
						fprintf(fp, "\taltnet 0.0.0.0/0\n"); /* default, allow all! */
				}
			}
			if (!count) {
				fclose(fp);
				unlink(igmpcfg);
				return;
			}

			char lanN_ifname[] = "lanXX_ifname";
			char multicast_lanN[] = "multicast_lanXX";
			char br;

			for (br = 0; br < BRIDGE_COUNT; br++) {
				char bridge[2] = "0";
				if (br != 0)
					bridge[0] += br;
				else
					memset(bridge, 0, sizeof(bridge));

				snprintf(lanN_ifname, sizeof(lanN_ifname), "lan%s_ifname", bridge);
				snprintf(multicast_lanN, sizeof(multicast_lanN), "multicast_lan%s", bridge);

				if ((strcmp(nvram_safe_get(multicast_lanN), "1") == 0) && (strcmp(nvram_safe_get(lanN_ifname), "") != 0)) {
				/*
				 * Configuration for Downstream Interface
				 * Example:
				 * phyint br0 downstream ratelimit 0 threshold 1
				 */
					fprintf(fp, "phyint %s downstream ratelimit 0 threshold 1\n", nvram_safe_get(lanN_ifname));
				}
			}
			fclose(fp);
			ret = eval("igmpproxy", (char *)igmpcfg);
		}
	}
	else {
		logerr(__FUNCTION__, __LINE__, igmpcfg);
		return;
	}

	if (!nvram_contains_word("debug_norestart", "igmprt"))
		pid_igmp = -2;

	if (ret)
		logmsg(LOG_ERR, "starting igmpproxy failed ...");
	else
		logmsg(LOG_INFO, "igmpproxy is started");

}

void stop_igmp_proxy(void)
{
	pid_igmp = -1;
	if (pidof("igmpproxy") > 0) {
		killall_tk_period_wait("igmpproxy", 50);
		logmsg(LOG_INFO, "igmpproxy is stopped");
	}

	/* clean-up */
	unlink(igmpcfg);
}

void start_udpxy(void)
{
	char wan_prefix[] = "wan"; /* not yet mwan ready, use wan for now */
	char buffer[32], buffer2[16];
	int i, bind_lan = 0;

	/* only if enabled */
	if (!nvram_get_int("udpxy_enable"))
		return;

	if ((check_wanup(wan_prefix)) && (get_wanx_proto(wan_prefix) != WP_DISABLED)) {
		memset(buffer, 0, sizeof(buffer)); /* reset */
		if (strlen(nvram_safe_get("udpxy_wanface")) > 0)
			snprintf(buffer, sizeof(buffer), "%s", nvram_safe_get("udpxy_wanface")); /* user entered upstream interface */
		else
			snprintf(buffer, sizeof(buffer), "%s", get_wanface(wan_prefix)); /* copy wanface to buffer */

		/* check interface to listen on */
		/* check udpxy enabled/selected for br0 - br3 */
		for (i = 0; i < BRIDGE_COUNT; i++) {
			int ret1 = 0, ret2 = 0;
			memset(buffer2, 0, sizeof(buffer2));
			snprintf(buffer2, sizeof(buffer2), (i == 0 ? "udpxy_lan" : "udpxy_lan%d"), i);
			ret1 = nvram_match(buffer2, "1");
			memset(buffer2, 0, sizeof(buffer2));
			snprintf(buffer2, sizeof(buffer2), (i == 0 ? "lan_ipaddr" : "lan%d_ipaddr"), i);
			ret2 = strcmp(nvram_safe_get(buffer2), "") != 0;
			if (ret1 && ret2) {
				memset(buffer2, 0, sizeof(buffer2));
				snprintf(buffer2, sizeof(buffer2), (i == 0 ? "lan_ifname" : "lan%d_ifname"), i);
#ifdef TOMATO64
				eval("udpxy", "-p", nvram_safe_get("udpxy_port"), "-c", nvram_safe_get("udpxy_clients"), "-a", nvram_safe_get(buffer2), "-m", buffer, (nvram_get_int("udpxy_stats") ? "-S" : ""));
#else
				eval("udpxy", (nvram_get_int("udpxy_stats") ? "-S" : ""), "-p", nvram_safe_get("udpxy_port"), "-c", nvram_safe_get("udpxy_clients"), "-a", nvram_safe_get(buffer2), "-m", buffer);
#endif /* TOMATO64 */
				bind_lan = 1;
				break; /* start udpxy only once and only for one lanX */
			}
		}
		/* address/interface to listen on: default = 0.0.0.0 */
		if (!bind_lan)
#ifdef TOMATO64
		{
			eval("udpxy", "-p", nvram_safe_get("udpxy_port"), "-c", nvram_safe_get("udpxy_clients"), "-m", buffer, (nvram_get_int("udpxy_stats") ? "-S" : ""));
		}
#else
		{
			eval("udpxy", (nvram_get_int("udpxy_stats") ? "-S" : ""), "-p", nvram_safe_get("udpxy_port"), "-c", nvram_safe_get("udpxy_clients"), "-m", buffer);
		}
#endif /* TOMATO64 */
	}
}

void stop_udpxy(void)
{
	killall_tk_period_wait("udpxy", 50);
}

void set_tz(void)
{
	f_write_string("/etc/TZ", nvram_safe_get("tm_tz"), (FW_CREATE | FW_NEWLINE), 0644);
#ifdef TOMATO64
	tzset();
#endif /* TOMATO64 */
}

void start_ntpd(void)
{
	FILE *f;
	char *servers, *ptr;
	int servers_len = 0, ntp_updates_int = 0, index = 2, off, i;
	char *ntpd_argv[] = { "/usr/sbin/ntpd", "-t", NULL, NULL, NULL, NULL, NULL, NULL }; /* -ddddddd -q -S /sbin/ntpd_synced -l */
	char cmd[256];

	if (serialize_restart("ntpd", 1))
		return;

	set_tz();

	if ((nvram_get_int("dnscrypt_proxy")) || (nvram_get_int("stubby_proxy")))
		eval("ntp2ip");

	/* this is the nvram var defining how the server should be run / how often to sync */
	ntp_updates_int = nvram_get_int("ntp_updates");

	/* the FreshTomato GUI allows the user to select an NTP Server region, and then string concats 1. 2. and 3. as prefix
	 * therefore, the nvram variable contains a string of 3 NTP servers - This code separates them and passes them to
	 * ntpd as separate parameters. this code should continue to work if GUI is changed to only store 1 value in the NVRAM var
	 */
	if (ntp_updates_int >= 0) { /* -1 = never */
		servers_len = strlen(nvram_safe_get("ntp_server"));

		/* allocating memory dynamically both so we don't waste memory, and in case of unanticipatedly long server name in nvram */
		if ((servers = malloc(servers_len + 1)) == NULL) {
			logmsg(LOG_ERR, "ntpd: failed allocating memory, exiting");
			return; /* just get out if we couldn't allocate memory */
		}
		memset(servers, 0, servers_len + 1);

		/* get the space separated list of ntp servers */
		strlcpy(servers, nvram_safe_get("ntp_server"), servers_len + 1);

		/* put the servers into the ntp config file */
		if ((f = fopen("/etc/ntp.conf", "w")) != NULL) {
			ptr = strtok(servers, " ");
			while(ptr) {
				fprintf(f, "server %s\n", ptr);
				ptr = strtok(NULL, " ");
			}
			fclose(f);
		}
		else {
			logerr(__FUNCTION__, __LINE__, "/etc/ntp.conf");
			return;
		}

		free(servers);

		if (nvram_contains_word("log_events", "ntp")) /* add verbose (doesn't work right now) */
			ntpd_argv[index++] = "-ddddddd";

		if (ntp_updates_int == 0) /* only at startup, then quit */
			ntpd_argv[index++] = "-q";
		else if (ntp_updates_int >= 1) { /* auto adjusted timing by ntpd since it doesn't currently implement minpoll and maxpoll */
			ntpd_argv[index++] = "-S";
			ntpd_argv[index++] = "/sbin/ntpd_synced";

			if (nvram_get_int("ntpd_enable")) /* enable local NTP server */
				ntpd_argv[index++] = "-l";
		}

		memset(cmd, 0, sizeof(cmd)); /* reset */
		off = snprintf(cmd, sizeof(cmd), "sh -c 'ulimit -c 0 -e 15 -r 15 -l 64 -m 4096 -n 512 -s 4096 -u 2 -v 4096; %s", ntpd_argv[0]);
		for (i = 1; ntpd_argv[i]; ++i)
			off += snprintf(cmd + off, sizeof(cmd) - off, " %s", ntpd_argv[i]);

		snprintf(cmd + off, sizeof(cmd) - off, "'");
		system(cmd);

		if (!nvram_contains_word("debug_norestart", "ntpd"))
			pid_ntpd = -2;

		sleep(1);
		if (pidof("ntpd") > 0)
			logmsg(LOG_INFO, "ntpd is started");
		else
			logmsg(LOG_ERR, "starting ntpd failed ...");
	}
}

void stop_ntpd(void)
{
	if (serialize_restart("ntpd", 0))
		return;

	pid_ntpd = -1;
	if (pidof("ntpd") > 0) {
		killall_tk_period_wait("ntpd", 50);
		logmsg(LOG_INFO, "ntpd is stopped");
	}
}

int ntpd_synced_main(int argc, char *argv[])
{
	if (!nvram_match("ntp_ready", "1") && (argc == 2 && !strcmp(argv[1], "step"))) {
		nvram_set("ntp_ready", "1");
		logmsg(LOG_INFO, "initial clock set");

		stop_httpd();
		start_httpd();
		start_sched();
		stop_ddns();
		start_ddns();
#ifdef TCONFIG_DNSCRYPT
		stop_dnscrypt();
		start_dnscrypt();
#endif
#ifdef TCONFIG_STUBBY
		stop_stubby();
		start_stubby();
#endif
#ifdef TCONFIG_DNSSEC
		if (nvram_get_int("dnssec_enable"))
			reload_dnsmasq();
#endif
#ifdef TCONFIG_OPENVPN
		start_ovpn_eas();
#endif
#ifdef TCONFIG_WIREGUARD
		start_wg_eas();
#endif
#ifdef TCONFIG_MDNS
		stop_mdns();
		start_mdns();
#endif
	}

	FILE *file;
	char message[300];
	char *stratum = safe_getenv("stratum");
	char *offset = safe_getenv("offset");
	char *freq_drift_ppm = safe_getenv("freq_drift_ppm");
	char *poll_interval = safe_getenv("poll_interval");
	char *server_hostname = safe_getenv("server_hostname");
	char *server_ip = safe_getenv("server_ip");
	char *discipline_jitter = safe_getenv("discipline_jitter");

	snprintf(message, sizeof(message), "Server: %s (%s)\n"
					   "Poll Interval: %ss\n"
					   "Stratum: %s\n"
					   "Offset: %ss\n"
					   "Jitter: %ss\n"
					   "Frequency: %sppm\n",
					    server_ip, server_hostname,
					    poll_interval,
					    stratum,
					    offset,
					    discipline_jitter,
					    freq_drift_ppm);

	if (!(file = fopen("/tmp/ntpd", "w"))) {
		return 1;
	}

	fprintf(file,"%s", message);
	fclose(file);
	return 0;
}

static void stop_rstats(void)
{
	int n, m;
	pid_t pid, pidz, ppidz;
	int w = 0;

	n = 60;
	m = 15;
	while ((n-- > 0) && ((pid = pidof("rstats")) > 0)) {
		w = 1;
		pidz = pidof("gzip");
		if (pidz < 0)
			pidz = pidof("cp");

		ppidz = ppid(ppid(pidz));
		if ((m > 0) && (pidz > 0) && (pid == ppidz)) {
			logmsg(LOG_DEBUG, "*** %s: (PID %d) shutting down, waiting for helper process to complete (PID %d, PPID %d)", __FUNCTION__, pid, pidz, ppidz);
			--m;
		}
		else
			kill(pid, SIGTERM);

		sleep(1);
	}
	if ((w == 1) && (n > 0))
		logmsg(LOG_INFO, "rstats stopped");
}

static void start_rstats(int new)
{
	if (nvram_get_int("rstats_enable")) {
		add_rstats_defaults(); /* backup: check nvram! */
		stop_rstats();
		if (new)
			xstart("rstats", "--new");
		else
			xstart("rstats");

		logmsg(LOG_INFO, "starting rstats%s", (new ? " (new datafile)" : ""));
	}
}

static void stop_cstats(void)
{
	int n, m;
	pid_t pid, pidz, ppidz;
	int w = 0;

	n = 60;
	m = 15;
	while ((n-- > 0) && ((pid = pidof("cstats")) > 0)) {
		w = 1;
		pidz = pidof("gzip");
		if (pidz < 0)
			pidz = pidof("cp");

		ppidz = ppid(ppid(pidz));
		if ((m > 0) && (pidz > 0) && (pid == ppidz)) {
			logmsg(LOG_DEBUG, "*** %s: (PID %d) shutting down, waiting for helper process to complete (PID %d, PPID %d)", __FUNCTION__, pid, pidz, ppidz);
			--m;
		}
		else
			kill(pid, SIGTERM);

		sleep(1);
	}
	if ((w == 1) && (n > 0))
		logmsg(LOG_INFO, "cstats stopped");
}

static void start_cstats(int new)
{
	if (nvram_get_int("cstats_enable")) {
		add_cstats_defaults(); /* backup: check nvram! */
		stop_cstats();
		if (new)
			xstart("cstats", "--new");
		else
			xstart("cstats");

		logmsg(LOG_INFO, "starting cstats%s", (new ? " (new datafile)" : ""));
	}
}

#ifdef TCONFIG_MEDIA_SERVER
static void start_media_server(int force)
{
	FILE *f;
	int port, https;
	pid_t pid;
	char *dbdir;
	char *argv[] = { "minidlna", "-f", "/etc/minidlna.conf", "-r", "-P", "/var/run/minidlna.pid", NULL, NULL };
	static int once = 1;
	int ret, index = 4, i;
	char *msi;
	unsigned char ea[ETHER_ADDR_LEN];
	char serial[18], uuident[37];
	char buffer[32], buffer2[8], buffer3[32];
	char *buf, *p, *q;
	char *path, *restricted;

	/* only if enabled or forced */
	if (!nvram_get_int("ms_enable") && force == 0)
		return;

	if (serialize_restart("minidlna", 1))
		return;

	if (!nvram_get_int("ms_sas")) { /* scan media at startup? */
		once = 0;
		argv[index - 1] = NULL;
	}
	else if (!once) /* already scanned */
		argv[index - 1] = NULL;

	if (nvram_get_int("ms_rescan")) { /* rescan on the next run? */
		argv[index - 1] = "-R";
		nvram_unset("ms_rescan");
	}

	if (f_exists("/etc/minidlna.alt"))
		argv[2] = "/etc/minidlna.alt";
	else {
		if ((f = fopen(argv[2], "w")) != NULL) {
			port = nvram_get_int("ms_port");
			https = nvram_get_int("https_enable");
			msi = nvram_safe_get("ms_ifname");
			dbdir = nvram_safe_get("ms_dbdir");
			if (!(*dbdir))
				dbdir = NULL;

			mkdir_if_none(dbdir ? : "/var/lib/minidlna");

			/* persistent ident (router's mac as serial) */
			if (!ether_atoe(nvram_safe_get("lan_hwaddr"), ea))
				gen_urandom(NULL, ea, ETHER_ADDR_LEN, 0);

			snprintf(serial, sizeof(serial), "%02x:%02x:%02x:%02x:%02x:%02x", ea[0], ea[1], ea[2], ea[3], ea[4], ea[5]);
			snprintf(uuident, sizeof(uuident), "4d696e69-444c-164e-9d41-%02x%02x%02x%02x%02x%02x", ea[0], ea[1], ea[2], ea[3], ea[4], ea[5]);

			if (strlen(msi)) {
				memset(buffer3, 0, sizeof(buffer3)); /* reset */
				for (i = 0; i < BRIDGE_COUNT; i++) {
					memset(buffer, 0, sizeof(buffer)); /* reset */
					snprintf(buffer, sizeof(buffer), (i == 0 ? "lan_ifname" : "lan%d_ifname"), i);
					memset(buffer2, 0, sizeof(buffer2)); /* reset */
					snprintf(buffer2, sizeof(buffer2), "br%d", i);
					if ((strlen(nvram_safe_get(buffer)) > 0) && (strstr(msi, buffer2) != NULL)) { /* bridge is up & present in 'ms_ifname' */
						if (strlen(buffer3) > 0)
							strlcat(buffer3, ",", sizeof(buffer3));

						strlcat(buffer3, buffer2, sizeof(buffer3));
					}
				}
				msi = buffer3;
			}

			fprintf(f, "network_interface=%s\n"
			           "port=%d\n"
			           "friendly_name=Tomato64 DLNA Server\n"
			           "db_dir=%s/.db\n"
			           "enable_tivo=%s\n"
			           "strict_dlna=%s\n"
			           "presentation_url=http%s://%s:%s/nas-media.asp\n"
			           "inotify=%s\n"
			           "notify_interval=600\n"
			           "album_art_names=Cover.jpg/cover.jpg/AlbumArtSmall.jpg/albumartsmall.jpg/AlbumArt.jpg/albumart.jpg/Album.jpg/album.jpg/Folder.jpg/folder.jpg/Thumb.jpg/thumb.jpg\n"
			           "log_dir=/var/log\n"
			           "log_level=general,artwork,database,inotify,scanner,metadata,http,ssdp,tivo=warn\n"
			           "serial=%s\n"
			           "uuid=%s\n"
			           "model_name=Windows Media Connect compatible (MiniDLNA)\n"
			           "model_number=%s\n\n"
			           "# Custom config\n"
			           "%s\n",
			           strlen(msi) ? msi : nvram_safe_get("lan_ifname"),
			           (port < 0) || (port >= 0xffff) ? 0 : port, /* 0 - means random port (feature applied as minidlna patch) */
			           dbdir ? : "/var/lib/minidlna",
			           nvram_get_int("ms_tivo") ? "yes" : "no",
			           nvram_get_int("ms_stdlna") ? "yes" : "no",
			           https ? "s" : "", nvram_safe_get("lan_ipaddr"), nvram_safe_get(https ? "https_lanport" : "http_lanport"),
			           nvram_get_int("ms_autoscan") ? "yes" : "no",
			           serial,
			           uuident,
			           tomato_version,
			           nvram_safe_get("ms_custom"));

			/* media directories */
			if ((buf = strdup(nvram_safe_get("ms_dirs"))) && (*buf)) {
				/* path<restricted[A|V|P|] */
				p = buf;
				while ((q = strsep(&p, ">")) != NULL) {
					if ((vstrsep(q, "<", &path, &restricted) < 1) || (!path) || (!*path))
						continue;

					fprintf(f, "media_dir=%s%s%s\n",
						restricted ? : "", (restricted && *restricted) ? "," : "", path);
				}
				free(buf);
			}
			fclose(f);
		}
		else {
			logerr(__FUNCTION__, __LINE__, argv[2]);
			return;
		}
	}

	if (nvram_get_int("ms_debug"))
		argv[index++] = "-v";

	ret = _eval(argv, NULL, 0, &pid);
	sleep(1);

	if ((pidof("minidlna") > 0) && !ret)
		once = 0;
	else
		logmsg(LOG_ERR, "starting minidlna failed ...");
}

static void stop_media_server(void)
{
	if (serialize_restart("minidlna", 0))
		return;

	if (pidof("minidlna") > 0)
		killall_tk_period_wait("minidlna", 50);

	/* clean-up */
	eval("rm", "-rf", "/var/run/minidlna");
}
#endif /* TCONFIG_MEDIA_SERVER */

#ifdef TCONFIG_HAVEGED
void start_haveged(void)
{
	pid_t pid;

	if (serialize_restart("haveged", 1))
		return;

	char *cmd_argv[] = { "haveged",
	                     "-r", "0",             /* 0 = run as daemon */
	                     "-w", "1024",          /* write_wakeup_threshold [bits] */
#ifndef TOMATO64
#ifdef TCONFIG_BCMARM /* it has to be checkd for all MIPS routers */
	                     "-d", "32",            /* data cache size [KB] - fallback to 16 */
	                     "-i", "32",            /* instruction cache size [KB] - fallback to 16 */
#endif
#endif /* TOMATO64 */
	                     NULL };

	_eval(cmd_argv, NULL, 0, &pid);
}

void stop_haveged(void)
{
	if (serialize_restart("haveged", 0))
		return;

	if (pidof("haveged") > 0)
		killall_tk_period_wait("haveged", 50);
}
#endif /* TCONFIG_HAVEGED */

#ifdef TCONFIG_USB
static void start_nas_services(void)
{
	if (nvram_get_int("g_upgrade") || nvram_get_int("g_reboot"))
		return;

	if (getpid() != 1) {
		start_service("usbapps");
		return;
	}

#ifdef TCONFIG_SAMBASRV
	start_samba(0);
#endif
#ifdef TCONFIG_FTP
	start_ftpd(0);
#endif
#ifdef TCONFIG_MEDIA_SERVER
	start_media_server(0);
#endif
}

static void stop_nas_services(void)
{
	if (getpid() != 1) {
		stop_service("usbapps");
		return;
	}

#ifdef TCONFIG_MEDIA_SERVER
	stop_media_server();
#endif
#ifdef TCONFIG_FTP
	stop_ftpd();
#endif
#ifdef TCONFIG_SAMBASRV
	stop_samba();
#endif
}

void restart_nas_services(int stop, int start)
{
	int fd = file_lock("usb");
	/* restart all NAS applications */
	if (stop)
		stop_nas_services();
	if (start)
		start_nas_services();

	file_unlock(fd);
}
#endif /* TCONFIG_USB */

/* -1 = Don't check for this program, it is not expected to be running.
 * Other = This program has been started and should be kept running.  If no
 * process with the name is running, call func to restart it.
 * Note: At startup, dnsmasq forks a short-lived child which forks a
 * long-lived (grand)child.  The parents terminate.
 * Many daemons use this technique.
 */
static void _check(pid_t pid, const char *name, void (*func)(void))
{
	if (pid == -1)
		return;

	if (pidof(name) > 0)
		return;

	logmsg(LOG_ERR, "%s terminated unexpectedly, restarting", name);
	func();

	/* force recheck in 500 msec */
	setitimer(ITIMER_REAL, &pop_tv, NULL);
}

void check_services(void)
{
	/* periodically reap any zombies */
	setitimer(ITIMER_REAL, &zombie_tv, NULL);

	/* do not restart if upgrading/rebooting */
	if (!nvram_get_int("g_upgrade") && !nvram_get_int("g_reboot")) {
		_check(pid_hotplug2, "hotplug2", start_hotplug2);

		if (!nvram_get_int("dnsmasq_norestart"))
			_check(pid_dnsmasq, "dnsmasq", start_dnsmasq);

		_check(pid_crond, "crond", start_cron);
		_check(pid_igmp, "igmpproxy", start_igmp_proxy);

		if (nvram_get_int("ntp_updates") >= 1)
			_check(pid_ntpd, "ntpd", start_ntpd);
	}
}

void start_services(void)
{
	static int once = 1;

#ifdef TOMATO64_WIFI
	start_wifi();
#endif /* TOMATO64_WIFI */
#ifdef TCONFIG_HAVEGED
	start_haveged();
#endif
	if (once) {
		once = 0;

		if (nvram_get_int("telnetd_eas"))
			start_telnetd();
		if (nvram_get_int("sshd_eas"))
			start_sshd();
	}
	start_dhcpc_lan(); /* start very early */
#ifndef TOMATO64
	start_nas();
#endif /* TOMATO64 */
#ifdef TCONFIG_ZEBRA
	start_zebra();
#endif
#ifdef TCONFIG_SDHC
	start_mmc();
#endif
	start_dnsmasq();
#ifdef TCONFIG_MDNS
	start_mdns();
#endif
	start_cifs();
	start_httpd();
#ifdef TCONFIG_NGINX
	start_nginx(0);
	start_mysql(0);
#endif
	start_cron();
#ifdef TCONFIG_PPTPD
	start_pptpd(0);
#endif
#ifdef TCONFIG_USB
	restart_nas_services(1, 1); /* Samba, FTP and Media Server */
	notice_set("nas", "" );
#endif
#ifdef TCONFIG_SNMP
	start_snmp();
#endif
	start_tomatoanon();
#ifdef TCONFIG_TOR
	start_tor(0);
#endif
#ifdef TCONFIG_BT
	start_bittorrent(0);
#endif
#ifdef TCONFIG_NOCAT
	start_nocat();
#endif
#ifdef TCONFIG_NFS
	start_nfs();
#endif
#ifdef TCONFIG_BCMARM
	/* do LED setup for Router */
	led_setup();
#endif
	start_rstats(0);
	start_cstats(0);
#ifdef TCONFIG_FANCTRL
	start_phy_tempsense();
#endif
#if 0 /* see load_wl() for dhd_msg_level */
#ifdef TCONFIG_BCM7
	if (!nvram_get_int("debug_wireless")) { /* suppress dhd debug messages (default 0x01) */
		system("/usr/sbin/dhd -i eth1 msglevel 0x00");
		system("/usr/sbin/dhd -i eth2 msglevel 0x00");
		system("/usr/sbin/dhd -i eth3 msglevel 0x00");
	}
#endif
#endif
#ifdef TCONFIG_BCMBSD
	start_bsd();
#endif
#ifdef TCONFIG_ROAM
	start_roamast();
#endif
#ifdef TCONFIG_IRQBALANCE
	start_irqbalance();
#endif
}

void stop_services(void)
{
	stop_dhcpc_lan(); /* stop very early */
	clear_resolv();
	stop_rstats();
	stop_cstats();
#ifdef TCONFIG_FANCTRL
	stop_phy_tempsense();
#endif
#ifdef TCONFIG_BT
	stop_bittorrent();
#endif
#ifdef TCONFIG_NOCAT
	stop_nocat();
#endif
#ifdef TCONFIG_SNMP
	stop_snmp();
#endif
#ifdef TCONFIG_TOR
	stop_tor();
#endif
	stop_tomatoanon();
#ifdef TCONFIG_NFS
	stop_nfs();
#endif
#ifdef TCONFIG_MDNS
	stop_mdns();
#endif
#ifdef TCONFIG_USB
	restart_nas_services(1, 0); /* Samba, FTP and Media Server */
#endif
#ifdef TCONFIG_PPTPD
	stop_pptpd();
#endif
	stop_sched();
	stop_cron();
#ifdef TCONFIG_NGINX
	stop_mysql();
	stop_nginx();
#endif
#ifdef TCONFIG_SDHC
	stop_mmc();
#endif
	stop_cifs();
	stop_httpd();
	stop_dnsmasq();
#ifdef TCONFIG_ZEBRA
	stop_zebra();
#endif
#ifndef TOMATO64
	stop_nas();
#endif /* TOMATO64 */
#ifdef TCONFIG_BCMBSD
	stop_bsd();
#endif
#ifdef TCONFIG_ROAM
	stop_roamast();
#endif
#ifdef TCONFIG_IRQBALANCE
	stop_irqbalance();
#endif
#ifdef TCONFIG_HAVEGED
	stop_haveged();
#endif
#ifdef TOMATO64_WIFI
	stop_wifi();
#endif /* TOMATO64_WIFI */
}

/* nvram "action_service" is: "service-action[-modifier]"
 * action is something like "stop" or "start" or "restart"
 * optional modifier is "c" for the "service" command-line command
 */
void exec_service(void)
{
	const int A_START = 1;
	const int A_STOP = 2;
	const int A_RESTART = 1|2;
	char buffer[128], buffer2[16], buffer3[16];
	char *service;
	char *act;
	char *next;
	char *modifier;
	int action, user;
	int i;
	int act_start, act_stop;

	strlcpy(buffer, nvram_safe_get("action_service"), sizeof(buffer));
	next = buffer;

TOP:
	act = strsep(&next, ",");
	service = strsep(&act, "-");
	if (act == NULL) {
		next = NULL;
		goto CLEAR;
	}
	modifier = act;
	action = 0;
	strsep(&modifier, "-");

	logmsg(LOG_DEBUG, "*** %s: service=%s action=%s modifier=%s", __FUNCTION__, service, act, modifier ? : "");

	if (strcmp(act, "start") == 0)
		action = A_START;
	if (strcmp(act, "stop") == 0)
		action = A_STOP;
	if (strcmp(act, "restart") == 0)
		action = A_RESTART;

	act_start = action & A_START;
	act_stop  = action & A_STOP;

	user = (modifier != NULL && *modifier == 'c');

	if (strcmp(service, "rstats_nvram") == 0) {
		if (act_stop) del_rstats_defaults();
		if (act_start) add_rstats_defaults();
		goto CLEAR;
	}

	if (strcmp(service, "cstats_nvram") == 0) {
		if (act_stop) del_cstats_defaults();
		if (act_start) add_cstats_defaults();
		goto CLEAR;
	}

#ifdef TCONFIG_FTP
	if (strcmp(service, "ftp_nvram") == 0) {
		if (act_stop) del_ftp_defaults();
		if (act_start) add_ftp_defaults();
		goto CLEAR;
	}
#endif /* TCONFIG_FTP */

#ifdef TCONFIG_SNMP
	if (strcmp(service, "snmp_nvram") == 0) {
		if (act_stop) del_snmp_defaults();
		if (act_start) add_snmp_defaults();
		goto CLEAR;
	}
#endif /* TCONFIG_SNMP */

	if (strcmp(service, "upnp_nvram") == 0) {
		if (act_stop) del_upnp_defaults();
		if (act_start) add_upnp_defaults();
		goto CLEAR;
	}

#ifdef TCONFIG_BCMBSD
	if (strcmp(service, "bsd_nvram") == 0) {
		if (act_stop) del_bsd_defaults();
		if (act_start) add_bsd_defaults();
		goto CLEAR;
	}
#endif /* TCONFIG_BCMBSD */

	for (i = 1; i <= MWAN_MAX; i++) {
		memset(buffer2, 0, sizeof(buffer2));
		snprintf(buffer2, sizeof(buffer2), (i == 1 ? "dhcpc_wan" : "dhcpc_wan%d"), i);
		if (strcmp(service, buffer2) == 0) {
			memset(buffer2, 0, sizeof(buffer2));
			snprintf(buffer2, sizeof(buffer2), (i == 1 ? "wan" : "wan%d"), i);
			if (act_stop) stop_dhcpc(buffer2);
			if (act_start) start_dhcpc(buffer2);
			goto CLEAR;
		}
	}

	if (strcmp(service, "dnsmasq") == 0) {
		if (act_stop) stop_dnsmasq();
		if (act_start && !nvram_get_int("g_upgrade")) {
			dns_to_resolv();
			start_dnsmasq();
		}
		goto CLEAR;
	}

	if (strcmp(service, "dns") == 0) {
		if (act_start) reload_dnsmasq();
		goto CLEAR;
	}

#ifdef TCONFIG_DNSCRYPT
	if ((strcmp(service, "dnscrypt") == 0) || (strcmp(service, "dnscrypt_proxy") == 0)) {
		if (act_stop) stop_dnscrypt();
		if (act_start) start_dnscrypt();
		goto CLEAR;
	}
#endif

#ifdef TCONFIG_STUBBY
	if (strcmp(service, "stubby") == 0) {
		if (act_stop) stop_stubby();
		if (act_start) start_stubby();
		goto CLEAR;
	}
#endif

#ifdef TCONFIG_MDNS
	if ((strcmp(service, "mdns") == 0) || (strcmp(service, "avahi_daemon") == 0)) {
		if (act_stop) stop_mdns();
		if (act_start) start_mdns();
		goto CLEAR;
	}
#endif

#ifdef TCONFIG_IRQBALANCE
	if (strcmp(service, "irqbalance") == 0) {
		if (act_stop) stop_irqbalance();
		if (act_start) start_irqbalance();
		goto CLEAR;
	}
#endif

#ifdef TCONFIG_HAVEGED
	if (strcmp(service, "haveged") == 0) {
		if (act_stop) stop_haveged();
		if (act_start) start_haveged();
		goto CLEAR;
	}
#endif

	if (strcmp(service, "adblock") == 0) {
		if (act_stop) stop_adblock();
		if (act_start) start_adblock(1); /* update lists immediately */
		goto CLEAR;
	}

	if (strcmp(service, "firewall") == 0) {
		if (act_stop) {
			stop_firewall();
			stop_igmp_proxy();
			stop_udpxy();
		}
		if (act_start) {
			start_firewall();
			start_igmp_proxy();
			start_udpxy();
		}
		goto CLEAR;
	}

	if (strcmp(service, "restrict") == 0) {
		if (act_stop)
			stop_firewall();

		if (act_start) {
			i = nvram_get_int("rrules_radio"); /* -1 = not used, 0 = enabled by rule, 1 = disabled by rule */

			start_firewall();

			/* if radio was disabled by access restriction, but no rule is handling it now, enable it */
			if (i == 1) {
				if (nvram_get_int("rrules_radio") < 0)
					eval("radio", "on");
			}
		}
		goto CLEAR;
	}

	if (strcmp(service, "arpbind") == 0) {
		if (act_stop) stop_arpbind();
		if (act_start) start_arpbind();
		goto CLEAR;
	}

	if (strcmp(service, "bwlimit") == 0) {
		if (act_stop) {
			stop_bwlimit();
#ifdef TCONFIG_NOCAT
			stop_nocat();
#endif
		}
		if (act_start) {
			start_bwlimit();
#ifdef TCONFIG_NOCAT
			start_nocat();
#endif
		}
		restart_firewall(); /* always restart */
		goto CLEAR;
	}

	if (strcmp(service, "qos") == 0) {
		if (act_stop) {
			for (i = 1; i <= MWAN_MAX; i++) {
				memset(buffer2, 0, sizeof(buffer2));
				snprintf(buffer2, sizeof(buffer2), (i == 1 ? "wan" : "wan%d"), i);
				stop_qos(buffer2);
			}
		}
		if (act_start) {
			for (i = 1; i <= MWAN_MAX; i++) {
				memset(buffer2, 0, sizeof(buffer2));
				snprintf(buffer2, sizeof(buffer2), (i == 1 ? "wan" : "wan%d"), i);
				if ((check_wanup(buffer2)) || (i == 1))
					start_qos(buffer2);
			}
			if (nvram_get_int("qos_reset"))
				f_write_string("/proc/net/clear_marks", "1", 0, 0);
		}
		restart_firewall(); /* always restart */
		goto CLEAR;
	}

	if ((strcmp(service, "upnp") == 0) || (strcmp(service, "miniupnpd") == 0)) {
		if (act_stop) stop_upnp();
		if (act_start) start_upnp();
		restart_firewall(); /* always restart */
		goto CLEAR;
	}

	if (strcmp(service, "telnetd") == 0) {
		if (act_stop) stop_telnetd();
		if (act_start) start_telnetd();
		goto CLEAR;
	}

	if (strcmp(service, "sshd") == 0 || strcmp(service, "dropbear") == 0) {
		if (act_stop) stop_sshd();
		if (act_start) start_sshd();
		goto CLEAR;
	}

	if (strcmp(service, "httpd") == 0) {
		if (act_stop) stop_httpd();
		if (act_start) start_httpd();
		goto CLEAR;
	}

#ifdef TCONFIG_IPV6
	if (strcmp(service, "dhcp6") == 0) {
		if (act_stop) stop_dhcp6c();
		if (act_start) start_dhcp6c();
		goto CLEAR;
	}
#endif

	if (strncmp(service, "admin", 5) == 0) {
		if (act_stop) {
			if (!(strcmp(service, "adminnosshd") == 0))
				stop_sshd();
			stop_telnetd();
			stop_httpd();
		}
		if (act_start) {
			stop_httpd();
			start_httpd();
			if (!(strcmp(service, "adminnosshd") == 0))
				create_passwd();
			if (nvram_get_int("telnetd_eas"))
				start_telnetd();
			if (nvram_get_int("sshd_eas") && (!(strcmp(service, "adminnosshd") == 0)))
				start_sshd();
		}
		restart_firewall(); /* always restart */
		goto CLEAR;
	}

	if (strcmp(service, "ddns") == 0) {
		if (act_stop) stop_ddns();
		if (act_start) start_ddns();
		goto CLEAR;
	}

	if (strcmp(service, "ntpd") == 0) {
		if (act_stop) stop_ntpd();
		if (act_start) start_ntpd();
		goto CLEAR;
	}

	if (strcmp(service, "logging") == 0) {
		if (act_stop) stop_syslog();
		if (act_start) start_syslog();
		if (!user) {
			/* always restarted except from "service" command */
			stop_cron();
			start_cron();
			restart_firewall();
		}
		goto CLEAR;
	}

	if (strcmp(service, "crond") == 0) {
		if (act_stop) stop_cron();
		if (act_start) start_cron();
		goto CLEAR;
	}

	if (strcmp(service, "hotplug") == 0) {
		if (act_stop) stop_hotplug2();
		if (act_start) start_hotplug2();
		goto CLEAR;
	}

	if (strcmp(service, "upgrade") == 0) {
		if (act_start) {
			nvram_set("g_upgrade", "1");

			if (nvram_get_int("webmon_bkp"))
				xstart("/usr/sbin/webmon_bkp", "hourly"); /* make a copy before upgrade */

			stop_sched();
			stop_cron();
#ifdef TCONFIG_NGINX
			stop_mysql();
			stop_nginx();
#endif
#ifdef TCONFIG_NFS
			stop_nfs();
#endif
#ifdef TCONFIG_USB
			restart_nas_services(1, 0); /* Samba, FTP and Media Server */
#endif
#ifdef TCONFIG_BT
			stop_bittorrent();
#endif
#ifdef TCONFIG_NOCAT
			stop_nocat();
#endif
#ifdef TCONFIG_TOR
			stop_tor();
#endif
			killall("rstats", SIGTERM);
			killall("cstats", SIGTERM);
			killall("buttons", SIGTERM);
			stop_upnp();
			if (!nvram_get_int("remote_upgrade")) {
				killall("xl2tpd", SIGTERM);
				killall("pppd", SIGTERM);
				stop_dnsmasq();
				killall("udhcpc", SIGTERM);
				stop_wan();
			} else
				stop_adblock();

			stop_tomatoanon();
			remove_conntrack();
#ifdef TCONFIG_ZEBRA
			stop_zebra();
#endif
#ifdef TCONFIG_IRQBALANCE
			stop_irqbalance();
#endif
#ifdef TCONFIG_MDNS
			stop_mdns();
#endif
#ifdef TCONFIG_HAVEGED
			stop_haveged();
#endif
			stop_jffs2();
			stop_syslog();
			sleep(1);
#ifdef TCONFIG_USB
#ifdef TCONFIG_USBAP
			stop_wireless();
			sleep(1);
#endif
			remove_storage_main(1);
			stop_usb();
#endif /* TCONFIG_USB */
		}
		goto CLEAR;
	}

#ifdef TCONFIG_CIFS
	if (strcmp(service, "cifs") == 0) {
		if (act_stop) stop_cifs();
		if (act_start) start_cifs();
		goto CLEAR;
	}
#endif

#ifdef TCONFIG_JFFS2
	if (strncmp(service, "jffs", 4) == 0) { /* could be jffs/jffs2 */
		if (act_stop) stop_jffs2();
		if (act_start) start_jffs2();
		goto CLEAR;
	}
#endif

#ifdef TCONFIG_ZEBRA
	if (strcmp(service, "zebra") == 0) {
		if (act_stop) stop_zebra();
		if (act_start) start_zebra();
		goto CLEAR;
	}
#endif

#ifdef TCONFIG_SDHC
	if (strcmp(service, "mmc") == 0) {
		if (act_stop) stop_mmc();
		if (act_start) start_mmc();
		goto CLEAR;
	}
#endif

	if (strcmp(service, "routing") == 0) {
		if (act_stop) {
#ifdef TCONFIG_ZEBRA
			stop_zebra();
#endif
			do_static_routes(0); /* remove old '_saved' */
			for (i = 0; i < BRIDGE_COUNT; i++) {
				memset(buffer2, 0, sizeof(buffer2));
				snprintf(buffer2, sizeof(buffer2), (i == 0 ? "lan_ifname" : "lan%d_ifname"), i);
				if ((i == 0) || (strcmp(nvram_safe_get(buffer2), "") != 0))
					eval("brctl", "stp", nvram_safe_get(buffer2), "0");
			}
		}
		if (act_start) {
			do_static_routes(1); /* add new */
#ifdef TCONFIG_ZEBRA
			start_zebra();
#endif
			for (i = 0; i < BRIDGE_COUNT; i++) {
				memset(buffer2, 0, sizeof(buffer2));
				snprintf(buffer2, sizeof(buffer2), (i == 0 ? "lan_ifname" : "lan%d_ifname"), i);
				if ((i == 0) || (strcmp(nvram_safe_get(buffer2), "") != 0)) {
					memset(buffer3, 0, sizeof(buffer3));
					snprintf(buffer3, sizeof(buffer3), (i == 0 ? "lan_stp" : "lan%d_stp"), i);
					eval("brctl", "stp", nvram_safe_get(buffer2), nvram_safe_get(buffer3));
				}
			}
		}
		restart_firewall(); /* always restart */
		goto CLEAR;
	}

	if (strcmp(service, "ctnf") == 0) {
		if (act_start) {
			setup_conntrack();
			restart_firewall(); /* always restart */
		}
		goto CLEAR;
	}

	if (strcmp(service, "wan") == 0) {
		if (act_stop) stop_wan();
		if (act_start) {
			rename("/tmp/ppp/wan_log", "/tmp/ppp/wan_log.~");
			start_wan();
			for (i = 1; i <= MWAN_MAX; i++) {
				memset(buffer2, 0, sizeof(buffer2));
				snprintf(buffer2, sizeof(buffer2), (i == 1 ? "wan" : "wan%d"), i);
				sleep(5);
				force_to_dial(buffer2);
			}
		}
		goto CLEAR;
	}

	for (i = 1; i <= MWAN_MAX; i++) {
		memset(buffer2, 0, sizeof(buffer2));
		snprintf(buffer2, sizeof(buffer2), "wan%d", i);
		if (strcmp(service, buffer2) == 0) {
			memset(buffer2, 0, sizeof(buffer2));
			snprintf(buffer2, sizeof(buffer2), (i == 1 ? "wan" : "wan%d"), i);
			if (act_stop) stop_wan_if(buffer2);
			if (act_start) {
				start_wan_if(buffer2);
				sleep(5);
				force_to_dial(buffer2);
			}
			goto CLEAR;
		}
	}

	if (strcmp(service, "net") == 0) {
		if (act_stop) {
#ifdef TCONFIG_USB
			stop_nas_services();
#endif
#ifdef TCONFIG_PPPRELAY
			stop_pppoerelay();
#endif
			stop_httpd();
#ifdef TOMATO64_WIFI
			stop_wifi();
#endif /* TOMATO64_WIFI */
#ifdef TCONFIG_MDNS
			stop_mdns();
#endif
			stop_dnsmasq();
#ifndef TOMATO64
			stop_nas();
#endif /* TOMATO64 */
			stop_wan();
			stop_arpbind();
			stop_lan();
			stop_vlan();
		}
		if (act_start) {
			start_vlan();
			start_lan();
			start_arpbind();
#ifndef TOMATO64
			start_nas();
#endif /* TOMATO64 */
			start_dnsmasq();
#ifdef TCONFIG_MDNS
			start_mdns();
#endif
			start_httpd();
#ifdef TOMATO64_WIFI
			start_wifi();
#endif /* TOMATO64_WIFI */
#ifndef TOMATO64
			start_wl();
#endif /* TOMATO64 */
#ifdef TCONFIG_USB
			start_nas_services();
#endif
			/* last one as ssh telnet httpd samba etc can fail to load until start_wan_done */
			start_wan();
		}
		goto CLEAR;
	}

#ifdef TOMATO64
	if (strcmp(service, "wifi") == 0) {
		if (act_stop) stop_wifi();
		if (act_start) start_wifi();
		goto CLEAR;
	}
#endif /* TOMATO64 */

#ifndef TOMATO64
	if ((strcmp(service, "wireless") == 0) || (strcmp(service, "wl") == 0)) { /* for tomato user --> 'service wl start' will restart wl allways (failsafe, even if wl was not stopped!) */
		if (act_stop) stop_wireless();
		if (act_start) restart_wireless();
		goto CLEAR;
	}

	if (strcmp(service, "wlgui") == 0) { /* for GUI to restart wireless (only stop wl once!) */
		if (act_stop) stop_wireless();
		if (act_start) start_wireless();
		goto CLEAR;
	}

	if (strcmp(service, "nas") == 0) {
		if (act_stop) stop_nas();
		if (act_start) {
			start_nas();
			start_wl();
		}
		goto CLEAR;
	}
#endif /* TOMATO64 */

#ifdef TCONFIG_BCMBSD
	if (strcmp(service, "bsd") == 0) {
		if (act_stop) stop_bsd();
		if (act_start) start_bsd();
		goto CLEAR;
	}
#endif /* TCONFIG_BCMBSD */

#ifdef TCONFIG_ROAM
	if ((strcmp(service, "roamast") == 0) || (strcmp(service, "rssi") == 0)) {
		if (act_stop) stop_roamast();
		if (act_start) start_roamast();
		goto CLEAR;
	}
#endif

	if (strncmp(service, "rstats", 6) == 0) {
		if (act_stop) stop_rstats();
		if (act_start) {
			if (strcmp(service, "rstatsnew") == 0)
				start_rstats(1);
			else
				start_rstats(0);
		}
		goto CLEAR;
	}

	if (strncmp(service, "cstats", 6) == 0) {
		if (act_stop) stop_cstats();
		if (act_start) {
			if (strcmp(service, "cstatsnew") == 0)
				start_cstats(1);
			else
				start_cstats(0);
		}
		goto CLEAR;
	}

	if (strcmp(service, "sched") == 0) {
		if (act_stop) stop_sched();
		if (act_start) start_sched();
		goto CLEAR;
	}

#ifdef TCONFIG_BT
	if ((strcmp(service, "bittorrent") == 0) || (strcmp(service, "transmission") == 0) || (strcmp(service, "transmission_da") == 0)) {
		if (act_stop) stop_bittorrent();
		if (act_start) start_bittorrent(1); /* force (re)start */
		goto CLEAR;
	}
#endif

#ifdef TCONFIG_NFS
	if ((strcmp(service, "nfs") == 0) || (strcmp(service, "nfsd") == 0)) {
		if (act_stop) stop_nfs();
		if (act_start) start_nfs();
		goto CLEAR;
	}
#endif

#ifdef TCONFIG_SNMP
	if (strcmp(service, "snmp") == 0) {
		if (act_stop) stop_snmp();
		if (act_start) start_snmp();
		goto CLEAR;
	}
#endif

#ifdef TCONFIG_TOR
	if (strcmp(service, "tor") == 0) {
		if (act_stop) stop_tor();
		if (act_start) start_tor(1); /* force (re)start */
		restart_firewall(); /* always restart */
		goto CLEAR;
	}
#endif

#ifdef TCONFIG_UPS
	if (strcmp(service, "ups") == 0) {
		if (act_stop) stop_ups();
		if (act_start) start_ups();
		goto CLEAR;
	}
#endif

	if (strcmp(service, "tomatoanon") == 0) {
		if (act_stop) stop_tomatoanon();
		if (act_start) start_tomatoanon();
		goto CLEAR;
	}

#ifdef TCONFIG_USB
	if (strcmp(service, "usb") == 0) {
		if (act_stop) stop_usb();
		if (act_start) {
			start_usb();
			/* restart Samba and ftp since they may be killed by stop_usb() */
			restart_nas_services(1, 1);
			/* remount all partitions by simulating hotplug event */
			add_remove_usbhost("-1", 1);
		}
		goto CLEAR;
	}

	if (strcmp(service, "usbapps") == 0) {
		if (act_stop) stop_nas_services();
		if (act_start) start_nas_services();
		goto CLEAR;
	}
#endif

#ifdef TCONFIG_FTP
	if ((strcmp(service, "ftpd") == 0) || (strcmp(service, "vsftpd") == 0)) {
		if (act_stop) stop_ftpd();
		setup_conntrack();
		if (act_start) start_ftpd(1); /* force (re)start */
		goto CLEAR;
	}
#endif

#ifdef TCONFIG_MEDIA_SERVER
	if ((strcmp(service, "media") == 0) || (strcmp(service, "minidlna") == 0)) {
		if (act_stop) stop_media_server();
		if (act_start) start_media_server(1); /* force (re)start */
		goto CLEAR;
	}
#endif

#ifdef TCONFIG_SAMBASRV
	if ((strcmp(service, "samba") == 0) || (strcmp(service, "smbd") == 0)) {
		if (act_stop) stop_samba();
		if (act_start) {
			create_passwd();
			stop_dnsmasq();
			start_dnsmasq();
			start_samba(1); /* force (re)start */
		}
		goto CLEAR;
	}
#endif

#ifdef TCONFIG_OPENVPN
	if (strncmp(service, "vpnclient", 9) == 0) {
		if (act_stop) stop_ovpn_client(atoi(&service[9]));
		if (act_start) start_ovpn_client(atoi(&service[9]));
		goto CLEAR;
	}

	if (strncmp(service, "vpnserver", 9) == 0) {
		if (act_stop) stop_ovpn_server(atoi(&service[9]));
		if (act_start) start_ovpn_server(atoi(&service[9]));
		goto CLEAR;
	}
#endif

#ifdef TCONFIG_WIREGUARD
	if (strncmp(service, "wireguard", 9) == 0) {
		if (act_stop) stop_wireguard(atoi(&service[9]));
		if (act_start) start_wireguard(atoi(&service[9]));
		goto CLEAR;
	}
#endif

#ifdef TCONFIG_TINC
	if ((strcmp(service, "tinc") == 0) || (strcmp(service, "tincd") == 0)) {
		if (act_stop) stop_tinc();
		if (act_start) start_tinc(1); /* force (re)start */
		goto CLEAR;
	}
#endif

#ifdef TCONFIG_FANCTRL
	if (strcmp(service, "fanctrl") == 0) {
		if (act_stop) stop_phy_tempsense();
		if (act_start) start_phy_tempsense();
		goto CLEAR;
	}
#endif

#ifdef TCONFIG_NOCAT
	if (strcmp(service, "splashd") == 0) {
		if (act_stop) stop_nocat();
		if (act_start) start_nocat();
		goto CLEAR;
	}
#endif

#ifdef TCONFIG_NGINX
	if (strcmp(service, "nginx") == 0) {
		if (act_stop) stop_nginx();
		if (act_start) start_nginx(1); /* force (re)start */
		goto CLEAR;
	}
	if ((strcmp(service, "mysql") == 0) || (strcmp(service, "mysqld") == 0)) {
		if (act_stop) stop_mysql();
		if (act_start) start_mysql(1); /* force (re)start */
		goto CLEAR;
	}
#endif

#ifdef TCONFIG_PPTPD
	if (strcmp(service, "pptpd") == 0) {
		if (act_stop) stop_pptpd();
		if (act_start) start_pptpd(1); /* force (re)start */
		goto CLEAR;
	}

	if (strcmp(service, "pptpclient") == 0) {
		if (act_stop) stop_pptp_client();
		if (act_start) start_pptp_client();
		goto CLEAR;
	}
#endif

	logmsg(LOG_WARNING, "no such service: %s", service);

CLEAR:
	if (next)
		goto TOP;

	/* some functions check action_service and must be cleared at end */
	nvram_set("action_service", "");

	/* force recheck in 500 msec */
	setitimer(ITIMER_REAL, &pop_tv, NULL);
}

static void do_service(const char *name, const char *action, int user)
{
	int n;
	char s[64], t[64];

	snprintf(t, sizeof(t), "%s", nvram_safe_get("action_service"));

	logmsg(LOG_DEBUG, "*** %s: IN name: %s action: %s user: %d", __FUNCTION__, name, action, user);

	n = 200;
	while (!nvram_match("action_service", "")) { /* wait until nvram 'action_service' is empty (max 20 seconds when not user, user can wait indefinitely [??]) */
		if (user) {
			putchar('*');
			fflush(stdout);
		}
		else if (--n < 0)
			break;

		usleep(100 * 1000); /* microseconds => 0,1s  */
	}

	snprintf(s, sizeof(s), "%s-%s%s", name, action, (user ? "-c" : ""));
	nvram_set("action_service", s); /* set new service to execute (for exec_service) */

	if (n < 190) /* log only above 1 sec */
		logmsg(LOG_DEBUG, "*** %s: waited %d second(s) for 'action_service' to be empty [%s] - [%s]", __FUNCTION__, ((200 - n) / 10), t, s);

	logmsg(LOG_DEBUG, "*** %s: setting new 'action_service': [%s]", __FUNCTION__, s);

	if (nvram_get_int("debug_rc_svc")) {
		nvram_unset("debug_rc_svc");
		exec_service();
	}
	else
		kill(1, SIGUSR1);

	n = 200;
	while (nvram_match("action_service", s)) { /* wait until nvram 'action_service' is not equal 'name' (max 20 seconds when not user, user can wait indefinitely[??]) */
		if (user) {
			putchar('.');
			fflush(stdout);
		}
		else if (--n < 0)
			break;

		usleep(100 * 1000); /* microseconds => 0,1s  */
	}

	if (n < 190) /* log only above 1 sec */
		logmsg(LOG_DEBUG, "*** %s: OUT waited %d second(s) for execution of 'action_service': [%s]", __FUNCTION__, ((200 - n) / 10), s);
}

int service_main(int argc, char *argv[])
{
	if (argc != 3)
		usage_exit(argv[0], "<service> <action>");

	do_service(argv[1], argv[2], 1);
	printf("\nDone.\n");

	return 0;
}

void start_service(const char *name)
{
	do_service(name, "start", 0);
}

void stop_service(const char *name)
{
	do_service(name, "stop", 0);
}

#ifdef TCONFIG_BCMBSD
int start_bsd(void)
{
	int ret;
	int bsd_enable = nvram_get_int("smart_connect_x");

	/* only if enabled */
	if (bsd_enable) {
		add_bsd_defaults(); /* add bsd nvram values only if feature is enabled! */

		/* band steering settings corrections, because 5 GHz module is the first one */
		switch (get_model()) {
			case MODEL_EA6350v1: /* EA6200 */
				if (nvram_match("boardnum", "20140309")) {
					/* nothing to do for EA6350v1 */
					break;
				}
				/* fall through */
			case MODEL_F9K1113v2:
			case MODEL_F9K1113v2_20X0: /* version 2000 and 2010 */
			case MODEL_R1D:
				nvram_set("wl1_bsd_steering_policy", "0 5 3 -52 0 110 0x22");
				nvram_set("wl0_bsd_steering_policy", "80 5 3 -82 0 0 0x20");
				nvram_set("wl1_bsd_sta_select_policy", "10 -52 0 110 0 1 1 0 0 0 0x122");
				nvram_set("wl0_bsd_sta_select_policy", "10 -82 0 0 0 1 1 0 0 0 0x20");
				nvram_set("wl1_bsd_if_select_policy", "eth1");
				nvram_set("wl0_bsd_if_select_policy", "eth2");
				nvram_set("wl1_bsd_if_qualify_policy", "0 0x0");
				nvram_set("wl0_bsd_if_qualify_policy", "60 0x0");
				break;
			default:
				/* nothing to do right now */
				break;
		}
	}

	stop_bsd();

	/* 0 = off, 1 = on (all-band), 2 = 5 GHz only! (no support, maybe later) */
	if (!bsd_enable) {
		ret = -1;
		logmsg(LOG_INFO, "wireless band steering disabled");
		return ret;
	}
	else
		ret = eval("/usr/sbin/bsd");

	if (ret)
		logmsg(LOG_ERR, "starting wireless band steering failed ...");
	else
		logmsg(LOG_INFO, "wireless band steering is started");

	return ret;
}

void stop_bsd(void)
{
	killall_tk_period_wait("bsd", 50);

	logmsg(LOG_INFO, "wireless band steering is stopped");
}
#endif /* TCONFIG_BCMBSD */

#ifdef TCONFIG_ROAM
#define TOMATO_WLIF_MAX 4

void stop_roamast(void)
{
	killall_tk_period_wait("roamast", 50);

	logmsg(LOG_INFO, "wireless roaming assistant is stopped");
}

void start_roamast(void)
{
	char *cmd[] = {"roamast", NULL};
	char prefix[] = "wl_XXXX";
	char tmp[32];
	pid_t pid;
	int i;

	stop_roamast();

	for (i = 0; i < TOMATO_WLIF_MAX; i++) {
		snprintf(prefix, sizeof(prefix), "wl%d_", i);
		if (nvram_get_int(strlcat_r(prefix, "user_rssi", tmp, sizeof(tmp))) != 0) {
			_eval(cmd, NULL, 0, &pid);
			logmsg(LOG_INFO, "wireless roaming assistant is started");
			break;
		}
	}
}
#endif /* TCONFIG_ROAM */
