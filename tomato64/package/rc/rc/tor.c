/*
 * tor.c
 *
 * Copyright (C) 2011 shibby
 *
 * Fixes/updates (C) 2018 - 2026 pedro
 * https://freshtomato.org/
 *
 */


#include "rc.h"

#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/time.h>

#define tor_config	"/etc/tor.conf"
#define tor_cookie_len	32

/* needed by logmsg() */
#define LOGMSG_DISABLE	DISABLE_SYSLOG_OSM
#define LOGMSG_NVDEBUG	"tor_debug"


/*
 * Send one Tor Control Protocol command and consume its single-line reply.
 *
 * Use the socket directly rather than an update-mode stdio stream: it avoids
 * read/write direction switching rules and lets MSG_NOSIGNAL protect rc from
 * a controller socket that closes while a command is being sent.
 */
static int tor_control_command(int fd, const char *command)
{
	char reply[128];
	const char *p = command;
	size_t left = strlen(command);
	ssize_t n;
	int used = 0;

	while (left > 0) {
		n = send(fd, p, left, MSG_NOSIGNAL);
		if (n < 0) {
			if (errno == EINTR)
				continue;
			return 0;
		}
		if (n == 0)
			return 0;

		p += n;
		left -= n;
	}

	/* AUTHENTICATE and SIGNAL NEWNYM both return one reply line. */
	while (used < (int)sizeof(reply) - 1) {
		n = recv(fd, &reply[used], 1, 0);
		if (n < 0) {
			if (errno == EINTR)
				continue;
			return 0;
		}
		if (n == 0)
			return 0;

		if (reply[used++] == '\n')
			break;
	}

	reply[used] = '\0';

	return (used >= 3) && (strncmp(reply, "250", 3) == 0);
}


/*
 * Ask the running Tor daemon for a new identity.
 *
 * The control listener is bound to loopback only and protected with Tor's
 * cookie authentication. CookieAuthFile is intentionally not configured:
 * Tor already stores the 32-byte cookie in DataDirectory/control_auth_cookie.
 *
 * SIGNAL NEWNYM affects new application requests only. Existing streams keep
 * their current circuits, and Tor may rate-limit repeated NEWNYM requests.
 */
int tor_newnym(void)
{
	struct sockaddr_in addr;
	struct timeval timeout;
	unsigned char cookie[tor_cookie_len];
	char cookie_hex[(tor_cookie_len * 2) + 1];
	char auth[sizeof(cookie_hex) + 16];
	char cookie_path[256];
	const char *datadir;
	int fd = -1;
	int port;
	int ret = -1;

	if (pidof("tor") <= 0)
		return -1;

	port = nvram_get_int("tor_ctrlport");
	if (port < 1 || port > 65535)
		goto out;

	datadir = nvram_safe_get("tor_datadir");
	if (snprintf(cookie_path, sizeof(cookie_path), "%s/control_auth_cookie", datadir) >= (int)sizeof(cookie_path))
		goto out;

	if (f_read(cookie_path, cookie, sizeof(cookie)) != (int)sizeof(cookie))
		goto out;

	if (bin2hex(cookie_hex, sizeof(cookie_hex), cookie, sizeof(cookie)) != 0)
		goto out;

	if ((fd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
		goto out;

	/*
	 * This action runs through rc's service dispatcher. Bound control I/O so
	 * an unhealthy Tor process cannot stall service handling indefinitely.
	 */
	timeout.tv_sec = 3;
	timeout.tv_usec = 0;
	if (setsockopt(fd, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout)) < 0)
		goto out;
	if (setsockopt(fd, SOL_SOCKET, SO_SNDTIMEO, &timeout, sizeof(timeout)) < 0)
		goto out;

	memset(&addr, 0, sizeof(addr));
	addr.sin_family = AF_INET;
	addr.sin_port = htons((unsigned short)port);
	addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);

	if (connect(fd, (struct sockaddr *)&addr, sizeof(addr)) < 0)
		goto out;

	snprintf(auth, sizeof(auth), "AUTHENTICATE %s\r\n", cookie_hex);
	if (!tor_control_command(fd, auth))
		goto out;

	if (!tor_control_command(fd, "SIGNAL NEWNYM\r\n"))
		goto out;

	ret = 0;
	logmsg(LOG_INFO, "Tor: new identity requested");

out:
	if (fd >= 0)
		close(fd);

	if (ret != 0)
		logmsg(LOG_WARNING, "Tor: failed to request new identity");

	return ret;
}


void start_tor(int force) {
	FILE *fp;
	char *ip;
	char buffer[16];
	int i;

	/* only if enabled or forced */
	if (!nvram_get_int("tor_enable") && force == 0)
		return;

	if (serialize_restart("tor", 1))
		return;

	/* dnsmasq uses this IP for nameserver to resolv .onion/.exit domains */
	ip = nvram_safe_get("lan_ipaddr");
	if (!nvram_get_int("tor_solve_only")) {
		for (i = 0 ; i < BRIDGE_COUNT; i++) {
			snprintf(buffer, sizeof(buffer), "br%d", i);
			if (nvram_match("tor_iface", buffer)) {
				ip = bridge_nvram_get(i, "ipaddr", buffer, sizeof(buffer));
				break;
			}
		}
	}

	/* writing data to file */
	if (!(fp = fopen(tor_config, "w"))) {
		logerr(__FUNCTION__, __LINE__, tor_config);
		return;
	}
	/* localhost ports, NoPreferIPv6Automap doesn't matter when applied only to DNSPort, but works fine with SocksPort */
	fprintf(fp, "SocksPort %d NoPreferIPv6Automap\n"
	            "AutomapHostsOnResolve 1\n" /* .exit/.onion domains support for LAN clients */
	            "VirtualAddrNetworkIPv4 172.16.0.0/12\n"
	            "VirtualAddrNetworkIPv6 [FC00::]/7\n"
	            "AvoidDiskWrites 1\n"
	            "RunAsDaemon 1\n"
	            "Log notice syslog\n"
	            "DataDirectory %s\n"
	            "TransPort %s:%s\n"
	            "DNSPort %s:%s\n"
	            "User nobody\n"
	            "%s\n"
	            "ControlPort 127.0.0.1:%d\n"
	            "CookieAuthentication 1\n",
	            nvram_get_int("tor_socksport"),
	            nvram_safe_get("tor_datadir"),
	            ip, nvram_safe_get("tor_transport"),
	            ip, nvram_safe_get("tor_dnsport"),
	            nvram_safe_get("tor_custom"),
	            nvram_get_int("tor_ctrlport"));

	fclose(fp);

	chmod(tor_config, 0644);
	chmod("/dev/null", 0666);

	mkdir(nvram_safe_get("tor_datadir"), 0700);

	xstart("chown", "nobody:nobody", nvram_safe_get("tor_datadir"));

	xstart("tor", "-f", tor_config);
}

void stop_tor(void) {
	if (serialize_restart("tor", 0))
		return;

	if (pidof("tor") > 0)
		killall_tk_period_wait("tor", 50);
}
