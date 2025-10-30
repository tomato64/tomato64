/*
 *
 * Tomato Firmware
 * Copyright (C) 2006-2009 Jonathan Zarate
 * Fixes/updates (C) 2018 - 2025 pedro
 *
 */


#include "rc.h"

#include <sys/stat.h>

#define CLASSES_NUM	10
static char qosfn[] = "/etc/wanX_qos";
static char qosdev[] = "iXXX";
static int qos_wan_num = 0;
static int qos_rate_start_index = 0;

#ifdef TCONFIG_BCMARM
const char disabled_classification_rules[] = "0<<-2<a<<0<<<<0<Default";
const char *rate_part = "5-100,0-0,0-0,0-0,0-0,0-0,0-0,0-0,0-0,0-0,";

static char *build_disabled_classification_rates(int mwan_count) {
	int i;
	size_t part_len = strlen(rate_part);
	size_t total_len = mwan_count * part_len;

	char *buffer = malloc(total_len);
	if (!buffer) {
		syslog(LOG_ERR, "qos: failed allocating memory");
		return NULL;
	}

	char *ptr = buffer;

	for (i = 0; i < mwan_count; ++i) {
		memcpy(ptr, rate_part, part_len);
		ptr += part_len;
	}
	*(ptr - 1) = '\0'; /* remove last comma */

	return buffer;
}
#endif

static void prep_qosstr(char *prefix)
{
	int i;
	char buf[8];

	for (i = 1; i <= MWAN_MAX; i++) {
		memset(buf, 0, sizeof(buf));
		snprintf(buf, sizeof(buf), (i == 1 ? "wan" : "wan%d"), i);
		if (!strcmp(prefix, buf)) {
			snprintf(buf, sizeof(buf), (i == 1 ? "/etc/wan_qos" : "/etc/wan%d_qos"), i);
			strlcpy(qosfn, buf, sizeof(qosfn));
			qos_wan_num = i;
			qos_rate_start_index = (i - 1) * 10;
#ifdef TCONFIG_BCMARM
			snprintf(buf, sizeof(buf), "ifb%d", (i - 1));
			strlcpy(qosdev, buf, sizeof(qosdev));
#else
			snprintf(buf, sizeof(buf), "imq%d", (i - 1));
			strlcpy(qosdev, buf, sizeof(qosdev));
#endif
		}
	}
}

