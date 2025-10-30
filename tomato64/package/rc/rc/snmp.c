/*
 * snmp.c
 *
 * Copyright (C) 2011 shibby
 *
 * Fixes/updates (C) 2018 - 2025 pedro
 * https://freshtomato.org/
 *
 */


#include "rc.h"

#include <sys/stat.h>


const char snmp_conf[] = "/etc/snmpd.conf";

void start_snmp(void)
{
	FILE *fp;
	const char *location, *contact, *name, *descr, *ro;

	/*  only if enabled... */
	if (nvram_match("snmp_enable", "1")) {
		add_snmp_defaults(); /* backup: check nvram! */

		/* writing data to file */
		if (!(fp = fopen(snmp_conf, "w"))) {
			logerr(__FUNCTION__, __LINE__, snmp_conf);
			return;
		}

		location = nvram_safe_get("snmp_location");
		contact = nvram_safe_get("snmp_contact");
		name = nvram_safe_get("snmp_name");
		descr = nvram_safe_get("snmp_descr");
		ro = nvram_safe_get("snmp_ro");

		fprintf(fp, "agentaddress udp:%d\n"
		            "syslocation %s\n"
		            "syscontact %s <%s>\n"
		            "rocommunity %s\n"
		            "sysName %s\n"
		            "sysDescr %s\n"
		            "extend device /bin/echo \"%s\"\n"
		            "extend version /bin/echo \"Tomato64 %s\"\n",
		            nvram_get_int("snmp_port"),
		            (location && *location ? location : "router"),
		            (contact && *contact ? contact : "admin@freshtomato"), (contact && *contact ? contact : "admin@freshtomato"),
		            (name && *name ? name : "Tomato64"),
		            (descr && *descr ? descr : "router1"),
		            (ro && *ro ? ro : "rocommunity"),
		            nvram_safe_get("t_model_name"),
		            tomato_version);

		fclose(fp);

		chmod(snmp_conf, 0644);

		xstart("snmpd", "-c", (char *)snmp_conf);

		syslog(LOG_INFO, "snmpd started");
	}
}

void stop_snmp(void)
{
	if (pidof("snmpd") > 0) {
		killall("snmpd", SIGTERM);
		syslog(LOG_INFO, "snmpd stopped");
	}
}
