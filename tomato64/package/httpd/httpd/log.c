/*
 *
 * Tomato Firmware
 * Copyright (C) 2006-2009 Jonathan Zarate
 * Fixes/updates (C) 2018 - 2023 pedro
 *
 */

#include "tomato.h"

#include <ctype.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>

/* Max number of log lines for GUI to display */
#define MAX_LOG_LINES	4000

/* Size of each input chunk to be read and allocate for. */
#ifndef READALL_CHUNK
#define READALL_CHUNK		262144
#endif

#define READALL_OK		 0	/* Success */
#define READALL_INVALID		-1	/* Invalid parameters */
#define READALL_ERROR		-2	/* Stream error */
#define READALL_TOOMUCH		-3	/* Too much input */
#define READALL_NOMEM		-4	/* Out of memory */

/* This function returns one of the READALL_ constants above.
   If the return value is zero == READALL_OK, then:
     (*dataptr) points to a dynamically allocated buffer, with
     (*sizeptr) chars read from the file.
     The buffer is allocated for one extra char, which is NUL,
     and automatically appended after the data.
   Initial values of (*dataptr) and (*sizeptr) are ignored.
 */
int readall(FILE *in, char **dataptr, size_t *sizeptr)
{
	char *data  = NULL, *temp;
	size_t size = 0;
	size_t used = 0;
	size_t n;

	/* None of the parameters can be NULL. */
	if ((in == NULL) || (dataptr == NULL) || (sizeptr == NULL))
		return READALL_INVALID;

	/* A read error already occurred? */
	if (ferror(in))
		return READALL_ERROR;

	while (1) {
		if ((used + READALL_CHUNK + 1) > size) {
			size = used + READALL_CHUNK + 1;

			/* Overflow check. Some ANSI C compilers may optimize this away, though. */
			if (size <= used) {
				free(data);
				return READALL_TOOMUCH;
			}

			temp = realloc(data, size);
			if (temp == NULL) {
				free(data);
				return READALL_NOMEM;
			}
			data = temp;
		}

		n = fread((data + used), 1, READALL_CHUNK, in);
		if (n == 0)
			break;

		used += n;
	}

	if (ferror(in)) {
		free(data);
		return READALL_ERROR;
	}

	temp = realloc(data, (used + 1));
	if (temp == NULL) {
		free(data);
		return READALL_NOMEM;
	}
	data = temp;
	data[used] = '\0';

	*dataptr = data;
	*sizeptr = used;

	return READALL_OK;
}

static int logok(void)
{
	if (nvram_match("log_file", "1"))
		return 1;

	resmsg_set("Internal logging disabled");
	redirect("error.asp");

	return 0;
}

/* Figure out & return the logfile name. */
void get_logfilename(char *lfn, size_t buf_sz)
{
	char *p;
	char cfg[256];
	char *nv;

	nv = (nvram_get_int("log_file_custom") != 0 ? nvram_safe_get("log_file_path") : "/var/log/messages");
	if (f_read_string("/etc/syslogd.cfg", cfg, sizeof(cfg)) > 0) {
		if ((p = strchr(cfg, '\n')))
			*p = 0;

		strtok(cfg, " \t");	/* skip rotsize */
		strtok(NULL, " \t");	/* skip backup cnt */
		if ((p = strtok(NULL, " \t")) && (*p == '/')) {
			/* check if we can write to the file */
			if (f_write(p, cfg, 0, FW_APPEND, 0) >= 0) {
				nv = p;	/* nv is the configured log filename */
			}
		}
	}
	if (lfn)
		strlcpy(lfn, nv, buf_sz);
}

void wo_viewlog(char *url)
{
	char *p, *c, *w;
	char s[128];
	char t[128];
	int logLines;
	char lfn[256];

	if (!logok())
		return;

	get_logfilename(lfn, sizeof(lfn));

	if ((w = webcgi_get("which")) == NULL)
		return;

	if (strcmp(w, "all") == 0)
		logLines = MAX_LOG_LINES;
	else if ((logLines = atoi(w)) <= 0)
		return;
	else if ((logLines = atoi(w)) > MAX_LOG_LINES)
		logLines = MAX_LOG_LINES;

	send_header(200, NULL, mime_plain, 0);

	/* show filtered */
	if ((p = webcgi_get("find")) != NULL) {
			if (strlen(p) > 64)
				return;

			c = t;
			while (*p) {
					switch (*p) {
					case '<':
					case '>':
					case '|':
					case '"':
					case '\\':
					case '`':
						*c++ = '\\';
						*c++ = *p;
						break;
					default:
						if (isprint(*p))
							*c++ = *p;
						break;
					}
					++p;
			}
			*c = 0;
			snprintf(s, sizeof(s), "grep -ih \"%s\" $(ls -1rv %s %s.* 2>/dev/null) 2>/dev/null | tail -n %d", t, lfn, lfn, logLines);
	}
	/* show all */
	else
		snprintf(s, sizeof(s), "cat $(ls -1rv %s %s.* 2>/dev/null) | tail -n %d", lfn, lfn, logLines);

	web_pipecmd(s, WOF_NONE);
}

