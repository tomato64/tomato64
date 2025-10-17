/*
 *
 * micro_httpd/mini_httpd
 *
 * Copyright © 1999,2000 by Jef Poskanzer <jef@acme.com>.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 */
/*
 *
 * Copyright 2003, CyberTAN  Inc.  All Rights Reserved
 *
 * This is UNPUBLISHED PROPRIETARY SOURCE CODE of CyberTAN Inc.
 * the contents of this file may not be disclosed to third parties,
 * copied or duplicated in any form without the prior written
 * permission of CyberTAN Inc.
 *
 * This software should be used as a reference only, and it not
 * intended for production use!
 *
 * THIS SOFTWARE IS OFFERED "AS IS", AND CYBERTAN GRANTS NO WARRANTIES OF ANY
 * KIND, EXPRESS OR IMPLIED, BY STATUTE, COMMUNICATION OR OTHERWISE.  CYBERTAN
 * SPECIFICALLY DISCLAIMS ANY IMPLIED WARRANTIES OF MERCHANTABILITY, FITNESS
 * FOR A SPECIFIC PURPOSE OR NONINFRINGEMENT CONCERNING THIS SOFTWARE
 *
 */
/*
 *
 * Modified for Tomato Firmware
 * Portions, Copyright (C) 2006-2009 Jonathan Zarate
 *
 * Fixes/updates (C) 2018 - 2025 pedro
 * https://freshtomato.org/
 *
 */


#ifdef TTYD_PROXY
#define _GNU_SOURCE
#endif /* TTYD_PROXY */
#include <errno.h>
#include <fcntl.h>
#include <time.h>
#include <netinet/in.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <getopt.h>
#include <sys/wait.h>
#ifndef TOMATO64
#include <error.h>
#include <sys/signal.h>
#else
#include <signal.h>
#endif /* TOMATO64 */
#include <netinet/tcp.h>
#include <sys/stat.h>
#ifdef TOMATO64
#include <limits.h>
#endif /* TOMATO64 */

#include <wlutils.h>
#include "tomato.h"
#ifdef TCONFIG_HTTPS
#include "mssl.h"
 #ifdef USE_OPENSSL
  #include <openssl/opensslv.h>
   #ifdef TTYD_PROXY
    #include <openssl/ssl.h>
   #endif /* TTYD_PROXY */
 #endif
#define HTTPS_CRT_VER		"1"
#endif

#ifndef TOMATO64
#define HTTP_MAX_LISTENERS	16
#else
#define HTTP_MAX_LISTENERS	17
#endif /* TOMATO64 */
#define SERVER_NAME		"httpd"
#define PROTOCOL		"HTTP/1.0"
#define RFC1123FMT		"%a, %d %b %Y %H:%M:%S GMT"
#ifdef TCONFIG_BCMARM
 #define MAX_CONN_ACCEPT	128
#else
 #define MAX_CONN_ACCEPT	64
#endif
#define MAX_CONN_TIMEOUT	30
#define USER_DEFAULT		"root"
#define PASS_DEFAULT		"admin"

/* needed by logmsg() */
#define LOGMSG_DISABLE		0
#define LOGMSG_NVDEBUG		"httpd_debug"


struct sockaddr_storage clientsai;
FILE *connfp = NULL;
int do_ssl;
int post;
int header_sent;
int connfd = -1;

const char mime_html[] = "text/html; charset=utf-8";
const char mime_plain[] = "text/plain";
const char mime_javascript[] = "text/javascript";
const char mime_binary[] = "application/tomato-binary-file"; /* instead of "application/octet-stream" to make browser just "save as" and prevent automatic detection weirdness */
const char mime_octetstream[] = "application/octet-stream";

typedef struct {
	int count;
	fd_set lfdset;
	struct {
		int listenfd;
		int ssl;
	} listener[HTTP_MAX_LISTENERS];
} listeners_t;

typedef enum {
	AUTH_NONE,
	AUTH_OK,
	AUTH_BAD
} auth_t;

char authinfo[512];
#ifdef TCONFIG_IPV6
char client_addr[INET6_ADDRSTRLEN];
#else
char client_addr[INET_ADDRSTRLEN];
#endif

static listeners_t listeners;
static int disable_maxage = 0;
static int http_port;
static int maxfd = -1;
static const int int_1 = 1;
static const char pidfile[] = "/var/run/httpd.pid";

#ifdef TTYD_PROXY
#include <sys/un.h>
#include <poll.h>
#include "ttyd.h"
#endif /* TTYD_PROXY */

static const char *http_status_desc(int status)
{
	switch (status) {
	case 200:
		return "OK";
	case 302:
		return "Found";
	case 400:
		return "Invalid Request";
	case 401:
		return "Unauthorized";
	case 404:
		return "Not Found";
	case 501:
		return "Not Implemented";
	}
	return "Unknown";
}

