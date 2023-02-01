/*
 *
 * Tomato Firmware
 * Copyright (C) 2006-2009 Jonathan Zarate
 *
 */


#include "tomato.h"

#include <ctype.h>
#include <sys/sysinfo.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <net/if.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <dirent.h>
#include <time.h>
#include <sys/statfs.h>
#include <netdb.h>
#include <net/route.h>

#ifdef TCONFIG_IPV6
#include <ifaddrs.h>
#endif

#include <wlioctl.h>
#include <wlutils.h>

typedef struct {
	unsigned long total;
	unsigned long free;
	unsigned long buffers;
	unsigned long cached;
	unsigned long swaptotal;
	unsigned long swapfree;
	unsigned long maxfreeram;
} meminfo_t;


/* to javascript-safe string */
char *js_string(const char *s)
{
	unsigned char c;
	char *buffer;
	char *b;

	if ((buffer = malloc((strlen(s) * 4) + 1)) != NULL) {
		b = buffer;
		while ((c = *s++) != 0) {
			if ((c == '"') || (c == '\'') || (c == '\\') || (!isprint(c)))
				b += sprintf(b, "\\x%02x", c);
			else
				*b++ = c;
		}
		*b = 0;
	}

	return buffer;
}

/* to html-safe string */
char *html_string(const char *s)
{
	unsigned char c;
	char *buffer;
	char *b;

	if ((buffer = malloc((strlen(s) * 6) + 1)) != NULL) {
		b = buffer;
		while ((c = *s++) != 0) {
			if ((c == '&') || (c == '<') || (c == '>') || (c == '"') || (c == '\'') || (!isprint(c)))
				b += sprintf(b, "&#%d;", c);
			else
				*b++ = c;
		}
		*b = 0;
	}

	return buffer;
}

/* removes \r */
char *unix_string(const char *s)
{
	char *buffer;
	char *b;
	char c;

	if ((buffer = malloc(strlen(s) + 1)) != NULL) {
		b = buffer;
		while ((c = *s++) != 0) {
			if (c != '\r')
				*b++ = c;
		}
		*b = 0;
	}

	return buffer;
}

/* # days, ##:##:## */
char *reltime(char *buf, time_t t)
{
	int days;
	int m;

	if (t < 0) t = 0;
	days = t / 86400;
	m = t / 60;
	sprintf(buf, "%d day%s, %02d:%02d:%02d", days, ((days==1) ? "" : "s"), ((m / 60) % 24), (m % 60), (int)(t % 60));

	return buf;
}

int get_client_info(char *mac, char *ifname)
{
	FILE *f;
	char s[256];
#ifdef TCONFIG_IPV6
	char ip[INET6_ADDRSTRLEN];
#else
	char ip[INET_ADDRSTRLEN];
#endif

/*
# ip neigh show fe80:0:0::201:02ff:fe03:0405
fe80::201:2ff:fe3:405 dev br0 lladdr 00:01:02:03:04:05 REACHABLE
*/
	if (clientsai.ss_family == AF_INET) {
		inet_ntop(clientsai.ss_family, &(((struct sockaddr_in*)&clientsai)->sin_addr), ip, sizeof(ip));
		snprintf(s, sizeof(s), "ip neigh show %s", ip);
	}
#ifdef TCONFIG_IPV6
	else if (clientsai.ss_family == AF_INET6) {
		inet_ntop(clientsai.ss_family, &(((struct sockaddr_in6*)&clientsai)->sin6_addr), ip, sizeof(ip));
		if (IN6_IS_ADDR_V4MAPPED(&(((struct sockaddr_in6*)&clientsai)->sin6_addr)))
			snprintf(s, sizeof(s), "ip neigh show %s", ip + 7); /* chop off the ::ffff: to get the ipv4 bit */
		else
			snprintf(s, sizeof(s), "ip neigh show %s", ip);
	}
#endif

	if ((f = popen(s, "r")) != NULL) {
		while (fgets(s, sizeof(s), f)) {
			if (sscanf(s, "%*s dev %16s lladdr %17s %*s", ifname, mac) == 2) {
				pclose(f);
				return 1;
			}
		}
		pclose(f);
	}

	return 0;
}

/* <% lanip(mode); %>
 * <mode>
 * 1	return first 3 octets (192.168.1)
 * 2	return last octet (1)
 * else	return full (192.168.1.1)
 *
 * =>	for all active bridges
 */
