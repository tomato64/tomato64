/*
 * PPTP CLIENT start/stop and configuration for Tomato
 * by Jean-Yves Avenard (c) 2008-2011
 *
 * Fixes/updates (C) 2018 - 2022 pedro
 *
 */


#include "rc.h"

#include <netdb.h>
#include <sys/types.h>
#include <sys/socket.h>

#define BUF_SIZE 128
#define PPTPC_DIR		"/etc/vpn"
#define PPTPC_TMP_DIR		"/tmp/ppp"
#define PPTPC_OPTIONS		PPTPC_DIR"/pptpc_options"
#define PPTPC_CLIENT		PPTPC_DIR"/pptpclient"
#define PPTPC_UP_SCRIPT		PPTPC_DIR"/pptpc_ip-up"
#define PPTPC_DOWN_SCRIPT	PPTPC_DIR"/pptpc_ip-down"

/* needed by logmsg() */
#define LOGMSG_DISABLE	DISABLE_SYSLOG_OSM
#define LOGMSG_NVDEBUG	"pptpc_debug"


void start_pptp_client(void)
{
	FILE *fd;
	int ok = 0;
	char *p;
	char buffer[BUF_SIZE];
	char *argv[5];
	int argc = 0;

	struct in_addr *addr;
	struct addrinfo hints;
	struct addrinfo *res;

	char *srv_addr = nvram_safe_get("pptp_client_srvip");

	if (serialize_restart("pptpclient", 1))
		return;

	unlink(PPTPC_UP_SCRIPT);
	unlink(PPTPC_DOWN_SCRIPT);
	unlink(PPTPC_OPTIONS);
	memset(buffer, 0, BUF_SIZE);
	snprintf(buffer, sizeof(buffer), PPTPC_CLIENT);
	unlink(buffer);

	/* Make sure vpn/ppp directory exists */
	mkdir(PPTPC_DIR, 0700);
	mkdir(PPTPC_TMP_DIR, 0700);

	/* Make sure symbolic link exists */
	ok |= symlink("/usr/sbin/pppd", buffer);
	ok |= symlink("/sbin/rc", PPTPC_UP_SCRIPT);
	ok |= symlink("/sbin/rc", PPTPC_DOWN_SCRIPT);
	if (ok) {
		logmsg(LOG_WARNING, "Creating symlink failed ...");
		return;
	}

	/* Get IP from hostname */
	if (inet_addr(srv_addr) == INADDR_NONE) {
		memset(&hints, 0, sizeof(hints));
		hints.ai_family   = AF_INET; /* TODO: IPv6 support(?) */
		hints.ai_socktype = SOCK_STREAM;
		hints.ai_flags    = AI_PASSIVE;

		if (getaddrinfo(srv_addr, NULL, &hints, &res) != 0) {
			logmsg(LOG_WARNING, "Can't get server IP ...");
			return;
		}
		struct sockaddr_in *ipv = (struct sockaddr_in *)res->ai_addr;
		addr = &(ipv->sin_addr);
		if (inet_ntop(res->ai_family, addr, srv_addr, sizeof(srv_addr)) == NULL) {
			freeaddrinfo(res);
			logmsg(LOG_WARNING, "Can't get server IP ...");
			return;
		}
		freeaddrinfo(res);
	}

	/* Generate ppp options */
	if ((fd = fopen(PPTPC_OPTIONS, "w")) != NULL) {
		ok = 1;
		fprintf(fd, "lock\n"
		            "noauth\n"
		            "refuse-eap\n"
		            "lcp-echo-interval 10\n"
		            "lcp-echo-failure 5\n"
		            "maxfail 0\n"
		            "persist\n"
		            "plugin pptp.so\n"
		            "pptp_server '%s'\n"
		            "idle 0\n"
		            "ipparam kelokepptpd\n"
		            "ktune\n"
		            "default-asyncmap nopcomp noaccomp\n"
		            "novj nobsdcomp nodeflate\n"
		            "holdoff 10\n"
		            "lcp-echo-adaptive\n"
		            "ipcp-accept-remote ipcp-accept-local noipdefault\n",
		            srv_addr);

		if (nvram_get_int("pptp_client_peerdns"))	/* 0: disable, 1 enable */
			fprintf(fd, "usepeerdns\n");

		/* MTU */
		/* see KB Q189595 -- historyless & mtu */
		if ((p = nvram_get("pptp_client_mtu")) == NULL)
			p = "1400";
		if (!nvram_get_int("pptp_client_mtuenable"))
			p = "1400";
		fprintf(fd, "mtu %s\n", p);

		/* MRU */
		if ((p = nvram_get("pptp_client_mru")) == NULL)
			p = "1400";
		if (!nvram_get_int("pptp_client_mruenable"))
			p = "1400";
		fprintf(fd, "mru %s\n", p);

		/* Login */
		if ((p = nvram_get("pptp_client_username")) == NULL)
			ok = 0;
		else
			fprintf(fd, "user \"%s\"\n", p);

		/* Password */
		if ((p = nvram_get("pptp_client_passwd")) == NULL)
			ok = 0;
		else
			fprintf(fd, "password \"%s\"\n", p);

		/* Encryption */
		switch (nvram_get_int("pptp_client_crypt")) {
			case 1:
				fprintf(fd, "nomppe nomppc\n");
				break;
			case 2:
				fprintf(fd, "nomppe-40\n"
				            "require-mppe\n"
				            "require-mppe-128\n");
				break;
			case 3:
				fprintf(fd, "require-mppe\n"
				            "require-mppe-40\n"
				            "require-mppe-56\n"
				            "require-mppe-128\n");
				break;
			default:
				break;
		}

		if (!nvram_get_int("pptp_client_stateless"))
			fprintf(fd, "mppe-stateful\n");
		else
			fprintf(fd, "nomppe-stateful\n");

		fprintf(fd, "unit 4\n"		/* UNIT (ppp4) */
		            "linkname pptpc\n"	/* link name for ID */
		            "ip-up-script "PPTPC_UP_SCRIPT"\n"
		            "ip-down-script "PPTPC_DOWN_SCRIPT"\n"
		            "%s\n",
		            nvram_safe_get("pptp_client_custom"));

		fclose(fd);
	}
	if (ok) {
		/* force route to PPTP server via selected wan */
		char *prefix = nvram_safe_get("pptp_client_usewan");
		if ((*prefix) && strcmp(prefix, "none")) {
			memset(buffer, 0, BUF_SIZE);
			snprintf(buffer, sizeof(buffer), "ip rule del lookup %d pref 120", get_wan_unit(prefix));
			system(buffer);
			memset(buffer, 0, BUF_SIZE);
			snprintf(buffer, sizeof(buffer), "ip rule add to %s lookup %d pref 120", srv_addr, get_wan_unit(prefix));
			system(buffer);
		}

		memset(buffer, 0, BUF_SIZE);
		snprintf(buffer, sizeof(buffer), PPTPC_CLIENT" file "PPTPC_OPTIONS);

		for (argv[argc = 0] = strtok(buffer, " "); argv[argc] != NULL; argv[++argc] = strtok(NULL, " "));
		if (_eval(argv, NULL, 0, NULL)) {
			logmsg(LOG_WARNING, "Creating pptp tunnel failed ...");
			return;
		}
		f_write(PPTPC_CLIENT"_connecting", NULL, 0, 0, 0);
	}
	else
		logmsg(LOG_WARNING, "Found error in configuration - aborting ...");
}