void send_header(int status, const char* header, const char* mime, int cache)
{
	time_t now;
	char tms[128];

	now = time(NULL);
	if (now < Y2K)
		now += Y2K;

	strftime(tms, sizeof(tms), RFC1123FMT, gmtime(&now));
	web_printf("%s %d %s\r\n"
	           "Server: %s\r\n"
	           "Date: %s\r\n"
	           "X-Frame-Options: SAMEORIGIN\r\n",
	           PROTOCOL, status, http_status_desc(status),
	           SERVER_NAME,
	           tms);

	if (mime)
		web_printf("Content-Type: %s\r\n", mime);

	if (cache > 0 && !disable_maxage)
		web_printf("Cache-Control: max-age=%d\r\n", cache * 3600);
	else {
		web_puts("Cache-Control: no-cache, no-store, must-revalidate, private\r\n"
		         "Expires: Thu, 31 Dec 1970 00:00:00 GMT\r\n"
		         "Pragma: no-cache\r\n");
	}
	if (header)
		web_printf("%s\r\n", header);

	web_puts("Connection: close\r\n\r\n");

	header_sent = 1;
}

void send_error(int status, const char *header, const char *text)
{
	const char *s = http_status_desc(status);

	send_header(status, header, mime_html, 0);
	web_printf("<html>"
	           "<head><title>Error</title></head>"
	           "<body>"
	           "<h2>%d %s</h2> %s"
	           "</body></html>",
	           status, s, text ? text : s);
}

void redirect(const char *path)
{
	char s[512];

	snprintf(s, sizeof(s), "Location: %s", path);
	send_header(302, s, mime_html, 0);
	web_puts(s);

	logmsg(LOG_DEBUG, "*** %s: Redirect: %s", __FUNCTION__, path);
}

int skip_header(int *len)
{
	char buf[2048];

	while (*len > 0) {
		if (!web_getline(buf, MIN((unsigned int) *len, sizeof(buf))))
			break;

		*len -= strlen(buf);
		if ((strcmp(buf, "\n") == 0) || (strcmp(buf, "\r\n") == 0))
			return 1;
	}

	return 0;
}

static void eat_garbage(void)
{
	int i;
	int flags;

	/* eat garbage \r\n (IE6, ...) or browser ends up with a tcp reset error message */
	if ((!do_ssl) && (post)) {
		if (((flags = fcntl(connfd, F_GETFL)) != -1) && (fcntl(connfd, F_SETFL, flags | O_NONBLOCK) != -1)) {
			for (i = 0; i < 1024; ++i) {
				if (fgetc(connfp) == EOF)
					break;
			}
			fcntl(connfd, F_SETFL, flags);
		}
	}
}

static void send_authenticate(void)
{
	char header[128];
	const char *realm;

	realm = nvram_get("router_name");
	if ((realm == NULL) || (*realm == 0) || (strlen(realm) > 64))
		realm = "unknown";

	memset(header, 0, sizeof(header));
	snprintf(header, sizeof(header), "WWW-Authenticate: Basic realm=\"%s\"", realm);
	send_error(401, header, NULL);
}

static void auth_fail(int clen)
{
	if (post)
		web_eat(clen);

	eat_garbage();
	send_authenticate();
}

static void get_client_addr(void)
{
	void *addr = NULL;

	if (clientsai.ss_family == AF_INET)
		addr = &(((struct sockaddr_in*) &clientsai)->sin_addr);
#ifdef TCONFIG_IPV6
	else if (clientsai.ss_family == AF_INET6)
		addr = &(((struct sockaddr_in6*) &clientsai)->sin6_addr);
#endif

	inet_ntop(clientsai.ss_family, addr, client_addr, sizeof(client_addr));
}

static auth_t auth_check(const char *authorization)
{
	const char *u, *p;
	char* pass;
	int len;

	/* basic authorization info? */
	if ((!authorization) || (strncmp(authorization, "Basic ", 6) != 0))
		return AUTH_NONE;

	/* something's wrong */
	if (base64_decoded_len(strlen(authorization + 6)) > sizeof(authinfo))
		return AUTH_BAD;

	/* decode it */
	len = base64_decode(authorization + 6, (unsigned char *)authinfo, strlen(authorization) - 6);
	authinfo[len] = '\0';

	/* split into user and password */
	pass = strchr(authinfo, ':');
	if (pass == (char*)0) {
		/* no colon? bogus auth info */
		return AUTH_NONE;
	}
	*pass++ = 0;

	/* is this the right user and password? */
	if (((u = nvram_get("http_username")) == NULL) || (*u == 0)) /* special case: empty username */
		u = USER_DEFAULT;

	if (((p = nvram_get("http_passwd")) == NULL) || (*p == 0)) /* special case: empty password */
		p = PASS_DEFAULT;

	if (strcmp(authinfo, u) == 0 && strcmp(pass, p) == 0) {
		return AUTH_OK;
	}
	else {
		/* failed login msg to syslog */
		logmsg(LOG_WARNING, "login '%s' failed (GUI) from %s:%d", authinfo, client_addr, http_port);
	}

	return AUTH_BAD;
}

static int check_wif(int idx, int unit, int subunit, void *param)
{
	sta_info_t *sti = param;

	return (wl_ioctl(nvram_safe_get(wl_nvname("ifname", unit, subunit)), WLC_GET_VAR, sti, sizeof(*sti)) == 0);
}

