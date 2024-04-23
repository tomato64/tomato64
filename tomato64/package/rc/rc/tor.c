/*
 * tor.c
 *
 * Copyright (C) 2011 shibby
 * Fixes/updates (C) 2018 - 2022 pedro
 *
 */


#include "rc.h"

#include <sys/stat.h>

#define tor_config	"/etc/tor.conf"


void start_tor(int force) {
	FILE *fp;
	char *ip;
	char buffer[16];
	int i;

	/* only if enabled or forced */
	if (!nvram_get_int("tor_enable") && force == 0)
		return;

	if (serialize_restart("tor", 1))
		return;

	/* dnsmasq uses this IP for nameserver to resolv .onion/.exit domains */
	ip = nvram_safe_get("lan_ipaddr");
	if (!nvram_get_int("tor_solve_only")) {
		for (i = 0 ; i < BRIDGE_COUNT; i++) {
			snprintf(buffer, sizeof(buffer), "br%d", i);
			if (nvram_match("tor_iface", buffer)) {
				snprintf(buffer, sizeof(buffer), (i == 0 ? "lan_ipaddr" : "lan%d_ipaddr"), i);
				ip = nvram_safe_get(buffer);
				break;
			}
		}
	}

	/* writing data to file */
	if (!(fp = fopen(tor_config, "w"))) {
		logerr(__FUNCTION__, __LINE__, tor_config);
		return;
	}
	/* localhost ports, NoPreferIPv6Automap doesn't matter when applied only to DNSPort, but works fine with SocksPort */
	fprintf(fp, "SocksPort %d NoPreferIPv6Automap\n"
	            "AutomapHostsOnResolve 1\n" /* .exit/.onion domains support for LAN clients */
	            "VirtualAddrNetworkIPv4 172.16.0.0/12\n"
	            "VirtualAddrNetworkIPv6 [FC00::]/7\n"
	            "AvoidDiskWrites 1\n"
	            "RunAsDaemon 1\n"
	            "Log notice syslog\n"
	            "DataDirectory %s\n"
	            "TransPort %s:%s\n"
	            "DNSPort %s:%s\n"
	            "User nobody\n"
	            "%s\n",
	            nvram_get_int("tor_socksport"),
	            nvram_safe_get("tor_datadir"),
	            ip, nvram_safe_get("tor_transport"),
	            ip, nvram_safe_get("tor_dnsport"),
	            nvram_safe_get("tor_custom"));

	fclose(fp);

	chmod(tor_config, 0644);
	chmod("/dev/null", 0666);

	mkdir(nvram_safe_get("tor_datadir"), 0700);

	xstart("chown", "nobody:nobody", nvram_safe_get("tor_datadir"));

	xstart("tor", "-f", tor_config);
}

void stop_tor(void) {
	if (serialize_restart("tor", 0))
		return;

	if (pidof("tor") > 0)
		killall_tk_period_wait("tor", 50);
}
