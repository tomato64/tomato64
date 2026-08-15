/*
 * dnsmasq.c
 *
 * Copyright (C) 2025 - 2026 FreshTomato
 * https://freshtomato.org/
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston,
 * MA 02111-1307 USA
 *
 */


#include "rc.h"

#include <wlutils.h>


/* needed by logmsg() */
#define LOGMSG_DISABLE		DISABLE_SYSLOG_OSM
#define LOGMSG_NVDEBUG		"dnsmasq_debug"


const char dmhosts[]      = "/etc/hosts.dnsmasq";
const char dmresolv[]     = "/etc/resolv.dnsmasq";
const char dmipset[]      = "/etc/dnsmasq.ipset";
const char dmcfg[]        = "/etc/dnsmasq.conf";
const char dmcfgcustom[]  = "/etc/dnsmasq.custom";
const char dmpid[]        = "/var/run/dnsmasq.pid";
const char dmcfgtrust[]   = "/etc/trust-anchors.conf";
const char dmadblock[]    = "/etc/dnsmasq.adblock";
const char dmwarning[]    = "Warning! Dnsmasq Custom configuration contains a disruptive syntax error. The Custom configuration is now excluded to allow dnsmasq to operate";
const char resolvcfg[]    = "/etc/resolv.conf";
const char resolvcfgrom[] = "/rom/etc/resolv.conf";

pid_t pid_dnsmasq = -1;
static void start_dnsmasq_wet(void);

static int check_bridge_modes(void) {
	/* check wireless ethernet bridge (wet) after stop_dnsmasq() */
#ifndef TOMATO64
	if (foreach_wif(1, NULL, is_wet)) {
		logmsg(LOG_INFO, "starting dnsmasq for wireless ethernet bridge mode");
		start_dnsmasq_wet();
		return 1;
	}
#ifdef TCONFIG_BCMWL6
	/* check media bridge (psta) after stop_dnsmasq() */
	if (foreach_wif(1, NULL, is_psta)) {
		logmsg(LOG_INFO, "starting dnsmasq for media bridge mode");
		start_dnsmasq_wet();
		return 1;
	}
#endif /* TCONFIG_BCMWL6 */
#endif /* TOMATO64 */

	return 0;
}

static const char *bridge_nvram_get(unsigned int bridge, const char *suffix, char *key, const size_t key_size)
{
	if (bridge == 0)
		snprintf(key, key_size, "lan_%s", suffix);
	else
		snprintf(key, key_size, "lan%u_%s", bridge, suffix);

	return nvram_safe_get(key);
}

static int bridge_has_l3(unsigned int bridge)
{
	char key[32];
	const char *ipaddr = bridge_nvram_get(bridge, "ipaddr", key, sizeof(key));

	return (*ipaddr && (strcmp(ipaddr, "0.0.0.0") != 0));
}

static void write_l2_bridge_exclusions(FILE *f)
{
	unsigned int bridge;
	char key[32];
	const char *ifname;

	for (bridge = 0; bridge < BRIDGE_COUNT; bridge++) {
		ifname = bridge_nvram_get(bridge, "ifname", key, sizeof(key));
		if ((*ifname) && (strncmp(ifname, "br", 2) == 0) && !bridge_has_l3(bridge))
			fprintf(f, "except-interface=%s\n", ifname);
	}
}