void stop_pptp_client(void)
{
	int argc;
	char *argv[8];
	char buffer[BUF_SIZE];
	char *prefix = nvram_safe_get("pptp_client_usewan");

	if (serialize_restart("pptpclient", 0))
		return;

	killall_tk_period_wait("pptpc_ip-up", 50);
	killall_tk_period_wait("pptpc_ip-down", 50);
	killall_tk_period_wait("pptpclient", 50);

	/* remove forced route to PPTP server via selected wan */
	if ((*prefix) && strcmp(prefix, "none")) {
		memset(buffer, 0, BUF_SIZE);
		snprintf(buffer, sizeof(buffer), "ip rule del lookup %d pref 120", get_wan_unit(prefix));
		system(buffer);
	}

	/* Delete all files for this client */
	unlink(PPTPC_CLIENT"_connecting");
	memset(buffer, 0, BUF_SIZE);
	snprintf(buffer, sizeof(buffer), "rm -rf "PPTPC_CLIENT" "PPTPC_DOWN_SCRIPT" "PPTPC_UP_SCRIPT" "PPTPC_OPTIONS" "PPTPC_TMP_DIR"/resolv.conf");
	for (argv[argc = 0] = strtok(buffer, " "); argv[argc] != NULL; argv[++argc] = strtok(NULL, " "));
	_eval(argv, NULL, 0, NULL);

	/* Attempt to remove directories. Will fail if not empty */
	rmdir(PPTPC_DIR);
	rmdir(PPTPC_TMP_DIR);
}