#ifdef TOMATO64
static int is_wifi_client(char* mac)
{
	FILE *f1;
	FILE *f2;
	char buffer1[32];
	char buffer2[128];

	const char cmd[] = "iw dev | grep Interface | awk '{print $2}'";

	if ((f1 = popen(cmd, "r"))) {
		while (fgets(buffer1, sizeof(buffer1), f1)) {
			buffer1[strcspn(buffer1, "\n")] = 0;
			snprintf(buffer2, sizeof(buffer2), "iw dev %s station dump | grep -i %s", buffer1, mac);
			if((f2 = popen(buffer2, "r"))) {
				if (fgets(buffer2, sizeof(buffer2), f2) != NULL) {
					pclose(f2);
					pclose(f1);
					return 1;
				}
				pclose(f2);
			}
		}
		pclose(f1);
	}
	return 0;
}
#endif /* TOMATO64 */

static int check_wlaccess(void)
{
	char mac[32];
	char ifname[32];
	sta_info_t sti;

	if (nvram_get_int("web_wl_filter")) {
		if (get_client_info(mac, ifname)) {
#ifndef TOMATO64
			memset(&sti, 0, sizeof(sti));
			strlcpy((char *)&sti, "sta_info", sizeof(sti)); /* sta_info0<mac> */
			ether_atoe(mac, (unsigned char *)&sti + 9);
			if (foreach_wif(1, &sti, check_wif)) {
#else
			if (is_wifi_client(mac)) {
#endif /* TOMATO64 */
				if (nvram_get_int("debug_logwlac"))
					logmsg(LOG_WARNING, "wireless access from %s blocked", mac);

				return 0;
			}
		}
	}

	return 1;
}

/*
 * Match a single pattern against a string.
 * Supports ?, *, and ** wildcards.
 * This implementation is fully NON-RECURSIVE.
 */
static int match_single(const char *pattern, int patternlen, const char *string)
{
	const char *p = pattern; /* current position in pattern */
	const char *s = string; /* current position in string */

	const char *last_star_pat = NULL; /* position in pattern after last '*' */
	const char *last_star_str = NULL; /* position in string corresponding to that '*' */
	int double_star = 0; /* flag for '**' */

	while (1) {
		/* handle '*' and '**' wildcards */
		if (p - pattern < patternlen && *p == '*') {
			++p;
			double_star = 0;
			if (p - pattern < patternlen && *p == '*') {
				++p;
				double_star = 1; /* double-wildcard matches across '/' */
			}

			/* save positions for possible backtracking */
			last_star_pat = p;
			last_star_str = s;

			/* if '*' at the end of pattern, it matches everything */
			if (p - pattern >= patternlen)
				return 1;

			continue;
		}

		/* handle '?' wildcard (matches any single char except end of string) */
		if (p - pattern < patternlen && *p == '?') {
			if (*s == '\0')
				goto backtrack;

			++p; ++s;
			continue;
		}

		/* direct character match */
		if (p - pattern < patternlen && *p == *s) {
			if (*s == '\0')
				return 0; /* string ended but pattern did not */

			++p; ++s;
			continue;
		}

		/* if both pattern and string are fully consumed => success */
		if (p - pattern >= patternlen && *s == '\0')
			return 1;

		/* attempt to backtrack if mismatch occurs */
backtrack:
		if (last_star_pat) {
			/* advance string by one char from last saved star position */
			if (*last_star_str == '\0')
				return 0; /* nothing left to consume */

			/* for single '*' we cannot cross '/' */
			if (!double_star && *last_star_str == '/')
				return 0;

			++last_star_str;
			p = last_star_pat;
			s = last_star_str;
			continue;
		}

		return 0; /* no backtrack available -> failure */
	}
}

/*
 * Match string against multiple patterns separated by '|'.
 * Also fully NON-RECURSIVE.
 */
static int match(const char *pattern, const char *string)
{
	const char *start = pattern;
	const char *sep;
	int len;

	for (;;) {
		sep = strchr(start, '|'); /* find '|' separator */
		len = (sep ? sep - start : (int)strlen(start));

		if (match_single(start, len, string))
			return 1; /* match success */

		if (!sep)
			break; /* no more patterns */

		start = sep + 1;
	}

	return 0; /* none of the patterns matched */
}

void do_file(char *path)
{
	FILE *f;
	char buf[1024];
	int nr;
	if ((f = fopen(path, "r"))) {
		while ((nr = fread(buf, 1, sizeof(buf), f)) > 0)
			web_write(buf, nr);

		fclose(f);
	}
}

static void handle_request(void)
{
	char line[10000], *cur;
	char *method, *path, *protocol, *authorization, *boundary, *useragent;
	char *cp;
	char *file;
	int len;
	const struct mime_handler *handler;
	int cl = 0;
	auth_t auth;

#ifdef TTYD_PROXY
	char *upgrade_header = NULL, *connection_header = NULL;
	char *ws_key_header = NULL;
#endif

	logmsg(LOG_DEBUG, "*** %s: IN ***", __FUNCTION__);

	/* initialize variables */
	header_sent = 0;
	authorization = boundary = useragent = NULL;
	memset(line, 0, sizeof(line));

	/* parse the first line of the request */
	if (!web_getline(line, sizeof(line))) {
		send_error(400, NULL, NULL);
		return;
	}

	logmsg(LOG_DEBUG, "*** %s: line: %s", __FUNCTION__, line);

	method = path = line;
	strsep(&path, " ");
	while (path && *path == ' ')
		path++;

	protocol = path;
	strsep(&protocol, " ");
	while (protocol && *protocol == ' ')
		protocol++;

	if (!path || !protocol) { /* avoid http server crash */
		send_error(400, NULL, NULL);
		return;
	}

	post = (strcasecmp(method, "post") == 0);
	if ((strcasecmp(method, "get") != 0) && !post) {
		send_error(501, NULL, NULL);
		return;
	}

	if (path[0] != '/') {
		send_error(400, NULL, NULL);
		return;
	}

	file = &(path[1]);
	len = strlen(file);

	logmsg(LOG_DEBUG, "*** %s: file: %s", __FUNCTION__, file);

	if ((cp = strchr(file, '?')) != NULL) {
		*cp = 0;
		setenv("QUERY_STRING", cp + 1, 1);
		webcgi_init(cp + 1);
	}

	logmsg(LOG_DEBUG, "*** %s: url : %s", __FUNCTION__, file);

	if ((file[0] == '/') || (strncmp(file, "..", 2) == 0) || (strncmp(file, "../", 3) == 0) || (strstr(file, "/../") != NULL) || (strcmp(&(file[len - 3]), "/..") == 0)) {
		send_error(400, NULL, NULL);
		return;
	}

	if ((file[0] == '\0') || (file[len - 1] == '/') || (strcmp(file, "index.asp") == 0))
		file = "status-overview.asp";
	else if ((strcmp(file, "ext/") == 0) || (strcmp(file, "ext") == 0))
		file = "ext/index.asp";

#ifdef TTYD_PROXY
	/* Helper macro to strip trailing whitespace - CRITICAL for string comparisons */
	#define STRIP_TRAILING_WS(str) do { \
		char *end = (str) + strlen(str) - 1; \
		while (end > (str) && (*end == ' ' || *end == '\t' || *end == '\r' || *end == '\n')) \
			*end-- = '\0'; \
	} while (0)
#endif /* TTYD_PROXY */

	cp = protocol;
	strsep(&cp, " ");
	cur = protocol + strlen(protocol) + 1;

	while (web_getline(cur, line + sizeof(line) - cur)) {
		if ((strcmp(cur, "\n") == 0) || (strcmp(cur, "\r\n") == 0))
			break;
		else if (strncasecmp(cur, "Authorization:", 14) == 0) {
			cp = &cur[14];
			cp += strspn(cp, " \t");
			authorization = cp;
			cur = cp + strlen(cp) + 1;
			logmsg(LOG_DEBUG, "*** %s: httpd authorization: %s", __FUNCTION__, authorization);
		}
		else if (strncasecmp(cur, "User-Agent:", 11) == 0) {
			cp = &cur[11];
			cp += strspn(cp, " \t");
			useragent = cp;
			cur = cp + strlen(cp) + 1;
			logmsg(LOG_DEBUG, "*** %s: httpd user-agent: %s", __FUNCTION__, useragent);
		}
		else if (strncasecmp(cur, "Content-Length:", 15) == 0) {
			cp = &cur[15];
			cp += strspn(cp, " \t");
			cl = strtoul(cp, NULL, 0);
			if ((cl < 0) || (cl >= INT_MAX)) {
				send_error(400, NULL, NULL);
				return;
			}
		}
		else if ((strncasecmp(cur, "Content-Type:", 13) == 0) && ((cp = strstr(cur, "boundary=")))) {
			boundary = &cp[9];
			for (cp = cp + 9; *cp && *cp != '\r' && *cp != '\n'; cp++);
			*cp = '\0';
			cur = ++cp;
			logmsg(LOG_DEBUG, "*** %s: boundary: %s", __FUNCTION__, boundary);
		}
#ifdef TTYD_PROXY
		else if (strncasecmp(cur, "Upgrade:", 8) == 0) {
			cp = &cur[8];
			cp += strspn(cp, " \t");
			upgrade_header = cp;
			STRIP_TRAILING_WS(cp);
			cur = upgrade_header + strlen(upgrade_header) + 1;
			logmsg(LOG_DEBUG, "*** %s: FOUND Upgrade header: '%s'", __FUNCTION__, upgrade_header);
		}
		else if (strncasecmp(cur, "Connection:", 11) == 0) {
			cp = &cur[11];
			cp += strspn(cp, " \t");
			connection_header = cp;
			STRIP_TRAILING_WS(cp);
			cur = connection_header + strlen(connection_header) + 1;
			logmsg(LOG_DEBUG, "*** %s: FOUND Connection header: '%s'", __FUNCTION__, connection_header);
		}
		else if (strncasecmp(cur, "Sec-WebSocket-Key:", 18) == 0) {
			cp = &cur[18];
			cp += strspn(cp, " \t");
			ws_key_header = cp;
			STRIP_TRAILING_WS(cp);
			cur = ws_key_header + strlen(ws_key_header) + 1;
			logmsg(LOG_DEBUG, "*** %s: WebSocket-Key: '%s'", __FUNCTION__, ws_key_header);
		}
#endif /* TTYD_PROXY */
	}

#ifdef TTYD_PROXY
	#undef STRIP_TRAILING_WS
#endif /* TTYD_PROXY */

	get_client_addr();
#ifdef TTYD_PROXY
	/* Check for WebSocket upgrade request */
	if (ttyd_is_websocket_upgrade(upgrade_header, connection_header)) {
		int auth_required = 0;
		if (ttyd_has_route(path, &auth_required)) {
			/* Check authentication if required */
			if (auth_required) {
				auth_t ws_auth = auth_check(authorization);
				if (ws_auth != AUTH_OK) {
					auth_fail(cl);
					return;
				}
			}

			if (!check_wlaccess()) {
				send_error(403, NULL, "Access denied");
				return;
			}

			/* Handle WebSocket upgrade */
			if (ttyd_handle_websocket_upgrade(path, ws_key_header) < 0) {
				send_error(500, NULL, "WebSocket upgrade failed");
			}
			return; /* CRITICAL: Return immediately - do not continue processing */
		}
		else {
			logmsg(LOG_DEBUG, "*** %s: No WebSocket route found for path: %s", __FUNCTION__, path);
			/* Fall through to regular HTTP handling */
		}
	}

	/* Handle regular HTTP requests to console paths (NOT WebSocket upgrades) */
	/* This only runs if we didn't return above from WebSocket handling */
	{
		char *response = NULL;
		size_t response_len = 0;
		int result = ttyd_handle_http_request(file, &response, &response_len);

		if (result == 1) {
			/* Console path was handled - check auth and send response */
			auth_t console_auth = auth_check(authorization);
			if (console_auth != AUTH_OK) {
				free(response);
				auth_fail(cl);
				return;
			}

			if (!check_wlaccess()) {
				free(response);
				send_error(403, NULL, "Access denied");
				return;
			}

			web_write(response, response_len);
			free(response);
			return;
		}
		else if (result == -1) {
			/* Error handling console request */
			send_error(502, NULL, "Failed to connect to terminal service");
			return;
		}
		/* result == 0: not a console path, fall through to regular HTTP handling */
	}
#endif /* TTYD_PROXY */
	auth = auth_check(authorization);

	if (auth == AUTH_BAD) {
		auth_fail(cl);
		return;
	}

	for (handler = &mime_handlers[0]; handler->pattern; handler++) {
		logmsg(LOG_DEBUG, "*** %s: handler->pattern: [%s] file: [%s]", __FUNCTION__, handler->pattern, file);

		if (match(handler->pattern, file)) {
			if ((handler->auth) && (auth != AUTH_OK)) {
				auth_fail(cl);
				return;
			}

			if (handler->input)
				handler->input(file, cl, boundary);

			eat_garbage();

			if (handler->mime_type != NULL)
				send_header(200, NULL, handler->mime_type, handler->cache);

			if (handler->output)
				handler->output(file);

			return;
		}
	}

	if (auth != AUTH_OK) {
		auth_fail(cl);
		return;
	}

	if (strcmp(file, "logout") == 0) { /* special case */
		wi_generic(file, cl, boundary);
		eat_garbage();
		send_authenticate();

		/* send logout msg to syslog */
		logmsg(LOG_INFO, "logout '%s' successful (GUI) %s:%d", authinfo, client_addr, http_port);
		send_error(404, NULL, "Goodbye");
		return;
	}

	send_error(404, NULL, NULL); /* not found */
}

