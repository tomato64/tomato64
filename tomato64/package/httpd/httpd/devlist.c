/*
 *
 * Tomato Firmware
 * Copyright (C) 2006-2009 Jonathan Zarate
 *
 */


#include "tomato.h"

#include <ctype.h>
#include <sys/ioctl.h>
#include <net/if.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <string.h>
#include <netdb.h>

#include <wlutils.h>

#define lease_file		"/var/tmp/dhcp/leases"
#define lease_file_tmp		lease_file".!"
#define MAX_CLIENTS_COUNT	128


char *strupr(char *str)
{
	size_t i;
	size_t len = strlen(str);

	for (i = 0; i < len; i++)
		str[i] = toupper((unsigned char)str[i]);

	return str;
}

void asp_arplist(int argc, char **argv)
{
	FILE *f;
	char s[512];
	char ip[16];
	char mac[18];
	char dev[17];
	char host[NI_MAXHOST];
	char comma;
	char *c;
	unsigned int flags;

	/*
	 * cat /proc/net/arp
	 * IP address       HW type     Flags       HW address            Mask     Device
	 * 192.168.0.1      0x1         0x2         00:01:02:03:04:05     *        vlan1
	 */

	web_puts("\narplist = [");
	comma = ' ';
	if ((f = fopen("/proc/net/arp", "r")) != NULL) {
		while (fgets(s, sizeof(s), f)) {
			if (sscanf(s, "%15s %*s 0x%X %17s %*s %16s", ip, &flags, mac, dev) != 4)
				continue;

			if ((strlen(mac) != 17) || (strcmp(mac, "00:00:00:00:00:00") == 0))
				continue;

			if (flags == 0)
				continue;

			if ((resolve_addr(ip, host) != 0) || (strcmp(ip, host) == 0))
				strcpy(host, "");

			if ((c = strchr(host, '.')) != NULL)
				*c = 0;

			strupr(mac);
			web_printf("%c['%s','%s','%s','%s']", comma, ip, mac, dev, host);
			comma = ',';
		}
		fclose(f);
	}
	web_puts("];\n");
}

/* checkme: any easier way to do this?	zzz */
static int get_wds_ifname(const struct ether_addr *ea, char *ifname)
{
	struct ifreq ifr;
	int sd;
	unsigned int i;
	struct ether_addr e;

	if ((sd = socket(PF_INET, SOCK_DGRAM, 0)) >= 0) {
		/* wds doesn't show up under SIOCGIFCONF; seems to start at 17 (?) */
		for (i = 1; i < 32; ++i) {
			ifr.ifr_ifindex = i;
			if ((ioctl(sd, SIOCGIFNAME, &ifr) == 0) && (strncmp(ifr.ifr_name, "wds", 3) == 0) && (wl_ioctl(ifr.ifr_name, WLC_WDS_GET_REMOTE_HWADDR, &e.octet, sizeof(e.octet)) == 0)) {
				if (memcmp(ea->octet, e.octet, sizeof(e.octet)) == 0) {
					close(sd);
					strlcpy(ifname, ifr.ifr_name, 16);
					return 1;
				}
			}
		}
		close(sd);
	}
	return 0;
}

static int get_wl_clients(int idx, int unit, int subunit, void *param)
{
	char *comma = param;
	unsigned int i;
	char *p;
	char buf[32];
	char *wlif;
	scb_val_t rssi;
#ifdef TCONFIG_BCMARM
	scb_val_t rate_backup;
#endif
	sta_info_t sti;
	int cmd;
	struct maclist *mlist = NULL;
	int mac_list_size;
	char ifname[16];

	mac_list_size = sizeof(struct maclist) + (MAX_CLIENTS_COUNT * sizeof(struct ether_addr)); /* buffer and length */
	if ((mlist = malloc(mac_list_size)) != NULL) {
		wlif = nvram_safe_get(wl_nvname("ifname", unit, subunit)); /* AB multiSSID */
		cmd = WLC_GET_ASSOCLIST;
		while (1) {
			mlist->count = MAX_CLIENTS_COUNT;
			if (wl_ioctl(wlif, cmd, mlist, mac_list_size) == 0) {
				for (i = 0; i < mlist->count; ++i) {
					memcpy(&rssi.ea, &(mlist->ea[i]), sizeof(struct ether_addr));
					rssi.val = 0;
					if (wl_ioctl(wlif, WLC_GET_RSSI, &rssi, sizeof(rssi)) != 0)
						continue;

					memset(&sti, 0, sizeof(sti)); /* reset */
					strcpy((char *)&sti, "sta_info"); /* sta_info */
					memcpy((char *)&sti + 9, &(mlist->ea[i]), sizeof(struct ether_addr)); /* sta_info0<mac> */
					if (wl_ioctl(wlif, WLC_GET_VAR, &sti, sizeof(sti)) != 0)
						continue;
#ifdef TCONFIG_BCMARM
					/* check rate values of the driver */
					if ((sti.tx_rate == 0) && (sti.rx_rate == 0)) { /* if both values are empty */
						memcpy(&rate_backup.ea, &(mlist->ea[i]), sizeof(struct ether_addr));
						rate_backup.val = 0; /* reset */

						/* get backup values to show RX / TX values at the GUI */
						if (wl_ioctl(wlif, WLC_GET_RATE, &rate_backup, sizeof(rate_backup)) != 0)
							continue;

						/* rate_backup value contains link speed up+down */
						sti.tx_rate = rate_backup.val * 1000 / 2; /* assume symmetric up/down link and convert */
						sti.rx_rate = sti.tx_rate;
					}
#endif /* TCONFIG_BCMARM */
					p = wlif;
					if (sti.flags & WL_STA_WDS) {
						if (cmd != WLC_GET_WDSLIST)
							continue;
						if ((sti.flags & WL_WDS_LINKUP) == 0)
							continue;
						if (get_wds_ifname(&rssi.ea, ifname))
							p = ifname;
					}

					web_printf("%c['%s','%s',%d,%d,%d,%u,%d]", *comma, p, ether_etoa(rssi.ea.octet, buf), rssi.val, sti.tx_rate, sti.rx_rate, sti.in, unit);
					*comma = ',';
				}
			}
			if (cmd == WLC_GET_WDSLIST)
				break;

			cmd = WLC_GET_WDSLIST;
		}
		free(mlist);
	}

	return 0;
}

