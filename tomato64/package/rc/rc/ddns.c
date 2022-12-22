/*

	Tomato Firmware
	Copyright (C) 2006-2009 Jonathan Zarate

*/


#include "rc.h"

#include <sys/types.h>
#include <sys/stat.h>
#include <time.h>
#include <arpa/inet.h>

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

	if (nvram_match("ddnsx_ip", "wan") || nvram_match("ddnsx_ip", "wan2")
#ifdef TCONFIG_MULTIWAN
	    || nvram_match("ddnsx_ip", "wan3") || nvram_match("ddnsx_ip", "wan4")
#endif
	)
		strlcpy(prefix, nvram_safe_get("ddnsx_ip"), sizeof(prefix));
	else
		strlcpy(prefix, "wan", sizeof(prefix));

	logmsg(LOG_DEBUG, "*** %s", __FUNCTION__);

	memset(s, 0, sizeof(s));
	snprintf(s, sizeof(s), "ddns%d", num);
	eval("cru", "d", s);
	logmsg(LOG_DEBUG, "*** %s: cru d %s", __FUNCTION__, s);

	memset(s, 0, sizeof(s));
	snprintf(s, sizeof(s), "ddnsf%d", num);
	eval("cru", "d", s);
	logmsg(LOG_DEBUG, "*** %s: cru d %s", __FUNCTION__, s);

	memset(ddnsx, 0, sizeof(ddnsx));
	snprintf(ddnsx, sizeof(ddnsx), "ddnsx%d", num);
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

	simple_lock("ddns");

	strlcpy(ip, nvram_safe_get("ddnsx_ip"), sizeof(ip));

	if (!check_wanup(prefix)) {
		if ((get_wan_proto() != WP_DISABLED) || (ip[0] == 0)) {
			logmsg(LOG_DEBUG, "*** %s: !check_wanup", __FUNCTION__);
			goto CLEANUP;
		}
	}

	if (ip[0] == '@') {
		if ((strcmp(serv, "zoneedit") == 0) || (strcmp(serv, "noip") == 0) || (strcmp(serv, "dnsomatic") == 0) || (strcmp(serv, "pairdomains") == 0) || (strcmp(serv, "changeip") == 0))
			strcpy(ip + 1, serv);
		else
			strcpy(ip + 1, "dyndns");
	}
	else if (inet_addr(ip) == (in_addr_t) -1)
		strcpy(ip, get_wanip(prefix));

	memset(cache_fn, 0, sizeof(cache_nv));
	snprintf(cache_fn, sizeof(cache_nv), "%s.cache", ddnsx_path);
	f_write_string(cache_fn, nvram_safe_get(cache_nv), 0, 0);

	if (!f_exists(msg_fn)) {
		logmsg(LOG_DEBUG, "*** %s: !f_exist(%s)", __FUNCTION__, msg_fn);
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
	           "addrcache extip\n"
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
	           cache_fn);

	if (nvram_get_int("debug_ddns"))
		fprintf(f, "dump /tmp/mdu-%s.txt\n", serv);

	fclose(f);

	exitcode = eval("mdu", "--service", serv, "--conf", conf_fn);
	logmsg(LOG_DEBUG, "*** %s: mdu exitcode=%d", __FUNCTION__, exitcode);

	memset(s, 0, sizeof(s));
	snprintf(s, sizeof(s), "%s_errors", ddnsx);
	if ((exitcode == 1) || (exitcode == 2)) {
		if (nvram_match("ddnsx_retry", "0"))
			goto CLEANUP;

		if (force)
			errors = 0;
		else {
			errors = nvram_get_int(s) + 1;
			if (errors < 1)
				errors = 1;

			if (errors >= 3) {
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

	if (!nvram_match(cache_nv, s)) {
		nvram_set(cache_nv, s);
		if (nvram_get_int("ddnsx_save") && (strstr(serv, "dyndns") == 0))
				*dirty = 1;
	}

	n = 28;
	if ((p = nvram_safe_get("ddnsx_refresh")) && (*p))
		n = atoi(p);

	if (n) {
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
		logmsg(LOG_DEBUG, "*** %s: SCHED", __FUNCTION__);

		/* need at least 10m spacing for checkip
		 * +1m to not trip over mdu's ip caching
		 * +5m for every error
		 */
		n = (11 + (errors * 5));
		if ((exitcode == 1) || (exitcode == 2)) {
			if (exitcode == 2)
				n = 30;

			memset(s, 0, sizeof(s));
			snprintf(s, sizeof(s), "\n#RETRY %d %d\n", n, errors); /* should be localized in basic-ddns.asp */
			f_write_string(msg_fn, s, FW_APPEND, 0);
			logmsg(LOG_DEBUG, "*** %s: msg='retry n=%d errors=%d'", __FUNCTION__, n, errors);
		}

		t = time(0) + (n * 60);
		tm = localtime(&t);
		logmsg(LOG_DEBUG, "*** %s: sch: %d:%d", __FUNCTION__, tm->tm_hour, tm->tm_min);

		memset(s, 0, sizeof(s));
		snprintf(s, sizeof(s), "ddns%d", num);
		memset(v, 0, sizeof(v));
		snprintf(v, sizeof(v), "%d * * * * ddns-update %d", tm->tm_min, num);
		logmsg(LOG_DEBUG, "*** %s: cru a %s %s", __FUNCTION__, s, v);

		eval("cru", "a", s, v);
	}

CLEANUP:
	logmsg(LOG_DEBUG, "*** %s: CLEANUP", __FUNCTION__);
	simple_unlock("ddns");
}

int ddns_update_main(int argc, char **argv)
{
	int num;
	int dirty;

	logmsg(LOG_DEBUG, "*** %s: %s %s", __FUNCTION__, (argc >= 2) ? argv[1] : "", (argc >= 3) ? argv[2] : "");

	dirty = 0;
	umask(077);

	if (argc == 1) {
		update(0, &dirty, 0);
		update(1, &dirty, 0);
	}
	else if ((argc == 2) || (argc == 3)) {
		num = atoi(argv[1]);
		if ((num == 0) || (num == 1))
			update(num, &dirty, (argc == 3) && (strcmp(argv[2], "force") == 0));
	}
	if (dirty)
		nvram_commit_x();

	return 0;
}

void start_ddns(void)
{
	logmsg(LOG_DEBUG, "*** %s", __FUNCTION__);

	stop_ddns();

	/* cleanup */
	simple_unlock("ddns");
	nvram_unset("ddnsx0_errors");
	nvram_unset("ddnsx1_errors");

	xstart("ddns-update");
}

void stop_ddns(void)
{
	logmsg(LOG_DEBUG, "*** %s", __FUNCTION__);

	eval("cru", "d", "ddns0");
	eval("cru", "d", "ddns1");
	eval("cru", "d", "ddnsf0");
	eval("cru", "d", "ddnsf1");

	killall("ddns-update", SIGKILL);
	killall("mdu", SIGKILL);
}
