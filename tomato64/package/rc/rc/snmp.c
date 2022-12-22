/*
 * snmp.c
 *
 * Copyright (C) 2011 shibby
 *
 */


#include "rc.h"

#include <sys/stat.h>

#define SNMP_CONF	"/etc/snmpd.conf"


void start_snmp(void)
{
	FILE *fp;

	/*  only if enabled... */
	if (nvram_match("snmp_enable", "1")) {
		/* writing data to file */
		if (!(fp = fopen(SNMP_CONF, "w"))) {
			logerr(__FUNCTION__, __LINE__, SNMP_CONF);
			return;
		}
		fprintf(fp, "agentaddress udp:%d\n"
		            "syslocation %s\n"
		            "syscontact %s <%s>\n"
		            "rocommunity %s\n"
		            "extend device /bin/echo \"%s\"\n"
		            "extend version /bin/echo \"FreshTomato %s\"\n",
		            nvram_get_int("snmp_port"),
		            nvram_safe_get("snmp_location"),
		            nvram_safe_get("snmp_contact"),
		            nvram_safe_get("snmp_contact"),
		            nvram_safe_get("snmp_ro"),
		            nvram_safe_get("t_model_name"),
		            nvram_safe_get("os_version"));

		fclose(fp);

		chmod(SNMP_CONF, 0644);

		xstart("snmpd", "-c", SNMP_CONF);
	}
}

void stop_snmp(void)
{
	killall("snmpd", SIGTERM);
}
