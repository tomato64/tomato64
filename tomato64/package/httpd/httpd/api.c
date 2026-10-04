/*
 * api.c
 *
 * Versioned JSON API. Self-contained on purpose: everything the endpoints
 * need is read here rather than borrowed from the asp_* layer, which emits
 * JavaScript object literals rather than JSON and would tie this file to that
 * output format. The only hooks into the rest of httpd are one row in
 * mime_handlers[] and the include that declares it.
 *
 * That row uses wi_generic_noid, not wi_generic. wi_generic calls check_id(),
 * which exit(1)s when the request carries no _http_id matching nvram - the
 * GUI's CSRF token. An API client has no way to obtain one, and the failure
 * is silent: the worker exits before stdio flushes, so the caller sees an
 * empty reply rather than an error.
 *
 * The endpoints are read-only, so dropping that check costs nothing today.
 * When mutating endpoints arrive they must authenticate by bearer token
 * rather than HTTP Basic, which removes the CSRF exposure at the root: a
 * browser attaches Basic credentials to a cross-site request by itself, and
 * never attaches a bearer token.
 */

#include "tomato.h"
#include "api.h"
#include "wlhelper.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <ctype.h>
#include <net/if.h>
#include <sys/sysinfo.h>
#include <sys/statvfs.h>

const char mime_json[] = "application/json";

#define API_VERSION	1

/* ------------------------------------------------------------- output -- */

static int comma;

static void j_open(const char *name)
{
	if (name)
		web_printf("%s\"%s\":{", comma ? "," : "", name);
	else
		web_puts("{");

	comma = 0;
}

static void j_close(void)
{
	web_puts("}");
	comma = 1;
}

static void j_sep(void)
{
	if (comma)
		web_puts(",");

	comma = 1;
}

/* JSON strings admit no control characters, and a value taken from nvram or
 * from /proc can hold anything at all. */
static void j_str(const char *name, const char *value)
{
	const unsigned char *p = (const unsigned char *)(value ? value : "");

	j_sep();
	web_printf("\"%s\":\"", name);

	for (; *p; p++) {
		switch (*p) {
		case '"':	web_puts("\\\"");	break;
		case '\\':	web_puts("\\\\");	break;
		case '\n':	web_puts("\\n");	break;
		case '\r':	web_puts("\\r");	break;
		case '\t':	web_puts("\\t");	break;
		default:
			if (*p < 0x20)
				web_printf("\\u%04x", *p);
			else
				web_printf("%c", *p);
			break;
		}
	}

	web_puts("\"");
}

static void j_nv(const char *name, const char *nvname)
{
	j_str(name, nvram_safe_get(nvname));
}

static void j_num(const char *name, long long value)
{
	j_sep();
	web_printf("\"%s\":%lld", name, value);
}

static void j_real(const char *name, double value)
{
	j_sep();
	web_printf("\"%s\":%.2f", name, value);
}

static void j_bool(const char *name, int value)
{
	j_sep();
	web_printf("\"%s\":%s", name, value ? "true" : "false");
}

static void j_null(const char *name)
{
	j_sep();
	web_printf("\"%s\":null", name);
}

/* A space-separated nvram list, as a JSON array of strings. */
static void j_list(const char *name, const char *value)
{
	char buf[512];
	char *tok, *save = NULL;

	j_sep();
	web_printf("\"%s\":[", name);
	comma = 0;

	strlcpy(buf, value ? value : "", sizeof(buf));

	for (tok = strtok_r(buf, " ", &save); tok; tok = strtok_r(NULL, " ", &save)) {
		j_sep();
		web_puts("\"");
		/* list members are interface names and addresses: nothing to escape */
		web_puts(tok);
		web_puts("\"");
	}

	web_puts("]");
	comma = 1;
}

static void j_arr(const char *name)
{
	j_sep();
	web_printf("\"%s\":[", name);
	comma = 0;
}

static void j_arr_end(void)
{
	web_puts("]");
	comma = 1;
}

static void j_elem(void)
{
	if (comma)
		web_puts(",");

	web_puts("{");
	comma = 0;
}

static void j_elem_end(void)
{
	web_puts("}");
	comma = 1;
}

