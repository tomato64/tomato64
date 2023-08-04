/*
 *
 * MDU -- Mini DDNS Updater
 * Copyright (C) 2007-2009 Jonathan Zarate
 *
 * Licensed under GNU GPL v2 or later versions.
 * Fixes/updates (C) 2018 - 2023 pedro
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

#ifdef USE_LIBCURL
#include <curl/curl.h>
#else
#include <netdb.h>
#include "mssl.h"
#endif

#include "shutils.h"
#include "shared.h"

// #define MDU_DEBUG
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
#define M_SAME_IP		"The IP address is the same."
#define M_SAME_RECORD		"Record already up-to-date."
#define M_DOWN			"Server temporarily down or under maintenance."

#ifdef USE_LIBCURL
int curl_sslerr = 1;
FILE *curl_dfile = NULL;
CURL *curl_handle = NULL;
#endif

/* needed by logmsg() */
#define LOGMSG_DISABLE		0
#define LOGMSG_NVDEBUG		"mdu_debug"


char *blob = NULL;
int error_exitcode = 1;

int g_argc;
char **g_argv;

char *f_argv[32];
int f_argc = -1;

static void save_cookie(void);

/* this should be in nvram so you can add/edit/remove checkers, but we have so little nvram it's impossible... */
static char services[][3][23] = { /* remember: the number in the third square bracket must be (len + 1) of the longest string */
/*	  service name			service hostname		path */
	{ "ipify.org",			"api.ipify.org",		"/"	},	/* txt */
	{ "amazonaws.com",		"checkip.amazonaws.com",	"/"	},	/* txt */
	{ "dyndns.org",			"checkip.dyndns.org",		"/"	},	/* "<html><head><title>Current IP Check</title></head><body>Current IP Address: 1.2.3.4</body></html>" */
	{ "corz.org",			"corz.org",			"/ip"	},	/* txt */
	{ "trackip.net",		"trackip.net",			"/ip"	},	/* txt */
	{ "changeip.com",		"ip.changeip.com",		"/"	},	/* "1.2.3.4\n<!--IPADDR=1.2.3.4-->" */
	{ "ifconfig.co",		"ifconfig.co",			"/ip"	},	/* txt */
	{ "ident.me",			"ident.me",			"/"	},	/* txt */
	{ "eth0.me",			"eth0.me",			"/"	},	/* txt */
	{ "myexternalip.com",		"myexternalip.com",		"/raw"	},	/* txt */
	{ "tyk.nu",			"ip.tyk.nu",			"/"	},	/* txt */
	{ "wgetip.com",			"wgetip.com",			"/"	},	/* txt? */
	{ "ipecho.net",			"ipecho.net",			"/plain"},	/* txt? */
	{ "ifconfig.me",		"ifconfig.me",			"/ip"	},	/* txt */
	{ "icanhazip.com",		"icanhazip.com",		"/"	},	/* txt */
};

static void trimamp(char *s)
{
	int n;

	n = strlen(s);
	if ((n > 0) && (s[--n] == '&'))
		s[n] = 0;
}

static const char *get_option(const char *name)
{
	char *p;
	int i;
	int n;
	FILE *f;
	const char *c;
	char s[384];

	if (f_argc < 0) {
		f_argc = 0;
		if ((c = get_option("conf")) != NULL) {
			if ((f = fopen(c, "r")) != NULL) {
				while (fgets(s, sizeof(s), f)) {
					p = s;
					if ((s[0] == '-') && (s[1] == '-'))
						p += 2;

					if ((c = strchr(p, ' ')) != NULL) {
						n = strlen(p);
						if (p[n - 1] == '\n')
							p[n - 1] = 0;

						n = strlen(c + 1);
						if (n <= 0) continue;
						if (n >= MAX_OPTION_LENGTH)
							exit(88);
						if ((p = strdup(p)) == NULL)
							exit(99);

						f_argv[f_argc++] = p;
						if ((unsigned int) f_argc >= (sizeof(f_argv) / sizeof(f_argv[0])))
							break;
					}
				}
				fclose(f);
			}
		}
	}

	n = strlen(name);
	for (i = 0; i < f_argc; ++i) {
		c = f_argv[i];
		if ((strncmp(c, name, n) == 0) && (c[n] == ' '))
			return c + n + 1;
	}

	for (i = 0; i < g_argc; ++i) {
		p = g_argv[i];
		if ((p[0] == '-') && (p[1] == '-')) {
			if (strcmp(p + 2, name) == 0) {
				++i;
				if ((i >= g_argc) || (strlen(g_argv[i]) >= MAX_OPTION_LENGTH))
					break;

				return g_argv[i];
			}
		}
	}
	return NULL;
}