static void write_basic_config(FILE *f)
{
	const char *nv;
	unsigned int min_port;
	int flags;

	/* domain from NVRAM */
	if (((nv = nvram_get("wan_domain")) != NULL) || ((nv = nvram_get("wan_get_domain")) != NULL)) {
		if (*nv)
			fprintf(f, "domain=%s\n", nv);
	}

	/* determine min-port */
	nv = nvram_safe_get("dns_minport");
	if (nv && *nv)
		min_port = atoi(nv);
	else
		min_port = 4096;

	/* base dnsmasq config */
	fprintf(f, "pid-file=%s\n"
	           "resolv-file=%s\n"				/* the real stuff is here */
	           "expand-hosts\n"				/* expand hostnames in hosts file */
	           "min-port=%u\n"				/* min port used for random src port */
	           "no-negcache\n"				/* disable negative caching */
	           "dhcp-name-match=set:wpad-ignore,wpad\n"	/* protect against VU#598349 */
	           "dhcp-ignore-names=tag:wpad-ignore\n"
	           "edns-packet-max=%d\n",
	           dmpid,
	           dmresolv,
	           min_port,
	           nvram_get_int("dnsmasq_edns_size"));

	/* DNS rebinding protection, will discard upstream RFC1918 responses */
	if (nvram_get_int("dns_norebind")) {
		fprintf(f, "stop-dns-rebind\n"
		           "rebind-localhost-ok\n");
	}

	/* instruct clients like Firefox to not auto-enable DoH */
	if (nvram_get_int("dns_priv_override")) {
		fprintf(f, "address=/use-application-dns.net/mask.icloud.com/mask-h2.icloud.com/\n"
		           "address=/_dns.resolver.arpa/\n");
	}

	/* forward local domain queries to upstream DNS */
	if (nvram_get_int("dns_fwd_local") != 1) {
		fprintf(f, "bogus-priv\n"			/* don't forward private reverse lookups upstream */
		           "domain-needed\n");			/* don't forward plain name queries upstream */
	}

	/* quiet mode flags */
	flags = nvram_get_int("dnsmasq_q");
	if (flags) {
		if (flags & 1)
			fprintf(f, "quiet-dhcp\n");
#ifdef TCONFIG_IPV6
		if (flags & 2)
			fprintf(f, "quiet-dhcp6\n");
		if (flags & 4)
			fprintf(f, "quiet-ra\n");
#endif
	}

	/* DNSCrypt/Stubby */
#ifdef TCONFIG_DNSCRYPT
	if (nvram_get_int("dnscrypt_proxy")) {
		fprintf(f, "server=127.0.0.1#%s\n", nvram_safe_get("dnscrypt_port"));
		if (nvram_match("dnscrypt_priority", "1"))
			fprintf(f, "strict-order\n");

		if (nvram_match("dnscrypt_priority", "2"))
			fprintf(f, "no-resolv\n");
	}
#endif
#ifdef TCONFIG_STUBBY
	if (nvram_get_int("stubby_proxy")) {
		fprintf(f, "server=127.0.0.1#%s\n", nvram_safe_get("stubby_port"));
		if (nvram_match("stubby_priority", "1"))
			fprintf(f, "strict-order\n");

		if (nvram_match("stubby_priority", "2"))
			fprintf(f, "no-resolv\n");
	}
#endif

	/* DHCP options */
	flags = nvram_get_int("dhcpd_lmax");

	fprintf(f, "dhcp-lease-max=%d\n", (flags > 0 ? flags : 255));
	if (nvram_get_int("dhcpd_auth") >= 0)
		fprintf(f, "dhcp-option=252,\"\\n\"\n"
		           "dhcp-authoritative\n");

	/* NTP server */
	if (nvram_get_int("ntpd_enable"))
		fprintf(f, "dhcp-option-force=42,%s\n", "0.0.0.0");

	/* generate a name for DHCP clients which do not otherwise have one */
	if (nvram_get_int("dnsmasq_gen_names"))
		fprintf(f, "dhcp-generate-names\n");

	/* DNSSEC and Stubby settings */
#if defined(TCONFIG_DNSSEC) || defined(TCONFIG_STUBBY)
	if (nvram_get_int("dnssec_enable")) {
#ifdef TCONFIG_STUBBY
		if ((!nvram_get_int("stubby_proxy")) || (nvram_match("dnssec_method", "0"))) {
#endif
#ifdef TCONFIG_DNSSEC
			fprintf(f, "conf-file=%s\n"
			           "dnssec\n",
			           dmcfgtrust);

			if (!nvram_get_int("ntp_ready"))
				fprintf(f, "dnssec-no-timecheck\n");
#endif
#ifdef TCONFIG_STUBBY
		}
		else
			fprintf(f, "proxy-dnssec\n");
#endif
	}
#endif /* TCONFIG_DNSSEC || TCONFIG_STUBBY */
}

static void write_tor_dns(FILE *f)
{
#ifdef TCONFIG_TOR
	char buf[16];
	int i;
	const char *t_ip = nvram_safe_get("lan_ipaddr"); /* default */

	if (nvram_get_int("tor_enable") && nvram_get_int("dnsmasq_onion_support")) {
		for (i = 1; i < BRIDGE_COUNT; i++) {
			snprintf(buf, sizeof(buf), "br%d", i);
			if (nvram_match("tor_iface", buf)) {
				snprintf(buf, sizeof(buf), "lan%d_ipaddr", i);
				t_ip = nvram_safe_get(buf);
				break;
			}
		}
		fprintf(f, "server=/onion/%s#%s\n", t_ip, nvram_safe_get("tor_dnsport"));
	}
#endif
}

static void write_wan_dns(FILE *f, const int mwan_num)
{
	int wan_unit, proto, n;
	char wan_prefix[16], key[32];
	const char *nv;
	const dns_list_t *dns;

	for (wan_unit = 1; wan_unit <= mwan_num; ++wan_unit) {
		nv = NULL;

		memset(wan_prefix, 0, sizeof(wan_prefix));
		get_wan_prefix(wan_unit, wan_prefix);

		/* allow RFC1918 responses for server domain (fix connect PPTP/L2TP WANs) */
		proto = get_wanx_proto(wan_prefix);
		if (proto == WP_PPTP)
			nv = nvram_safe_get(strlcat_r(wan_prefix, "_pptp_server_ip", key, sizeof(key)));
		else if (proto == WP_L2TP)
			nv = nvram_safe_get(strlcat_r(wan_prefix, "_l2tp_server_ip", key, sizeof(key)));

		if (nv && *nv)
			fprintf(f, "rebind-domain-ok=%s\n", nv);

		dns = get_dns(wan_prefix); /* this always points to a static buffer */
		if (check_wanup(wan_prefix) == 0 && dns->count == 0)
			continue;

		/* dns list with non-standart ports */
		for (n = 0; n < dns->count; ++n) {
			if (dns->dns[n].port != 53)
				fprintf(f, "server=%s#%u\n", inet_ntoa(dns->dns[n].addr), dns->dns[n].port);
		}
	}
}

static void write_dhcp_ignore(FILE *f)
{
	unsigned int i;
	char buf[32];

	/* ignore DHCP requests from unknown devices for given LAN */
	for (i = 0; i < BRIDGE_COUNT; i++) {
		if (!bridge_has_l3(i))
			continue;

		snprintf(buf, sizeof(buf), (i == 0 ? "dhcpd_ostatic" : "dhcpd%u_ostatic"), i);
		if (nvram_get_int(buf))
			fprintf(f, "dhcp-ignore=tag:br%u,tag:!known\n", i);
	}
}