void start_pptp_client_eas(void)
{
	if (nvram_get_int("pptp_client_eas"))
		start_pptp_client();
	else
		stop_pptp_client();
}

void stop_pptp_client_eas(void)
{
	if (nvram_get_int("pptp_client_eas"))
		stop_pptp_client();
}

void pptp_client_firewall(const char *table, const char *opt, _tf_ipt_write ipt_writer)
{
	char *pptpcface = nvram_safe_get("pptp_client_iface");
	char *srvsub = nvram_safe_get("pptp_client_srvsub");
	char *srvsubmsk = nvram_safe_get("pptp_client_srvsubmsk");
	int dflroute = nvram_get_int("pptp_client_dfltroute");

	if (pidof("pptpclient") < 0 || (!pptpcface) || (!*pptpcface) || (!strcmp(pptpcface, "none")))
		return;

	chains_log_detection();

	if ((!strcmp(table, "INPUT")) && dflroute) {
		ipt_writer("-A INPUT -s %s/%s -i %s -j %s\n", srvsub, srvsubmsk, pptpcface, chain_in_accept);
	}
	else if ((!strcmp(table, "OUTPUT")) && dflroute) {
		ipt_writer("-A OUTPUT -d %s/%s -o %s -j %s\n", srvsub, srvsubmsk, pptpcface, chain_out_accept);
	}
	else if ((!strcmp(table, "FORWARD")) && dflroute) {
		ipt_writer("-A FORWARD -d %s/%s -o %s -j ACCEPT\n", srvsub, srvsubmsk, pptpcface);
		ipt_writer("-A FORWARD -s %s/%s -i %s -j ACCEPT\n", srvsub, srvsubmsk, pptpcface);
	}
	else if ((!strcmp(table, "POSTROUTING")) && nvram_get_int("pptp_client_nat")) {
		/* PPTP Client NAT */
		if (nvram_get_int("ne_snat") != 1)
			ipt_writer("-A POSTROUTING %s -o %s -j MASQUERADE\n", opt, pptpcface);
		else
			ipt_writer("-A POSTROUTING %s -o %s -j SNAT --to-source %s\n", opt, pptpcface, nvram_safe_get("pptp_client_ipaddr"));
	}
}

int write_pptp_client_resolv(FILE* f)
{
	FILE *dnsf;
	int usepeer;
	int ch;

	if ((usepeer = nvram_get_int("pptp_client_peerdns")) <= 0)
		return 0;

	if (pidof("pptpclient") > 0) { /* write DNS only for active client */
		if (!(dnsf = fopen(PPTPC_TMP_DIR"/resolv.conf", "r" )))
			return 0;

		fputs("# pptp client dns:\n", f);
		while (!feof(dnsf)) {
			ch = fgetc(dnsf);
			fputc(ch == EOF ? '\n' : ch, f);
		}
		fclose(dnsf);
	}

	return (usepeer == 2) ? 1 : 0;
}

