/*
 * Tomato Firmware
 * Copyright (C) 2006-2008 Jonathan Zarate
 * rate limit & connection limit by conanxu
 * 2011 modified by Victek & Shibby for 2.6 kernel
 * Fixes/updates (C) 2018 - 2023 pedro
 *
 */


#include "rc.h"

#include <sys/stat.h>

/* int chain
 * 1  = MANGLE
 * 2  = NAT
 * 3  = Filter
 */

static char *bwlimitfn = "/etc/bwlimit";

#ifdef TCONFIG_BCMARM
static const char *leaf_qdisc = "fq_codel";
#else
static const char *leaf_qdisc = "sfq perturb 10";
#endif

#define IP_ADDRESS	0
#define MAC_ADDRESS	1
#define IP_RANGE	2


void address_checker(int *address_type, char *ipaddr_old, char *ipaddr, const size_t buf_sz)
{
	char *second_part, *last_dot;
	int length_to_minus, length_to_dot;
	
	second_part = strchr(ipaddr_old, '-');
	if (second_part != NULL) {
		/* ip range */
		*address_type = IP_RANGE;
		if (strchr(second_part + 1, '.') != NULL)
			/* long notation */
			strlcpy(ipaddr, ipaddr_old, buf_sz);
		else {
			/* short notation */
			last_dot = strrchr(ipaddr_old, '.');
			length_to_minus = second_part - ipaddr_old;
			length_to_dot = last_dot - ipaddr_old;

			strlcpy(ipaddr, ipaddr_old, length_to_minus + 2);
			strlcpy(ipaddr + length_to_minus + 1, ipaddr, length_to_dot + 2);
			strlcpy(ipaddr + length_to_minus + length_to_dot + 2, second_part + 1, buf_sz - length_to_minus - length_to_dot - 2);
		}
	}
	else {
		/* mac address or ip address */
		if (strlen(ipaddr_old) != 17)
			/* IP_ADDRESS */
			*address_type = IP_ADDRESS;
		else
			/* MAC ADDRESS */
			*address_type = MAC_ADDRESS;

		strlcpy(ipaddr, ipaddr_old, buf_sz);
	}
}