static void write_dhcp_ranges(FILE *f, int *do_dhcpd_hosts, int *do_dns_ptr, char *sdhcp_lease, const size_t buf_sz, const int mwan_num)
{
	int do_dhcpd, do_dns, br;
	int wan_unit, dhcp_lease, nval, m;
	const dns_list_t *dns;
	const char *router_ip, *lan_ip, *e, *wins, *gw, *start, *end;
	char *p, *gw2;
	char wan_prefix[] = "wanXX";
	char lanN_proto[] = "lanXX_proto";
	char lanN_ifname[] = "lanXX_ifname";
	char lanN_ipaddr[] = "lanXX_ipaddr";
	char lanN_netmask[] = "lanXX_netmask";
	char dhcpdN_startip[] = "dhcpdXX_startip";
	char dhcpdN_endip[] = "dhcpdXX_endip";
	char dhcpN_lease[] = "dhcpXX_lease";
	char lan_base[32], buf[512];
	unsigned int start_ip = 2, end_ip = 50;

	*do_dhcpd_hosts = 0;
	do_dns = *do_dns_ptr;

	for (br = 0; br < BRIDGE_COUNT; br++) {
		char bridge[2] = "0";
		if (br != 0)
			bridge[0] += br;
		else
			memset(bridge, 0, sizeof(bridge));

		snprintf(lanN_proto, sizeof(lanN_proto), "lan%s_proto", bridge);
		snprintf(lanN_ifname, sizeof(lanN_ifname), "lan%s_ifname", bridge);
		snprintf(lanN_ipaddr, sizeof(lanN_ipaddr), "lan%s_ipaddr", bridge);
		snprintf(lanN_netmask, sizeof(lanN_netmask), "lan%s_netmask", bridge);
		snprintf(dhcpdN_startip, sizeof(dhcpdN_startip), "dhcpd%s_startip", bridge);
		snprintf(dhcpdN_endip, sizeof(dhcpdN_endip), "dhcpd%s_endip", bridge);

		/* 0.0.0.0 marks a pure L2 bridge: do not bind DNS, DHCP or RA to it. */
		if (!bridge_has_l3(br) || !*nvram_safe_get(lanN_ifname))
			continue;

		do_dhcpd = nvram_match(lanN_proto, "dhcp");
		if (do_dhcpd) {
			(*do_dhcpd_hosts)++;

			router_ip = nvram_safe_get(lanN_ipaddr);
			strlcpy(lan_base, router_ip, sizeof(lan_base));
			if ((p = strrchr(lan_base, '.')) != NULL)
				*(p + 1) = 0;

			fprintf(f, "interface=%s\n", nvram_safe_get(lanN_ifname));

			/* DHCP lease time */
			snprintf(dhcpN_lease, sizeof(dhcpN_lease), "dhcp%s_lease", bridge);
			dhcp_lease = nvram_get_int(dhcpN_lease);
			if (dhcp_lease <= 0)
				dhcp_lease = 1440;

			e = nvram_get("dhcpd_slt");
			nval = (e && *e) ? atoi(e) : 0;
			if (nval < 0)
				strlcpy(sdhcp_lease, "infinite", buf_sz);
			else
				snprintf(sdhcp_lease, buf_sz, "%dm", (nval > 0 ? nval : dhcp_lease));

			if (!do_dns) { /* if not using dnsmasq for dns */
				for (wan_unit = 1; wan_unit <= mwan_num; ++wan_unit) {
					memset(wan_prefix, 0, sizeof(wan_prefix));
					get_wan_prefix(wan_unit, wan_prefix);

					/* skip inactive WAN connections
					 * TBD: need to check if there is no WANs active do we need skip here also?!?
					 */
					if (check_wanup(wan_prefix) == 0)
						continue;

					dns = get_dns(wan_prefix); /* static buffer */

					if (dns->count == 0 && nvram_get_int("dhcpd_llndns")) {
						/* no DNS might be temporary. use a low lease time to force clients to update. */
						dhcp_lease = 2;
						strlcpy(sdhcp_lease, "2m", sizeof(sdhcp_lease));
						do_dns = 1;
					}
					else {
						/* pass the dns directly */
						memset(buf, 0, sizeof(buf));
						for (m = 0; m < dns->count; ++m) {
							if (dns->dns[m].port == 53) /* check: option 6 doesn't seem to support other ports */
								snprintf(buf + strlen(buf), sizeof(buf) - strlen(buf), ",%s", inet_ntoa(dns->dns[m].addr));
						}
						fprintf(f, "dhcp-option=tag:%s,6%s\n", nvram_safe_get(lanN_ifname), buf);
					}
				}
			}

			start = nvram_safe_get(dhcpdN_startip);
			end   = nvram_safe_get(dhcpdN_endip);

			if (start && *start && end && *end)
				fprintf(f, "dhcp-range=tag:%s,%s,%s,%s,%dm\n", nvram_safe_get(lanN_ifname), start, end, nvram_safe_get(lanN_netmask), dhcp_lease);
			else
				/* defaults if not present in nvram */
				fprintf(f, "dhcp-range=tag:%s,%s%u,%s%u,%s,%dm\n", nvram_safe_get(lanN_ifname), lan_base, start_ip, lan_base, end_ip, nvram_safe_get(lanN_netmask), dhcp_lease);

			lan_ip = nvram_safe_get(lanN_ipaddr);
			gw = lan_ip;
			if ((nvram_get_int("dhcpd_gwmode") == 1) && (get_wan_proto() == WP_DISABLED)) {
				gw2 = nvram_safe_get("lan_gateway");
				if ((*gw2) && (strcmp(gw2, "0.0.0.0") != 0))
					gw = gw2;
			}
			fprintf(f, "dhcp-option=tag:%s,3,%s\n", nvram_safe_get(lanN_ifname), gw); /* gateway */

			wins = nvram_safe_get("wan_wins");
			if (*wins && strcmp(wins, "0.0.0.0") != 0)
				fprintf(f, "dhcp-option=tag:%s,44,%s\n", nvram_safe_get(lanN_ifname), wins); /* netbios-ns */
#ifdef TCONFIG_SAMBASRV
			else if ((nvram_get_int("smbd_enable") || (pidof("smbd") > 0)) && nvram_invmatch("lan_hostname", "") && nvram_get_int("smbd_wins")) {
				if ((!*wins) || (strcmp(wins, "0.0.0.0") == 0))
					/* Samba will serve as a WINS server */
					fprintf(f, "dhcp-option=tag:%s,44,%s\n", nvram_safe_get(lanN_ifname), nvram_safe_get(lanN_ipaddr)); /* netbios-ns */
			}
#endif /* TCONFIG_SAMBASRV */
		}
		else {
			/* No DHCP on this LAN */
			if (strcmp(nvram_safe_get(lanN_ifname), "") != 0)
				fprintf(f, "interface=%s\n", nvram_safe_get(lanN_ifname));
		}
	}

	*do_dns_ptr = do_dns;
}

