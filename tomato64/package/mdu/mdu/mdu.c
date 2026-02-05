/*
 *
 * MDU -- Mini DDNS Updater
 * Copyright (C) 2007-2009 Jonathan Zarate
 *
 * Licensed under GNU GPL v2 or later versions.
 * Fixes/updates (C) 2018 - 2026 pedro
 *
 */


#define _GNU_SOURCE

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <syslog.h>
#include <stdarg.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <sys/stat.h>
#include <ctype.h>

#include <bcmnvram.h>
#include <shutils.h>
#include <shared.h>

#ifdef USE_LIBCURL
 #include <curl/curl.h>
#else
 #include <netdb.h>
 #include "mssl.h"
#endif

#define VERSION			"2.2"
#define AGENT			"Mozilla/5.0 (X11; Linux x86_64; rv:10.0) Gecko/20100101 Firefox/109.0"
#define MAX_OPTION_LENGTH	256
#define BLOB_SIZE		(4 * 1024)
#define HALF_BLOB		(BLOB_SIZE >> 1)
#define QUARTER_BLOB		(BLOB_SIZE >> 2)
#define DDNS_IP_CACHE		5 * 60 /* 5-minute cache for IP extracted by a given checker */

#define M_UNKNOWN_ERROR__D	"Unknown error (%d)."
#define M_UNKNOWN_RESPONSE__D	"Unknown response (%d)."
#define M_INVALID_HOST		"Invalid hostname."
#define M_INVALID_AUTH		"Invalid authentication."
#define M_INVALID_PARAM__D	"Invalid parameter (%d)."
#define M_INVALID_PARAM__S	"Invalid parameter (%s)."
#define M_TOOSOON		"Update was too soon or too frequent."
#define M_ERROR_GET_IP		"Error obtaining IP address."
#define M_ERROR_MEM_STREAM	"Failed to open memory stream."
#define M_ERROR_MEM_ALLOC	"Memory allocation failed."
#define M_SAME_IP		"The IP address is the same."
#define M_SAME_RECORD		"Record already up-to-date."
#define M_DOWN			"Server temporarily down or under maintenance."

#define MDU_ROUTE_FN		"/tmp/mdu-route"

#ifdef USE_LIBCURL
 FILE *curl_dfile = NULL;
 CURL *curl_handle = NULL;
 struct curl_slist *headers = NULL;
 char errbuf[CURL_ERROR_SIZE];
 char curl_err_str[512];
#endif

/* needed by logmsg() */
#define LOGMSG_DISABLE		0
#define LOGMSG_NVDEBUG		"mdu_debug"


char *blob = NULL;
char ifname[16];
char sPrefix[8];
int error_exitcode = 1;
int g_argc;
char **g_argv;

char *f_argv[32];
int f_argc = -1;

static void save_cookie(void);
static void error(const char *fmt, ...);

/* this should be in nvram so you can add/edit/remove checkers, but we have so little nvram it's impossible... */
static char services[][2][23] = { /* remember: the number in the third square bracket must be (len + 1) of the longest string */
/*	  service			path */
	{ "api.ipify.org",		"/"	},	/* txt */
	{ "checkip.amazonaws.com",	"/"	},	/* txt */
	{ "checkip.dyndns.org",		"/"	},	/* "<html><head><title>Current IP Check</title></head><body>Current IP Address: 1.2.3.4</body></html>" */
	{ "ipecho.net",			"/plain"},	/* txt */
	{ "trackip.net",		"/ip"	},	/* txt */
	{ "ip.changeip.com",		"/"	},	/* "1.2.3.4\n<!--IPADDR=1.2.3.4-->" */
	{ "ifconfig.co",		"/ip"	},	/* txt */
	{ "ident.me",			"/"	},	/* txt */
	{ "eth0.me",			"/"	},	/* txt */
	{ "myexternalip.com",		"/raw"	},	/* txt */
	{ "ip.tyk.nu",			"/"	},	/* txt */
	{ "wgetip.com",			"/"	},	/* txt */
	{ "ifconfig.me",		"/ip"	},	/* txt */
	{ "icanhazip.com",		"/"	}	/* txt */
};

static void route_adddel(const char *ip, unsigned int add)
{
	char cmd[256];
	char buf[64];
	char buf2[128];

	if (ifname[0] != '\0' && nvram_get_int("mwan_num") > 1) { /* only for MultiWAN */
		logmsg(LOG_DEBUG, "*** IN %s: add=[%d] ip=[%s] ifname=[%s] - %s routes ...", __FUNCTION__, add, ip, ifname, (add ? "adding" : "deleting"));

		memset(buf, 0, sizeof(buf)); /* reset */
		strlcpy(buf, "/tmp/ppp/pppd", sizeof(buf));
		strlcat(buf, sPrefix, sizeof(buf));
		if (!f_exists(buf)) { /* not pppd */
			memset(buf, 0, sizeof(buf)); /* reset */
			memset(buf2, 0, sizeof(buf2)); /* reset */
			strlcpy(buf, sPrefix, sizeof(buf));
			strlcat(buf, "_gateway", sizeof(buf));
			snprintf(buf2, sizeof(buf2), "via %s", nvram_safe_get(buf)); /* gateway_fragment */
		}
		else
			buf2[0] = '\0';

		system("ip route | grep default | cut -d' ' -f2- > " MDU_ROUTE_FN);
		memset(buf, 0, sizeof(buf)); /* reset */
		if (f_read_string(MDU_ROUTE_FN, buf, sizeof(buf)) > 2) { /* default_route_fragment */
			memset(cmd, 0, sizeof(cmd)); /* reset */
			if ((size_t)snprintf(cmd, sizeof(cmd), "ip route %s %s %s", (add ? "add" : "del"), ip, buf) >= sizeof(cmd))
				logmsg(LOG_WARNING, "*** %s: route cmd truncated", __FUNCTION__);

			logmsg(LOG_DEBUG, "*** %s: cmd=%s", __FUNCTION__, cmd);
			system(cmd);
		}

		memset(cmd, 0, sizeof(cmd)); /* reset */
		if ((size_t)snprintf(cmd, sizeof(cmd), "ip route %s %s dev %s %s metric 50000", (add ? "add" : "del"), ip, ifname, buf2) >= sizeof(cmd))
			logmsg(LOG_WARNING, "*** %s: route cmd truncated", __FUNCTION__);

		logmsg(LOG_DEBUG, "*** %s: cmd=%s", __FUNCTION__, cmd);
		system(cmd);
	}
}

static int check_stop(void)
{
	char buf[8];

	return (f_read(MDU_STOP_FN, buf, sizeof(buf)) >  0); /* check if we have to stop */
}

static void trimamp(char *s)
{
	size_t n;

	n = strlen(s);
	if ((n > 0) && (s[--n] == '&'))
		s[n] = '\0';
}

static const char *get_option(const char *name)
{
	char *p, *key_end, *entry, *value;
	int i, n;
	size_t entry_len;
	FILE *f;
	const char *c;
	char s[384];

	if (f_argc < 0) {
		f_argc = 0;
		if ((c = get_option("conf")) != NULL) {
			if ((f = fopen(c, "r")) != NULL) {
				while (fgets(s, sizeof(s), f)) {
					p = s;

					/* trim leading whitespace */
					while (*p && isspace((unsigned char)*p))
						++p;

					if ((*p == '\0') || (*p == '#')) /* blank line or comment */
						continue;

					/* trim trailing whitespace */
					key_end = p + strlen(p) - 1;
					while (key_end > p && isspace((unsigned char)*key_end))
						--key_end;

					*(key_end + 1) = '\0';

					/* optional -- prefix */
					if (p[0] == '-' && p[1] == '-')
						p += 2;

					/* find the end of the key (first space/tab) */
					key_end = p;
					while (*key_end && !isspace((unsigned char)*key_end))
						++key_end;

					if (*key_end == '\0') {
						/* option with no value – e.g. "wildcard" */
						if ((p = strdup(p)) == NULL) {
							fclose(f);
							error(M_ERROR_MEM_ALLOC);
						}
						f_argv[f_argc++] = p;
						if ((unsigned int)f_argc >= ASIZE(f_argv))
							break;

						continue;
					}

					*key_end = '\0';
					value = key_end + 1;
					while (*value && isspace((unsigned char)*value))
						++value;

					/* save the entire "key value" as one string */
					entry_len = strlen(p) + 1 + strlen(value) + 1;
					entry = malloc(entry_len);
					if (!entry) {
						fclose(f);
						error(M_ERROR_MEM_ALLOC);
					}
					snprintf(entry, entry_len, "%s %s", p, value);

					f_argv[f_argc++] = entry;
					if ((unsigned int)f_argc >= ASIZE(f_argv))
						break;
				}
				fclose(f);
			}
		}
	}

	n = strlen(name);
	for (i = 0; i < f_argc; ++i) {
		c = f_argv[i];
		if (strncmp(c, name, n) == 0 && ((c[n] == ' ') || (c[n] == '\0'))) {
			return c + n + (c[n] == ' ' ? 1 : 0); /* skip spaces if present */
		}
	}

	/* fallback to command line */
	for (i = 0; i < g_argc; ++i) {
		p = g_argv[i];
		if (p[0] == '-' && p[1] == '-' && strcmp(p + 2, name) == 0) {
			if ((++i >= g_argc) || (strlen(g_argv[i]) >= MAX_OPTION_LENGTH))
				break;

			return g_argv[i];
		}
	}

	return NULL;
}

