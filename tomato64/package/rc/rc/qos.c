/*
 *
 * Tomato Firmware
 * Copyright (C) 2006-2009 Jonathan Zarate
 *
 */


#include "rc.h"

#include <sys/stat.h>

#define CLASSES_NUM	10
static char qosfn[] = "/etc/wanX_qos";
static char qosdev[] = "iXXX";
static int qos_wan_num = 0;
static int qos_rate_start_index = 0;
#ifdef TCONFIG_MULTIWAN
const char disabled_classification_rates[] = "5-100,0-0,0-0,0-0,0-0,0-0,0-0,0-0,0-0,0-0,"
                                             "5-100,0-0,0-0,0-0,0-0,0-0,0-0,0-0,0-0,0-0,"
                                             "5-100,0-0,0-0,0-0,0-0,0-0,0-0,0-0,0-0,0-0,"
                                             "5-100,0-0,0-0,0-0,0-0,0-0,0-0,0-0,0-0,0-0";
#else
const char disabled_classification_rates[] = "5-100,0-0,0-0,0-0,0-0,0-0,0-0,0-0,0-0,0-0,"
                                             "5-100,0-0,0-0,0-0,0-0,0-0,0-0,0-0,0-0,0-0";
#endif
const char disabled_classification_rules[] = "0<<-2<a<<0<<<<0<Default";