void asp_showsyslog(int argc, char **argv)
{
	char s[128];
	int logLines = MAX_LOG_LINES;
	char lfn[256];

	if (!logok())
		return;

	get_logfilename(lfn, sizeof(lfn));

	if (argc > 1) {
		if ((logLines = atoi(argv[1])) <= 0)
			return;
		else if ((logLines = atoi(argv[1])) > MAX_LOG_LINES)
			logLines = MAX_LOG_LINES;
	}

	asp_time(0, 0); /* get current time and print in the first line */
	web_puts("\n");

	snprintf(s, sizeof(s), "cat $(ls -1rv %s %s.* 2>/dev/null) | tail -n %d", lfn, lfn, logLines);
	web_pipecmd(s, WOF_NONE);
}

static void webmon_list(char *name, int webmon, int resolve, unsigned int maxcount)
{
	FILE *f;
	char *js, *jh;
	char comma = ' ';
	unsigned long time;
	char host[NI_MAXHOST];
	char s[512], ip[64], val[256];
	size_t filesize;

	web_printf("\nwm_%s = [", name);

	if (webmon) {
		snprintf(s, sizeof(s), "/proc/webmon_recent_%s", name);
		if ((f = fopen(s, "r")) != NULL) {
			int readall_ok;
			char *data;
			readall_ok = readall(f, &data, &filesize);
			if (readall_ok == READALL_OK) {
				char *end = data + filesize;
				char *lineStart;
				unsigned int lines_processed = 0;
				for (lineStart = end; lineStart >= data; lineStart--) {
					if ((*lineStart == '\n') || (*lineStart == '\r') || (lineStart == data)) {
						const int length = end - lineStart;
						char *line = malloc(length + 1);
						char *start = lineStart;
						if (start > data)
							start += 1;

						strlcpy(line, start, length);
						line[length] = '\0';
						if (sscanf(line, "%lu\t%s\t%s", &time, ip, val) != 3)
							continue;

						jh = NULL;
						if (resolve && (resolve_addr(ip, host) == 0))
							jh = js_string(host);

						js = utf8_to_js_string(val);
						web_printf("%c['%lu','%s','%s', '%s']", comma, time, ip, js ? : "", jh ? : "");
						free(js);
						free(jh);
						comma = ',';
						free(line);
						end = lineStart;
						lines_processed++;

						if ((maxcount > 0) && (lines_processed >= maxcount))
							break;
					}
				}
			}
			fclose(f);
		}
	}
	web_puts("];\n");
}

void asp_webmon(int argc, char **argv)
{
	int webmon = nvram_get_int("log_wm");
	int maxcount = (argc > 0) ? atoi(argv[0]) : 0;
	int resolve = (argc > 1) ? atoi(argv[1]) : 0;

	webmon_list("domains", webmon, resolve, maxcount);
	webmon_list("searches", webmon, resolve, maxcount);
}

void wo_webmon(char *url)
{
	nvram_set("log_wmclear", webcgi_get("clear"));
	exec_service("firewall-restart");
	nvram_unset("log_wmclear");
}

static int webmon_ok(int searches)
{
	if (nvram_get_int("log_wm") && nvram_get_int(searches ? "log_wmsmax" : "log_wmdmax") > 0)
		return 1;

	resmsg_set("Web Monitoring disabled");
	redirect("error.asp");

	return 0;
}

void wo_syslog(char *url)
{
	char lfn[256];
	char s[128];

	get_logfilename(lfn, sizeof(lfn));

	if (strncmp(url, "webmon_", 7) == 0) {
		/* web monitor */
		char file[64];
		snprintf(file, sizeof(file), "/proc/%s", url);
		if (!webmon_ok(strstr(url, "searches") != NULL))
			return;

		send_header(200, NULL, mime_binary, 0);
		do_file(file);
	}
	else {
		/* syslog */
		if (!logok())
			return;

		send_header(200, NULL, mime_binary, 0);
		snprintf(s, sizeof(s), "cat $(ls -1rv %s %s.* 2>/dev/null)", lfn, lfn);
		web_pipecmd(s, WOF_NONE);
	}
}