static const char *get_option_required(const char *name)
{
	const char *p;

	if ((p = get_option(name)) != NULL)
		return p;

	logmsg(LOG_DEBUG, "required option --%s is missing", name);
	fprintf(stderr, "Required option --%s is missing.\n", name);

	exit(2);
}

static const char *get_option_or(const char *name, const char *alt)
{
	return get_option(name) ? : alt;
}

static int get_option_onoff(const char *name, int def)
{
	const char *p;

	if ((p = get_option(name)) == NULL)
		return def;
	if ((strcmp(p, "on") == 0) || (strcmp(p, "1") == 0))
		return 1;
	if ((strcmp(p, "off") == 0) || (strcmp(p, "0") == 0))
		return 0;

	logmsg(LOG_DEBUG, "--%s requires the value off/on or 0/1", name);
	fprintf(stderr, "--%s requires the value off/on or 0/1.\n", name);

	exit(2);
}

static void save_msg(const char *s)
{
	const char *path;

	if ((path = get_option("msg")) != NULL)
		f_write_string(path, s, FW_NEWLINE, 0);
}

static void error(const char *fmt, ...)
{
	va_list args;
	char s[128];

	va_start(args, fmt);
	vsnprintf(s, sizeof(s), fmt, args);
	s[sizeof(s) - 1] = 0;
	va_end(args);

	logmsg(LOG_ERR, "%s", s);
	printf("%s\n", s);
	save_msg(s);

	exit(error_exitcode);
}

static void success_msg(const char *msg, const unsigned int do_exit)
{
	save_cookie();

	logmsg(LOG_DEBUG, "*** %s: msg=[%s]", __FUNCTION__, msg);
	printf("%s\n", msg);
	save_msg(msg);

	if (do_exit == 1)
		exit(0);
}

static void success(void)
{
	success_msg("Update successful.", 1);
}

static const char *get_dump_name(void)
{
	return get_option("dump");
}

#ifdef USE_LIBCURL
static int curl_dump_cb(CURL *handle, curl_infotype type, char *data, size_t size, void *userptr)
{
	FILE *stream = (FILE *)userptr;
	const char *prefix;
	char *in, *out;
	unsigned char c;
	unsigned int is_info = 0;
	size_t i;
	struct tm *stm;
	time_t now;
	char t_buf[20];

	/* add timestamp */
	time(&now);
	stm = localtime(&now);
	memset(t_buf, 0, sizeof(t_buf));
	strftime(t_buf, sizeof(t_buf), "%b %d %H:%M:%S ", stm);

	switch (type) {
		case CURLINFO_HEADER_OUT:
			prefix = "=> H ";
			break;
		case CURLINFO_DATA_OUT:
			prefix = "=> D ";
			break;
		case CURLINFO_HEADER_IN:
			prefix = "<= H ";
			break;
		case CURLINFO_DATA_IN:
			prefix = "<= D ";
			break;
		case CURLINFO_TEXT:
			prefix = "== I ";
			is_info = 1;
			break;
		default:
			return 0;
	}

	/* pretty up a bit (remove carriage returns and process data in chunks) */
	for (in = out = data; in < data + size; ++in) {
		if (*in != '\r') {
			*out = *in;
			++out;
		}
	}
	*out = '\0'; /* null-terminate the modified string (for convenience) */

	/* adjust the size after removing '\r' */
	size = out - data; 

	if (data[size - 1] == '\n')
		size -= 1;
	if (is_info && data[size - 1] == ':')
		size -= 1;

	/* write timestamp and prefix to file */
	fputs(t_buf, stream);
	fputs(prefix, stream);

	/* loop over the data and print it */
	for (i = 0; i < size; ++i) {
		c = data[i];
		if (c == '\n') {
			fputc('\n', stream);
			fputs(t_buf, stream);
			fputs(prefix, stream);
		}
		else /* if character is printable (0x20 - 0x7E), print it, otherwise print '.' */
			fputc((c >= 0x20 && c < 0x80) ? c : '.', stream);
	}

	/* end with a newline */
	fputc('\n', stream);

	return 0;
}

static void curl_cleanup()
{
	if (curl_dfile)
		fclose(curl_dfile);

	curl_easy_cleanup(curl_handle);
	curl_global_cleanup();
}

static void curl_setup(const unsigned int ssl)
{
	CURLsslset result;
	const char *dump;
	unsigned int curl_sslerr = 1;

#ifdef USE_WOLFSSL
	result = curl_global_sslset(CURLSSLBACKEND_WOLFSSL, NULL, NULL);
#else
	result = curl_global_sslset(CURLSSLBACKEND_OPENSSL, NULL, NULL);
#endif
	if ((result == CURLSSLSET_OK) || (result == CURLSSLSET_TOO_LATE))
		curl_sslerr = 0;

	if (curl_global_init(CURL_GLOBAL_ALL) || !(curl_handle = curl_easy_init()))
		error("libcurl initialization failure.");

#ifndef TCONFIG_STUBBY
	curl_easy_setopt(curl_handle, CURLOPT_SSL_VERIFYPEER, 0L);
#endif
	curl_easy_setopt(curl_handle, CURLOPT_FOLLOWLOCATION, 1L);
	curl_easy_setopt(curl_handle, CURLOPT_MAXREDIRS, 3L);
	curl_easy_setopt(curl_handle, CURLOPT_CONNECTTIMEOUT, 5L);
	curl_easy_setopt(curl_handle, CURLOPT_TIMEOUT, 20L);
	curl_easy_setopt(curl_handle, CURLOPT_VERBOSE, 1L);
	curl_easy_setopt(curl_handle, CURLOPT_ERRORBUFFER, errbuf);

	if ((dump = get_dump_name()) != NULL) {
		if ((curl_dfile = fopen(dump, "a")) != NULL) {
			curl_easy_setopt(curl_handle, CURLOPT_DEBUGFUNCTION, curl_dump_cb);
			curl_easy_setopt(curl_handle, CURLOPT_DEBUGDATA, (void *)curl_dfile);
		}
	}

	if (ssl) {
		curl_easy_setopt(curl_handle, CURLOPT_DEFAULT_PROTOCOL, "https");
		if (curl_sslerr) {
			curl_cleanup();
			error("SSL failure with libcurl.");
		}
	}
	else
		curl_easy_setopt(curl_handle, CURLOPT_DEFAULT_PROTOCOL, "http");
}

static struct curl_slist *curl_headers(const char *header)
{
	char *sub = NULL;
	struct curl_slist *tmp = NULL;
	size_t n = strlen(header);
	headers = NULL;

	if (!header)
		return NULL;

	sub = strstr(header, "\r\n");
	while (sub || (n > 0)) {
		if (sub)
			sub = NULL;
		if (header) {
			tmp = curl_slist_append(headers, header);
			if (tmp == NULL) {
				curl_slist_free_all(headers);
				curl_cleanup();
				error("libcurl header failure.");
			}
		}
		if (sub) {
			n -= sub + 2 - header;
			headers = tmp;
			header = sub + 2;
			*sub = '\r';
			sub = strstr(header, "\r\n");
		}
		else {
			n = 0;
			headers = tmp;
		}
	}

	return headers;
}

static char *curl_resolve_ip(const unsigned int ssl, const char *url, const char *header)
{
	char *ip;
	CURLcode r;
	int trys, stop = 0;
	unsigned int ok = 0;
	headers = NULL;

	curl_setup(ssl);

	curl_easy_setopt(curl_handle, CURLOPT_URL, url);

	if (header)
		headers = curl_headers(header);
	else
		headers = curl_headers("User-Agent: " AGENT "\r\nCache-Control: no-cache");

	curl_easy_setopt(curl_handle, CURLOPT_HTTPHEADER, headers);

	if (ifname[0] != '\0')
		curl_easy_setopt(curl_handle, CURLOPT_INTERFACE, ifname);

	for (trys = 4; trys > 0; --trys) {
		errbuf[0] = 0;
		r = curl_easy_perform(curl_handle);
		stop = check_stop();
		if ((r != CURLE_COULDNT_CONNECT) || (stop == 1))
			break;

		sleep(2);
	}

	if (((r == CURLE_OK) || (r == CURLE_RECV_ERROR)) && !curl_easy_getinfo(curl_handle, CURLINFO_PRIMARY_IP, &ip) && ip) /* CURLE_RECV_ERROR needed for clouflare */
		ok = 1;
	else {
		memset(curl_err_str, 0, sizeof(curl_err_str));
		snprintf(curl_err_str, sizeof(curl_err_str), "libcurl error (%d) - %s.", r, (strlen(errbuf) ? errbuf : curl_easy_strerror(r)));
		logmsg(LOG_DEBUG, "*** %s: error - (%s)", __FUNCTION__, curl_err_str);
	}

	curl_slist_free_all(headers);
	curl_cleanup();

	if (stop == 1)
		error("Force stop.");

	logmsg(LOG_DEBUG, "*** %s: OUT IP=[%s]", __FUNCTION__, (ok ? ip : "unknown"));

	return ok ? ip : "0";
}
#endif /* USE_LIBCURL */

