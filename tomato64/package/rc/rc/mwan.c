/*
 *
 * Multi WAN
 * By Arctic QQ:317869867 E-Mail:zengchen228@vip.qq.com
 *
 * Fixes/updates (C) 2018 - 2026 pedro
 * https://freshtomato.org/
 *
 */


#include "rc.h"

/* needed by logmsg() */
#define LOGMSG_DISABLE	DISABLE_SYSLOG_OS
#define LOGMSG_NVDEBUG	"mwan_debug"


static char mwan_curr[MWAN_MAX + 1] = { [0 ... MWAN_MAX - 1] = '0', [MWAN_MAX] = '\0' };
static char mwan_last[MWAN_MAX + 1] = { [0 ... MWAN_MAX - 1] = '0', [MWAN_MAX] = '\0' };

typedef struct
{
	char wan_name[16];
	char wan_ipaddr[32];
	char wan_netmask[32];
	char wan_gateway[32];
	const dns_list_t *dns;
	int wan_weight;
} waninfo_t;

static waninfo_t wan_info;

static void mwan_argv_to_cmd(char *const argv[], char *cmd, const size_t cmd_sz)
{
	int i, n, off, rem;

	if (cmd_sz == 0)
		return;

	cmd[0] = '\0';
	off = 0;

	for (i = 0; argv[i]; ++i) {
		rem = (int)cmd_sz - off;
		if (rem <= 1)
			break;

		n = snprintf(cmd + off, rem, "%s%s", (i == 0) ? "" : " ", argv[i]);
		if ((n < 0) || (n >= rem)) {
			cmd[cmd_sz - 1] = '\0';
			break;
		}

		off += n;
	}
}

static int mwan_eval(char *const argv[], const char *func, const char *prefix)
{
	char cmd[2048];

	mwan_argv_to_cmd(argv, cmd, sizeof(cmd));

	if (prefix)
		logmsg(LOG_DEBUG, "*** %s: PREFIX=[%s], cmd=[%s]", func, prefix, cmd);
	else
		logmsg(LOG_DEBUG, "*** %s: cmd=[%s]", func, cmd);

	return _eval(argv, NULL, 0, NULL);
}


int get_sta_wan_prefix(char *sPrefix, const size_t buf_sz)
{
	int mwan_num;
	int wan_unit;
	char prefix[16];
	char tmp[32];
	int found = 0;

	mwan_num = nvram_get_int("mwan_num");
	if ((mwan_num < 1) || (mwan_num > MWAN_MAX))
		mwan_num = 1;

	for (wan_unit = 1; wan_unit <= mwan_num; ++wan_unit) {
		memset(prefix, 0, sizeof(prefix));
		get_wan_prefix(wan_unit, prefix);

		if (strcmp(nvram_safe_get(strlcat_r(prefix, "_sta", tmp, sizeof(tmp))), "")) {
			found = 1;
			break;
		}
	}

	if (found)
		strlcpy(sPrefix, prefix, buf_sz);
	else
		strlcpy(sPrefix, "wan", buf_sz);

	return found;
}

