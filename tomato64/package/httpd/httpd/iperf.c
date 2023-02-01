/*

	Tomato Firmware
	Copyright (C) 2018 Michal Obrembski

*/

#include "tomato.h"
#include "httpd.h"
#include <sys/stat.h>

#define DEBUG_NOISY

#include <shared.h>

/*
Always clear log files before run!
IF IPERF is running and iperf.pid is present and INTERVAL is not present -> SERVER awaiting OK
IF IPERF is running and iperf.pid is present and INTERVAL is present -> SERVER receivin OK
IF IPERF is not running but unclar(INTERVAL) and logfile present -> Server finished
IF IPERF is running and no iperf.pid file is present and INTERVAL is present -> Client receiving
IF IPERF is running and no iperf.pid file is present and log is present -> Client receiving
*/

void wo_ttcpstatus(char *url)
{
	int iperf_pid;
	struct stat st;
	if (access( "/tmp/iperf_log", F_OK ) != -1) {
		/* OK, we got log but it may be empty (iperf just started) */
		stat("/tmp/iperf_log", &st);
		printf("Size of iperflog: %jd\n", (intmax_t)st.st_size);
		if (st.st_size > 0 ) {
			printf("Sending content of iperf log\n");
			do_file("/tmp/iperf_log");
			return;
		}
	}
	iperf_pid = pidof("iperf");
	_dprintf("PID of iperf: %d\n", iperf_pid);
	if (iperf_pid < 0) {
		web_puts("{ \"mode\": \"Stopped\" }");
		return;
	}
	if (access( "/var/run/iperf.pid", F_OK ) != -1) {
		/* We know we're in server mode */
		_dprintf("iperf.pid found, server mode\n");
		if (access( "/tmp/iperf_interval", F_OK ) != -1) {
			_dprintf("Sending contents of interval\n");

			stat("/tmp/iperf_interval", &st);
			if (st.st_size > 0) {
				do_file("/tmp/iperf_interval");
			} else {
				web_puts("{ \"mode\": \"Server waiting\"}");
			}
		} else {
			web_puts("{ \"mode\": \"Server waiting\"}");
		}
	} else {
		_dprintf("Client mode, sending interval\n");
		/* We know we're in client mode */
		stat("/tmp/iperf_interval", &st);
		if (st.st_size > 0) {
			do_file("/tmp/iperf_interval");
		} else {
			web_puts("{ \"mode\": \"Client preparing\"}");
		}
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

	unlink("/tmp/iperf_interval");
	unlink("/tmp/iperf_log");

	port = atoi(webcgi_safeget("_port", "5201"));
	udpMode = atoi(webcgi_safeget("_udpProto", "0"));
	byteLimitMode = atoi(webcgi_safeget("_limitMode", "0"));
	limit = strtoull(webcgi_safeget("_limit", "10"), NULL, 0);
	_dprintf("port: %d UDP: %s LimitMode %s Limit %llu Mode\n", port, udpMode == 0 ? "NO" : "YES", byteLimitMode == 0 ? "TIME" : "BYTE", limit);

	if ((v = webcgi_get("_mode")) != NULL && (*v)) {
		snprintf(tmp, sizeof(tmp), "%d", port);
		if (!strcmp(v, "server")) {
			_dprintf("Server\n");
			snprintf(cmdBuffer, sizeof(cmdBuffer), "iperf -J --logfile /tmp/iperf_log --intervalfile \
			        /tmp/iperf_interval -I /var/run/iperf.pid -s -1 -D -p %d", port);
		} else {
			if ((host = webcgi_get("_host")) != NULL && (*host)) {
				_dprintf("Client Address %s\n", host);
				snprintf(cmdBuffer, sizeof(cmdBuffer), "iperf -J --logfile /tmp/iperf_log --intervalfile \
				        /tmp/iperf_interval -p %d %s %s %llu -c %s &", port, udpMode == 1 ? "-u" : "", byteLimitMode == 1 ? "-n" : "-t", limit, host);
			}
		}
		_dprintf("Running command:\n %s \n", cmdBuffer);
		run_program(cmdBuffer);
	}
	_dprintf("\n");
}

void wo_ttcpkill(char *url)
{
	killall("iperf", SIGTERM);
	unlink("/tmp/iperf_interval");
	unlink("/tmp/iperf_log");
}