void asp_lanip(int argc, char **argv)
{
	char *nv, *p;
	char s[64];
	char mode;
	char comma = ' ';
	int br;

	mode = argc ? *argv[0] : 0;

	web_puts("\nvar lanip = [");
	for (br = 0; br < BRIDGE_COUNT; br++) {
		memset(s, 0, sizeof(s));
		snprintf(s, sizeof(s), (br == 0 ? "lan_ipaddr" : "lan%d_ipaddr"), br);
		if ((nv = nvram_get(s)) != NULL) {
			memset(s, 0, sizeof(s));
			snprintf(s, sizeof(s), "%s", nv);
			if ((p = strrchr(s, '.')) != NULL) {
				*p = 0;
				web_printf("%c'%s'", comma, ((mode == '1') ? s : (mode == '2') ? (p + 1) : nv));
				comma = ',';
			}
		}
	}
	web_puts("];\n");
}

/*	<% psup(process); %>
 *	returns 1 if process is running
 */
void asp_psup(int argc, char **argv)
{
	if (argc == 1)
		web_printf("%d", pidof(argv[0]) > 0);
}

static int get_memory(meminfo_t *m)
{
	FILE *f;
	char s[128];
	int ok = 0;

	memset(m, 0, sizeof(*m));
	if ((f = fopen("/proc/meminfo", "r")) != NULL) {
		while (fgets(s, sizeof(s), f)) {
			if (strncmp(s, "MemTotal:", 9) == 0) {
				m->total = strtoul(s + 12, NULL, 10) * 1024;
				++ok;
			}
			else if (strncmp(s, "MemFree:", 8) == 0) {
				m->free = strtoul(s + 12, NULL, 10) * 1024;
				++ok;
			}
			else if (strncmp(s, "Buffers:", 8) == 0) {
				m->buffers = strtoul(s + 12, NULL, 10) * 1024;
				++ok;
			}
			else if (strncmp(s, "Cached:", 7) == 0) {
				m->cached = strtoul(s + 12, NULL, 10) * 1024;
				++ok;
			}
			else if (strncmp(s, "SwapTotal:", 10) == 0) {
				m->swaptotal = strtoul(s + 12, NULL, 10) * 1024;
				++ok;
			}
			else if (strncmp(s, "SwapFree:", 9) == 0) {
				m->swapfree = strtoul(s + 11, NULL, 10) * 1024;
				++ok;
			}
		}
		fclose(f);
	}
	if (ok == 0)
		return 0;

	m->maxfreeram = m->free;
	if (nvram_match("t_cafree", "1"))
		m->maxfreeram += (m->cached + m->buffers);

	return 1;
}

static char* get_cfeversion(char *buf)
{
	FILE *f;
	char s[16] = "";
	int len = 0;
	char *netgear = nvram_get("board_id"); /* U12HXXXXXX_NETGEAR for FT mips and arm */
	char *cfe_version = nvram_get("cfe_version"); /* Ex.: Netgear R7000 cfe_version=v1.0.22 ; for FT mips and arm */
	char *bl_version = nvram_get("bl_version"); /* Ex.: Asus RT-N18U bl_version=2.0.0.9 ; for FT mips and arm */

	strcpy(buf, "");

	/* check nvram first to speed up */
	/* Asus */
	if (bl_version != NULL) {
		len = strlen(bl_version);
		strncpy(s, bl_version, sizeof(s)-1);
		s[sizeof(s)-1] = '\0';
	}
	/* Netgear */
	else if (cfe_version != NULL) {
		len = strlen(cfe_version);
		strncpy(s, cfe_version, sizeof(s)-1);
		s[sizeof(s)-1] = '\0';
	}
	else {
		/* get ASUS Bootloader version */
		if ((netgear == NULL) && ((f = popen("strings /dev/mtd0ro | grep bl_version | cut -d '=' -f2", "r")) != NULL)) {
			if (fgets(s, 15, f) != NULL)
				len = strlen(s);

			pclose(f);
		}

		if (len == 0 && netgear != NULL && !strncmp(netgear, "U12H", 4)) { /* check for netgear router to speed up here! */
			/* get NETGEAR CFE version */
			if ((f = popen("strings /dev/mtd1ro | grep cfe_version | cut -d '=' -f2", "r")) != NULL) {
				if (fgets(s, 15, f) != NULL)
					len = strlen(s);

				pclose(f);
			}
		}
	}

	if (len == 0)
		strcpy(buf, "--");
	else {
		strcpy(buf, s);
		buf[strcspn(buf, "\n")] = 0;
	}

	return buf;
}

#ifdef TCONFIG_IPV6
#define NOT_AVAIL		"--"

