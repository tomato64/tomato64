/*
 * transmission.c
 *
 * Copyright (C) 2011 shibby
 *
 * Fixes/updates (C) 2018 - 2026 pedro
 * https://freshtomato.org/
 *
 */


#include "rc.h"

#include <sys/types.h>

#define tr_dir			"/etc/transmission"
#define tr_settings		tr_dir"/settings.json"
#define tr_fw_script		tr_dir"/tr-fw.sh"
#define tr_fw_del_script	tr_dir"/tr-clear-fw-tmp.sh"
#define tr_child_pid		tr_dir"/child.pid"

/* needed by logmsg() */
#define LOGMSG_DISABLE	DISABLE_SYSLOG_OSM
#define LOGMSG_NVDEBUG	"transmission_debug"


static void setup_tr_watchdog(void)
{
	FILE *fp;
	char buffer[64], buffer2[64];
	int nvi;

	if ((nvi = nvram_get_int("bt_check_time")) > 0) {
		snprintf(buffer, sizeof(buffer), tr_dir"/watchdog.sh");

		if ((fp = fopen(buffer, "w"))) {
			fprintf(fp, "#!/bin/sh\n"
			            "[ -z \"$(pidof transmission-daemon)\" -a \"$(nvram get g_upgrade)\" != \"1\" -a \"$(nvram get g_reboot)\" != \"1\" ] && {\n"
			            " logger -t transmission-watchdog transmission-daemon stopped? Starting...\n"
			            " service bittorrent restart\n"
			            "}\n");
			fclose(fp);
			chmod(buffer, (S_IRUSR | S_IWUSR | S_IXUSR));

			snprintf(buffer2, sizeof(buffer2), "*/%d * * * * %s", nvi, buffer);
			eval("cru", "a", "CheckTransmission", buffer2);
		}
	}
}

static void build_tr_firewall(void)
{
	FILE *p;

	/* create firewall script */
	if (!(p = fopen(tr_fw_script, "w"))) {
		logerr(__FUNCTION__, __LINE__, tr_fw_script);
		return;
	}

	chains_log_detection();

	/* open BT port */
	fprintf(p, "#!/bin/sh\n"
	           "iptables -A INPUT -p tcp --dport %s -j %s\n"
	           "iptables -A INPUT -p udp --dport %s -j %s\n",
	            nvram_safe_get("bt_port"), chain_in_accept,
	            nvram_safe_get("bt_port"), chain_in_accept);
#ifdef TCONFIG_IPV6
	if (ipv6_enabled())
		fprintf(p, "ip6tables -A INPUT -p tcp --dport %s -j %s\n"
		           "ip6tables -A INPUT -p udp --dport %s -j %s\n",
		           nvram_safe_get("bt_port"), chain_in_accept,
		           nvram_safe_get("bt_port"), chain_in_accept);
#endif

	/* GUI WAN access */
	if (nvram_get_int("bt_rpc_wan"))
		fprintf(p, "iptables -A INPUT -p tcp --dport %s -j %s\n"
		           "iptables -t nat -A WANPREROUTING -p tcp --dport %s -j DNAT --to-destination %s\n", /* nat table */
		            nvram_safe_get("bt_port_gui"), chain_in_accept,
		            nvram_safe_get("bt_port_gui"), nvram_safe_get("lan_ipaddr"));

	fclose(p);
	chmod(tr_fw_script, 0744);
}

