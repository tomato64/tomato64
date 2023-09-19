/*
* Multi WAN
* By Arctic QQ:317869867 E-Mail:zengchen228@vip.qq.com
*/


#include "rc.h"

/* needed by logmsg() */
#define LOGMSG_DISABLE	DISABLE_SYSLOG_OS
#define LOGMSG_NVDEBUG	"mwan_debug"

#ifdef TCONFIG_MULTIWAN
static char mwan_curr[] = {'0', '0', '0', '0', '\0'};
static char mwan_last[] = {'0', '0', '0', '0', '\0'};
#else
static char mwan_curr[] = {'0', '0', '\0'};
static char mwan_last[] = {'0', '0', '\0'};
#endif

typedef struct
{
	char wan_iface[10];
	char wan_ifname[10];
	char wan_ipaddr[32];
	char wan_netmask[32];
	char wan_gateway[32];
	const dns_list_t *dns;
	int wan_weight;
} waninfo_t;

static waninfo_t wan_info;

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
	char tmp[100];
	int proto = get_wanx_proto(sPrefix);

	strlcpy(wan_info.wan_iface, nvram_safe_get(strlcat_r(sPrefix, "_iface", tmp, sizeof(tmp))), sizeof(wan_info.wan_iface));
	strlcpy(wan_info.wan_ifname, nvram_safe_get(strlcat_r(sPrefix, "_ifname", tmp, sizeof(tmp))), sizeof(wan_info.wan_ifname));
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

	if ((proto == WP_L2TP) || (proto == WP_PPTP) || (proto == WP_PPPOE) || (proto == WP_PPP3G))
		strlcpy(wan_info.wan_netmask, "255.255.255.255", sizeof(wan_info.wan_netmask));
	else
		strlcpy(wan_info.wan_netmask, nvram_safe_get(strlcat_r(sPrefix, "_netmask", tmp, sizeof(tmp))), sizeof(wan_info.wan_netmask));

	strlcpy(wan_info.wan_gateway, wan_gateway(sPrefix), sizeof(wan_info.wan_gateway));
	wan_info.dns = get_dns(sPrefix); /* static buffer */
	wan_info.wan_weight = atoi(nvram_safe_get(strlcat_r(sPrefix, "_weight", tmp, sizeof(tmp))));
}

void get_wan_ip(int proto, char *name, const size_t buf_sz)
{
	if ((proto == WP_DHCP) || (proto == WP_LTE) || (proto == WP_STATIC))
		snprintf(name, buf_sz, "%s", wan_info.wan_gateway);
	else
		snprintf(name, buf_sz, "%s", wan_info.wan_ipaddr);
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
	char cmd[256];

	logmsg(LOG_DEBUG, "*** IN %s", __FUNCTION__);

	wan_unit = get_wan_unit(sPrefix);
	get_wan_info(sPrefix); /* get the current wan infos to work with */

	/* ip rule del table WAN1 pref 101 (gateway); table: 1 to 4; pref: 101 to 104 */
	memset(cmd, 0, sizeof(cmd));
	snprintf(cmd, sizeof(cmd), "ip rule del table %d pref 10%d", wan_unit, wan_unit);
	logmsg(LOG_DEBUG, "*** %s: %s, cmd=%s", __FUNCTION__, sPrefix, cmd);
	system(cmd);
	
	/* ip rule del table WAN1 pref 111 (dns); table: 1 to 4; pref: 111 to 114 */
	/* delete only active & valid DNS; two options right now: only AUTO DNS server (1x DNS) or Manual DNS server (2x DNS) (see GUI network-basic.asp) */
	for (i = 0 ; i < wan_info.dns->count; ++i) {
		memset(cmd, 0, sizeof(cmd));
		snprintf(cmd, sizeof(cmd), "ip rule del table %d pref 11%d", wan_unit, wan_unit);
		logmsg(LOG_DEBUG, "*** %s: %s, cmd=%s", __FUNCTION__, sPrefix, cmd);
		system(cmd);
	}

	/* ip rule del fwmark 0x100/0xf00 table 1 pref 121 (mark); table: 1 to 4; pref: 121 to 124 */
	memset(cmd, 0, sizeof(cmd));
	snprintf(cmd, sizeof(cmd), "ip rule del table %d pref 12%d", wan_unit, wan_unit);
	logmsg(LOG_DEBUG, "*** %s: %s, cmd=%s", __FUNCTION__, sPrefix, cmd);
	system(cmd);

	logmsg(LOG_DEBUG, "*** OUT %s", __FUNCTION__);
}