/* in mangle table */
void ipt_qos(void)
{
	/*
	 * Mark bytes: F FF FF F F F
	 *             ^ ^^ ^^ ^ | ^---- QoS priority (1-10) - matched by tc filter
	 *             | |  |  | |______ Bandwidth limiter brX general rule (matched by tc filter)
	 *             | |  |  |________ 1-8 WAN number (PBR) - matched by ip rule (see pbr.c)
	 *             | |  |            9-15 OpenVPN connection number (PBR) - matched by ip rule (see vpnrouting.sh)
	 *             | |  |___________ QoS size group - matched by iptables to clear mark after max bytes passed.
	 *             | |               255 is a reserved value which indicates to disable skipping QOSO even if
	 *             | |               the connection got classified (needed for all rules after L7 rule or size range
	 *             | |               with max rule and all after it)
	 *             | |______________ Bandwidth limiter br0 specific rules (matched by tc filter)
	 *             |________________ Unused.
	 */

	char *buf;
	char *g;
	char *p;
	char *addr_type, *addr;
	char *proto;
	char *port_type, *port;
	char *class_prio;
#ifndef TOMATO64
	char *ipp2p, *layer7;
#endif /* TOMATO64 */
#ifdef TOMATO64
	char *ndpi;
#endif /* TOMATO64 */
	char *bcount;
	char *dscp;
	char *desc;
	int class_num;
	int proto_num;
	int v4v6_ok;
	int i, j;
	char sport[192];
	char saddr[256];
	char end[256];
	char s[32];
	char app[256];
	int inuse;
#ifdef TOMATO64
	int usemacsrc;
#endif /* TOMATO64 */
	const char *chain;
#ifdef TOMATO64
	const char *chain2;
#endif /* TOMATO64 */
	unsigned long min;
	unsigned long max;
	unsigned long prev_max;
	const char *qface;
	int sizegroup;
	int class_flag;
	int rule_num;
	int wanup[MWAN_MAX];
#ifndef TCONFIG_BCMARM
	int qosDevNumStr = 0;
#else
	char *disabled_classification_rates;
#endif

	if (!nvram_get_int("qos_enable"))
		return;

	inuse = 0;
	class_flag = 0;
	sizegroup = 0;
	prev_max = 0;
	rule_num = 0;

	/* Don't reclassify an already classified connection (qos class is in rightmost half-byte of mark)
	 * that also doesn't have a size group. If it has a size group, processing needs to continue
	 * to allow the mark to get cleared after the max bytes have been sent/received by QOSSIZE chain
	 */
	ip46t_write(ipv6_enabled,
	            ":QOSO - [0:0]\n"
	            "-A QOSO -m connmark --mark 0/0xff000 -m connmark ! --mark 0/0xf -j RETURN\n");
#ifdef TOMATO64
	ip46t_write(ipv6_enabled,
	            ":QOSO2 - [0:0]\n"
	            "-A QOSO2 -m connmark --mark 0/0xff000 -m connmark ! --mark 0/0xf -j RETURN\n");
#endif /* TOMATO64 */

#ifdef TCONFIG_BCMARM
	if (!nvram_get_int("qos_classify"))
		g = buf = strdup(disabled_classification_rules);
	else
#endif
		g = buf = strdup(nvram_safe_get("qos_orules"));

	while (g) {

		/*
		 * addr_type<addr<proto<port_type<port<ipp2p<L7<bcount<dscp<class_prio<desc
		 *
		 * addr_type:
		 * 	0 = any
		 * 	1 = dest ip
		 * 	2 = src ip
		 * 	3 = src mac
		 * addr:
		 * 	ip/mac if addr_type == 1-3
		 * proto:
		 * 	0-65535 = protocol
		 * 	-1 = tcp or udp
		 * 	-2 = any protocol
		 * port_type:
		 * 	if proto == -1,tcp,udp:
		 * 		d = dest
		 * 		s = src
		 * 		x = both
		 * 		a = any
		 * port:
		 * 	port # if proto == -1,tcp,udp
		 * bcount:
		 * 	min:max
		 * 	blank = none
		 * dscp:
		 * 	empty - any
		 * 	numeric (0:63) - dscp value
		 * 	afXX, csX, be, ef - dscp class
		 * class_prio:
		 * 	0-10
		 * 	-1 = disabled
		 *
		 */

		if ((p = strsep(&g, ">")) == NULL)
			break;

#ifndef TOMATO64
		i = vstrsep(p, "<", &addr_type, &addr, &proto, &port_type, &port, &ipp2p, &layer7, &bcount, &dscp, &class_prio, &desc);
		rule_num++;
		if (i < 11)
			continue;
#else
		i = vstrsep(p, "<", &addr_type, &addr, &proto, &port_type, &port, &ndpi, &bcount, &dscp, &class_prio, &desc);
		rule_num++;
		if (i < 10)
			continue;

		usemacsrc = 0;
#endif /* TOMATO64 */

		class_num = atoi(class_prio);
		if ((class_num < 0) || (class_num > 9))
			continue;

		i = 1 << class_num;
		++class_num;

		if ((inuse & i) == 0)
			inuse |= i;

		v4v6_ok = IPT_V4;
#ifdef TCONFIG_IPV6
		if (ipv6_enabled)
			v4v6_ok |= IPT_V6;
#endif

		saddr[0] = '\0';
		end[0] = '\0';
		/* mac or ip address */
		if ((*addr_type == '1') || (*addr_type == '2')) { /* match ip */
			v4v6_ok &= ipt_addr(saddr, sizeof(saddr), addr, (*addr_type == '1') ? "dst" : "src", v4v6_ok, (v4v6_ok==IPT_V4), "QoS", desc);
			if (!v4v6_ok)
				continue;
		}
		else if (*addr_type == '3') { /* match mac */
			snprintf(saddr, sizeof(saddr), "-m mac --mac-source %s", addr); /* (-m mac modified, returns !match in OUTPUT) */
#ifdef TOMATO64
		usemacsrc = 1;
#endif /* TOMATO64 */
		}

#ifndef TOMATO64
		/* IPP2P/Layer7 */
		memset(app, 0, sizeof(app));
		if (ipt_ipp2p(ipp2p, app, sizeof(app)))
			v4v6_ok &= ~IPT_V6;
		else
			ipt_layer7(layer7, app, sizeof(app));

		if (app[0]) {
			v4v6_ok &= ~IPT_V6; /* L7 for IPv6 not working either! */
			strlcat(saddr, app, sizeof(saddr));
		}
#endif /* TOMATO64 */

#ifdef TOMATO64
		/* ndpi */
		memset(app, 0, sizeof(app));
		ipt_ndpi(ndpi, app, sizeof(app));

		if (app[0]) {
			strlcat(saddr, app, sizeof(saddr));
		}
#endif /* TOMATO64 */

		/* dscp */
		memset(s, 0, sizeof(s));
		if (ipt_dscp(dscp, s, sizeof(s)))
			strlcat(saddr, s, sizeof(saddr));

		class_flag = 0;

		/* -m connbytes --connbytes x:y --connbytes-dir both --connbytes-mode bytes */
		if (*bcount) {
			min = strtoul(bcount, &p, 10);
			if (*p != 0) {
				strlcat(saddr, " -m connbytes --connbytes-mode bytes --connbytes-dir both --connbytes ", sizeof(saddr));
				++p;
				if (*p == 0)
					snprintf(saddr + strlen(saddr), sizeof(saddr) - strlen(saddr), "%lu:", min * 1024);
				else {
					max = strtoul(p, NULL, 10);
					snprintf(saddr + strlen(saddr), sizeof(saddr) - strlen(saddr), "%lu:%lu", min * 1024, (max * 1024) - 1);
					if (!sizegroup) {
						/* create table of connbytes sizes, pass appropriate connections there and only continue processing them if mark was wiped */
						ip46t_write(ipv6_enabled,
						            ":QOSSIZE - [0:0]\n"
						            "-I QOSO 2 -m connmark ! --mark 0/0xff000 -j QOSSIZE\n"
						            "-I QOSO 3 -m connmark ! --mark 0/0xff000 -m connmark ! --mark 0xff000/0xff000 -j RETURN\n");
#ifdef TOMATO64
						ip46t_write(ipv6_enabled,
						            "-I QOSO2 2 -m connmark ! --mark 0/0xff000 -j QOSSIZE\n"
						            "-I QOSO2 3 -m connmark ! --mark 0/0xff000 -m connmark ! --mark 0xff000/0xff000 -j RETURN\n");
#endif /* TOMATO64 */
					}
					if (max != prev_max && sizegroup < 255) {
						class_flag = ++sizegroup << 12;
						prev_max = max;
						/* Clear the QoS mark to allow the packet to be reclassified. Note: the last used size group will remain left behind so that
						 * we can match on it in the next rule and return from the chain
						 */
						ip46t_flagged_write(ipv6_enabled, v4v6_ok, "-A QOSSIZE -m connmark --mark 0x%x/0xff000 -m connbytes --connbytes-mode bytes --connbytes-dir both "
						                    "--connbytes %lu: -j CONNMARK --set-mark 0xff000/0xff00f\n", (sizegroup << 12), (max * 1024));
						ip46t_flagged_write(ipv6_enabled, v4v6_ok, "-A QOSSIZE -m connmark --mark 0xff000/0xff000 -m connbytes --connbytes-mode bytes --connbytes-dir both "
						                    "--connbytes %lu: -j RETURN\n", (max * 1024));
					}
					else
						class_flag = sizegroup << 12;
				}
			}
			else
				bcount = "";
		}

		/* If any rule requires the QOSSIZE chain, set all subsequent rules to be "non-persistent"
		 * by setting a reserved value for the size group: 0xff. Non-persistent means that
		 * the QOS0 chain will have to be processed for that connection each time
		 */
		if (sizegroup > 0 && class_flag == 0)
			class_flag = 255 << 12;

		chain = "QOSO";
#ifdef TOMATO64
		chain2 = "QOSO2";
#endif /* TOMATO64 */
		class_num |= class_flag;
		snprintf(end + strlen(end), sizeof(end) - strlen(end), " -j CONNMARK --set-mark 0x%x/0xff00f\n", class_num);

		/* If we found a L7 rule, make all next rules follow the case as if we found
		 * a size rule, but do this AFTER the L7 rule mark was computed because L7
		 * classification is final (not like size which has a max after which the
		 * classification is cleared)
		 */
		if (app[0] && sizegroup == 0)
			++sizegroup;

		/* protocol & ports */
		proto_num = atoi(proto);
		if (proto_num > -2) {
			if ((proto_num == 6) || (proto_num == 17) || (proto_num == -1)) {
				if (*port_type != 'a') {
					if ((*port_type == 'x') || (strchr(port, ',')))
						/* dst-or-src port matches, and anything with multiple lists "," use multiport */
						snprintf(sport, sizeof(sport), "-m multiport --%sports %s", (*port_type == 's') ? "s" : ((*port_type == 'd') ? "d" : ""), port);
					else
						/* single or simple x:y range, use built-in tcp/udp match */
						snprintf(sport, sizeof(sport), "--%sport %s", (*port_type == 's') ? "s" : ((*port_type == 'd') ? "d" : ""), port);
				}
				else
					sport[0] = 0;

				if (proto_num != 6) {
					ip46t_flagged_write(ipv6_enabled, v4v6_ok, "-A %s -p %s %s %s %s", chain, "udp", sport, saddr, end);
					ip46t_flagged_write(ipv6_enabled, v4v6_ok, "-A %s -p %s %s %s -j RETURN\n", chain, "udp", sport, saddr);
#ifdef TOMATO64
					if(usemacsrc == 0) {
						ip46t_flagged_write(ipv6_enabled, v4v6_ok, "-A %s -p %s %s %s %s", chain2, "udp", sport, saddr, end);
						ip46t_flagged_write(ipv6_enabled, v4v6_ok, "-A %s -p %s %s %s -j RETURN\n", chain2, "udp", sport, saddr);
					}
#endif /* TOMATO64 */
				}
				if (proto_num != 17) {
					ip46t_flagged_write(ipv6_enabled, v4v6_ok, "-A %s -p %s %s %s %s", chain, "tcp", sport, saddr, end);
					ip46t_flagged_write(ipv6_enabled, v4v6_ok, "-A %s -p %s %s %s -j RETURN\n", chain, "tcp", sport, saddr);
#ifdef TOMATO64
					if(usemacsrc == 0) {
						ip46t_flagged_write(ipv6_enabled, v4v6_ok, "-A %s -p %s %s %s %s", chain2, "tcp", sport, saddr, end);
						ip46t_flagged_write(ipv6_enabled, v4v6_ok, "-A %s -p %s %s %s -j RETURN\n", chain2, "tcp", sport, saddr);
					}
#endif /* TOMATO64 */
				}
			}
			else {
				ip46t_flagged_write(ipv6_enabled, v4v6_ok, "-A %s -p %d %s %s", chain, proto_num, saddr, end);
				ip46t_flagged_write(ipv6_enabled, v4v6_ok, "-A %s -p %d %s -j RETURN\n", chain, proto_num, saddr);
#ifdef TOMATO64
				if(usemacsrc == 0) {
					ip46t_flagged_write(ipv6_enabled, v4v6_ok, "-A %s -p %d %s %s", chain2, proto_num, saddr, end);
					ip46t_flagged_write(ipv6_enabled, v4v6_ok, "-A %s -p %d %s -j RETURN\n", chain2, proto_num, saddr);
				}
#endif /* TOMATO64 */
			}
		}
		else { /* any protocol */
			ip46t_flagged_write(ipv6_enabled, v4v6_ok, "-A %s %s %s", chain, saddr, end);
			ip46t_flagged_write(ipv6_enabled, v4v6_ok, "-A %s %s -j RETURN\n", chain, saddr);
#ifdef TOMATO64
			if(usemacsrc == 0) {
				ip46t_flagged_write(ipv6_enabled, v4v6_ok, "-A %s %s %s", chain2, saddr, end);
				ip46t_flagged_write(ipv6_enabled, v4v6_ok, "-A %s %s -j RETURN\n", chain2, saddr);
			}
#endif /* TOMATO64 */
		}
	}
	free(buf);

	i = nvram_get_int("qos_default");
	if ((i < 0) || (i > 9))
		i = 3; /* "low" */

	if (class_flag != 0)
		class_flag = 255 << 12;

	class_num = i + 1;
	class_num |= class_flag;
	ip46t_write(ipv6_enabled, "-A QOSO -j CONNMARK --set-mark 0x%x/0xff00f\n", class_num);
	ip46t_write(ipv6_enabled, "-A QOSO -j RETURN\n");
#ifdef TOMATO64
        ip46t_write(ipv6_enabled, "-A QOSO2 -j CONNMARK --set-mark 0x%x/0xff00f\n", class_num);
        ip46t_write(ipv6_enabled, "-A QOSO2 -j RETURN\n");
#endif /* TOMATO64 */

	for (i = 2; i <= MWAN_MAX; i++) { /* always add rules for 1st WAN, so doesn't matter if it's up */
		memset(s, 0, sizeof(s));
		snprintf(s, sizeof(s), "wan%d", i);
		wanup[i - 1] = check_wanup(s);
	}

	/* tc in tomato can only match from fw in filter using PACKET (not connection) mark.
	 * Copy the connection mark to packet mark in POSTROUTING (to apply egress qos)
	 */
	for (i = 1; i <= MWAN_MAX; i++) {
		if ((wanup[i - 1]) || (i == 1)) {
			qface = wanfaces[i - 1].iface[0].name;
			ipt_write("-A FORWARD -o %s -j QOSO\n"
#ifndef TOMATO64
			          "-A OUTPUT -o %s -j QOSO\n"
#else
			          "-A OUTPUT -o %s -j QOSO2\n"
#endif /* TOMATO64 */
			          "-A POSTROUTING -o %s -j CONNMARK --restore-mark --mask 0xf\n",
			          qface, qface, qface);
		}
	}

#ifdef TCONFIG_IPV6
	if (ipv6_enabled && *wan6face)
		ip6t_write("-A FORWARD -o %s -j QOSO\n"
		           "-A OUTPUT -o %s -p icmpv6 -j RETURN\n"
#ifndef TOMATO64
		           "-A OUTPUT -o %s -j QOSO\n"
#else
		           "-A OUTPUT -o %s -j QOSO2\n"
#endif /* TOMATO64 */
		           "-A POSTROUTING -o %s -j CONNMARK --restore-mark --mask 0xf\n"
		           ,wan6face, wan6face, wan6face, wan6face);
#endif /* TCONFIG_IPV6 */

	inuse |= (1 << i) | 1; /* default and highest are always built */
	memset(s, 0, sizeof(s));
	snprintf(s, sizeof(s), "%d", inuse);
	nvram_set("qos_inuse", s);

#ifdef TCONFIG_BCMARM
	if (!nvram_get_int("qos_classify")) {
		disabled_classification_rates = build_disabled_classification_rates(MWAN_MAX);
		g = buf = strdup(disabled_classification_rates);
	}
	else
#endif
		g = buf = strdup(nvram_safe_get("qos_irates"));

	for (i = 0; i < (CLASSES_NUM * MWAN_MAX) ; ++i) {
		if ((!g) || ((p = strsep(&g, ",")) == NULL))
			continue;
		if ((inuse & (1 << (i % CLASSES_NUM))) == 0)
			continue;
		
		unsigned int rate;
		unsigned int ceil;
		
		/* check if we've got a percentage definition in the form of "rate-ceiling" and that rate > 1 */
		if ((sscanf(p, "%u-%u", &rate, &ceil) == 2) && (rate >= 1)) {
			qface = wanfaces[0].iface[0].name;

			/* tc in tomato can only match from fw in filter using PACKET (not connection) mark.
			 * Copy the connection mark to packet mark in PREROUTING (to apply ingress qos)
			 */
			for (j = 1; j <= MWAN_MAX; j++) {
				if ((wanup[j - 1]) || (j == 1)) {
					qface = wanfaces[j - 1].iface[0].name;
					ipt_write("-A PREROUTING -i %s -j CONNMARK --restore-mark --mask 0xf\n", qface);
				}
			}
#ifndef TCONFIG_BCMARM
#ifdef TCONFIG_PPTPD
			if (nvram_get_int("pptpc_enable") && !nvram_match("pptpc_iface", "")) {
				qface = nvram_safe_get("pptpc_iface");
				ipt_write("-A PREROUTING -i %s -j CONNMARK --restore-mark --mask 0xf\n", qface);
			}
#endif /* TCONFIG_PPTPD */

			if (nvram_get_int("qos_udp")) {
				for (j = 1; j <= MWAN_MAX; j++) {
					if ((wanup[j - 1]) || (j == 1)) {
						qface = wanfaces[j - 1].iface[0].name;
						qosDevNumStr = j - 1;
						ipt_write("-A PREROUTING -i %s -p tcp -j IMQ --todev %d\n", qface, qosDevNumStr); /* pass only tcp */
					}
				}
			}
			else {
				for (j = 1; j <= MWAN_MAX; j++) {
					if ((wanup[j - 1]) || (j == 1)) {
						qface = wanfaces[j - 1].iface[0].name;
						qosDevNumStr = j - 1;
						ipt_write("-A PREROUTING -i %s -j IMQ --todev %d\n", qface, qosDevNumStr); /* pass everything thru ingress */
					}
				}
			}
#endif /* !TCONFIG_BCMARM */

#ifdef TCONFIG_IPV6
			if (ipv6_enabled && *wan6face) {
				ip6t_write("-A PREROUTING -i %s -j CONNMARK --restore-mark --mask 0xf\n", wan6face);
#ifndef TCONFIG_BCMARM
				qosDevNumStr = 0;
				if (nvram_get_int("qos_udp"))
						ip6t_write("-A PREROUTING -i %s -p tcp -j IMQ --todev %d\n", wan6face, qosDevNumStr); /* pass only tcp */
				else
						ip6t_write("-A PREROUTING -i %s -j IMQ --todev %d\n", wan6face, qosDevNumStr); /* pass everything thru ingress */
#endif /* !TCONFIG_BCMARM */
			}
#endif /* TCONFIG_IPV6 */
			break;
		}
	}
	free(buf);
}