static const char *get_option_required(const char *name)
{
	const char *p;

	if ((p = get_option(name)) != NULL)
		return p;

	logmsg(LOG_ERR, "Required option --%s is missing.", name);
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

	logmsg(LOG_ERR, "--%s requires the value off/on or 0/1.\n", name);
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
	printf("%s", s);
	save_msg(s);

	exit(error_exitcode);
}

static void success_msg(const char *msg)
{
	save_cookie();

	logmsg(LOG_DEBUG, "*** %s, msg: %s", __FUNCTION__, msg);
	printf("%s", msg);
	save_msg(msg);

	exit(0);
}

static void success(void)
{
	success_msg("Update successful.");
}

static const char *get_dump_name(void)
{
#ifdef MDU_DEBUG
	return get_option_or("dump", "/tmp/mdu.txt");
#else
	return get_option("dump");
#endif
}

#ifdef USE_LIBCURL
static int curl_dump(CURL *handle, curl_infotype type, char *data, size_t size, void *userptr)
{
	const char *prefix;
	FILE *f_out;
	size_t i;
	unsigned char c;
	int is_info = 0;

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

	/* pretty up a bit */
	if (is_info) {
		if (data[size - 1] == '\n')
			size -= 1;
		if (data[size - 1] == ':')
			size -= 1;
	}
	else if (data[size - 2] == '\r' && data[size - 1] == '\n')
		size -= 2;

	f_out = (FILE *)userptr;
	fputs(prefix, f_out);

	c = 0;
	for (i = 0; i < size; ++i) {
		c = data[i];
		if (c == '\r' && !is_info)
			fputc('\n', f_out);
		else if (c == '\n') {
			if (is_info)
				fputc('\n', f_out);

			fputs(prefix, f_out);
		}
		else
			fputc((c >= 0x20 && c < 0x80) ? c : '.', f_out);
	}
	fputc('\n', f_out);

	return 0;
}

static void curl_setup()
{
	CURLsslset result;
	const char *dump;

	result = curl_global_sslset(CURLSSLBACKEND_OPENSSL, NULL, NULL);
	if ((result == CURLSSLSET_OK) || (result == CURLSSLSET_TOO_LATE))
		curl_sslerr = 0;

	if (curl_global_init(CURL_GLOBAL_ALL) || !(curl_handle = curl_easy_init()))
		error("libcurl initialization failure.");

#ifndef TCONFIG_STUBBY
	curl_easy_setopt(curl_handle, CURLOPT_SSL_VERIFYPEER, 0L);
#endif
	curl_easy_setopt(curl_handle, CURLOPT_FOLLOWLOCATION, 1L);
	curl_easy_setopt(curl_handle, CURLOPT_MAXREDIRS, 5L);
	curl_easy_setopt(curl_handle, CURLOPT_CONNECTTIMEOUT, 10L);
	curl_easy_setopt(curl_handle, CURLOPT_TIMEOUT, 10L);

	if ((dump = get_dump_name()) != NULL) {
		curl_easy_setopt(curl_handle, CURLOPT_VERBOSE, 1L);
		if ((curl_dfile = fopen(dump, "a")) != NULL) {
			curl_easy_setopt(curl_handle, CURLOPT_DEBUGFUNCTION, curl_dump);
			curl_easy_setopt(curl_handle, CURLOPT_DEBUGDATA, (void *)curl_dfile);
		}
	}
}

static void curl_cleanup()
{
	if (curl_dfile != NULL)
		fclose(curl_dfile);

	curl_easy_cleanup(curl_handle);
	curl_global_cleanup();
}