void get_wan_info(char *sPrefix)
{
	char tmp[32];
	int proto = get_wanx_proto(sPrefix);

	/* WAN if name */
	strlcpy(wan_info.wan_name, get_wanface(sPrefix), sizeof(wan_info.wan_name)); /* use correct wan interface */

	/* WAN IP address */
	switch (proto) {
		case WP_L2TP:
		case WP_PPTP:
			strlcpy(wan_info.wan_ipaddr, nvram_safe_get(strlcat_r(sPrefix, "_ppp_get_ip", tmp, sizeof(tmp))), sizeof(wan_info.wan_ipaddr));
			break;
		case WP_PPPOE:
			if (using_dhcpc(sPrefix))
				strlcpy(wan_info.wan_ipaddr, nvram_safe_get(strlcat_r(sPrefix, "_ppp_get_ip", tmp, sizeof(tmp))), sizeof(wan_info.wan_ipaddr));
			else
				strlcpy(wan_info.wan_ipaddr, nvram_safe_get(strlcat_r(sPrefix, "_ipaddr", tmp, sizeof(tmp))), sizeof(wan_info.wan_ipaddr));
			break;
		default:
			strlcpy(wan_info.wan_ipaddr, nvram_safe_get(strlcat_r(sPrefix, "_ipaddr", tmp, sizeof(tmp))), sizeof(wan_info.wan_ipaddr));
			break;
	}

	/* WAN netmask */
	if ((proto == WP_L2TP) || (proto == WP_PPTP) || (proto == WP_PPPOE) || (proto == WP_PPP3G))
		strlcpy(wan_info.wan_netmask, "255.255.255.255", sizeof(wan_info.wan_netmask));
	else
		strlcpy(wan_info.wan_netmask, nvram_safe_get(strlcat_r(sPrefix, "_netmask", tmp, sizeof(tmp))), sizeof(wan_info.wan_netmask));

	/* WAN gateway */
	strlcpy(wan_info.wan_gateway, wan_gateway(sPrefix), sizeof(wan_info.wan_gateway));

	/* WAN dns */
	wan_info.dns = get_dns(sPrefix); /* static buffer */

	/* WAN weight */
	wan_info.wan_weight = atoi(nvram_safe_get(strlcat_r(sPrefix, "_weight", tmp, sizeof(tmp))));

	logmsg(LOG_DEBUG, "*** %s: PREFIX=[%s], wan_name=[%s] wan_ipaddr=[%s] wan_netmask=[%s] wan_gateway=[%s] wan_weight=[%d]", __FUNCTION__, sPrefix, wan_info.wan_name, wan_info.wan_ipaddr, wan_info.wan_netmask, wan_info.wan_gateway, wan_info.wan_weight);
}

void get_wan_ip(int proto, char *name, const size_t buf_sz)
{
	if ((proto == WP_DHCP) || (proto == WP_LTE) || (proto == WP_STATIC))
		strlcpy(name, wan_info.wan_gateway, buf_sz);
	else
		strlcpy(name, wan_info.wan_ipaddr, buf_sz);
}

void get_cidr(char *ipaddr, char *netmask, char *cidr, const size_t buf_sz)
{
	struct in_addr in_ipaddr, in_netmask, in_network;
	int netmask_bit = 0;
	unsigned long int bits = 1;

	inet_aton(ipaddr, &in_ipaddr);
	inet_aton(netmask, &in_netmask);

	unsigned int i;
	for (i = 1; i < sizeof(bits) * 8; i++) {
		if (in_netmask.s_addr & bits)
			netmask_bit++;
		bits = bits << 1;
	}

	in_network.s_addr = in_ipaddr.s_addr & in_netmask.s_addr;
	snprintf(cidr, buf_sz, "%s/%d", inet_ntoa(in_network), netmask_bit);
}

void mwan_table_del(char *sPrefix)
{
	int wan_unit;
	int i;
	char table[12];
	char pref[12];
	char *rule_del_argv[] = { "ip", "rule", "del", "table", table, "pref", pref, NULL };

	logmsg(LOG_DEBUG, "*** %s IN", __FUNCTION__);

	wan_unit = get_wan_unit(sPrefix);
	get_wan_info(sPrefix); /* get the current wan infos to work with */

	snprintf(table, sizeof(table), "%d", wan_unit);

	/* ip rule del table WAN1 pref 101 (gateway); table: 1 to 4; pref: 101 to 104 */
	snprintf(pref, sizeof(pref), "10%d", wan_unit);
	mwan_eval(rule_del_argv, __FUNCTION__, sPrefix);

	/* ip rule del table WAN1 pref 111 (dns); table: 1 to 4; pref: 111 to 114 */
	/* delete only active & valid DNS; two options right now: only AUTO DNS server (1x DNS) or Manual DNS server (2x DNS) (see GUI basic-network.asp) */
	for (i = 0 ; i < wan_info.dns->count; ++i) {
		snprintf(pref, sizeof(pref), "11%d", wan_unit);
		mwan_eval(rule_del_argv, __FUNCTION__, sPrefix);
	}

	/* ip rule del fwmark 0x100/0xf00 table 1 pref 121 (mark); table: 1 to 4; pref: 121 to 124 */
	snprintf(pref, sizeof(pref), "12%d", wan_unit);
	mwan_eval(rule_del_argv, __FUNCTION__, sPrefix);

	logmsg(LOG_DEBUG, "*** %s OUT", __FUNCTION__);
}

