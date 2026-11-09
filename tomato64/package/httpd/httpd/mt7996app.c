/*
 * mt7996app.c
 *
 * Tools > Wireless Monitor: proxy to the mt7996app dashboard daemon.
 *
 * mt7996app serves its own page and JSON API but has no authentication, so rc
 * starts it on a unix socket only (rc/mt7996app.c) and the browser reaches it
 * through here, as /mt7996app/..., behind the normal web UI login - this is a
 * mime_handlers[] row with auth set, so a request never gets this far without
 * one.
 *
 * Only the daemon's read-only GET endpoints are passed on, each named below,
 * with a query string limited to the characters they use. Its POST control
 * endpoint is not: rc never enables it, and it has its own token scheme that
 * the web UI login would not cover.
 *
 * The daemon's status, Content-Type and its own security headers are kept;
 * everything else in the reply header is httpd's (send_header), so the login
 * cookie and frame policy match every other page. The body is streamed - the
 * history JSON can run to a couple of MB.
 *
 * Hooks into the rest of httpd: one row in mime_handlers[] and the include
 * that declares it.
 */

#include "tomato.h"
#include "mt7996app.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <unistd.h>
#include <errno.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <sys/un.h>

/* needed by logmsg() */
#define LOGMSG_DISABLE		DISABLE_SYSLOG_OSM
#define LOGMSG_NVDEBUG		"mt7996app_debug"

#define MT7996APP_SOCK		"/var/run/mt7996app.sock"	/* must match rc/mt7996app.c */
#define MT7996APP_PREFIX	"mt7996app/"

/* The daemon answers from memory; this only bounds a wedged one. */
#define IO_TIMEOUT_S		15

#define QUERY_MAX		64
#define HDR_MAX			4096

/* request path under MT7996APP_PREFIX -> path on the daemon */
static const struct {
	const char *url;
	const char *backend;
} routes[] = {
	{ "index.html",		"/"		},
	{ "api/state",		"/api/state"	},
	{ "api/history",	"/api/history"	},
	{ "api/events",		"/api/events"	},
	{ NULL,			NULL		}
};

static const char *backend_path(const char *url)
{
	int i;

	if (strncmp(url, MT7996APP_PREFIX, sizeof(MT7996APP_PREFIX) - 1) != 0)
		return NULL;

	url += sizeof(MT7996APP_PREFIX) - 1;
	for (i = 0; routes[i].url; i++) {
		if (strcmp(url, routes[i].url) == 0)
			return routes[i].backend;
	}

	return NULL;
}

/* "tier=1m", "since=42": nothing that could end the request line or add a header */
static int query_ok(const char *q)
{
	if (strlen(q) > QUERY_MAX)
		return 0;

	for (; *q; q++) {
		if (!((*q >= 'a' && *q <= 'z') || (*q >= 'A' && *q <= 'Z') || (*q >= '0' && *q <= '9') ||
		      *q == '=' || *q == '&' || *q == '_' || *q == '.' || *q == '-'))
			return 0;
	}

	return 1;
}

static int backend_connect(void)
{
	struct sockaddr_un sa;
	struct timeval tv = { IO_TIMEOUT_S, 0 };
	int fd;

	if ((fd = socket(AF_UNIX, SOCK_STREAM, 0)) < 0)
		return -1;

	setsockopt(fd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));
	setsockopt(fd, SOL_SOCKET, SO_SNDTIMEO, &tv, sizeof(tv));

	memset(&sa, 0, sizeof(sa));
	sa.sun_family = AF_UNIX;
	strlcpy(sa.sun_path, MT7996APP_SOCK, sizeof(sa.sun_path));

	if (connect(fd, (struct sockaddr *)&sa, sizeof(sa)) < 0) {
		logmsg(LOG_DEBUG, "*** [mt7996app] %s: %s: %s", __FUNCTION__, MT7996APP_SOCK, strerror(errno));
		close(fd);
		return -1;
	}

	return fd;
}

static int send_all(int fd, const char *buf, int len)
{
	int n;

	while (len > 0) {
		if ((n = send(fd, buf, len, MSG_NOSIGNAL)) < 0) {
			if (errno == EINTR)
				continue;
			return 0;
		}
		buf += n;
		len -= n;
	}

	return 1;
}