#ifdef TCONFIG_HTTPS
static void save_cert(void)
{
	if (eval("tar", "-C", "/", "-czf", "/tmp/cert.tgz", "etc/cert.pem", "etc/key.pem") == 0) {
		if (nvram_set_file("https_crt_file", "/tmp/cert.tgz", 8192)) {
			nvram_set("crt_ver", HTTPS_CRT_VER);
			nvram_commit_x();
		}
	}
	unlink("/tmp/cert.tgz");
}

static void erase_cert(void)
{
	unlink("/etc/cert.pem");
	unlink("/etc/key.pem");
	unlink("/etc/server.pem");
	nvram_unset("https_crt_file");
	nvram_unset("https_crt_gen");
}

static void start_ssl(void)
{
	int i, lock, ok, retry, save;
	char t[32];

	lock = file_lock("httpd");

	/* avoid collisions if another httpd instance is initializing SSL cert */
	for (i = 1; i < 5; i++) {
		if (lock < 0)
			sleep(i * i);
		else
			i = 5;
	}

	if (nvram_match("https_crt_gen", "1"))
		erase_cert();

	retry = 1;
	while (1) {
		save = nvram_get_int("https_crt_save");

		if ((!f_exists("/etc/cert.pem")) || (!f_exists("/etc/key.pem"))
#if defined(USE_OPENSSL) && OPENSSL_VERSION_NUMBER >= 0x10100000L
		    || !mssl_cert_key_match("/etc/cert.pem", "/etc/key.pem")
#endif
		    || ((!nvram_get_int("https_crt_timeset")) && (time(NULL) > Y2K))) {
			ok = 0;
			if (save && nvram_match("crt_ver", HTTPS_CRT_VER)) {
				if (nvram_get_file("https_crt_file", "/tmp/cert.tgz", 8192)) {
					if (eval("tar", "-xzf", "/tmp/cert.tgz", "-C", "/", "etc/cert.pem", "etc/key.pem") == 0) {
						system("cat /etc/key.pem /etc/cert.pem > /etc/server.pem");

#if defined(USE_OPENSSL) && OPENSSL_VERSION_NUMBER >= 0x10100000L
						/* check key and cert pair, if they are mismatched, regenerate key and cert */
						if (mssl_cert_key_match("/etc/cert.pem", "/etc/key.pem")) {
							logmsg(LOG_INFO, "mssl_cert_key_match : PASS");
#endif
							ok = 1;
#if defined(USE_OPENSSL) && OPENSSL_VERSION_NUMBER >= 0x10100000L
						}
#endif
					}
					unlink("/tmp/cert.tgz");
				}
			}

			if (!ok) {
				erase_cert();
				logmsg(LOG_INFO, "generating SSL certificate...");

				/* browsers seem to like this when the ip address moves... */
				gen_urandom(t, NULL, sizeof(t), 0);
				eval("gencert.sh", t);
			}
		}

		if ((save) && (*nvram_safe_get("https_crt_file")) == 0)
			save_cert();

		if (mssl_init("/etc/cert.pem", "/etc/key.pem")) {
			file_unlock(lock);
			return;
		}
		erase_cert();

		logmsg(retry ? LOG_WARNING : LOG_ERR, "unable to start SSL");

		if (!retry) {
			file_unlock(lock);
			exit(1);
		}
		retry = 0;
	}
}
#endif /* TCONFIG_HTTPS */