/* set multiwan ip route table & ip rule table */
void mwan_table_add(char *sPrefix)
{
	int mwan_num, wan_unit, proto;
	int wanid;
	int i;
	char table[12];
	char pref[12];
	char mark[16];
	char wanid_s[12];
	char cidr[32];
	char nexthop[32];
	char nvram_var[16];
	char *lan_ifname;
	char *lan_ipaddr;
	char *lan_netmask;
	char *route_dst;
	char *rule_from_argv[] = { "ip", "rule", "add", "from", wan_info.wan_ipaddr, "table", table, "pref", pref, NULL };
	char *rule_dns_argv[] = { "ip", "rule", "add", "to", NULL, "table", table, "pref", pref, NULL };
	char *rule_mark_argv[] = { "ip", "rule", "add", "fwmark", mark, "table", table, "pref", pref, NULL };
	char *route_link_argv[] = { "ip", "route", "append", NULL, "dev", wan_info.wan_name, "proto", "kernel", "scope", "link", "src", wan_info.wan_ipaddr, "table", wanid_s, NULL };
	char *route_lan_argv[] = { "ip", "route", "append", cidr, "dev", NULL, "proto", "kernel", "scope", "link", "src", NULL, "table", table, NULL };
	char *route_lo_argv[] = { "ip", "route", "append", "127.0.0.0/8", "dev", "lo", "scope", "link", "table", table, NULL };
	char *route_default_argv[] = { "ip", "route", "append", "default", "via", nexthop, "dev", wan_info.wan_name, "table", table, NULL };

	logmsg(LOG_DEBUG, "*** %s IN for %s", __FUNCTION__, sPrefix);

	/* delete already existed table first */
	mwan_table_del(sPrefix);

	mwan_num = nvram_get_int("mwan_num");
	if ((mwan_num == 1) || (mwan_num > MWAN_MAX))
		return;

	wan_unit = get_wan_unit(sPrefix);
	get_wan_info(sPrefix); /* get the current wan infos to work with */
	proto = get_wanx_proto(sPrefix);
	snprintf(table, sizeof(table), "%d", wan_unit);

	if (check_wanup(sPrefix)) {
		/* ip rule add from WAN_IP table route_id pref 10X */
		snprintf(pref, sizeof(pref), "10%d", wan_unit);
		mwan_eval(rule_from_argv, __FUNCTION__, sPrefix);

		/* set the routing rules of DNS */
		for (i = 0; i < wan_info.dns->count; ++i) {
			snprintf(pref, sizeof(pref), "11%d", wan_unit);
			rule_dns_argv[4] = inet_ntoa(wan_info.dns->dns[i].addr);
			mwan_eval(rule_dns_argv, __FUNCTION__, sPrefix);
		}

		/* ip rule add fwmark 0x100/0xf00 table 1 pref 121 */
		snprintf(mark, sizeof(mark), "0x%d00/0xf00", wan_unit);
		snprintf(pref, sizeof(pref), "12%d", wan_unit);
		mwan_eval(rule_mark_argv, __FUNCTION__, sPrefix);

		for (wanid = 1; wanid <= mwan_num; ++wanid) {
			/* ip route add 10.0.10.1 dev ppp3 proto kernel scope link table route_id */
			snprintf(wanid_s, sizeof(wanid_s), "%d", wanid);

			if ((proto == WP_DHCP) || (proto == WP_LTE) || (proto == WP_STATIC)) {
				get_cidr(wan_info.wan_ipaddr, wan_info.wan_netmask, cidr, sizeof(cidr));
				route_dst = cidr;
			}
			else
				route_dst = wan_info.wan_gateway;

			route_link_argv[3] = route_dst;
			mwan_eval(route_link_argv, __FUNCTION__, sPrefix);
		}

		/* ip route add 192.168.1.0/24 dev br0 proto kernel scope link  src 192.168.1.1  table 2 */
		for (i = 0; i < BRIDGE_COUNT; i++) {
			snprintf(nvram_var, sizeof(nvram_var), (i == 0 ? "lan_ifname" : "lan%d_ifname"), i);
			lan_ifname = nvram_safe_get(nvram_var);
			snprintf(nvram_var, sizeof(nvram_var), (i == 0 ? "lan_ipaddr" : "lan%d_ipaddr"), i);
			lan_ipaddr = nvram_safe_get(nvram_var);
			snprintf(nvram_var, sizeof(nvram_var), (i == 0 ? "lan_netmask" : "lan%d_netmask"), i);
			lan_netmask = nvram_safe_get(nvram_var);

			if ((lan_ifname[0] == '\0') || (lan_ipaddr[0] == '\0') || (lan_netmask[0] == '\0'))
				continue;

			get_cidr(lan_ipaddr, lan_netmask, cidr, sizeof(cidr));
			route_lan_argv[5] = lan_ifname;
			route_lan_argv[11] = lan_ipaddr;
			mwan_eval(route_lan_argv, __FUNCTION__, sPrefix);
		}

		/* ip route add 127.0.0.0/8 dev lo scope link table 1 */
		mwan_eval(route_lo_argv, __FUNCTION__, sPrefix);

		/* ip route add default via 10.0.10.1 dev ppp3 table route_id */
		get_wan_ip(proto, nexthop, sizeof(nexthop));
		mwan_eval(route_default_argv, __FUNCTION__, sPrefix);
	}

	logmsg(LOG_DEBUG, "*** %s OUT", __FUNCTION__);
}

