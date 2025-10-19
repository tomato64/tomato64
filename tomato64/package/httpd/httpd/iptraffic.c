/*
 *
 * IPTraffic monitoring extensions for Tomato
 * Copyright (C) 2011-2012 Augusto Bott
 *
 * Tomato Firmware
 * Copyright (C) 2006-2009 Jonathan Zarate
 *
 * Fixes/updates (C) 2018 - 2025 pedro
 * https://freshtomato.org/
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */


#include "tomato.h"
#include "shared.h"

#include <arpa/inet.h>
#include "iptraffic.h"


static void iptraffic_conntrack_init(void)
{
	FILE *a;
	char sa[256], sb[256];
	char a_src[INET_ADDRSTRLEN], a_dst[INET_ADDRSTRLEN], b_src[INET_ADDRSTRLEN],  b_dst[INET_ADDRSTRLEN];
	unsigned int a_time, a_proto;
	char *p, *next;
	int x;
	unsigned long rip[BRIDGE_COUNT], lan[BRIDGE_COUNT], mask[BRIDGE_COUNT];
	unsigned short int br;
	char ipaddr[INET_ADDRSTRLEN];
	char skip;
	Node tmp;
	Node *ptr;
#ifndef TOMATO64
	const char conntrack[] = "/proc/net/ip_conntrack";
#else
	const char conntrack[] = "/proc/net/nf_conntrack";
	char a_address[5];
#endif /* TOMATO64 */

	for(br = 0; br < BRIDGE_COUNT; br++) {
		char bridge[2] = "0";
		if (br != 0)
			bridge[0] += br;
		else
			memset(bridge, 0, sizeof(bridge));

		memset(sa, 0, sizeof(sa));
		snprintf(sa, sizeof(sa), "lan%s_ifname", bridge);

		if (strcmp(nvram_safe_get(sa), "") != 0) {
			memset(sa, 0, sizeof(sa));
			snprintf(sa, sizeof(sa), "lan%s_ipaddr", bridge);
			rip[br] = inet_addr(nvram_safe_get(sa));

			memset(sa, 0, sizeof(sa));
			snprintf(sa, sizeof(sa), "lan%s_netmask", bridge);
			mask[br] = inet_addr(nvram_safe_get(sa));
			lan[br] = rip[br] & mask[br];
		}
		else {
			mask[br] = 0;
			rip[br] = 0;
			lan[br] = 0;
		}
	}

	if (!(a = fopen(conntrack, "r")))
		return;

	ctvbuf(a); /* if possible, read in one go */

	while (fgets(sa, sizeof(sa), a)) {
#ifndef TOMATO64
		if (sscanf(sa, "%*s %u %u", &a_proto, &a_time) != 2)
			continue;
#else
		if (sscanf(sa, "%4s %*u %*s %u %u", a_address, &a_proto, &a_time) != 3)
			continue;
		if (strncmp(a_address, "ipv4", 4) != 0)
			continue;
#endif /* TOMATO64 */

		if ((a_proto != 6) && (a_proto != 17))
			continue;

		if ((p = strstr(sa, "src=")) == NULL)
			continue;

		if (sscanf(p, "src=%s dst=%s %n", a_src, a_dst, &x) != 2)
			continue;

		p += x;

		if ((p = strstr(p, "src=")) == NULL)
			continue;

		if (sscanf(p, "src=%s dst=%s", b_src, b_dst) != 2)
			continue;

		snprintf(sb, sizeof(sb), "%s %s %s %s", a_src, a_dst, b_src, b_dst);
		remove_dups(sb, sizeof(sb));

		next = NULL;

		foreach(ipaddr, sb, next) {
			skip = 1;
			for (br = 0; br < BRIDGE_COUNT; br++) {
				if ((mask[br] != 0) && ((inet_addr(ipaddr) & mask[br]) == lan[br])) {
					skip = 0;
					break;
				}
			}
			if (skip == 1)
				continue;

			strlcpy(tmp.ipaddr, ipaddr, INET_ADDRSTRLEN);
			ptr = TREE_FIND(&tree, _Node, linkage, &tmp);

			if (!ptr) {
				//_dprintf("%s: new ip: %s\n", __FUNCTION__, ipaddr);
				TREE_INSERT(&tree, _Node, linkage, Node_new(ipaddr));
				ptr = TREE_FIND(&tree, _Node, linkage, &tmp);
			}
			if (a_proto == 6)
				++ptr->tcp_conn;

			if (a_proto == 17)
				++ptr->udp_conn;
		}
	}
	fclose(a);
}

void asp_iptraffic(int argc, char **argv)
{
	FILE *a;
	char sa[256];
	char ip[INET_ADDRSTRLEN];
	char *exclude;
	unsigned long tx_bytes, rx_bytes;
	unsigned long tp_tcp, rp_tcp;
	unsigned long tp_udp, rp_udp;
	unsigned long tp_icmp, rp_icmp;
	unsigned int ct_tcp, ct_udp;
	char comma, br;
	Node tmp;
	Node *ptr;
	char name[] = "/proc/net/ipt_account/lanX";

	exclude = nvram_safe_get("cstats_exclude");

	iptraffic_conntrack_init();

	web_puts("\n\niptraffic=[");
	comma = ' ';

	for (br = 0; br < BRIDGE_COUNT; br++) {
		char bridge[2] = "0";
		if (br != 0)
			bridge[0] += br;
		else
			memset(bridge, 0, sizeof(bridge));

		snprintf(name, sizeof(name), "/proc/net/ipt_account/lan%s", bridge);

		if (!(a = fopen(name, "r")))
			continue;

		fgets(sa, sizeof(sa), a); /* network */
		while (fgets(sa, sizeof(sa), a)) {
			if (sscanf(sa, "ip = %s bytes_src = %lu %*u %*u %*u %*u packets_src = %*u %lu %lu %lu %*u bytes_dst = %lu %*u %*u %*u %*u packets_dst = %*u %lu %lu %lu %*u time = %*u",
			    ip, &tx_bytes, &tp_tcp, &tp_udp, &tp_icmp, &rx_bytes, &rp_tcp, &rp_udp, &rp_icmp) != 9)
				continue;

			if (find_word(exclude, ip))
				continue;

			if ((tx_bytes > 0) || (rx_bytes > 0)) {
				strlcpy(tmp.ipaddr, ip, INET_ADDRSTRLEN);
				ptr = TREE_FIND(&tree, _Node, linkage, &tmp);
				if (!ptr) {
					ct_tcp = 0;
					ct_udp = 0;
				}
				else {
					ct_tcp = ptr->tcp_conn;
					ct_udp = ptr->udp_conn;
				}
				web_printf("%c['%s', %lu, %lu, %lu, %lu, %lu, %lu, %lu, %lu, %u, %u]", comma, ip, rx_bytes, tx_bytes, rp_tcp, tp_tcp, rp_udp, tp_udp, rp_icmp, tp_icmp, ct_tcp, ct_udp);
				comma = ',';
			}
		}
		fclose(a);
	}
	web_puts("];\n");

	TREE_FORWARD_APPLY(&tree, _Node, linkage, Node_housekeeping, NULL);
	TREE_INIT(&tree, Node_compare);
}
