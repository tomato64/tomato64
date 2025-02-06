/*
 *
 * Tomato Firmware
 * Copyright (C) 2006-2009 Jonathan Zarate
 * Fixes/updates (C) 2018 - 2024 pedro
 *
 */


#include "rc.h"

#include <sys/types.h>
#include <sys/stat.h>
#include <time.h>
#include <arpa/inet.h>

#define CRU_TMP_FN	"/tmp/cru-ddns"

/* needed by logmsg() */
#define LOGMSG_DISABLE	DISABLE_SYSLOG_OS
#define LOGMSG_NVDEBUG	"ddns_debug"


static void update(int num, int *dirty, int force)
{
	char config[2048];
	char *p;
	char *serv, *user, *pass, *host, *wild, *mx, *bmx, *cust;
	time_t t;
	struct tm *tm;
	int n;
	char ddnsx[16];
	char ddnsx_path[32];
	char s[128];
	char v[128];
	char cache_fn[32];
	char conf_fn[32];
	char cache_nv[32];
	char msg_fn[32];
	char ip[32];
	int exitcode, errors;
	FILE *f;
	char prefix[] = "wanXX";

	logmsg(LOG_DEBUG, "*** %s: IN num=[%d] dirty=[%d] force=[%d]", __FUNCTION__, num, *dirty, force);

	memset(ddnsx, 0, sizeof(ddnsx));
	snprintf(ddnsx, sizeof(ddnsx), "ddnsx%d", num);

	memset(s, 0, sizeof(s));
	snprintf(s, sizeof(s), "%s_ip", ddnsx);

	if (nvram_match(s, "wan") || nvram_match(s, "wan2")
#ifdef TCONFIG_MULTIWAN
	    || nvram_match(s, "wan3") || nvram_match(s, "wan4")
#endif
	)
		strlcpy(prefix, nvram_safe_get(s), sizeof(prefix));
	else
		strlcpy(prefix, "wan", sizeof(prefix)); /* default */

	memset(s, 0, sizeof(s));
	snprintf(s, sizeof(s), "ddns%d", num);
	eval("cru", "d", s);
	logmsg(LOG_DEBUG, "*** %s: cru d %s", __FUNCTION__, s);

	memset(s, 0, sizeof(s));
	snprintf(s, sizeof(s), "ddnsf%d", num);
	eval("cru", "d", s);
	logmsg(LOG_DEBUG, "*** %s: cru d %s", __FUNCTION__, s);

	memset(ddnsx_path, 0, sizeof(ddnsx_path));
	snprintf(ddnsx_path, sizeof(ddnsx_path), "/var/lib/mdu/%s", ddnsx);
	strlcpy(config, nvram_safe_get(ddnsx), sizeof(config));

	mkdir("/var/lib/mdu", 0700);
	memset(msg_fn, 0, sizeof(msg_fn));
	snprintf(msg_fn, sizeof(msg_fn), "%s.msg", ddnsx_path);

	if ((vstrsep(config, "<", &serv, &user, &host, &wild, &mx, &bmx, &cust) < 7) || (*serv == 0)) {
		logmsg(LOG_DEBUG, "*** %s: msg=''", __FUNCTION__);
		f_write(msg_fn, NULL, 0, 0, 0);
		return;
	}

	if ((pass = strchr(user, ':')) != NULL)
		*pass++ = 0;
	else
		pass = "";

	if (!wait_action_idle(10)) {
		logmsg(LOG_DEBUG, "*** %s: !wait_action_idle", __FUNCTION__);
		return;
	}

	memset(cache_nv, 0, sizeof(cache_nv));
	snprintf(cache_nv, sizeof(cache_nv), "%s_cache", ddnsx);
	if (force) {
		logmsg(LOG_DEBUG, "*** %s: force=1", __FUNCTION__);
		nvram_set(cache_nv, "");
	}

	memset(s, 0, sizeof(s));
	snprintf(s, sizeof(s), "ddns%d", num);
	simple_lock(s);

	memset(s, 0, sizeof(s));
	snprintf(s, sizeof(s), "%s_ip", ddnsx);
	strlcpy(ip, nvram_safe_get(s), sizeof(ip));

	if (!check_wanup(prefix)) {
		if ((get_wanx_proto(prefix) != WP_DISABLED) || (ip[0] == 0)) {
			logmsg(LOG_DEBUG, "*** %s: !check_wanup", __FUNCTION__);
			goto CLEANUP;
		}
	}

	if ((ip[0] != '@') && (inet_addr(ip) == (in_addr_t) - 1)) {
		strlcpy(ip, get_wanip(prefix), sizeof(ip));
		logmsg(LOG_DEBUG, "*** %s: inet_addr ip: %s", __FUNCTION__, ip);
	}

	/* copy content of nvram cache to a file cache */
	memset(cache_fn, 0, sizeof(cache_fn));
	snprintf(cache_fn, sizeof(cache_fn), "%s.cache", ddnsx_path);
	f_write_string(cache_fn, nvram_safe_get(cache_nv), 0, 0);

	/* if nvram cache is empty, the 'Force next update' option is probably checked - reset also cache file .extip */
	memset(s, 0, sizeof(s));
	snprintf(s, sizeof(s), "%s.extip", ddnsx_path);
	if (strcmp(nvram_safe_get(cache_nv), "") == 0)
		f_write(s, NULL, 0, 0, 0);

	if (!f_exists(msg_fn)) {
		logmsg(LOG_DEBUG, "*** %s: !f_exist(%s) - creating ...", __FUNCTION__, msg_fn);
		f_write(msg_fn, NULL, 0, 0, 0);
	}

	memset(conf_fn, 0, sizeof(conf_fn));
	snprintf(conf_fn, sizeof(conf_fn), "%s.conf", ddnsx_path);
	if ((f = fopen(conf_fn, "w")) == NULL)
		goto CLEANUP;

	/* note: options not needed for the service are ignored by mdu */
	fprintf(f, "user %s\n"
	           "pass %s\n"
	           "host %s\n"
	           "addr %s\n"
	           "mx %s\n"
	           "backmx %s\n"
	           "wildcard %s\n"
	           "url %s\n"
	           "ahash %s\n"
	           "msg %s\n"
	           "cookie %s\n"
	           "addrcache %s\n"
	           "",
	           user,
	           pass,
	           host,
	           ip,
	           mx,
	           bmx,
	           wild,
	           cust,
	           cust,
	           msg_fn,
	           cache_fn,
	           s);

	if (nvram_get_int("debug_ddns"))
		fprintf(f, "dump /tmp/mdu%d-%s.txt\n", num, serv);

	fclose(f);

	logmsg(LOG_DEBUG, "*** mdu <<<<<<<");
	exitcode = eval("mdu", "--service", serv, "--conf", conf_fn);
	logmsg(LOG_DEBUG, "*** mdu >>>>>>> %s: service: %s config: %s; exitcode: %d", __FUNCTION__, serv, conf_fn, exitcode);

	memset(s, 0, sizeof(s));
	snprintf(s, sizeof(s), "%s_errors", ddnsx);
	if ((exitcode == 1) || (exitcode == 2)) {
		if (force)
			errors = 0;
		else {
			errors = nvram_get_int(s) + 1;
			if (errors < 1)
				errors = 1;

			if (errors >= 10) { /* remove from cru, go to standby */
				nvram_unset(s);
				goto CLEANUP;
			}
		}
		memset(v, 0, sizeof(v));
		snprintf(v, sizeof(v), "%d", errors);
		nvram_set(s, v);
		goto SCHED;
	}
	else {
		nvram_unset(s);
		errors = 0;
	}

	f_read_string(cache_fn, s, sizeof(s));
	if ((p = strchr(s, '\n')) != NULL)
		*p = 0;

	t = strtoul(s, &p, 10);
	if (*p != ',')
		goto CLEANUP;

	if (!nvram_match(cache_nv, s)) { /* nvram cache is different than this in file */
		nvram_set(cache_nv, s);

		memset(v, 0, sizeof(v));
		snprintf(v, sizeof(v), "%s_save", ddnsx);
		if (nvram_get_int(s) && (strstr(serv, "dyndns") == 0))
			*dirty = 1;
	}

	n = 28;
	memset(v, 0, sizeof(v));
	snprintf(v, sizeof(v), "%s_refresh", ddnsx);
	if ((p = nvram_safe_get(v)) && (*p))
		n = atoi(p);

	if (n) {
		logmsg(LOG_DEBUG, "*** %s: add scheduler [refresh DDNS server] ...", __FUNCTION__);

		if ((n < 0) || (n > 90))
			n = 28;

		t += (n * 86400); /* refresh every n days */
		
		/* fix: if time is in the past, make it current */
		time_t now = time(0) + (60 * 5);
		if (t < now)
			t = now;

		tm = localtime(&t);

		memset(s, 0, sizeof(s));
		snprintf(s, sizeof(s), "ddnsf%d", num);
		memset(v, 0, sizeof(v));
		snprintf(v, sizeof(v), "%d %d %d %d * ddns-update %d force", tm->tm_min, tm->tm_hour, tm->tm_mday, tm->tm_mon + 1, num);

		logmsg(LOG_DEBUG, "*** %s: cru a %s %s", __FUNCTION__, s, v);

		eval("cru", "a", s, v);
	}

	if (ip[0] == '@') {
SCHED:
		logmsg(LOG_DEBUG, "*** %s: add scheduler [external checker] ...", __FUNCTION__);

		memset(s, 0, sizeof(s));
		snprintf(s, sizeof(s), "%s_cktime", ddnsx);
		n = ((nvram_get_int(s) ? nvram_get_int(s) : 10) + (errors * 2));
		if ((exitcode == 1) || (exitcode == 2)) {
			if (exitcode == 2) /* special case [update_dua()]: server down */
				n = 30;

			memset(s, 0, sizeof(s));
			snprintf(s, sizeof(s), "#RETRY %d %d", n, errors); /* should be localized in basic-ddns.asp */
			f_write_string(msg_fn, s, FW_APPEND, 0);
			logmsg(LOG_DEBUG, "*** %s: msg='retry n=%d errors=%d'", __FUNCTION__, n, errors);
		}

		t = time(0) + (n * 60); /* minutes */
		tm = localtime(&t);

		memset(s, 0, sizeof(s));
		snprintf(s, sizeof(s), "ddns%d", num);
		memset(v, 0, sizeof(v));
		snprintf(v, sizeof(v), "%d * * * * ddns-update %d", tm->tm_min, num);

		logmsg(LOG_DEBUG, "*** %s: cru a %s %s", __FUNCTION__, s, v);

		eval("cru", "a", s, v);
	}

CLEANUP:
	memset(s, 0, sizeof(s));
	snprintf(s, sizeof(s), "ddns%d", num);
	simple_unlock(s);

	logmsg(LOG_DEBUG, "*** %s: OUT", __FUNCTION__);
}