static long _http_req(const unsigned int ssl, int static_host, const char *host, const char *req, const char *query, const char *header, int auth, char *data, char **body)
{
	logmsg(LOG_DEBUG, "*** %s: IN host=[%s] query=[%s] ssl=[%d] header=[%s] auth=[%d] data=[%s] req=[%s] ifname=[%s]", __FUNCTION__, host, query, ssl, header, auth, data ? data : "NULL", req, ifname);

#ifdef USE_LIBCURL
	FILE *curl_wbuf = NULL;
	FILE *curl_rbuf = NULL;
	char url[HALF_BLOB];
	char ip[INET6_ADDRSTRLEN];
	char *ip_ret;
	CURLcode r;
	int trys;
	int stop = 0;
	long code = -1;
	headers = NULL;

	if (!static_host)
		host = get_option_or("server", host);

	/* build URL */
	memset(url, 0, HALF_BLOB); /* reset */
	if (snprintf(url, sizeof(url), "%s%s", host, query) >= (int)sizeof(url))
		logmsg(LOG_WARNING, "*** %s: URL truncated", __FUNCTION__);

	memset(ip, 0, sizeof(ip)); /* reset */
	/* resolve IP for routing if MultiWAN */
	if (ifname[0] != '\0') {
		logmsg(LOG_DEBUG, "*** %s: resolving IP of server %s ...", __FUNCTION__, host);
		ip_ret = curl_resolve_ip(ssl, url, header);
		if (strcmp(ip_ret, "0") != 0) {
			strlcpy(ip, ip_ret, INET6_ADDRSTRLEN); /* copy as it will be reused in the next request */
			logmsg(LOG_DEBUG, "*** %s: resolved IP=[%s]", __FUNCTION__, ip);
		}
		else
			return code; /* couldn't resolve */
	}

	/* memory stream for response body */
	curl_wbuf = fmemopen(blob, BLOB_SIZE, "w");
	if (!curl_wbuf) {
		logmsg(LOG_ERR, M_ERROR_MEM_STREAM);
		return -2;
	}
	setbuf(curl_wbuf, NULL); /* disable buffering */

	curl_setup(ssl);

	curl_easy_setopt(curl_handle, CURLOPT_URL, url);

	if (header)
		headers = curl_headers(header);
	else
		headers = curl_headers("User-Agent: " AGENT "\r\nCache-Control: no-cache");

	curl_easy_setopt(curl_handle, CURLOPT_HTTPHEADER, headers);

	if (ifname[0] != '\0')
		curl_easy_setopt(curl_handle, CURLOPT_INTERFACE, ifname);

	if (auth) {
		curl_easy_setopt(curl_handle, CURLOPT_USERNAME, get_option_required("user"));
		curl_easy_setopt(curl_handle, CURLOPT_PASSWORD, get_option_required("pass"));
		curl_easy_setopt(curl_handle, CURLOPT_HTTPAUTH, CURLAUTH_BASIC);
	}
	else
		curl_easy_setopt(curl_handle, CURLOPT_HTTPAUTH, CURLAUTH_NONE);

	curl_easy_setopt(curl_handle, CURLOPT_WRITEDATA, (void *)curl_wbuf);

	if (data) { /* only cloudflare (for now) */
		curl_rbuf = fmemopen(data, strlen(data), "r");
		if (curl_rbuf) {
			curl_easy_setopt(curl_handle, CURLOPT_READDATA, (void *)curl_rbuf);
			curl_easy_setopt(curl_handle, CURLOPT_INFILESIZE, (long)strlen(data));
			curl_easy_setopt(curl_handle, CURLOPT_UPLOAD, 1L);
		}
	}
	else {
		curl_easy_setopt(curl_handle, CURLOPT_READDATA, NULL);
		curl_easy_setopt(curl_handle, CURLOPT_INFILESIZE, 0L);
		curl_easy_setopt(curl_handle, CURLOPT_UPLOAD, 0L);
	}

	if (!strcmp(req, "POST"))
		curl_easy_setopt(curl_handle, CURLOPT_POST, 1L);
	else if (!strcmp(req, "GET"))
		curl_easy_setopt(curl_handle, CURLOPT_HTTPGET, 1L);
	else if (!strcmp(req, "PUT"))
		curl_easy_setopt(curl_handle, CURLOPT_UPLOAD, 1L);

	/* add route */
	if (ip[0] != '\0')
		route_adddel(ip, 1);

	for (trys = 4; trys > 0; --trys) {
		errbuf[0] = '\0';
		r = curl_easy_perform(curl_handle);
		stop = check_stop();
		if ((r != CURLE_COULDNT_CONNECT) || (stop))
			break;

		sleep(2);
	}

	/* del route */
	if (ip[0] != '\0')
		route_adddel(ip, 0);

	curl_easy_getinfo(curl_handle, CURLINFO_RESPONSE_CODE, &code);

	logmsg(LOG_DEBUG, "*** %s: response code=%ld", __FUNCTION__, code);

	if ((r == CURLE_OK) || (r == CURLE_RECV_ERROR)) { /* CURLE_RECV_ERROR needed for cloudflare */
		*body = blob;
		/* body is pointer into global blob - caller must NOT free it */
	}
	else {
		memset(curl_err_str, 0, sizeof(curl_err_str));
		snprintf(curl_err_str, sizeof(curl_err_str), "libcurl error (%d) - %s.", r, errbuf[0] ? errbuf : curl_easy_strerror(r));
		logmsg(LOG_DEBUG, "*** %s: error - %s", __FUNCTION__, curl_err_str);
	}

	fclose(curl_wbuf);
	if (curl_rbuf)
		fclose(curl_rbuf);

	if (curl_dfile) {
		fputc('\n', curl_dfile);
		fflush(curl_dfile);
	}

	curl_slist_free_all(headers);
	curl_cleanup();

	if (stop)
		error("Force stop.");

	return code;
#else /* !USE_LIBCURL */
	FILE *f = NULL;
	char addrstr[INET6_ADDRSTRLEN];
	char *request = NULL;
	char *httpv, *colon, *body_start;
	int port;
	char a[512], b[512], authbuf[512];
	const char *c_ip, *c;
	long i;
	struct addrinfo hints;
	struct addrinfo *result, *rp;
	struct timeval tv;
	struct tm *stm;
	time_t now;
	char cport[12], clen[32], buf[20];
	unsigned int trys;
	int sockfd = -1;
	int stop = 0;

	httpv = (strncmp(host, "updates.opendns.com", 19) == 0 || strncmp(host, "api.cloudflare.com", 18) == 0) ? "HTTP/1.1" : "HTTP/1.0";

	if (!static_host)
		host = get_option_or("server", host);

	/* build request header */
	strlcpy(a, req, sizeof(a));
	strlcat(a, " ", sizeof(a));
	strlcat(a, query, sizeof(a));
	strlcat(a, " ", sizeof(a));
	strlcat(a, httpv, sizeof(a));
	strlcat(a, "\r\nHost: ", sizeof(a));
	strlcat(a, host, sizeof(a));
	strlcat(a, "\r\n", sizeof(a));

	if (!header)
		strlcat(a, "User-Agent: " AGENT "\r\nCache-Control: no-cache\r\n", sizeof(a));

	if (auth) {
		snprintf(authbuf, sizeof(authbuf), "%s:%s", get_option_required("user"), get_option_required("pass"));
		i = base64_encode(authbuf, b, strlen(authbuf));
		b[i] = '\0';
		strlcat(a, "Authorization: Basic ", sizeof(a));
		strlcat(a, b, sizeof(a));
		strlcat(a, "\r\n", sizeof(a));
	}

	if (header) {
		strlcat(a, header, sizeof(a));
		if (header[strlen(header)-1] != '\n')
			strlcat(a, "\r\n", sizeof(a));
	}

	if (data) {
		snprintf(clen, sizeof(clen), "Content-Length: %zu\r\n", strlen(data));
		strlcat(a, clen, sizeof(a));
	}

	strlcat(a, "\r\n", sizeof(a));
	if (data)
		strlcat(a, data, sizeof(a));

	if (snprintf(blob, BLOB_SIZE, "%s", a) >= BLOB_SIZE)
		logmsg(LOG_WARNING, "*** %s: request header truncated", __FUNCTION__);

	/* duplicate for sending */
	request = strdup(blob);
	if (!request) {
		logmsg(LOG_DEBUG, "*** %s: strdup failed", __FUNCTION__);
		return -1;
	}

	/* parse port */
	port = ssl ? 443 : 80;
	strlcpy(a, host, sizeof(a));
	if ((colon = strrchr(a, ':'))) {
		*colon = '\0';
		port = atoi(colon + 1);
	}

	memset(&hints, 0, sizeof(hints));
#ifdef TCONFIG_IPV6
	hints.ai_family = AF_UNSPEC; /* allow IPv4 or IPv6 */
#else
	hints.ai_family = AF_INET;
#endif
	hints.ai_socktype = SOCK_STREAM;

	memset(cport, 0, sizeof(cport));
	snprintf(cport, sizeof(cport), "%d", port);

	for (trys = 4; trys > 0; --trys) {
		logmsg(LOG_DEBUG, "*** %s: attempt %d", __FUNCTION__, 5 - trys);

		if (getaddrinfo(a, cport, &hints, &result) != 0) {
			sleep(2);
			continue;
		}

		for (rp = result; rp != NULL; rp = rp->ai_next) {
			sockfd = socket(rp->ai_family, rp->ai_socktype, rp->ai_protocol);
			if (sockfd == -1)
				continue;

			if (ifname[0] != '\0') {
				struct ifreq ifr;
				memset(&ifr, 0, sizeof(ifr));
				strlcpy(ifr.ifr_name, ifname, sizeof(ifr.ifr_name));
				if (setsockopt(sockfd, SOL_SOCKET, SO_BINDTODEVICE, &ifr, sizeof(ifr)) < 0) {
					logmsg(LOG_DEBUG, "*** %s: bind to device %s failed", __FUNCTION__, ifname);
					close(sockfd);
					continue;
				}
			}

			/* add route */
#ifdef TCONFIG_IPV6
			c_ip = inet_ntop(rp->ai_family,
			                 rp->ai_family == AF_INET ?
			                 (void *)&((struct sockaddr_in *)rp->ai_addr)->sin_addr :
			                 (void *)&((struct sockaddr_in6 *)rp->ai_addr)->sin6_addr,
			                 addrstr, sizeof(addrstr));
#else
			c_ip = inet_ntop(rp->ai_family, &(((struct sockaddr_in *)rp->ai_addr)->sin_addr),
			                 addrstr, sizeof(addrstr));
#endif
			if (c_ip)
				route_adddel(c_ip, 1);

			logmsg(LOG_DEBUG, "*** %s: [%s][%s] - connecting ...", __FUNCTION__, c_ip, cport);

			if (connect_timeout(sockfd, rp->ai_addr, rp->ai_addrlen, 10) != -1) {
				logmsg(LOG_DEBUG, "*** %s: connected!", __FUNCTION__);
				/* del route */
				if (c_ip)
					route_adddel(c_ip, 0);

				freeaddrinfo(result);
				goto connected;
			}

			/* del route */
			if (c_ip)
				route_adddel(c_ip, 0);

			close(sockfd);
			sockfd = -1;

			stop = check_stop();
			if (stop)
				break;
		}

		freeaddrinfo(result);
		if (stop)
			break;

		sleep(2);
	}

	free(request);
	if (stop)
		error("Force stop.");

connected:
	logmsg(LOG_DEBUG, "*** %s: connected:", __FUNCTION__);

	stop = check_stop();
	if (stop) {
		close(sockfd);
		free(request);
		error("Force stop.");
	}

	/* timeouts */
	tv.tv_sec = 10;
	tv.tv_usec = 0;
	setsockopt(sockfd, SOL_SOCKET, SO_SNDTIMEO, &tv, sizeof(tv));
	setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));

	/* SSL or plain */
	if (ssl) {
		mssl_init(NULL, NULL);
		f = ssl_client_fopen_name(sockfd, a);
	}
	else
		f = fdopen(sockfd, "r+");

	if (!f) {
		logerr(__FUNCTION__, __LINE__, "error opening");
		close(sockfd);
		free(request);
		return -1;
	}

	/* send request */
	i = strlen(request);
	if (fwrite(request, 1, i, f) != (size_t)i) {
		logerr(__FUNCTION__, __LINE__, "error writing");
		fclose(f);
		free(request);
		return -1;
	}
	logmsg(LOG_DEBUG, "*** %s: sent request", __FUNCTION__);

	/* read response */
	i = fread(blob, 1, BLOB_SIZE - 1, f);
	blob[i >= 0 ? i : 0] = '\0'; /* null-terminate the string */

	fclose(f);
	close(sockfd);
	free(request);

	/* make dump */
	if ((c = get_dump_name()) != NULL) {
		if ((f = fopen(c, "a")) != NULL) {
			time(&now);
			stm = localtime(&now);
			memset(buf, 0, sizeof(buf));
			strftime(buf, sizeof(buf), "%b %d %H:%M:%S", stm);

			fprintf(f, "[%s -> %s:%d]\nREQUEST =>\n", buf, a, port);
			fputs(request, f);
			fputs("REPLY <=\n", f);
			fputs(blob, f);
			fputs("\nEND\n\n", f);
			fclose(f);
		}
	}

	/* parse response code and body */
	i = -1;
	body_start = NULL;
	if (sscanf(blob, "HTTP/1.%*d %ld", &i) == 1 && i >= 100 && i <= 999) {
		if ((body_start = strstr(blob, "\r\n\r\n")) != NULL)
			body_start += 4;
		else if ((body_start = strstr(blob, "\n\n")) != NULL)
			body_start += 2;
	}

	if (body_start && body)
		*body = body_start;

	return i;