static void print_ipv6_infos(void) /* show IPv6 DUID and addresses: wan, dns, lan, lan-ll, lan1, lan1-ll, lan2, lan2-ll, lan3, lan3-ll */
{
	char buffer[INET6_ADDRSTRLEN];
	char buffer2[16];
	const char *p_tmp;
	char *dns = NULL;
	char *next = NULL;
	struct in6_addr addr;
	int cnt = 0;
	char br;
	FILE *fp = NULL;
	char line[TOMATO_DUID_MAX_LEN];

	if (!ipv6_enabled())
		return;

	/* DUID */
	if ((fp = fopen(TOMATO_DUID_GUI, "r")) != NULL) {
		fgets(line, sizeof(line), fp);
		web_printf("\tip6_duid: '%s',\n", line);
		fclose(fp);
	}
	else {
		web_printf("\tip6_duid: '%s',\n", NOT_AVAIL);
	}

	/* check LAN */
	for (br = 0; br < BRIDGE_COUNT; br++) {
		char bridge[2] = "0";
		if (br != 0)
			bridge[0] += br;
		else
			strcpy(bridge, "");

		memset(buffer2, 0, sizeof(buffer2));
		snprintf(buffer2, sizeof(buffer2), "lan%s_ipaddr", bridge);
		if (strcmp(nvram_safe_get(buffer2), "") != 0) {
			/* check LANx IPv6 address and copy to buffer */
			memset(buffer2, 0, sizeof(buffer2));
			snprintf(buffer2, sizeof(buffer2), "lan%s_ifname", bridge);
			p_tmp = NULL;
			p_tmp = getifaddr(nvram_safe_get(buffer2), AF_INET6, 0); /* global address */
			if (p_tmp != NULL) {
				memset(buffer, 0, sizeof(buffer));
				snprintf(buffer, sizeof(buffer), "%s", p_tmp);
				web_printf("\tip6_lan%s: '%s',\n", bridge, buffer);
			}
			/* check LAN IPv6 link local address and copy to buffer */
			p_tmp = NULL;
			p_tmp = getifaddr(nvram_safe_get(buffer2), AF_INET6, 1); /* link local address */
			if (p_tmp != NULL) {
				memset(buffer, 0, sizeof(buffer));
				snprintf(buffer, sizeof(buffer), "%s", p_tmp);
				web_printf("\tip6_lan%s_ll: '%s',\n", bridge, buffer);
			}
		}
	}

	/* check WAN */
	if (strcmp(get_wan6face(),"") != 0) {
		/* check WAN IPv6 address and copy to buffer */
		p_tmp = NULL;
		p_tmp = getifaddr((char *) get_wan6face(), AF_INET6, 0); /* global address */
		if (p_tmp != NULL) {
			memset(buffer, 0, sizeof(buffer));
			snprintf(buffer, sizeof(buffer), "%s", p_tmp);
			web_printf("\tip6_wan: '%s',\n", buffer);
		}
	}

	/* IPv6 DNS */
	dns = nvram_safe_get("ipv6_dns"); /* check static dns first */

	memset(buffer, 0, sizeof(buffer));
	foreach(buffer, dns, next) {
		/* verify that this is a valid IPv6 address */
		if ((cnt == 0) && inet_pton(AF_INET6, buffer, &addr) == 1) {
			web_printf("\tip6_wan_dns1: '%s',\n", buffer);
			cnt++; /* found and UP */
		}
		else if ((cnt == 1) && inet_pton(AF_INET6, buffer, &addr) == 1) {
			web_printf("\tip6_wan_dns2: '%s',\n", buffer);
			cnt++;  /* found and UP */
		}
	}
	if (cnt == 0) { /* check auto dns if no valid static dns found */
		dns = nvram_safe_get("ipv6_get_dns");

		memset(buffer, 0, sizeof(buffer));
		foreach(buffer, dns, next) {
			/* verify that this is a valid IPv6 address */
			if ((cnt == 0) && inet_pton(AF_INET6, buffer, &addr) == 1) {
				web_printf("\tip6_wan_dns1: '%s',\n", buffer);
				cnt++; /* found and UP */
			}
			else if ((cnt == 1) && inet_pton(AF_INET6, buffer, &addr) == 1) {
				web_printf("\tip6_wan_dns2: '%s',\n", buffer);
				cnt++;  /* found and UP */
			}
		}
	}
	memset(buffer, 0, sizeof(buffer)); /* reset */
	p_tmp = NULL;
	next = NULL;
}