void mwan_state_files(void)
{
	int mwan_num, wan_unit;
	char prefix[16];
	char tmp[64];
	FILE *f;

	mwan_num = nvram_get_int("mwan_num");
	if ((mwan_num == 1) || (mwan_num > MWAN_MAX))
		return;

	for (wan_unit = 1; wan_unit <= mwan_num; ++wan_unit) {
		memset(prefix, 0, sizeof(prefix));
		get_wan_prefix(wan_unit, prefix);

		snprintf(tmp, sizeof(tmp), "/var/lib/misc/%s_state", prefix);
		if ((f = fopen(tmp, "r")) == NULL) {
			/* if file does not exist then we create it with value "0".
			 * later on mwwatchdog will set it to 1 when it proves that
			 * the wan is actually working (wan can connect but still be not working)
			 */
			f = fopen(tmp, "w+");
			fprintf(f, (nvram_get_int("mwan_state_init") ? "1\n" : "0\n")); /* also allow to init state file with value "1" instead of "0" */
		}
		fclose(f);
	}
}

void mwan_status_update(void)
{
	int mwan_num, wan_unit, allwan_down = 1;
	unsigned int i;
	char prefix[16];

	mwan_num = nvram_get_int("mwan_num");
	if ((mwan_num == 1) || (mwan_num > MWAN_MAX))
		return;

	logmsg(LOG_DEBUG, "*** %s: IN, mwan_curr=%s", __FUNCTION__, mwan_curr);

	for (wan_unit = 1; wan_unit <= mwan_num; ++wan_unit) {
		memset(prefix, 0, sizeof(prefix));
		get_wan_prefix(wan_unit, prefix);
		get_wan_info(prefix);
		if (check_wanup(prefix)) {
			if (wan_info.wan_weight > 0)
				mwan_curr[wan_unit - 1] = '2'; /* connected, load balancing */
			else
				mwan_curr[wan_unit - 1] = '1'; /* connected, failover */
		}
		else
			mwan_curr[wan_unit - 1] = '0'; /* disconnected */
	}

	for (i = 1; i <= MWAN_MAX; i++) { /* let's stick to our iteration (1 ---> <= MWAN_MAX) */
		if (mwan_curr[i - 1] >= '2') {
			allwan_down = 0;
			break;
		}
	}

	if (allwan_down) {
		/* all connections down, searching failover interfaces */
		for (wan_unit = 1; wan_unit <= mwan_num; ++wan_unit) {
			if (mwan_curr[wan_unit - 1] == '1') {
				memset(prefix, 0, sizeof(prefix));
				get_wan_prefix(wan_unit, prefix);
				get_wan_info(prefix);
				if (wan_info.wan_weight == 0) {
					if (mwan_last[wan_unit - 1] != '2')
						logmsg(LOG_INFO, "Failover in action - WAN%d", (wan_unit - 1));

					mwan_curr[wan_unit - 1] = '2';
				}
			}
		}
	}

	logmsg(LOG_DEBUG, "*** %s: OUT, mwan_curr=%s", __FUNCTION__, mwan_curr);
}