#endif /* USE_LIBCURL */
}

static long http_req(const unsigned int ssl, int static_host, const char *host, const char *get, const char *header, int auth, char **body)
{
	return _http_req(ssl, static_host, host, "GET", get, header, auth, NULL, body);
}

static int read_tmaddr(const char *name, long *tm, char *addr)
{
	char s[192];

	logmsg(LOG_DEBUG, "*** %s: IN cachename: %s", __FUNCTION__, name);

	memset(s, 0, sizeof(s)); /* reset */
	if (f_read_string(name, s, sizeof(s)) > 0) {
		if (sscanf(s, "%ld,%63s", tm, addr) == 2) {
			logmsg(LOG_DEBUG, "*** %s: tm=%ld addr=%s", __FUNCTION__, *tm, addr);
			if (*tm > 0 && (inet_pton(AF_INET, addr, &(struct in_addr){0}) == 1 ||
#ifdef TCONFIG_IPV6
			                inet_pton(AF_INET6, addr, &(struct in6_addr){0}) == 1))
#else
			                0))
#endif
				return 1;
		}
		else
			logmsg(LOG_DEBUG, "*** %s: unknown=%s", __FUNCTION__, s);
	}

	return 0;
}

static const char *get_address(int required)
{
	char *body;
	struct in_addr ipv4;
#ifdef TCONFIG_IPV6
	struct in6_addr ipv6;
#endif
	const char *c;
	const void *src;
	char *p, *end;
	char s[192];
	char cache_name[128];
	char normalized[64];
	static char addr[64];
	long ut, et, af, expire;
	int rows, service_num, n, i, j, temp, max_tries;
	int indices[ASIZE(services)];
	size_t len;

	/* addr is present in the config */
	if ((c = get_option("addr")) != NULL) {
		/* do not use custom IP address, run IP checker */
		if (*c == '@') {
			ut = get_uptime();

			strlcpy(cache_name, get_option_required("addrcache"), sizeof(cache_name));

			if (read_tmaddr(cache_name, &et, addr)) {
				if ((et > ut) && ((et - ut) <= DDNS_IP_CACHE)) {
					logmsg(LOG_DEBUG, "*** %s: OUT using cached address %s from %s (expires in %ld s)", __FUNCTION__, addr, cache_name, (et - ut));
					return addr;
				}
			}

			rows = ASIZE(services);

			/* Fisher-Yates shuffle */
			for (i = 0; i < rows; i++) indices[i] = i;
			srand(time(NULL) ^ (unsigned int)ut);
			for (i = rows - 1; i > 0; i--) {
				j = rand() % (i + 1);
				temp = indices[i];
				indices[i] = indices[j];
				indices[j] = temp;
			}

			max_tries = (rows < 5) ? rows : 5; /* try 5 times on different checkers, if no response it means (probably) WAN is down - wait */

			for (n = 0; n < max_tries; n++) {
				service_num = indices[n];

				body = NULL;
				if (http_req(0, 1, services[service_num][0], services[service_num][1], NULL, 0, &body) == 200 && body) { /* do not use ssl */
					/* body points to global blob - no free needed */
					if ((p = strstr(body, "Address:")) != NULL) /* dyndns */
						p += 8;
					else /* the rest */
						p = body;

					while (*p && isspace((unsigned char)*p))
						++p;

					if (*p == '\0')
						continue;

					end = p + strcspn(p, " \t\r\n");
					len = end - p;

					if ((len == 0) || (len >= sizeof(addr))) {
						logmsg(LOG_DEBUG, "*** %s: invalid length from %s", __FUNCTION__, services[service_num][0]);
						continue;
					}

					/* copy with null-termination */
					memcpy(addr, p, len);
					addr[len] = '\0';

					/* strip square brackets for IPv6 if present */
					if (addr[0] == '[') {
						memmove(addr, addr + 1, len);
						len--;
						addr[len] = '\0';
					}
					if (len > 0 && addr[len - 1] == ']')
						addr[len - 1] = '\0';

					/* validate and normalize */
					af = 0;
					src = NULL;

					if (inet_pton(AF_INET, addr, &ipv4) == 1) {
						af = AF_INET;
						src = &ipv4;
					}
#ifdef TCONFIG_IPV6
					else if (inet_pton(AF_INET6, addr, &ipv6) == 1) {
						af = AF_INET6;
						src = &ipv6;
					}
#endif
					/* write to cache if addr is OK */
					memset(normalized, 0, sizeof(normalized));
					if (af != 0 && inet_ntop(af, src, normalized, sizeof(normalized))) {
						expire = ut + DDNS_IP_CACHE;
						snprintf(s, sizeof(s), "%ld,%s", expire, normalized);
						f_write_string(cache_name, s, 0, 0);

						strlcpy(addr, normalized, sizeof(addr));

						logmsg(LOG_DEBUG, "*** %s: detected %s via %s, cached as %s until %ld", __FUNCTION__, normalized, services[service_num][0], s, expire);

						success_msg("Update successful.", 0); /* do not exit! */

						return addr;
					}

					logmsg(LOG_DEBUG, "*** %s: invalid address format from %s: %s", __FUNCTION__, services[service_num][0], addr);
				}
			}

			/* all attempts failed */
#ifdef USE_LIBCURL
			logmsg(LOG_DEBUG, "*** %s: %s (%s) after %d attempts", __FUNCTION__, curl_err_str, services[service_num][0], n);
			error(curl_err_str);
#else
			logmsg(LOG_DEBUG, "*** %s: " M_ERROR_GET_IP " (%s) after %d attempts", __FUNCTION__, services[service_num][0], n);
			error(M_ERROR_GET_IP);
#endif
		}
		return c;
	}

	return required ? get_option_required("addr") : NULL;
}