void prep_qosstr(char *prefix)
{
	if (!strcmp(prefix, "wan")) {
		strcpy(qosfn, "/etc/wan_qos");
		qos_wan_num = 1;
		qos_rate_start_index = 0;
#ifdef TCONFIG_BCMARM
		strcpy(qosdev, "ifb0");
#else
		strcpy(qosdev, "imq0");
#endif
	}
	else if (!strcmp(prefix, "wan2")) {
		strcpy(qosfn, "/etc/wan2_qos");
		qos_wan_num = 2;
		qos_rate_start_index = 10;
#ifdef TCONFIG_BCMARM
		strcpy(qosdev, "ifb1");
#else
		strcpy(qosdev, "imq1");
#endif
	}
#ifdef TCONFIG_MULTIWAN
	else if (!strcmp(prefix, "wan3")) {
		strcpy(qosfn, "/etc/wan3_qos");
		qos_wan_num = 3;
		qos_rate_start_index = 20;
#ifdef TCONFIG_BCMARM
		strcpy(qosdev, "ifb2");
#else
		strcpy(qosdev, "imq2");
#endif
	}
	else if (!strcmp(prefix, "wan4")) {
		strcpy(qosfn, "/etc/wan4_qos");
		qos_wan_num = 4;
		qos_rate_start_index = 30;
#ifdef TCONFIG_BCMARM
		strcpy(qosdev, "ifb3");
#else
		strcpy(qosdev, "imq3");
#endif
	}
#endif /* TCONFIG_MULTIWAN */
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
	char *ipp2p, *layer7;
	char *bcount;
	char *dscp;
	char *desc;
	int class_num;
	int proto_num;
	int v4v6_ok;
	int i;
	char sport[192];
	char saddr[256];
	char end[256];
	char s[32];
	char app[256];
	int inuse;
	const char *chain;
	unsigned long min;
	unsigned long max;
	unsigned long prev_max;
	int process_size_groups;
	const char *qface;
	int sizegroup;
	int class_flag;
	int rule_num;
	int wan2_up;
#ifdef TCONFIG_MULTIWAN
	int wan3_up;
	int wan4_up;
	int mwan_num = 4;
#else
	int mwan_num = 2;
#endif
#ifndef TCONFIG_BCMARM
	int qosDevNumStr = 0;
#endif

	if (!nvram_get_int("qos_enable"))
		return;

	inuse = 0;
	class_flag = 0;
	process_size_groups = 1;
	sizegroup = 0;
	prev_max = 0;
	rule_num = 0;

	/* Don't reclassify an already classified connection (qos class is in rightmost half-byte of mark)
	 * that also doesn't have a size group. If it has a size group, processing needs to continue
	 * to allow the mark to get cleared after the max bytes have been sent/received by QOSSIZE chain
	 */
	ip46t_write(":QOSO - [0:0]\n"
	            "-A QOSO -m connmark --mark 0/0xff000 -m connmark ! --mark 0/0xf -j RETURN\n");

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

		i = vstrsep(p, "<", &addr_type, &addr, &proto, &port_type, &port, &ipp2p, &layer7, &bcount, &dscp, &class_prio, &desc);
		rule_num++;
		if (i < 11)
			continue;

		class_num = atoi(class_prio);
		if ((class_num < 0) || (class_num > 9))
			continue;

		i = 1 << class_num;
		++class_num;

		if ((inuse & i) == 0)
			inuse |= i;

		v4v6_ok = IPT_V4;
#ifdef TCONFIG_IPV6
		if (ipv6_enabled())
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
			sprintf(saddr, "-m mac --mac-source %s", addr); /* (-m mac modified, returns !match in OUTPUT) */
		}

		/* IPP2P/Layer7 */
		memset(app, 0, 256);
		if (ipt_ipp2p(ipp2p, app))
			v4v6_ok &= ~IPT_V6;
		else
			ipt_layer7(layer7, app);

		if (app[0]) {
			v4v6_ok &= ~IPT_V6; /* L7 for IPv6 not working either! */
			strcat(saddr, app);
		}

		/* dscp */
		memset(s, 0, 32);
		if (ipt_dscp(dscp, s))
			strcat(saddr, s);

		class_flag = 0;

		/* -m connbytes --connbytes x:y --connbytes-dir both --connbytes-mode bytes */
		if (*bcount) {
			min = strtoul(bcount, &p, 10);
			if (*p != 0) {
				strcat(saddr, " -m connbytes --connbytes-mode bytes --connbytes-dir both --connbytes ");
				++p;
				if (*p == 0)
					sprintf(saddr + strlen(saddr), "%lu:", min * 1024);
				else {
					max = strtoul(p, NULL, 10);
					sprintf(saddr + strlen(saddr), "%lu:%lu", min * 1024, (max * 1024) - 1);
					if (!sizegroup) {
						/* create table of connbytes sizes, pass appropriate connections there and only continue processing them if mark was wiped */
						ip46t_write(":QOSSIZE - [0:0]\n"
						            "-I QOSO 2 -m connmark ! --mark 0/0xff000 -j QOSSIZE\n"
						            "-I QOSO 3 -m connmark ! --mark 0/0xff000 -m connmark ! --mark 0xff000/0xff000 -j RETURN\n");
					}
					if (max != prev_max && sizegroup < 255) {
						class_flag = ++sizegroup << 12;
						prev_max = max;
						/* Clear the QoS mark to allow the packet to be reclassified. Note: the last used size group will remain left behind so that
						 * we can match on it in the next rule and return from the chain
						 */
						ip46t_flagged_write(v4v6_ok, "-A QOSSIZE -m connmark --mark 0x%x/0xff000 -m connbytes --connbytes-mode bytes --connbytes-dir both "
						                             "--connbytes %lu: -j CONNMARK --set-mark 0xff000/0xff00f\n", (sizegroup << 12), (max * 1024));
						ip46t_flagged_write(v4v6_ok, "-A QOSSIZE -m connmark --mark 0xff000/0xff000 -m connbytes --connbytes-mode bytes --connbytes-dir both "
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
		class_num |= class_flag;
		sprintf(end + strlen(end), " -j CONNMARK --set-mark 0x%x/0xff00f\n", class_num);

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
						sprintf(sport, "-m multiport --%sports %s", (*port_type == 's') ? "s" : ((*port_type == 'd') ? "d" : ""), port);
					else
						/* single or simple x:y range, use built-in tcp/udp match */
						sprintf(sport, "--%sport %s", (*port_type == 's') ? "s" : ((*port_type == 'd') ? "d" : ""), port);
				}
				else
					sport[0] = 0;

				if (proto_num != 6) {
					ip46t_flagged_write(v4v6_ok, "-A %s -p %s %s %s %s", chain, "udp", sport, saddr, end);
					ip46t_flagged_write(v4v6_ok, "-A %s -p %s %s %s -j RETURN\n", chain, "udp", sport, saddr);
				}
				if (proto_num != 17) {
					ip46t_flagged_write(v4v6_ok, "-A %s -p %s %s %s %s", chain, "tcp", sport, saddr, end);
					ip46t_flagged_write(v4v6_ok, "-A %s -p %s %s %s -j RETURN\n", chain, "tcp", sport, saddr);
				}
			}
			else {
				ip46t_flagged_write(v4v6_ok, "-A %s -p %d %s %s", chain, proto_num, saddr, end);
				ip46t_flagged_write(v4v6_ok, "-A %s -p %d %s -j RETURN\n", chain, proto_num, saddr);
			}
		}
		else { /* any protocol */
			ip46t_flagged_write(v4v6_ok, "-A %s %s %s", chain, saddr, end);
			ip46t_flagged_write(v4v6_ok, "-A %s %s -j RETURN\n", chain, saddr);
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
	ip46t_write("-A QOSO -j CONNMARK --set-mark 0x%x/0xff00f\n", class_num);
	ip46t_write("-A QOSO -j RETURN\n");

	wan2_up = check_wanup("wan2");
#ifdef TCONFIG_MULTIWAN
	wan3_up = check_wanup("wan3");
	wan4_up = check_wanup("wan4");
#endif

	/* tc in tomato can only match from fw in filter using PACKET (not connection) mark.
	 * Copy the connection mark to packet mark in POSTROUTING (to apply egress qos)
	 */
	qface = wanfaces.iface[0].name;
	ipt_write("-A FORWARD -o %s -j QOSO\n"
	          "-A OUTPUT -o %s -j QOSO\n"
	          "-A POSTROUTING -o %s -j CONNMARK --restore-mark --mask 0xf\n"
	          ,qface, qface, qface);

	if (wan2_up) {
		qface = wan2faces.iface[0].name;
		ipt_write("-A FORWARD -o %s -j QOSO\n"
		          "-A OUTPUT -o %s -j QOSO\n"
		          "-A POSTROUTING -o %s -j CONNMARK --restore-mark --mask 0xf\n"
		          ,qface, qface, qface);
	}
#ifdef TCONFIG_MULTIWAN
	if (wan3_up) {
		qface = wan3faces.iface[0].name;
		ipt_write("-A FORWARD -o %s -j QOSO\n"
		          "-A OUTPUT -o %s -j QOSO\n"
		          "-A POSTROUTING -o %s -j CONNMARK --restore-mark --mask 0xf\n"
		          ,qface, qface, qface);
	}
	if (wan4_up) {
		qface = wan4faces.iface[0].name;
		ipt_write("-A FORWARD -o %s -j QOSO\n"
		          "-A OUTPUT -o %s -j QOSO\n"
		          "-A POSTROUTING -o %s -j CONNMARK --restore-mark --mask 0xf\n"
		          ,qface, qface, qface);
	}
#endif /* TCONFIG_MULTIWAN */

#ifdef TCONFIG_IPV6
	if (*wan6face)
		ip6t_write("-A FORWARD -o %s -j QOSO\n"
		           "-A OUTPUT -o %s -p icmpv6 -j RETURN\n"
		           "-A OUTPUT -o %s -j QOSO\n"
		           "-A POSTROUTING -o %s -j CONNMARK --restore-mark --mask 0xf\n"
		           ,wan6face, wan6face, wan6face, wan6face);
#endif /* TCONFIG_IPV6 */

	inuse |= (1 << i) | 1; /* default and highest are always built */
	memset(s, 0, 32);
	sprintf(s, "%d", inuse);
	nvram_set("qos_inuse", s);

#ifdef TCONFIG_BCMARM
	if (!nvram_get_int("qos_classify"))
		g = buf = strdup(disabled_classification_rates);
	else
#endif
		g = buf = strdup(nvram_safe_get("qos_irates"));

	for (i = 0; i < (CLASSES_NUM * mwan_num) ; ++i) {
		if ((!g) || ((p = strsep(&g, ",")) == NULL))
			continue;
		if ((inuse & (1 << (i % CLASSES_NUM))) == 0)
			continue;
		
		unsigned int rate;
		unsigned int ceil;
		
		/* check if we've got a percentage definition in the form of "rate-ceiling" and that rate > 1 */
		if ((sscanf(p, "%u-%u", &rate, &ceil) == 2) && (rate >= 1)) {
			qface = wanfaces.iface[0].name;

			/* tc in tomato can only match from fw in filter using PACKET (not connection) mark.
			 * Copy the connection mark to packet mark in PREROUTING (to apply ingress qos)
			 */
			ipt_write("-A PREROUTING -i %s -j CONNMARK --restore-mark --mask 0xf\n", qface);

			if (wan2_up) {
				qface = wan2faces.iface[0].name;
				ipt_write("-A PREROUTING -i %s -j CONNMARK --restore-mark --mask 0xf\n", qface);
			}

#ifdef TCONFIG_MULTIWAN
			if (wan3_up) {
				qface = wan3faces.iface[0].name;
				ipt_write("-A PREROUTING -i %s -j CONNMARK --restore-mark --mask 0xf\n", qface);
			}

			if (wan4_up) {
				qface = wan4faces.iface[0].name;
				ipt_write("-A PREROUTING -i %s -j CONNMARK --restore-mark --mask 0xf\n", qface);
			}
#endif

#ifndef TCONFIG_BCMARM
#ifdef TCONFIG_PPTPD
			if (nvram_get_int("pptp_client_enable") && !nvram_match("pptp_client_iface", "")) {
				qface = nvram_safe_get("pptp_client_iface");
				ipt_write("-A PREROUTING -i %s -j CONNMARK --restore-mark --mask 0xf\n", qface);
			}
#endif /* TCONFIG_PPTPD */

			if (nvram_get_int("qos_udp")) {
				qface = wanfaces.iface[0].name;
				qosDevNumStr = 0;
				ipt_write("-A PREROUTING -i %s -p tcp -j IMQ --todev %d\n", qface, qosDevNumStr); /* pass only tcp */

				if (wan2_up) {
					qface = wan2faces.iface[0].name;
					qosDevNumStr = 1;
					ipt_write("-A PREROUTING -i %s -p tcp -j IMQ --todev %d\n", qface, qosDevNumStr); /* pass only tcp */
				}
#ifdef TCONFIG_MULTIWAN
				if (wan3_up) {
					qface = wan3faces.iface[0].name;
					qosDevNumStr = 2;
					ipt_write("-A PREROUTING -i %s -p tcp -j IMQ --todev %d\n", qface, qosDevNumStr); /* pass only tcp */
				}
				if (wan4_up) {
					qface = wan4faces.iface[0].name;
					qosDevNumStr = 3;
					ipt_write("-A PREROUTING -i %s -p tcp -j IMQ --todev %d\n", qface, qosDevNumStr); /* pass only tcp */
				}
#endif /* TCONFIG_MULTIWAN */
			}
			else {
				qface = wanfaces.iface[0].name;
				qosDevNumStr = 0;
				ipt_write("-A PREROUTING -i %s -j IMQ --todev %d\n", qface, qosDevNumStr); /* pass everything thru ingress */

				if (wan2_up) {
					qface = wan2faces.iface[0].name;
					qosDevNumStr = 1;
					ipt_write("-A PREROUTING -i %s -j IMQ --todev %d\n", qface, qosDevNumStr); /* pass everything thru ingress */
				}
#ifdef TCONFIG_MULTIWAN
				if (wan3_up) {
					qface = wan3faces.iface[0].name;
					qosDevNumStr = 2;
					ipt_write("-A PREROUTING -i %s -j IMQ --todev %d\n", qface, qosDevNumStr); /* pass everything thru ingress */
				}
				if (wan4_up) {
					qface = wan4faces.iface[0].name;
					qosDevNumStr = 3;
					ipt_write("-A PREROUTING -i %s -j IMQ --todev %d\n", qface, qosDevNumStr); /* pass everything thru ingress */
				}
#endif /* TCONFIG_MULTIWAN */
			}
#endif /* !TCONFIG_BCMARM */

#ifdef TCONFIG_IPV6
			if (*wan6face) {
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
#endif

	/* Network Congestion Control */
	x = nvram_get_int("ne_vegas");
	if (x) {
		char alpha[10], beta[10], gamma[10];
		memset(alpha, 0, 10);
		sprintf(alpha, "alpha=%d", nvram_get_int("ne_valpha"));
		memset(beta, 0, 10);
		sprintf(beta, "beta=%d", nvram_get_int("ne_vbeta"));
		memset(gamma, 0, 10);
		sprintf(gamma, "gamma=%d", nvram_get_int("ne_vgamma"));
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
	incomingBWkbps = strtoul(nvram_safe_get(strcat_r(prefix, "_qos_ibw", tmp)), NULL, 10);

	prep_qosstr(prefix);

	if ((f = fopen(qosfn, "w")) == NULL) {
		logerr(__FUNCTION__, __LINE__, qosfn);
		return;
	}

	i = nvram_get_int("qos_burst0");
	if (i > 0)
		sprintf(burst_root, "burst %dk", i);
	else
		burst_root[0] = 0;

	i = nvram_get_int("qos_burst1");
	if (i > 0)
		sprintf(burst_leaf, "burst %dk", i);
	else
		burst_leaf[0] = 0;

	mtu = strtoul(nvram_safe_get(strcat_r(prefix, "_mtu", tmp)), NULL, 10);
	bw = strtoul(nvram_safe_get(strcat_r(prefix, "_qos_obw", tmp)), NULL, 10);
	overhead = strtoul(nvram_safe_get(strcat_r(prefix, "_qos_overhead", tmp)), NULL, 10);
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

		switch (nvram_get_int(strcat_r(prefix, "_qos_encap", tmp))) {
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
			if (nvram_get_int(strcat_r(prefix, "_qos_encap", tmp)))
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
	if (!nvram_get_int("qos_classify"))
		g = buf = strdup(disabled_classification_rates);
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
				sprintf(s, "ceil %ukbit", calc(bw, ceil));
			else
				s[0] = 0;

			x = (i + 1) * 10;

		fprintf(f, "\t$TCA parent 1:1 classid 1:%d htb rate %ukbit %s %s prio %d quantum %u",
		           x, calc(bw, rate), s, burst_leaf, (i + 1), mtu);

			if (overhead > 0) {
				fprintf(f, " overhead %u", overhead);

				/* HTB only supports ATM value */
				if (nvram_get_int(strcat_r(prefix, "_qos_encap", tmp)))
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
	if (!nvram_get_int("qos_classify"))
		g = buf = strdup(disabled_classification_rates);
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
					if (nvram_get_int(strcat_r(prefix, "_qos_encap", tmp)))
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
	fprintf(f, "\n\t$TFA parent ffff: protocol ip prio 10 u32 match ip %s action mirred egress redirect dev $QOS_DEV\n", (nvram_get_int("qos_udp") ? "protocol 6 0xff" : "dst 0.0.0.0/0"));
#ifdef TCONFIG_IPV6
	fprintf(f, "\t$TFA parent ffff: protocol ipv6 prio 11 u32 match ip6 %s action mirred egress redirect dev $QOS_DEV\n", (nvram_get_int("qos_udp") ? "protocol 6 0xff" : "dst ::/0"));
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