void mwan_load_balance(void)
{
	int mwan_num, wan_unit, proto, wan_default, wan_weight = 0, not_allwan_down = 0;
	int argc, nh;
	char prefix[16];
	char buf[32];
	char cmd[2048];
	char via[MWAN_MAX][32];
	char dev[MWAN_MAX][16];
	char weight[MWAN_MAX][12];
	char del_via[32];
	char *del_route_argv[] = { "ip", "route", "del", "default", "via", del_via, "dev", wan_info.wan_name, NULL };
	char *lb_argv[8 + (MWAN_MAX * 8)];
	unsigned int i;

	mwan_num = nvram_get_int("mwan_num");
	if ((mwan_num == 1) || (mwan_num > MWAN_MAX))
		return;

	logmsg(LOG_DEBUG, "*** %s: IN, mwan_curr=%s", __FUNCTION__, mwan_curr);

	mwan_status_update();

	argc = 0;
	nh = 0;
	lb_argv[argc++] = "ip";
	lb_argv[argc++] = "route";
	lb_argv[argc++] = "replace";
	lb_argv[argc++] = "default";
	lb_argv[argc++] = "scope";
	lb_argv[argc++] = "global";

	for (wan_unit = 1; wan_unit <= mwan_num; ++wan_unit) {
		get_wan_prefix(wan_unit, prefix);
		get_wan_info(prefix);
		proto = get_wanx_proto(prefix);
		get_wan_ip(proto, buf, sizeof(buf));

		if (check_wanup(prefix) && (mwan_curr[wan_unit - 1] == '2')) { /* up and actively routing WAN */
			if (wan_info.wan_weight == 0) /* override weight for failover interface */
				wan_info.wan_weight = 1;

			strlcpy(via[nh], buf, sizeof(via[nh]));
			strlcpy(dev[nh], wan_info.wan_name, sizeof(dev[nh]));
			snprintf(weight[nh], sizeof(weight[nh]), "%d", wan_info.wan_weight);

			lb_argv[argc++] = "nexthop";
			lb_argv[argc++] = "via";
			lb_argv[argc++] = via[nh];
			lb_argv[argc++] = "dev";
			lb_argv[argc++] = dev[nh];
			lb_argv[argc++] = "weight";
			lb_argv[argc++] = weight[nh];

			++nh;
		}

		/* ip route del default via 10.0.10.1 dev ppp3 (from main route table) */
		if (strcmp(wan_info.wan_name, "none")) { /* skip disabled / disconnected ppp WAN */
			strlcpy(del_via, buf, sizeof(del_via));
			mwan_eval(del_route_argv, __FUNCTION__, prefix);
		}
	}

	/* check if all down */
	for (i = 1; i <= MWAN_MAX; i++) { /* let's stick to our iteration (1 ---> <= MWAN_MAX) */
		if (mwan_curr[i - 1] != '0') {
			not_allwan_down = 1;
			break;
		}
	}

	if (not_allwan_down) {
		lb_argv[argc] = NULL;
		mwan_argv_to_cmd(lb_argv, cmd, sizeof(cmd));
		logmsg(LOG_DEBUG, "*** %s: LOAD BALANCING: cmd=[%s]", __FUNCTION__, cmd);
	}
	else {
		wan_default = 1; /* first assume that main wan is WAN0 */
		for (wan_unit = 1; wan_unit <= mwan_num; ++wan_unit) {
			get_wan_prefix(wan_unit, prefix);
			get_wan_info(prefix);

			if (wan_unit == 1) {
				wan_weight = wan_info.wan_weight;
			}
			else if (wan_info.wan_weight > wan_weight) { /* but later choose WAN with highest weight */
				wan_default = wan_unit;
				wan_weight = wan_info.wan_weight;
			}
		}

		get_wan_prefix(wan_default, prefix);
		get_wan_info(prefix);
		proto = get_wanx_proto(prefix);

		get_wan_ip(proto, buf, sizeof(buf));

		strlcpy(via[nh], buf, sizeof(via[nh]));
		strlcpy(dev[nh], wan_info.wan_name, sizeof(dev[nh]));
		snprintf(weight[nh], sizeof(weight[nh]), "%d", wan_info.wan_weight);

		lb_argv[argc++] = "nexthop";
		lb_argv[argc++] = "via";
		lb_argv[argc++] = via[nh];
		lb_argv[argc++] = "dev";
		lb_argv[argc++] = dev[nh];
		lb_argv[argc++] = "weight";
		lb_argv[argc++] = weight[nh];
		lb_argv[argc] = NULL;

		mwan_argv_to_cmd(lb_argv, cmd, sizeof(cmd));
		logmsg(LOG_DEBUG, "*** %s: EMERGENCY ROUTE: cmd=[%s]", __FUNCTION__, cmd);
	}
	/* always execute lb_cmd: if all wans are down - add default route via WAN with highest weight */
	_eval(lb_argv, NULL, 0, NULL);

	logmsg(LOG_DEBUG, "*** %s: OUT, mwan_curr=%s", __FUNCTION__, mwan_curr);
}