#ifdef TCONFIG_IPV6
static int get_address6(char *buf, const size_t buf_sz)
{
	const char *lanif;
	int n, ret = 0;

	memset(buf, 0, buf_sz); /* reset */

	for (n = 1; n < 5; n++) {
		lanif = getifaddr(nvram_safe_get("lan_ifname"), AF_INET6, 0); /* get global address */

		if (lanif != NULL) {
			strlcpy(buf, lanif, buf_sz);
			ret = 1;
			logmsg(LOG_DEBUG, "*** %s: - valid global IPv6 address %s after %d secs ...", __FUNCTION__, lanif, (n - 1) * (n - 1));
			break; /* All OK and break here */
		}

		logmsg(LOG_DEBUG, "*** %s: - no global IPv6 address yet, retrying in %d secs ...", __FUNCTION__, n * n);
		sleep(n * n); /* try up to 30 sec */
	}

	return ret;
}
#endif /* TCONFIG_IPV6 */

static void append_addr_option(char *buffer, const char *format)
{
	const char *c;

	if ((c = get_address(0)) != NULL)
		snprintf(buffer + strlen(buffer), sizeof(buffer) - strlen(buffer), format, c);
}

/*
	DNS Update API
	http://www.dyndns.com/developers/specs/

	---

	DynDNS:
		http: 80, 8245
		https: 443

	http://test:test@members.dyndns.org/nic/update?system=dyndns&hostname=test.shacknet.nu

	GET /nic/update?
	    system=statdns&
	    hostname=yourhost.ourdomain.ext,yourhost2.dyndns.org&
	    myip=ipaddress&
	    wildcard=OFF&
	    mx=mail.exchanger.ext&
	    backmx=NO&
	    offline=NO
	    HTTP/1.0
	Host: members.dyndns.org
	Authorization: Basic username:pass
	User-Agent: Company - Device - Version Number
*/
static void update_dua(const char *type, const unsigned int ssl, const char *server, const char *path, int reqhost)
{
	const char *p;
	char query[2048];
	long r;
	char *body;

	/* +opt */
	memset(query, 0, sizeof(query));
	snprintf(query, sizeof(query), "%s?", path ? path : get_option_required("path"));

	/* +opt */
	if (type)
		snprintf(query + strlen(query), sizeof(query) - strlen(query), "system=%s&", type);

	/* +opt */
	p = reqhost ? get_option_required("host") : get_option("host");
	if (p)
		snprintf(query + strlen(query), sizeof(query) - strlen(query), "hostname=%s&", p);

	/* +opt */
	if (((p = get_option("mx")) != NULL) && (*p))
		snprintf(query + strlen(query), sizeof(query) - strlen(query), "mx=%s&backmx=%s&", p, (get_option_onoff("backmx", 0)) ? "YES" : "NO");

	/* +opt */
	append_addr_option(query, "myip=%s&");

	if (get_option_onoff("wildcard", 0))
		strlcat(query, "wildcard=ON", sizeof(query));

	trimamp(query);

	r = http_req(ssl, 0, server ? server : get_option_required("server"), query, NULL, 1, &body);
	switch (r) {
	case 200:
		if ((strstr(body, "dnserr")) || (strstr(body, "911"))) {
			error_exitcode = 2;
			error(M_DOWN);
		}

		if ((strstr(body, "badsys")) || (strstr(body, "numhost")) || (strstr(body, "ILLEGAL")))
			error(M_INVALID_PARAM__D, -1);

		if (strstr(body, "badagent"))
			error(M_INVALID_PARAM__D, -2);

		if ((strstr(body, "badauth")) || (strstr(body, "NOACCESS")) || (strstr(body, "!donator")))
			error(M_INVALID_AUTH);

		if ((strstr(body, "notfqdn")) || (strstr(body, "!yours")) || strstr(body, "nohost") || (strstr(body, "abuse")) || strstr(body, "NOSERVICE"))
			error(M_INVALID_HOST);

		if (strstr(body, "TOOSOON"))
			error(M_TOOSOON);

		if ((strstr(body, "nochg")) || (strstr(body, "good")) || (strstr(body, "NOERROR"))) {
			success();
			return;
		}

		error(M_UNKNOWN_RESPONSE__D, -1);
		break;
	case 401:
	case 403:
		error(M_INVALID_AUTH);
		break;
	}
	error(M_UNKNOWN_ERROR__D, r);
}

/*
	namecheap.com
	http://namecheap.simplekb.com/kb.aspx?show=article&articleid=27&categoryid=22

	---

http://dynamicdns.park-your-domain.com/update?host=host_name&domain=domain.com&password=domain_password[&ip=your_ip]

good:
"HTTP/1.1 200 OK
...

<?xml version="1.0"?>
<interface-response>
<IP>12.123.123.12</IP>
<Command>SETDNSHOST</Command>
<Language>eng</Language>
<ErrCount>0</ErrCount>
<ResponseCount>0</ResponseCount>
<MinPeriod>1</MinPeriod>
<MaxPeriod>10</MaxPeriod>
<Server>Reseller9</Server>
<Site>Namecheap</Site>
<IsLockable>True</IsLockable>
<IsRealTimeTLD>True</IsRealTimeTLD>
<TimeDifference>+03.00</TimeDifference>
<ExecTime>0.0625</ExecTime>
<Done>true</Done>
<debug><![CDATA[]]></debug>
</interface-response>"


bad:
"HTTP/1.1 200 OK
...

<?xml version="1.0"?>
<interface-response>
<Command>SETDNSHOST</Command>
<Language>eng</Language>
<ErrCount>1</ErrCount>
<errors>
<Err1>Passwords do not match</Err1>
</errors>
<ResponseCount>1</ResponseCount>
<responses>
<response>
<ResponseNumber>304156</ResponseNumber>
<ResponseString>Validation error; invalid ; password</ResponseString>
</response>
</responses>
<MinPeriod>1</MinPeriod>
<MaxPeriod>10</MaxPeriod>
<Server>Reseller1</Server>
<Site></Site>
<IsLockable>True</IsLockable>
<IsRealTimeTLD>True</IsRealTimeTLD>
<TimeDifference>+03.00</TimeDifference>
8<ExecTime>0.0625</ExecTime>
<Done>true</Done>
<debug><![CDATA[]]></debug>
</interface-response>"
*/
static void update_namecheap(const unsigned int ssl)
{
	long r;
	char *p;
	char *q;
	char *body;
	char query[2048];

	/* +opt +opt +opt */
	memset(query, 0, sizeof(query));
	snprintf(query, sizeof(query), "/update?host=%s&domain=%s&password=%s", get_option_required("host"), get_option("user") ? : get_option_required("domain"), get_option_required("pass"));

	/* +opt */
	append_addr_option(query, "&ip=%s");

	r = http_req(ssl, 0, "dynamicdns.park-your-domain.com", query, NULL, 0, &body);
	if (r == 200) {
		if (strstr(body, "<ErrCount>0<"))
			success();

		if ((p = strstr(body, "<Err1>")) != NULL) {
			p += 6;
			if ((q = strstr(p, "</")) != NULL) {
				*q = 0;
				if ((q - p) >= 64)
					p[64] = 0;

				error("%s", p);
			}
		}
		error(M_UNKNOWN_RESPONSE__D, -1);
	}

	error(M_UNKNOWN_ERROR__D, r);
}