/* -------------------------------------------------------------- probes -- */

/* /proc/meminfo, in kB. Returns 0 when a field was not found. */
static unsigned long meminfo_field(const char *field)
{
	char line[128];
	unsigned long value = 0;
	size_t len = strlen(field);
	FILE *f;

	if ((f = fopen("/proc/meminfo", "r")) == NULL)
		return 0;

	while (fgets(line, sizeof(line), f)) {
		if (strncmp(line, field, len) == 0 && line[len] == ':') {
			value = strtoul(line + len + 1, NULL, 10);
			break;
		}
	}

	fclose(f);

	return value;
}

/*
 * Milli-degrees C from the first thermal zone, or -1 where there is none.
 * Deliberately not get_cpuinfo(): that returns a formatted string and its
 * cputemp argument only exists on some platforms.
 */
static long cpu_temp_mc(void)
{
	char buf[32];
	long mc = -1;
	FILE *f;

	if ((f = fopen("/sys/class/thermal/thermal_zone0/temp", "r")) == NULL)
		return -1;

	if (fgets(buf, sizeof(buf), f))
		mc = strtol(buf, NULL, 10);

	fclose(f);

	return mc;
}

/* ----------------------------------------------------------- endpoints -- */

static void api_router(void)
{
	struct sysinfo si;
	char model[64];

	model[0] = '\0';
	sysinfo(&si);
	get_cpumodel(model, sizeof(model));

	j_open("router");
	j_nv("name", "router_name");
	j_nv("model", "t_model_name");
	j_str("cpu", model);
	j_str("firmware", tomato_version);
	j_str("build", tomato_buildtime);
	j_num("uptime", (long long)si.uptime);
	j_close();
}

static void api_system(void)
{
	struct sysinfo si;
	struct statvfs vfs;
	unsigned long total, avail;
	long mc;

	sysinfo(&si);

	total = meminfo_field("MemTotal");
	avail = meminfo_field("MemAvailable");

	j_open("system");

	/* si.loads is in fixed point, 1<<SI_LOAD_SHIFT == 1.0 */
	j_real("load1",  si.loads[0] / (double)(1 << SI_LOAD_SHIFT));
	j_real("load5",  si.loads[1] / (double)(1 << SI_LOAD_SHIFT));
	j_real("load15", si.loads[2] / (double)(1 << SI_LOAD_SHIFT));

	j_num("mem_total_kb", (long long)total);
	j_num("mem_avail_kb", (long long)avail);
	j_num("mem_used_kb",  (long long)((total > avail) ? total - avail : 0));

	if ((mc = cpu_temp_mc()) >= 0)
		j_real("cpu_temp_c", mc / 1000.0);
	else
		j_null("cpu_temp_c");

	if (statvfs("/", &vfs) == 0 && vfs.f_blocks > 0) {
		j_num("flash_total_kb", (long long)vfs.f_blocks * vfs.f_frsize / 1024);
		j_num("flash_used_kb", (long long)(vfs.f_blocks - vfs.f_bfree) * vfs.f_frsize / 1024);
	}

	j_close();
}

static int api_netdev_sysfs(const char *device, const char *attr, char *out, size_t len)
{
	char path[128];
	FILE *f;
	size_t n;

	snprintf(path, sizeof(path), "/sys/class/net/%s/%s", device, attr);

	if ((f = fopen(path, "r")) == NULL)
		return 0;

	if (fgets(out, len, f) == NULL) {
		fclose(f);
		return 0;
	}

	fclose(f);

	n = strlen(out);

	while (n > 0 && (out[n - 1] == '\n' || out[n - 1] == '\r' || out[n - 1] == ' '))
		out[--n] = '\0';

	return (n > 0);
}

/*
 * Every LAN bridge, not only the first. Tomato64 carries up to BRIDGE_COUNT of
 * them - br0 under the plain lan_ keys, the rest under lan1_ and up - and the
 * web interface lists them all, so a panel that showed one was showing a
 * fraction of the router.
 *
 * "up" is administrative, not carrier: a bridge with no member port yet reads
 * "unknown" in sysfs, but it holds an address and the router answers on it,
 * which is what someone reading the list wants to know.
 */