void asp_calc6rdlocalprefix(int argc, char **argv)
{
	struct in6_addr prefix_addr, local_prefix_addr;
	int prefix_len = 0, relay_prefix_len = 0, local_prefix_len = 0;
	struct in_addr wanip_addr;
	char local_prefix[INET6_ADDRSTRLEN];
	char s[128];
	char prefix[] = "wan";

	if (argc != 3)
		return;

	inet_pton(AF_INET6, argv[0], &prefix_addr);
	prefix_len = atoi(argv[1]);
	relay_prefix_len = atoi(argv[2]);
	inet_pton(AF_INET, get_wanip(prefix), &wanip_addr);

	if (calc_6rd_local_prefix(&prefix_addr, prefix_len, relay_prefix_len, &wanip_addr, &local_prefix_addr, &local_prefix_len)
	    && inet_ntop(AF_INET6, &local_prefix_addr, local_prefix, sizeof(local_prefix)) != NULL) {
		snprintf(s, sizeof(s), "\nlocal_prefix = '%s/%d';\n", local_prefix, local_prefix_len);
		web_puts(s);
	}
}
#endif

static int get_flashsize()
{
/*
# cat /proc/mtd
dev:    size   erasesize  name
mtd0: 00020000 00010000 "pmon"
mtd1: 007d0000 00010000 "linux"
*/
	FILE *f;
	char s[512];
	unsigned int size;
	char partname[17];
	int found = 0;

	if ((f = fopen("/proc/mtd", "r")) != NULL) {
		while (fgets(s, sizeof(s), f)) {
			if (sscanf(s, "%*s %X %*s %16s", &size, partname) != 2)
				continue;

			if (strcmp(partname, "\"linux\"") == 0) {
				found = 1;
				break;
			}
		}
		fclose(f);
	}

#ifdef TCONFIG_NAND
	return 128; /* little trick for now - FIXIT */
#else
	if (found) {
		if ((size > 0x2000000) && (size < 0x4000000))
			return 64;
		else if ((size > 0x1000000) && (size < 0x2000000))
			return 32;
		else if ((size > 0x800000) && (size < 0x1000000))
			return 16;
		else if ((size > 0x400000) && (size < 0x800000))
			return 8;
		else if ((size > 0x200000) && (size < 0x400000))
			return 4;
		else if ((size > 0x100000) && (size < 0x200000))
			return 2;
		else
			return 1;
	}
	else
		return 0;
#endif
}

#ifdef TCONFIG_BCMARM
void asp_jiffies(int argc, char **argv)
{
	char sa[64];
	FILE *a;
	char *e = NULL;
	char *f= NULL;

	const char procstat[] = "/proc/stat";
	if ((a = fopen(procstat, "r")) != NULL) {
		fgets(sa, sizeof(sa), a);

		e = sa;

		if ((e = strchr(sa, ' ')) != NULL)
			e = e + 2;
		if ((f = strchr(sa, 10)) != NULL)
			*f = 0;

		web_printf("\njiffies = [ '");
		web_printf("%s", e);
		web_puts("' ];\n");
		fclose(a);
	}
}
#endif

void asp_etherstates(int argc, char **argv)
{
	FILE *f;
	char s[32], *a, b[16];
	unsigned n;

	if (nvram_match("lan_state", "1")) {
		web_puts("\netherstates = {");

		system("/usr/sbin/ethstate");
		n = 0;
		if ((f = fopen("/tmp/ethernet.state", "r")) != NULL) {
			while (fgets(s, sizeof(s), f)) {
				if (sscanf(s, "Port 0: %s", b) == 1)
					a = "port0";
				else if (sscanf(s, "Port 1: %s", b) == 1)
					a = "port1";
				else if (sscanf(s, "Port 2: %s", b) == 1)
					a = "port2";
				else if (sscanf(s, "Port 3: %s", b) == 1)
					a = "port3";
				else if (sscanf(s, "Port 4: %s", b) == 1)
					a = "port4";
				else
					continue;

				web_printf("%s\t%s: '%s'", n ? ",\n" : "", a, b);
				n++;
			}
			fclose(f);
		}
		web_puts("\n};\n");
	}
	else
		web_puts("\netherstates = {\tport0: 'disabled'\n};\n");
}

void asp_anonupdate(int argc, char **argv)
{
	FILE *f;
	char s[32], *a, b[16];
	unsigned n;

	if (nvram_match("tomatoanon_answer", "1") && nvram_match("tomatoanon_enable", "1") && nvram_match("tomatoanon_notify", "1")) {
		web_puts("\nanonupdate = {");

		n = 0;
		if ((f = fopen("/tmp/anon.version", "r")) != NULL) {
			while (fgets(s, sizeof(s), f)) {
				if (sscanf(s, "have_update=%s", b) == 1)
					a = "update";
				else
					continue;

				web_printf("%s\t%s: '%s'", n ? ",\n" : "", a, b);
				n++;
			}
			fclose(f);
		}
		web_puts("\n};\n");
	}
	else
		web_puts("\nanonupdate = {\tupdate: 'no'\n};\n");
}