/*
	eNom
	http://www.enom.com/help/faq_dynamicdns.asp

	---

good:
;URL Interface<br>
;Machine is Reseller5<br>
IP=127.0.0.1
Command=SETDNSHOST
Language=eng

ErrCount=0
ResponseCount=0
MinPeriod=1
MaxPeriod=10
Server=Reseller5
Site=eNom
IsLockable=True
IsRealTimeTLD=True
TimeDifference=+08.00
ExecTime=0.500

bad:
;URL Interface<br>
;Machine is Reseller4<br>
-Command=SETDNSHOST
Language=eng
ErrCount=1
Err1=Passwords do not match
ResponseCount=1
ResponseNumber1=304156
ResponseString1=Validation error; invalid ; password
MinPeriod=1
MaxPeriod=10
Server=Reseller4
Site=
IsLockable=True
IsRealTimeTLD=True
TimeDifference=+08.00
ExecTime=0.235
Done=true
*/
static void update_enom(const unsigned int ssl)
{
	long r;
	char *p;
	char *q;
	char *body;
	char query[2048];

	/* http://dynamic.name-services.com/interface.asp?Command=SetDNSHost&HostName=test&Zone=test.com&Address=1.2.3.4&DomainPassword=password */

	/* +opt +opt +opt */
	memset(query, 0, sizeof(query));
	snprintf(query, sizeof(query), "/interface.asp?Command=SetDNSHost&HostName=%s&Zone=%s&DomainPassword=%s", get_option_required("host"), get_option("user") ? : get_option_required("domain"), get_option_required("pass"));

	/* +opt */
	append_addr_option(query, "&Address=%s");

	r = http_req(ssl, 0, "dynamic.name-services.com", query, NULL, 0, &body);
	if (r == 200) {
		if (strstr(body, "ErrCount=0"))
			success();

		if ((p = strstr(body, "Err1=")) != NULL) {
			p += 5;
			if ((q = strchr(p, '\n')) != NULL) {
				*q = 0;
				if ((q - p) >= 64)
					p[64] = 0;
				if ((q = strchr(p, '\r')) != NULL)
					*q = 0;

				error("%s", p);
			}
		}
		error(M_UNKNOWN_RESPONSE__D, -1);
	}

	error(M_UNKNOWN_ERROR__D, r);
}

/*
	dnsExit
	http://www.dnsexit.com/Direct.sv?cmd=ipClients

	---

"HTTP/1.1 200 OK
...

 HTTP/1.1 200 OK
0=Success"

" HTTP/1.1 200 OK
11=fail to find foo.bar.com"

" HTTP/1.1 200 OK
4=Update too often. Please wait at least 8 minutes since the last update"

" HTTP/1.1 200 OK" <-- extra in body?
*/
static void update_dnsexit(const unsigned int ssl)
{
	long r;
	char *body;
	char query[2048];

	/* +opt +opt +opt */
	memset(query, 0, sizeof(query));
	snprintf(query, sizeof(query), "/RemoteUpdate.sv?login=%s&password=%s&host=%s", get_option_required("user"), get_option_required("pass"), get_option_required("host"));

	/* +opt */
	append_addr_option(query, "&myip=%s");

	r = http_req(ssl, 0, "update.dnsexit.com", query, NULL, 0, &body);
	if (r == 200) { /* (\d+)=.+ */
		if ((strstr(body, "0=Success")) || (strstr(body, "1=IP")))
			success();

		if ((strstr(body, "2=Invalid")) || (strstr(body, "3=User")))
			error(M_INVALID_AUTH);

		if ((strstr(body, "10=Host")) || (strstr(body, "11=fail")))
			error(M_INVALID_HOST);

		if (strstr(body, "4=Update"))
			error(M_TOOSOON);

		error(M_UNKNOWN_RESPONSE__D, -1);
	}

	error(M_UNKNOWN_ERROR__D, r);
}

/*
	ZoneEdit
	http://www.zoneedit.com/doc/dynamic.html

	---

"HTTP/1.1 200 OK
...
<SUCCESS CODE="200" TEXT="Update succeeded." ZONE="test123.com" HOST="www.test123.com" IP="1.2.3.4">"

"<ERROR CODE="707" TEXT="Duplicate updates for the same host/ip, adjust client settings" ZONE="testexamplesite4321.com" HOST="test.testexamplesite4321.com">"

"HTTP/1.1 401 Authorization Required
...
<title>Authentication Failed </title>"

ERROR CODE="[701-799]" TEXT="Description of the error" ZONE="Zone that Failed"
ERROR CODE="702" TEXT="Update failed." ZONE="%zone%"
ERROR CODE="703" TEXT="one of either parameters 'zones' or 'host' are required."
ERROR CODE="705" TEXT="Zone cannot be empty" ZONE="%zone%"
ERROR CODE="707" TEXT="Duplicate updates for the same host/ip, adjust client settings" ZONE="%zone%"
ERROR CODE="707" TEXT="Too frequent updates for the same host, adjust client settings" ZONE="%zone%"
ERROR CODE="704" TEXT="Zone must be a valid 'dotted' internet name." ZONE="%zone%"
ERROR CODE="701" TEXT="Zone is not set up in this account." ZONE="%zone%"
ERROR CODE="708" TEXT="Login/authorization error"
SUCCESS CODE="[200-201]" TEXT="Description of the success" ZONE="Zone that Succeeded"
SUCCESS CODE="200" TEXT="Update succeeded." ZONE="%zone%" IP="%dnsto%"
SUCCESS CODE="201" TEXT="No records need updating." ZONE="%zone%"
*/
static void update_zoneedit(const unsigned int ssl)
{
	long r;
	char *body;
	char *c;
	char query[2048];

	/* +opt */
	memset(query, 0, sizeof(query));
	snprintf(query, sizeof(query), "/auth/dynamic.html?host=%s", get_option_required("host"));

	/* +opt */
	append_addr_option(query, "&dnsto=%s");

	r = http_req(ssl, 0, "dynamic.zoneedit.com", query, NULL, 1, &body);
	switch (r) {
	case 200:
		if (strstr(body, "<SUCCESS CODE"))
			success();

		if ((c = strstr(body, "<ERROR CODE=\"")) != NULL) {
			r = atoi(c + 13);
			switch (r) {
			case 701: /* invalid "zone" */
				error(M_INVALID_HOST);
				break;
			case 702:
				error(M_TOOSOON);
				break;
			case 707: /* update is the same ip address? / too frequent updates */
				if (strstr(c, "Duplicate"))
					success();
				else
					error(M_TOOSOON);
				break;
			case 708: /* authorization error */
				error(M_INVALID_AUTH);
				break;
			case 709:
				error(M_INVALID_HOST);
				break;
			}
			error(M_UNKNOWN_RESPONSE__D, r);
		}
		error(M_UNKNOWN_RESPONSE__D, -1);
		break;
	case 401:
		error(M_INVALID_AUTH);
		break;
	}

	error(M_UNKNOWN_ERROR__D, r);
}

/*
	FreeDNS.afraid.org

	---

https://freedns.afraid.org/dynamic/update.php?XXXXXXXXXXYYYYYYYYYYYZZZZZZZ1111222&address=127.0.0.1

good:
	+"Updated foobar.mooo.com to 127.0.0.1 in 0.326 seconds"
	-"ERROR: Address 127.0.0.1 has not changed."
bad:
	-"ERROR: "800.0.0.1" is an invalid IP address."
	-"ERROR: Unable to locate this record (changed password recently? deleted and re-created this dns entry?)"
	-"ERROR: Invalid update URL (2)"
*/
static void update_afraid(const unsigned int ssl)
{
	long r;
	char *body;
	char query[2048];

	/* +opt */
	memset(query, 0, sizeof(query));
	snprintf(query, sizeof(query), "/dynamic/update.php?%s", get_option_required("ahash"));

	/* +opt */
	append_addr_option(query, "&address=%s");

	r = http_req(ssl, 0, "freedns.afraid.org", query, NULL, 0, &body);
	if (r == 200) {
		if ((strstr(body, "Updated")) && (strstr(body, "seconds")))
			success();
		else if ((strstr(body, "ERROR")) || (strstr(body, "fail"))) {
			if (strstr(body, "has not changed"))
				success_msg(M_SAME_RECORD, 1); /* update cookie */

			error(M_INVALID_AUTH);
		}
		else
			error(M_UNKNOWN_RESPONSE__D, -1);
	}

	error(M_UNKNOWN_ERROR__D, r);
}