static void pptp_client_add_route(void)
{
	if (nvram_get_int("pptp_client_dfltroute") == 1) {
		char buffer[BUF_SIZE];
		memset(buffer, 0, BUF_SIZE);
		snprintf(buffer, sizeof(buffer), "ip route replace default scope global via %s dev %s", nvram_safe_get("pptp_client_ipaddr"), nvram_safe_get("pptp_client_iface"));
		system(buffer);
	}
}

static void pptp_client_del_route(void)
{
	char buffer[BUF_SIZE];
	char pmw[] = "wanXX";
	char *prefix = nvram_safe_get("pptp_client_usewan");

	/* remove default route */
	if (nvram_get_int("pptp_client_dfltroute") == 1) {

		/* delete default route via PPTP */
		memset(buffer, 0, BUF_SIZE);
		snprintf(buffer, sizeof(buffer), "ip route del default via %s dev %s", nvram_safe_get("pptp_client_ipaddr"), nvram_safe_get("pptp_client_iface"));
		system(buffer);

		char *wan_ipaddr, *wan_gw, *wan_iface;
		wanface_list_t wanfaces;

		/* restore default route via binded WAN */
		if (check_wanup(prefix)) {
			wan_ipaddr = (char *)get_wanip(prefix);
			wan_gw = wan_gateway(prefix);
			memcpy(&wanfaces, get_wanfaces(prefix), sizeof(wanfaces));
			wan_iface = wanfaces.iface[0].name;
		}
		/* or via last primary wan */
		else {
			int num = nvram_get_int("wan_primary");
			get_wan_prefix(num, pmw);
			wan_ipaddr = (char *)get_wanip(pmw);
			wan_gw = wan_gateway(pmw);
			memcpy(&wanfaces, get_wanfaces(pmw), sizeof(wanfaces));
			wan_iface = wanfaces.iface[0].name;
			prefix = pmw;
		}

		if (check_wanup(prefix)) {
			int proto = get_wanx_proto(prefix);
			memset(buffer, 0, BUF_SIZE);
			snprintf(buffer, sizeof(buffer), "ip route add default via %s dev %s", (proto == WP_DHCP || proto == WP_LTE || proto == WP_STATIC) ? wan_gw : wan_ipaddr, wan_iface);
			system(buffer);
		}
	}
}

static void pptp_client_del_table(void)
{
	char buffer[BUF_SIZE];

	/* ip route flush table PPTP (remove all PPTP routes) */
	memset(buffer, 0, BUF_SIZE);
	snprintf(buffer, sizeof(buffer), "ip route flush table %s", PPTP_CLIENT_TABLE_NAME);
	system(buffer);

	/* ip rule del table PPTP pref 105 (from PPTP_IP) */
	memset(buffer, 0, BUF_SIZE);
	snprintf(buffer, sizeof(buffer), "ip rule del table %s pref 10%d", PPTP_CLIENT_TABLE_NAME, PPTP_CLIENT_TABLE_ID);
	system(buffer);

	/* ip rule del table PPTP pref 110 (to PPTP_DNS) */
	memset(buffer, 0, BUF_SIZE);
	snprintf(buffer, sizeof(buffer), "ip rule del table %s pref 110", PPTP_CLIENT_TABLE_NAME);
	system(buffer); /* del PPTP DNS1 */
	system(buffer); /* del PPTP DNS2 */

	/* ip rule del fwmark 0x500/0xf00 table PPTP pref 125 (FWMARK) */
	memset(buffer, 0, BUF_SIZE);
	snprintf(buffer, sizeof(buffer), "ip rule del table %s pref 12%d", PPTP_CLIENT_TABLE_NAME, PPTP_CLIENT_TABLE_ID);
	system(buffer);
}