static void init_id(void)
{
	char s[128];

	if (strncmp(nvram_safe_get("http_id"), "TID", 3) != 0) {
		gen_urandom(s, NULL, sizeof(s), 1);
		nvram_set("http_id", s);
	}

	nvram_unset("http_id_warn");
}

void check_id(const char *url)
{
	char s[72];
	char *i;
	char *u;

	if (!nvram_match("http_id", webcgi_safeget("_http_id", ""))) {
		memset(s, 0, sizeof(s));
		snprintf(s, sizeof(s), "%s,%ld", client_addr, time(NULL) & 0xFFC0);
		if (!nvram_match("http_id_warn", s)) {
			nvram_set("http_id_warn", s);

			strlcpy(s, webcgi_safeget("_http_id", ""), sizeof(s));
			i = js_string(s); /* quicky scrub */

			strlcpy(s, url, sizeof(s));
			u = js_string(s);

			if ((i != NULL) && (u != NULL)) {
				get_client_addr();
				logmsg(LOG_WARNING, "invalid ID '%s' from %s for /%s", i, client_addr, u);
			}
		}

		exit(1);
	}
}

static void add_listen_socket(const char *addr, int server_port, int do_ipv6, int do_ssl)
{
	int listenfd;
	struct sockaddr_storage sai_stor;

#ifdef TCONFIG_IPV6
	sa_family_t HTTPD_FAMILY = do_ipv6 ? AF_INET6 : AF_INET;
#else
#define HTTPD_FAMILY AF_INET
#endif

	if (listeners.count >= HTTP_MAX_LISTENERS) {
		logmsg(LOG_ERR, "number of listeners exceeded the max allowed (%d)", HTTP_MAX_LISTENERS);
		return;
	}

	if (server_port <= 0) {
		IF_TCONFIG_HTTPS(if (do_ssl) server_port = 443; else)
		server_port = 80;
	}

	if ((listenfd = socket(HTTPD_FAMILY, SOCK_STREAM, 0)) < 0) {
		logmsg(LOG_ERR, "create listening socket: %m");
		return;
	}
	fcntl(listenfd, F_SETFD, FD_CLOEXEC);

	setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR, &int_1, sizeof(int_1));