static struct curl_slist *curl_headers(const char *header)
{
	char *sub = NULL;
	struct curl_slist *headers = NULL;
	struct curl_slist *tmp = NULL;
	size_t n = strlen(header);

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
#else /* !USE_LIBCURL */
static int _http_req(int ssl, const char *host, int port, const char *request, char *buffer, int bufsize, char **body)
{
	struct hostent *he;
	struct sockaddr_in sa;
	int sd = -1;
	FILE *f;
	unsigned int i;
	int trys;
	char *p;
	const char *c;
	struct timeval tv;

	logmsg(LOG_DEBUG, "*** %s: %s", __FUNCTION__, host);

	for (trys = 4; trys > 0; --trys) {
		logmsg(LOG_DEBUG, "*** %s: _http_req trys=%d\n", __FUNCTION__, trys);

		for (i = 4; i > 0; --i) {
			if ((he = gethostbyname(host)) != NULL) {
				if ((sd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
					return -1;

				memset(&sa, 0, sizeof(sa));
				sa.sin_family = AF_INET;
				sa.sin_port = htons(port);
				memcpy(&sa.sin_addr, he->h_addr, sizeof(sa.sin_addr));

#ifdef MDU_DEBUG
				struct in_addr ia;
				ia.s_addr = sa.sin_addr.s_addr;

				logmsg(LOG_DEBUG, "*** %s: [%s][%d] - connecting...", __FUNCTION__, inet_ntoa(ia), port);
#endif

				if (connect_timeout(sd, (struct sockaddr *)&sa, sizeof(sa), 10) == 0) {
					logmsg(LOG_DEBUG, "*** %s: connected", __FUNCTION__);
					break;
				}
#ifdef MDU_DEBUG
				logerr(__FUNCTION__, __LINE__, "connect");
#endif
				close(sd);
				sleep(2);
			}
		}
		if (i <= 0)
			return -1;

		tv.tv_sec = 10;
		tv.tv_usec = 0;
		setsockopt(sd, SOL_SOCKET, SO_SNDTIMEO, &tv, sizeof(tv));
		setsockopt(sd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));

		if (ssl) {
			mssl_init(NULL, NULL);
			f = ssl_client_fopen_name(sd, host);
		}
		else
			f = fdopen(sd, "r+");

		if (f == NULL) {
			logerr(__FUNCTION__, __LINE__, "error opening");
			close(sd);
			continue;
		}

		i = strlen(request);
		if (fwrite(request, 1, i, f) != i) {
			logerr(__FUNCTION__, __LINE__, "error writing");
			fclose(f);
			close(sd);
			continue;
		}
		logmsg(LOG_DEBUG, "*** %s: sent request", __FUNCTION__);

		i = fread(buffer, 1, bufsize, f);
		if (i <= 0) {
			fclose(f);
			close(sd);
			logerr(__FUNCTION__, __LINE__, "error reading");
			continue;
		}
		buffer[i] = 0;

		logmsg(LOG_DEBUG, "*** %s: recvd=[%s], i=%d", __FUNCTION__, buffer, i);

		fclose(f);
		close(sd);

		if ((c = get_dump_name()) != NULL) {
			if ((f = fopen(c, "a")) != NULL) {
				fprintf(f, "\n[%s:%d]\nREQUEST\n", host, port);
				fputs(request, f);
				fputs("\nREPLY\n", f);
				fputs(buffer, f);
				fputs("\nEND\n", f);
				fclose(f);
			}
		}

		if ((sscanf(buffer, "HTTP/1.%*d %d", &i) == 1) && (i >= 100) && (i <= 999)) {
			logmsg(LOG_DEBUG, "*** %s: HTTP/1.* i=%d", __FUNCTION__, i);
			if ((p = strstr(buffer, "\r\n\r\n")) != NULL)
				p += 4;
			else if ((p = strstr(buffer, "\n\n")) != NULL)
				p += 2;

			if (p) {
				if (body) {
					*body = p;
					logmsg(LOG_DEBUG, "*** %s: body=[%s]", __FUNCTION__, p);
				}
				return i;
			}
			else
				logmsg(LOG_DEBUG, "*** %s: !p", __FUNCTION__);
		}
	}

	return -1;
}
#endif /* USE_LIBCURL */

static int http_req(int ssl, int static_host, const char *host, const char *req, const char *query, const char *header, int auth, char *data, char **body)
{
	logmsg(LOG_DEBUG, "*** %s: IN host=[%s] query=[%s] header=[%s] auth=[%d] data=[%s] req=[%s]", __FUNCTION__, host, query, header, auth, data, req);
#ifdef USE_LIBCURL
	struct curl_slist *headers = NULL;
	char url[HALF_BLOB];
	FILE *curl_wbuf = NULL;
	FILE *curl_rbuf = NULL;
	CURLcode r;
	int trys;
	long code;

	if (!static_host)
		host = get_option_or("server", host);

	if (ssl) {
		if (curl_sslerr) {
			curl_cleanup();
			error("SSL failure with libcurl.");
		}
		snprintf(url, HALF_BLOB, "https://%s%s", host, query);
	}
	else
		snprintf(url, HALF_BLOB, "http://%s%s", host, query);

	curl_easy_setopt(curl_handle, CURLOPT_URL, url);
	if (header)
		headers = curl_headers(header);
	else
		headers = curl_headers("User-Agent: " AGENT "\r\nCache-Control: no-cache");

	curl_easy_setopt(curl_handle, CURLOPT_HTTPHEADER, headers);

	if (auth) {
		curl_easy_setopt(curl_handle, CURLOPT_USERNAME, get_option_required("user"));
		curl_easy_setopt(curl_handle, CURLOPT_PASSWORD, get_option_required("pass"));
		curl_easy_setopt(curl_handle, CURLOPT_HTTPAUTH, CURLAUTH_BASIC);
	}
	else
		curl_easy_setopt(curl_handle, CURLOPT_HTTPAUTH, CURLAUTH_NONE);

	curl_wbuf = fmemopen(blob, HALF_BLOB, "w");
	setbuf(curl_wbuf, NULL);
	curl_easy_setopt(curl_handle, CURLOPT_WRITEDATA, (void *)curl_wbuf);
	if (data) {
		curl_rbuf = fmemopen(data, strlen(data), "r");
		curl_easy_setopt(curl_handle, CURLOPT_READDATA, (void *)curl_rbuf);
		curl_easy_setopt(curl_handle, CURLOPT_INFILESIZE, strlen(data));
		curl_easy_setopt(curl_handle, CURLOPT_UPLOAD, 1L);
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

	for (trys = 4; trys > 0; --trys) {
		r = curl_easy_perform(curl_handle);
		if (r != CURLE_COULDNT_CONNECT)
			break;
#ifdef MDU_DEBUG
		logerr(__FUNCTION__, __LINE__, "connect");
#endif
		sleep(2);
	}
	curl_slist_free_all(headers);
	curl_easy_getinfo(curl_handle, CURLINFO_RESPONSE_CODE, &code);
	fclose(curl_wbuf);
	if (curl_rbuf)
		fclose(curl_rbuf);

	if (curl_dfile) {
		fputc('\n', curl_dfile);
		fflush(curl_dfile);
	}
	if (r != CURLE_OK) {
		curl_cleanup();
		error("Unknown libcurl error %d with response code %ld.", r, code);
	}

	*body = blob;
	return code;
#else /* !USE_LIBCURL */
	char *p;
	int port;
	char a[512];
	char b[512];
	int n;
	char *httpv;

	if (strncmp(host, "updates.opendns.com", 19) == 0 || strncmp(host, "api.cloudflare.com", 18) == 0)
		httpv = "HTTP/1.1";
	else
		httpv = "HTTP/1.0";

	if (!static_host)
		host = get_option_or("server", host);

	n = strlen(query);
	if (header)
		n += strlen(header);
	if (data)
		n += strlen(data);
	if (n > (BLOB_SIZE - 512)) /* just don't go over 512 below... */
		return -1;

	snprintf(blob, BLOB_SIZE, "%s %s %s\r\nHost: %s\r\nUser-Agent: " AGENT "\r\nCache-Control: no-cache\r\n", req, query, httpv, host);

	if (auth) {
		memset(a, 0, sizeof(a));
		snprintf(a, sizeof(a), "%s:%s", get_option_required("user"), get_option_required("pass"));

		n = base64_encode((const char *) a, b, strlen(a));
		b[n] = 0;
		snprintf(blob + strlen(blob), BLOB_SIZE - strlen(blob), "Authorization: Basic %s\r\n", b);
	}
	if ((header) && ((n = strlen(header)) > 0)) {
		strlcat(blob, header, BLOB_SIZE);
		if (header[n - 1] != '\n')
			strlcat(blob, "\r\n", BLOB_SIZE);
	}
	if (data)
		snprintf(blob + strlen(blob), BLOB_SIZE - strlen(blob), "Content-Length: %d\r\n", strlen(data));

	strlcat(blob, "\r\n", BLOB_SIZE);

	if (data)
		strlcat(blob, data, BLOB_SIZE);

	port = ssl ? 443 : 80;
	memset(a, 0, sizeof(a));
	strlcpy(a, host, sizeof(a));
	if ((p = strrchr(a, ':')) != NULL) {
		*p = 0;
		if ((n = atoi(p + 1)) > 0)
			port = n;
	}

	if ((p = strdup(blob)) == NULL)
		return -1;

	logmsg(LOG_DEBUG, "*** %s: host=[%s] port=[%d] request=[%s] ssl=[%d] buffer=[%s]", __FUNCTION__, a, port, p, ssl, blob);

	n = _http_req(ssl, a, port, p, blob, BLOB_SIZE, body);
	free(p);

	logmsg(LOG_DEBUG, "*** %s: n=%d", __FUNCTION__, n);

	return n;
#endif /* USE_LIBCURL */
}

static int wget(int ssl, int static_host, const char *host, const char *get, const char *header, int auth, char **body)
{
	return http_req(ssl, static_host, host, "GET", get, header, auth, NULL, body);
}

int read_tmaddr(const char *name, long *tm, char *addr)
{
	char s[64];

	logmsg(LOG_DEBUG, "*** %s: IN cachename: %s", __FUNCTION__, name);

	if (f_read_string(name, s, sizeof(s)) > 0) {
		if (sscanf(s, "%ld,%15s", tm, addr) == 2) {
			logmsg(LOG_DEBUG, "*** %s: s=%s tm=%ld addr=%s", __FUNCTION__, s, *tm, addr);

			if ((tm > 0) && (inet_addr(addr) != INADDR_NONE))
				return 1;
		}
		else
			logmsg(LOG_DEBUG, "*** %s: unknown=%s", __FUNCTION__, s);
	}
	return 0;
}

const char *get_address(int required)
{
	char *body;
	struct in_addr ia;
	const char *c, *d;
	char *p, *q;
	char s[64];
	char cache_name[64];
	static char addr[16];
	long ut, et;
	int rows, service_num, n;

	/* addr is present in the config */
	if ((c = get_option("addr")) != NULL) {
		/* do not use custom IP address, run IP checker */
		if (*c == '@') {
			ut = get_uptime();

			d = get_option_required("addrcache");
			strlcpy(cache_name, d, sizeof(cache_name));

			if (read_tmaddr(cache_name, &et, addr)) {
				if ((et > ut) && ((et - ut) <= DDNS_IP_CACHE)) {
					logmsg(LOG_DEBUG, "*** %s: Using cached address %s from %s. Expires in %ld seconds", __FUNCTION__, addr, cache_name, (et - ut));
					return addr;
				}
			}

			logmsg(LOG_DEBUG, "*** %s: running External IP address checker ...", __FUNCTION__);

			rows = sizeof(services) / sizeof(services[0]);
			n = 5; /* try 5 times on different checkers, if no response it means (probably) WAN is down - wait */
			while (n-- > 0) {
				srand(time(0));
				service_num = (rand() % (rows));
				if (wget(0, 1, services[service_num][1], services[service_num][2], NULL, 0, &body) == 200) {

					if ((p = strstr(body, "Address:")) != NULL) /* dyndns */
						p += 8;
					else /* the rest */
						p = body;

					/* sanitize */
					while (*p == ' ')
						++p;

					q = p;

					while (((*q >= '0') && (*q <= '9')) || (*q == '.'))
						++q;

					memset(addr, 0, sizeof(addr)); /* reset */
					strncpy(addr, p, (q - p));
					q = NULL;

					/* write to cache if addr is OK */
					if ((ia.s_addr = inet_addr(addr)) != INADDR_NONE) {
						q = inet_ntoa(ia);
						memset(s, 0, sizeof(s));
						snprintf(s, sizeof(s), "%ld,%s", ut + DDNS_IP_CACHE, q);
						f_write_string(cache_name, s, 0, 0);

						logmsg(LOG_DEBUG, "*** %s: used %s service; time,address (%s) saved to %s", __FUNCTION__, services[service_num][0], s, cache_name);
						return q;
					}
				}
				else {
					if (n == 0) {
						logmsg(LOG_DEBUG, "*** %s: " M_ERROR_GET_IP, __FUNCTION__);
						error(M_ERROR_GET_IP);
					}
					else
						logmsg(LOG_DEBUG, "*** %s: " M_ERROR_GET_IP " (%s) - trying another one ...", __FUNCTION__, services[service_num][0]);
				}
			}
		}
		return c;
	}

	return required ? get_option_required("addr") : NULL;
}

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
static void update_dua(const char *type, int ssl, const char *server, const char *path, int reqhost)
{
	const char *p;
	char query[2048];
	int r;
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

	r = wget(ssl, 0, server ? server : get_option_required("server"), query, NULL, 1, &body);
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
static void update_namecheap(int ssl)
{
	int r;
	char *p;
	char *q;
	char *body;
	char query[2048];

	/* +opt +opt +opt */
	memset(query, 0, sizeof(query));
	snprintf(query, sizeof(query), "/update?host=%s&domain=%s&password=%s", get_option_required("host"), get_option("user") ? : get_option_required("domain"), get_option_required("pass"));

	/* +opt */
	append_addr_option(query, "&ip=%s");

	r = wget(ssl, 0, "dynamicdns.park-your-domain.com", query, NULL, 0, &body);
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
static void update_enom(int ssl)
{
	int r;
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

	r = wget(ssl, 0, "dynamic.name-services.com", query, NULL, 0, &body);
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
static void update_dnsexit(int ssl)
{
	int r;
	char *body;
	char query[2048];

	/* +opt +opt +opt */
	memset(query, 0, sizeof(query));
	snprintf(query, sizeof(query), "/RemoteUpdate.sv?login=%s&password=%s&host=%s", get_option_required("user"), get_option_required("pass"), get_option_required("host"));

	/* +opt */
	append_addr_option(query, "&myip=%s");

	r = wget(ssl, 0, "update.dnsexit.com", query, NULL, 0, &body);
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
	ieserver.net
	http://www.ieserver.net/tools.html

	---

	http://ieserver.net/cgi-bin/dip.cgi?username=XXX&domain=XXX&password=XXX&updatehost=1

	username = hostname
	domain = dip.jp, fam.cx, etc.
*/
static void update_ieserver(int ssl)
{
	int r;
	char *body;
	char query[2048];
	char *p;

	/* +opt +opt */
	memset(query, 0, sizeof(query));
	snprintf(query, sizeof(query), "/cgi-bin/dip.cgi?username=%s&domain=%s&password=%s&updatehost=1", get_option_required("user"), get_option_required("host"), get_option_required("pass"));

	r = wget(ssl, 0, "ieserver.net", query, NULL, 0, &body);
	if (r == 200) {
		if (strstr(body, "<title>Error")) {
			/* <p>yuuzaa na mata pasuwoodo (EUC-JP) */
			if ((p = strstr(body, "<p>\xA5\xE6\xA1\xBC\xA5\xB6\xA1\xBC")) != NULL) /* <p>user */
				error(M_INVALID_AUTH);

			error(M_UNKNOWN_RESPONSE__D, -1);
		}

		success();
	}

	error(M_UNKNOWN_ERROR__D, r);
}

/*
	dyns.cx
	http://www.dyns.cx/documentation/technical/protocol/v1.1.php

	---

"HTTP/1.1 200 OK
...

401 testuser not authenticated"
*/
static void update_dyns(int ssl)
{
	int r;
	char *body;
	char query[2048];


	/* +opt +opt +opt */
	memset(query, 0, sizeof(query));
	snprintf(query, sizeof(query), "/postscript011.php?username=%s&password=%s&host=%s", get_option_required("user"), get_option_required("pass"), get_option_required("host"));

	/* +opt */
	append_addr_option(query, "&ip=%s");

	r = wget(ssl, 0, "www.dyns.net", query, NULL, 0, &body);
	if (r == 200) {
		while ((*body == ' ') || (*body == '\r') || (*body == '\n')) {
			++body;
		}
		switch (r = atoi(body)) {
		case 200:
			success();
			break;
		case 400:
			error(M_INVALID_PARAM__D, r);
			break;
		case 401:
			error(M_INVALID_AUTH);
			break;
		case 402:
			error(M_TOOSOON);
			break;
		case 405:
			error(M_INVALID_HOST);
			break;
		}

		error(M_UNKNOWN_RESPONSE__D, r);
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
static void update_zoneedit(int ssl)
{
	int r;
	char *body;
	char *c;
	char query[2048];

	/* +opt */
	memset(query, 0, sizeof(query));
	snprintf(query, sizeof(query), "/auth/dynamic.html?host=%s", get_option_required("host"));

	/* +opt */
	append_addr_option(query, "&dnsto=%s");

	r = wget(ssl, 0, "dynamic.zoneedit.com", query, NULL, 1, &body);
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
static void update_afraid(int ssl)
{
	int r;
	char *body;
	char query[2048];

	/* +opt */
	memset(query, 0, sizeof(query));
	snprintf(query, sizeof(query), "/dynamic/update.php?%s", get_option_required("ahash"));

	/* +opt */
	append_addr_option(query, "&address=%s");

	r = wget(ssl, 0, "freedns.afraid.org", query, NULL, 0, &body);
	if (r == 200) {
		if ((strstr(body, "Updated")) && (strstr(body, "seconds")))
			success();
		else if ((strstr(body, "ERROR")) || (strstr(body, "fail"))) {
			if (strstr(body, "has not changed"))
				success();

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
static int cloudflare_errorcheck(int code, const char *req, char *body)
{
	unsigned int n = 0, i = 0;
	for (i = 0; i < strlen(body); ++i) {
		if (body[i] != ' ')
			body[n++] = body[i];
	}
	body[n] = '\0';

	if (code == 200) {
		if (strstr(body, "\"success\":true") != NULL) {
			if (strstr(body, "\"total_count\":0") != NULL)
				return 1;

			return 0;
		}
		else
			error(M_UNKNOWN_RESPONSE__D, -1);
	}
	else if (code == 400 && strstr(body, "\"code\":6003") != NULL)
		error(M_INVALID_AUTH);
	else if (code == 403 && strstr(body, "\"code\":9103") != NULL)
		error(M_INVALID_AUTH);

	error("%s returned HTTP error code %d.", req, code);

	return -1;
}

static void update_cloudflare(int ssl)
{
	char header[HALF_BLOB];
	const char *zone;
	const char *host;
	char query[QUARTER_BLOB];
	char *body;
	int r;
	const char *req;
	const char *addr;
	int prox;
	const char *find;
	char *found;
	char data[QUARTER_BLOB];

	/* +opt */
	snprintf(header, HALF_BLOB, "User-Agent: " AGENT "\r\nAuthorization: Bearer %s\r\nContent-Type: application/json\r\n", get_option_required("pass"));

	zone = get_option_required("url");
	host = get_option_required("host");
	/* +opt +opt */
	snprintf(query, QUARTER_BLOB, "/client/v4/zones/%s/dns_records?type=A&name=%s&order=name&direction=asc", zone, host);

	r = wget(ssl, 1, "api.cloudflare.com", query, header, 0, &body);
	r = cloudflare_errorcheck(r, "GET", body);
	req = "PUT";
	addr = get_address(1);
	prox = get_option_onoff("wildcard", 0);
	if (r == 1) {
		if (get_option_onoff("backmx", 0)) {
			//req = "POST";
			snprintf(query, QUARTER_BLOB, "/client/v4/zones/%s/dns_records", zone);
		}
		else
			error(M_INVALID_HOST);
	}
	else if (r == 0) {
		/* check the current IP to see if we actually need to update */
		find = "\"content\":\"";
		if ((found = strstr(body, find)) == NULL)
			error(M_UNKNOWN_RESPONSE__D, -1);
		found += strlen(find);
		if (strncmp(addr, found, strlen(addr)) == 0) {
			if (strstr(body, "\"proxiable\":true") != NULL) {
				if (strstr(body, "\"proxied\":true") != NULL) {
					if (prox)
						success_msg(M_SAME_RECORD); /* use success to update the cookie */
				}
				else if (!prox)
					success_msg(M_SAME_RECORD); /* use success to update the cookie */
			}
			else
				success_msg(M_SAME_RECORD); /* use success to update the cookie */
		}

		find = "\"id\":\"";
		if ((found = strstr(body, find)) == NULL)
			error(M_UNKNOWN_RESPONSE__D, -1);

		found += strlen(find);
		*strchr(found, '"') = 0; /* assume we can find the closing quote */

		snprintf(query, QUARTER_BLOB, "/client/v4/zones/%s/dns_records/%s", zone, found);
	}
	else
		error(M_UNKNOWN_ERROR__D, r);

	/* +opt +opt */
	snprintf(data, QUARTER_BLOB, "{\"type\":\"A\",\"name\":\"%s\",\"content\":\"%s\",\"proxied\":%s}", host, addr, (prox ? "true" : "false"));

	r = http_req(ssl, 1, "api.cloudflare.com", req, query, header, 0, data, &body);
	r = cloudflare_errorcheck(r, req, body);
	if (r != 0)
		error(M_UNKNOWN_ERROR__D, r);

	success();
}

/* duckdns.org
 * https://www.duckdns.org/install.jsp
 */
static void update_duckdns(int ssl)
{
	int r;
	char *body;
	char query[2048];

	memset(query, 0, sizeof(query));
	snprintf(query, sizeof(query), "/update?domains=%s&token=%s", get_option_required("host"), get_option_required("ahash"));

	append_addr_option(query, "&ip=%s");

	r = wget(ssl, 0, "www.duckdns.org", query, NULL, 0, &body);
	if (r == 200) {
		if (strstr(body, "OK"))
			success();

		error(M_UNKNOWN_RESPONSE__D, -1);
	}

	error(M_UNKNOWN_ERROR__D, r);
}

/* wget/custom */
static void update_wget(void)
{
	int r;
	char *c;
	char url[256];
	char s[256];
	int https;
	char *host;
	char path[256];
	char *p;
	char *body;

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

	if ((c = strstr(path, "@IP")) != NULL) {
		strlcpy(s, c + 3, sizeof(s));
		strlcpy(c, get_address(1), sizeof(path) - strlen(path) + 3); /*  space left in path is the sizeof the array - the currently used chars + 3 as @IP gets replace */
		strlcat(c, s, sizeof(path) - strlen(path)); /* space left is the size of the path array - the currently used chars (which now include the IP address) */
	}

	logmsg(LOG_DEBUG, "*** %s: host: %s, path: %s", __FUNCTION__, host, path);

	if ((c = strrchr(host, '@')) != NULL) {
		*c = 0;
		host = c + 1;
		r = wget(https, 1, host, path, NULL, 1, &body);
	}
	else
		r = wget(https, 1, host, path, NULL, 0, &body);

	logmsg(LOG_DEBUG, "*** %s: IP: %s", __FUNCTION__, body);

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

	logmsg(LOG_DEBUG, "*** %s: cookie=%s", __FUNCTION__, s);
}

int main(int argc, char *argv[])
{
	const char *p;

	g_argc = argc;
	g_argv = argv;

	printf("MDU\nCopyright (C) 2007-2009 Jonathan Zarate\n\n");

	openlog("mdu", LOG_PID, LOG_DAEMON);

	logmsg(LOG_DEBUG, "*** %s: IN", __FUNCTION__);

	if ((blob = malloc(BLOB_SIZE)) == NULL) {
		logmsg(LOG_ERR, "Cannot alocate memory, aborting ...");
		return 1;
	}

	mkdir("/var/lib/mdu", 0700);
	chdir("/var/lib/mdu");

#ifdef USE_LIBCURL
	curl_setup();
#endif

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
	else if (strcmp(p, "dyns") == 0)
		update_dyns(0); /* no https */
	else if (strcmp(p, "easydns") == 0)
		update_dua(NULL, 1, "members.easydns.com", "/dyn/dyndns.php", 1);
	else if (strcmp(p, "enom") == 0)
		update_enom(0); /* bad cert */
	else if (strcmp(p, "afraid") == 0)
		update_afraid(1);
	else if (strcmp(p, "heipv6tb") == 0)
		update_dua("heipv6tb", 1, "ipv4.tunnelbroker.net", "/nic/update", 1);
	else if (strcmp(p, "ieserver") == 0)
		update_ieserver(1); /* TLS 1.0 only... */
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
		update_wget();
	else
		error("Unknown service");

#ifdef USE_LIBCURL
	curl_cleanup();
#endif

	logmsg(LOG_DEBUG, "*** %s: OUT", __FUNCTION__);

	return 1;
}