int ddns_update_main(int argc, char **argv)
{
	int num;
	int dirty;

	logmsg(LOG_DEBUG, "*** %s: args: %s %s", __FUNCTION__, (argc >= 2) ? argv[1] : "", (argc >= 3) ? argv[2] : "");

	dirty = 0;
	umask(077);

	if (argc == 1) {
		update(0, &dirty, 0);
		update(1, &dirty, 0);
#if !defined(TCONFIG_NVRAM_32K) && !defined(TCONFIG_OPTIMIZE_SIZE)
		update(2, &dirty, 0);
		update(3, &dirty, 0);
#endif
	}
	else if ((argc == 2) || (argc == 3)) {
		num = atoi(argv[1]);
		if ((num == 0) || (num == 1)
#if !defined(TCONFIG_NVRAM_32K) && !defined(TCONFIG_OPTIMIZE_SIZE)
		    || (num == 2) || (num == 3)
#endif
		   )
			update(num, &dirty, ((argc == 3) && (strcmp(argv[2], "force") == 0)));
	}
	if (dirty)
		nvram_commit_x();

	return 0;
}

void start_ddns(void)
{
	char tmp[8];

	logmsg(LOG_DEBUG, "*** %s: IN", __FUNCTION__);

	system("cru l | grep ddns-update | wc -l >" CRU_TMP_FN);
	memset(tmp, 0, sizeof(tmp));
	f_read(CRU_TMP_FN, tmp, sizeof(tmp));

	if ((pidof("ddns-update") > 0) || (pidof("mdu") > 0) || (atoi(tmp) > 0))
		stop_ddns();

	/* cleanup */
	simple_unlock("ddns0");
	simple_unlock("ddns1");
#if !defined(TCONFIG_NVRAM_32K) && !defined(TCONFIG_OPTIMIZE_SIZE)
	simple_unlock("ddns2");
	simple_unlock("ddns3");
#endif
	nvram_unset("ddnsx0_errors");
	nvram_unset("ddnsx1_errors");
#if !defined(TCONFIG_NVRAM_32K) && !defined(TCONFIG_OPTIMIZE_SIZE)
	nvram_unset("ddnsx2_errors");
	nvram_unset("ddnsx3_errors");
#endif

	xstart("ddns-update");

	logmsg(LOG_INFO, "ddns is started");
}