#ifdef TCONFIG_IPV6
	if (do_ipv6) {
		struct sockaddr_in6 *sai = (struct sockaddr_in6 *) &sai_stor;
		sai->sin6_family = HTTPD_FAMILY;
		sai->sin6_port = htons(server_port);
		if (addr && *addr)
			inet_pton(HTTPD_FAMILY, addr, &(sai->sin6_addr));
		else
			sai->sin6_addr = in6addr_any;

		setsockopt(listenfd, IPPROTO_IPV6, IPV6_V6ONLY, &int_1, sizeof(int_1));
	} else
#endif /* TCONFIG_IPV6 */
	{
		struct sockaddr_in *sai = (struct sockaddr_in *) &sai_stor;
		sai->sin_family = HTTPD_FAMILY;
		sai->sin_port = htons(server_port);
		sai->sin_addr.s_addr = (addr && *addr) ? inet_addr(addr) : INADDR_ANY;
	}

	if (bind(listenfd, (struct sockaddr *)&sai_stor, sizeof(sai_stor)) < 0) {
		logmsg(LOG_ERR, "bind: [%s]:%d: %m", (addr && *addr) ? addr : (IF_TCONFIG_IPV6(do_ipv6 ? "::" :) ""), server_port);
		close(listenfd);
		return;
	}

	if (listen(listenfd, MAX_CONN_ACCEPT) < 0) {
		logmsg(LOG_ERR, "listen: %m");
		close(listenfd);
		return;
	}

	if (listenfd >= 0) {
		logmsg(LOG_DEBUG, "*** %s: added IPv%d listener [%d] for %s:%d, ssl=%d", __FUNCTION__, (do_ipv6 ? 6 : 4), listenfd, (addr && *addr) ? addr : "addr_any", server_port, do_ssl);

		listeners.listener[listeners.count].listenfd = listenfd;
		listeners.listener[listeners.count++].ssl = do_ssl;
		FD_SET(listenfd, &listeners.lfdset);

		if (maxfd < listenfd)
			maxfd = listenfd;
	}
}

