/*
 *
 * FreshTomato Firmware
 * Copyright (C) 2018 Michal Obrembski
 *
 * Fixes/updates (C) 2018 - 2025 pedro
 * https://freshtomato.org/
 *
 */


#include "tomato.h"
#include <stdbool.h>
#include <sys/stat.h>

/* needed by logmsg() */
#define LOGMSG_DISABLE	DISABLE_SYSLOG_OSM
#define LOGMSG_NVDEBUG	"iperf_debug"


const char iperf_log[] = "/tmp/iperf_log";
const char iperf_interval[] = "/tmp/iperf_interval";
const char iperf_pidfile[] = "/var/run/iperf.pid";

/*
 * Always clear log files before run!
 * IF IPERF is running and iperf.pid is present and INTERVAL is not present -> SERVER awaiting OK
 * IF IPERF is running and iperf.pid is present and INTERVAL is present -> SERVER receiving OK
 * IF IPERF is not running but unclar(INTERVAL) and logfile present -> Server finished
 * IF IPERF is running and no iperf.pid file is present and INTERVAL is present -> Client receiving
 * IF IPERF is running and no iperf.pid file is present and log is present -> Client receiving
 */

static inline bool is_ascii_letter(unsigned char c)
{
	unsigned char lc = (unsigned char)(c | 0x20);

	return lc >= 'a' && lc <= 'z';
}

static inline bool is_ascii_digit(unsigned char c)
{
	return c >= '0' && c <= '9';
}

static bool is_valid_hostname(const char *s) {
	unsigned char c;
	const unsigned char *p;

	if (!s)
		return false;

	for (p = (const unsigned char *)s; *p; ++p) {
		c = *p;

		if ((is_ascii_letter(c)) || (is_ascii_digit(c)) || (c == '.') || (c == '-') || (c == ':'))
			continue;

		return false;
	}

	return true;
}

static void run_program(const char *program)
{
	pclose(popen(program, "r"));
}

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
			do_file((char *)iperf_log);
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
				do_file((char *)iperf_interval);
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
			do_file((char *)iperf_interval);
		else
			web_puts("{ \"mode\": \"Client preparing\"}");
	}
}

void wo_ttcprun(char *url)
{
	char tmp[128], cmdBuffer[256];
	char *v, *host;
	int port = 5201;
	int udpMode = 0;
	int byteLimitMode = 0;		/* Time limit by default */
	unsigned long long limit = 10;	/* 10 Seconds, by default */
#if defined(TCONFIG_BCMARM) && defined(TCONFIG_BCMSMP)
	int cpu_num = sysconf(_SC_NPROCESSORS_CONF);
	if (cpu_num < 1)
		cpu_num = 1;
#endif

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
			snprintf(cmdBuffer, sizeof(cmdBuffer), "iperf -J --logfile %s --intervalfile %s -I %s -s -1 -D -p %d", iperf_log, iperf_interval, iperf_pidfile, port);
		}
		else { /* client */
			host = webcgi_get("_host");
			if (!is_valid_hostname(host)) /* sanitize host name */
				return;

#if defined(TCONFIG_BCMARM) && defined(TCONFIG_BCMSMP)
			logmsg(LOG_INFO, "iperf started in client mode, address: %s, number of parallel streams: %d", host, cpu_num);
			snprintf(cmdBuffer, sizeof(cmdBuffer), "iperf -J --logfile %s --intervalfile %s -p %d %s %s %llu -c %s -P %d &", iperf_log, iperf_interval, port, udpMode == 1 ? "-u" : "", byteLimitMode == 1 ? "-n" : "-t", limit, host, cpu_num);
#else
			logmsg(LOG_INFO, "iperf started in client mode, address: %s", host);
			snprintf(cmdBuffer, sizeof(cmdBuffer), "iperf -J --logfile %s --intervalfile %s -p %d %s %s %llu -c %s &", iperf_log, iperf_interval, port, udpMode == 1 ? "-u" : "", byteLimitMode == 1 ? "-n" : "-t", limit, host);
#endif
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
