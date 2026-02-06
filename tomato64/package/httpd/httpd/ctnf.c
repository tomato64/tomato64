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

#include <ctype.h>
#include <sys/sysinfo.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <net/if.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <time.h>
#include <dirent.h>

#define IP6_PREFIX_NOT_MATCH(a, b, bits) (memcmp(&a.s6_addr[0], &b.s6_addr[0], bits/8) != 0 || (bits % 8 && (a.s6_addr[bits/8] ^ b.s6_addr[bits/8]) >> (8-(bits % 8))))
#define MAX_SEARCH	10


void ctvbuf(FILE *f)
{
	int n;
	struct sysinfo si;
	const char *p;

	if ((p = nvram_get("ct_max")) != NULL) {
		n = atoi(p);
		if (n == 0)
			n = 2048;
		else if (n < 1024)
			n = 1024;
		else if (n > 10240)
			n = 10240;
	}
	else
		n = 2048;

	n *= 170; /* avg tested */

	sysinfo(&si);
	if (si.freeram < (unsigned int) (n + (64 * 1024)))
		n = si.freeram - (64 * 1024);

	if (n > 4096)
		setvbuf(f, NULL, _IOFBF, n);
}

void asp_ctcount(int argc, char **argv)
{
	static const char *states[10] = { "NONE", "ESTABLISHED", "SYN_SENT", "SYN_RECV", "FIN_WAIT", "TIME_WAIT", "CLOSE", "CLOSE_WAIT", "LAST_ACK", "LISTEN" };
	int count[13]; /* tcp(10) + udp(2) + total(1) = 13 / max classes = 10 */
	FILE *f;
	char s[512];
	char *p, *t;
	unsigned int i;
	int n, mode;
	unsigned long rip, lan, mask;

	if (argc != 1)
		return;

#ifdef TCONFIG_IPV6
	char src[INET6_ADDRSTRLEN];
	char dst[INET6_ADDRSTRLEN];
	struct in6_addr rip6;
	struct in6_addr lan6;
	struct in6_addr in6;
	int lan6_prefix_len;

	lan6_prefix_len = nvram_get_int("ipv6_prefix_length");
	if (ipv6_enabled()) {
		inet_pton(AF_INET6, nvram_safe_get("ipv6_prefix"), &lan6);
		ipv6_router_address(&rip6);
	}
#endif

	mode = atoi(argv[0]);

	memset(count, 0, sizeof(count));

#ifndef TOMATO64
#ifdef TCONFIG_IPV6
	if ((f = fopen("/proc/net/nf_conntrack", "r")) != NULL) {
#else
	if ((f = fopen("/proc/net/ip_conntrack", "r")) != NULL) {
#endif
#else
	if ((f = fopen("/proc/net/nf_conntrack", "r")) != NULL) {
#endif /* TOMATO64 */
		ctvbuf(f); /* if possible, read in one go */

		if (nvram_match("t_hidelr", "1")) {
			mask = inet_addr(nvram_safe_get("lan_netmask"));
			rip = inet_addr(nvram_safe_get("lan_ipaddr"));
			lan = rip & mask;
		}
		else
			rip = lan = mask = 0;

		while (fgets(s, sizeof(s), f)) {
#ifdef TCONFIG_IPV6
			if (strncmp(s, "ipv4", 4) == 0) {
				t = s + 11;
#else
				t = s;
#endif
				if (rip != 0) {
					/* src=x.x.x.x dst=x.x.x.x  DIR_ORIGINAL */
#ifdef TOMATO64
					if ((p = strstr(t, "src=")) == NULL)
#else
					if ((p = strstr(t + 14, "src=")) == NULL)
#endif /* TOMATO64 */
						continue;

					if ((inet_addr(p + 4) & mask) == lan) {
						if ((p = strstr(p + 13, "dst=")) == NULL)
							continue;
						if (inet_addr(p + 4) == rip)
							continue;
					}
				}
#ifdef TCONFIG_IPV6
			}
			else if (strncmp(s, "ipv6", 4) == 0) {
				t = s + 12;

				if (rip != 0) {
#ifdef TOMATO64
					if ((p = strstr(t, "src=")) == NULL)
#else
					if ((p = strstr(t + 14, "src=")) == NULL)
#endif /* TOMATO64 */
						continue;

					if (sscanf(p, "src=%s dst=%s", src, dst) != 2)
						continue;

					if (inet_pton(AF_INET6, src, &in6) <= 0)
						continue;

					inet_ntop(AF_INET6, &in6, src, sizeof(src));

					if (!IP6_PREFIX_NOT_MATCH(lan6, in6, lan6_prefix_len))
						continue;
				}
			}
			else
				continue; /* another proto family?! */
#endif
			if (mode == 0) {
				/* count connections per state */
				if (strncmp(t, "tcp", 3) == 0) {
#ifdef TOMATO64
					/* Flow-offloaded TCP entries lack a state string;
					 * check for [OFFLOAD]/[HW_OFFLOAD] first.
					 */
					if (strstr(s, "OFFLOAD]") != NULL)
						count[1]++; /* ESTABLISHED */
					else
#endif /* TOMATO64 */
					for (i = 9; i >= 0; --i) {
						if (strstr(s, states[i]) != NULL) {
							count[i]++;
							break;
						}
					}
				}
				else if (strncmp(t, "udp", 3) == 0) {
					if (strstr(s, "[ASSURED]") != NULL)
						count[11]++;
					else
						count[10]++;
				}
				count[12]++;
			}
			else {
				/* count connections per mark */
				if ((p = strstr(s, " mark=")) != NULL) {
					n = atoi(p + 6) & 0xFF;
					if (n <= 10)
						count[n]++;
				}
			}
		}

		fclose(f);
	}

	s[0] = '\0';

	if (mode == 0) {
		p = s;
		for (i = 0; i < 12; ++i) {
			p += snprintf(p, sizeof(s) - (p - s), ",%d", count[i]);
		}
		web_printf("\nconntrack = [%d%s];\n", count[12], s);
	}
	else {
		p = s;
		for (i = 1; i < 11; ++i) {
			p += snprintf(p, sizeof(s) - (p - s), ",%d", count[i]);
		}
		web_printf("\nnfmarks = [%d%s];\n", count[0], s);
	}
}

void asp_ctdump(int argc, char **argv)
{
	FILE *f;
	char s[512];
	char *p, *q;
	int x, mark, rule, findmark, dir_reply;
	unsigned int proto, time;
	char sport[16], dport[16], byteso[16], bytesi[16];
	unsigned long rip, lan, mask;
	char comma;
#ifdef TCONFIG_IPV6
	unsigned int family;
	char src[INET6_ADDRSTRLEN];
	char dst[INET6_ADDRSTRLEN];
	struct in6_addr rip6;
	struct in6_addr lan6;
	struct in6_addr in6;
	int lan6_prefix_len;
#else
	const unsigned int family = 2;
	char src[INET_ADDRSTRLEN];
	char dst[INET_ADDRSTRLEN];
#endif

	if (argc != 1)
		return;

	findmark = atoi(argv[0]);

	mask = inet_addr(nvram_safe_get("lan_netmask"));
	rip = inet_addr(nvram_safe_get("lan_ipaddr"));
	lan = rip & mask;

#ifdef TCONFIG_IPV6
	lan6_prefix_len = nvram_get_int("ipv6_prefix_length");
	if (ipv6_enabled()) {
		inet_pton(AF_INET6, nvram_safe_get("ipv6_prefix"), &lan6);
		ipv6_router_address(&rip6);
	}
#endif

	if (nvram_match("t_hidelr", "0"))
		rip = 0; /* hide lan -> router? */

/*
 * /proc/net/nf_conntrack prefix (compared to ip_conntrack):
 * "ipvx" + 5 spaces + "2" or "10" + 1 space
 *
 * add bytes out/in to table
 *
 */

	web_puts("\nctdump = [");
	comma = ' ';
#ifndef TOMATO64
#ifdef TCONFIG_IPV6
	if ((f = fopen("/proc/net/nf_conntrack", "r")) != NULL) {
#else
	if ((f = fopen("/proc/net/ip_conntrack", "r")) != NULL) {
#endif
#else
	if ((f = fopen("/proc/net/nf_conntrack", "r")) != NULL) {
#endif /* TOMATO64 */
		ctvbuf(f);
		while (fgets(s, sizeof(s), f)) {
			dir_reply = 0;
			if ((p = strstr(s, " mark=")) == NULL)
				continue;

			mark = atoi(p + 6);
			rule = (mark >> 20) & 0xFF;
			if ((mark &= 0xFF) > 10)
				mark = 0;

			if ((findmark != -1) && (mark != findmark))
				continue;
#ifdef TOMATO64
#ifdef TCONFIG_IPV6
			if (sscanf(s, "%*s %u %*s %u %u", &family, &proto, &time) != 3) {
				/* Flow-offloaded entries lack the timeout field */
				if (sscanf(s, "%*s %u %*s %u", &family, &proto) != 2)
					continue;
				time = 0;
			}

			if ((p = strstr(s + 14, "src=")) == NULL)
				continue;
#else
			if (sscanf(s, "%*s %u %u", &proto, &time) != 2) {
				if (sscanf(s, "%*s %u", &proto) != 1)
					continue;
				time = 0;
			}

			if ((p = strstr(s + 7, "src=")) == NULL)
				continue;
#endif
#else /* TOMATO64 */
#ifdef TCONFIG_IPV6
			if (sscanf(s, "%*s %u %*s %u %u", &family, &proto, &time) != 3)
				continue;

			if ((p = strstr(s + 25, "src=")) == NULL)
				continue;
#else
			if (sscanf(s, "%*s %u %u", &proto, &time) != 2)
				continue;

			if ((p = strstr(s + 14, "src=")) == NULL)
				continue;
#endif
#endif /* TOMATO64 */
			if (sscanf(p, "src=%s dst=%s %n", src, dst, &x) != 2)
				continue;

			p += x;
			if ((proto == 6) || (proto == 17)) {
				if (sscanf(p, "sport=%s dport=%s %*s bytes=%s %n", sport, dport, byteso, &x) != 3)
					continue;

				p += x;
				if ((q = strstr(p, "bytes=")) == NULL)
					continue;

				if (sscanf(q, "bytes=%s", bytesi) != 1)
					continue;
			}
			else {
				sport[0] = 0;
				dport[0] = 0;
				byteso[0] = 0;
				bytesi[0] = 0;
			}

			switch (family) {
				case 2:
					if ((inet_addr(src) & mask) != lan) {
						dir_reply = 1;
						/* de-nat */
						if ((p = strstr(p, "src=")) == NULL)
							continue;

						if ((proto == 6) || (proto == 17)) {
							if (sscanf(p, "src=%s dst=%s sport=%s dport=%s", dst, src, dport, sport) != 4)
								continue; /* intentionally backwards */
						}
						else {
							if (sscanf(p, "src=%s dst=%s", dst, src) != 2)
								continue;
						}
					}
					else if (rip != 0 && inet_addr(dst) == rip)
						continue;
				break;
#ifdef TCONFIG_IPV6
				case 10:
					if (inet_pton(AF_INET6, src, &in6) <= 0)
						continue;

					inet_ntop(AF_INET6, &in6, src, sizeof(src));

					if (IP6_PREFIX_NOT_MATCH(lan6, in6, lan6_prefix_len))
						dir_reply = 1;

					if (inet_pton(AF_INET6, dst, &in6) <= 0)
						continue;

					inet_ntop(AF_INET6, &in6, dst, sizeof(dst));

					if (dir_reply == 0 && rip != 0 && (IN6_ARE_ADDR_EQUAL(&rip6, &in6)))
						continue;
				break;
#endif
			}

			if (dir_reply == 1)
				web_printf("%c[%u,%u,'%s','%s','%s','%s','%s','%s',%d,%d,1]", comma, proto, time, dst, src, dport, sport, bytesi, byteso, mark, rule);
			else
				web_printf("%c[%u,%u,'%s','%s','%s','%s','%s','%s',%d,%d,0]", comma, proto, time, src, dst, sport, dport, byteso, bytesi, mark, rule);

			comma = ',';
		}
	}
	web_puts("];\n");
}

void asp_ctrate(int argc, char **argv)
{
	FILE *a;
	FILE *b;
	char sa[512], sb[512];
	char a_sport[16], a_dport[16];
	unsigned int a_time, a_proto, a_bytes_o, a_bytes_i;
	unsigned int b_time, b_proto;
	unsigned int b_bytes_o, b_bytes_i;
	unsigned long rip, lan, mask;
	long b_pos, outbytes, inbytes;
	char comma;
	char *p, *q, *buffer;
	int n, x, len, delay, thres, dir_reply;
	size_t count;
#ifdef TCONFIG_IPV6
	unsigned int a_fam, b_fam;
	char a_src[INET6_ADDRSTRLEN];
	char a_dst[INET6_ADDRSTRLEN];
	struct in6_addr rip6;
	struct in6_addr lan6;
	struct in6_addr in6;
	int lan6_prefix_len;
#else
	const unsigned int a_fam = 2;
	char a_src[INET_ADDRSTRLEN];
	char a_dst[INET_ADDRSTRLEN];
#endif

	mask = inet_addr(nvram_safe_get("lan_netmask"));
	rip = inet_addr(nvram_safe_get("lan_ipaddr"));
	lan = rip & mask;

#ifdef TCONFIG_IPV6
	lan6_prefix_len = nvram_get_int("ipv6_prefix_length");
	if (ipv6_enabled()) {
		inet_pton(AF_INET6, nvram_safe_get("ipv6_prefix"), &lan6);
		ipv6_router_address(&rip6);
	}
#endif

	if (nvram_match("t_hidelr", "0"))
		rip = 0; /* hide lan -> router? */

	web_puts("\nctrate = [");
	comma = ' ';

#ifndef TOMATO64
#ifdef TCONFIG_IPV6
	const char name[] = "/proc/net/nf_conntrack";
#else
	const char name[] = "/proc/net/ip_conntrack";
#endif
#else
	const char name[] = "/proc/net/nf_conntrack";
#endif /* TOMATO64 */

	if (argc != 2)
		return;

	delay = atoi(argv[0]);
	thres = atoi(argv[1]) * delay;

	if ((a = fopen(name, "r")) == NULL)
		return;

	if ((b = tmpfile()) == NULL) {
		fclose(a);
		return;
	}

	buffer = (char *)malloc(1024);

	while (!feof(a)) {
		count = fread(buffer, 1, 1024, a);
		fwrite(buffer, 1, count, b);
	}

	rewind(b);
	rewind(a);

	usleep(1000000 * (int)delay);

	/* a = current, b = previous */
	while (fgets(sa, sizeof(sa), a)) {
#ifdef TOMATO64
#ifdef TCONFIG_IPV6
		if (sscanf(sa, "%*s %u %*s %u %u", &a_fam, &a_proto, &a_time) != 3) {
			/* Flow-offloaded entries lack the timeout field */
			if (sscanf(sa, "%*s %u %*s %u", &a_fam, &a_proto) != 2)
				continue;
			a_time = 0;
		}
#else
		if (sscanf(sa, "%*s %u %u", &a_proto, &a_time) != 2) {
			if (sscanf(sa, "%*s %u", &a_proto) != 1)
				continue;
			a_time = 0;
		}
#endif
#else /* TOMATO64 */
#ifdef TCONFIG_IPV6
		if (sscanf(sa, "%*s %u %*s %u %u", &a_fam, &a_proto, &a_time) != 3)
			continue;
#else
		if (sscanf(sa, "%*s %u %u", &a_proto, &a_time) != 2)
			continue;
#endif
#endif /* TOMATO64 */
		if ((a_proto != 6) && (a_proto != 17))
			continue;

		if ((p = strstr(sa, "src=")) == NULL)
			continue;

		if (sscanf(p, "src=%s dst=%s sport=%s dport=%s%n %*s bytes=%u %n", a_src, a_dst, a_sport, a_dport, &len, &a_bytes_o, &x) != 5)
			continue;

		if ((q = strstr(p+x, "bytes=")) == NULL)
			continue;

		if (sscanf(q, "bytes=%u", &a_bytes_i) != 1)
			continue;

		dir_reply = 0;

		switch(a_fam) {
			case 2:
				if ((inet_addr(a_src) & mask) != lan)
					dir_reply = 1;
				else if (rip != 0 && inet_addr(a_dst) == rip)
					continue;
				break;
#ifdef TCONFIG_IPV6
			case 10:
				if (inet_pton(AF_INET6, a_src, &in6) <= 0)
					continue;

				inet_ntop(AF_INET6, &in6, a_src, sizeof(a_src));

				if (IP6_PREFIX_NOT_MATCH(lan6, in6, lan6_prefix_len))
					dir_reply = 1;

				if (inet_pton(AF_INET6, a_dst, &in6) <= 0)
					continue;

				inet_ntop(AF_INET6, &in6, a_dst, sizeof(a_dst));

				if (dir_reply == 0 && rip != 0 && (IN6_ARE_ADDR_EQUAL(&rip6, &in6)))
					continue;
				break;
			default:
				continue;
#endif
		}

		b_pos = ftell(b);
		n = 0;
		while (fgets(sb, sizeof(sb), b) && ++n < MAX_SEARCH) {
#ifdef TOMATO64
#ifdef TCONFIG_IPV6
			if (sscanf(sb, "%*s %u %*s %u %u", &b_fam, &b_proto, &b_time) != 3) {
				/* Flow-offloaded entries lack the timeout field */
				if (sscanf(sb, "%*s %u %*s %u", &b_fam, &b_proto) != 2)
					continue;
				b_time = 0;
			}

			if ((b_fam != a_fam))
				continue;
#else
			if (sscanf(sb, "%*s %u %u", &b_proto, &b_time) != 2) {
				if (sscanf(sb, "%*s %u", &b_proto) != 1)
					continue;
				b_time = 0;
			}
#endif
#else /* TOMATO64 */
#ifdef TCONFIG_IPV6
			if (sscanf(sb, "%*s %u %*s %u %u", &b_fam, &b_proto, &b_time) != 3)
				continue;

			if ((b_fam != a_fam))
				continue;
#else
			if (sscanf(sb, "%*s %u %u", &b_proto, &b_time) != 2)
				continue;
#endif
#endif /* TOMATO64 */
			if ((b_proto != a_proto))
				continue;

			if ((q = strstr(sb, "src=")) == NULL)
				continue;

			if (strncmp(p, q, (size_t)len))
				continue;

			/* Ok, they should be the same now. Grab the byte counts */
			if ((q = strstr(q + len, "bytes=")) == NULL)
				continue;

			if (sscanf(q, "bytes=%u", &b_bytes_o) != 1)
				continue;

			if ((q = strstr(q + len, "bytes=")) == NULL)
				continue;

			if (sscanf(q, "bytes=%u", &b_bytes_i) != 1)
				continue;

			break;
		}

		if ((feof(b)) || (n >= MAX_SEARCH)) {
			/* Assume this is a new connection */
			b_bytes_o = 0;
			b_bytes_i = 0;
			b_time = 0;

			/* Reset the search so we don't miss anything */
			fseek(b, b_pos, SEEK_SET);
			n = -n;
		}

		outbytes = ((long)a_bytes_o - (long)b_bytes_o);
		inbytes = ((long)a_bytes_i - (long)b_bytes_i);

		if ((outbytes < thres) && (inbytes < thres))
			continue;

		if (dir_reply == 1 && a_fam == 2) {
			/* de-nat */
			if ((q = strstr(p + x, "src=")) == NULL)
				continue;
			if (sscanf(q, "src=%s dst=%s sport=%s dport=%s", a_dst, a_src, a_dport, a_sport) != 4)
				continue;
		}

		if (dir_reply == 1)
			web_printf("%c[%u,'%s','%s','%s','%s',%li,%li,1]", comma, a_proto, a_dst, a_src, a_dport, a_sport, inbytes, outbytes);
		else
			web_printf("%c[%u,'%s','%s','%s','%s',%li,%li,0]", comma, a_proto, a_src, a_dst, a_sport, a_dport, outbytes, inbytes);

		comma = ',';
	}
	web_puts("];\n");

	fclose(a);
	fclose(b);
}

static void retrieveRatesFromTc(const char* deviceName, unsigned long ratesArray[])
{
	FILE *f;
	char s[256];
	unsigned long u;
	char *e;
	int n;

	snprintf(s, sizeof(s), "tc -s class ls dev %s", deviceName);
	if ((f = popen(s, "r")) != NULL) {
		n = 1;
		while (fgets(s, sizeof(s), f)) {
			if (strncmp(s, "class htb 1:", 12) == 0)
				n = atoi(s + 12);
			else if (strncmp(s, " rate ", 6) == 0) {
				if ((n % 10) == 0) {
					n /= 10;
					if ((n >= 1) && (n <= 10)) {
						u = strtoul(s + 6, &e, 10);
						if (*e == 'K')
							u *= 1000;
						else if (*e == 'M')
							u *= 1000 * 1000;

						ratesArray[n - 1] = u;
						n = 1;
					}
				}
			}
		}
		pclose(f);
	}
}

void asp_qrate(int argc, char **argv)
{
	unsigned long rates[10];
	char buf[32];
	unsigned int n, i;
	char *a[1];

	a[0] = "1";
	asp_ctcount(1, a);

	web_puts("\nvar qrates_out = [];");
	web_puts("\nvar qrates_in = [];");

	for (i = 1; i <= MWAN_MAX; i++) {
		memset(rates, 0, sizeof(rates));
		memset(buf, 0, sizeof(buf));
		snprintf(buf, sizeof(buf), (i == 1 ? "wan" : "wan%u"), i);
		retrieveRatesFromTc(get_wanface(buf), rates);

		snprintf(buf, sizeof(buf), "\nqrates_out[%u] = [0", i);
		web_puts(buf);
		for (n = 0; n < 10; ++n) {
			web_printf(",%lu", rates[n]);
		}
		web_puts("];");

		memset(rates, 0, sizeof(rates));
		memset(buf, 0, sizeof(buf));
#ifdef TCONFIG_BCMARM
		snprintf(buf, sizeof(buf), "ifb%u", (i - 1));
#else
		snprintf(buf, sizeof(buf), "imq%u", (i - 1));
#endif
		retrieveRatesFromTc(buf, rates);

		memset(buf, 0, sizeof(buf));
		snprintf(buf, sizeof(buf), "\nqrates_in[%u] = [0", i);
		web_puts(buf);
		for (n = 0; n < 10; ++n) {
			web_printf(",%lu", rates[n]);
		}
		web_puts("];");
	}
}

#ifndef TOMATO64
static void layer7_list(const char *path, int *first)
{
	DIR *dir;
	struct dirent *de;
	char *p;
	char name[NAME_MAX];

	if ((dir = opendir(path)) != NULL) {
		while ((de = readdir(dir)) != NULL) {
			strlcpy(name, de->d_name, sizeof(name));
			if ((p = strstr(name, ".pat")) == NULL)
				continue;

			*p = 0;
			web_printf("%s'%s'", *first ? "" : ",", name);
			*first = 0;
		}
		closedir(dir);
	}
}

void asp_layer7(int argc, char **argv)
{
	int first = 1;
	web_puts("\nlayer7 = [");
	layer7_list("/etc/l7-extra", &first);
	layer7_list("/etc/l7-protocols", &first);
	web_puts("];\n");
}
#endif /* TOMATO64 */

#ifdef TOMATO64
void asp_ndpi(int argc, char **argv)
{
	FILE *f;
	char proto[50];
	int first = 1;

	const char cmd[] = "/usr/sbin/iptables -m ndpi --help |/usr/bin/tail -n +$(( 1 + $(/usr/sbin/iptables -m ndpi --help |/bin/grep -n \"Enabled protocols\"|/usr/bin/tail -n1|/usr/bin/cut -d: -f1) )) |/usr/bin/xargs -n1";

	web_puts("\nndpi = [");
	if ((f = popen(cmd, "r")) != NULL) {
		while (fgets(proto, sizeof(proto), f)) {
			proto[strcspn(proto, "\n")] = '\0';
			web_printf("%s'%s'", first ? "" : ",", proto);
			first = 0;
		}
		pclose(f);
	}
	web_puts("];\n");
}

void asp_wireless(int argc, char **argv)
{
	FILE *f;
	char row[50];

	const char cmd[] = "/usr/bin/enumerate-phy.sh";

	web_puts("\nwireless = {\n");
	if ((f = popen(cmd, "r")) != NULL) {
		while (fgets(row, sizeof(row), f)) {
			web_printf("%s", row);
		}
		pclose(f);
	}
	web_puts("};\n");
}
#endif /* TOMATO64 */

void wo_expct(char *url)
{
	f_write_string("/proc/net/expire_early", "15", 0, 0);
}
