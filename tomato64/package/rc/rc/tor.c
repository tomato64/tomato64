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

	/* only if enabled or forced */
	if (!nvram_get_int("tor_enable") && force == 0)
		return;

	if (serialize_restart("tor", 1))
		return;

	/* dnsmasq uses this IP for nameserver to resolv .onion/.exit domains */
	ip = nvram_safe_get("lan_ipaddr");
	if (!nvram_get_int("tor_solve_only")) {
		if (nvram_match("tor_iface", "br0"))      { ip = nvram_safe_get("lan_ipaddr");  }
		else if (nvram_match("tor_iface", "br1")) { ip = nvram_safe_get("lan1_ipaddr"); }
		else if (nvram_match("tor_iface", "br2")) { ip = nvram_safe_get("lan2_ipaddr"); }
		else if (nvram_match("tor_iface", "br3")) { ip = nvram_safe_get("lan3_ipaddr"); }
		else                                      { ip = nvram_safe_get("lan_ipaddr");  }
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