static void listen_wan(char* wan, wanface_list_t wanXfaces, int wanport)
{
	int i;
	char *ip;

	if (check_wanup(wan)) {
		memcpy(&wanXfaces, get_wanfaces(wan), sizeof(wanXfaces));
		for (i = 0; i < wanXfaces.count; ++i) {
			ip = wanXfaces.iface[i].ip;
			if (!(*ip) || strcmp(ip, "0.0.0.0") == 0)
				continue;

			add_listen_socket(ip, wanport, 0, nvram_get_int("remote_mgt_https"));
		}
	}
}

static void setup_listeners(int do_ipv6)
{
	char ipaddr[BRIDGE_COUNT][INET6_ADDRSTRLEN] = {{0}};
	int http_lan_listeners = nvram_get_int("http_lan_listeners"); /* Enable listeners: bit 0 = LAN1, bit 1 = LAN2, bit 2 = LAN3, 1 == TRUE, 0 == FALSE */
	IF_TCONFIG_IPV6(const char *wanaddr);
	int wanport = nvram_get_int("http_wanport");
	IF_TCONFIG_IPV6(int wan6port = wanport);
	int i, j, lanport;
	char buf[8];
	wanface_list_t wanfaces[MWAN_MAX];

#ifdef TCONFIG_IPV6
	/* get the configured routable IPv6 address from the lan iface.
	 * add_listen_socket() will fall back to in6addr_any
	 * if NULL or empty address is returned
	 */
	if (do_ipv6)
		strlcpy(ipaddr[0], getifaddr(nvram_safe_get("lan_ifname"), AF_INET6, 0) ? : "", sizeof(ipaddr[0]));
	else
#endif /* TCONFIG_IPV6 */
		strlcpy(ipaddr[0], nvram_safe_get("lan_ipaddr"), sizeof(ipaddr[0]));

	for (i = 1; i < BRIDGE_COUNT; i++)
	{
		char b[16];
#ifdef TCONFIG_IPV6
		char *nv;
		if (do_ipv6) {
			snprintf(b, sizeof(b), "lan%d_ifname", i);
			nv = nvram_safe_get(b);
			if (strncmp(nv, "br", 2) == 0) {
				strlcpy(ipaddr[i], getifaddr(nv, AF_INET6, 0) ? : "", sizeof(ipaddr[i]));
			}
			else {
				strlcpy(ipaddr[i], "", sizeof(ipaddr[i]));
			}
		} else
#endif /* TCONFIG_IPV6 */
		{
			snprintf(b, sizeof(b), "lan%d_ipaddr", i);
			strlcpy(ipaddr[i], nvram_safe_get(b), sizeof(ipaddr[i]));
		}
	}

	if (nvram_get_int("http_enable")) {
		lanport = nvram_get_int("http_lanport");
		add_listen_socket(ipaddr[0], lanport, do_ipv6, 0);
		for(i = 1; i < BRIDGE_COUNT; i++)
		{
			if ((strcmp(ipaddr[i], "") != 0) && (http_lan_listeners & (1 << (i-1)))) /* check for LAN1, LAN2, LAN3 */
				add_listen_socket(ipaddr[i], lanport, do_ipv6, 0);
		}

		IF_TCONFIG_IPV6(if (do_ipv6 && wanport == lanport) wan6port = 0);
	}

#ifdef TCONFIG_HTTPS
	if (nvram_get_int("https_enable")) {
		do_ssl = 1;
		lanport = nvram_get_int("https_lanport");
		add_listen_socket(ipaddr[0], lanport, do_ipv6, 1);
		for(i = 1; i < BRIDGE_COUNT; i++)
		{
			if ((strcmp(ipaddr[i], "") != 0) && (http_lan_listeners & (1 << (i-1)))) /* check for LAN1, LAN2, LAN3 */
				add_listen_socket(ipaddr[i], lanport, do_ipv6, 1);
		}

		IF_TCONFIG_IPV6(if (do_ipv6 && wanport == lanport) wan6port = 0);
	}
#endif /* TCONFIG_HTTPS */

	/* remote management */
	if ((wanport) && nvram_get_int("remote_management")) {
		IF_TCONFIG_HTTPS(if (nvram_get_int("remote_mgt_https")) do_ssl = 1);
#ifdef TCONFIG_IPV6
		if (do_ipv6) {
			if (*ipaddr[0] && wan6port)
				add_listen_socket(ipaddr[0], wan6port, 1, nvram_get_int("remote_mgt_https"));

			if (*ipaddr[0] || wan6port) {
				/* get the IPv6 address from wan iface */
				wanaddr = getifaddr((char *)get_wan6face(), AF_INET6, 0);
				if (wanaddr && *wanaddr && strcmp(wanaddr, ipaddr[0]) != 0)
					add_listen_socket(wanaddr, wanport, 1, nvram_get_int("remote_mgt_https"));
			}
		} else
#endif /* TCONFIG_IPV6 */
		{
			for (j = 1; j <= MWAN_MAX; j++) {
				memset(buf, 0, sizeof(buf));
				snprintf(buf, sizeof(buf), (j == 1 ? "wan" : "wan%d"), j);
				listen_wan(buf, wanfaces[j - 1], wanport);
			}
		}
	}
}

static void close_listen_sockets(void)
{
	int i;

	for (i = listeners.count - 1; i >= 0; --i) {
		if (listeners.listener[i].listenfd >= 0)
			close(listeners.listener[i].listenfd);
	}
	listeners.count = 0;
	FD_ZERO(&listeners.lfdset);
	maxfd = -1;
}