void asp_sysinfo(int argc, char **argv)
{
	struct sysinfo si;
	char s[64];
	meminfo_t mem;

	char system_type[64];
	char cpu_model[64];
	char bogomips[8];
	char cpuclk[8];
	char cfe_version[16];

#if defined(TCONFIG_BLINK) || defined(TCONFIG_BCMARM) /* RT-N+ */
	char wl_tempsense[128];
#endif

#ifdef TCONFIG_BCMARM
	char sa[64];
	FILE *a;
	char *e = NULL;
	char *f= NULL;
	const char procstat[] = "/proc/stat";
	char cputemp[8];

	get_cpuinfo(system_type, cpu_model, bogomips, cpuclk, cputemp);
#else
	get_cpuinfo(system_type, cpu_model, bogomips, cpuclk);
#endif

#if defined(TCONFIG_BLINK) || defined(TCONFIG_BCMARM) /* RT-N+ */
	get_wl_tempsense(wl_tempsense);
#endif

	get_cfeversion(cfe_version);

	web_puts("\nsysinfo = {\n");

#ifdef TCONFIG_IPV6
	print_ipv6_infos();
#endif
	sysinfo(&si);
	get_memory(&mem);
	web_printf("\tuptime: %ld,\n"
	           "\tuptime_s: '%s',\n"
	           "\tloads: [%ld, %ld, %ld],\n"
	           "\ttotalram: %lu,\n"
	           "\tfreeram: %lu,\n"
	           "\tbufferram: %lu,\n"
	           "\tcached: %lu,\n"
	           "\ttotalswap: %lu,\n"
	           "\tfreeswap: %lu,\n"
	           "\ttotalfreeram: %lu,\n"
	           "\tprocs: %d,\n"
	           "\tflashsize: %d,\n"
	           "\tsystemtype: '%s',\n"
	           "\tcpumodel: '%s',\n"
	           "\tbogomips: '%s',\n"
	           "\tcpuclk: '%s',\n"
#ifdef TCONFIG_BCMARM
	           "\tcputemp: '%s',\n"
#endif
#if defined(TCONFIG_BLINK) || defined(TCONFIG_BCMARM) /* RT-N+ */
	           "\twlsense: '%s',\n"
#endif
	           "\tcfeversion: '%s'",
	           si.uptime,
	           reltime(s, si.uptime),
	           si.loads[0], si.loads[1], si.loads[2],
	           mem.total, mem.free,
	           mem.buffers, mem.cached,
	           mem.swaptotal, mem.swapfree,
	           mem.maxfreeram,
	           si.procs,
	           get_flashsize(),
	           system_type,
	           cpu_model,
	           bogomips,
	           cpuclk,
#ifdef TCONFIG_BCMARM
	           cputemp,
#endif
#if defined(TCONFIG_BLINK) || defined(TCONFIG_BCMARM) /* RT-N+ */
	           wl_tempsense,
#endif
	           cfe_version);

#ifdef TCONFIG_BCMARM
	if ((a = fopen(procstat, "r")) != NULL) {
		fgets(sa, sizeof(sa), a);
		e = sa;
		if ((e = strchr(sa, ' ')) != NULL)
			e = e + 2;
		if ((f = strchr(sa, 10)) != NULL)
			*f = 0;
		web_printf(",\n\tjiffies: '");
		web_printf("%s", e);
		web_puts("'\n");
		fclose(a);
	}
	else
		web_puts("\n");
#endif

	web_puts("};\n");
}

