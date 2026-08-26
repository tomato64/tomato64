/*
 * Wireless network adapter utilities (linux-specific)
 *
 * Copyright (C) 2014, Broadcom Corporation. All Rights Reserved.
 * 
 * Permission to use, copy, modify, and/or distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY
 * SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN ACTION
 * OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN
 * CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 *
 * $Id: wl_linux.c 435840 2013-11-12 09:30:26Z $
 */

/*
 * Wireless network adapter utilities (linux-specific)
 *
 * Copyright 2005, Broadcom Corporation
 * All Rights Reserved.
 * 
 * THIS SOFTWARE IS OFFERED "AS IS", AND BROADCOM GRANTS NO WARRANTIES OF ANY
 * KIND, EXPRESS OR IMPLIED, BY STATUTE, COMMUNICATION OR OTHERWISE. BROADCOM
 * SPECIFICALLY DISCLAIMS ANY IMPLIED WARRANTIES OF MERCHANTABILITY, FITNESS
 * FOR A SPECIFIC PURPOSE OR NONINFRINGEMENT CONCERNING THIS SOFTWARE.
 *
 * $Id: wl_linux.c,v 1.1.1.8 2005/03/07 07:31:20 kanki Exp $
 */

#ifdef TOMATO64
#include <typedefs.h>
#endif /* TOMATO64 */
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <sys/ioctl.h>
#ifdef TCONFIG_BCMARM
#include <sys/socket.h>
#endif
#include <net/if.h>
#ifdef TCONFIG_BCMARM
#include <linux/types.h>
#endif

typedef u_int64_t u64;
typedef u_int32_t u32;
typedef u_int16_t u16;
typedef u_int8_t u8;
#ifndef TCONFIG_BCMARM
#include <linux/types.h>
#endif
#include <linux/sockios.h>
#include <linux/ethtool.h>

#include <typedefs.h>
#include <wlioctl.h>
#include <wlutils.h>
#include <syslog.h>

#include "shared.h"

// xref: nas,wlconf,httpd,rc,
int wl_ioctl(char *name, int cmd, void *buf, int len)
{
	struct ifreq ifr;
	wl_ioctl_t ioc;
	int ret = 0;
	int s;
#ifdef TCONFIG_BCMARM
	char buffer[100];
#endif

	/* open socket to kernel */
	if ((s = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
		logerr(__FUNCTION__, __LINE__, "socket");
		return errno;
	}

	/* do it */
	ioc.cmd = cmd;
	ioc.buf = buf;
	ioc.len = len;

	/* initializing the remaining fields */
	ioc.set = FALSE;
	ioc.used = 0;
	ioc.needed = 0;

	strlcpy(ifr.ifr_name, name, IFNAMSIZ);
	ifr.ifr_data = (caddr_t) &ioc;
#ifdef TCONFIG_BCMARM
	if ((ret = ioctl(s, SIOCDEVPRIVATE, &ifr)) < 0)
		if (cmd != WLC_GET_MAGIC && cmd != WLC_GET_BSSID) {
			if ((cmd == WLC_GET_VAR) || (cmd == WLC_SET_VAR)) {
				snprintf(buffer, sizeof(buffer), "%s: WLC_%s_VAR(%s)", name,
				         cmd == WLC_GET_VAR ? "GET" : "SET", (char *)buf);
			} else {
				snprintf(buffer, sizeof(buffer), "%s: cmd=%d", name, cmd);
			}
			logerr(__FUNCTION__, __LINE__, buffer);
		}
#else
	ret = ioctl(s, SIOCDEVPRIVATE, &ifr);
/*
	if ((ret = ioctl(s, SIOCDEVPRIVATE, &ifr)) < 0)
		if (cmd != WLC_GET_MAGIC)
			logerr(__FUNCTION__, __LINE__, ifr.ifr_name);
*/
#endif

	/* cleanup */
	close(s);
	return ret;
}

// xref: wl.c:wl_probe()
int wl_get_dev_type(char *name, void *buf, int len)
{
	int s;
	int ret;
	struct ifreq ifr;
	struct ethtool_drvinfo info;

	/* open socket to kernel */
	if ((s = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
		logerr(__FUNCTION__, __LINE__, "socket");
		return -1;
	}

	/* get device type */
	memset(&info, 0, sizeof(info));
	info.cmd = ETHTOOL_GDRVINFO;
	ifr.ifr_data = (caddr_t)&info;
	strlcpy(ifr.ifr_name, name, IFNAMSIZ);
	if ((ret = ioctl(s, SIOCETHTOOL, &ifr)) < 0) {
		*(char *)buf = '\0';
	}
	else {
		strlcpy(buf, info.driver, len);
	}

	close(s);
	return ret;
}

// xref: nas,wlconf,
int wl_hwaddr(char *name, unsigned char *hwaddr)
{
	struct ifreq ifr;
	int ret = 0;
	int s;

	/* open socket to kernel */
	if ((s = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
		logerr(__FUNCTION__, __LINE__, "socket");
		return errno;
	}

	/* do it */
	strlcpy(ifr.ifr_name, name, IFNAMSIZ);
	if ((ret = ioctl(s, SIOCGIFHWADDR, &ifr)) == 0)
		memcpy(hwaddr, ifr.ifr_hwaddr.sa_data, ETHER_ADDR_LEN);

	/* cleanup */
	close(s);
	return ret;
}
