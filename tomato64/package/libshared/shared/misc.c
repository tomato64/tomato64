/*
 *
 * Tomato Firmware
 * Copyright (C) 2006-2009 Jonathan Zarate
 *
 */


#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <stdarg.h>
#include <syslog.h>
#include <sys/ioctl.h>
#include <net/if.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <ifaddrs.h>
#include <sys/sysinfo.h>
#include <sys/types.h>
#include <sys/wait.h>

#include <bcmnvram.h>
#include <bcmdevs.h>
#include <wlutils.h>

#include "shutils.h"
#include "shared.h"

/* needed by logmsg() */
#define LOGMSG_DISABLE	DISABLE_SYSLOG_OS
#define LOGMSG_NVDEBUG	"misc_debug"


void get_wan_prefix(int iWan_unit, char *sPrefix)
{
	if (iWan_unit == 1)
		strcpy(sPrefix, "wan");
	else if (iWan_unit == 2)
		strcpy(sPrefix, "wan2");
#ifdef TCONFIG_MULTIWAN
	else if (iWan_unit == 3)
		strcpy(sPrefix, "wan3");
	else if (iWan_unit == 4)
		strcpy(sPrefix, "wan4");
#endif
	else
		strcpy(sPrefix, "wan");
}

int get_wan_unit(const char *sPrefix)
{
	if (!strcmp(sPrefix, "wan"))
		return 1;
	else if (!strcmp(sPrefix, "wan2"))
		return 2;
#ifdef TCONFIG_MULTIWAN
	else if (!strcmp(sPrefix, "wan3"))
		return 3;
	else if (!strcmp(sPrefix, "wan4"))
		return 4;
#endif
	else
		return 1;
}

int get_wan_unit_with_value(const char *suffix, const char *value)
{
	char tmp[64];
	int mwan_num, wan_unit;

	mwan_num = nvram_get_int("mwan_num");
	if ((mwan_num < 1) || (mwan_num > MWAN_MAX))
		mwan_num = 1;

	for (wan_unit = 1; wan_unit <= mwan_num; ++wan_unit) {
		get_wan_prefix(wan_unit, tmp);
		strlcat(tmp, suffix, sizeof(tmp));

		if (nvram_match(tmp, value))
			return wan_unit;
	}

	return -1;
}

int get_wan_proto(void)
{
	return get_wanx_proto("wan");
}

int get_wanx_proto(char *prefix)
{
	char tmp[100];
	const char *names[] = {	/* order must be synced with def at shared.h */
		"static",
		"dhcp",
		"l2tp",
		"pppoe",
		"pptp",
		"ppp3g",
		"lte",
		NULL
	};
	int i;
	const char *p;

	p = nvram_safe_get(strlcat_r(prefix, "_proto", tmp, sizeof(tmp)));
	for (i = 0; names[i] != NULL; ++i) {
		if (strcmp(p, names[i]) == 0)
			return i + 1;
	}

	return WP_DISABLED;
}

#ifdef TCONFIG_IPV6
int get_ipv6_service(void)
{
	const char *names[] = {	/* order must be synced with def at shared.h */
		"native",	/* IPV6_NATIVE */
		"native-pd",	/* IPV6_NATIVE_DHCP */
		"6to4",		/* IPV6_ANYCAST_6TO4 */
		"sit",		/* IPV6_6IN4 */
		"other",	/* IPV6_MANUAL */
		"6rd",		/* IPV6_6RD */
		"6rd-pd",	/* IPV6_6RD_DHCP */
		NULL
	};
	int i;
	const char *p;

	p = nvram_safe_get("ipv6_service");
	for (i = 0; names[i] != NULL; ++i) {
		if (strcmp(p, names[i]) == 0)
			return i + 1;
	}

	return IPV6_DISABLED;
}

const char *ipv6_router_address(struct in6_addr *in6addr)
{
	char *p;
	struct in6_addr addr;
	static char addr6[INET6_ADDRSTRLEN];

	addr6[0] = '\0';

	if ((p = nvram_get("ipv6_rtr_addr")) && *p) {
		inet_pton(AF_INET6, p, &addr);
	}
	else if ((p = nvram_get("ipv6_prefix")) && *p) {
		inet_pton(AF_INET6, p, &addr);
		addr.s6_addr16[7] = htons(0x0001);
	}
	else {
		return addr6;
	}

	inet_ntop(AF_INET6, &addr, addr6, sizeof(addr6));
	if (in6addr)
		memcpy(in6addr, &addr, sizeof(addr));

	return addr6;
}

/* trim useless 0 from IPv6 address */
const char *ipv6_address(const char *ipaddr6)
{
	struct in6_addr addr;
	static char addr6[INET6_ADDRSTRLEN];

	addr6[0] = '\0';

	if (inet_pton(AF_INET6, ipaddr6, &addr) > 0)
		inet_ntop(AF_INET6, &addr, addr6, sizeof(addr6));

	return addr6;
}

