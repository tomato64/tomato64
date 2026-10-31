/*
 * wanlink.c: Physical WAN link monitor for DHCP WAN
 *
 * Copyright (C) 2026 FreshTomato
 * https://freshtomato.org/
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston,
 * MA 02111-1307 USA
 *
 */


#include "rc.h"

/* needed by logmsg() */
#define LOGMSG_DISABLE		DISABLE_SYSLOG_OSM
#define LOGMSG_NVDEBUG		"wanlink_debug"

#define WANLINK_POLL_SECONDS	1
#define WANLINK_DEBOUNCE	2


static volatile sig_atomic_t wanlink_stop;

static void wanlink_signal(int sig)
{
	(void)sig;
	wanlink_stop = 1;
}

void stop_wanlink(char *prefix)
{
	char pidfile[64];

	if (!prefix || !*prefix)
		return;

	snprintf(pidfile, sizeof(pidfile), "/var/run/wanlink-%s.pid", prefix);
	if (kill_pidfile_s(pidfile, SIGTERM) == 0) {
		/* Let the old monitor remove its own pidfile before replacement. */
		if (!f_wait_notexists(pidfile, 3)) {
			kill_pidfile_s(pidfile, SIGKILL);
			unlink(pidfile);
		}
	}
	else {
		unlink(pidfile);
	}
}

void start_wanlink(char *prefix)
{
	if (!prefix || !*prefix || (get_wanx_proto(prefix) != WP_DHCP) || (wan_phy_link(prefix) < 0))
		return;

	stop_wanlink(prefix);
	xstart("wanlink", prefix);
}

int wanlink_main(int argc, char **argv)
{
	char *prefix;
	char pidfile[64], pidbuf[16];
	int stable = -1, candidate = -1, samples = 0;
	int current;

	if (argc < 2)
		return EINVAL;

	prefix = argv[1];
	if (!*prefix || (get_wanx_proto(prefix) != WP_DHCP) ||
	    (wan_phy_link(prefix) < 0))
		return EINVAL;

	snprintf(pidfile, sizeof(pidfile), "/var/run/wanlink-%s.pid", prefix);
	snprintf(pidbuf, sizeof(pidbuf), "%d", getpid());
	if (f_write_string(pidfile, pidbuf, FW_CREATE, 0644) < 0)
		return errno ? errno : EIO;

	signal(SIGTERM, wanlink_signal);
	signal(SIGINT, wanlink_signal);

	while (!wanlink_stop) {
		/* stop_wan_if() normally terminates us, but also stop cleanly if
		 * configuration changed underneath a running monitor.
		 */
		if (get_wanx_proto(prefix) != WP_DHCP)
			break;

		current = wan_phy_link(prefix);

		if (current >= 0) {
			if (stable < 0) {
				/* Learn boot state without generating a synthetic transition. */
				stable = current;
				candidate = -1;
				samples = 0;
			}
			else if (current == stable) {
				candidate = -1;
				samples = 0;
			}
			else {
				if (candidate != current) {
					candidate = current;
					samples = 1;
				}
				else {
					samples++;
				}

				if (samples >= WANLINK_DEBOUNCE) {
					stable = current;
					candidate = -1;
					samples = 0;

					if (!stable) {
						logmsg(LOG_NOTICE, "%s physical link DOWN; releasing DHCP lease", prefix);
						eval("dhcpc-release", prefix);
					}
					else {
						logmsg(LOG_NOTICE, "%s physical link UP; renewing DHCP lease", prefix);
						eval("dhcpc-renew", prefix);
					}
				}
			}
		}

		sleep(WANLINK_POLL_SECONDS);
	}

	unlink(pidfile);
	return 0;
}