static void api_lan(void)
{
	char key[32], state[32];
	const char *ifname;
	unsigned int i;

	j_arr("lan");

	for (i = 0; i < BRIDGE_COUNT; i++) {
		get_bridge_nvram_key(i, "ifname", key, sizeof(key));
		ifname = nvram_safe_get(key);

		if (*ifname == '\0')
			continue;

		j_elem();
		j_num("index", i);
		j_str("ifname", ifname);

		get_bridge_nvram_key(i, "ipaddr", key, sizeof(key));
		j_nv("ipaddr", key);
		get_bridge_nvram_key(i, "netmask", key, sizeof(key));
		j_nv("netmask", key);
		get_bridge_nvram_key(i, "ifnames", key, sizeof(key));
		j_list("ports", nvram_safe_get(key));

		j_bool("up", api_netdev_sysfs(ifname, "operstate", state, sizeof(state)) &&
		             strcmp(state, "down") != 0);
		j_elem_end();
	}

	j_arr_end();
}

/*
 * Only the primary WAN for now. Multi-WAN walks wan, wan2, wan3, wan4 with
 * the same prefix argument that check_wanup() already takes, so adding it is
 * a loop rather than a rework.
 */
static void api_wan(void)
{
	int up = check_wanup("wan");

	j_open("wan");
	j_bool("up", up);
	j_str("iface", get_wanface("wan"));
	j_nv("proto", "wan_proto");
	j_nv("ipaddr", "wan_ipaddr");
	j_nv("netmask", "wan_netmask");
	j_nv("gateway", "wan_gateway");
	j_list("dns", *nvram_safe_get("wan_dns") ? nvram_safe_get("wan_dns")
						  : nvram_safe_get("wan_get_dns"));
	j_num("uptime", up ? (long long)check_wanup_time("wan") : 0);
	j_close();
}

/*
 * Live clients, from the ARP table with names resolved by reverse DNS.
 *
 * Names deliberately do NOT come from /var/tmp/dhcp/leases. Tomato's dnsmasq
 * is patched so that file exists only transiently: devlist.c creates
 * leases.!, sends SIGUSR2, dnsmasq dumps the table and renames it into place,
 * and it is deleted once the reader is done. There is nothing to read between
 * those dances, which is why a naive read returns no names at all.
 *
 * resolve_addr() asks the local resolver instead, which is dnsmasq itself
 * answering PTR for the leases it holds - the same thing devlist.c does for
 * its arplist, live and with no signalling.
 *
 * ARP is what decides who is actually present.
 */

/*
 * True when an ARP entry's device is one of our LAN bridges.
 *
 * Without this the list carries the WAN side too - every host the router has
 * ARPed for upstream, including the ISP gateway - which is not what anything
 * asking for "clients" means.
 */
static int is_lan_iface(const char *dev)
{
	char key[32];
	const char *ifname;
	unsigned int i;

	for (i = 0; i < BRIDGE_COUNT; i++) {
		get_bridge_nvram_key(i, "ifname", key, sizeof(key));
		ifname = nvram_safe_get(key);

		if (*ifname && strcmp(ifname, dev) == 0)
			return 1;
	}

	return 0;
}

static void api_clients(void)
{
	char line[256], ip[24], mac[24], dev[24], host[128];
	unsigned int flags;
	int count = 0;
	char *dot;
	FILE *f;

	j_open("clients");
	j_arr("list");

	if ((f = fopen("/proc/net/arp", "r")) != NULL) {
		while (fgets(line, sizeof(line), f)) {
			if (sscanf(line, "%23s %*s 0x%X %17s %*s %23s", ip, &flags, mac, dev) != 4)
				continue;

			/* flags 0 is an incomplete entry: no one is there */
			if (flags == 0 || strlen(mac) != 17 ||
			    strcmp(mac, "00:00:00:00:00:00") == 0)
				continue;

			if (!is_lan_iface(dev))
				continue;

			host[0] = '\0';
			if ((resolve_addr(ip, host) != 0) || (strcmp(ip, host) == 0))
				host[0] = '\0';
			else if ((dot = strchr(host, '.')) != NULL)
				*dot = '\0';	/* short name, as the GUI shows it */

			j_elem();
			j_str("ip", ip);
			j_str("mac", mac);
			j_str("iface", dev);
			j_str("hostname", host);
			j_elem_end();

			count++;
		}

		fclose(f);
	}

	j_arr_end();
	j_num("count", count);
	j_close();
}