void asp_activeroutes(int argc, char **argv)
{
	FILE *f;
	char s[512];
	char dev[17];
	unsigned long dest, gateway, flags, mask;
	unsigned metric;
	struct in_addr ia;
	char s_dest[16];
	char s_gateway[16];
	char s_mask[16];
	int n;

	web_puts("\nactiveroutes = [");
	n = 0;
	if ((f = fopen("/proc/net/route", "r")) != NULL) {
		while (fgets(s, sizeof(s), f)) {
			if (sscanf(s, "%16s%lx%lx%lx%*s%*s%u%lx", dev, &dest, &gateway, &flags, &metric, &mask) != 6)
				continue;
			if ((flags & RTF_UP) == 0)
				continue;

			if (dest != 0) {
				ia.s_addr = dest;
				strcpy(s_dest, inet_ntoa(ia));
			}
			else
				strcpy(s_dest, "default");

			if (gateway != 0) {
				ia.s_addr = gateway;
				strcpy(s_gateway, inet_ntoa(ia));
			}
			else
				strcpy(s_gateway, "*");

			ia.s_addr = mask;
			strcpy(s_mask, inet_ntoa(ia));
			web_printf("%s['%s','%s','%s','%s',%u]", n ? "," : "", dev, s_dest, s_gateway, s_mask, metric);
			++n;
		}
		fclose(f);
	}

#ifdef TCONFIG_IPV6
	int pxlen;
	char addr6x[80];
	struct sockaddr_in6 snaddr6;
	char addr6[40], nhop6[40];

	if ((ipv6_enabled()) && (f = fopen("/proc/net/ipv6_route", "r")) != NULL) {
		while (fgets(s, sizeof(s), f)) {
			if (sscanf(s, "%32s%x%*s%*s%32s%x%*s%*s%lx%s\n", addr6x + 14, &pxlen, addr6x + 40 + 7, &metric, &flags, dev) != 6)
				continue;

			if ((flags & RTF_UP) == 0)
				continue;

			int i = 0;
			char *p = addr6x+14;
			do {
				if (!*p) {
					if (i == 40) { /* nul terminator for 1st address? */
						addr6x[39] = 0; /* Fixup... need 0 instead of ':' */
						++p; /* skip and continue */
						continue;
					}
					goto OUT;
				}
				addr6x[i++] = *p++;
				if (!((i+1) % 5))
					addr6x[i++] = ':';
			} while (i < 40 + 28 + 7);

			inet_pton(AF_INET6, addr6x, (struct sockaddr *) &snaddr6.sin6_addr);
			if (IN6_IS_ADDR_UNSPECIFIED(&snaddr6.sin6_addr))
				strcpy(addr6, "default");
			else
				inet_ntop(AF_INET6, &snaddr6.sin6_addr, addr6, sizeof(addr6));

			inet_pton(AF_INET6, addr6x + 40, (struct sockaddr *) &snaddr6.sin6_addr);
			if (IN6_IS_ADDR_UNSPECIFIED(&snaddr6.sin6_addr))
				strcpy(nhop6, "*");
			else
				inet_ntop(AF_INET6, &snaddr6.sin6_addr, nhop6, sizeof(nhop6));

			web_printf("%s['%s','%s','%s','%d',%u]", n ? "," : "", dev, addr6, nhop6, pxlen, metric);
			++n;
		}
OUT:
		fclose(f);
	}
#endif

	web_puts("];\n");
}

void asp_cgi_get(int argc, char **argv)
{
	const char *v;
	int i;

	for (i = 0; i < argc; ++i) {
		v = webcgi_get(argv[i]);
		if (v)
			web_puts(v);
	}
}

void asp_time(int argc, char **argv)
{
	time_t t;
	char s[64];

	t = time(NULL);
	if (t < Y2K)
		web_puts("<span class=\"blinking\">Not Available</span>");
	else {
		strftime(s, sizeof(s), "%a, %d %b %Y %H:%M:%S %z", localtime(&t));
		web_puts(s);
	}
}

#ifdef TCONFIG_SDHC
void asp_mmcid(int argc, char **argv) {
	FILE *f;
	char s[32], *a, b[16];
	unsigned n, size;

	web_puts("\nmmcid = {");
	n = 0;
	if ((f = fopen("/proc/mmc/status", "r")) != NULL) {
		while (fgets(s, sizeof(s), f)) {
			size = 1;
			if (sscanf(s, "Card Type      : %16s", b) == 1)
				a = "type";
			else if (sscanf(s, "Spec Version   : %16s", b) == 1)
				a = "spec";
			else if (sscanf(s, "Num. of Blocks : %d", &size) == 1) {
				a = "size";
				snprintf(b, sizeof(b), "%lld", ((unsigned long long)size) * 512);
			}
			else if (sscanf(s, "Voltage Range  : %16s", b) == 1)
				a = "volt";
			else if (sscanf(s, "Manufacture ID : %16s", b) == 1)
				a = "manuf";
			else if (sscanf(s, "Application ID : %16s", b) == 1)
				a = "appl";
			else if (sscanf(s, "Product Name   : %16s", b) == 1)
				a = "prod";
			else if (sscanf(s, "Revision       : %16s", b) == 1)
				a = "rev";
			else if (sscanf(s, "Serial Number  : %16s", b) == 1)
				a = "serial";
			else if (sscanf(s, "Manu. Date     : %8c", b) == 1) {
				a = "date";
				b[9] = 0;
			}
			else
				continue;

			web_printf(size == 1 ? "%s\t%s: '%s'" : "%s\t%s: %s", n ? ",\n" : "", a, b);
			n++;
		}
		fclose(f);
	}
	web_puts("\n};\n");
}
#endif