int main(int argc, char **argv)
{
	FILE *pid_fp;
	int c;
	fd_set rfdset;
	int i;
	struct sockaddr_storage sai;
	socklen_t sz;
	char bind[128];
	char *port = NULL;
#ifdef TCONFIG_IPV6
	int ip6 = 0;
#else
#define ip6 0
#endif

	openlog("httpd", LOG_PID, LOG_DAEMON);

	do_ssl = 0;
	http_port = nvram_get_int("http_lanport");
	listeners.count = 0;
	FD_ZERO(&listeners.lfdset);
	memset(bind, 0, sizeof(bind));

	if (argc) {
		while ((c = getopt(argc, argv, "Np:s:")) != -1) {
			switch (c) {
			case 'N':
				disable_maxage = 1;
				break;
			case 'p':
			case 's':
				/* [addr:]port */
				if ((port = strrchr(optarg, ':')) != NULL) {
					if ((optarg[0] == '[') && (port > optarg) && (port[-1] == ']'))
						memcpy(bind, optarg + 1, MIN(sizeof(bind), (unsigned int) (port - optarg) - 2));
					else
						memcpy(bind, optarg, MIN(sizeof(bind), (unsigned int) (port - optarg)));
					port++;
				}
				else
					port = optarg;

				IF_TCONFIG_HTTPS(if (c == 's') do_ssl = 1);
				IF_TCONFIG_IPV6(ip6 = (*bind && strchr(bind, ':')));
				http_port = atoi(port);
				add_listen_socket(bind, http_port, ip6, (c == 's'));

				memset(bind, 0, sizeof(bind));
				break;
			default:
				fprintf(stderr, "ERROR: unknown option %c\n", c);
				break;
			}
		}
	}

	setup_listeners(0);
	IF_TCONFIG_IPV6(if (ipv6_enabled() && nvram_get_int("http_ipv6")) setup_listeners(1)); /* Listen on IPv6 if enabled via GUI admin-access.asp; Check IPv6 first! */

	if (listeners.count == 0) {
		logmsg(LOG_ERR, "can't bind to any address");
		return 1;
	}

	IF_TCONFIG_HTTPS(if (do_ssl) start_ssl());

	init_id();

#ifdef TTYD_PROXY
    if (ttyd_init() < 0) {
        logmsg(LOG_WARNING, "Failed to initialize WebSocket support");
    }
#endif /* TTYD_PROXY */
	if (daemon(1, 1) == -1) {
		logmsg(LOG_ERR, "daemon: %m");
		return 0;
	}

	if (!(pid_fp = fopen(pidfile, "w"))) {
		logerr(__FUNCTION__, __LINE__, pidfile);
		return errno;
	}
	fprintf(pid_fp, "%d", getpid());
	fclose(pid_fp);

	signal(SIGPIPE, SIG_IGN);
	signal(SIGALRM, SIG_IGN);
	signal(SIGHUP, SIG_IGN);
	signal(SIGCHLD, SIG_IGN);

	for (;;) {
		/* Do a select() on at least one and possibly many listen fds.
		 * If there's only one listen fd then we could skip the select
		 * and just do the (blocking) accept(), saving one system call;
		 * that's what happened up through version 1.18. However there
		 * is one slight drawback to that method: the blocking accept()
		 * is not interrupted by a signal call. Since we definitely want
		 * signals to interrupt a waiting server, we use select() even
		 * if there's only one fd.
		 */
		rfdset = listeners.lfdset;
		if (select(maxfd + 1, &rfdset, NULL, NULL, NULL) < 0) {
			if (errno != EINTR && errno != EAGAIN)
				sleep(1);

			continue;
		}

		for (i = listeners.count - 1; i >= 0; --i) {
			if (listeners.listener[i].listenfd < 0 || !FD_ISSET(listeners.listener[i].listenfd, &rfdset))
				continue;

			do_ssl = 0;
			sz = sizeof(sai);

			connfd = accept(listeners.listener[i].listenfd, (struct sockaddr *)&sai, &sz);
			if (connfd < 0) {
				continue;
			}

			if (!wait_action_idle(10)) {
				logmsg(LOG_WARNING, "router is busy");
				continue;
			}

			if (fork() == 0) {
				IF_TCONFIG_HTTPS(do_ssl = listeners.listener[i].ssl);
				close_listen_sockets();
				webcgi_init(NULL);

				clientsai = sai;
				if (!check_wlaccess())
					exit(0);

				struct timeval tv;
				tv.tv_sec = MAX_CONN_TIMEOUT;
				tv.tv_usec = 0;

				/* set receive/send timeouts */
				setsockopt(connfd, SOL_SOCKET, SO_SNDTIMEO, &tv, sizeof(tv));
				setsockopt(connfd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));

				/* set the KEEPALIVE option to cull dead connections */
				setsockopt(connfd, SOL_SOCKET, SO_KEEPALIVE, &int_1, sizeof(int_1));

				fcntl(connfd, F_SETFD, FD_CLOEXEC);

				if (web_open())
					handle_request();

				web_close();
				exit(0);
			}
			close(connfd);
		}
	}
#ifdef TTYD_PROXY
    ttyd_cleanup();
#endif /* TTYD_PROXY */
	close_listen_sockets();

	return 0;
}