/* set pptp_client ip route table & ip rule table */
static void pptp_client_add_table(void)
{
	int mwan_num, i, wanid, proto;
	char buffer[BUF_SIZE];
	char ip_cidr[32];
	char remote_cidr[32];
	char sPrefix[] = "wanXX";
	char tmp[100];

	char *pptp_client_ipaddr = nvram_safe_get("pptp_client_ipaddr");
	char *pptp_client_gateway = nvram_safe_get("pptp_client_gateway");
	char *pptp_client_iface = nvram_safe_get("pptp_client_iface");
	const dns_list_t *pptp_dns = get_dns("pptp_client");

	pptp_client_del_table();

	mwan_num = nvram_get_int("mwan_num");

	/*
	 * RULES
	 */

	/* ip rule add from PPTP_IP table PPTP pref 105 */
	memset(buffer, 0, BUF_SIZE);
	snprintf(buffer, sizeof(buffer), "ip rule add from %s table %s pref 10%d", pptp_client_ipaddr, PPTP_CLIENT_TABLE_NAME, PPTP_CLIENT_TABLE_ID);
	system(buffer);

	for (i = 0 ; i < pptp_dns->count; ++i) {
		/* ip rule add to PPTP_DNS table PPTP pref 110 */
		memset(buffer, 0, BUF_SIZE);
		snprintf(buffer, sizeof(buffer), "ip rule add to %s table %s pref 110", inet_ntoa(pptp_dns->dns[i].addr), PPTP_CLIENT_TABLE_NAME);
		system(buffer);
	}

	/* ip rule add fwmark 0x500/0xf00 table PPTP pref 125 */
	memset(buffer, 0, BUF_SIZE);
	snprintf(buffer, sizeof(buffer), "ip rule add fwmark 0x%d00/0xf00 table %s pref 12%d", PPTP_CLIENT_TABLE_ID, PPTP_CLIENT_TABLE_NAME, PPTP_CLIENT_TABLE_ID);
	system(buffer);

	/*
	 * ROUTES
	 */

	/* all active WANX in PPTP table ? FIXME! check iface / ifname / gw for various WAN types */
	for (wanid = 1; wanid <= mwan_num; ++wanid) {
		get_wan_prefix(wanid, sPrefix);
		if (check_wanup(sPrefix)) {
			proto = get_wanx_proto(sPrefix);
			memset(buffer, 0, BUF_SIZE);
			if (proto == WP_DHCP || proto == WP_LTE || proto == WP_STATIC) {
				/* wan ip/netmask, wan_iface, wan_ipaddr */
				get_cidr(nvram_safe_get(strlcat_r(sPrefix, "_ipaddr", tmp, sizeof(tmp))), nvram_safe_get(strlcat_r(sPrefix, "_netmask", tmp, sizeof(tmp))), ip_cidr, sizeof(ip_cidr));
				snprintf(buffer, sizeof(buffer), "ip route append %s dev %s proto kernel scope link src %s table %s",
					ip_cidr, /* wan ip / netmask */
					nvram_safe_get(strlcat_r(sPrefix, "_iface", tmp, sizeof(tmp))),
					nvram_safe_get(strlcat_r(sPrefix, "_ipaddr", tmp, sizeof(tmp))),
					PPTP_CLIENT_TABLE_NAME);
			}
			else if ((proto == WP_PPTP || proto == WP_L2TP || proto == WP_PPPOE) && using_dhcpc(sPrefix)) {
				/* MAN: wan_gateway, wan_ifname, wan_ipaddr */
				snprintf(buffer, sizeof(buffer), "ip route append %s dev %s proto kernel scope link src %s table %s",
					nvram_safe_get(strlcat_r(sPrefix, "_gateway", tmp, sizeof(tmp))),
					nvram_safe_get(strlcat_r(sPrefix, "_ifname", tmp, sizeof(tmp))),
					nvram_safe_get(strlcat_r(sPrefix, "_ipaddr", tmp, sizeof(tmp))),
					PPTP_CLIENT_TABLE_NAME);
				system(buffer);
				/* WAN: wan_gateway_get, wan_iface, wan_ppp_get_ip */
				snprintf(buffer, sizeof(buffer), "ip route append %s dev %s proto kernel scope link src %s table %s",
					nvram_safe_get(strlcat_r(sPrefix, "_gateway_get", tmp, sizeof(tmp))),
					nvram_safe_get(strlcat_r(sPrefix, "_iface", tmp, sizeof(tmp))),
					nvram_safe_get(strlcat_r(sPrefix, "_ppp_get_ip", tmp, sizeof(tmp))),
					PPTP_CLIENT_TABLE_NAME);
			}
			else {
				/* wan gateway, wan_iface, wan_ipaddr */
				snprintf(buffer, sizeof(buffer), "ip route append %s dev %s proto kernel scope link src %s table %s",
					wan_gateway(sPrefix), 
					nvram_safe_get(strlcat_r(sPrefix, "_iface", tmp, sizeof(tmp))),
					nvram_safe_get(strlcat_r(sPrefix, "_ipaddr", tmp, sizeof(tmp))),
					PPTP_CLIENT_TABLE_NAME);
			}
			system(buffer);
		}
	}

	/* ip route add 172.16.36.1 dev ppp4 proto kernel scope link src 172.16.36.13 */
	memset(buffer, 0, BUF_SIZE);
	snprintf(buffer, sizeof(buffer), "ip route append %s dev %s proto kernel scope link src %s", pptp_client_gateway, pptp_client_iface, pptp_client_ipaddr);
	system(buffer);

	for (wanid = 1; wanid <= mwan_num; ++wanid) {
		get_wan_prefix(wanid, sPrefix);
		if (check_wanup(sPrefix)) {
			memset(buffer, 0, BUF_SIZE);
			snprintf(buffer, sizeof(buffer), "ip route append %s dev %s proto kernel scope link src %s table %d", pptp_client_gateway, pptp_client_iface, pptp_client_ipaddr, wanid);
			system(buffer);
		}
	}

	/* ip route add 172.16.36.1 dev ppp4 proto kernel scope link src 172.16.36.13 table PPTP (pptp gw) */
	memset(buffer, 0, BUF_SIZE);
	snprintf(buffer, sizeof(buffer), "ip route append %s dev %s proto kernel scope link src %s table %s", pptp_client_gateway, pptp_client_iface, pptp_client_ipaddr, PPTP_CLIENT_TABLE_NAME);
	system(buffer);
	/* ip route add 192.168.1.0/24 dev br0 proto kernel scope link src 192.168.1.1 table PPTP (LAN) */
	get_cidr(nvram_safe_get("lan_ipaddr"), nvram_safe_get("lan_netmask"), ip_cidr, sizeof(ip_cidr));
	memset(buffer, 0, BUF_SIZE);
	snprintf(buffer, sizeof(buffer), "ip route append %s dev %s proto kernel scope link src %s table %s", ip_cidr, nvram_safe_get("lan_ifname"), nvram_safe_get("lan_ipaddr"), PPTP_CLIENT_TABLE_NAME);
	system(buffer);
	/* ip route add 127.0.0.0/8 dev lo scope link table PPTP (lo setup) */
	memset(buffer, 0, BUF_SIZE);
	snprintf(buffer, sizeof(buffer), "ip route append 127.0.0.0/8 dev lo scope link table %s", PPTP_CLIENT_TABLE_NAME);
	system(buffer);
	/* ip route add default via 10.0.10.1 dev ppp3 table PPTP (default route) */
	memset(buffer, 0, BUF_SIZE);
	snprintf(buffer, sizeof(buffer), "ip route append default via %s dev %s table %s", pptp_client_ipaddr, pptp_client_iface, PPTP_CLIENT_TABLE_NAME);
	system(buffer);

	/* PPTP network */
	if (!nvram_match("pptp_client_srvsub", "0.0.0.0") && !nvram_match("pptp_client_srvsubmsk", "0.0.0.0")) {
		/* add PPTP network to main table */
		get_cidr(nvram_safe_get("pptp_client_srvsub"), nvram_safe_get("pptp_client_srvsubmsk"), remote_cidr, sizeof(remote_cidr));
		memset(buffer, 0, BUF_SIZE);
		snprintf(buffer, sizeof(buffer), "ip route append %s via %s dev %s scope link table %s", remote_cidr, pptp_client_ipaddr, pptp_client_iface, "main");
		system(buffer);
		/* add PPTP network to all WANX tables */
		for (wanid = 1; wanid <= mwan_num; ++wanid) {
			get_wan_prefix(wanid, sPrefix);
			if (check_wanup(sPrefix)) {
				memset(buffer, 0, BUF_SIZE);
				snprintf(buffer, sizeof(buffer), "ip route append %s via %s dev %s scope link table %d", remote_cidr, pptp_client_ipaddr, pptp_client_iface, wanid);
				system(buffer);
			}
		}
		/* add PPTP network to table PPTP */
		get_cidr(nvram_safe_get("pptp_client_srvsub"), nvram_safe_get("pptp_client_srvsubmsk"), remote_cidr, sizeof(remote_cidr));
		memset(buffer, 0, BUF_SIZE);
		snprintf(buffer, sizeof(buffer), "ip route append %s via %s dev %s scope link table %s", remote_cidr, pptp_client_ipaddr, pptp_client_iface, PPTP_CLIENT_TABLE_NAME);
		system(buffer);
	}
}