void stop_ddns(void)
{
	int m = 15;

	logmsg(LOG_DEBUG, "*** %s: IN", __FUNCTION__);

	eval("cru", "d", "ddns0");
	eval("cru", "d", "ddns1");
#if !defined(TCONFIG_NVRAM_32K) && !defined(TCONFIG_OPTIMIZE_SIZE)
	eval("cru", "d", "ddns2");
	eval("cru", "d", "ddns3");
#endif
	eval("cru", "d", "ddnsf0");
	eval("cru", "d", "ddnsf1");
#if !defined(TCONFIG_NVRAM_32K) && !defined(TCONFIG_OPTIMIZE_SIZE)
	eval("cru", "d", "ddnsf2");
	eval("cru", "d", "ddnsf3");
#endif

	killall("ddns-update", SIGKILL);

	/* prevent mdu from leaving unwanted routing (only in MultiWAN mode) */
	if (nvram_get_int("mwan_num") > 1 && pidof("mdu") > 0 && !nvram_get_int("g_upgrade") && !nvram_get_int("g_reboot")) {
		f_write_string(MDU_STOP_FN, "1", 0, 0); /* create stop file */
		while (pidof("mdu") > 0 && (m-- > 0)) {
			logmsg(LOG_DEBUG, "*** %s: waiting for mdu to end, %d secs left ...", __FUNCTION__, m);
			sleep(1);
		}
	}
	killall("mdu", SIGKILL);

	logmsg(LOG_INFO, "ddns is stopped");
}