static unsigned calc(unsigned bw, unsigned pct)
{
	unsigned n = ((unsigned long)bw * pct) / 100;

	return (n < 2) ? 2 : n;
}

void start_qos(char *prefix)
{
	int i;
	char *buf, *g, *p, *qos;
	unsigned int rate;
	unsigned int ceil;
	unsigned int bw;
	unsigned int incomingBWkbps;
	unsigned int mtu;
	unsigned int r2q;
	unsigned int qosDefaultClassId;
	unsigned int overhead;
	FILE *f;
	int x;
	int inuse;
	char s[256];
	int first;
	char burst_root[32];
	char burst_leaf[32];
	char tmp[100];
	int cake;
#ifdef TCONFIG_BCMARM
	char *cake_encap_root = "";
	char *cake_prio_mode_root = "";
	char *disabled_classification_rates;
#endif

	/* Network Congestion Control */
	x = nvram_get_int("ne_vegas");
	if (x) {
		char alpha[10], beta[10], gamma[10];
		memset(alpha, 0, sizeof(alpha));
		snprintf(alpha, sizeof(alpha), "alpha=%d", nvram_get_int("ne_valpha"));
		memset(beta, 0, sizeof(beta));
		snprintf(beta, sizeof(beta), "beta=%d", nvram_get_int("ne_vbeta"));
		memset(gamma, 0, sizeof(gamma));
		snprintf(gamma, sizeof(gamma), "gamma=%d", nvram_get_int("ne_vgamma"));
		modprobe("tcp_vegas", alpha, beta, gamma);
		f_write_procsysnet("ipv4/tcp_congestion_control", "vegas");
	}
	else {
		modprobe_r("tcp_vegas");
		f_write_procsysnet("ipv4/tcp_congestion_control",
#ifdef TCONFIG_BCMARM
		               "cubic"
#else
		               "reno"
#endif
		               );
	}

	if (!nvram_get_int("qos_enable"))
		return;

	qosDefaultClassId = (nvram_get_int("qos_default") + 1) * 10;
	incomingBWkbps = strtoul(nvram_safe_get(strlcat_r(prefix, "_qos_ibw", tmp, sizeof(tmp))), NULL, 10);

	prep_qosstr(prefix);

	if ((f = fopen(qosfn, "w")) == NULL) {
		logerr(__FUNCTION__, __LINE__, qosfn);
		return;
	}

	i = nvram_get_int("qos_burst0");
	if (i > 0)
		snprintf(burst_root, sizeof(burst_root), "burst %dk", i);
	else
		burst_root[0] = 0;

	i = nvram_get_int("qos_burst1");
	if (i > 0)
		snprintf(burst_leaf, sizeof(burst_leaf), "burst %dk", i);
	else
		burst_leaf[0] = 0;

	mtu = strtoul(nvram_safe_get(strlcat_r(prefix, "_mtu", tmp, sizeof(tmp))), NULL, 10);
	bw = strtoul(nvram_safe_get(strlcat_r(prefix, "_qos_obw", tmp, sizeof(tmp))), NULL, 10);
	overhead = strtoul(nvram_safe_get(strlcat_r(prefix, "_qos_overhead", tmp, sizeof(tmp))), NULL, 10);
	r2q = 10;

	if ((bw * 1000) / (8 * r2q) < mtu) {
		r2q = (bw * 1000) / (8 * mtu);
		if (r2q < 1)
			r2q = 1;
	}
	else if ((bw * 1000) / (8 * r2q) > 60000)
		r2q = (bw * 1000) / (8 * 60000) + 1;

	x = nvram_get_int("qos_pfifo");
	if (x == 1)
		qos = "pfifo limit 256";
	else if (x == 2)
		qos = "codel";
	else if (x == 3)
		qos = "fq_codel";
	else
		qos = "sfq perturb 10";

	x = nvram_get_int("qos_mode");
	if (x == 2) {
		qos = "cake";
		cake = 1;
	}
	else
		cake = 0;

	fprintf(f, "#!/bin/sh\n"
	           "WAN_DEV=%s\n"
	           "QOS_DEV=%s\n"
	           "TQA=\"tc qdisc add dev $WAN_DEV\"\n"
	           "TCA=\"tc class add dev $WAN_DEV\"\n"
	           "TFA=\"tc filter add dev $WAN_DEV\"\n"
	           "TQA_QOS=\"tc qdisc add dev $QOS_DEV\"\n"
	           "TCA_QOS=\"tc class add dev $QOS_DEV\"\n"
	           "TFA_QOS=\"tc filter add dev $QOS_DEV\"\n"
	           "Q=\"%s\"\n"
	           "\n"
	           "case \"$1\" in\n"
	           "start)\n"
	           "\ttc qdisc del dev $WAN_DEV root 2>/dev/null\n",
	           get_wanface(prefix),
	           qosdev,
	           qos);

#ifdef TCONFIG_BCMARM
	if (cake) {
		fprintf(f, "\t$TQA root handle 1: cake bandwidth %ukbit nat egress fwmark 0xf", bw);

		if (overhead > 0)
			fprintf(f, " overhead %u", overhead);

		switch (nvram_get_int(strlcat_r(prefix, "_qos_encap", tmp, sizeof(tmp)))) {
			case 1:
				cake_encap_root = " atm";
				break;
			case 2:
				cake_encap_root = " ptm";
				break;
			default:
				cake_encap_root = " noatm";
		}
		fprintf(f, cake_encap_root);

		switch (nvram_get_int("qos_cake_prio_mode")) {
			case 1:
				/* 8 priority classes - see cake_config_diffserv8 in sch_cake.c */
				cake_prio_mode_root = " diffserv8";
				break;
			case 2:
				/* 4 priority classes - see cake_config_diffserv4 in sch_cake.c */
				cake_prio_mode_root = " diffserv4";
				break;
			case 3:
				/* 3 priority classes - see cake_config_diffserv4 in sch_cake.c */
				cake_prio_mode_root = " diffserv3";
				break;
			case 4:
				cake_prio_mode_root = " precedence";
				break;
			default:
				/* All fwmarks ignored */
				cake_prio_mode_root = " besteffort";
				break;
		}
		fprintf(f, cake_prio_mode_root);

		if (nvram_get_int("qos_cake_wash"))
			fprintf(f, " wash");
	}
	else
#endif /* TCONFIG_BCMARM */
	{
		fprintf(f, "\t$TQA root handle 1: htb default %u r2q %u\n"
		           "\t$TCA parent 1: classid 1:1 htb rate %ukbit ceil %ukbit %s",
		           qosDefaultClassId, r2q,
		           bw, bw, burst_root);

		if (overhead > 0) {
			fprintf(f, " overhead %u", overhead);

			/* HTB only supports ATM value */
			if (nvram_get_int(strlcat_r(prefix, "_qos_encap", tmp, sizeof(tmp))))
#ifdef TCONFIG_BCMARM
				fprintf(f, " linklayer atm");
#else
				fprintf(f, " atm");
#endif
		}
	}

	fprintf(f, "\n");

	inuse = nvram_get_int("qos_inuse");
#ifdef TCONFIG_BCMARM
	if (!nvram_get_int("qos_classify")) {
		disabled_classification_rates = build_disabled_classification_rates(MWAN_MAX);
		g = buf = strdup(disabled_classification_rates);
	}
	else
#endif
		g = buf = strdup(nvram_safe_get("qos_orates"));

	/* Cake doesn't support setting rate/ceil for each class and it can read
	 * the "tin" number directly from the fwmark
	 */
	if (!cake) {
		for (i = 0; i < (CLASSES_NUM * qos_wan_num) ; ++i) {
			if ((!g) || ((p = strsep(&g, ",")) == NULL))
				break;

			if (i < qos_rate_start_index || (inuse & (1 << (i % CLASSES_NUM))) == 0)
				continue;

			/* check if we've got a percentage definition in the form of "rate-ceiling" */
			if ((sscanf(p, "%u-%u", &rate, &ceil) != 2) || (rate < 1))
				continue; /* 0=off */

			if (ceil > 0)
				snprintf(s, sizeof(s), "ceil %ukbit", calc(bw, ceil));
			else
				s[0] = 0;

			x = (i + 1) * 10;

		fprintf(f, "\t$TCA parent 1:1 classid 1:%d htb rate %ukbit %s %s prio %d quantum %u",
		           x, calc(bw, rate), s, burst_leaf, (i + 1), mtu);

			if (overhead > 0) {
				fprintf(f, " overhead %u", overhead);

				/* HTB only supports ATM value */
				if (nvram_get_int(strlcat_r(prefix, "_qos_encap", tmp, sizeof(tmp))))
#ifdef TCONFIG_BCMARM
					fprintf(f, " linklayer atm");
#else
					fprintf(f, " atm");
#endif
			}

			fprintf(f, "\n\t$TQA parent 1:%d handle %d: $Q\n"
			           "\t$TFA parent 1: prio %d protocol ip handle %d/0xf fw flowid 1:%d\n",
			           x, x,
			           x, (i + 1), x);

#ifdef TCONFIG_IPV6
			fprintf(f, "\t$TFA parent 1: prio %d protocol ipv6 handle %d/0xf fw flowid 1:%d\n",
			           x + 100, (i + 1), x);
#endif
		}
	}
	free(buf);

	/*
	 * 10000 = ACK
	 * 00100 = RST
	 * 00010 = SYN
	 * 00001 = FIN
	*/

	if (nvram_get_int("qos_ack") && !cake)
		fprintf(f, "\n\t$TFA parent 1: prio 14 protocol ip u32 "
		           "match ip protocol 6 0xff "		/* TCP */
		           "match u8 0x05 0x0f at 0 "		/* IP header length */
		           "match u16 0x0000 0xffc0 at 2 "	/* total length (0-63) */
		           "match u8 0x10 0xff at 33 "		/* ACK only */
		           "flowid 1:10\n");

	if (nvram_get_int("qos_syn") && !cake)
		fprintf(f, "\n\t$TFA parent 1: prio 15 protocol ip u32 "
		           "match ip protocol 6 0xff "		/* TCP */
		           "match u8 0x05 0x0f at 0 "		/* IP header length */
		           "match u16 0x0000 0xffc0 at 2 "	/* total length (0-63) */
		           "match u8 0x02 0x02 at 33 "		/* SYN,* */
		           "flowid 1:10\n");

	if (nvram_get_int("qos_fin") && !cake)
		fprintf(f, "\n\t$TFA parent 1: prio 17 protocol ip u32 "
		           "match ip protocol 6 0xff "		/* TCP */
		           "match u8 0x05 0x0f at 0 "		/* IP header length */
		           "match u16 0x0000 0xffc0 at 2 "	/* total length (0-63) */
		           "match u8 0x01 0x01 at 33 "		/* FIN,* */
		           "flowid 1:10\n");

	if (nvram_get_int("qos_rst") && !cake)
		fprintf(f, "\n\t$TFA parent 1: prio 19 protocol ip u32 "
		           "match ip protocol 6 0xff "		/* TCP */
		           "match u8 0x05 0x0f at 0 "		/* IP header length */
		           "match u16 0x0000 0xffc0 at 2 "	/* total length (0-63) */
		           "match u8 0x04 0x04 at 33 "		/* RST,* */
		           "flowid 1:10\n");

	if (nvram_get_int("qos_icmp") && !cake)
		fprintf(f, "\n\t$TFA parent 1: prio 13 protocol ip u32 match ip protocol 1 0xff flowid 1:10\n");

	/*
	 * INCOMING TRAFFIC SHAPING
	 */
#ifdef TCONFIG_BCMARM
	if (!nvram_get_int("qos_classify")) {
		disabled_classification_rates = build_disabled_classification_rates(MWAN_MAX);
		g = buf = strdup(disabled_classification_rates);
	}
	else
#endif
		g = buf = strdup(nvram_safe_get("qos_irates"));

#ifdef TCONFIG_BCMARM
	fprintf(f, "\n\ttc qdisc del dev $WAN_DEV ingress 2>/dev/null\n"
	           "\t$TQA handle ffff: ingress\n");
#endif

	fprintf(f, "\n\ttc qdisc del dev $QOS_DEV 2>/dev/null\n");

	/* Cake doesn't support setting rate/ceil for each class and it can read
	 * the "tin" number directly from the fwmark
	 */
	if (!cake) {
		first = 1;
		for (i = 0; i < (CLASSES_NUM * qos_wan_num) ; ++i) {
			if ((!g) || ((p = strsep(&g, ",")) == NULL))
				break;

			if (i < qos_rate_start_index || (inuse & (1 << (i % CLASSES_NUM))) == 0)
				continue;

			/* check if we've got a percentage definition in the form of "rate-ceiling" */
			if ((sscanf(p, "%u-%u", &rate, &ceil) != 2) || (rate < 1))
				continue; /* 0=off */

			/* class ID */
			unsigned int classid = ((unsigned int)i + 1) * 10;

			/* priority */
			unsigned int priority = (unsigned int)i + 1; /* prios 1-10 */

			/* rate in kb/s */
			unsigned int rateInkbps = calc(incomingBWkbps, rate);

			/* ceiling in kb/s */
			unsigned int ceilingInkbps = calc(incomingBWkbps, ceil);

			r2q = 10;
			if ((incomingBWkbps * 1000) / (8 * r2q) < mtu) {
				r2q = (incomingBWkbps * 1000) / (8 * mtu);
				if (r2q < 1)
					r2q = 1;
			}
			else if ((incomingBWkbps * 1000) / (8 * r2q) > 60000)
				r2q = (incomingBWkbps * 1000) / (8 * 60000) + 1;

			if (first) {
				first = 0;

				fprintf(f, "\t$TQA_QOS handle 1: root htb default %u r2q %u\n"
				           "\t$TCA_QOS parent 1: classid 1:1 htb rate %ukbit ceil %ukbit",
				           qosDefaultClassId, r2q,
				           incomingBWkbps,
				           incomingBWkbps);

				if (overhead > 0) {
					fprintf(f, " overhead %u", overhead);

					/* HTB only supports ATM value */
					if (nvram_get_int(strlcat_r(prefix, "_qos_encap", tmp, sizeof(tmp))))
#ifdef TCONFIG_BCMARM
						fprintf(f, " linklayer atm");
#else
						fprintf(f, " atm");
#endif
				}
			}

			fprintf(f, "\n\t# class id %u: rate %ukbit ceil %ukbit\n"
			           "\t$TCA_QOS parent 1:1 classid 1:%u htb rate %ukbit ceil %ukbit prio %u quantum %u",
			           classid, rateInkbps, ceilingInkbps,
			           classid, rateInkbps, ceilingInkbps, priority, mtu);

			if (overhead > 0)
#ifdef TCONFIG_BCMARM
				fprintf(f, " overhead %u linklayer atm", overhead);
#else
				fprintf(f, " overhead %u atm", overhead);
#endif

			fprintf(f, "\n\t$TQA_QOS parent 1:%u handle %u: $Q\n"
			           "\t$TFA_QOS parent 1: prio %u protocol ip handle %u/0xf fw flowid 1:%u\n",
			           classid, classid,
			           classid, priority, classid);
#ifdef TCONFIG_IPV6
			fprintf(f, "\t$TFA_QOS parent 1: prio %u protocol ipv6 handle %u/0xf fw flowid 1:%u\n", (classid + 100), priority, classid);
#endif
		} /* for */
	}
#ifdef TCONFIG_BCMARM
	else {
		fprintf(f, "\t$TQA_QOS root handle 1: cake bandwidth %ukbit nat ingress fwmark 0xf", incomingBWkbps);

		if (overhead > 0)
			fprintf(f, " overhead %u", overhead);

		fprintf(f, cake_encap_root);
		fprintf(f, cake_prio_mode_root);

		if (nvram_get_int("qos_cake_wash"))
			fprintf(f, " wash");

		fprintf(f, "\n");
	} /* if (!cake) */
#endif /* TCONFIG_BCMARM */
	free(buf);

#ifdef TCONFIG_BCMARM
#ifndef TOMATO64
	fprintf(f, "\n\t$TFA parent ffff: protocol ip prio 10 u32 match ip %s action mirred egress redirect dev $QOS_DEV\n", (nvram_get_int("qos_udp") ? "protocol 6 0xff" : "dst 0.0.0.0/0"));
#else
	fprintf(f, "\n\t$TFA parent ffff: protocol ip prio 10 u32 match ip %s action connmark mirred egress redirect dev $QOS_DEV\n", (nvram_get_int("qos_udp") ? "protocol 6 0xff" : "dst 0.0.0.0/0"));
#endif /* TOMATO64 */
#ifdef TCONFIG_IPV6
#ifndef TOMATO64
	fprintf(f, "\t$TFA parent ffff: protocol ipv6 prio 11 u32 match ip6 %s action mirred egress redirect dev $QOS_DEV\n", (nvram_get_int("qos_udp") ? "protocol 6 0xff" : "dst ::/0"));
#else
	fprintf(f, "\t$TFA parent ffff: protocol ipv6 prio 11 u32 match ip6 %s action connmark mirred egress redirect dev $QOS_DEV\n", (nvram_get_int("qos_udp") ? "protocol 6 0xff" : "dst ::/0"));
#endif /* TOMATO64 */
#endif
#endif /* TCONFIG_BCMARM */

	/* write commands which adds rule to forward traffic to IFB device */
	fprintf(f, "\n\t# set up the IFB device (otherwise this won't work) to limit the incoming data\n"
	           "\tip link set $QOS_DEV up\n\n"
			   "\tlogger -t qos \"QoS (%s) custom script (if exists) starting\"\n"
			   "\t[ -f /etc/wan_qos.custom ] && /etc/wan_qos.custom start %d\n"
			   "\tlogger -t qos \"QoS (%s) custom script (if exists) started\"\n"
	           "\tlogger -t qos \"QoS (%s) is started\"\n"
	           "\t;;\n"
	           "stop)\n"
			   "\tlogger -t qos \"QoS (%s) custom script (if exists) stopping\"\n"
			   "\t[ -f /etc/wan_qos.custom ] && /etc/wan_qos.custom stop %d\n"
			   "\tlogger -t qos \"QoS (%s) custom script (if exists) stopped\"\n"
	           "\tip link set $QOS_DEV down\n"
	           "\ttc qdisc del dev $WAN_DEV root 2>/dev/null\n"
	           "\ttc qdisc del dev $QOS_DEV root 2>/dev/null\n",
			   prefix,
			   qos_wan_num,
			   prefix,
	           prefix,
			   prefix,
			   qos_wan_num,
			   prefix);

#ifdef TCONFIG_BCMARM
	fprintf(f, "\ttc filter del dev $WAN_DEV parent ffff: protocol ip prio 10 u32 match ip %s action mirred egress redirect dev $QOS_DEV 2>/dev/null\n", (nvram_get_int("qos_udp") ? "protocol 6 0xff" : "dst 0.0.0.0/0"));
#ifdef TCONFIG_IPV6
	fprintf(f, "\ttc filter del dev $WAN_DEV parent ffff: protocol ipv6 prio 11 u32 match ip6 %s action mirred egress redirect dev $QOS_DEV 2>/dev/null\n", (nvram_get_int("qos_udp") ? "protocol 6 0xff" : "dst ::/0"));
#endif
#endif /* TCONFIG_BCMARM */

	fprintf(f, "\ttc qdisc del dev $WAN_DEV ingress 2>/dev/null\n\n"
	           "\tlogger -t qos \"QoS (%s) is stopped\"\n"
	           "\t;;\n"
	           "*)\n"
	           "\techo \"...\"\n"
	           "\techo \"... OUTGOING QDISCS AND CLASSES FOR $WAN_DEV\"\n"
	           "\techo \"...\"\n"
	           "\ttc -s -d qdisc ls dev $WAN_DEV\n"
	           "\techo\n"
	           "\ttc -s -d class ls dev $WAN_DEV\n"
	           "\techo\n"
	           "\techo \"...\"\n"
	           "\techo \"... INCOMING QDISCS AND CLASSES FOR $WAN_DEV (routed through $QOS_DEV)\"\n"
	           "\techo \"...\"\n"
	           "\ttc -s -d qdisc ls dev $QOS_DEV\n"
	           "\techo\n"
	           "\ttc -s -d class ls dev $QOS_DEV\n"
	           "\techo\n"
	           "esac\n",
	           prefix);

	fclose(f);

	chmod(qosfn, 0700);

	eval(qosfn, "start");
}

void stop_qos(char *prefix)
{
	FILE *f;

	prep_qosstr(prefix);

	if ((f = fopen(qosfn, "r")) == NULL)
		return;

	fclose(f);

	eval(qosfn, "stop");
}

/*
 * PREROUTING (mn) ----> x ----> FORWARD (f) ----> + ----> POSTROUTING (n)
 *            QD         |                         ^
 *                       |                         |
 *                       v                         |
 *                     INPUT (f)                 OUTPUT (mnf)
 */