int mwan_route_main(int argc, char **argv)
{
	int check_time = 15;
	int mwan_num;
	unsigned int i;
	FILE *fp;

	mkdir("/etc/iproute2", 0744);
	if ((fp = fopen("/etc/iproute2/rt_tables", "w")) != NULL) {
		for (i = 1; i <= MWAN_MAX; i++) {
			fprintf(fp, "%u WAN%u\n", i, i);
		}
#ifdef TCONFIG_PPTPD
		fprintf(fp, "%d %s\n", PPTPC_TABLE_ID, PPTPC_TABLE_NAME);
#endif
		fclose(fp);
	}

	/* add special type of proto for mwwatchdog to easily recognize it in 'ip route show table main' (OpenVPN/wireguard PBR) */
	if ((fp = fopen("/etc/iproute2/rt_protos", "w")) != NULL) {
		fprintf(fp, "99    mwwatchdog\n");
		fclose(fp);
	}

	mwan_num = nvram_get_int("mwan_num");
	if ((mwan_num == 1) || (mwan_num > MWAN_MAX))
		return 0;

	logmsg(LOG_DEBUG, "*** %s: MWAN: mwanroute launched", __FUNCTION__);

	while(1) {
		mwan_status_update();

		if (strcmp(mwan_last, mwan_curr)) {
			logmsg(LOG_WARNING, "Multiwan status has changed, last_status=%s, now_status=%s, Update multiwan policy", mwan_last, mwan_curr);
			mwan_load_balance();

			stop_dnsmasq();
			start_dnsmasq();
		}
		strlcpy(mwan_last, mwan_curr, sizeof(mwan_last));
		sleep(check_time);
	}
}