static FILE *write_static_hosts(void)
{
	FILE *hf;
	unsigned int i;
	unsigned int mwan_num = mwan_active_num();
	char tmp[32];
	const char *router_ip, *hostname, *p;

	/* WAN0 remains available in DNS configuration for AP-only mode. */
	if (mwan_num == 0)
		mwan_num = 1;

	/* write static lease entries & create hosts file */
	router_ip = nvram_safe_get("lan_ipaddr");

	if ((hf = fopen(dmhosts, "w")) != NULL) {
		if ((hostname = nvram_safe_get("wan_hostname")) && (*hostname))
			fprintf(hf, "%s %s\n", router_ip, hostname);
#ifdef TCONFIG_SAMBASRV
		else if ((hostname = nvram_safe_get("lan_hostname")) && (*hostname)) /* FIXME: it has to be implemented (lan_hostname is always empty) */
			fprintf(hf, "%s %s\n", router_ip, hostname);
#endif
		for (i = 1; i <= mwan_num; i++) {
			snprintf(tmp, sizeof(tmp), (i == 1 ? "wan" : "wan%u"), i);
			p = get_wanip(tmp);
			if ((!*p) || (strcmp(p, "0.0.0.0") == 0))
				p = "127.0.0.1";

			fprintf(hf, "%s wan%u-ip\n", p, i);
		}
	}

	return hf;
}

#ifdef TCONFIG_DMZMAC
static int dmz_valid_mac(const char *mac, unsigned char ea[ETHER_ADDR_LEN])
{
	unsigned int i;
	int nonzero = 0;

	if (!ether_atoe(mac, ea))
		return 0;

	/* Only individual, non-zero Ethernet addresses make sense here. */
	if (ea[0] & 0x01)
		return 0;

	for (i = 0; i < ETHER_ADDR_LEN; i++) {
		if (ea[i]) {
			nonzero = 1;
			break;
		}
	}

	return nonzero;
}

/*
 * Validate a dhcpd_static MAC list the same way write_static_reservations()
 * does. Return 1 if target is present, 0 if it is not, and -1 if the list
 * itself is invalid and would not generate a dhcp-host entry.
 */
static int dmz_mac_list_state(const char *list, const unsigned char target[ETHER_ADDR_LEN])
{
	unsigned char ea[ETHER_ADDR_LEN];
	char copy[64], *save, *mac;
	int matched = 0;

	if (!list || !*list)
		return -1;

	strlcpy(copy, list, sizeof(copy));
	save = NULL;

	for (mac = strtok_r(copy, ",", &save); mac != NULL; mac = strtok_r(NULL, ",", &save)) {
		if (!ether_atoe(mac, ea))
			return -1;

		if (memcmp(ea, target, ETHER_ADDR_LEN) == 0)
			matched = 1;
	}

	return matched;
}