/*
	cloudflare.com
	https://api.cloudflare.com/#dns-records-for-a-zone-update-dns-record

	---

PUT https://api.cloudflare.com/client/v4/zones/zone_id/dns_records/record_id
Headers: Authorization: Bearer API-token
         Content-Type: application/json
Data:    {"type":"A","name":"e.example.com","content":"198.51.100.4","proxied":false}

good:
"HTTP/1.1 200 OK
 {
  "success": true,
  "errors": [],
  "messages": [],
  "result": {
    "id": "372e67954025e0ba6aaa6d586b9e0b59",
    "type": "A",
    "name": "e.example.com",
    "content": "198.51.100.4",
    "proxiable": true,
    "proxied": false,
    "ttl": {},
    "locked": false,
    "zone_id": "023e105f4ecef8ad9ca31a8372d0c353",
    "zone_name": "example.com",
    "created_on": "2014-01-01T05:20:00.12345Z",
    "modified_on": "2014-01-01T05:20:00.12345Z",
    "data": {}
  }
}"

bad: HTTP non-200 response and JSON response has 'success:"false"'; 400 and 403
     mean bad or invalid auth and JSON response will include "code:6003" for 400
     and "code:9103" for 403
     HTTP 200 response but JSON response has "total_count:0", which means the
     search for the given hostname produced no results


records can be created via a POST request to the same URL, minus the record_id:
PUT https://api.cloudflare.com/client/v4/zones/zone_id/dns_records
and relevant reponses are similar.

record_id can be retrieved via
GET https://api.cloudflare.com/client/v4/zones/zone_id/dns_records?type=A&name=e.example.com
good:
"{
  "success": true,
  "errors": [],
  "messages": [],
  "result": [
    {
      "id": "372e67954025e0ba6aaa6d586b9e0b59",
      "type": "A",
      "name": "e.example.com",
      "content": "198.51.100.4",
      "proxiable": true,
      "proxied": false,
      "ttl": {},
      "locked": false,
      "zone_id": "zone_id",
      "zone_name": "example.com",
      "created_on": "2014-01-01T05:20:00.12345Z",
      "modified_on": "2014-01-01T05:20:00.12345Z",
      "data": {}
    }
  ]
}"

zone_id can be retrieved via
GET https://api.cloudflare.com/client/v4/zones?name=example.com&status=active
but this is unimplemented here.
*/
static char *remove_spaces(const char *body)
{
	int i, length = 0, j = 0;
	char *copy;

	/* first, count how many non-space characters there are */
	for (i = 0; body[i] != '\0'; i++) {
		if (body[i] != ' ')
			length++;
	}

	/* allocate memory for the new string (plus null terminator) */
	copy = (char *)malloc(length + 1);
	if (!copy)
		error("memory allocation failed");

	/* copy non-space characters to the new string */
	for (i = 0; body[i] != '\0'; i++) {
		if (body[i] != ' ')
			copy[j++] = body[i];
	}
	copy[j] = '\0'; /* null-terminate the new string */

	return copy;
}

static int cloudflare_errorcheck(const int code, const char *req, char *body)
{
	if (code == 200) {
		if (strstr(body, "\"success\":true") != NULL) {
			if (strstr(body, "\"total_count\":0") != NULL)
				return 1;

			return 0;
		}
		else {
			free(body);
			error(M_UNKNOWN_RESPONSE__D, -1);
		}
	}
	else if (code == 400 && strstr(body, "\"code\":6003") != NULL) {
		free(body);
		error(M_INVALID_AUTH);
	}
	else if (code == 403 && strstr(body, "\"code\":9103") != NULL) {
		free(body);
		error(M_INVALID_AUTH);
	}
	else if (code == 403 && strstr(body, "\"code\":10000") != NULL) {
		free(body);
		error(M_INVALID_AUTH);
	}

	free(body);
	error("%s returned HTTP error code %d.", req, code);

	return -1;
}

/* warning! doesn't work (in libcurl version) with dump enabled! */
static void update_cloudflare(const unsigned int ssl)
{
	char header[HALF_BLOB];
	const char *zone;
	const char *host;
	char query[QUARTER_BLOB];
	char *body = NULL;
	char *body_copy = NULL;
	long s;
	const char *addr;
	int prox, r, current_proxied;
	char *find;
	char *found;
	char data[QUARTER_BLOB];

	/* +opt */
	snprintf(header, HALF_BLOB, "User-Agent: " AGENT "\r\nAuthorization: Bearer %s\r\nContent-Type: application/json\r\nCache-Control: no-cache", get_option_required("pass"));

	zone = get_option_required("url");
	host = get_option_required("host");
	/* +opt +opt */
	snprintf(query, QUARTER_BLOB, "/client/v4/zones/%s/dns_records?type=A&name=%s&order=name&direction=asc", zone, host);

	s = http_req(ssl, 1, "api.cloudflare.com", query, header, 0, &body);

	if (s == -1)
		error(M_ERROR_GET_IP);
	else if (s == -2)
		error(M_ERROR_MEM_STREAM);

	body_copy = remove_spaces(body);
	if (!body_copy)
		error(M_ERROR_MEM_ALLOC);

	r = cloudflare_errorcheck(s, "GET", body_copy);

	addr = get_address(1);
	prox = get_option_onoff("wildcard", 0);

	if (r == 1) { /* no existing record - create with POST */
		if (get_option_onoff("backmx", 0)) {
			snprintf(query, QUARTER_BLOB, "/client/v4/zones/%s/dns_records", zone);
			/* continue to PUT/POST logic below */
		}
		else {
			free(body_copy);
			error(M_INVALID_HOST);
		}
	}
	else if (r == 0) { /* record exists */
		/* check if IP actually changed */
		find = "\"content\":\"";
		if ((found = strstr(body_copy, find)) == NULL) {
			free(body_copy);
			error(M_UNKNOWN_RESPONSE__D, -1);
		}

		found += strlen(find);
		if (strncmp(addr, found, strlen(addr)) == 0) {
			/* IP is the same - check proxied flag consistency */
			current_proxied = (strstr(body_copy, "\"proxied\":true") != NULL);
			if ((prox && current_proxied) || (!prox && !current_proxied)) {
				free(body_copy);
				success_msg(M_SAME_RECORD, 1); /* update cookie and exit */
			}
			/* if proxied flag differs, we still need to update */
		}

		/* extract record ID */
		find = "\"id\":\"";
		if ((found = strstr(body_copy, find)) == NULL) {
			free(body_copy);
			error(M_UNKNOWN_RESPONSE__D, -1);
		}

		found += strlen(find);
		*strchr(found, '"') = '\0'; /* truncate at closing quote */

		snprintf(query, QUARTER_BLOB, "/client/v4/zones/%s/dns_records/%s", zone, found);
	}
	else {
		/* r == -1 - error already handled inside cloudflare_errorcheck */
		free(body_copy);
		error(M_UNKNOWN_ERROR__D, r);
	}

	free(body_copy);
	body_copy = NULL;

	/* prepare JSON payload */
	snprintf(data, QUARTER_BLOB, "{\"content\":\"%s\",\"name\":\"%s\",\"proxied\":%s,\"type\":\"A\"}", addr, host, (prox ? "true" : "false"));

	/* POST for create, PUT for update */
	s = _http_req(ssl, 1, "api.cloudflare.com", (r == 1) ? "POST" : "PUT", query, header, 0, data, &body);

	if (s == -1)
		error(M_ERROR_GET_IP);
	else if (s == -2)
		error(M_ERROR_MEM_STREAM);

	body_copy = remove_spaces(body);
	if (!body_copy)
		error(M_ERROR_MEM_ALLOC);

	r = cloudflare_errorcheck(s, (r == 1) ? "POST" : "PUT", body_copy);

	free(body_copy); /* always free before exit */

	if (r != 0)
		error(M_UNKNOWN_ERROR__D, r);

	success();
}

/* duckdns.org
 * https://www.duckdns.org/install.jsp
 */
static void update_duckdns(const unsigned int ssl)
{
	long r;
	char *body;
	char query[2048];

	memset(query, 0, sizeof(query));
	snprintf(query, sizeof(query), "/update?domains=%s&token=%s", get_option_required("host"), get_option_required("ahash"));

	append_addr_option(query, "&ip=%s");

	r = http_req(ssl, 0, "www.duckdns.org", query, NULL, 0, &body);
	if (r == 200) {
		if (strstr(body, "OK"))
			success();

		error(M_UNKNOWN_RESPONSE__D, -1);
	}

	error(M_UNKNOWN_ERROR__D, r);
}

/* wget/custom */
static void update_custom(void)
{
	long r;
	char *c;
	char url[256];
	char s[256];
	int https;
	char *host;
	char path[256];
	char *p;
	char *body;
#ifdef TCONFIG_IPV6
	char buffer[INET6_ADDRSTRLEN];
#endif /* TCONFIG_IPV6 */

	/* https://user:pass@domain:port/path?query */

	strlcpy(url, get_option_required("url"), sizeof(url));
	https = 0;
	host = url + 7;
	if (strncasecmp(url, "https://", 8) == 0) {
		https = 1;
		++host;
	}
	else if (strncasecmp(url, "http://", 7) != 0)
		error(M_INVALID_PARAM__S, "url");

	if ((p = strchr(host, '/')) == NULL)
		error(M_INVALID_PARAM__S, "url");

	strlcpy(path, p, sizeof(path));
	*p = 0;

#ifdef TCONFIG_IPV6
	/* check for "@IP6" first but only if IPv6 is enabled! */
	if (ipv6_enabled() && ((c = strstr(path, "@IP6")) != NULL)) {

		/* try to get IPv6 address */
		if (get_address6(buffer, sizeof(buffer))) {
			size_t sizeOfPath = sizeof(path);
			strlcpy(s, c + 4, sizeof(s));
			strlcpy(c, buffer, sizeOfPath - strnlen(path, sizeOfPath) + 4); /*  space left in path is the sizeof the array - the currently used chars + 4 as @IP6 gets replaced */
			strlcat(c, s, sizeOfPath - strnlen(path, sizeOfPath)); /* space left is the size of the path array - the currently used chars (which now include the IP address) */
		}
		else {
			error("Unable to get global IPv6 address (br0)");
		}
	}
	else
#endif /* TCONFIG_IPV6 */
	if ((c = strstr(path, "@IP")) != NULL) {
		size_t sizeOfPath = sizeof(path);
		strlcpy(s, c + 3, sizeof(s));
		strlcpy(c, get_address(1), sizeOfPath - strnlen(path, sizeOfPath) + 3); /*  space left in path is the sizeof the array - the currently used chars + 3 as @IP gets replaced */
		strlcat(c, s, sizeOfPath - strnlen(path, sizeOfPath)); /* space left is the size of the path array - the currently used chars (which now include the IP address) */
	}

	logmsg(LOG_DEBUG, "*** %s: host: %s, path: %s", __FUNCTION__, host, path);

	if ((c = strrchr(host, '@')) != NULL) {
		*c = 0;
		host = c + 1;
		r = http_req(https, 1, host, path, NULL, 1, &body);
	}
	else
		r = http_req(https, 1, host, path, NULL, 0, &body);

	logmsg(LOG_DEBUG, "*** %s: IP: %s HOST: %s", __FUNCTION__, body, host);

	switch (r) {
	case 200:
	case 302: /* redirect -- assume ok */
		success();
		break;
	case 401:
		error(M_INVALID_AUTH);
		break;
	}

	error(M_UNKNOWN_ERROR__D, r);
}