void start_bittorrent(int force)
{
	FILE *fp;
	char *pb, *pc, *pd, *pe, *pf, *ph, *pi, *pj, *pk, *pl, *pm, *pn, *po, *pp, *pr, *pt, *pu;
	char *whitelistEnabled;
	char buf[256], buf2[64], settings_dir[256], log_path[256];
	int n;
	pid_t pidof_child = 0;

	/* only if enabled or forced */
	if (!nvram_get_int("bt_enable") && force == 0)
		return;

	if (serialize_restart("transmission-da", 1))
		return;

	memset(buf2, 0, sizeof(buf2));
	if (f_read_string(tr_child_pid, buf2, sizeof(buf2)) > 0 && atoi(buf2) > 0 && ppid(atoi(buf2)) > 0) { /* fork is still up */
		logmsg(LOG_WARNING, "%s: another process (PID: %s) still up, aborting ...", __FUNCTION__, buf2);
		return;
	}

	/* collecting data */
	if (nvram_get_int("bt_rpc_enable"))              { pb = "true"; } else { pb = "false"; }
	if (nvram_get_int("bt_dl_enable"))               { pc = "true"; } else { pc = "false"; }
	if (nvram_get_int("bt_ul_enable"))               { pd = "true"; } else { pd = "false"; }
	if (nvram_get_int("bt_incomplete"))              { pe = "true"; } else { pe = "false"; }
	if (nvram_get_int("bt_autoadd"))                 { pf = "true"; } else { pf = "false"; }
	if (nvram_get_int("bt_ratio_enable"))            { ph = "true"; } else { ph = "false"; }
	if (nvram_get_int("bt_dht"))                     { pi = "true"; } else { pi = "false"; }
	if (nvram_get_int("bt_pex"))                     { pj = "true"; } else { pj = "false"; }
	if (nvram_get_int("bt_blocklist"))               { pm = "true"; } else { pm = "false"; }
	if (nvram_get_int("bt_lpd"))                     { po = "true"; } else { po = "false"; }
	if (nvram_get_int("bt_utp"))                     { pp = "true"; } else { pp = "false"; }
	if (nvram_get_int("bt_ratio_idle_enable"))       { pr = "true"; } else { pr = "false"; }
	if (nvram_get_int("bt_dl_queue_enable"))         { pt = "true"; } else { pt = "false"; }
	if (nvram_get_int("bt_ul_queue_enable"))         { pu = "true"; } else { pu = "false"; }

	if      (nvram_match("bt_settings", "down_dir")) { pk = nvram_safe_get("bt_dir"); }
	else if (nvram_match("bt_settings", "custom"))   { pk = nvram_safe_get("bt_settings_custom"); }
	else                                             { pk = nvram_safe_get("bt_settings"); }

	if      (nvram_match("bt_binary", "internal"))   { pn = "/usr/bin"; }
	else if (nvram_match("bt_binary", "optware") )   { pn = "/opt/bin"; }
	else                                             { pn = nvram_safe_get("bt_binary_custom"); }

	if (nvram_get_int("bt_auth")) {
		pl = "true";
		whitelistEnabled = "false";
	}
	else {
		pl = "false";
		whitelistEnabled = "true";
	}

	/* writing data to file */
	mkdir_if_none(tr_dir);
	if (!(fp = fopen(tr_settings, "w"))) {
		logerr(__FUNCTION__, __LINE__, tr_settings);
		return;
	}

	fprintf(fp, "{\n"
	            "\"peer-port\": %s,\n"
	            "\"speed-limit-down-enabled\": %s,\n"
	            "\"speed-limit-up-enabled\": %s,\n"
	            "\"speed-limit-down\": %s,\n"
	            "\"speed-limit-up\": %s,\n"
	            "\"rpc-enabled\": %s,\n"
	            "\"rpc-port\": %s,\n"
	            "\"rpc-bind-address\": \"0.0.0.0\",\n"
	            "\"rpc-whitelist\": \"*\",\n"
	            "\"rpc-whitelist-enabled\": %s,\n"
	            "\"rpc-host-whitelist\": \"*\",\n"
	            "\"rpc-host-whitelist-enabled\": %s,\n"
	            "\"rpc-username\": \"%s\",\n"
	            "\"rpc-password\": \"%s\",\n"
	            "\"download-dir\": \"%s\",\n"
	            "\"incomplete-dir-enabled\": \"%s\",\n"
	            "\"incomplete-dir\": \"%s/.incomplete\",\n"
	            "\"watch-dir\": \"%s\",\n"
	            "\"watch-dir-enabled\": %s,\n"
	            "\"peer-limit-global\": %s,\n"
	            "\"peer-limit-per-torrent\": %s,\n"
	            "\"upload-slots-per-torrent\": %s,\n"
	            "\"dht-enabled\": %s,\n"
	            "\"pex-enabled\": %s,\n"
	            "\"lpd-enabled\": %s,\n"
	            "\"utp-enabled\": %s,\n"
	            "\"ratio-limit-enabled\": %s,\n"
	            "\"ratio-limit\": %s,\n"
	            "\"idle-seeding-limit-enabled\": %s,\n"
	            "\"idle-seeding-limit\": %s,\n"
	            "\"blocklist-enabled\": %s,\n"
	            "\"blocklist-url\": \"%s\",\n"
	            "\"download-queue-enabled\": %s,\n"
	            "\"download-queue-size\": %s,\n"
	            "\"seed-queue-enabled\": %s,\n"
	            "\"seed-queue-size\": %s,\n"
	            "\"message-level\": %s,\n"
	            "%s%s"
	            "\"rpc-authentication-required\": %s\n"
	            "}\n",
	            nvram_safe_get( "bt_port"),
	            pc,
	            pd,
	            nvram_safe_get("bt_dl"),
	            nvram_safe_get("bt_ul"),
	            pb,
	            nvram_safe_get("bt_port_gui"),
	            whitelistEnabled,
	            whitelistEnabled,
	            nvram_safe_get("bt_login"),
	            nvram_safe_get("bt_password"),
	            nvram_safe_get("bt_dir"),
	            pe,
	            nvram_safe_get("bt_dir"),
	            nvram_safe_get("bt_dir"),
	            pf,
	            nvram_safe_get("bt_peer_limit_global"),
	            nvram_safe_get("bt_peer_limit_per_torrent"),
	            nvram_safe_get("bt_ul_slot_per_torrent"),
	            pi,
	            pj,
	            po,
	            pp,
	            ph,
	            nvram_safe_get("bt_ratio"),
	            pr,
	            nvram_safe_get("bt_ratio_idle"),
	            pm,
	            nvram_safe_get("bt_blocklist_url"),
	            pt,
	            nvram_safe_get("bt_dl_queue_size"),
	            pu,
	            nvram_safe_get("bt_ul_queue_size"),
	            nvram_safe_get("bt_message"),
	            nvram_safe_get("bt_custom"),
	            (strcmp(nvram_safe_get("bt_custom"), "") ? ",\n" : ""),
	            pl);

	fclose(fp);

	chmod(tr_settings, 0644);

	/* create firewall script */
	build_tr_firewall();

	/* fork new process */
	if (fork() != 0) /* foreground process */
		return;

	pidof_child = getpid();

	/* write child pid to a file */
	snprintf(buf2, sizeof(buf2), "%d", pidof_child);
	f_write_string(tr_child_pid, buf2, 0, 0);

	/* tune buffers */
	f_write_procsysnet("core/rmem_max", "4194304");
	f_write_procsysnet("core/wmem_max", "2080768");
	memset(buf2, 0, sizeof(buf2));
	if (f_read_string("/proc/sys/net/ipv4/tcp_adv_win_scale", buf2, sizeof(buf2)) > 0 && atoi(buf2) > 0);
		f_write_procsysnet("ipv4/tcp_adv_win_scale", "4");

	/* wait a given time for partition to be mounted, etc */
	n = atoi(nvram_safe_get("bt_sleep"));
	if (n > 0 && n < 60) {
		logmsg(LOG_INFO, "transmission-daemon - delaying start by %d seconds ...", n);
		sleep(n);
	}

	/* only now prepare subdirs */
	mkdir_if_none(pk);
	snprintf(buf, sizeof(buf), "%s/.settings", pk);
	mkdir_if_none(buf);
	if (nvram_get_int("bt_incomplete")) {
		snprintf(buf, sizeof(buf), "%s/.incomplete", pk);
		mkdir_if_none(buf);
	}

	/* TBD: need better regex, trim triple commas, be safe for passwords etc */
	eval("sed", "-i", "s/,,\\s/, /g", tr_settings);

	snprintf(buf, sizeof(buf), "%s/.settings", pk);
	eval("cp", tr_settings, buf);

	snprintf(buf, sizeof(buf), "%s/.settings/blocklists", pk);
	eval("rm", "-rf", buf);

	if (nvram_get_int("bt_blocklist")) {
		snprintf(buf, sizeof(buf), "%s/.settings/blocklists/level1.gz", pk);
#ifdef TCONFIG_STUBBY
		eval("wget", nvram_safe_get("bt_blocklist_url"), "-O", buf);
#else
		eval("wget", "--no-check-certificate", nvram_safe_get("bt_blocklist_url"), "-O", buf);
#endif
		eval("gunzip", buf);
	}

	run_bt_firewall_script();

	snprintf(buf, sizeof(buf), "%s/transmission-daemon", pn);
	snprintf(settings_dir, sizeof(settings_dir), "%s/.settings", pk);

	setenv("EVENT_NOEPOLL", "1", 1);
#ifdef TCONFIG_STUBBY
	setenv("CURL_CA_BUNDLE", "/etc/ssl/cert.pem", 1);
#endif

	if (nvram_get_int("bt_log")) {
		snprintf(log_path, sizeof(log_path), "%s/transmission.log", nvram_safe_get("bt_log_path"));
		eval(buf, "-g", settings_dir, "-e", log_path);
	}
	else
		eval(buf, "-g", settings_dir);

	sleep(1);

	if (pidof("transmission-da") > 0) {
		logmsg(LOG_INFO, "transmission-daemon started");
		setup_tr_watchdog();
		eval("rm", "-f", tr_child_pid);
	}
	else {
		logmsg(LOG_ERR, "starting transmission-daemon failed - check configuration ...");
		eval("rm", "-f", tr_child_pid);
		stop_bittorrent();
	}

	/* terminate the child */
	_exit(0);
}

