/*
 * CGI helper functions
 *
 * Copyright 2005, Broadcom Corporation
 * All Rights Reserved.
 *
 * THIS SOFTWARE IS OFFERED "AS IS", AND BROADCOM GRANTS NO WARRANTIES OF ANY
 * KIND, EXPRESS OR IMPLIED, BY STATUTE, COMMUNICATION OR OTHERWISE. BROADCOM
 * SPECIFICALLY DISCLAIMS ANY IMPLIED WARRANTIES OF MERCHANTABILITY, FITNESS
 * FOR A SPECIFIC PURPOSE OR NONINFRINGEMENT CONCERNING THIS SOFTWARE.
 *
 * $Id: cgi.c,v 1.10 2005/03/07 08:35:32 kanki Exp $
 *
 * Fixes/updates (C) 2018 - 2025 pedro
 * https://freshtomato.org/
 *
 */


#include "tomato.h"

#if defined(__GLIBC__) || defined(__UCLIBC__)
 #define __USE_GNU
#else
 #define _GNU_SOURCE    //musl
#endif	/* ! (__GLIBC__ || __UCLIBC__) */
#include <search.h>

/* needed by logmsg() */
#define LOGMSG_DISABLE	DISABLE_SYSLOG_OS
#define LOGMSG_NVDEBUG	"cgi_debug"


/* CGI hash table */
#if defined(__GLIBC__) || defined(__UCLIBC__)
static struct hsearch_data htab = { .table = NULL };
#else
static struct hsearch_data htab;
#endif	/* ! (__GLIBC__ || __UCLIBC__) */

static void unescape(char *s)
{
	unsigned int c;

	while ((s = strpbrk(s, "%+"))) {
		if (*s == '%') {
			if ((strlen(s + 1)) >= 2) {
				sscanf(s + 1, "%02x", &c);
				*s++ = (char)c;
				strlcpy(s, s + 2, strlen(s) + 1);
			}
			else {
				/* something's wrong - skip... */
				strlcpy(s, "", strlen(s) + 1);
				logmsg(LOG_DEBUG, "*** [cgi] %s: malformed substring (skipped)!", __FUNCTION__);
			}
		}
		else if (*s == '+')
			*s++ = ' ';
	}
}

int str_replace(char* str, char* str_src, char* str_des)
{
	char *ptr = NULL;
	char buff[10240], buff2[10240];
	int i = 0;

	if (str != NULL)
		strlcpy(buff2, str, sizeof(buff2));
	else {
		logmsg(LOG_DEBUG, "*** [cgi] %s: error - NULL string!", __FUNCTION__);
		return -1;
	}

	memset(buff, 0x00, sizeof(buff));
	while ((ptr = strstr(buff2, str_src)) != 0) {
		if (ptr - buff2 != 0)
			memcpy(&buff[i], buff2, ptr - buff2);

		memcpy(&buff[i + ptr - buff2], str_des, strlen(str_des));
		i += ptr - buff2 + strlen(str_des);
		strlcpy(buff2, ptr + strlen(str_src), sizeof(buff2));
	}

	strlcat(buff, buff2, sizeof(buff));
	strlcpy(str, buff, strlen(str) + 1);

	return 0;
}

char *webcgi_get(const char *name)
{
	ENTRY e, *ep;

#if !(defined(__GLIBC__) || defined(__UBLIBC__))
	if (!htab.__tab)
#else
	if (!htab.table)
#endif
		return NULL;

	e.key = (char *)name;
	hsearch_r(e, FIND, &ep, &htab);

	logmsg(LOG_DEBUG, "*** [cgi] %s: %s=%s", __FUNCTION__, name, ep ? (char*)ep->data : "(null)");

	return ep ? ep->data : NULL;
}

void webcgi_set(char *name, char *value)
{
	ENTRY e, *ep;

#if !(defined(__GLIBC__) || defined(__UBLIBC__))
	if (!htab.__tab)
#else
	if (!htab.table)
#endif
		hcreate_r(16, &htab);

	e.key = name;
	hsearch_r(e, FIND, &ep, &htab);
	if (ep)
		ep->data = value;
	else {
		e.data = value;
		hsearch_r(e, ENTER, &ep, &htab);
	}
}

void webcgi_init(char *query)
{
	int nel;
	char *q, *end, *name, *value;

#if !(defined(__GLIBC__) || defined(__UBLIBC__))
	if (htab.__tab)
#else
	if (htab.table)
#endif
		hdestroy_r(&htab);

	if (query == NULL)
		return;

	logmsg(LOG_DEBUG, "*** [cgi] %s: query = %s", __FUNCTION__, query);

	end = query + strlen(query);
	q = query;
	nel = 1;
	while (strsep(&q, "&;")) {
		nel++;
	}
	hcreate_r(nel, &htab);

	for (q = query; q < end;) {
		value = q;
		q += strlen(q) + 1;
		str_replace(value, "%u", "~u");
		unescape(value);
		name = strsep(&value, "=");

		if (value)
			webcgi_set(name, value);
	}
}