static void check_cookie(void)
{
	const char *c;
	char addr[16];
	long tm;

	logmsg(LOG_DEBUG, "*** %s: IN", __FUNCTION__);

	if (((c = get_option("cookie")) == NULL) || (!read_tmaddr(c, &tm, addr))) {
		logmsg(LOG_DEBUG, "*** %s: no cookie", __FUNCTION__);
		return;
	}

	if ((c = get_address(0)) == NULL) {
		logmsg(LOG_DEBUG, "*** %s: no address specified", __FUNCTION__);
		return;
	}
	if (strcmp(c, addr) != 0) {
		logmsg(LOG_DEBUG, "*** %s: address is different (%s != %s)", __FUNCTION__, c, addr);
		return;
	}

	logmsg(LOG_DEBUG, "*** %s: " M_SAME_IP " (%s)", __FUNCTION__, c);
	puts(M_SAME_IP);

	logmsg(LOG_DEBUG, "*** %s: EXIT", __FUNCTION__);

	exit(3);
}

static void save_cookie(void)
{
	const char *cookie;
	const char *c;
	long now;
	char s[256];

	now = time(NULL);
	if (now < Y2K) {
		logmsg(LOG_DEBUG, "*** %s: no time", __FUNCTION__);
		return;
	}

	if ((cookie = get_option("cookie")) == NULL) {
		logmsg(LOG_DEBUG, "*** %s: no cookie", __FUNCTION__);
		return;
	}

	if ((c = get_address(0)) == NULL) {
		logmsg(LOG_DEBUG, "*** %s: no address specified", __FUNCTION__);
		return;
	}

	memset(s, 0, sizeof(s));
	snprintf(s, sizeof(s), "%ld,%s", now, c);
	f_write_string(cookie, s, FW_NEWLINE, 0);

	logmsg(LOG_DEBUG, "*** %s: OUT cookie=[%s]", __FUNCTION__, s);
}

int main(int argc, char *argv[])
{
	const char *p, *c;
	char tmp[16];
	int mwan_num, wan_unit;
	unsigned int no_wan_mode = 1;

	g_argc = argc;
	g_argv = argv;

	printf("mdu v" VERSION
#ifdef USE_LIBCURL
	" [libcurl]\n\n"
#else
	" [sockets]\n\n"
#endif
	"Copyright (C) 2007-2009 Jonathan Zarate\n"
	"Fixes/updates (C) 2018 - 2025 pedro\n\n");

	openlog("mdu", LOG_PID, LOG_DAEMON);

#ifdef USE_LIBCURL
	logmsg(LOG_DEBUG, "*** %s: IN v" VERSION " [libcurl]", __FUNCTION__);
#else
	logmsg(LOG_DEBUG, "*** %s: IN v" VERSION " [sockets]", __FUNCTION__);
#endif

	/* global variable */
	if ((blob = malloc(BLOB_SIZE)) == NULL) {
		logmsg(LOG_ERR, "Cannot alocate memory, aborting ...");
		return 1;
	}
	memset(blob, 0, BLOB_SIZE); /* reset */

	mkdir("/var/lib/mdu", 0700);
	chdir("/var/lib/mdu");
	eval("rm", "-f", MDU_STOP_FN); /* remove stop file on start */

	memset(sPrefix, 0, sizeof(sPrefix)); /* reset */
	memset(ifname, 0, sizeof(ifname)); /* reset */
	memset(tmp, 0, sizeof(tmp)); /* reset */

	/* addr (@...) is present in the config */
	if (((c = get_option("addr")) != NULL) && *c == '@') {
		if (nvram_get_int("mwan_num") > 1) {
			/* via what WAN check external IP? */
			snprintf(sPrefix, sizeof(sPrefix), (atoi(c + 1) == 1 ? "wan": "wan%s"), c + 1);
			snprintf(ifname, sizeof(ifname), "%s", get_wanface(sPrefix));
			if ((strcmp(ifname, "none") == 0) || (get_wanx_proto(sPrefix) == WP_DISABLED)) /* in some cases */
				memset(ifname, 0, sizeof(ifname)); /* reset again */
		}
		/* check if it's no WAN mode, if so - add custom interface */
		mwan_num = nvram_get_int("mwan_num");
		if (mwan_num > MWAN_MAX)
			mwan_num = MWAN_MAX;

		for (wan_unit = 1; wan_unit <= mwan_num; ++wan_unit) {
			memset(tmp, 0, sizeof(tmp)); /* reset */
			get_wan_prefix(wan_unit, tmp);
			if (get_wanx_proto(tmp) != WP_DISABLED) {
				logmsg(LOG_DEBUG, "*** %s: checking for no WAN mode - false, using default interface: %s", __FUNCTION__, ifname[0] != '\0' ? ifname : get_wanface("wan"));
				no_wan_mode = 0;
				break;
			}
		}
		if (no_wan_mode == 1) {
			logmsg(LOG_DEBUG, "*** %s: checking for no WAN mode - true, using custom interface: %s", __FUNCTION__, nvram_safe_get("ddnsx_custom_if"));
			memset(ifname, 0, sizeof(ifname)); /* reset */
			snprintf(ifname, sizeof(ifname), nvram_safe_get("ddnsx_custom_if"));
		}
	}

	check_cookie();

	p = get_option_required("service");

	logmsg(LOG_DEBUG, "*** %s: proceeding DDNS server update [service: %s ] ...", __FUNCTION__, p);

	if (strcmp(p, "changeip") == 0)
		update_dua("dyndns", 1, "nic.changeip.com", "/nic/update", 1);
	else if (strcmp(p, "cloudflare") == 0)
		update_cloudflare(1);
	else if (strcmp(p, "dnsexit") == 0)
		update_dnsexit(1);
	else if (strcmp(p, "dnshenet") == 0)
		update_dua(NULL, 1, "dyn.dns.he.net", "/nic/update", 0);
	else if (strcmp(p, "dnsomatic") == 0)
		update_dua(NULL, 1, "updates.dnsomatic.com", "/nic/update", 0);
	else if (strcmp(p, "dyndns") == 0)
		update_dua("dyndns", 1, "members.dyndns.org", "/nic/update", 1);
	else if (strcmp(p, "dyndns-static") == 0)
		update_dua("statdns", 1, "members.dyndns.org", "/nic/update", 1);
	else if (strcmp(p, "dyndns-custom") == 0)
		update_dua("custom", 1, "members.dyndns.org", "/nic/update", 1);
	else if (strcmp(p, "easydns") == 0)
		update_dua(NULL, 1, "members.easydns.com", "/dyn/dyndns.php", 1);
	else if (strcmp(p, "enom") == 0)
		update_enom(1); /* fixed cert */
	else if (strcmp(p, "afraid") == 0)
		update_afraid(1);
	else if (strcmp(p, "heipv6tb") == 0)
		update_dua("heipv6tb", 1, "ipv4.tunnelbroker.net", "/nic/update", 1);
	else if (strcmp(p, "namecheap") == 0)
		update_namecheap(1);
	else if (strcmp(p, "noip") == 0)
		update_dua(NULL, 1, "dynupdate.no-ip.com", "/nic/update", 1);
	else if (strcmp(p, "opendns") == 0)
		update_dua(NULL, 1, "updates.opendns.com", "/nic/update", 0);
	else if (strcmp(p, "ovh") == 0)
		update_dua("dyndns", 1, "www.ovh.com", "/nic/update", 1);
	else if (strcmp(p, "pairdomains") == 0)
		update_dua(NULL, 1, "dynamic.pairdomains.com", "/nic/update", 1);
	else if (strcmp(p, "pubyun") == 0)
		update_dua(NULL, 0, "members.3322.org", "/dyndns/update", 1); /* bad cert */
	else if (strcmp(p, "pubyun-static") == 0)
		update_dua("statdns", 0, "members.3322.org", "/dyndns/update", 1); /* bad cert */
	else if (strcmp(p, "zoneedit") == 0)
		update_zoneedit(1);
	else if (strcmp(p,  "duckdns") ==0)
		update_duckdns(1);
	else if ((strcmp(p, "wget") == 0) || (strcmp(p, "custom") == 0))
		update_custom();
	else
		error("Unknown service");

	logmsg(LOG_DEBUG, "*** %s: OUT", __FUNCTION__);

	return 1;
}
