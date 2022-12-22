/*
 * nocat.c
 *
 * Copyright (C) 2009 zd <tomato@winddns.cn>
 * Copyright (C) 2011 Modifications for K2.6 Victek, Roadkill 
 *
 * $Id:
 */


#include "rc.h"

#include <stdlib.h>
#include <shutils.h>
#include <utils.h>
#include <syslog.h>
#include <sys/stat.h>

#define NOCAT_SCRIPTS		"/usr/libexec/nocat/"
#define NOCAT_CONF		"/tmp/etc/nocat.conf"
#define NOCAT_START_SCRIPT	"/tmp/start_splashd.sh"
#define NOCAT_LEASES		"/tmp/nocat.leases"
#define NOCAT_LOGFILE		"/tmp/nocat.log"
#define NOCAT_LOCKFILE		"/tmp/var/lock/splashd.lock"


void build_nocat_conf(void)
{
	FILE *fp;
	char *p;

	if (!(fp = fopen(NOCAT_CONF, "w"))) {
		logerr(__FUNCTION__, __LINE__, NOCAT_CONF);
		return;
	}

	/*
	 * settings that need to be set based on router configurations
	 * Autodetected on the device: lan_ifname & NC_Iface variable
	 */
	fprintf(fp, "#\n"
	            "ExternalDevice\t%s\n"
	            "RouteOnly\t%s\n",
	            nvram_safe_get("wan_iface"),
	            "1");

	if (nvram_match("NC_BridgeLAN", "br0"))
		fprintf(fp, "InternalDevice\t%s\n"
		            "GatewayAddr\t%s\n", nvram_safe_get("lan_ifname"), nvram_safe_get("lan_ipaddr"));

	if (nvram_match("NC_BridgeLAN", "br1"))
		fprintf(fp, "InternalDevice\t%s\n"
		            "GatewayAddr\t%s\n", nvram_safe_get("lan1_ifname"), nvram_safe_get("lan1_ipaddr"));

	if (nvram_match("NC_BridgeLAN", "br2"))
		fprintf(fp, "InternalDevice\t%s\n"
		            "GatewayAddr\t%s\n", nvram_safe_get("lan2_ifname"), nvram_safe_get("lan2_ipaddr"));

	if (nvram_match("NC_BridgeLAN", "br3"))
		fprintf(fp, "InternalDevice\t%s\n"
		            "GatewayAddr\t%s\n", nvram_safe_get("lan3_ifname"), nvram_safe_get("lan3_ipaddr"));


	fprintf(fp, "GatewayMAC\t%s\n", nvram_safe_get("lan_hwaddr"));

	/*
	 * these are user defined, eventually via the web page 
	 */
	if ((p = nvram_get("NC_Verbosity")) == NULL)
		p = "2";
	fprintf(fp, "Verbosity\t%s\n", p);

	if ((p = nvram_get("NC_GatewayName")) == NULL)
		p = "FreshTomato Portal";
	fprintf(fp, "GatewayName\t%s\n", p);

	if ((p = nvram_get("NC_GatewayPort")) == NULL)
		p = "5280";
	fprintf(fp, "GatewayPort\t%s\n", p);

	if ((p = nvram_get("NC_Password")) == NULL)
		p = "";
	fprintf(fp, "GatewayPassword\t%s\n", p);

	if ((p = nvram_get("NC_GatewayMode")) == NULL)
		p = "Open";
	fprintf(fp, "GatewayMode\t%s\n", p);

	if ((p = nvram_get("NC_DocumentRoot")) == NULL)
		p = "/tmp/splashd";
	fprintf(fp, "DocumentRoot\t%s\n", p);

	if (nvram_invmatch("NC_SplashURL", "")) {
		fprintf(fp, "SplashURL\t%s\n"
		            "SplashURLTimeout\t%s\n", nvram_safe_get("NC_SplashURL"), nvram_safe_get("NC_SplashURLTimeout"));
	}

	/*
	 * Open-mode and common options 
	 */
	fprintf(fp, "LeaseFile\t%s\n"
	            "FirewallPath\t%s\n"
	            "ExcludePorts\t%s\n"
	            "IncludePorts\t%s\n"
	            "AllowedWebHosts\t%s %s\n"
	            "MACWhiteList\t%s\n"	/* MACWhiteList to ignore given machines or routers on the local net (e.g. routers with an alternate Auth) */
	            "AnyDNS\t%s\n"		/* AnyDNS to pass through any client-defined servers */
	            "HomePage\t%s\n"
	            "PeerCheckTimeout\t%s\n",
	            NOCAT_LEASES,
	            NOCAT_SCRIPTS,
	            nvram_safe_get("NC_ExcludePorts"),
	            nvram_safe_get("NC_IncludePorts"),
	            nvram_safe_get("lan_ipaddr"),
	            nvram_safe_get("NC_AllowedWebHosts"),
	            nvram_safe_get("NC_MACWhiteList"),
	            "1",
	            nvram_safe_get("NC_HomePage"),
	            nvram_safe_get("NC_PeerChecktimeout"));

	if ((p = nvram_get("NC_ForcedRedirect")) == NULL)
		p = "0";
	fprintf(fp, "ForcedRedirect\t%s\n", p);

	if ((p = nvram_get("NC_IdleTimeout")) == NULL)
		p = "0";
	fprintf(fp, "IdleTimeout\t%s\n", p);

	if ((p = nvram_get("NC_MaxMissedARP")) == NULL)
		p = "5";
	fprintf(fp, "MaxMissedARP\t%s\n", p);

	if ((p = nvram_get("NC_LoginTimeout")) == NULL)
		p = "6400";
	fprintf(fp, "LoginTimeout\t%s\n", p);

	if ((p = nvram_get("NC_RenewTimeout")) == NULL)
		p = "0";
	fprintf(fp, "RenewTimeout\t%s\n", p);

	fclose(fp);

	fprintf(stderr, "Wrote: %s\n", NOCAT_CONF);
}