/* Value of header "name" in a NUL-terminated header block, copied into out. */
static int header_value(const char *hdrs, const char *name, char *out, size_t outlen)
{
	const char *p = hdrs;
	const char *v, *e;
	size_t nlen = strlen(name);

	while ((p = strstr(p, "\r\n")) != NULL) {
		p += 2;
		if ((strncasecmp(p, name, nlen) == 0) && (p[nlen] == ':')) {
			v = p + nlen + 1;
			v += strspn(v, " \t");
			if ((e = strstr(v, "\r\n")) == NULL)
				e = v + strlen(v);
			if ((size_t)(e - v) >= outlen)
				return 0;
			memcpy(out, v, e - v);
			out[e - v] = 0;
			return 1;
		}
	}

	return 0;
}

static void fail(int status, const char *msg)
{
	send_header(status, NULL, mime_plain, 0);
	web_puts(msg);
}

/*
 * GET mt7996app/index.html | mt7996app/api/{state,history,events}
 *
 * The row carries no mime type: the reply header is built here, from the
 * daemon's reply, so a daemon that is not running answers 503 instead of an
 * empty 200.
 */
void wo_mt7996app(char *url)
{
	char hdr[HDR_MAX + 1];
	char req[256];
	char ctype[128], csp[512], nosniff[32];
	char extra[sizeof(csp) + sizeof(nosniff) + 64];
	const char *path, *query;
	char *body;
	int fd, n, len, status;

	if (post) {
		fail(400, "read-only\n");
		return;
	}

	if ((path = backend_path(url)) == NULL) {
		fail(404, "not found\n");
		return;
	}

	query = getenv("QUERY_STRING");
	if (!query)
		query = "";
	if (!query_ok(query)) {
		fail(400, "bad query\n");
		return;
	}

	if ((fd = backend_connect()) < 0) {
		fail(503, "mt7996app is not running. Enable it on Tools > Wireless Monitor.\n");
		return;
	}

	len = snprintf(req, sizeof(req), "GET %s%s%s HTTP/1.0\r\nHost: localhost\r\n\r\n", path, (*query ? "?" : ""), query);
	if ((len >= (int)sizeof(req)) || !send_all(fd, req, len)) {
		close(fd);
		fail(503, "mt7996app did not take the request\n");
		return;
	}

	/* Read up to the end of the reply header; whatever follows it is body. */
	len = 0;
	body = NULL;
	while (len < HDR_MAX) {
		if ((n = recv(fd, hdr + len, HDR_MAX - len, 0)) < 0) {
			if (errno == EINTR)
				continue;
			break;
		}
		if (n == 0)
			break;
		len += n;
		hdr[len] = 0;
		if ((body = strstr(hdr, "\r\n\r\n")) != NULL)
			break;
	}

	if ((body == NULL) || (sscanf(hdr, "HTTP/%*d.%*d %d", &status) != 1)) {
		close(fd);
		fail(503, "bad reply from mt7996app\n");
		return;
	}

	body += 4;
	n = len - (body - hdr);	/* body bytes already read */
	body[-2] = 0;		/* end the header block after its last "\r\n" */

	if (!header_value(hdr, "Content-Type", ctype, sizeof(ctype)))
		strlcpy(ctype, mime_plain, sizeof(ctype));

	extra[0] = 0;
	if (header_value(hdr, "Content-Security-Policy", csp, sizeof(csp)))
		snprintf(extra, sizeof(extra), "Content-Security-Policy: %s", csp);
	if (header_value(hdr, "X-Content-Type-Options", nosniff, sizeof(nosniff)))
		snprintf(extra + strlen(extra), sizeof(extra) - strlen(extra), "%sX-Content-Type-Options: %s", (extra[0] ? "\r\n" : ""), nosniff);

	send_header(status, (extra[0] ? extra : NULL), ctype, 0);

	if (n > 0)
		web_write(body, n);

	while ((n = recv(fd, hdr, HDR_MAX, 0)) != 0) {
		if (n < 0) {
			if (errno == EINTR)
				continue;
			logmsg(LOG_DEBUG, "*** [mt7996app] %s: %s", __FUNCTION__, strerror(errno));
			break;
		}
		if (web_write(hdr, n) != n)
			break;
	}

	close(fd);
}