static int dmz_dhcp_enabled_for_ip(const char *ip, const struct in_addr *addr, char *ifname_out, const size_t ifname_len)
{
	struct in_addr lan, mask, network, broadcast;
	char ifname[IFNAMSIZ + 1];
	char num[4], ifkey[24], ipkey[24], maskkey[24], protokey[24];
	unsigned int i;

	if (!lan_ifname_for_ipv4(ip, ifname, sizeof(ifname)))
		return 0;

	for (i = 0; i < BRIDGE_COUNT; i++) {
		if (i == 0)
			num[0] = '\0';
		else
			snprintf(num, sizeof(num), "%u", i);

		snprintf(ifkey, sizeof(ifkey), "lan%s_ifname", num);
		if (strcmp(nvram_safe_get(ifkey), ifname) != 0)
			continue;

		snprintf(protokey, sizeof(protokey), "lan%s_proto", num);
		if (!nvram_match(protokey, "dhcp"))
			return 0;

		snprintf(ipkey, sizeof(ipkey), "lan%s_ipaddr", num);
		snprintf(maskkey, sizeof(maskkey), "lan%s_netmask", num);
		if ((inet_pton(AF_INET, nvram_safe_get(ipkey), &lan) != 1) ||
		    (inet_pton(AF_INET, nvram_safe_get(maskkey), &mask) != 1))
			return 0;

		network.s_addr = lan.s_addr & mask.s_addr;
		broadcast.s_addr = network.s_addr | ~mask.s_addr;

		if ((addr->s_addr == lan.s_addr) ||
		    (addr->s_addr == network.s_addr) ||
		    (addr->s_addr == broadcast.s_addr))
			return 0;

		if (ifname_out)
			strlcpy(ifname_out, ifname, ifname_len);

		return 1;
	}

	return 0;
}

static int dmz_static_reservation_state(const unsigned char target_mac[ETHER_ADDR_LEN], const struct in_addr *target_ip, const char *target_ifname)
{
	char *nve, *nvp, *p;
	const char *mac, *ip, *name, *bind, *ip6;
	struct in_addr in4;
	char ifname[IFNAMSIZ + 1];
	int mac_state, same_mac, state = 0;

	nve = nvp = strdup(nvram_safe_get("dhcpd_static"));
	while (nvp && (p = strsep(&nvp, ">")) != NULL) {
		mac = ip = name = bind = ip6 = NULL;

		if ((vstrsep(p, "<", &mac, &ip, &name, &bind, &ip6)) < 4)
			continue;

		mac_state = dmz_mac_list_state(mac, target_mac);
		if (mac_state < 0)
			continue;
		same_mac = (mac_state > 0);

		if (!ip || !*ip)
			continue;

		if (inet_pton(AF_INET, ip, &in4) != 1)
			continue;

		if (same_mac) {
			if (in4.s_addr != target_ip->s_addr) {
				/*
				 * dnsmasq allows multiple dhcp-host entries for the same
				 * MAC when their addresses belong to different subnets.
				 */
				if (lan_ifname_for_ipv4(ip, ifname, sizeof(ifname)) && strcmp(ifname, target_ifname) != 0)
					continue;

				state = -1;
				break;
			}
			state = 1;
		}
		else if (in4.s_addr == target_ip->s_addr) {
			state = -1;
			break;
		}
	}

	if (nve)
		free(nve);

	return state;
}

static void write_dmz_reservation(FILE *f, const char *sdhcp_lease)
{
	const char *mac, *ip;
	struct in_addr in4;
	unsigned char ea[ETHER_ADDR_LEN];
	char canonical_mac[18], target_ifname[IFNAMSIZ + 1];
	int state;

	if (!nvram_get_int("dmz_enable"))
		return;

	mac = nvram_safe_get("dmz_macaddr");
	if (!*mac)
		return;

	ip = nvram_safe_get("dmz_ipaddr");

	if (!dmz_valid_mac(mac, ea)) {
		logmsg(LOG_WARNING, "DMZ: invalid MAC address '%s'; DHCP reservation not added", mac);
		return;
	}

	if ((inet_pton(AF_INET, ip, &in4) != 1) ||
	    (in4.s_addr == INADDR_ANY) ||
	    (in4.s_addr == INADDR_LOOPBACK) ||
	    (in4.s_addr == INADDR_BROADCAST)) {
		logmsg(LOG_WARNING, "DMZ: invalid IPv4 address '%s'; DHCP reservation not added", ip);
		return;
	}

	if (!dmz_dhcp_enabled_for_ip(ip, &in4, target_ifname, sizeof(target_ifname))) {
		logmsg(LOG_WARNING, "DMZ: IPv4 address '%s' is not on a DHCP-enabled LAN; DHCP reservation not added", ip);
		return;
	}

	state = dmz_static_reservation_state(ea, &in4, target_ifname);
	if (state < 0) {
		logmsg(LOG_WARNING, "DMZ: MAC/IP conflicts with Static DHCP; DHCP reservation not added");
		return;
	}
	if (state > 0)
		return;

	ether_etoa(ea, canonical_mac);
	fprintf(f, "dhcp-host=%s,%s", canonical_mac, ip);

	if (nvram_get_int("dhcpd_slt") != 0)
		fprintf(f, ",%s", sdhcp_lease);

	fprintf(f, "\n");
}
#endif /* TCONFIG_DMZMAC */