void asp_wanup(int argc, char **argv)
{
	char prefix[] = "wanXX";

	if (argc > 0)
		strcpy(prefix, argv[0]);
	else
		strcpy(prefix, "wan");

	web_puts(check_wanup(prefix) ? "1" : "0");
}

void asp_wanstatus(int argc, char **argv)
{
	char prefix[] = "wanXX";
	const char *p;
	char renew_file[64];
	char wanconn_file[64];

	if (argc > 0)
		strcpy(prefix, argv[0]);
	else
		strcpy(prefix, "wan");

	memset(renew_file, 0, 64);
	snprintf(renew_file, sizeof(renew_file), "/var/lib/misc/%s_dhcpc.renewing", prefix);
	memset(wanconn_file, 0, 64);
	snprintf(wanconn_file, sizeof(wanconn_file), "/var/lib/misc/%s.connecting", prefix);

	if ((using_dhcpc(prefix)) && (f_exists(renew_file)))
		p = "Renewing...";
	else if (check_wanup(prefix))
		p = "Connected";
	else if (f_exists(wanconn_file))
		p = "Connecting...";
	else
		p = "Disconnected";

	web_puts(p);
}

void asp_link_uptime(int argc, char **argv)
{
	char buf[64];
	long uptime;
	char prefix[] = "wanXX";

	if (argc > 0)
		strcpy(prefix, argv[0]);
	else
		strcpy(prefix, "wan");

	buf[0] = '-';
	buf[1] = 0;
	if (check_wanup(prefix)) {
		uptime = check_wanup_time(prefix); /* get wanX uptime */
		reltime(buf, uptime);
	}
	web_puts(buf);
}

void asp_rrule(int argc, char **argv)
{
	char s[32];
	int i;

	i = nvram_get_int("rruleN");
	snprintf(s, sizeof(s), "rrule%d", i);
	web_puts("\nrrule = '");
	web_putj_utf8(nvram_safe_get(s));
	web_printf("';\nrruleN = %d;\n", i);
}

void asp_compmac(int argc, char **argv)
{
	char mac[32];
	char ifname[32];

	if (get_client_info(mac, ifname))
		web_puts(mac);
}

void asp_ident(int argc, char **argv)
{
	web_puth(nvram_safe_get("router_name"));
}

void asp_statfs(int argc, char **argv)
{
	struct statfs sf;
	int mnt;

	if (argc != 2)
		return;

	/* used for /cifs/, /jffs/... if it returns squashfs type, assume it's not mounted */
	if ((statfs(argv[0], &sf) != 0) || (sf.f_type == 0x73717368)
#if defined(TCONFIG_BCMARM) || defined(TCONFIG_BLINK)
	    || (sf.f_type == 0x71736873)
#endif
	) {
		mnt = 0;
		memset(&sf, 0, sizeof(sf));
#ifdef TCONFIG_JFFS2
		/* for jffs, try to get total size from mtd partition */
		if ((strncmp(argv[1], "jffs", 4) == 0) || (strncmp(argv[1], "brcmnand", 8) == 0)) {
			int part;

			if (mtd_getinfo(argv[1], &part, (int *)&sf.f_blocks))
				sf.f_bsize = 1;
		}
#endif
	}
	else
		mnt = 1;

	web_printf("\n%s = {\n"
	           "\tmnt: %d,\n"
	           "\tsize: %llu,\n"
	           "\tfree: %llu\n"
	           "};\n",
	           argv[1], mnt,
	           ((uint64_t)sf.f_bsize * sf.f_blocks),
	           ((uint64_t)sf.f_bsize * sf.f_bfree));
}

void asp_notice(int argc, char **argv)
{
	char s[64];
	char buf[2048];

	if (argc != 1)
		return;

	if ((strstr(argv[0], "/") > 0) || (strstr(argv[0], ".") > 0))
		return;

	snprintf(s, sizeof(s), "/var/notice/%s", argv[0]);
	if (f_read_string(s, buf, sizeof(buf)) <= 0)
		return;

	web_putj(buf);
}

