/*
 *
 * Tomato Firmware
 * Copyright (C) 2006-2009 Jonathan Zarate
 *
 * Fixes/updates (C) 2018 - 2026 pedro
 * https://freshtomato.org/
 *
 */


#include "tomato.h"
#ifdef TCONFIG_HTTPS
 #include "mssl.h"
#endif
#include <errno.h>
#include <stdarg.h>


extern FILE *connfp;
extern int do_ssl;
extern int connfd;

int web_getline(char *buffer, int max)
{
	while (fgets(buffer, max, connfp) == NULL) {
		if (errno != EINTR) return 0;
	}
	//cprintf("%s", buffer);

	return 1;
}

void web_puts(const char *buffer)
{
	web_write(buffer, strlen(buffer));
}

void web_putj(const char *buffer)
{
	char *p;

	p = js_string(buffer);
	if (p) {
		web_puts(p);
		free(p);
	}
}

void web_putj_utf8(const char *buffer)
{
	char *p;

	p = utf8_to_js_string(buffer);
	if (p) {
		web_puts(p);
		free(p);
	}
}

/* output a JS variable assignment with a safely escaped string value.
 * uses web_puts for the template and web_putj for the value to prevent XSS.
 */
void web_putj_nvram(const char *varname, const char *value)
{
	web_printf("\n%s = '", varname);
	web_putj(value);
	web_puts("';");
}

void web_puth(const char *buffer)
{
	char *p;

	p = html_string(buffer);
	if (p) {
		web_puts(p);
		free(p);
	}
}

void web_puth_utf8(const char *buffer)
{
	char *p;

	p = utf8_to_html_string(buffer);
	if (p) {
		web_puts(p);
		free(p);
	}
}

int _web_printf(wofilter_t wof, const char *format, ...)
{
	va_list args;
	char *b, *p;
	int size, n;

	size = 1024;
	while (1) {
		if ((b = malloc(size)) == NULL)
			return 0;

		va_start(args, format);
		n = vsnprintf(b, size, format, args);
		va_end(args);

		if (n > -1) {
			if (n < size) {
				switch (wof) {
				case WOF_JAVASCRIPT:
					p = js_string(b);
					free(b);
					break;
				case WOF_HTML:
					p = html_string(b);
					free(b);
					break;
				default:
					p = b;
					break;
				}
				if (!p) return 0;
				web_puts(p);
				free(p);
				return 1;
			}
			size = n + 1;
		}
		else
			size *= 2;

		free(b);
		if (size > (10 * 1024))
			return 0;
	}
}

int web_write(const char *buffer, int len)
{
	size_t n;
	size_t r;

	/* nothing to write; also protects the cast to size_t below */
	if (len <= 0)
		return 0;

	n = (size_t)len;

	while (n > 0) {
		/*
		 * clear errno before fwrite(), so a stale errno value from an
		 * earlier call is not mistaken for the cause of this result
		 */
		errno = 0;

		r = fwrite(buffer, 1, n, connfp);

		if (r > 0) {
			/* partial writes are valid; continue until all bytes are written */
			buffer += r;
			n -= r;
			continue;
		}

		/*
		 * fwrite() returns 0 on failure here. If the operation was
		 * interrupted by a signal, clear the stream error flag and retry
		 */
		if (ferror(connfp) && errno == EINTR) {
			clearerr(connfp);
			continue;
		}

		/* real write error, or an unexpected zero-byte write */
		return -1;
	}

	/* return the total number of bytes requested */
	return len;
}

int web_read(void *buffer, int len)
{
	size_t r;

	/* nothing to read; also protects the cast to size_t below */
	if (len <= 0)
		return 0;

	for (;;) {
		/*
		 * clear errno before fread(), so a stale errno value from an
		 * earlier call is not mistaken for the cause of this result
		 */
		errno = 0;

		r = fread(buffer, 1, (size_t)len, connfp);

		if (r > 0)
			return (int)r;

		/* EOF is a clean connection close / end of input */
		if (feof(connfp))
			return 0;

		/*
		 * if the read was interrupted by a signal, clear the stream
		 * error flag and retry the same read
		 */
		if (ferror(connfp) && errno == EINTR) {
			clearerr(connfp);
			continue;
		}

		/* real read error, or an unexpected zero-byte read without EOF */
		return -1;
	}
}

int web_read_x(void *buffer, int len)
{
	int n, total = 0;

	while (total < len) {
		n = web_read((char *)buffer + total, len - total);

		if (n <= 0)
			return -1; /* error or EOF */

		total += n;
	}

	return total; /* == len on success */
}

int web_eat(int max)
{
	char buf[512];
	int n;

	while (max > 0) {
		if ((n = web_read(buf, ((unsigned int) max < sizeof(buf)) ? (unsigned int) max : sizeof(buf))) <= 0)
			return 0;

		max -= n;
	}

	return 1;
}

int web_flush(void)
{
	return (fflush(connfp) == 0);
}

int web_open(void)
{
	if (do_ssl) {
#ifdef TCONFIG_HTTPS
		if ((connfp = ssl_server_fopen(connfd)) != NULL)
			return 1;
#endif
	}
	else
		if ((connfp = fdopen(connfd, "r+")) != NULL)
			return 1;

	return 0;
}

int web_close(void)
{
	if (connfp != NULL) {
		fflush(connfp);
		fclose(connfp);
		connfp = NULL;
	}
	if (connfd != -1) {
		//shutdown(connfd, SHUT_RDWR);
		close(connfd);
		connfd = -1;
	}

	return 1;
}

static void _web_putfile(FILE *f, wofilter_t wof)
{
	char buf[2048];
	int nr;
	int total = 0;
	int max = 10 * 1024 * 1024; /* 10MB limit */

	while ((nr = fread(buf, 1, sizeof(buf) - 1, f)) > 0) {
		/* enforce output limit */
		if (total + nr > max)
			nr = max - total;

		buf[nr] = 0;

		switch (wof) {
		case WOF_JAVASCRIPT:
			web_putj_utf8(buf);
			break;
		case WOF_HTML:
			web_puth_utf8(buf);
			break;
		default:
			web_puts(buf);
			break;
		}

		total += nr;

		if (total >= max)
			break;
	}
}

int web_putfile(const char *fname, wofilter_t wof)
{
	FILE *f;

	if ((f = fopen(fname, "r")) == NULL)
		return 0;

	_web_putfile(f, wof);
	fclose(f);

	return 1;
}

int web_pipecmd(const char *cmd, wofilter_t wof)
{
	FILE *f;

	if ((f = popen(cmd, "r")) == NULL)
		return 0;

	_web_putfile(f, wof);
	pclose(f);

	return 1;
}
