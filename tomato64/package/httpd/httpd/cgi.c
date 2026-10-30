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
 * Fixes/updates (C) 2018 - 2026 pedro
 * https://freshtomato.org/
 *
 */


#ifndef _GNU_SOURCE
 #define _GNU_SOURCE
#endif

#include "tomato.h"

#if defined(__GLIBC__) || defined(__UCLIBC__)
 #define __USE_GNU
#else
 #define _GNU_SOURCE    //musl
#endif	/* ! (__GLIBC__ || __UCLIBC__) */
#include <search.h>
#include <ctype.h>

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
			if ((strlen(s + 1) >= 2) && isxdigit((unsigned char)s[1]) && isxdigit((unsigned char)s[2]) && (sscanf(s + 1, "%02x", &c) == 1)) {
				*s++ = (char)c;
				strlcpy(s, s + 2, strlen(s) + 1);
			}
			else {
				/* malformed percent escape - discard the invalid suffix */
				strlcpy(s, "", strlen(s) + 1);
				logmsg(LOG_DEBUG, "*** [cgi] %s: malformed substring (skipped)!", __FUNCTION__);
			}
		}
		else if (*s == '+')
			*s++ = ' ';
	}
}

int str_replace(char *str, size_t str_size, const char *str_src, const char *str_des)
{
	char *ptr;
	size_t len, src_len, des_len, tail_len;

	if (!str || !str_size || !str_src || !*str_src || !str_des)
		return -1;

	len = strnlen(str, str_size);
	if (len == str_size)
		return -1;

	src_len = strlen(str_src);
	des_len = strlen(str_des);

	ptr = str;
	while ((ptr = strstr(ptr, str_src)) != NULL) {
		size_t off = (size_t)(ptr - str);

		if ((des_len > src_len) &&
		    (len > SIZE_MAX - (des_len - src_len) ||
		     len + (des_len - src_len) >= str_size))
			return -1;

		tail_len = len - off - src_len;

		if (des_len != src_len)
			memmove(ptr + des_len, ptr + src_len, tail_len + 1);

		memcpy(ptr, str_des, des_len);
		len = len - src_len + des_len;
		ptr += des_len;
	}

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
		str_replace(value, strlen(value) + 1, "%u", "~u");
		unescape(value);
		name = strsep(&value, "=");

		if (value)
			webcgi_set(name, value);
	}
}