/* set multiwan ip route table & ip rule table */
void mwan_table_add(char *sPrefix)
{
	int mwan_num, wan_unit, proto;
	int wanid;
	int i;
	char cmd[256];
	char buf[32];

	logmsg(LOG_DEBUG, "*** IN %s", __FUNCTION__);

	/* delete already existed table first */
	mwan_table_del(sPrefix);

	mwan_num = nvram_get_int("mwan_num");
	if ((mwan_num == 1) || (mwan_num > MWAN_MAX))
		return;

	wan_unit = get_wan_unit(sPrefix);
	get_wan_info(sPrefix); /* get the current wan infos to work with */
	proto = get_wanx_proto(sPrefix);

	if (check_wanup(sPrefix)) {
		/* ip rule add from WAN_IP table route_id pref 10X */
		memset(cmd, 0, sizeof(cmd));
		snprintf(cmd, sizeof(cmd), "ip rule add from %s table %d pref 10%d", wan_info.wan_ipaddr, wan_unit, wan_unit);
		logmsg(LOG_DEBUG, "*** %s: %s, cmd=%s", __FUNCTION__, sPrefix, cmd);
		system(cmd);
		
		/* set the routing rules of DNS */
		for (i = 0; i < wan_info.dns->count; ++i) {
			memset(cmd, 0, sizeof(cmd));
			snprintf(cmd, sizeof(cmd), "ip rule add to %s table %d pref 11%d", inet_ntoa(wan_info.dns->dns[i].addr), wan_unit, wan_unit);
			logmsg(LOG_DEBUG, "*** %s: %s, cmd=%s", __FUNCTION__, sPrefix, cmd);
			system(cmd);
		}

		/* ip rule add fwmark 0x100/0xf00 table 1 pref 121 */
		memset(cmd, 0, sizeof(cmd));
		snprintf(cmd, sizeof(cmd), "ip rule add fwmark 0x%d00/0xf00 table %d pref 12%d", wan_unit, wan_unit, wan_unit);
		logmsg(LOG_DEBUG, "*** %s: %s, cmd=%s", __FUNCTION__, sPrefix, cmd);
		system(cmd);

		for (wanid = 1; wanid <= mwan_num; ++wanid) {
			/* ip route add 10.0.10.1 dev ppp3 proto kernel scope link table route_id */
			memset(cmd, 0, sizeof(cmd));
			if ((proto == WP_DHCP) || (proto == WP_LTE) || (proto == WP_STATIC)) {
				memset(buf, 0, sizeof(buf));
				get_cidr(wan_info.wan_ipaddr, wan_info.wan_netmask, buf, sizeof(buf));
				snprintf(cmd, sizeof(cmd), "ip route append %s dev %s proto kernel scope link src %s table %d", buf, wan_info.wan_iface, wan_info.wan_ipaddr, wanid);
			}
			else
				snprintf(cmd, sizeof(cmd), "ip route append %s dev %s proto kernel scope link src %s table %d", wan_info.wan_gateway, wan_info.wan_iface, wan_info.wan_ipaddr, wanid);

			logmsg(LOG_DEBUG, "*** %s: %s, cmd=%s", __FUNCTION__, sPrefix, cmd);
			system(cmd);
		}

		/* ip route add 192.168.1.0/24 dev br0 proto kernel scope link  src 192.168.1.1  table 2 */
		for (i = 0; i < BRIDGE_COUNT; i++) {
			char nvram_var[16];
			char* lan_ifname;
			char* lan_ipaddr;
			char* lan_netmask;

			memset(nvram_var, 0, sizeof(nvram_var));
			snprintf(nvram_var, sizeof(nvram_var), (i == 0 ? "lan_ifname" : "lan%d_ifname"), i);
			lan_ifname = nvram_safe_get(nvram_var);
			memset(nvram_var, 0, sizeof(nvram_var));
			snprintf(nvram_var, sizeof(nvram_var), (i == 0 ? "lan_ipaddr" : "lan%d_ipaddr"), i);
			lan_ipaddr = nvram_safe_get(nvram_var);
			memset(nvram_var, 0, sizeof(nvram_var));
			snprintf(nvram_var, sizeof(nvram_var), (i == 0 ? "lan_netmask" : "lan%d_netmask"), i);
			lan_netmask = nvram_safe_get(nvram_var);

			if ((lan_ifname[0] == '\0') || (lan_ipaddr[0] == '\0') || (lan_netmask[0] == '\0'))
				continue;

			memset(buf, 0, sizeof(buf));
			get_cidr(lan_ipaddr, lan_netmask, buf, sizeof(buf));
			memset(cmd, 0, sizeof(cmd));
			snprintf(cmd, sizeof(cmd), "ip route append %s dev %s proto kernel scope link src %s table %d", buf, lan_ifname, lan_ipaddr, wan_unit);
			logmsg(LOG_DEBUG, "*** %s: %s, cmd=%s", __FUNCTION__, sPrefix, cmd);
			system(cmd);
		}

		/* ip route add 127.0.0.0/8 dev lo scope link table 1 */
		memset(cmd, 0, sizeof(cmd));
		snprintf(cmd, sizeof(cmd), "ip route append 127.0.0.0/8 dev lo scope link table %d", wan_unit);
		logmsg(LOG_DEBUG, "*** %s: %s, cmd=%s", __FUNCTION__, sPrefix, cmd);
		system(cmd);

		/* ip route add default via 10.0.10.1 dev ppp3 table route_id */
		memset(cmd, 0, sizeof(cmd));
		memset(buf, 0, sizeof(buf));
		get_wan_ip(proto, buf, sizeof(buf));
		snprintf(cmd, sizeof(cmd), "ip route append default via %s dev %s table %d", buf, wan_info.wan_iface, wan_unit);
		logmsg(LOG_DEBUG, "*** %s: %s, cmd=%s", __FUNCTION__, sPrefix, cmd);
		system(cmd);
	}

	logmsg(LOG_DEBUG, "*** OUT %s", __FUNCTION__);
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

		memset(tmp, 0, sizeof(tmp));
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
	int mwan_num, wan_unit;
	char prefix[16];

	mwan_num = nvram_get_int("mwan_num");
	if ((mwan_num == 1) || (mwan_num > MWAN_MAX))
		return;

	logmsg(LOG_DEBUG, "*** IN %s: mwan_curr=%s", __FUNCTION__, mwan_curr);

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

#ifdef TCONFIG_MULTIWAN
	if ((mwan_curr[0] < '2') && (mwan_curr[1] < '2') && (mwan_curr[2] < '2') && (mwan_curr[3] < '2')) {
#else
	if ((mwan_curr[0] < '2') && (mwan_curr[1] < '2')) {
#endif
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

	logmsg(LOG_DEBUG, "*** OUT %s: mwan_curr=%s", __FUNCTION__, mwan_curr);
}

void mwan_load_balance(void)
{
	int mwan_num, wan_unit, proto, wan_default, wan_weight = 0;
	char prefix[16];
	char cmd[256];
	char lb_cmd[2048];
	char buf[32];

	mwan_num = nvram_get_int("mwan_num");
	if ((mwan_num == 1) || (mwan_num > MWAN_MAX))
		return;

	logmsg(LOG_DEBUG, "*** IN %s: mwan_curr=%s", __FUNCTION__, mwan_curr);

	mwan_status_update();

	memset(lb_cmd, 0, sizeof(lb_cmd)); /* loadbalancing cmd */
	strlcpy(lb_cmd, "ip route replace default scope global", sizeof(lb_cmd));

	for (wan_unit = 1; wan_unit <= mwan_num; ++wan_unit) {
		memset(prefix, 0, sizeof(prefix));
		get_wan_prefix(wan_unit, prefix);
		get_wan_info(prefix);
		proto = get_wanx_proto(prefix);

		if (check_wanup(prefix) && (mwan_curr[wan_unit - 1] == '2')) { /* up and actively routing WAN */
			if (wan_info.wan_weight == 0) /* override weight for failover interface */
				wan_info.wan_weight = 1;

			memset(cmd, 0, sizeof(cmd));
			memset(buf, 0, sizeof(buf));
			get_wan_ip(proto, buf, sizeof(buf));
			snprintf(cmd, sizeof(cmd), " nexthop via %s dev %s weight %d", buf, wan_info.wan_iface, wan_info.wan_weight);
			strlcat(lb_cmd, cmd, sizeof(lb_cmd));
		}
		/* ip route del default via 10.0.10.1 dev ppp3 (from main route table) */
		if (strcmp(wan_info.wan_iface, "none")) { /* skip disabled / disconnected ppp WAN */
			memset(cmd, 0, sizeof(cmd));
			memset(buf, 0, sizeof(buf));
			get_wan_ip(proto, buf, sizeof(buf));
			snprintf(cmd, sizeof(cmd), "ip route del default via %s dev %s", buf, wan_info.wan_iface);
			logmsg(LOG_DEBUG, "*** %s: %s, cmd=%s", __FUNCTION__, prefix, cmd);
			system(cmd);
		}
	}
#ifdef TCONFIG_MULTIWAN
	if (strcmp(mwan_curr, "0000")) {
#else
	if (strcmp(mwan_curr, "00")) {
#endif
		logmsg(LOG_DEBUG, "*** %s: cmd=%s", __FUNCTION__, lb_cmd);
	}
	else {
		wan_default = 1; /* first assume that main wan is WAN0 */
		for (wan_unit = 1; wan_unit <= mwan_num; ++wan_unit) {
			memset(prefix, 0, sizeof(prefix));
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

		memset(prefix, 0, sizeof(prefix));
		get_wan_prefix(wan_default, prefix);
		get_wan_info(prefix);
		proto = get_wanx_proto(prefix);

		memset(cmd, 0, sizeof(cmd));
		memset(buf, 0, sizeof(buf));
		get_wan_ip(proto, buf, sizeof(buf));
		snprintf(cmd, sizeof(cmd), " nexthop via %s dev %s weight %d", buf, wan_info.wan_iface, wan_info.wan_weight);
		strlcat(lb_cmd, cmd, sizeof(lb_cmd));

		logmsg(LOG_DEBUG, "*** %s: EMERGENCY ROUTE: cmd=%s", __FUNCTION__, lb_cmd);
	}
	/* always execute lb_cmd: if all wans are down - add default route via WAN with highest weight */
	system(lb_cmd);

	logmsg(LOG_DEBUG, "*** OUT %s: mwan_curr=%s", __FUNCTION__, mwan_curr);
}

int mwan_route_main(int argc, char **argv)
{
	int check_time = 15;
	int mwan_num;
	FILE *fp;

	mkdir("/etc/iproute2", 0744);
	if ((fp = fopen("/etc/iproute2/rt_tables", "w")) != NULL) {
		fprintf(fp, "1 WAN1\n"
		            "2 WAN2\n"
#ifdef TCONFIG_MULTIWAN
		            "3 WAN3\n"
		            "4 WAN4\n"
#endif
#ifdef TCONFIG_PPTPD
		            "%d %s\n", PPTP_CLIENT_TABLE_ID, PPTP_CLIENT_TABLE_NAME
#endif
		);
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