/*
 * Wireless, one entry per enabled AP interface.
 *
 * Note for callers: wlhelper_get_channel_stats() and wlhelper_foreach_station()
 * each spawn iwinfo/iw, so this endpoint costs several processes. It is meant
 * to be polled every few seconds at most, not every frame.
 */

static int station_count_cb(const char *ifname, int phy,
                            const struct wlhelper_station_info *station,
                            void *user_data)
{
	(void)ifname; (void)phy; (void)station;

	(*(int *)user_data)++;

	return 0;
}

static int api_wireless_iface(int phy, int iface, const char *ifname, void *user_data)
{
	char key[64], mac[24], proto[16];
	int channel = 0, mhz = 0, nbw = 0, noise = 0, center = 0;
	int stations = 0;
	float rate = 0;

	(void)user_data;

	j_elem();
	j_num("phy", phy);
	j_num("iface", iface);
	j_str("ifname", ifname);

	snprintf(key, sizeof(key), "wifi_phy%diface%d_essid", phy, iface);
	j_nv("essid", key);
	snprintf(key, sizeof(key), "wifi_phy%diface%d_encryption", phy, iface);
	j_nv("encryption", key);
	snprintf(key, sizeof(key), "wifi_phy%diface%d_network", phy, iface);
	j_nv("network", key);
	snprintf(key, sizeof(key), "wifi_phy%diface%d_hidden", phy, iface);
	j_bool("hidden", nvram_get_int(key));
	snprintf(key, sizeof(key), "wifi_phy%d_band", phy);
	j_nv("band", key);

	if (wlhelper_get_mac_address(ifname, mac, sizeof(mac)) == 0)
		j_str("bssid", mac);

	proto[0] = '\0';
	/* No signal: that one reports the upstream AP's, which only means
	   something on a client interface, and this walks the APs. */
	if (wlhelper_get_channel_stats(ifname, &channel, &mhz, &nbw, &noise,
	                               &rate, &center, proto, sizeof(proto),
	                               NULL) == 0) {
		j_num("channel", channel);
		j_num("mhz", mhz);
		j_num("width", nbw);
		j_num("noise", noise);
		j_str("proto", proto);
	}

	wlhelper_foreach_station(ifname, phy, station_count_cb, &stations);
	j_num("stations", stations);

	j_elem_end();

	return 0;
}

static void api_wireless(void)
{
	j_arr("wireless");
	wlhelper_foreach_interface(WLHELPER_FILTER_ENABLED | WLHELPER_FILTER_AP_MODE,
	                           api_wireless_iface, NULL);
	j_arr_end();
}

/* The digits at the end of ethN, which is the port number everything else in
   this firmware uses - portN_label, asp_etherstates' portN - so a name set on
   the Port Labels page lands on the same socket here. */
static int api_port_index(const char *device)
{
	const char *p = device;

	while (*p && !isdigit((unsigned char)*p))
		p++;

	return *p ? atoi(p) : -1;
}


/*
 * The chassis ports: the WAN and the LAN bridge's members, in ethN order,
 * which on Tomato64 runs left to right across the case because set_devs_<board>
 * numbers them that way.
 *
 * The name is whatever the Port Labels page set for that port; failing that a
 * derived one. An SFP cage is found from the device tree label rather than a
 * per-board table, because the netdev itself is renamed to a plain ethN and
 * only the label survives in sysfs.
 */