static void write_static_reservations(FILE *f, FILE *hf, int do_dhcpd_hosts, const char *sdhcp_lease)
{
	char *nve, *nvp, *p;
	const char *mac, *ip, *name, *bind, *ip6;
	struct in_addr in4;
#ifdef TCONFIG_IPV6
	struct in6_addr in6;
#endif
	unsigned char ea[ETHER_ADDR_LEN];
	char mac_copy[64];
	char *m_save, *m_tok;
	int macs_ok;

	/* add dhcp reservations
	 *
	 * FORMAT (static ARP binding after hostname; [ ] denotes an optional IPv6 reservation):
	 * 00:aa:bb:cc:dd:ee<123.123.123.123<xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx.xyz<a[<::50]>
	 * 00:aa:bb:cc:dd:ee,00:aa:bb:cc:dd:ee<123.123.123.123<xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx.xyz<a>
	 */

	nve = nvp = strdup(nvram_safe_get("dhcpd_static"));
	while (nvp && (p = strsep(&nvp, ">")) != NULL) {
		mac = ip = name = bind = ip6 = NULL;

		/* minimum 4 fields required (5th, IPv6, is optional) */
		if ((vstrsep(p, "<", &mac, &ip, &name, &bind, &ip6)) < 4)
			continue;

		/* validate IP */
		if (*ip && ((inet_pton(AF_INET, ip, &in4) <= 0) || (in4.s_addr == INADDR_ANY) || 
		    (in4.s_addr == INADDR_LOOPBACK) || (in4.s_addr == INADDR_BROADCAST))) /* invalid IP (if any) */
			continue;

#ifdef TCONFIG_IPV6
		/* validate IPv6 */
		if (ip6 && *ip6 && (inet_pton(AF_INET6, ip6, &in6) <= 0))
			ip6 = NULL;
#else
		ip6 = NULL; /* no IPv6 support in this build */
#endif

		/* add to hosts file (gives the reservation forward A/AAAA and reverse PTR records) */
		if (hf && *name) {
			if (*ip)
				fprintf(hf, "%s %s\n", ip, name);
#ifdef TCONFIG_IPV6
			/* skip a suffix-only reservation (e.g. ::50) in the hosts file - dnsmasq fills in the prefix at lease time */
			if (ip6 && *ip6 && (in6.s6_addr32[0] || in6.s6_addr32[1]))
				fprintf(hf, "%s %s\n", ip6, name);
#endif
		}

		/* validate MAC(s) - dhcpd_static may carry a single MAC or a pair joined by ',' for one reservation */
		macs_ok = 0;
		if (mac && *mac) {
			m_save = NULL;
			strlcpy(mac_copy, mac, sizeof(mac_copy));
			macs_ok = 1;
			for (m_tok = strtok_r(mac_copy, ",", &m_save);
			     m_tok != NULL;
			     m_tok = strtok_r(NULL, ",", &m_save)) {
				if (!ether_atoe(m_tok, ea)) {
					macs_ok = 0;
					break;
				}
			}
		}

		/* add to dnsmasq conf */
		if (do_dhcpd_hosts > 0 && macs_ok && (*ip || (ip6 && *ip6) || *name)) {
			int have_addr = 0;

			fprintf(f, "dhcp-host=%s", mac);
			if (*ip) {
				fprintf(f, ",%s", ip);
				have_addr = 1;
			}
			if (ip6 && *ip6) {
				fprintf(f, ",[%s]", ip6);
				have_addr = 1;
			}
			if (!have_addr && *name)
				fprintf(f, ",%s", name);

			if (nvram_get_int("dhcpd_slt") != 0)
				fprintf(f, ",%s", sdhcp_lease);

			fprintf(f, "\n");
		}
	}

	if (nve)
		free(nve);
}