/* extract prefix from configured IPv6 address */
const char *ipv6_prefix(struct in6_addr *in6addr)
{
	static char prefix[INET6_ADDRSTRLEN];
	struct in6_addr addr;
	int i, r;

	prefix[0] = '\0';
	memset(&addr, 0, sizeof(addr));

	if (inet_pton(AF_INET6, nvram_safe_get("ipv6_rtr_addr"), &addr) > 0) {
		r = nvram_get_int("ipv6_prefix_length") ? : 64;
		for (r = 128 - r, i = 15; r > 0; r -= 8) {
			if (r >= 8) {
				addr.s6_addr[i--] = 0;
			}
			else {
				addr.s6_addr[i--] &= (0xff << r);
			}
		}
		inet_ntop(AF_INET6, &addr, prefix, sizeof(prefix));
	}

	if (in6addr)
		memcpy(in6addr, &addr, sizeof(addr));

	return prefix;
}

int calc_6rd_local_prefix(const struct in6_addr *prefix,
	int prefix_len, int relay_prefix_len,
	const struct in_addr *local_ip,
	struct in6_addr *local_prefix, int *local_prefix_len)
{
	/* the following code is based on ipv6calc's code */
	uint32_t local_ip_bits, j;
	int i;

	if (!prefix || !local_ip || !local_prefix || !local_prefix_len) {
		return 0;
	}

	*local_prefix_len = prefix_len + 32 - relay_prefix_len;
	if (*local_prefix_len > 64) {
		return 0;
	}

	local_ip_bits = ntohl(local_ip->s_addr) << relay_prefix_len;

	for (i = 0; i < 4; i++) {
		local_prefix->s6_addr32[i] = prefix->s6_addr32[i];
	}

	for (j = 0x80000000, i = prefix_len; i < *local_prefix_len; i++, j >>= 1)
	{
		if (local_ip_bits & j)
			local_prefix->s6_addr[i >> 3] |= (0x80 >> (i & 0x7));
	}

	return 1;
}
#endif

int using_dhcpc(char *prefix)
{
	char tmp[100];
	switch (get_wanx_proto(prefix)) {
		case WP_DHCP:
		case WP_LTE:
			return 1;
		case WP_L2TP:
		case WP_PPTP:
		case WP_PPPOE: /* PPPoE with MAN */
			return nvram_get_int(strlcat_r(prefix, "_pptp_dhcp", tmp, sizeof(tmp)));
	}

	return 0;
}

#ifdef TCONFIG_BCMWL6
int is_psta_client(int unit, int subunit)
{
	char *mode;
	int ret = 0;

	if (unit < 0)
		return ret;

	mode = nvram_safe_get(wl_nvname("mode", unit, subunit));

	if (strcmp(mode, "psta") == 0)
		ret = 1;

	return ret;
}
#endif /* TCONFIG_BCMWL6 */

int wl_client(int unit, int subunit)
{
	char *mode = nvram_safe_get(wl_nvname("mode", unit, subunit));

	return ((strcmp(mode, "sta") == 0) || (strcmp(mode, "wet") == 0)
#ifdef TCONFIG_BCMWL6
		|| (strcmp(mode, "psta") == 0)
#endif /* TCONFIG_BCMWL6 */
		);
}