void ipt_bwlimit(int chain)
{
	char *buf;
	char *g;
	char *p;
	char p1[128];
	char seq[32];			/* mark number */
	int iSeq = 1;
	char *ipaddr_old;
	char ipaddr[32];		/* ip address */
	char *dlrate, *dlceil;		/* guaranteed rate & maximum rate for download */
	char *ulrate, *ulceil;		/* guaranteed rate & maximum rate for upload */
	char *priority;			/* priority */
	char *lanipaddr;		/* lan ip address */
	char *lanmask;			/* lan netmask */
	char *lanX_ipaddr;		/* (br1 - br3) */
	char *lanX_mask;		/* (br1 - br3) */
	char *tcplimit, *udplimit;	/* tcp connection limit & udp packets per second */
	char *description;		/* description */
	char *enabled;
	int priority_num;
	char *bwl_br0_tcp, *bwl_br0_udp;
	int i, address_type;

	if (!nvram_get_int("bwl_enable"))
		return;

	/* read bwl rules from nvram */
	g = buf = strdup(nvram_safe_get("bwl_rules"));

	lanipaddr = nvram_safe_get("lan_ipaddr");
	lanmask = nvram_safe_get("lan_netmask");

	bwl_br0_tcp = nvram_safe_get("bwl_br0_tcp");
	bwl_br0_udp = nvram_safe_get("bwl_br0_udp");

	/* MANGLE */
	if (chain == 1) {
		if (nvram_get_int("bwl_br0_enable") == 1)
			/* These mark values and masks have been intentionally chosen to avoid conflicting
			 * with qos, wan pbr, and vpnrouting marks. See qos.c
			 */
			ipt_write("-A POSTROUTING ! -s %s/%s -d %s/%s -j MARK --set-mark 0x10/0xf0\n"
			          "-A PREROUTING -s %s/%s ! -d %s/%s -j MARK --set-mark 0x10/0xf0\n",
			          lanipaddr, lanmask, lanipaddr, lanmask,
			          lanipaddr, lanmask, lanipaddr, lanmask);

		/* br[1-(BRIDGE_COUNT - 1)] */
		for (i = 1 ; i < BRIDGE_COUNT; i++) {
			char buffer[16];

			snprintf(buffer, sizeof(buffer), "bwl_br%d_enable", i);
			if (nvram_get_int(buffer) == 1) {

				snprintf(buffer, sizeof(buffer), "lan%d_ipaddr", i);
				lanX_ipaddr = nvram_safe_get(buffer);

				snprintf(buffer, sizeof(buffer), "lan%d_netmask", i);
				lanX_mask = nvram_safe_get(buffer);

				ipt_write("-A POSTROUTING ! -s %s/%s -d %s/%s -j MARK --set-mark 0x%d0/0xf0\n"
				          "-A PREROUTING -s %s/%s ! -d %s/%s -j MARK --set-mark 0x%d0/0xf0\n",
				          lanX_ipaddr, lanX_mask, lanX_ipaddr, lanX_mask, (i + 1),
				          lanX_ipaddr, lanX_mask, lanX_ipaddr, lanX_mask, (i + 1));
			}
		}
	}

	/* NAT */
	if (chain == 2) {
		if (nvram_get_int("bwl_br0_enable") == 1) {
#ifndef TCONFIG_BCMARM
			if (nvram_get_int("bwl_br0_tcp") > 0)
				ipt_write("-A PREROUTING -s %s/%s -p tcp --syn -m connlimit --connlimit-above %s -j DROP\n", lanipaddr, lanmask, bwl_br0_tcp);
#endif
			if (nvram_get_int("bwl_br0_udp") > 0)
				ipt_write("-A PREROUTING -s %s/%s -p udp -m limit --limit %s/sec -j ACCEPT\n", lanipaddr, lanmask, bwl_br0_udp);
		}
	}

#ifdef TCONFIG_BCMARM
	/* Filter */
	if (chain == 3) {
		if (nvram_get_int("bwl_br0_enable") == 1) {
			if (nvram_get_int("bwl_br0_tcp") > 0)
				ipt_write("-A FORWARD -s %s/%s -p tcp --syn -m connlimit --connlimit-above %s -j DROP\n", lanipaddr, lanmask, bwl_br0_tcp);
		}
	}
#endif

	while (g) {
		/*
		 * old: ipaddr_old<dlrate<dlceil<ulrate<ulceil<priority<tcplimit<udplimit>
		 * new: enabled<ipaddr_old<dlrate<dlceil<ulrate<ulceil<priority<tcplimit<udplimit<description>
		*/
		if ((p = strsep(&g, ">")) == NULL)
			break;

		strlcpy(p1, p, sizeof(p1));
		i = vstrsep(p, "<", &enabled, &ipaddr_old, &dlrate, &dlceil, &ulrate, &ulceil, &priority, &tcplimit, &udplimit, &description);
		if (i < 10) {
			i = vstrsep(p1, "<", &ipaddr_old, &dlrate, &dlceil, &ulrate, &ulceil, &priority, &tcplimit, &udplimit); /* compat */
			if (i < 8)
				continue;
		}
		if (i == 10 && *enabled && atoi(enabled) != 1)
			continue;

		priority_num = atoi(priority);
		if ((priority_num < 0) || (priority_num > 5))
			continue;
		if (!strcmp(ipaddr_old, ""))
			continue;
		
		address_checker(&address_type, ipaddr_old, ipaddr, sizeof(ipaddr));
		memset(seq, 0, sizeof(seq));
		snprintf(seq, sizeof(seq), "0x%x/0xff00000", iSeq << 20);
		iSeq++;

		if (!strcmp(dlceil, ""))
			dlceil = dlrate;

		if (strcmp(dlrate, "") && strcmp(dlceil, "")) {
			if (chain == 1) {
				switch (address_type) {
					case IP_ADDRESS:
						ipt_write("-A POSTROUTING ! -s %s/%s -d %s -j MARK --set-mark %s\n", lanipaddr, lanmask, ipaddr, seq);
						break;
					case MAC_ADDRESS:
						break;
					case IP_RANGE:
						ipt_write("-A POSTROUTING ! -s %s/%s -m iprange --dst-range %s -j MARK --set-mark %s\n", lanipaddr, lanmask, ipaddr, seq);
						break;
				}
			}
		}

		if (!strcmp(ulceil, ""))
			ulceil = ulrate;

		if (strcmp(ulrate, "") && strcmp(ulceil, "")) {
			if (chain == 1) {
				switch (address_type) {
					case IP_ADDRESS:
						ipt_write("-A PREROUTING -s %s ! -d %s/%s -j MARK --set-mark %s\n", ipaddr, lanipaddr, lanmask, seq);
						break;
					case MAC_ADDRESS:
						ipt_write("-A PREROUTING -m mac --mac-source %s ! -d %s/%s  -j MARK --set-mark %s\n", ipaddr, lanipaddr, lanmask, seq);
						break;
					case IP_RANGE:
						ipt_write("-A PREROUTING -m iprange --src-range %s ! -d %s/%s -j MARK --set-mark %s\n", ipaddr, lanipaddr, lanmask, seq);
						break;
				}
			}
		}

		if (atoi(tcplimit) > 0) {
#ifdef TCONFIG_BCMARM
			if (chain == 3) {
#else
			if (chain == 2) {
#endif
				switch (address_type) {
					case IP_ADDRESS:
#ifdef TCONFIG_BCMARM
						ipt_write("-A FORWARD -s %s -p tcp --syn -m connlimit --connlimit-above %s -j DROP\n", ipaddr, tcplimit);
#else
						ipt_write("-A PREROUTING -s %s -p tcp --syn -m connlimit --connlimit-above %s -j DROP\n", ipaddr, tcplimit);
#endif
						break;
					case MAC_ADDRESS:
#ifdef TCONFIG_BCMARM
						ipt_write("-A FORWARD -m mac --mac-source %s -p tcp --syn -m connlimit --connlimit-above %s -j DROP\n", ipaddr, tcplimit);
#else
						ipt_write("-A PREROUTING -m mac --mac-source %s -p tcp --syn -m connlimit --connlimit-above %s -j DROP\n", ipaddr, tcplimit);
#endif
						break;
					case IP_RANGE:
#ifdef TCONFIG_BCMARM
						ipt_write("-A FORWARD -m iprange --src-range %s -p tcp --syn -m connlimit --connlimit-above %s -j DROP\n", ipaddr, tcplimit);
#else
						ipt_write("-A PREROUTING -m iprange --src-range %s -p tcp --syn -m connlimit --connlimit-above %s -j DROP\n", ipaddr, tcplimit);
#endif
						break;
				}
			}
		}

		if (atoi(udplimit) > 0) {
			if (chain == 2) {
				switch (address_type) {
					case IP_ADDRESS:
						ipt_write("-A PREROUTING -s %s -p udp -m limit --limit %s/sec -j ACCEPT\n", ipaddr, udplimit);
						break;
					case MAC_ADDRESS:
						ipt_write("-A PREROUTING -m mac --mac-source %s -p udp -m limit --limit %s/sec -j ACCEPT\n", ipaddr, udplimit);
						break;
					case IP_RANGE:
						ipt_write("-A PREROUTING -m iprange --src-range %s -p udp -m limit --limit %s/sec -j ACCEPT\n", ipaddr, udplimit);
						break;
				}
			}
		}
	}
	free(buf);
}

void start_bwlimit(void)
{
	FILE *tc;
	char *buf;
	char *g;
	char *p;
	char p1[128];
	char *ibw, *obw;		/* bandwidth */
	int mark;			/* mark number */
	int seq;
	int iSeq = 1;
	char *ipaddr_old; 
	char ipaddr[30];		/* ip address */
	char *dlrate, *dlceil;		/* guaranteed rate & maximum rate for download */
	char *ulrate, *ulceil;		/* guaranteed rate & maximum rate for upload */
	char *priority;			/* priority */
	char *tcplimit, *udplimit;	/* tcp connection limit & udp packets per second */
	char *description;		/* description */
	char *enabled;
	int priority_num;
	char *dlr, *dlc, *ulr, *ulc, *prio; /* download / upload - rate / ceiling / prio */
	int i, address_type;
	int s[6];
	char *waniface;

	if (!nvram_get_int("bwl_enable"))
		return;

	/* read bwl rules from nvram */
	g = buf = strdup(nvram_safe_get("bwl_rules"));

	ibw = nvram_safe_get("wan_qos_ibw"); /* read from QOS setting */
	obw = nvram_safe_get("wan_qos_obw"); /* read from QOS setting */

	waniface = nvram_safe_get("wan_iface");

	if ((tc = fopen(bwlimitfn, "w")) == NULL) {
		logerr(__FUNCTION__, __LINE__, bwlimitfn);
		return;
	}

	fprintf(tc, "#!/bin/sh\n"
	            "\n"
	            "TCA=\"tc class add dev br0\"\n"
	            "TFA=\"tc filter add dev br0\"\n"
	            "TQA=\"tc qdisc add dev br0\"\n"
	            "TCAU=\"tc class add dev %s\"\n"
	            "TFAU=\"tc filter add dev %s\"\n"
	            "TQAU=\"tc qdisc add dev %s\"\n"
	            "Q=\"%s\"\n"
	            "\n"
	            "case \"$1\" in\n"
	            "start)\n"
	            "\ttc qdisc del dev br0 root 2>/dev/null\n"
	            "\t[ \"$(nvram get qos_enable)\" == \"0\" ] && {\n"
	            "\t\ttc qdisc del dev %s root 2>/dev/null\n"
	            "\t}\n"
	            "\n"
	            "\t$TQA root handle 1: htb\n"
	            "\t$TCA parent 1: classid 1:1 htb rate %skbit\n"
	            "\n"
	            "\t[ \"$(nvram get qos_enable)\" == \"0\" ] && {\n"
	            "\t\t$TQAU root handle 2: htb\n"
	            "\t\t$TCAU parent 2: classid 2:1 htb rate %skbit\n"
	            "\t}\n"
	            "\n",
	            waniface,
	            waniface,
	            waniface,
	            leaf_qdisc,
	            waniface,
	            ibw,
	            obw);

	while (g) {
		/*
		 * old: ipaddr_old<dlrate<dlceil<ulrate<ulceil<priority<tcplimit<udplimit>
		 * new: enabled<ipaddr_old<dlrate<dlceil<ulrate<ulceil<priority<tcplimit<udplimit<description>
		*/
		if ((p = strsep(&g, ">")) == NULL)
			break;

		strlcpy(p1, p, sizeof(p1));
		i = vstrsep(p, "<", &enabled, &ipaddr_old, &dlrate, &dlceil, &ulrate, &ulceil, &priority, &tcplimit, &udplimit, &description);
		if (i < 10) {
			i = vstrsep(p1, "<", &ipaddr_old, &dlrate, &dlceil, &ulrate, &ulceil, &priority, &tcplimit, &udplimit); /* compat */
			if (i < 8)
				continue;
		}
		if (i == 10 && *enabled && atoi(enabled) != 1)
			continue;

		priority_num = atoi(priority);
		if ((priority_num < 0) || (priority_num > 5))
			continue;
		if (!strcmp(ipaddr_old, ""))
			continue;

		/* increment the priority number by 1 because for filters prio = 0 will make it lowest priority */
		priority_num++;
		sprintf(priority, "%d", priority_num);

		address_checker(&address_type, ipaddr_old, ipaddr, sizeof(ipaddr));
		seq = iSeq * 10;
		mark = iSeq << 20;
		iSeq++;

		if (!strcmp(dlceil, ""))
			dlceil = dlrate;

		if (strcmp(dlrate, "") && strcmp(dlceil, "")) {
			fprintf(tc, "\t$TCA parent 1:1 classid 1:%d htb rate %skbit ceil %skbit prio %s\n"
			            "\t$TQA parent 1:%d handle %d: $Q\n",
			            seq, dlrate, dlceil, priority,
			            seq, seq);

			if (address_type != MAC_ADDRESS)
				fprintf(tc, "\t$TFA parent 1:0 prio %s protocol all handle 0x%x/0xff00000 fw flowid 1:%d\n\n", priority, mark, seq);
			else if (address_type == MAC_ADDRESS) {
				sscanf(ipaddr, "%02X:%02X:%02X:%02X:%02X:%02X", &s[0], &s[1], &s[2], &s[3], &s[4], &s[5]);

				fprintf(tc, "\t$TFA parent 1:0 protocol all prio %s u32 match u16 0x0800 0xFFFF at -2 match u32 0x%02X%02X%02X%02X 0xFFFFFFFF at -12 match u16 0x%02X%02X 0xFFFF at -14 flowid 1:%d\n\n",
				            priority, s[2], s[3], s[4], s[5], s[0], s[1], seq);
			}
		}

		if (!strcmp(ulceil, ""))
			ulceil = dlrate;

		if (strcmp(ulrate, "") && strcmp(ulceil, ""))
			fprintf(tc, "\t[ \"$(nvram get qos_enable)\" == \"0\" ] && {\n"
			            "\t\t$TCAU parent 2:1 classid 2:%d htb rate %skbit ceil %skbit prio %s\n"
			            "\t\t$TQAU parent 2:%d handle %d: $Q\n"
			            "\t\t$TFAU parent 2:0 prio %s protocol all handle 0x%x/0xff00000 fw flowid 2:%d\n"
			            "\t}\n"
			            "\n",
			            seq, ulrate, ulceil, priority,
			            seq, seq,
			            priority, mark, seq);
	} /* while */
	free(buf);

	/* the order matters! first rules for BWL for br0 (above) */

	/* limit br0 */
	dlr = nvram_safe_get("bwl_br0_dlr");		/* download rate */
	dlc = nvram_safe_get("bwl_br0_dlc");		/* download ceiling */
	ulr = nvram_safe_get("bwl_br0_ulr");		/* upload rate */
	ulc = nvram_safe_get("bwl_br0_ulc");		/* upload ceiling */
	prio = nvram_safe_get("bwl_br0_prio");		/* priority */
	if ((nvram_get_int("bwl_br0_enable") == 1) && strcmp(dlr, "") && strcmp(ulr, "")) {
		if (!strcmp(dlc, ""))
			dlc = dlr;
		if (!strcmp(ulc, ""))
			ulc = ulr;

		/* for br0 only, we need to shift the priority values to be after 1 - 5 used for listed IPs/MACs
		 * to prevent conflict between filters (there cannot be 2 filters with same class parent with
		 * same pref with different fwmark). Just add a 1 on the left of the prio to make it 10-14.
		 */
		fprintf(tc, "\t$TCA parent 1:1 classid 1:16 htb rate %skbit ceil %skbit prio %s\n"
		            "\t$TQA parent 1:16 handle 16: $Q\n"
		            "\t$TFA parent 1:0 prio 1%s protocol all handle 0x10/0xf0 fw flowid 1:16\n"
		            "\n"
		            "\t[ \"$(nvram get qos_enable)\" == \"0\" ] && {\n"
		            "\t\t$TCAU parent 2:1 classid 2:16 htb rate %skbit ceil %skbit prio %s\n"
		            "\t\t$TQAU parent 2:16 handle 16: $Q\n"
		            "\t\t$TFAU parent 2:0 prio 1%s protocol all handle 0x10/0xf0 fw flowid 2:16\n"
		            "\t}\n"
		            "\n",
		            dlr, dlc, prio,
		            prio,
		            ulr, ulc, prio,
		            prio);
	}

	for (i = 1 ; i < BRIDGE_COUNT; i++) {
		char buffer[16];
		int id1 = ((2 * i) +2); // br[1-(BRIDGE_COUNT - 1)] (4, 6, 8, 10, 12, 14, 16)
		int id2 =((16 * (i - 1)) +32); // br[1-(BRIDGE_COUNT - 1)] (32, 48, 64, 80, 96, 112, 128)

		snprintf(buffer, sizeof(buffer), "bwl_br%d_enable", i);
		if (nvram_get_int(buffer) == 1) {

			snprintf(buffer, sizeof(buffer), "bwl_br%d_dlr", i);
			dlr = nvram_safe_get(buffer);			/* download rate */

			snprintf(buffer, sizeof(buffer), "bwl_br%d_dlc", i);
			dlc = nvram_safe_get(buffer);			/* download ceiling */

			snprintf(buffer, sizeof(buffer), "bwl_br%d_ulr", i);
			ulr = nvram_safe_get(buffer);			/* upload rate */

			snprintf(buffer, sizeof(buffer), "bwl_br%d_ulc", i);
			ulc = nvram_safe_get(buffer);			/* upload ceiling */

			snprintf(buffer, sizeof(buffer), "bwl_br%d_prio", i);
			prio = nvram_safe_get(buffer);			/* priority */

			if (!strcmp(dlc, ""))
				dlc = dlr;
			if (!strcmp(ulc, ""))
				ulc = ulr;

			/* download for br[1-(BRIDGE_COUNT -1)] */
			fprintf(tc, "\tTCA%d=\"tc class add dev br%d\"\n"
			            "\tTFA%d=\"tc filter add dev br%d\"\n"
			            "\tTQA%d=\"tc qdisc add dev br%d\"\n"
			            "\ttc qdisc del dev br%d root\n"
			            "\ttc qdisc add dev br%d root handle %d: htb\n"
			            "\ttc class add dev br%d parent %d: classid %d:1 htb rate %skbit\n"
			            "\t$TCA%d parent %d:1 classid %d:%d htb rate %skbit ceil %skbit prio %s\n"
			            "\t$TQA%d parent %d:%d handle %d: $Q\n"
			            "\t$TFA%d parent %d:0 prio %s protocol all handle 0x%d0/0xf0 fw flowid %d:%d\n",
			            i, i,
			            i, i,
			            i, i,
			            i,
			            i, id1,
			            i, id1, id1, ibw,
			            i, id1, id1, id2, dlr, dlc, prio,
			            i, id1, id2, id2,
			            i, id1, prio, (i + 1), id1, id2);

			/* upload for br[1-(BRIDGE_COUNT - 1)] */
			fprintf(tc, "\t[ \"$(nvram get qos_enable)\" == \"0\" ] && {\n"
			            "\t\t$TCAU parent 2:1 classid 2:%d htb rate %skbit ceil %skbit prio %s\n"
			            "\t\t$TQAU parent 2:%d handle %d: $Q\n"
			            "\t\t$TFAU parent 2:0 prio %s protocol all handle 0x%d0/0xf0 fw flowid 2:%d\n"
			            "\t}\n\n",
			            id2, ulr, ulc, prio,
			            id2, id2,
			            prio, (i + 1), id2);
		}
	}


	fprintf(tc, "\tlogger -t bwlimit \"BW Limiter is started\"\n"
	            "\t;;\n"
	            "stop)\n"
	            "\t[ \"$(nvram get qos_enable)\" == \"0\" ] && {\n"
	            "\t\ttc qdisc del dev %s root\n"
	            "\t}\n",
	            waniface);

	for (i = 0 ; i < BRIDGE_COUNT; i++) {
		fprintf(tc, "\ttc qdisc del dev br%d root 2>/dev/null\n", i);
	}
	fprintf(tc, "\n");

	fprintf(tc, "\tlogger -t bwlimit \"BW Limiter is stopped\"\n"
	            "\t;;\n"
	            "*)\n"
	            "\techo \"Usage: $0 <start|stop>\"\n"
	            "esac\n");

	fclose(tc);

	chmod(bwlimitfn, 0700);

	eval(bwlimitfn, "start");
}

void stop_bwlimit(void)
{
	FILE *tc;

	if ((tc = fopen(bwlimitfn, "r")) == NULL)
		return;

	fclose(tc);

	eval(bwlimitfn, "stop");
}

/*
 * PREROUTING (mn) ----> x ----> FORWARD (f) ----> + ----> POSTROUTING (n)
 *            QD         |                         ^
 *                       |                         |
 *                       v                         |
 *                    INPUT (f)                 OUTPUT (mnf)
 */