static void write_ipv6_config(FILE *f)
{
#ifdef TCONFIG_IPV6
	if (ipv6_enabled() && (nvram_get_int("ipv6_radvd") || nvram_get_int("ipv6_dhcpd"))) {
		unsigned int bridge;
		char key[32], mtu_str[16] = { 0 };
		const char *ifname;
		int have_l3_bridge = 0;
		int ipv6_lease = 0; /* DHCP IPv6 lease time */
		int service = get_ipv6_service();
		char lease_unit = 'h';

		for (bridge = 0; bridge < BRIDGE_COUNT; bridge++) {
			ifname = bridge_nvram_get(bridge, "ifname", key, sizeof(key));
			if (bridge_has_l3(bridge) && (*ifname) && (strncmp(ifname, "br", 2) == 0)) {
				have_l3_bridge = 1;
				break;
			}
		}

		if (!have_l3_bridge)
			return;

		/* get mtu for IPv6 --> only for "wan" (no multiwan support) */
		switch (service) {
			case IPV6_ANYCAST_6TO4: /* use tun mtu (visible at basic-ipv6.asp) */
			case IPV6_6IN4:
				snprintf(mtu_str, sizeof(mtu_str), "%d", (nvram_get_int("ipv6_tun_mtu") > 0) ? nvram_get_int("ipv6_tun_mtu") : 1280);
			break;
			case IPV6_6RD: /* use wan mtu and calculate it */
			case IPV6_6RD_DHCP:
				snprintf(mtu_str, sizeof(mtu_str), "%d", (nvram_get_int("wan_mtu") > 0) ? (nvram_get_int("wan_mtu") - 20) : 1280);
			break;
			default:
				snprintf(mtu_str, sizeof(mtu_str), "%d", (nvram_get_int("wan_mtu") > 0) ? nvram_get_int("wan_mtu") : 1280);
			break;
		}

		/* check for DHCPv6 PD (and use IPv6 preferred lifetime in that case) */
		if (service == IPV6_NATIVE_DHCP) {
			ipv6_lease = nvram_get_int("ipv6_pd_pltime"); /* get IPv6 preferred lifetime (seconds) */
			if ((ipv6_lease < IPV6_MIN_LIFETIME) || (ipv6_lease > ONEMONTH_LIFETIME)) /* check lease time and limit the range (120 sec up to one month) */
				ipv6_lease = IPV6_MIN_LIFETIME;
			lease_unit = 's';
		}
		else {
			ipv6_lease = nvram_get_int("ipv6_lease_time"); /* get DHCP IPv6 lease time via GUI */
			if ((ipv6_lease < 1) || (ipv6_lease > 720)) /* check lease time and limit the range (1...720 hours, 30 days should be enough) */
				ipv6_lease = 12;
		}

		/* Router Advertisements and DHCPv6 are configured only on L3 bridges. */
		fprintf(f, "enable-ra\n");
		for (bridge = 0; bridge < BRIDGE_COUNT; bridge++) {
			ifname = bridge_nvram_get(bridge, "ifname", key, sizeof(key));
			if (!bridge_has_l3(bridge) || !*ifname || (strncmp(ifname, "br", 2) != 0))
				continue;

			if (nvram_get_int("ipv6_fast_ra"))
				fprintf(f, "ra-param=%s, mtu:%s, 15, 600\n", ifname, mtu_str); /* ra-interval = 15 sec, router-lifetime = 600 sec (10 min) */
			else
				fprintf(f, "ra-param=%s, mtu:%s, 60, 1200\n", ifname, mtu_str); /* ra-interval = 60 sec, router-lifetime = 1200 sec (20 min) */

			/* only SLAAC and NO DHCPv6 */
			if (nvram_get_int("ipv6_radvd") && !nvram_get_int("ipv6_dhcpd"))
				fprintf(f, "dhcp-range=::, constructor:%s, ra-names, ra-stateless, 64, %d%c\n", ifname, ipv6_lease, lease_unit);

			/* only DHCPv6 and NO SLAAC */
			if (nvram_get_int("ipv6_dhcpd") && !nvram_get_int("ipv6_radvd"))
				fprintf(f, "dhcp-range=::2, ::FFFF:FFFF, constructor:%s, 64, %d%c\n", ifname, ipv6_lease, lease_unit);

			/* SLAAC and DHCPv6 (2 IPv6 IPs) */
			if (nvram_get_int("ipv6_radvd") && nvram_get_int("ipv6_dhcpd"))
				fprintf(f, "dhcp-range=::2, ::FFFF:FFFF, constructor:%s, ra-names, 64, %d%c\n", ifname, ipv6_lease, lease_unit);
		}

		/* DHCPv6 DNS servers */
		{
			char dns6[MAX_DNS6_SERVER_LAN][INET6_ADDRSTRLEN] = {{ 0 }, { 0 }};
			char word[INET6_ADDRSTRLEN], *next = NULL;
			struct in6_addr addr;
			int cntdns = 0;

			/* first check DNS servers (DNS1 + DNS2) via GUI (advanced-dhcpdns.asp)
			 * and verify that this is a valid IPv6 address
			 */
			foreach(word, nvram_safe_get("ipv6_dns_lan"), next) {
				if ((cntdns < MAX_DNS6_SERVER_LAN) && (inet_pton(AF_INET6, word, &addr) == 1)) {
					strlcpy(dns6[cntdns], ipv6_address(word), INET6_ADDRSTRLEN);
					cntdns++;
				}
			}
			if (cntdns == 2)
				fprintf(f, "dhcp-option=option6:dns-server,[%s],[%s]\n", dns6[0], dns6[1]); /* take FT user DNS1 + DNS2 address */
			else if (cntdns == 1)
				fprintf(f, "dhcp-option=option6:dns-server,[%s]\n", dns6[0]); /* take FT user DNS1 address */
			/* default - No DNS server via GUI (advanced-dhcpdns.asp) */
			else
				fprintf(f, "dhcp-option=option6:dns-server,%s\n", "[::]"); /* use global address */
		}

		/* DHCPv6 SNTP & NTP servers (disabled: :: means none) */
		if (nvram_get_int("ntpd_enable")) {
			fprintf(f, "dhcp-option=option6:31,%s\n", "[::]");
			fprintf(f, "dhcp-option=option6:56,%s\n", "[::]");
		}
	}
#endif /* TCONFIG_IPV6 */
}

static void write_tftp_config(FILE *f)
{
#ifdef TCONFIG_USB_EXTRAS
	unsigned int i;
	char lan_ifname[16], lan_ip[16], key[32];

	if (nvram_get_int("dnsmasq_tftp")) {
		fprintf(f, "enable-tftp\n"
		           "tftp-no-fail\n"
		           "tftp-root=%s\n",
		           nvram_safe_get("dnsmasq_tftp_path"));

		for (i = 0; i < BRIDGE_COUNT; i++) {
			snprintf(key, sizeof(key), (i == 0 ? "dnsmasq_pxelan" : "dnsmasq_pxelan%u"), i);
			snprintf(lan_ifname, sizeof(lan_ifname), (i == 0 ? "lan_ifname" : "lan%u_ifname"), i);

			if (bridge_has_l3(i) && nvram_get_int(key) && strlen(nvram_safe_get(lan_ifname)) > 0) {
				snprintf(lan_ip, sizeof(lan_ip), (i == 0 ? "lan_ipaddr" : "lan%u_ipaddr"), i);
				fprintf(f, "dhcp-boot=pxelinux.0,,%s\n", nvram_safe_get(lan_ip));
			}
		}
	}
#endif /* TCONFIG_USB_EXTRAS */
}

static void write_custom_and_ipset(FILE *f)
{
	if (!nvram_get_int("dnsmasq_safe")) {
		fprintf(f, "%s\n", nvram_safe_get("dnsmasq_custom"));
		fappend(f, dmcfgcustom); /* append custom config */
	}
	else
		logmsg(LOG_WARNING, dmwarning);

	fappend(f, dmipset); /* append ipset file */
}

