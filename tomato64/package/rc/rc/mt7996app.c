/*
 * mt7996app.c
 *
 * Wi-Fi 7 dashboard daemon (https://github.com/rmandrad/mt7996app) for boards
 * with an mt7996e radio. It polls iw / nl80211 / hostapd / mt76 debugfs and
 * serves its own web UI. It has no authentication of its own, so it listens
 * only on a unix socket that httpd proxies under /mt7996app/ (behind the
 * normal web UI login); see httpd/mt7996app.c.
 *
 * Hooks into the rest of rc: start/stop in start_services()/stop_services()
 * and a "mt7996app" row in the service table.
 */

#include "rc.h"

#ifdef TOMATO64_HAS_MT7996APP

/* needed by logmsg() */
#define LOGMSG_DISABLE	DISABLE_SYSLOG_OS
#define LOGMSG_NVDEBUG	"mt7996app_debug"

#define MT7996APP_BIN	"/usr/sbin/mt7996app"
#define MT7996APP_SOCK	"/var/run/mt7996app.sock"	/* must match httpd/mt7996app.c */
#define HOSTAPD_CTRL	"/var/run/hostapd"

/* Bring the daemon in line with nvram. A running daemon is left alone. */
void start_mt7996app(void)
{
	char *argv[] = { MT7996APP_BIN,
	                 "--listen", "unix:" MT7996APP_SOCK,
	                 "--hostapd-ctrl", HOSTAPD_CTRL,
#if defined(TOMATO64_MT6000)
	                 /* mt7915e (Wi-Fi 6): the daemon only picks mt7996e phys by default */
	                 "--any-driver",
#endif
	                 NULL, NULL,	/* --history-file PATH */
	                 NULL };
	const int hist_arg = sizeof(argv) / sizeof(argv[0]) - 3;
	const char *hist;
	pid_t pid;

	if (!nvram_get_int("mt7996app_enable")) {
		stop_mt7996app();
		return;
	}

	if (pidof("mt7996app") > 0)
		return;

	/* Only an absolute path: anything else would be relative to wherever
	 * rc happens to be, and a leading '-' would read as an option.
	 */
	hist = nvram_safe_get("mt7996app_history");
	if (*hist == '/') {
		argv[hist_arg] = "--history-file";
		argv[hist_arg + 1] = (char *)hist;
	}
	else if (*hist)
		logmsg(LOG_WARNING, "mt7996app: ignoring history file '%s' (not an absolute path)", hist);

	if (_eval(argv, NULL, 0, &pid) != 0)
		logmsg(LOG_ERR, "mt7996app: failed to start");
	else
		logmsg(LOG_INFO, "mt7996app is started");
}

/* SIGTERM: the daemon writes its history file (if any) before it exits. */
void stop_mt7996app(void)
{
	if (pidof("mt7996app") > 0) {
		killall_tk_period_wait("mt7996app", 50);
		logmsg(LOG_INFO, "mt7996app is stopped");
	}
}

#endif /* TOMATO64_HAS_MT7996APP */