void wo_wakeup(char *url)
{
	char *mac;
	char *p;
	char *end;

	if ((mac = webcgi_get("mac")) != NULL) {
		end = mac + strlen(mac);
		while (mac < end) {
			while ((*mac == ' ') || (*mac == '\t') || (*mac == '\r') || (*mac == '\n')) ++mac;
			if (*mac == 0)
				break;

			p = mac;
			while ((*p != 0) && (*p != ' ') && (*p != '\r') && (*p != '\n'))
				++p;

			*p = 0;

			eval("ether-wake", "-b", "-i", nvram_safe_get("lan_ifname"), mac);
			if (strcmp(nvram_safe_get("lan1_ifname"), "") != 0)
				eval("ether-wake", "-b", "-i", nvram_safe_get("lan1_ifname"), mac);
			if (strcmp(nvram_safe_get("lan2_ifname"), "") != 0)
				eval("ether-wake", "-b", "-i", nvram_safe_get("lan2_ifname"), mac);
			if (strcmp(nvram_safe_get("lan3_ifname"), "") != 0)
				eval("ether-wake", "-b", "-i", nvram_safe_get("lan3_ifname"), mac);
			mac = p + 1;
		}
	}
	common_redirect();
}

void asp_dns(int argc, char **argv)
{
	char s[128];
	int i;
	const dns_list_t *dns;
	char prefix[] = "wanXX";

	if (argc > 0)
		strcpy(prefix, argv[0]);
	else
		strcpy(prefix, "wan");

	dns = get_dns(prefix); /* static buffer */
	strcpy(s, "[");
	for (i = 0 ; i < dns->count; ++i)
		snprintf(s + strlen(s), sizeof(s), "%s'%s:%u'", i ? "," : "", inet_ntoa(dns->dns[i].addr), dns->dns[i].port);

	strcat(s, "]");
	web_puts(s);
}

int resolve_addr(const char *ip, char *host)
{
	struct addrinfo hints;
	struct addrinfo *res;
	int ret;

	memset(&hints, 0, sizeof(hints));
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;

	ret = getaddrinfo(ip, NULL, &hints, &res);
	if (ret == 0) {
		ret = getnameinfo(res->ai_addr, res->ai_addrlen, host, NI_MAXHOST, NULL, 0, 0);
		freeaddrinfo(res);
	}

	return ret;
}

void wo_resolve(char *url)
{
	char *p;
	char *ip;
	char host[NI_MAXHOST];
	char comma;
	char *js;

	comma = ' ';
	web_puts("\nresolve_data = [\n");
	if ((p = webcgi_get("ip")) != NULL) {
		while ((ip = strsep(&p, ",")) != NULL) {
			if (resolve_addr(ip, host) != 0)
				continue;

			js = js_string(host);
			web_printf("%c['%s','%s']", comma, ip, js);
			free(js);
			comma = ',';
		}
	}
	web_puts("];\n");
}

#ifdef TCONFIG_STUBBY
void asp_stubby_presets(int argc, char **argv)
{
	FILE *fp;
	char buf[256], *datafile, *ptr, *item, *lsep, *fsep;

	if (argc != 1)
		return;

	if (!strcmp(argv[0], "dot"))
		datafile = "/rom/dot-servers.dat";
	else
		return;

	if (!(fp = fopen(datafile, "r")))
		return;

	for (lsep = ""; (ptr = fgets(buf, sizeof(buf), fp)) != NULL;) {
		buf[sizeof(buf) - 1] = '\0';
		ptr = strsep(&ptr, "#\n");
		if (*ptr == '\0')
			continue;

		web_printf("%s[", lsep);
		for (fsep = ""; (item = strsep(&ptr, ",")) != NULL;) {
			web_printf("%s\"%s\"", fsep, item);
			fsep = ",";
		}
		web_puts("]");
		lsep = ",";
	}

	fclose(fp);
}
#endif

#ifdef TCONFIG_DNSCRYPT
void asp_dnscrypt_presets(int argc, char **argv)
{
	FILE *fp;

	char comma;
	char line[512];
	char *name1, *dnssec, *logs, *a, *b, *c, *d, *e, *f;

	comma = ' ';

	if (!(fp = fopen("/etc/dnscrypt-resolvers-alt.csv", "r"))) { /* try alternative (ex. newly downloaded) resolvers file first */
		if (!(fp = fopen("/etc/dnscrypt-resolvers.csv", "r")))
			return;
	}

	fgets(line, sizeof(line), fp); /* header */
	while (fgets(line, sizeof(line), fp) != NULL) {
		name1 = dnssec = logs = NULL;

		if (((vstrsep(line, ",", &name1, &a, &b, &c, &d, &e, &f, &dnssec, &logs)) < 9) || (!*name1) || (!*dnssec) || (!*logs))
			continue;

		web_printf("%c['%s','%s (DNSSEC:%s NOLOGS:%s)']", comma, name1, name1, dnssec, logs);
		comma = ',';
	}

	fclose(fp);
}
#endif
