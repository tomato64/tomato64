/*
 *
 * FreshTomato Firmware
 * Copyright (C) 2018 Michal Obrembski
 *
 * Fixes/updates (C) 2019 - 2024 pedro
 *
 */


#include "tomato.h"
#include <sys/stat.h>

#define iperf_log	"/tmp/iperf_log"
#define iperf_interval	"/tmp/iperf_interval"
#define iperf_pidfile	"/var/run/iperf.pid"

/* needed by logmsg() */
#define LOGMSG_DISABLE	DISABLE_SYSLOG_OSM
#define LOGMSG_NVDEBUG	"iperf_debug"


/*
 * Always clear log files before run!
 * IF IPERF is running and iperf.pid is present and INTERVAL is not present -> SERVER awaiting OK
 * IF IPERF is running and iperf.pid is present and INTERVAL is present -> SERVER receiving OK
 * IF IPERF is not running but unclar(INTERVAL) and logfile present -> Server finished
 * IF IPERF is running and no iperf.pid file is present and INTERVAL is present -> Client receiving
 * IF IPERF is running and no iperf.pid file is present and log is present -> Client receiving
 */


void wo_ttcpstatus(char *url)
{
	pid_t iperf_pid;
	struct stat st;

	if (access(iperf_log, F_OK ) != -1) {
		/* OK, we got log but it may be empty (iperf just started) */
		stat(iperf_log, &st);
		logmsg(LOG_DEBUG, "*** %s: %d: Size of iperflog: %jd", __FUNCTION__, __LINE__, (intmax_t)st.st_size);

		if (st.st_size > 0 ) {
			logmsg(LOG_DEBUG, "*** %s: %d: Sending content of iperf log", __FUNCTION__, __LINE__);
			do_file(iperf_log);
			return;
		}
	}

	iperf_pid = pidof("iperf");
	logmsg(LOG_DEBUG, "*** %s: %d: PID of iperf: %d", __FUNCTION__, __LINE__, iperf_pid);

	if (iperf_pid < 0) {
		web_puts("{ \"mode\": \"Stopped\" }");
		return;
	}
	if (access(iperf_pidfile, F_OK ) != -1) {
		/* We know we're in server mode */
		logmsg(LOG_DEBUG, "*** %s: %d: iperf.pid found, server mode", __FUNCTION__, __LINE__);

		if (access(iperf_interval, F_OK ) != -1) {
			logmsg(LOG_DEBUG, "*** %s: %d: Sending contents of interval", __FUNCTION__, __LINE__);

			stat(iperf_interval, &st);
			if (st.st_size > 0)
				do_file(iperf_interval);
			else
				web_puts("{ \"mode\": \"Server waiting\"}");
		}
		else
			web_puts("{ \"mode\": \"Server waiting\"}");
	}
	else {
		/* We know we're in client mode */
		logmsg(LOG_DEBUG, "*** %s: %d: Client mode, sending interval", __FUNCTION__, __LINE__);

		stat(iperf_interval, &st);
		if (st.st_size > 0)
			do_file(iperf_interval);
		else
			web_puts("{ \"mode\": \"Client preparing\"}");
	}
}

static void run_program(const char *program)
{
	pclose(popen(program, "r"));
}

void wo_ttcprun(char *url)
{
	char tmp[128];
	char cmdBuffer[256];
	char *v;
	char *host;
	int port = 5201;
	int udpMode = 0;
	int byteLimitMode = 0;		/* Time limit by default */
	unsigned long long limit = 10;	/* 10 Seconds, by default */

	unlink(iperf_interval);
	unlink(iperf_log);

	port = atoi(webcgi_safeget("_port", "5201"));
	udpMode = atoi(webcgi_safeget("_udpProto", "0"));
	byteLimitMode = atoi(webcgi_safeget("_limitMode", "0"));
	limit = strtoull(webcgi_safeget("_limit", "10"), NULL, 0);
	logmsg(LOG_DEBUG, "*** %s: %d: port: %d UDP: %s LimitMode: %s Limit %llu Mode", __FUNCTION__, __LINE__, port, udpMode == 0 ? "NO" : "YES", byteLimitMode == 0 ? "TIME" : "BYTE", limit);

	if ((v = webcgi_get("_mode")) != NULL && (*v)) {
		snprintf(tmp, sizeof(tmp), "%d", port);
		if (!strcmp(v, "server")) { /* server */
			logmsg(LOG_INFO, "iperf started in server mode");
			snprintf(cmdBuffer, sizeof(cmdBuffer), "iperf -J --logfile "iperf_log" --intervalfile "iperf_interval" -I "iperf_pidfile" -s -1 -D -p %d", port);
		}
		else { /* client */
			if ((host = webcgi_get("_host")) != NULL && (*host)) {
				if ((strstr(host, "/") > 0) || (strstr(host, ";") > 0) || (strstr(host, "`") > 0))
					return;

				logmsg(LOG_INFO, "iperf started in client mode, address %s", host);
				snprintf(cmdBuffer, sizeof(cmdBuffer), "iperf -J --logfile "iperf_log" --intervalfile "iperf_interval" -p %d %s %s %llu -c %s &", port, udpMode == 1 ? "-u" : "", byteLimitMode == 1 ? "-n" : "-t", limit, host);
			}
		}
		logmsg(LOG_DEBUG, "*** %s: %d: Running command: %s", __FUNCTION__, __LINE__, cmdBuffer);
		run_program(cmdBuffer);
	}
}

void wo_ttcpkill(char *url)
{
	pid_t pid;
	pid = pidof("iperf");

	killall("iperf", SIGTERM);
	unlink(iperf_interval);
	unlink(iperf_log);

	if (pid > 0)
		logmsg(LOG_INFO, "iperf stopped");
}