void start_nocat(void)
{
	FILE *fp;
	char splashfile[255];
	char logofile[255];
	char iconfile[255];
	char cmd[255];
	char *p;

	if ((!nvram_match("NC_enable", "1")) || (!nvram_match("mwan_num", "1")))
		return;

	if (serialize_restart("splashd", 1))
		return;

	build_nocat_conf();

	if ((p = nvram_get("NC_DocumentRoot")) == NULL)
		p = "/tmp/splashd";

	memset(splashfile, 0, sizeof(splashfile));
	sprintf(splashfile, "%s/splash.html", p);
	memset(logofile, 0, sizeof(logofile));
	sprintf(logofile, "%s/style.css", p);
	memset(iconfile, 0, sizeof(iconfile));
	sprintf(iconfile, "%s/favicon.ico", p);

	if (!f_exists(splashfile)) {
		nvram_get_file("NC_SplashFile", splashfile, 8192);
		if (!f_exists(splashfile)) {
			memset(cmd, 0, sizeof(cmd));
			sprintf(cmd, "cp /www/splash.html %s", splashfile);
			system(cmd);
			memset(cmd, 0, sizeof(cmd));
			sprintf(cmd, "cp /www/style.css %s", logofile);
			system(cmd);
			memset(cmd, 0, sizeof(cmd));
			sprintf(cmd, "cp /www/favicon.ico %s", iconfile);
			system(cmd);
		}
	}

	if (!(fp = fopen(NOCAT_START_SCRIPT, "w"))) {
		logerr(__FUNCTION__, __LINE__, NOCAT_START_SCRIPT);
		return;
	}
	
	/*if (!pidof("splashd") > 0 && (fp = fopen(NOCAT_LOCKFILE, "r"))) {
		unlink(NOCAT_LOCKFILE);
	}*/

	fprintf(fp, "#!/bin/sh\n"
	            "LOGGER=\"logger -t splashd\"\n"
	            "LOCK_FILE="NOCAT_LOCKFILE"\n"
	            "if [ -f $LOCK_FILE ]; then\n"
	            "	$LOGGER \"Captive Portal halted (0), other process starting\"\n"
	            "	exit\n"
	            "fi\n"
	            "echo \"FRESHTOMATO\" > $LOCK_FILE\n"
	            "sleep 20\n"
	            "$LOGGER \"Captive Portal Splash Daemon started\"\n"
	            "echo 1 > /proc/sys/net/ipv4/tcp_tw_reuse\n"
	            "/usr/sbin/splashd >> "NOCAT_LOGFILE" 2>&1 &\n"
	            "sleep 2\n"
	            "echo 0 > /proc/sys/net/ipv4/tcp_tw_reuse\n"
	            "rm $LOCK_FILE\n");

	fclose(fp);

	chmod(NOCAT_START_SCRIPT, 0700);

	xstart(NOCAT_START_SCRIPT);
}

void stop_nocat(void)
{
	pid_t pid;

	if (serialize_restart("splashd", 0))
		return;

	if ((pid = pidof("splashd")) > 0) {
		killall_tk_period_wait("splashd", 50);
		syslog(LOG_INFO, "Captive Portal Splash daemon stopped");
	}

	if (f_exists(NOCAT_SCRIPTS"/uninitialize.fw"))
		eval(NOCAT_SCRIPTS"/uninitialize.fw");

	system("rm -f "NOCAT_LEASES);
	system("rm -f "NOCAT_START_SCRIPT);
	system("rm -f "NOCAT_LOGFILE);

	if (pid > 0)
		start_wan();
}