int pptpc_ipup_main(int argc, char **argv)
{
	char *pptp_client_ifname;
	struct sysinfo si;

	/* ipup receives six arguments:
	 * <interface name> <tty device> <speed> <local IP address> <remote IP address> <ipparam>
	 * ppp1 vlan1 0 71.135.98.32 151.164.184.87 0
	 */

	logmsg(LOG_DEBUG, "*** IN %s: IFNAME=%s IPLOCAL=%s IPREMOTE=%s", __FUNCTION__, safe_getenv("IFNAME"), getenv("IPLOCAL"), getenv("IPREMOTE"));

	sysinfo(&si);
	f_write(PPTPC_CLIENT"_time", &si.uptime, sizeof(si.uptime), 0, 0);
	unlink(PPTPC_CLIENT"_connecting");

	pptp_client_ifname = safe_getenv("IFNAME");
	if ((!pptp_client_ifname) || (!*pptp_client_ifname) || (!wait_action_idle(10)))
		return -1;

	nvram_set("pptp_client_iface", pptp_client_ifname); /* ppp# */

	f_write_string(PPTPC_CLIENT"_link", argv[1], 0, 0);

	if (env2nv("IPLOCAL", "pptp_client_ipaddr"))
		nvram_set("pptp_client_netmask", "255.255.255.255");

	env2nv("IPREMOTE", "pptp_client_gateway");

	pptp_client_add_table();
	pptp_client_add_route();

	stop_firewall();
	start_firewall();

	stop_dnsmasq();
	dns_to_resolv();
	start_dnsmasq();

	return 0;
}

int pptpc_ipdown_main(int argc, char **argv)
{
	if (!wait_action_idle(10))
		return -1;

	pptp_client_del_table();
	pptp_client_del_route();

	unlink(PPTPC_CLIENT"_link");
	unlink(PPTPC_CLIENT"_time");
	unlink(PPTPC_CLIENT"_connecting");

	nvram_set("pptp_client_iface", "");
	nvram_set("pptp_client_ipaddr", "0.0.0.0");
	nvram_set("pptp_client_netmask", "0.0.0.0");
	nvram_set("pptp_client_gateway", "0.0.0.0");

	stop_firewall();
	start_firewall();

	stop_dnsmasq();
	dns_to_resolv();
	start_dnsmasq();

	return 1;
}