static void write_debug_and_adblock(FILE *f)
{
	if (nvram_get_int("dnsmasq_debug"))
		fprintf(f, "log-queries\n");

	if ((nvram_get_int("adblock_enable")) && (f_exists(dmadblock)))
		fprintf(f, "conf-file=%s\n", dmadblock);
}

static void start_dnsmasq_wet(void)
{
	FILE *f;
	const char *nv;
	char br;
	char lanN_ifname[] = "lanXX_ifname";

	if ((f = fopen(dmcfg, "w")) == NULL) {
		logerr(__FUNCTION__, __LINE__, dmcfg);
		return;
	}

	fprintf(f, "pid-file=%s\n"
	           "resolv-file=%s\n"		/* the real stuff is here */
	           "min-port=%u\n"		/* min port used for random src port */
	           "no-negcache\n"		/* disable negative caching */
	           "bind-dynamic\n",
	           dmpid,
	           dmresolv,
	           4096);

	for (br = 0; br < BRIDGE_COUNT; br++) {
		char bridge[2] = "0";
		if (br != 0)
			bridge[0] += br;
		else
			memset(bridge, 0, sizeof(bridge));

		snprintf(lanN_ifname, sizeof(lanN_ifname), "lan%s_ifname", bridge);
		nv = nvram_safe_get(lanN_ifname);

		if (strncmp(nv, "br", 2) == 0) {
			if (bridge_has_l3(br)) {
				fprintf(f, "interface=%s\n", nv);
				fprintf(f, "no-dhcp-interface=%s\n", nv);
			}
			else
				fprintf(f, "except-interface=%s\n", nv);
		}
	}

	write_debug_and_adblock(f);
	write_custom_and_ipset(f);

	fclose(f);

	unlink(resolvcfg);
	symlink(resolvcfgrom, resolvcfg); /* nameserver 127.0.0.1 */

	eval("dnsmasq", "-c", "4096", "--log-async");
}

void start_dnsmasq(void) {
	FILE *f;
	FILE *hf = NULL;
	int mwan_num, do_dhcpd_hosts = 0;
	int do_dns = nvram_get_int("dhcpd_dmdns");
	char sdhcp_lease[32];

	if (serialize_restart("dnsmasq", 1))
		return;

	if (check_bridge_modes())
		return;

	if ((f = fopen(dmcfg, "w")) == NULL) {
		logerr(__FUNCTION__, __LINE__, dmcfg);
		return;
	}

	mwan_num = mwan_active_num();
	/* WAN0 stores static DNS settings even when WAN is disabled. */
	if (mwan_num == 0)
		mwan_num = 1;

	write_basic_config(f);
	write_l2_bridge_exclusions(f);
	write_tor_dns(f);
	write_wan_dns(f, mwan_num);
	write_dhcp_ignore(f);
	write_dhcp_ranges(f, &do_dhcpd_hosts, &do_dns, sdhcp_lease, sizeof(sdhcp_lease), mwan_num);

	hf = write_static_hosts();
	write_static_reservations(f, hf, do_dhcpd_hosts, sdhcp_lease);
#ifdef TCONFIG_DMZMAC
	write_dmz_reservation(f, sdhcp_lease);
#endif /* TCONFIG_DMZMAC */

	if (hf) {
		/* add directory with additional hosts files */
		fprintf(f, "addn-hosts=%s\n", dmhosts);
		fclose(hf);
	}

#ifdef TCONFIG_PPTPD
	write_pptpd_dnsmasq_config(f);
#endif
#ifdef TCONFIG_OPENVPN
	write_ovpn_dnsmasq_config(f);
#endif
#ifdef TCONFIG_WIREGUARD
	write_wg_dnsmasq_config(f);
#endif

	write_ipv6_config(f);
	write_tftp_config(f);
	write_debug_and_adblock(f);
	write_custom_and_ipset(f);

	fclose(f);

	if (do_dns) {
		unlink(resolvcfg);
		symlink(resolvcfgrom, resolvcfg);
	}

#ifdef TCONFIG_DNSCRYPT
	stop_dnscrypt();
	start_dnscrypt();
#endif
#ifdef TCONFIG_STUBBY
	stop_stubby();
	start_stubby();
#endif

	/* default to some values we like, but allow the user to override them */
	eval("dnsmasq", "-c", "4096", "--log-async");

	if (!nvram_contains_word("debug_norestart", "dnsmasq"))
		pid_dnsmasq = -2;
}

void stop_dnsmasq(void)
{
	if (serialize_restart("dnsmasq", 0))
		return;

	pid_dnsmasq = -1;

	unlink(resolvcfg);
	symlink(dmresolv, resolvcfg);

	killall_tk_period_wait("dnsmasq", 50);

#ifdef TCONFIG_DNSCRYPT
	stop_dnscrypt();
#endif
#ifdef TCONFIG_STUBBY
	stop_stubby();
#endif
}

void reload_dnsmasq(void)
{
	/* notify dnsmasq */
	logmsg(LOG_INFO, "reloading dnsmasq");
	killall("dnsmasq", SIGINT);
}

void clear_resolv(void)
{
	logmsg(LOG_DEBUG, "*** %s: clear all DNS entries", __FUNCTION__);
	f_write(dmresolv, NULL, 0, 0, 0); /* blank */
}