int foreach_wif(int include_vifs, void *param,
	int (*func)(int idx, int unit, int subunit, void *param))
{
	char ifnames[256];
	char name[64], ifname[64], *next = NULL;
	int unit = -1, subunit = -1;
	int i;
	int ret = 0;

#ifdef TCONFIG_MULTIWAN
#ifdef TCONFIG_AC3200
	snprintf(ifnames, sizeof(ifnames), "%s %s %s %s %s %s %s %s %s %s %s %s %s %s %s",
#else
	snprintf(ifnames, sizeof(ifnames), "%s %s %s %s %s %s %s %s %s %s %s %s %s",
#endif /* TCONFIG_AC3200 */
#else
#ifdef TCONFIG_AC3200
	snprintf(ifnames, sizeof(ifnames), "%s %s %s %s %s %s %s %s %s %s %s %s %s",
#else
	snprintf(ifnames, sizeof(ifnames), "%s %s %s %s %s %s %s %s %s %s %s",
#endif /* TCONFIG_AC3200 */
#endif /* TCONFIG_MULTIWAN */
		nvram_safe_get("lan_ifnames"),
		nvram_safe_get("lan1_ifnames"),
		nvram_safe_get("lan2_ifnames"),
		nvram_safe_get("lan3_ifnames"),
		nvram_safe_get("wan_ifnames"),
		nvram_safe_get("wan2_ifnames"),
#ifdef TCONFIG_MULTIWAN
		nvram_safe_get("wan3_ifnames"),
		nvram_safe_get("wan4_ifnames"),
#endif /* TCONFIG_MULTIWAN */
#ifdef TCONFIG_AC3200
		nvram_safe_get("wl2_ifname"),
		nvram_safe_get("wl2_vifs"),
#endif /* TCONFIG_AC3200 */
		nvram_safe_get("wl_ifname"),
		nvram_safe_get("wl0_ifname"),
		nvram_safe_get("wl0_vifs"),
		nvram_safe_get("wl1_ifname"),
		nvram_safe_get("wl1_vifs"));

	remove_dups(ifnames, sizeof(ifnames));
	sort_list(ifnames, sizeof(ifnames));

	i = 0;
	foreach(name, ifnames, next) {
		if (nvifname_to_osifname(name, ifname, sizeof(ifname)) != 0)
			continue;

		if (wl_probe(ifname) || wl_ioctl(ifname, WLC_GET_INSTANCE, &unit, sizeof(unit)))
			continue;

		/* Convert eth name to wl name */
		if (osifname_to_nvifname(name, ifname, sizeof(ifname)) != 0)
			continue;

		/* Slave intefaces have a '.' in the name */
		if (strchr(ifname, '.') && !include_vifs)
			continue;

		if (get_ifname_unit(ifname, &unit, &subunit) < 0)
			continue;

		ret |= func(i++, unit, subunit, param);
	}
	return ret;
}

void notice_set(const char *path, const char *format, ...)
{
	char p[256];
	char buf[2048];
	va_list args;

	va_start(args, format);
	vsnprintf(buf, sizeof(buf), format, args);
	va_end(args);

	mkdir("/var/notice", 0755);
	snprintf(p, sizeof(p), "/var/notice/%s", path);
	f_write_string(p, buf, 0, 0);

	if (buf[0])
		logmsg(LOG_INFO, "notice[%s]: %s", path, buf);
}

int wan_led(int mode) /* mode: 0 - OFF, 1 - ON */
{
	int model;

	logmsg(LOG_DEBUG, "*** %s: wan_led: led(LED_WHITE,%s)", __FUNCTION__, (mode ? "ON" : "OFF"));

	/* get router model */
	model = get_model();

	/* check router model according to shared/led.c table, LED WHITE */
	if ((model == MODEL_RTN18U)
	    || (model == MODEL_R7000)
	    || (model == MODEL_R6400)
	    || (model == MODEL_R6400v2)
	    || (model == MODEL_R6700v1)
	    || (model == MODEL_R6700v3)
	    || (model == MODEL_R6900)
	    || (model == MODEL_XR300)
	    || (model == MODEL_RTAC67U)
	    || (model == MODEL_DSLAC68U)
	    || (model == MODEL_RTAC68U)
	    || (model == MODEL_RTAC68UV3)
	    || (model == MODEL_RTAC66U_B1)
	    || (model == MODEL_RTAC1900P)
	    || (model == MODEL_RTAC56U)
	    || (model == MODEL_DIR868L)
	    || (model == MODEL_F9K1113v2_20X0)
	    || (model == MODEL_F9K1113v2)
	    || (model == MODEL_WS880)
	    || (model == MODEL_R6200v2)
	    || (model == MODEL_R6250)
	    || (model == MODEL_AC1450)
	    || (model == MODEL_R6300v2)
	    || (model == MODEL_EA6350v1)
	    || (model == MODEL_EA6350v2)
	    || (model == MODEL_EA6400)
	    || (model == MODEL_EA6700)
	    || (model == MODEL_EA6900)
	    || (model == MODEL_R1D)
	    || (model == MODEL_WZR1750)
#ifdef TCONFIG_BCM714
	    || (model == MODEL_RTAC3100)
	    || (model == MODEL_RTAC88U)
#endif
#ifdef TCONFIG_AC3200
#ifdef TCONFIG_AC5300
	    || (model == MODEL_RTAC5300)
#endif
	    || (model == MODEL_RTAC3200)
	    || (model == MODEL_R8000)
#endif
	) {
		led(LED_WHITE, mode);
	}

	return mode;
}

int wan_led_off(char *prefix) /* off WAN LED only if no other WAN active */
{
	char tmp[100];
	char ppplink_file[32];
	char *names[] = { /* FIXME: hardcoded to 4 WANs */
		"wan",
		"wan2",
#ifdef TCONFIG_MULTIWAN
		"wan3",
		"wan4",
#endif
		NULL
	};
	int i;
	int f;
	struct ifreq ifr;
	int up;
	int count = 0; /* initialize with zero */
	int proto;
	int mwan_num = atoi(nvram_safe_get("mwan_num"));
	if (mwan_num < 1 || mwan_num > MWAN_MAX) {
		mwan_num = 1;
	}

	for (i = 0; (names[i] != NULL) && (i <= mwan_num-1); ++i) {
		up = 0; /* default is 0 (LED_OFF) */
		if (!strcmp(prefix, names[i]))
			continue; /* only check others */

		logmsg(LOG_DEBUG, "*** %s: check %s aliveness...", __FUNCTION__, names[i]);

		switch (proto = get_wanx_proto(names[i])) {
			case WP_DISABLED:
				break; /* WAN is disabled - skip */
			case WP_STATIC:
			case WP_DHCP:
			case WP_LTE:
				if (!nvram_match(strlcat_r(names[i], "_ipaddr", tmp, sizeof(tmp)), "0.0.0.0")) { /* have IP, assume ON */
					up = 1;
					if (((f = socket(AF_INET, SOCK_DGRAM, 0)) >= 0)) { /* check interface */
						strlcpy(ifr.ifr_name, nvram_safe_get(strlcat_r(names[i], "_iface", tmp, sizeof(tmp))), sizeof(ifr.ifr_name));
						if (ioctl(f, SIOCGIFFLAGS, &ifr) < 0)
							up = 0;
						close(f);
						if ((ifr.ifr_flags & IFF_UP) == 0 || (ifr.ifr_flags & IFF_RUNNING) == 0)
							up = 0;
						if (proto == WP_STATIC) { /* check port state for static */

							int a;		/* port number: 0/1/2/3/4 */
							char b[16];	/* port state: DOWN/SPEED */
							int c;		/* port vlan: 1/2/3/4/etc */
							char d[4];
							FILE *f = NULL;

							strcpy(d, &ifr.ifr_name[4]); /* trim vlan */
							int vlannum = atoi(d);
							if ((f = popen("/usr/sbin/robocfg showports", "r")) != NULL) {
								while (fgets(tmp, sizeof(tmp), f)) {
									if (sscanf(tmp, "Port %d: %s %*s %*s %*s vlan: %d %*s", &a, b, &c) == 3) {
										if ((strncmp(b, "DOWN", 4) == 0) && (c == vlannum)) {
											logmsg(LOG_DEBUG, "*** %s: state = DOWN for vlan%d", __FUNCTION__, vlannum);
											up = 0;
										}
									}
								}
								pclose(f);
							}
						}
					}
				}
				if (up)
					++count;
				break;
			case WP_L2TP:
			case WP_PPTP:
			case WP_PPPOE:
			case WP_PPP3G:
				memset(ppplink_file , 0, sizeof(ppplink_file));
				FILE *f_tmp = NULL;
				snprintf(ppplink_file, sizeof(ppplink_file), "/tmp/ppp/%s_link", names[i]);
				if ((f_tmp = fopen(ppplink_file, "r")) != NULL) { /* have PPP link, assume ON */
					up = 1;
					fclose(f_tmp);
				}
				if (up)
					++count;
				break;
			default:
				break;
		}
	}

	if (count > 0) {
		logmsg(LOG_DEBUG, "*** %s: OUT - %s, active WANs count:%d, stay on", __FUNCTION__, prefix, count);
		return count; /* do not LED OFF */
	}
	else {
		logmsg(LOG_DEBUG, "*** %s: OUT - %s, no other active WANs, turn off led", __FUNCTION__, prefix);
		return wan_led(LED_OFF); /* LED OFF */
	}
}

/* function for rstats, cstats, httpd */
long check_wanup_time(char *prefix)
{
	long wanuptime = 0; /* wanX uptime in seconds */
	struct sysinfo si;
	long uptime;
	char wanuptime_file[128];

	sysinfo(&si); /* get time */
	memset(wanuptime_file, 0, sizeof(wanuptime_file)); /* reset */
	snprintf(wanuptime_file, sizeof(wanuptime_file), "/var/lib/misc/%s_time", prefix);

	if (f_read(wanuptime_file, &uptime, sizeof(uptime)) == sizeof(uptime)) {
		wanuptime = si.uptime - uptime; /* calculate the difference */
		if (wanuptime < 0) /* something wrong? */
			wanuptime = 0;
	}
	else {
		wanuptime = 0; /* something wrong? f_read()? */
	}

	return wanuptime;
}

int check_wanup(char *prefix)
{
	FILE *f;
	int up = 0;
	int proto;
	char buf1[64];
	char buf2[64];
	const char *name;
	int s;
	struct ifreq ifr;
	char tmp[100];
	char ppplink_file[256];
	char pppd_name[256];

	proto = get_wanx_proto(prefix);

	if (proto == WP_DISABLED) {
		return 0;
	}

	if ((proto == WP_PPTP) || (proto == WP_L2TP) || (proto == WP_PPPOE) || (proto == WP_PPP3G)) {
		memset(ppplink_file, 0, sizeof(ppplink_file));
		snprintf(ppplink_file, sizeof(ppplink_file), "/tmp/ppp/%s_link", prefix);

		if (f_read_string(ppplink_file, buf1, sizeof(buf1)) > 0) {
			/* contains the base name of a file in /var/run/ containing pid of a daemon */
			snprintf(buf2, sizeof(buf2), "/var/run/%s.pid", buf1);

			if (f_read_string(buf2, buf1, sizeof(buf1)) > 0) {
				name = psname(atoi(buf1), buf2, sizeof(buf2));

				memset(pppd_name, 0, sizeof(pppd_name));
				snprintf(pppd_name, sizeof(pppd_name), "pppd%s", prefix);
				logmsg(LOG_DEBUG, "*** %s: pppd name=%s, psname=%s", __FUNCTION__, pppd_name, name);

				if (strcmp(name, pppd_name) == 0)
					up = 1;

				if (proto == WP_L2TP) {
					snprintf(pppd_name, sizeof(pppd_name), "pppd");
					logmsg(LOG_DEBUG, "*** %s: L2TP pppd name=%s, psname=%s", __FUNCTION__, pppd_name, name);

					if (strcmp(name, pppd_name) == 0)
						up = 1;
				}
			}
			else {
				logmsg(LOG_DEBUG, "*** %s: error reading %s", __FUNCTION__, buf2);
			}
			if (!up) {
				unlink(ppplink_file); /* stale PPP connection fix, also used in wan_led_off */
				logmsg(LOG_DEBUG, "*** %s: required daemon not found, assuming link is dead", __FUNCTION__);
			}
		}
		else {
			logmsg(LOG_DEBUG, "*** %s: error reading %s", __FUNCTION__, ppplink_file);
		}
	}
	else if (!nvram_match(strlcat_r(prefix, "_ipaddr", tmp, sizeof(tmp)), "0.0.0.0")) {
		logmsg(LOG_DEBUG, "*** %s: %s have IP, assume ON", __FUNCTION__, prefix);
		up = 1;
	}
	else {
		logmsg(LOG_DEBUG, "*** %s: default !up", __FUNCTION__);
		goto state; /* don't turn off WAN LED */
	}

	if ((up) && ((s = socket(AF_INET, SOCK_DGRAM, 0)) >= 0)) {
		strlcpy(ifr.ifr_name, nvram_safe_get(strlcat_r(prefix, "_iface", tmp, sizeof(tmp))), sizeof(ifr.ifr_name));

		if (ioctl(s, SIOCGIFFLAGS, &ifr) < 0) {
			up = 0;
			logmsg(LOG_DEBUG, "*** %s: SIOCGIFFLAGS", __FUNCTION__);
		}
		close(s);

		if ((ifr.ifr_flags & IFF_UP) == 0 || (ifr.ifr_flags & IFF_RUNNING) == 0) {
			up = 0;
			logmsg(LOG_DEBUG, "*** %s: !IFF_UP || !IFF_RUNNING", __FUNCTION__);
		}

		if (proto == WP_STATIC) {
			/* Ethernet WAN port state checker (static IF is always UP and RUNNING)
			   ifr.ifr_name = vlan2, vlan3 etc
			   nvram get vlan2ports
			   4 5
			   nvram get vlan3ports
			   1 5
			   robocfg showports
			   Switch: enabled
			   Port 0: 1000FD enabled stp: none vlan: 1 jumbo: off mac: 00:00:80:00:00:00
			   Port 1:   DOWN enabled stp: none vlan: 3 jumbo: off mac: 00:00:00:00:00:00
			   Port 2:   DOWN enabled stp: none vlan: 1 jumbo: off mac: 00:00:00:00:00:00
			   Port 3:   DOWN enabled stp: none vlan: 1 jumbo: off mac: 00:00:00:00:00:00
			   Port 4:  100FD enabled stp: none vlan: 2 jumbo: off mac: 00:00:00:00:00:00
			   Port 8:   DOWN enabled stp: none vlan: 1 jumbo: off mac: 00:00:00:00:00:00
			*/

			int a;		/* port number: 0/1/2/3/4 */
			char b[16];	/* port state: DOWN/SPEED */
			int c;		/* port vlan: 1/2/3/4/etc */
			char d[4];
			FILE *f;

			strcpy(d, &ifr.ifr_name[4]); /* trim vlan */
			int vlannum = atoi(d);
			logmsg(LOG_DEBUG, "*** %s: %s vlan num: %d", __FUNCTION__, prefix, vlannum);

			if ((f = popen("/usr/sbin/robocfg showports", "r")) != NULL) {
				while (fgets(tmp, sizeof(tmp), f)) {
					if (sscanf(tmp, "Port %d: %s %*s %*s %*s vlan: %d %*s", &a, b, &c) == 3) {
						if ((strncmp(b, "DOWN", 4) == 0) && (c == vlannum)) {
							logmsg(LOG_DEBUG, "*** %s: port state = DOWN for vlan%d", __FUNCTION__, vlannum);
							up = 0;
						}
					}
				}
				pclose(f);
			}
		}
	}

state:
	memset(buf1, 0, sizeof(buf1));
	snprintf(buf1, sizeof(buf1), "%s_ck_pause", prefix);

	if (up == 1) { /* also check result from mwwatchdog */
		if ((nvram_get_int("mwan_cktime") == 0) || nvram_get_int(buf1)) /* skip checking on this WAN */
			return up;

		memset(buf1, 0, sizeof(buf1));
		snprintf(buf1, sizeof(buf1), "/var/lib/misc/%s_state", prefix);
		if ((f = fopen(buf1, "r")) == NULL) /* no state file? so probably wan is just up */
			return up;

		fscanf(f, "%d", &up);
		fclose(f);
	}

	return up;
}

const dns_list_t *get_dns(char *prefix)
{
	static dns_list_t dns;
	char s[512];
	int n;
	int i, j;
	struct in_addr ia;
	char d[7][22];
	unsigned short port;
	char *c;
	char tmp[100];

	dns.count = 0;

	memset(s, 0, sizeof(s)); /* reset */
	if (nvram_get_int(strlcat_r(prefix, "_dns_auto", tmp, sizeof(tmp))))
		snprintf(s, sizeof(s), " %s", nvram_safe_get(strlcat_r(prefix, "_get_dns", tmp, sizeof(tmp))));
	else {
		strlcpy(s, nvram_safe_get(strlcat_r(prefix, "_dns", tmp, sizeof(tmp))), sizeof(s));
		if (nvram_get_int("dns_addget")) /* add received DNS servers to the static DNS server list */
			snprintf(s + strlen(s), sizeof(s) - strlen(s), " %s", nvram_safe_get(strlcat_r(prefix, "_get_dns", tmp, sizeof(tmp))));
	}

	n = sscanf(s, "%21s %21s %21s %21s %21s %21s %21s", d[0], d[1], d[2], d[3], d[4], d[5], d[6]);
	for (i = 0; i < n; ++i) {
		port = 53;

		if ((c = strchr(d[i], ':')) != NULL) {
			*c++ = 0;
			if (((j = atoi(c)) < 1) || (j > 0xFFFF))
				continue;

			port = j;
		}

		if (inet_pton(AF_INET, d[i], &ia) > 0) {
			for (j = dns.count - 1; j >= 0; --j) {
				if ((dns.dns[j].addr.s_addr == ia.s_addr) && (dns.dns[j].port == port))
					break;
			}
			if (j < 0) {
				dns.dns[dns.count].port = port;
				dns.dns[dns.count++].addr.s_addr = ia.s_addr;
				if (dns.count == 6)
					break;
			}
		}
	}

	return &dns;
}

void set_action(int a)
{
	int r = 3;
	while (f_write("/var/lock/action", &a, sizeof(a), 0, 0) != sizeof(a)) {
		sleep(1);
		if (--r == 0)
			return;
	}
	if (a != ACT_IDLE)
		sleep(2);
}

int check_action(void)
{
	int a;
	int r = 3;

	while (f_read("/var/lock/action", &a, sizeof(a)) != sizeof(a)) {
		sleep(1);
		if (--r == 0)
			return ACT_UNKNOWN;
	}

	return a;
}

int wait_action_idle(int n)
{
	while (n-- > 0) {
		if (check_action() == ACT_IDLE)
			return 1;
		sleep(1);
	}

	return 0;
}

const wanface_list_t *get_wanfaces(char *prefix)
{
	static wanface_list_t wanfaces;
	char *ip, *iface;
	int proto;
	char tmp[100];

	wanfaces.count = 0;

	switch ((proto = get_wanx_proto(prefix))) {
		case WP_PPTP:
		case WP_L2TP:
			while (wanfaces.count < 2) {
				if (wanfaces.count == 0) {
					ip = nvram_safe_get(strlcat_r(prefix, "_ppp_get_ip", tmp, sizeof(tmp)));
					iface = nvram_safe_get(strlcat_r(prefix, "_iface", tmp, sizeof(tmp)));
					if (!(*iface))
						iface = "ppp+";
				}
				else /* if (wanfaces.count == 1) */ {
					ip = nvram_safe_get(strlcat_r(prefix, "_ipaddr", tmp, sizeof(tmp)));
					if ((!(*ip) || strcmp(ip, "0.0.0.0") == 0) && (wanfaces.count > 0))
						iface = "";
					else
						iface = nvram_safe_get(strlcat_r(prefix, "_ifname", tmp, sizeof(tmp)));
				}
				strlcpy(wanfaces.iface[wanfaces.count].ip, ip, sizeof(wanfaces.iface[0].ip));
				strlcpy(wanfaces.iface[wanfaces.count].name, iface, IFNAMSIZ);
				++wanfaces.count;
			}
			break;
		case WP_PPPOE:
			if (using_dhcpc(prefix)) { /* PPPoE with MAN */
				while (wanfaces.count < 2) {
					if (wanfaces.count == 0) {
						ip = nvram_safe_get(strlcat_r(prefix, "_ppp_get_ip", tmp, sizeof(tmp)));
						iface = nvram_safe_get(strlcat_r(prefix, "_iface", tmp, sizeof(tmp)));
						if (!(*iface)) iface = "ppp+";
					}
					else /* if (wanfaces.count == 1) */ {
						ip = nvram_safe_get(strlcat_r(prefix, "_ipaddr", tmp, sizeof(tmp)));
						if ((!(*ip) || strcmp(ip, "0.0.0.0") == 0) && (wanfaces.count > 0))
							iface = "";
						else
							iface = nvram_safe_get(strlcat_r(prefix, "_ifname", tmp, sizeof(tmp)));
					}
					strlcpy(wanfaces.iface[wanfaces.count].ip, ip, sizeof(wanfaces.iface[0].ip));
					strlcpy(wanfaces.iface[wanfaces.count].name, iface, IFNAMSIZ);
					++wanfaces.count;
				}
			}
			else { /* PPPoE */
				ip = (proto == WP_DISABLED) ? "0.0.0.0" : nvram_safe_get(strlcat_r(prefix, "_ipaddr", tmp, sizeof(tmp)));
				iface = nvram_safe_get(strlcat_r(prefix, "_iface", tmp, sizeof(tmp)));
				if (!(*iface))
					iface = "ppp+";
				strlcpy(wanfaces.iface[wanfaces.count].ip, ip, sizeof(wanfaces.iface[0].ip));
				strlcpy(wanfaces.iface[wanfaces.count++].name, iface, IFNAMSIZ);
			}
			break;
		default:
			ip = (proto == WP_DISABLED) ? "0.0.0.0" : nvram_safe_get(strlcat_r(prefix, "_ipaddr", tmp, sizeof(tmp)));
			if (proto == WP_PPP3G) {
				iface = nvram_safe_get(strlcat_r(prefix, "_iface", tmp, sizeof(tmp)));
				if (!(*iface))
					iface = "ppp+";
			}
			else {
				iface = nvram_safe_get(strlcat_r(prefix, "_ifname", tmp, sizeof(tmp)));
			}
			strlcpy(wanfaces.iface[wanfaces.count].ip, ip, sizeof(wanfaces.iface[0].ip));
			strlcpy(wanfaces.iface[wanfaces.count++].name, iface, IFNAMSIZ);
			break;
	}

	return &wanfaces;
}

const char *get_wanface(char *prefix)
{
	return (*get_wanfaces(prefix)).iface[0].name;
}

#ifdef TCONFIG_IPV6
const char *get_wan6face(void)
{
	switch (get_ipv6_service()) {
		case IPV6_NATIVE:
		case IPV6_NATIVE_DHCP:
			return get_wanface("wan");
		case IPV6_ANYCAST_6TO4:
			return "v6to4";
		case IPV6_6IN4:
			return "v6in4";
		case IPV6_6RD:
			return "6rd";
		case IPV6_6RD_DHCP:
			return "6rd-pd";
	}

	return nvram_safe_get("ipv6_ifname");
}
#endif

const char *get_wanip(char *prefix)
{
	if (!check_wanup(prefix))
		return "0.0.0.0";

	return (*get_wanfaces(prefix)).iface[0].ip;
}

const char *getifaddr(char *ifname, int family, int linklocal)
{
	static char buf[INET6_ADDRSTRLEN];
	void *addr = NULL;
	struct ifaddrs *ifap, *ifa;

	if (getifaddrs(&ifap) != 0) {
		logmsg(LOG_DEBUG, "*** %s: getifaddrs failed: %s", __FUNCTION__, strerror(errno));
		return NULL;
	}

	for (ifa = ifap; ifa; ifa = ifa->ifa_next) {
		if ((ifa->ifa_addr == NULL) ||
		    (strncmp(ifa->ifa_name, ifname, IFNAMSIZ) != 0) ||
		    (ifa->ifa_addr->sa_family != family))
			continue;

#ifdef TCONFIG_IPV6
		if (ifa->ifa_addr->sa_family == AF_INET6) {
			struct sockaddr_in6 *s6 = (struct sockaddr_in6 *)(ifa->ifa_addr);
			if (IN6_IS_ADDR_LINKLOCAL(&s6->sin6_addr) ^ linklocal)
				continue;
			addr = (void *)&(s6->sin6_addr);
		}
		else
#endif
		{
			struct sockaddr_in *s = (struct sockaddr_in *)(ifa->ifa_addr);
			addr = (void *)&(s->sin_addr);
		}

		if ((addr) && inet_ntop(ifa->ifa_addr->sa_family, addr, buf, sizeof(buf)) != NULL) {
			freeifaddrs(ifap);
			return buf;
		}
	}

	freeifaddrs(ifap);

	return NULL;
}

int is_intf_up(const char* ifname)
{
	struct ifreq ifr;
	int sfd;
	int ret = 0;

	if (!((sfd = socket(AF_INET, SOCK_RAW, IPPROTO_RAW)) < 0)) {
		strlcpy(ifr.ifr_name, ifname, sizeof(ifr.ifr_name));
		if (!ioctl(sfd, SIOCGIFFLAGS, &ifr) && (ifr.ifr_flags & IFF_UP))
			ret = 1;

		close(sfd);
	}

	return ret;
}

long get_uptime(void)
{
	struct sysinfo si;
	sysinfo(&si);

	return si.uptime;
}

char *wl_nvname(const char *nv, int unit, int subunit)
{
	static char tmp[128];
	char prefix[] = "wlXXXXXXXXXX_";

	if (unit < 0)
		strlcpy(prefix, "wl_", sizeof(prefix));
	else if (subunit > 0)
		snprintf(prefix, sizeof(prefix), "wl%d.%d_", unit, subunit);
	else
		snprintf(prefix, sizeof(prefix), "wl%d_", unit);

	return strlcat_r(prefix, nv, tmp, sizeof(tmp));
}

int get_radio(int unit)
{
	uint32 n;

	return (wl_ioctl(nvram_safe_get(wl_nvname("ifname", unit, 0)), WLC_GET_RADIO, &n, sizeof(n)) == 0) &&
		((n & WL_RADIO_SW_DISABLE)  == 0);
}

void set_radio(int on, int unit)
{
	uint32 n;

#ifndef WL_BSS_INFO_VERSION
#error WL_BSS_INFO_VERSION
#endif

#if WL_BSS_INFO_VERSION >= 108
	n = on ? (WL_RADIO_SW_DISABLE << 16) : ((WL_RADIO_SW_DISABLE << 16) | 1);
	wl_ioctl(nvram_safe_get(wl_nvname("ifname", unit, 0)), WLC_SET_RADIO, &n, sizeof(n));
	if (!on) {
		if (unit == 0)
			led(LED_WLAN, LED_OFF);
		if (unit == 1)
			led(LED_5G, LED_OFF);
#ifdef TCONFIG_AC3200
		if (unit == 2)
			led(LED_52G, LED_OFF);
#endif
	}
	else {
		if (unit == 0)
			led(LED_WLAN, LED_ON);
		if (unit == 1)
			led(LED_5G, LED_ON);
#ifdef TCONFIG_AC3200
		if (unit == 2)
			led(LED_52G, LED_ON);
#endif
	}
#else /* WL_BSS_INFO_VERSION >= 108 */
	n = on ? 0 : WL_RADIO_SW_DISABLE;
	wl_ioctl(nvram_safe_get(wl_nvname("ifname", unit, 0)), WLC_SET_RADIO, &n, sizeof(n));
	if (!on) {
		led(LED_WLAN, LED_OFF);
	}
	else {
		led(LED_WLAN, LED_ON);
	}
#endif /* WL_BSS_INFO_VERSION >= 108 */
}

int mtd_getinfo(const char *mtdname, int *part, int *size)
{
	FILE *f;
	char s[256];
	char t[256];
	int r;

	r = 0;
	if ((strlen(mtdname) < 128) && (strcmp(mtdname, "pmon") != 0)) {
		snprintf(t, sizeof(t), "\"%s\"", mtdname);
		if ((f = fopen("/proc/mtd", "r")) != NULL) {
			while (fgets(s, sizeof(s), f) != NULL) {
				if ((sscanf(s, "mtd%d: %x", part, size) == 2) && (strstr(s, t) != NULL)) {
					/* don't accidentally mess with bl (0) */
					if (*part > 0)
						r = 1;
					break;
				}
			}
			fclose(f);
		}
	}
	if (!r) {
		*size = 0;
		*part = -1;
	}
	return r;
}

int nvram_get_int(const char *key)
{
	return atoi(nvram_safe_get(key));
}

int nvram_set_int(const char *key, int value)
{
	char nvramstr[16];

	snprintf(nvramstr, sizeof(nvramstr), "%d", value);

	return nvram_set(key, nvramstr);
}

/*
long nvram_xget_long(const char *name, long min, long max, long def)
{
	const char *p;
	char *e;
	long n;

	p = nvram_get(name);
	if ((p != NULL) && (*p != 0)) {
		n = strtol(p, &e, 0);
		if ((e != p) && ((*e == 0) || (*e == ' ')) && (n > min) && (n < max)) {
			return n;
		}
	}
	return def;
}
*/

int nvram_get_file(const char *key, const char *fname, int max)
{
	int n;
	char *p;
	unsigned char *b;
	int r;

	r = 0;
	p = nvram_safe_get(key);
	n = strlen(p);
	if (n <= max) {
		if ((b = malloc(base64_decoded_len(n) + 128)) != NULL) {
			n = base64_decode(p, b, n);
			if (n > 0)
				r = (f_write(fname, b, n, 0, 0644) == n);
			free(b);
		}
	}
	return r;
/*
	char b[2048];
	int n;
	char *p;

	p = nvram_safe_get(key);
	n = strlen(p);
	if (n <= max) {
		n = base64_decode(p, b, n);
		if (n > 0) return (f_write(fname, b, n, 0, 0700) == n);
	}
	return 0;
*/
}

int nvram_set_file(const char *key, const char *fname, int max)
{
	char *in;
	char *out;
	long len;
	int n;
	int r;

	if ((len = f_size(fname)) > max)
		return 0;

	max = (int)len;
	r = 0;
	if (f_read_alloc(fname, &in, max) == max) {
		if ((out = malloc(base64_encoded_len(max) + 128)) != NULL) {
			n = base64_encode(in, out, max);
			out[n] = 0;
			nvram_set(key, out);
			free(out);
			r = 1;
		}
		free(in);
	}
	return r;
/*
	char a[2048];
	char b[4096];
	int n;

	if (((n = f_read(fname, &a, sizeof(a))) > 0) && (n <= max)) {
		n = base64_encode(a, b, n);
		b[n] = 0;
		nvram_set(key, b);
		return 1;
	}
	return 0;
*/
}

int nvram_contains_word(const char *key, const char *word)
{
	return (find_word(nvram_safe_get(key), word) != NULL);
}

int nvram_is_empty(const char *key)
{
	char *p;

	return (((p = nvram_get(key)) == NULL) || (*p == 0));
}

void nvram_commit_x(void)
{
	if (!nvram_get_int("debug_nocommit"))
		nvram_commit();
}

char *getNVRAMVar(const char *text, const int unit)
{
	char buffer[32];
	memset(buffer, 0, sizeof(buffer));
	snprintf(buffer, sizeof(buffer), text, unit);

	return nvram_safe_get(buffer);
}

int connect_timeout(int fd, const struct sockaddr *addr, socklen_t len, int timeout)
{
	fd_set fds;
	struct timeval tv;
	int flags;
	socklen_t n;
	int r;

	if (((flags = fcntl(fd, F_GETFL, 0)) < 0) ||
		(fcntl(fd, F_SETFL, flags | O_NONBLOCK) < 0)) {
		logmsg(LOG_DEBUG, "*** %s: error in F_*ETFL %d", __FUNCTION__, fd);
		return -1;
	}

	if (connect(fd, addr, len) < 0) {
		logmsg(LOG_DEBUG, "*** %s: connect %d = <0", __FUNCTION__, fd);

		if (errno != EINPROGRESS) {
			logmsg(LOG_DEBUG, "*** %s: error in connect %d errno=%d", __FUNCTION__, fd, errno);
			return -1;
		}

		while (1) {
			tv.tv_sec = timeout;
			tv.tv_usec = 0;
			FD_ZERO(&fds);
			FD_SET(fd, &fds);
			r = select(fd + 1, NULL, &fds, NULL, &tv);
			if (r == 0) {
				logmsg(LOG_DEBUG, "*** %s: timeout in select %d", __FUNCTION__, fd);
				return -1;
			}
			else if (r < 0) {
				if (errno != EINTR) {
					logmsg(LOG_DEBUG, "*** %s: error in select %d", __FUNCTION__, fd);
					return -1;
				}
				/* loop */
			}
			else {
				r = 0;
				n = sizeof(r);
				if ((getsockopt(fd, SOL_SOCKET, SO_ERROR, &r, &n) < 0) || (r != 0)) {
					logmsg(LOG_DEBUG, "*** %s: error in SO_ERROR %d", __FUNCTION__, fd);
					return -1;
				}
				break;
			}
		}
	}

	if (fcntl(fd, F_SETFL, flags) < 0) {
		logmsg(LOG_DEBUG, "*** %s: error in F_*ETFL %d", __FUNCTION__, fd);
		return -1;
	}

	logmsg(LOG_DEBUG, "*** %s: OK %d", __FUNCTION__, fd);

	return 0;
}

void chld_reap(int sig)
{
	while (waitpid(-1, NULL, WNOHANG) > 0) {}
}
