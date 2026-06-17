/*
 *
 * Tomato Firmware
 * Copyright (C) 2006-2009 Jonathan Zarate
 *
 * Fixes/updates (C) 2018 - 2026 pedro
 * https://freshtomato.org/
 *
 */


#include "rc.h"


#ifdef TCONFIG_IPV6
/* find the LAN bridge interface that owns this IPv4 (subnet match) */
static int lan_ifname_for_ipv4(const char *ip, char *ifname, size_t len)
{
	struct in_addr a, lan, mask;
	const char *lanip, *lanmask, *lanif;
	char ipkey[24], mkey[24], ifkey[24], num[4];
	unsigned int i;

	if (inet_pton(AF_INET, ip, &a) <= 0)
		return 0;

	for (i = 0; i < BRIDGE_COUNT; i++) {
		if (i == 0)
			num[0] = '\0';
		else
			snprintf(num, sizeof(num), "%u", i);

		snprintf(ipkey, sizeof(ipkey), "lan%s_ipaddr", num);
		snprintf(mkey, sizeof(mkey), "lan%s_netmask", num);
		snprintf(ifkey, sizeof(ifkey), "lan%s_ifname", num);

		lanif = nvram_safe_get(ifkey);
		lanip = nvram_safe_get(ipkey);
		lanmask = nvram_safe_get(mkey);

		if (!*lanif || !*lanip || !*lanmask)
			continue;
		if ((inet_pton(AF_INET, lanip, &lan) <= 0) || (inet_pton(AF_INET, lanmask, &mask) <= 0))
			continue;

		if ((a.s_addr & mask.s_addr) == (lan.s_addr & mask.s_addr)) {
			strlcpy(ifname, lanif, len);
			return 1;
		}
	}

	return 0;
}

/* build the address to bind: a full address passes through, a ::suffix is
 * combined with the bridge's current global /64 prefix. returns 1 on success. */
static int ipv6_neigh_addr(const char *ifname, const char *in, char *out, size_t len)
{
	struct in6_addr want, pfx;
	const char *ga;
	int i;

	if (inet_pton(AF_INET6, in, &want) <= 0)
		return 0;

	/* full address (any of the upper 64 bits set): use as-is */
	for (i = 0; i < 8; i++) {
		if (want.s6_addr[i])
			return (inet_ntop(AF_INET6, &want, out, len) != NULL);
	}

	/* ::suffix - need the bridge's live global prefix to complete it */
	ga = getifaddr((char *)ifname, AF_INET6, 0);
	if (!ga || !*ga || (inet_pton(AF_INET6, ga, &pfx) <= 0))
		return 0;

	memcpy(want.s6_addr, pfx.s6_addr, 8); /* take the upper 64 bits from the prefix */

	return (inet_ntop(AF_INET6, &want, out, len) != NULL);
}

/* clear the static (permanent) IPv6 neighbor entries from the LAN bridges */
static void stop_ndpbind(void)
{
	const char *lanif;
	char ifkey[24], num[4];
	unsigned int i;

	for (i = 0; i < BRIDGE_COUNT; i++) {
		if (i == 0)
			num[0] = '\0';
		else
			snprintf(num, sizeof(num), "%u", i);

		snprintf(ifkey, sizeof(ifkey), "lan%s_ifname", num);
		lanif = nvram_safe_get(ifkey);
		if (*lanif)
			eval("ip", "-6", "neigh", "flush", "dev", (char *)lanif, "nud", "permanent");
	}
}
#endif /* TCONFIG_IPV6 */

void start_arpbind(void)
{
	char *nvp, *nv, *b;
	const char *ipaddr, *macaddr;
	const char *name, *bind, *ip6;
#ifdef TCONFIG_IPV6
	char ifname[IFNAMSIZ];
	char addr6[INET6_ADDRSTRLEN];
#endif

	nvp = nv = strdup(nvram_safe_get("dhcpd_static"));
	if (!nv)
		return;

	/* clear arp/neighbor tables first */
	stop_arpbind();

	while ((b = strsep(&nvp, ">")) != NULL) {
		/*
		 * macaddr<ip.ad.dr.ess<hostname<arpbind[<ipv6]>anotherhwaddr<other.ip.addr.ess<othername<arpbind
		*/
		ipaddr = macaddr = name = bind = ip6 = NULL;

		if ((vstrsep(b, "<", &macaddr, &ipaddr, &name, &bind, &ip6)) < 4)
			continue;
		if (strchr(macaddr, ',') != NULL)
			continue;
		if (strcmp(bind, "1") != 0)
			continue;

		/* IPv4 static ARP (arp auto-detects the interface from the route) */
		if (*ipaddr)
			eval("arp", "-s", (char *)ipaddr, (char *)macaddr);

#ifdef TCONFIG_IPV6
		/* IPv6 static neighbor (NDP) - ip neigh requires an explicit interface */
		if (ip6 && *ip6) {
			/* derive the bridge from the IPv4, otherwise fall back to the main LAN */
			if (!(*ipaddr) || !lan_ifname_for_ipv4(ipaddr, ifname, sizeof(ifname)))
				strlcpy(ifname, nvram_safe_get("lan_ifname"), sizeof(ifname));

			if (*ifname && ipv6_neigh_addr(ifname, ip6, addr6, sizeof(addr6)))
				eval("ip", "-6", "neigh", "replace", addr6, "lladdr", (char *)macaddr, "dev", ifname, "nud", "permanent");
		}
#endif
	}

	free(nv);
}

void stop_arpbind(void)
{
	FILE *f;
	char buf[512];
	char ipaddr[48] = "";

	if ((f = fopen("/proc/net/arp", "r")) != NULL) {
		while (fgets(buf, sizeof(buf), f)) {
			if (sscanf(buf, "%s %*s %*s %*s %*s %*s", ipaddr) != 1)
				continue;

			if (strcmp(ipaddr, "IP") == 0)
				continue;

			eval ("arp", "-d", (char *)ipaddr);
		}
		fclose(f);
	}

#ifdef TCONFIG_IPV6
	stop_ndpbind();
#endif
}