static void api_ports(void)
{
	char devices[16][IFNAMSIZ];
	char wan[IFNAMSIZ], buf[64], key[16];
	char lanbuf[256], *tok, *save;
	unsigned int bridge;
	int count = 0, lanports = 0, i, j;

	strlcpy(wan, get_wanface("wan"), sizeof(wan));

	if (wan[0] != '\0')
		strlcpy(devices[count++], wan, IFNAMSIZ);

	/* Across every bridge: a socket assigned to br1 is still a socket on the
	   front of the case, and listing only br0's would lose it. */
	for (bridge = 0; bridge < BRIDGE_COUNT; bridge++) {
		char key[32];

		get_bridge_nvram_key(bridge, "ifnames", key, sizeof(key));
		strlcpy(lanbuf, nvram_safe_get(key), sizeof(lanbuf));

		for (tok = strtok_r(lanbuf, " ", &save);
		     tok != NULL && count < (int)(sizeof(devices) / IFNAMSIZ);
		     tok = strtok_r(NULL, " ", &save)) {
			if (strncmp(tok, "eth", 3) != 0)
				continue;

			for (i = 0; i < count; i++)
				if (strcmp(devices[i], tok) == 0)
					break;

			if (i == count)
				strlcpy(devices[count++], tok, IFNAMSIZ);
		}
	}

	/* ethN order, so the list reads across the chassis. */
	for (i = 1; i < count; i++) {
		char tmp[IFNAMSIZ];

		strlcpy(tmp, devices[i], sizeof(tmp));

		for (j = i - 1; j >= 0 && api_port_index(devices[j]) > api_port_index(tmp); j--)
			strlcpy(devices[j + 1], devices[j], IFNAMSIZ);

		strlcpy(devices[j + 1], tmp, IFNAMSIZ);
	}

	j_arr("ports");

	for (i = 0; i < count; i++) {
		const char *device = devices[i];
		int num = api_port_index(device);
		int is_wan = (wan[0] != '\0' && strcmp(device, wan) == 0);
		int is_sfp = 0;
		char name[32], *label;

		/* Lowercased rather than strcasestr(), which needs _GNU_SOURCE
		   and is not defined for this build. */
		if (api_netdev_sysfs(device, "of_node/label", buf, sizeof(buf))) {
			char *c;

			for (c = buf; *c; c++)
				*c = tolower((unsigned char)*c);

			is_sfp = (strstr(buf, "sfp") != NULL);
		}

		snprintf(key, sizeof(key), "port%d_label", num);
		label = nvram_safe_get(key);

		if (label[0] != '\0')
			strlcpy(name, label, sizeof(name));
		else if (is_wan)
			strlcpy(name, "WAN", sizeof(name));
		else if (is_sfp)
			strlcpy(name, "SFP+", sizeof(name));
		else
			snprintf(name, sizeof(name), "LAN %d", ++lanports);

		j_elem();
		j_str("device", device);
		j_num("port", num);
		j_str("name", name);
		j_str("role", is_wan ? "wan" : (is_sfp ? "sfp" : "lan"));
		j_bool("up", api_netdev_sysfs(device, "carrier", buf, sizeof(buf)) && buf[0] == '1');

		if (api_netdev_sysfs(device, "speed", buf, sizeof(buf)) && atoi(buf) > 0)
			j_num("speed", atoi(buf));
		else
			j_null("speed");

		if (api_netdev_sysfs(device, "duplex", buf, sizeof(buf)))
			j_str("duplex", buf);
		else
			j_null("duplex");

		j_elem_end();
	}

	j_arr_end();
}

/*
 * Every tunnel this firmware can run, whether or not it is up: a panel that
 * only listed the running ones would have nothing to switch on.
 *
 * "enabled" is start-at-boot, from the comma separated lists rc reads in
 * start_ovpn_eas(); "up" is the live answer, the same one the GUI's own status
 * uses - a pid for OpenVPN, the interface's operstate for WireGuard.
 *
 * The service name is what rc's service table calls it, so a caller that wants
 * to start or stop one has the argument for `service <name> start` in hand.
 */
#if defined(TCONFIG_OPENVPN) || defined(TCONFIG_WIREGUARD)
static int api_vpn_listed(const char *list, int num)
{
	char buf[32], *cur, *save;
	int found = 0;

	strlcpy(buf, nvram_safe_get(list), sizeof(buf));

	for (cur = strtok_r(buf, ",", &save); cur != NULL; cur = strtok_r(NULL, ",", &save))
		if (atoi(cur) == num)
			found = 1;

	return found;
}