void asp_devlist(int argc, char **argv)
{
	FILE *f;
	char buf[1024];
	char mac[32];
	char ip[40];
	char hostname[256];
	char comma;
	char *host;
	unsigned long expires;

	/* must be here for easier call via update.cgi. arg is ignored */
	asp_arplist(0, NULL);
	asp_wlnoise(0, NULL);

	web_puts("wldev = [");
	comma = ' ';
	foreach_wif(1, &comma, get_wl_clients);
	web_puts("];\n");

	char *nvram_argv[] = { "wan_ifname,wan2_ifname,wan3_ifname,wan4_ifname,\
wan_iface,wan2_iface,wan3_iface,wan4_iface,\
wan_proto,wan2_proto,wan3_proto,wan4_proto,\
wan_ifnameX,wan2_ifnameX,wan3_ifnameX,wan4_ifnameX,\
wan_ifnames,wan2_ifnames,wan3_ifnames,wan4_ifnames,\
wan_ipaddr,wan2_ipaddr,wan3_ipaddr,wan4_ipaddr,\
wan_hwaddr,wan2_hwaddr,wan3_hwaddr,wan4_hwaddr,\
wan_ppp_get_ip,wan2_ppp_get_ip,wan3_ppp_get_ip,wan4_ppp_get_ip,\
wan_gateway_get,wan2_gateway_get,wan3_gateway_get,wan4_gateway_get,\
wan_gateway,wan2_gateway,wan3_gateway,wan4_gateway,\
wan_pptp_dhcp,wan2_pptp_dhcp,wan3_pptp_dhcp,wan4_pptp_dhcp,\
wan_pptp_server_ip,wan2_pptp_server_ip,wan3_pptp_server_ip,wan4_pptp_server_ip,\
lan_ifname,lan1_ifname,lan2_ifname,lan3_ifname,\
lan_ipaddr,lan1_ipaddr,lan2_ipaddr,lan3_ipaddr,\
lan_netmask,lan1_netmask,lan2_netmask,lan3_netmask,\
lan_ifnames,lan1_ifnames,lan2_ifnames,lan3_ifnames,\
web_svg,web_css,\
cstats_enable,cstats_labels,\
dhcpd_static,\
wl_ifname,wl_mode,wl_radio,wl_nband,wl_wds_enable"
	};

	asp_nvram(1, nvram_argv);

	web_puts("dhcpd_lease = [");

	if ((nvram_match("lan_proto", "dhcp")) || (nvram_match("lan1_proto", "dhcp")) || (nvram_match("lan2_proto", "dhcp")) || (nvram_match("lan3_proto", "dhcp"))) {
		f_write(lease_file_tmp, NULL, 0, 0, 0666);

		/* dump the leases to a file */
		if (killall("dnsmasq", SIGUSR2) == 0) {
			/* helper in dnsmasq will remove this when it's done */
			f_wait_notexists(lease_file_tmp, 5);
		}

		if ((f = fopen(lease_file, "r")) != NULL) {
			comma = ' ';
			while (fgets(buf, sizeof(buf), f)) {
				if (sscanf(buf, "%lu %17s %39s %255s", &expires, mac, ip, hostname) != 4)
					continue;

				host = js_string((hostname[0] == '*') ? "" : hostname);
				web_printf("%c['%s','%s','%s','%s']", comma, (host ? host : ""), ip, mac, ((expires == 0) ? "non-expiring" : reltime(buf, expires)));
				free(host);
				comma = ',';
			}
			fclose(f);
		}
		unlink(lease_file);
	}
	web_puts("];\n");
#if defined(TCONFIG_BCMARM) || defined(TCONFIG_MIPSR2)
	web_puts("gc_time = ");
	memset(buf, 0, sizeof(buf));
	if (f_read_string("/proc/sys/net/ipv4/neigh/default/gc_stale_time", buf, sizeof(buf)) > 0)
		web_printf("%d", atoi(buf));
	else
#ifdef TCONFIG_BCMARM
		web_puts("65");
#else
		web_puts("125");
#endif

	web_puts(";");
#endif /* TCONFIG_BCMARM || TCONFIG_MIPSR2 */
}