void stop_bittorrent(void)
{
	pid_t pid;
	char buf[16];
	int m = atoi(nvram_safe_get("bt_sleep")) + 10;

	if (serialize_restart("transmission-da", 0))
		return;

	pid = pidof("transmission-da");

	eval("cru", "d", "CheckTransmission");

	/* wait for child of start_bittorrent to finish (if any) */
	memset(buf, 0, sizeof(buf));
	while (f_read_string(tr_child_pid, buf, sizeof(buf)) > 0 && atoi(buf) > 0 && ppid(atoi(buf)) > 0 && (m-- > 0)) {
		logmsg(LOG_DEBUG, "*** %s: waiting for child process of start_bittorrent to end, %d secs left ...", __FUNCTION__, m);
		sleep(1);
	}

	killall_and_waitfor("transmission-da", 10, 50);

	if (pid > 0)
		logmsg(LOG_INFO, "transmission-daemon stopped");

	simple_lock("firewall");
	run_del_firewall_script(tr_fw_script, tr_fw_del_script);

	/* clean-up */
	eval("rm", "-rf", tr_dir);
	simple_unlock("firewall");

	/* restore default buffers */
	memset(buf, 0, sizeof(buf));
	if (f_read_string("/proc/sys/net/core/rmem_default", buf, sizeof(buf)) > 0 && atoi(buf) > 0);
		f_write_procsysnet("core/rmem_max", buf);

	memset(buf, 0, sizeof(buf));
	if (f_read_string("/proc/sys/net/core/wmem_default", buf, sizeof(buf)) > 0 && atoi(buf) > 0);
		f_write_procsysnet("core/wmem_max", buf);

	memset(buf, 0, sizeof(buf));
	if (f_read_string("/proc/sys/net/ipv4/tcp_adv_win_scale", buf, sizeof(buf)) > 0 && atoi(buf) > 0);
		f_write_procsysnet("ipv4/tcp_adv_win_scale", "2");
}

void run_bt_firewall_script(void)
{
	FILE *fp;

	/* first remove existing firewall rule(s) */
	simple_lock("firewall");
	run_del_firewall_script(tr_fw_script, tr_fw_del_script);

	/* then (re-)add firewall rule(s) */
	if ((fp = fopen(tr_fw_script, "r"))) {
		fclose(fp);
		logmsg(LOG_DEBUG, "*** %s: running firewall script: %s", __FUNCTION__, tr_fw_script);
		eval(tr_fw_script);
		fix_chain_in_drop();
	}
	simple_unlock("firewall");
}