static void api_vpn_entry(const char *kind, const char *service, int num,
                          const char *name, int enabled, int up)
{
	j_elem();
	j_str("kind", kind);
	j_str("service", service);
	j_num("index", num);
	j_str("name", name);
	j_bool("enabled", enabled);
	j_bool("up", up);
	j_elem_end();
}
#endif /* TCONFIG_OPENVPN || TCONFIG_WIREGUARD */

static void api_vpn(void)
{
#if defined(TCONFIG_OPENVPN) || defined(TCONFIG_WIREGUARD)
	char service[32], name[32];
	int i;
#endif

	j_arr("vpn");

#ifdef TCONFIG_OPENVPN
	for (i = 1; i <= OVPN_CLIENT_COUNT; i++) {
		snprintf(service, sizeof(service), "vpnclient%d", i);
		snprintf(name, sizeof(name), "OpenVPN client %d", i);
		api_vpn_entry("openvpn-client", service, i, name,
		              api_vpn_listed("vpnc_eas", i), pidof(service) > 0);
	}

	for (i = 1; i <= OVPN_SERVER_COUNT; i++) {
		snprintf(service, sizeof(service), "vpnserver%d", i);
		snprintf(name, sizeof(name), "OpenVPN server %d", i);
		api_vpn_entry("openvpn-server", service, i, name,
		              api_vpn_listed("vpns_eas", i), pidof(service) > 0);
	}
#endif /* TCONFIG_OPENVPN */

#ifdef TCONFIG_WIREGUARD
	for (i = 0; i < WG_INTERFACE_COUNT; i++) {
		char iface[8], key[16];

		snprintf(iface, sizeof(iface), "wg%d", i);
		snprintf(service, sizeof(service), "wireguard%d", i);
		snprintf(name, sizeof(name), "WireGuard %d", i);
		snprintf(key, sizeof(key), "wg%d_enable", i);

		api_vpn_entry("wireguard", service, i, name,
		              nvram_get_int(key) == 1, wg_status(iface));
	}
#endif /* TCONFIG_WIREGUARD */

	j_arr_end();
}

static void api_status(void)
{
	j_open(NULL);
	j_num("api", API_VERSION);
	api_router();
	api_system();
	api_lan();
	api_wan();
	api_clients();
	api_wireless();
	api_ports();
	api_vpn();
	j_close();
}

/* Gathers nothing. Answers whether the endpoint is reachable at all. */
static void api_ping(void)
{
	j_open(NULL);
	j_num("api", API_VERSION);
	j_bool("ok", 1);
	j_close();
}

static void api_one(void (*section)(void))
{
	j_open(NULL);
	j_num("api", API_VERSION);
	section();
	j_close();
}

static void api_error(const char *message)
{
	j_open(NULL);
	j_str("error", message);
	j_close();
}

/* -------------------------------------------------------------- router -- */

/*
 * url arrives without a leading slash and with the query string already
 * stripped, e.g. "api/v1/status". The pattern in mime_handlers[] guarantees
 * the prefix, so only what follows it has to be matched.
 */
void wo_api(char *url)
{
	const char *path;

	comma = 0;

	path = url;
	if (strncmp(path, "api/v1/", 7) == 0)
		path += 7;
	else {
		api_error("unsupported api version");
		return;
	}

	if (strcmp(path, "status") == 0)
		api_status();
	else if (strcmp(path, "ping") == 0)
		api_ping();
	else if (strcmp(path, "router") == 0)
		api_one(api_router);
	else if (strcmp(path, "system") == 0)
		api_one(api_system);
	else if (strcmp(path, "lan") == 0)
		api_one(api_lan);
	else if (strcmp(path, "wan") == 0)
		api_one(api_wan);
	else if (strcmp(path, "clients") == 0)
		api_one(api_clients);
	else if (strcmp(path, "wireless") == 0)
		api_one(api_wireless);
	else if (strcmp(path, "vpn") == 0)
		api_one(api_vpn);
	else if (strcmp(path, "ports") == 0)
		api_one(api_ports);
	else
		api_error("no such endpoint");
}
