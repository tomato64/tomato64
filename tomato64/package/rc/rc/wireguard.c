/*
 * wireguard.c
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
#include "curve25519.h"

/* needed by logmsg() */
#define LOGMSG_DISABLE		DISABLE_SYSLOG_OSM
#define LOGMSG_NVDEBUG		"wireguard_debug"

#define IF_SIZE			8
#define PEER_COUNT		3
#define MAX_LINE		1024

/* uncomment to add default routing (also in patches/wireguard-tools/101-tomato-specific.patch line 412 - 414) after kernel fix */
#define KERNEL_WG_FIX


/* interfaces that we want to ignore in standard PRB mode */
static const char *vpn_ifaces[] = { "wg0", 
                                    "wg1",
                                    "wg2",
                                    "tun11",
                                    "tun12",
                                    "tun13",
                                    NULL };

char port[BUF_SIZE_8];
char fwmark[BUF_SIZE_16];
unsigned int restart_dnsmasq = 0;
unsigned int restart_fw = 0;

/* structure for storing a dynamic array of domains */
typedef struct {
	char **domains;
	int count;
	int capacity;
} domain_list_t;

/* initializing the domain list */
static int init_domain_list(domain_list_t *list)
{
	list->count = 0;
	list->capacity = 10;
	list->domains = (char**)malloc(list->capacity * sizeof(char*));

	if (!list->domains)
		return -1;

	return 0;
}

/* freeing up domain list memory */
static void free_domain_list(domain_list_t *list)
{
	int i;

	if (list->domains) {
		for (i = 0; i < list->count; i++) {
			if (list->domains[i])
				free(list->domains[i]);

		}
		free(list->domains);
		list->domains = NULL;
	}

	list->count = 0;
	list->capacity = 0;
}

/* add domain to the list */
static int add_domain(domain_list_t *list, const char *domain)
{
	char **temp_domains;

	/* check if increase the array size is needed */
	if (list->count >= list->capacity - 1) { /* -1 for NULL at the end */
		list->capacity *= 2;
		temp_domains = (char**)realloc(list->domains, list->capacity * sizeof(char*));
		if (!temp_domains)
			return -1;

		list->domains = temp_domains;
	}

	/* allocate memory for the new domain */
	list->domains[list->count] = (char*)malloc((strlen(domain) + 1) * sizeof(char));
	if (!list->domains[list->count])
		return -1;

	/* copy domain */
	strlcpy(list->domains[list->count], domain, strlen(domain) + 1);
	list->count++;

	return 0;
}

static void update_dnsmasq_ipset(const char *tag, domain_list_t *list, int add)
{
	FILE *fp_read, *fp_write;
	char line[MAX_LINE], new_line[MAX_LINE];
	char temp_file[64], domain_entry[128];
	char *pos, *tag_pos;
	int found, tag_found, i, domain_count = 0;

	/* count domains if adding */
	if (add) {
		if (!list || !list->domains || list->count == 0)
			return;

		if (list->domains && list->count < list->capacity)
			list->domains[list->count] = NULL;

		while (list->domains[domain_count] != NULL)
		domain_count++;
	}

	if (!f_exists(dmipset))
		f_write(dmipset, NULL, 0, 0, 0);

	if (!(fp_read = fopen(dmipset, "r"))) {
		logmsg(LOG_WARNING, "cannot open file for reading: %s (%s)", dmipset, strerror(errno));
		return;
	}

	/* create temporary file path */
	memset(temp_file, 0, sizeof(temp_file));
	snprintf(temp_file, sizeof(temp_file), "%s.tmp", dmipset);
	if (!(fp_write = fopen(temp_file, "w"))) {
		logmsg(LOG_WARNING, "cannot open file for writing: %s (%s)", temp_file, strerror(errno));
		fclose(fp_read);
		return;
	}

	/* process existing file */
	memset(line, 0, MAX_LINE);
	while (fgets(line, MAX_LINE, fp_read)) {
		/* remove newline */
		pos = strchr(line, '\n');
		if (pos) *pos = '\0';

		/* skip empty lines */
		if (strlen(line) == 0)
			continue;

		/* check if line starts with ipset=/ */
		if (strncmp(line, "ipset=/", 7) != 0) {
			fprintf(fp_write, "%s\n", line);
			continue;
		}

		/* find domain part */
		pos = strchr(line + 7, '/');
		if (!pos) {
			fprintf(fp_write, "%s\n", line);
			continue;
		}

		/* extract domain */
		*pos = '\0';
		memset(domain_entry, 0, sizeof(domain_entry));
		strlcpy(domain_entry, line + 7, sizeof(domain_entry));
		*pos = '/';

		/* check if this domain should be processed */
		found = 0;
		if (add && list->domains) {
			for (i = 0; i < domain_count; i++) {
				if (strcmp(domain_entry, list->domains[i]) == 0) {
					found = 1;
					break;
				}
			}
		}

		/* look for our tag in the tags part */
		tag_pos = pos + 1;
		tag_found = 0;

		/* create a copy to work with */
		memset(new_line, 0, MAX_LINE);
		strlcpy(new_line, "ipset=/", MAX_LINE);
		strlcat(new_line, domain_entry, MAX_LINE);
		strlcat(new_line, "/", MAX_LINE);

		/* parse and rebuild tags */
		pos = strtok(tag_pos, ",");
		while (pos) {
			if (strcmp(pos, tag) == 0) {
				tag_found = 1;
				if (!add) {
					/* skip this tag when removing */
					pos = strtok(NULL, ",");
					continue;
				}
			}

			/* add tag to new line */
			if (strlen(new_line) > strlen("ipset=/") + strlen(domain_entry) + 1)
				strlcat(new_line, ",", MAX_LINE);

			strlcat(new_line, pos, MAX_LINE);
			pos = strtok(NULL, ",");
		}

		/* add our tag if adding and not found, and this domain is in our list */
		if (add && found && !tag_found) {
			if (strlen(new_line) > strlen("ipset=/") + strlen(domain_entry) + 1)
				strlcat(new_line, ",", MAX_LINE);

			strlcat(new_line, tag, MAX_LINE);
		}

		/* write line if it has tags after the domain */
		if (strlen(new_line) > strlen("ipset=/") + strlen(domain_entry) + 1)
			fprintf(fp_write, "%s\n", new_line);

	}

	/* add new entries for domains not found in file */
	if (add && list->domains) {
		rewind(fp_read); /* reset file pointer to beginning */

		for (i = 0; i < domain_count; i++) {
			found = 0;

			/* check if domain was already processed */
			memset(line, 0, MAX_LINE);
			while (fgets(line, MAX_LINE, fp_read)) {
				pos = strchr(line, '\n');
				if (pos)
					*pos = '\0';

				if (strncmp(line, "ipset=/", 7) == 0) {
					pos = strchr(line + 7, '/');
					if (pos) {
						*pos = '\0';
						if (strcmp(line + 7, list->domains[i]) == 0) {
							found = 1;
							break;
						}
						*pos = '/';
					}
				}
			}

			/* add new entry if domain not found */
			if (!found)
				fprintf(fp_write, "ipset=/%s/%s\n", list->domains[i], tag);
		}
	}

	fclose(fp_read);
	fclose(fp_write);

	/* replace original file with temporary file */
	if (rename(temp_file, dmipset) != 0) {
		logmsg(LOG_WARNING, "cannot rename file: %s (%s)", dmipset, strerror(errno));
		unlink(temp_file);
	}
}

static int file_contains(const char *filename, const char *pattern)
{
	FILE *fp;
	char line[BUF_SIZE];
	int found = 0;

	if (!(fp = fopen(filename, "r")))
		return 0;

	while (fgets(line, BUF_SIZE, fp)) {
		if (strstr(line, pattern)) {
			found = 1;
			break;
		}
	}
	fclose(fp);

	return found;
}

static int replace_in_file(const char *filename, const char *old_str, const char *new_str)
{
	FILE *fp_in = NULL;
	FILE *fp_out = NULL;
	char tmp_filename[FILENAME_MAX];
	char buf[4096];
	size_t old_len = strlen(old_str);
	size_t new_len = new_str ? strlen(new_str) : 0;

	if (!(fp_in = fopen(filename, "r"))) {
		logmsg(LOG_WARNING, "cannot open file for reading: %s (%s)", filename, strerror(errno));
		return -1;
	}

	memset(tmp_filename, 0, FILENAME_MAX);
	snprintf(tmp_filename, FILENAME_MAX, "/tmp/%s.tmp", filename);
	if (!(fp_out = fopen(tmp_filename, "w"))) {
		logmsg(LOG_WARNING, "could not create temporary file: %s (%s)", tmp_filename, strerror(errno));
		fclose(fp_in);
		return -1;
	}

	while (fgets(buf, sizeof(buf), fp_in)) {
		char *match = strstr(buf, old_str);
		if (match) {
			if (!new_str) /* delete line */
				continue;

			char *pos = buf;
			while ((match = strstr(pos, old_str))) {
				fwrite(pos, 1, match - pos, fp_out);
				fwrite(new_str, 1, new_len, fp_out);
				pos = match + old_len;
			}
			fputs(pos, fp_out);
		}
		else
			fputs(buf, fp_out);
	}

	fclose(fp_in);
	fclose(fp_out);

	if (rename(tmp_filename, filename) != 0) {
		logmsg(LOG_WARNING, "failed to overwrite %s: %s", filename, strerror(errno));
		eval("rm", "-rf", tmp_filename);
		return -1;
	}

	return 0;
}

static void wg_build_firewall(const int unit, const char *port) {
	FILE *fp;
	char buffer[BUF_SIZE_64];
	char tmp[BUF_SIZE_16];
	char *dns;
	int nvi;

	memset(buffer, 0, BUF_SIZE_64);
	snprintf(buffer, BUF_SIZE_64, WG_FW_DIR"/wg%d-fw.sh", unit);

	/* script with firewall rules (port, unit) */
	/* (..., open wireguard port, accept packets from wireguard internal subnet, set up forwarding) */
	if ((fp = fopen(buffer, "w"))) {
		chains_log_detection();

		fprintf(fp, "#!/bin/sh\n"
		            "\n# FW\n");

		nvi = atoi(getNVRAMVar("wg%d_fw", unit));

		/* Handle firewall rules if appropriate */
		memset(tmp, 0, BUF_SIZE_16);
		snprintf(tmp, BUF_SIZE_16, "wg%d_firewall", unit);
		if (atoi(getNVRAMVar("wg%d_com", unit)) == 3 && !nvram_contains_word(tmp, "custom")) { /* 'External - VPN Provider' & auto */
			fprintf(fp, "iptables -I INPUT -i wg%d -m state --state NEW -j %s\n"
			            "iptables -I FORWARD -i wg%d -m state --state NEW -j %s\n"
			            "iptables -I FORWARD -o wg%d -j ACCEPT\n"
			            "echo 1 > /proc/sys/net/ipv4/conf/all/src_valid_mark\n",
			            unit, (nvi ? chain_in_drop : chain_in_accept),
			            unit, (nvi ? "DROP" : "ACCEPT"),
			            unit);

			/* masquerade all peer outbound traffic regardless of source subnet */
			if (atoi(getNVRAMVar("wg%d_nat", unit)) == 1)
				fprintf(fp, "iptables -t nat -I POSTROUTING -o wg%d -j MASQUERADE\n", unit);

			if (atoi(getNVRAMVar("wg%d_rgwr", unit)) >= VPN_RGW_POLICY) {
				/* Disable rp_filter when in policy mode */
				fprintf(fp, "echo 0 > /proc/sys/net/ipv4/conf/wg%d/rp_filter\n"
				            "echo 0 > /proc/sys/net/ipv4/conf/all/rp_filter\n",
				            unit);
			}
		}
		else if (atoi(getNVRAMVar("wg%d_com", unit)) != 3) { /* other */
			fprintf(fp, "iptables -A INPUT -p udp --dport %s -j %s\n"
			            "iptables -A INPUT -i wg%d -j %s\n"
			            "iptables -A FORWARD -i wg%d -j ACCEPT\n",
			            port, chain_in_accept,
			            unit, chain_in_accept,
			            unit);
		}

		if (!nvram_get_int("ctf_disable")) { /* bypass CTF if enabled */
			fprintf(fp, "iptables -t mangle -I PREROUTING -i wg%d -j MARK --set-mark 0x01/0x7\n"
			            "iptables -t mangle -I POSTROUTING -o wg%d -j MARK --set-mark 0x01/0x7\n",
			            unit, unit);
#ifdef TCONFIG_IPV6
			if (ipv6_enabled()) {
				fprintf(fp, "ip6tables -t mangle -I PREROUTING -i wg%d -j MARK --set-mark 0x01/0x7\n"
				            "ip6tables -t mangle -I POSTROUTING -o wg%d -j MARK --set-mark 0x01/0x7\n",
				            unit, unit);
			}
#endif
		}

		dns = getNVRAMVar("wg%d_dns", unit);
		if (getNVRAMVar("wg%d_file", unit)[0] == '\0') { /* only if no optional config file has been added */
			/* script to add/remove fw rules for dns servers (unit, dns) */
			fprintf(fp, "\n# DNS\n"
			            "DNS_CHAIN=\"wg-ft-wg%d-dns\"\n"
			            "DNS_FILE=\"%s/wg%d.conf\"\n"
			            "STATUS=$?\n"
			            "\niptables -nL | grep \"$DNS_CHAIN\" && {\n"
			            " # remove rules\n"
			            " [ -r \"$DNS_FILE\" ] && {\n"
			            "  rm $DNS_FILE\n"
			            "  nohup service dnsmasq restart &\n"
			            "  iptables -D OUTPUT -j $DNS_CHAIN\n"
			            "  iptables -F $DNS_CHAIN\n"
			            "  iptables -X $DNS_CHAIN\n"
			            " }\n"
			            "} || {\n"
			            " # add rules\n"
			            " [ \"%s\" != \"\" ] && {\n"
			            "  > $DNS_FILE\n"
			            "  iptables -N $DNS_CHAIN\n"
			            "  for NAMESERVER in $(echo \"%s\" | tr \",\" \" \" ); do\n"
			            "   echo \"server=$NAMESERVER\" >> $DNS_FILE\n"
			            "   iptables -A $DNS_CHAIN -i wg%d -p tcp --dst $NAMESERVER/32 --dport 53 -j ACCEPT\n"
			            "   iptables -A $DNS_CHAIN -i wg%d -p udp --dst $NAMESERVER/32 --dport 53 -j ACCEPT\n"
			            "  done\n"
			            "  iptables -A OUTPUT -j $DNS_CHAIN\n"
			            "  [ $? -eq 0 ] && nohup service dnsmasq restart &\n"
			            " } || {\n"
			            "  exit $STATUS\n"
			            " }\n"
			            "}\n",
			            unit,
			            WG_DNS_DIR, unit,
			            dns,
			            dns,
			            unit,
			            unit);
		}
		fclose(fp);
		chmod(buffer, (S_IRUSR | S_IWUSR | S_IXUSR));
	}
}

static void wg_build_routing(const int unit, const char *fwmark, const char *fwmark_mask, const char *wgrouting_mark) {
	FILE *fp;
	char *enable, *type, *value, *kswitch;
	char *nv, *nvp, *b;
	char buffer[BUF_SIZE_64];
	int policy;
	domain_list_t my_domains;

	memset(buffer, 0, BUF_SIZE_64);
	snprintf(buffer, BUF_SIZE_64, WG_FW_DIR"/wg%d-fw-routing.sh", unit);

	/* script with routing policy rules */
	if ((fp = fopen(buffer, "w"))) {
		fprintf(fp, "#!/bin/sh\n"
		            "\n# Routing\n"
		            "iptables -t mangle -A PREROUTING -m set --match-set %s dst,src -j MARK --set-mark %s\n",
		            wgrouting_mark, fwmark_mask);

		if (init_domain_list(&my_domains) != 0) {
			logmsg(LOG_WARNING, "cannot initialize domain list");
			fclose(fp);
			eval("rm", "-rf", buffer);
			return;
		}

		/* example of routing_val: 1<2<8.8.8.8<1>1<1<1.2.3.4<0>1<3<domain.com<0> (enabled<type<domain_or_IP<kill_switch>) */
		nv = nvp = strdup(getNVRAMVar("wg%d_routing_val", unit));

		while (nvp && (b = strsep(&nvp, ">")) != NULL) {
			enable = type = value = kswitch = NULL;

			/* enable<type<domain_or_IP<kill_switch> */
			if ((vstrsep(b, "<", &enable, &type, &value, &kswitch)) < 4)
				continue;

			/* check if rule is enabled and type is set and IP/domain is set */
			if ((atoi(enable) != 1) || (*type == '\0') || (*value == '\0'))
				continue;

			policy = atoi(type);
			switch (policy) {
			case 1: /* from source */
				logmsg(LOG_INFO, "type: %d - add %s (wg%d)", policy, value, unit);
				if (strstr(value, "-")) /* range */
					fprintf(fp, "iptables -t mangle -A PREROUTING -m iprange --src-range %s -j MARK --set-mark %s\n", value, fwmark_mask);
				else
					fprintf(fp, "iptables -t mangle -A PREROUTING -s %s -j MARK --set-mark %s\n", value, fwmark_mask);
				break;
			case 2: /* to destination */
				logmsg(LOG_INFO, "type: %d - add %s (wg%d)", policy, value, unit);
				fprintf(fp, "iptables -t mangle -A PREROUTING -d %s -j MARK --set-mark %s\n", value, fwmark_mask);
				break;
			case 3: /* to domain */
				logmsg(LOG_INFO, "type: %d - add %s (wg%d)", policy, value, unit);
				add_domain(&my_domains, value);
				restart_dnsmasq = 1;
				break;
			default:
				continue;
			}
		}
		if (nv)
			free(nv);

		fclose(fp);
		chmod(buffer, (S_IRUSR | S_IWUSR | S_IXUSR));

		if (my_domains.count) {
			update_dnsmasq_ipset(wgrouting_mark, &my_domains, 1);
			free_domain_list(&my_domains);
		}

		restart_fw = 1;
	}
}

static int wg_quick_iface(char *iface, const char *file, const int up)
{
	char buffer[BUF_SIZE_32];
	char *up_down = (up == 1 ? "up" : "down");
	FILE *f;

	/* copy config to wg dir with proper name */
	memset(buffer, 0, BUF_SIZE_32);
	snprintf(buffer, BUF_SIZE_32, WG_DIR"/%s.conf", iface);
	if ((f = fopen(buffer, "w")) != NULL) {
		if (fappend(f, file) < 0) {
			logmsg(LOG_WARNING, "fappend failed for %s", buffer);
			fclose(f);
			return -1;
		}
	}
	else
		logmsg(LOG_WARNING, "unable to open wireguard configuration file %s for interface %s!", file, iface);

	/* set up/down wireguard IF */
	if (eval("wg-quick", up_down, buffer, iface, "--norestart")) {
		logmsg(LOG_WARNING, "unable to set %s wireguard interface %s from file %s!", up_down, iface, file);
		return -1;
	}
	else
		logmsg(LOG_DEBUG, "wireguard interface %s from file %s set %s", iface, file, up_down);

	stop_dnsmasq();
	start_dnsmasq();

	return 0;
}

static void wg_find_port(const int unit, char *port)
{
	char *b;

	b = getNVRAMVar("wg%d_port", unit);
	memset(port, 0, BUF_SIZE_8);
	if (b[0] == '\0')
		snprintf(port, BUF_SIZE_8, "%d", 51820 + unit);
	else
		snprintf(port, BUF_SIZE_8, "%s", b);
}

static void wg_find_fwmark(const int unit, char *port, char *fwmark)
{
	char *b;

	b = getNVRAMVar("wg%d_fwmark", unit);
	memset(fwmark, 0, BUF_SIZE_16);
	if (b[0] == '\0' || b[0] == '0')
		snprintf(fwmark, BUF_SIZE_16, "%s", port);
	else
		snprintf(fwmark, BUF_SIZE_16, "%s", b);
}

static int wg_if_exist(const char *ifname)
{
	return if_nametoindex(ifname) ? 1 : 0;
}

static void wg_setup_dirs(void) {
	/* main dir */
	if (mkdir_if_none(WG_DIR))
		chmod(WG_DIR, (S_IRUSR | S_IWUSR | S_IXUSR));

	/* script dir */
	if (mkdir_if_none(WG_SCRIPTS_DIR))
		chmod(WG_SCRIPTS_DIR, (S_IRUSR | S_IWUSR | S_IXUSR));

	/* keys dir */
	if (mkdir_if_none(WG_KEYS_DIR))
		chmod(WG_KEYS_DIR, (S_IRUSR | S_IWUSR | S_IXUSR));

	/* dns dir */
	if (mkdir_if_none(WG_DNS_DIR))
		chmod(WG_DNS_DIR, (S_IRUSR | S_IWUSR | S_IXUSR));

	/* FW dir */
	if (mkdir_if_none(WG_FW_DIR))
		chmod(WG_FW_DIR, (S_IRUSR | S_IWUSR | S_IXUSR));
}

static void wg_cleanup_dirs(void) {
	eval("rm", "-rf", WG_DIR);
}

static void wg_setup_watchdog(const int unit)
{
	FILE *fp;
	char buffer[BUF_SIZE_64], buffer2[BUF_SIZE_64];
	char taskname[BUF_SIZE_32];
	int nvi;

	if ((nvi = atoi(getNVRAMVar("wg%d_poll", unit))) > 0) {
		memset(buffer, 0, BUF_SIZE_64);
		snprintf(buffer, BUF_SIZE_64, WG_SCRIPTS_DIR"/watchdog%d.sh", unit);

		if ((fp = fopen(buffer, "w"))) {
			fprintf(fp, "#!/bin/sh\n"
			            "pingme() {\n"
			            "[ \"3\" != \"%d\" -o \"%d\" = \"0\" ] && return 0\n"
			            " local i=1\n"
			            " while :; do\n"
			            "  ping -qc1 -W3 -I wg%d %s &>/dev/null && return 0\n"
			            "  [ $((i++)) -ge 3 ] && break || sleep 5\n"
			            " done\n"
			            " return 1\n"
			            "}\n"
			            "ISUP=$(cat /sys/class/net/wg%d/operstate)\n"
			            "[ \"$(nvram get g_upgrade)\" != \"1\" -a \"$(nvram get g_reboot)\" != \"1\" ] && {\n"
			            " [ \"$ISUP\" == \"unknown\" -o \"$ISUP\" == \"up\" ] && pingme && exit 0\n"
			            " logger -t wg-watchdog wg%d stopped? Starting...\n"
			            " service wireguard%d restart\n"
			            "}\n",
			            atoi(getNVRAMVar("wg%d_com", unit)), /* only for 'External' (3) mode */ atoi(getNVRAMVar("wg%d_tchk", unit)),
			            unit, nvram_safe_get("wan_checker"),
			            unit,
			            unit,
			            unit);
			fclose(fp);
			chmod(buffer, (S_IRUSR | S_IWUSR | S_IXUSR));

			memset(taskname, 0, BUF_SIZE_32);
			snprintf(taskname, BUF_SIZE_32,"CheckWireguard%d", unit);
			memset(buffer2, 0, BUF_SIZE_64);
			snprintf(buffer2, BUF_SIZE_64, "*/%d * * * * %s", nvi, buffer);
			eval("cru", "a", taskname, buffer2);
		}
	}
}

static int wg_create_iface(char *iface)
{
	/* Make sure module is loaded */
	modprobe("wireguard");
	f_wait_exists("/sys/module/wireguard", 5);

	/* Create wireguard interface */
	if (eval("ip", "link", "add", "dev", iface, "type", "wireguard")) {
		logmsg(LOG_WARNING, "unable to create wireguard interface %s!", iface);
		return -1;
	}
	else
		logmsg(LOG_DEBUG, "wireguard interface %s has been created", iface);

	return 0;
}

static int wg_set_iface_addr(char *iface, const char *addr)
{
	char *nv, *nv_orig, *b;

	/* Flush all addresses from interface */
	if (eval("ip", "addr", "flush", "dev", iface)) {
		logmsg(LOG_WARNING, "unable to flush wireguard interface %s!", iface);
		return -1;
	}
	else
		logmsg(LOG_DEBUG, "successfully flushed wireguard interface %s", iface);

	/* Set wireguard interface address(es) */
	nv = nv_orig = strdup(addr);
	while (nv && (b = strsep(&nv, ",")) != NULL) {
		if (eval("ip", "addr", "add", b, "dev", iface)) {
			logmsg(LOG_WARNING, "unable to add wireguard interface %s address of %s!", iface, b);
			if (nv_orig)
				free(nv_orig);

			return -1;
		}
		else
			logmsg(LOG_DEBUG, "wireguard interface %s has had address %s add to it", iface, b);
	}

	if (nv_orig)
		free(nv_orig);

	return 0;
}

static int wg_set_iface_port(char *iface, char *port)
{
	if (eval("wg", "set", iface, "listen-port", port)) {
		logmsg(LOG_WARNING, "unable to set wireguard interface %s port to %s!", iface, port);
		return -1;
	}
	else
		logmsg(LOG_DEBUG, "wireguard interface %s has had its port set to %s", iface, port);

	return 0;
}

static int wg_set_iface_privkey(char *iface, const char *privkey)
{
	FILE *fp;
	char buffer[BUF_SIZE];

	/* write private key to file */
	memset(buffer, 0, BUF_SIZE);
	snprintf(buffer, BUF_SIZE, WG_KEYS_DIR"/%s", iface);

	if (!(fp = fopen(buffer, "w"))) {
		logmsg(LOG_WARNING, "cannot open file for writing: %s (%s)", buffer, strerror(errno));
		return -1;
	}
	fprintf(fp, "%s", privkey);
	fclose(fp);

	chmod(buffer, (S_IRUSR | S_IWUSR));

	/* set interface private key */
	if (eval("wg", "set", iface, "private-key", buffer)) {
		logmsg(LOG_WARNING, "unable to set wireguard interface %s private key!", iface);
		return -1;
	}
	else
		logmsg(LOG_DEBUG, "wireguard interface %s has had its private key set", iface);

	/* remove file for security */
	remove(buffer);

	return 0;
}

static int wg_set_iface_fwmark(char *iface, char *fwmark)
{
	if (eval("wg", "set", iface, "fwmark", fwmark)) {
		logmsg(LOG_WARNING, "unable to set wireguard interface %s fwmark to %s!", iface, fwmark);
		return -1;
	}
	else
		logmsg(LOG_DEBUG, "wireguard interface %s has had its fwmark set to %s", iface, fwmark);

	return 0;
}

static int wg_set_iface_mtu(char *iface, char *mtu)
{
	if (eval("ip", "link", "set", "dev", iface, "mtu", mtu)) {
		logmsg(LOG_WARNING, "unable to set wireguard interface %s mtu to %s!", iface, mtu);
		return -1;
	}
	else
		logmsg(LOG_DEBUG, "wireguard interface %s has had its mtu set to %s", iface, mtu);

	return 0;
}

static int wg_set_iface_up(char *iface)
{
	int retry = 0;

	while (retry < 5) {
		if (!(eval("ip", "link", "set", "up", "dev", iface))) {
			logmsg(LOG_DEBUG, "wireguard interface %s has been brought up", iface);
			return 0;
		}
		else if (retry < 4) {
			logmsg(LOG_WARNING, "unable to bring up wireguard interface %s, retrying %d ...", iface, retry + 1);
			sleep(4);
		}
		retry += 1;
	}

	logmsg(LOG_WARNING, "unable to bring up wireguard interface %s!", iface);

	return -1;
}

static int wg_iface_script(const int unit, const char *script_name)
{
	char *script;
	char buffer[BUF_SIZE_32];
	char path[BUF_SIZE_64];
	FILE *fp;

	memset(buffer, 0, BUF_SIZE_32);
	snprintf(buffer, BUF_SIZE_32, "wg%d_%s", unit, script_name);

	script = nvram_safe_get(buffer);

	if (strcmp(script, "") != 0) {
		/* build path */
		memset(path, 0, BUF_SIZE_64);
		snprintf(path, BUF_SIZE_64, WG_SCRIPTS_DIR"/wg%d-%s.sh", unit, script_name);

		if (!(fp = fopen(path, "w"))) {
			logmsg(LOG_WARNING, "unable to open %s for writing!", path);
			return -1;
		}
		fprintf(fp, "#!/bin/sh\n%s\n", script);
		fclose(fp);

		/* replace %i with interface */
		memset(buffer, 0, BUF_SIZE_32);
		snprintf(buffer, BUF_SIZE_32, "wg%d", unit);
		if (replace_in_file(path, "%i", buffer) != 0) {
			logmsg(LOG_WARNING, "unable to substitute interface name in %s script for wireguard interface wg%d!", script_name, unit);
			return -1;
		}
		else
			logmsg(LOG_DEBUG, "interface substitution in %s script for wireguard interface wg%d has executed successfully", script_name, unit);

		/* run script */
		chmod(path, (S_IRUSR | S_IWUSR | S_IXUSR));
		if (eval(path)) {
			logmsg(LOG_WARNING, "unable to execute %s script for wireguard interface wg%d!", script_name, unit);
			return -1;
		}
		else
			logmsg(LOG_DEBUG, "%s script for wireguard interface wg%d has executed successfully", script_name, unit);
	}

	return 0;
}

static void wg_iface_pre_up(const int unit)
{
	wg_iface_script(unit, "preup");
}

static void wg_iface_post_up(const int unit)
{
	wg_iface_script(unit, "postup");
}

static void wg_iface_pre_down(const int unit)
{
	wg_iface_script(unit, "predown");
}

static void wg_iface_post_down(const int unit)
{
	wg_iface_script(unit, "postdown");
}

static int wg_set_peer_psk(char *iface, char *pubkey, const char *presharedkey)
{
	FILE *fp;
	char buffer[BUF_SIZE];
	int err = 0;

	/* write preshared key to file */
	memset(buffer, 0, BUF_SIZE);
	snprintf(buffer, BUF_SIZE, WG_KEYS_DIR"/%s.psk", iface);

	if (!(fp = fopen(buffer, "w"))) {
		logmsg(LOG_WARNING, "cannot open file for writing: %s (%s)", buffer, strerror(errno));
		return -1;
	}
	fprintf(fp, "%s", presharedkey);
	fclose(fp);

	if (eval("wg", "set", iface, "peer", pubkey, "preshared-key", buffer)) {
		logmsg(LOG_WARNING, "unable to add preshared key to peer %s on wireguard interface %s!", pubkey, iface);
		err = -1;
	}
	else
		logmsg(LOG_DEBUG, "preshared key has been added to peer %s on wireguard interface %s", pubkey, iface);

	/* remove file for security */
	remove(buffer);

	return err;
}

static int wg_set_peer_keepalive(char *iface, char *pubkey, char *keepalive)
{
	if (eval("wg", "set", iface, "peer", pubkey, "persistent-keepalive", keepalive)) {
		logmsg(LOG_WARNING, "unable to add persistent-keepalive of %s to peer %s on wireguard interface %s!", keepalive, pubkey, iface);
		return -1;
	}
	else
		logmsg(LOG_DEBUG, "persistent-keepalive of %s has been added to peer %s on wireguard interface %s", keepalive, pubkey, iface);

	return 0;
}

static int wg_set_peer_endpoint(const int unit, char *iface, char *pubkey, const char *endpoint, const char *port)
{
	char buffer[BUF_SIZE_64];

	memset(buffer, 0, BUF_SIZE_64);

	if (atoi(getNVRAMVar("wg%d_com", unit)) == 3) /* 'External - VPN Provider' */
		snprintf(buffer, BUF_SIZE_64, "%s", endpoint);
	else
		snprintf(buffer, BUF_SIZE_64, "%s:%s", endpoint, port);

	if (eval("wg", "set", iface, "peer", pubkey, "endpoint", buffer)) {
		logmsg(LOG_WARNING, "unable to add endpoint of %s to peer %s on wireguard interface %s!", buffer, pubkey, iface);
		return -1;
	}
	else
		logmsg(LOG_DEBUG, "endpoint of %s has been added to peer %s on wireguard interface %s", buffer, pubkey, iface);

	return 0;
}

static int wg_route_peer(char *iface, char *route, char *table, const int add)
{
	if (add == 1) {
		if (table != NULL) {
			if (eval("ip", "route", "add", route, "dev", iface, "table", table)) {
				logmsg(LOG_WARNING, "unable to add route of %s to table %s for wireguard interface %s! When using mask, check if the entry is correct (for the /24-31 mask the last IP octet must be 0, for the /9-16 mask the last two octets must be 0, etc.)", route, table, iface);
				return -1;
			}
			else
				logmsg(LOG_DEBUG, "wireguard interface %s has had a route added to table %s for %s", iface, table, route);
		}
		else {
			if (eval("ip", "route", "add", route, "dev", iface)) {
				logmsg(LOG_WARNING, "unable to add route of %s for wireguard interface %s! When using mask, check if the entry is correct (for the /24-31 mask the last IP octet must be 0, for the /9-16 mask the last two octets must be 0, etc.)", route, iface);
				return -1;
			}
			else
				logmsg(LOG_DEBUG, "wireguard interface %s has had a route added for %s", iface, route);
		}
	}
	else {
		if (table != NULL) {
			if (eval("ip", "route", "delete", route, "dev", iface, "table", table)) {
				logmsg(LOG_WARNING, "unable to remove route of %s to table %s for wireguard interface %s!", route, table, iface);
				return -1;
			}
			else
				logmsg(LOG_DEBUG, "wireguard interface %s has had a route removed to table %s for %s", iface, table, route);
		}
		else {
			if (eval("ip", "route", "delete", route, "dev", iface)) {
				logmsg(LOG_WARNING, "unable to remove route of %s for wireguard interface %s!", route, iface);
				return -1;
			}
			else
				logmsg(LOG_DEBUG, "wireguard interface %s has had a route removed for %s", iface, route);
		}
	}
	return 0;
}

static void wg_route_bridges(char *fwmark, const int add)
{
	int i;
	char cmd[BUF_SIZE];
	char line[BUF_SIZE];
	char *field;
	FILE *fp;

	for (i = 0; i < BRIDGE_COUNT; i++) { /* todo: add to GUI the option to select which bridge wg should route traffic to */
		memset(cmd, 0, BUF_SIZE);
		snprintf(cmd, BUF_SIZE, "ip route show dev br%d", i);
		if ((fp = popen(cmd, "r"))) {
			while (fgets(line, BUF_SIZE, fp)) {
				line[strcspn(line, "\n")] = '\0';

				field = strtok(line, " \t");
				if (field && strlen(field) > 1) {
					memset(cmd, 0, BUF_SIZE);
					snprintf(cmd, BUF_SIZE, "ip route %s %s dev br%d table %s", (add ? "add" : "delete"), field, i, fwmark);
					logmsg(LOG_DEBUG, "[wg_route_bridges]: %s", cmd);
					system(cmd);
				}
			}
			pclose(fp);
		}
	}
}

static void wg_route_peer_default(char *iface, char *route, char *fwmark, const int add)
{
	if (add == 1) {
#ifdef KERNEL_WG_FIX
		wg_route_peer(iface, route, fwmark, 1);

		if (eval("ip", "rule", "add", "not", "fwmark", fwmark, "table", fwmark))
			logmsg(LOG_WARNING, "unable to filter fwmark %s for default route of %s on wireguard interface %s!", fwmark, route, iface);

		if (eval("ip", "rule", "add", "table", "main", "suppress_prefixlength", "0"))
			logmsg(LOG_WARNING, "unable to suppress prefix length of 0 for default route of %s on wireguard interface %s!", route, iface);
#else
		wg_route_bridges(fwmark, 1);

		wg_route_peer(iface, route, fwmark, 1);

		if (eval("ip", "rule", "add", "not", "fwmark", fwmark, "table", fwmark))
			logmsg(LOG_WARNING, "unable to filter fwmark %s for default route of %s on wireguard interface %s!", fwmark, route, iface);
#endif
	}
	else {
#ifdef KERNEL_WG_FIX
		if (eval("ip", "rule", "delete", "table", "main", "suppress_prefixlength", "0"))
			logmsg(LOG_WARNING, "unable to remove suppress prefix length of 0 for default route of %s on wireguard interface %s!", route, iface);

		if (eval("ip", "rule", "delete", "not", "from", "all", "fwmark", fwmark, "lookup", fwmark))
			logmsg(LOG_WARNING, "unable to remove filter fwmark %s for default route of %s on wireguard interface %s!", fwmark, route, iface);

		wg_route_peer(iface, route, fwmark, 0);
#else
		if (eval("ip", "rule", "delete", "not", "from", "all", "fwmark", fwmark, "lookup", fwmark))
			logmsg(LOG_WARNING, "unable to remove filter fwmark %s for default route of %s on wireguard interface %s!", fwmark, route, iface);

		wg_route_peer(iface, route, fwmark, 0);

		wg_route_bridges(fwmark, 0);
#endif
	}
}

static void wg_init_table(char *iface, char *fwmark)
{
	FILE *fp;
	char route[BUF_SIZE];
	char cmd[BUF_SIZE_64];
	unsigned int i, n_ifaces;
	int routing = atoi(getNVRAMVar("wg%d_rgwr", atoi(&iface[2])));

	logmsg(LOG_INFO, "creating wireguard (wg%d) routing table (mode %d)", atoi(&iface[2]), routing);

	/* strict - copy routes from main routing table only for this interface */
	if (routing == VPN_RGW_POLICY_STRICT) {
		memset(cmd, 0, BUF_SIZE_64);
		snprintf(cmd, BUF_SIZE_64, "ip route show table main dev %s", iface);

		if ((fp = popen(cmd, "r")) != NULL) {
			while (fgets(route, BUF_SIZE, fp)) {
				route[strcspn(route, "\n")] = '\0';
				eval("ip", "route", "add", "table", fwmark, route, "dev", iface);
				logmsg(LOG_DEBUG, "[PBR strict] added: ip route add table %s %s dev %s", fwmark, route, iface);
			}
			pclose(fp);
		}
	}
	/* standard - copy routes from main routing table (exclude vpns and all default gateways) */
	else if (routing == VPN_RGW_POLICY) {
		if ((fp = popen("ip route show table main", "r")) != NULL) {
			n_ifaces = ASIZE(vpn_ifaces);

			while (fgets(route, BUF_SIZE, fp)) {
				char *nl = strchr(route, '\n');
				unsigned int skip = 0;

				if (nl)
					*nl = '\0';

				/* skip all default gateways */
				if ((strncmp(route, "default ", 8) == 0) || (strncmp(route, "0.0.0.0/1 ", 10) == 0) || (strncmp(route, "128.0.0.0/1 ", 12) == 0))
					continue;

				/* skip iface from vpn_ifaces[] */
				for (i = 0; i < n_ifaces; i++) {
					if (vpn_ifaces[i] == NULL)
						break;
					if (strstr(route, vpn_ifaces[i])) {
						skip = 1;
						break;
					}
				}
				if (skip)
					continue;

				eval("ip", "route", "add", "table", fwmark, route);
				logmsg(LOG_DEBUG, "[PBR std] added: ip route add table %s %s", fwmark, route);
			}
			pclose(fp);
		}
	}

	wg_route_bridges(fwmark, 1); /* add */
}

static void wg_routing_policy(char *iface, char *route, char *fwmark, const int add)
{
	char buffer[BUF_SIZE_64];
	char fwmark_mask[BUF_SIZE_16];
	char wgrouting_mark[BUF_SIZE_16];
	char *priority;

	/* first always remove everything */

	logmsg(LOG_INFO, "clean-up wireguard routing - interface %s - table %s", iface, fwmark);

	eval("ip", "route", "flush", "table", fwmark);
	eval("ip", "route", "flush", "cache");

	memset(fwmark_mask, 0, BUF_SIZE_16);
	snprintf(fwmark_mask, BUF_SIZE_16, "%s/0xf00", fwmark);

	memset(wgrouting_mark, 0, BUF_SIZE_16);
	snprintf(wgrouting_mark, BUF_SIZE_16, "wgrouting%s", fwmark);

	eval("ip", "rule", "delete", "table", fwmark, "fwmark", fwmark_mask);

	wg_route_bridges(fwmark, 0); /* remove */

	memset(buffer, 0, BUF_SIZE_64);
	snprintf(buffer, BUF_SIZE_64, WG_FW_DIR"/%s-fw-routing.sh", iface);
	if (f_exists(buffer)) {
		simple_lock("firewall");
		/* replace -I & -A with -D */
		if ((replace_in_file(buffer, "-I", "-D") != 0) || (replace_in_file(buffer, "-A", "-D") != 0))
			logmsg(LOG_WARNING, "unable to substitute -I or -A with -D in FW script for wireguard interface %s!", iface);
		else
			logmsg(LOG_DEBUG, "substitution -I and -A with -D in FW script for wireguard interface %s was done successfully", iface);

		/* remove routing */
		chmod(buffer, (S_IRUSR | S_IWUSR | S_IXUSR));
		system(buffer);

		/* delete routing file */
		eval("rm", "-rf", buffer);
		simple_unlock("firewall");
	}

	eval("ipset", "destroy", wgrouting_mark);

	if (f_exists(dmipset)) {
		/* remove lines with wgroutingXXXX */
		memset(buffer, 0, BUF_SIZE_64);
		snprintf(buffer, BUF_SIZE_64, "wgrouting%s", fwmark);

		if (file_contains(dmipset, buffer)) {
			/* ipset was used on this unit so dnsmasq restart is needed */
			restart_dnsmasq = 1;
			update_dnsmasq_ipset(buffer, NULL, 0);
		}
	}

	/* then, add if needed */
	if (add == 1) {
		modprobe("ip_set");
		modprobe("xt_set");
		modprobe("ip_set_hash_ip");

		logmsg(LOG_INFO, "starting routing policy for wireguard%d - interface %s - table %s", atoi(&iface[2]), iface, fwmark);

		eval("ip", "route", "add", "default", "dev", iface, "table", fwmark);

		priority = getNVRAMVar("wg%d_prio", atoi(&iface[2]));
		memset(buffer, 0, BUF_SIZE_64);
		if (priority[0] == '\0')
			snprintf(buffer, BUF_SIZE_64, "%d", 100 + atoi(&iface[2])); /* default: 100, 101, 102 ... */
		else
			snprintf(buffer, BUF_SIZE_64, "%s", priority);

		eval("ip", "rule", "add", "fwmark", fwmark_mask, "table", fwmark, "priority", buffer);

		wg_init_table(iface, fwmark);

		eval("ipset", "create", wgrouting_mark, "hash:ip");

		wg_build_routing(atoi(&iface[2]), fwmark, fwmark_mask, wgrouting_mark);

		logmsg(LOG_INFO, "completed routing policy configuration for wireguard - interface %s - table %s", iface, fwmark);
	}

	/* restart services on start/stop if it's required */
	if (restart_dnsmasq == 1) {
		stop_dnsmasq();
		start_dnsmasq();
	}
	if (restart_fw == 1)
		restart_firewall();
}

static void wg_route_peer_allowed_ips(const int unit, char *iface, const char *allowed_ips, const char *fwmark, const int add)
{
	char *aip, *aip_orig, *b, *table, *rt, *tp, *ip, *nm;
	int parsed, route_type = 1;
	char buffer[BUF_SIZE_32];
	char table_buf[BUF_SIZE_32];

	memset(table_buf, 0, BUF_SIZE_32);
	table = rt = NULL;

	tp = b = strdup(getNVRAMVar("wg%d_route", unit));
	if (tp) {
		parsed = vstrsep(b, "|", &rt, &table);

		if (!rt || rt[0] == '\0') {
			logmsg(LOG_WARNING, "invalid route format for wg%d: missing routetype in '%s'", unit, tp);
			/* use default route_type = 1, table = NULL */
		}
		else if (parsed == 1) {
			route_type = atoi(rt);
			table = NULL;
		}
		else {
			route_type = atoi(rt);
			strlcpy(table_buf, table, BUF_SIZE_32);
			table = table_buf;
		}
		free(tp);
	}

	logmsg(LOG_DEBUG, "*** %s: routing: iface=[%s] route_type=[%d] table=[%s]", __FUNCTION__, iface, route_type, table);

	/* check which routing type the user specified */
	if (route_type > 0) { /* !off */
		aip = aip_orig = strdup(allowed_ips);
		while (aip && (b = strsep(&aip, ",")) != NULL) {
			memset(buffer, 0, BUF_SIZE_32);
			snprintf(buffer, BUF_SIZE_32, "%s", b);

			if ((vstrsep(b, "/", &ip, &nm) == 2) && (atoi(nm) == 0)) { /* default route */
				if (atoi(getNVRAMVar("wg%d_rgwr", unit)) >= VPN_RGW_POLICY) { /* routing policy+ */
					/* we don't want to mark packets for PBR */
					if (add)
						wg_set_iface_fwmark(iface, "0");

					logmsg(LOG_DEBUG, "*** %s: running wg_routing_policy() iface=[%s] route=[%s] fwmark=[%s] add=[%d]", __FUNCTION__, iface, buffer, fwmark, add);
					wg_routing_policy(iface, buffer, (char *)fwmark, add);
				}
				else {
					logmsg(LOG_DEBUG, "*** %s: running wg_route_peer_default() iface=[%s] route=[%s] fwmark=[%s] add=[%d]", __FUNCTION__, iface, buffer, fwmark, add);
					wg_route_peer_default(iface, buffer, (char *)fwmark, add);
				}
			}
			else { /* std route */
				logmsg(LOG_DEBUG, "*** %s: running wg_route_peer() iface=[%s] route=[%s] table=[%s] add=[%d]", __FUNCTION__, iface, buffer, (route_type == 1 || !table) ? "-" : table, add);
				wg_route_peer(iface, buffer, (route_type == 1) ? NULL : table, add);
			}
		}
		if (aip_orig)
			free(aip_orig);
	}
}

static void wg_set_peer_allowed_ips(char *iface, char *pubkey, char *allowed_ips, const char *fwmark)
{
	if (eval("wg", "set", iface, "peer", pubkey, "allowed-ips", allowed_ips))
		logmsg(LOG_WARNING, "unable to add allowed ips %s for peer %s to wireguard interface %s!", allowed_ips, pubkey, iface);
	else
		logmsg(LOG_DEBUG, "peer %s for wireguard interface %s has had its allowed ips set to %s", pubkey, iface, allowed_ips);
}

static void wg_add_peer(const int unit, char *iface, char *pubkey, char *allowed_ips, const char *presharedkey, char *keepalive, const char *endpoint, const char *fwmark, const char *port)
{
	/* set allowed ips / create peer */
	wg_set_peer_allowed_ips(iface, pubkey, allowed_ips, fwmark);

	/* set peer psk */
	if (presharedkey[0] != '\0')
		wg_set_peer_psk(iface, pubkey, presharedkey);

	/* set peer keepalive */
	if (atoi(keepalive) > 0)
		wg_set_peer_keepalive(iface, pubkey, keepalive);

	/* set peer endpoint */
	if (endpoint[0] != '\0')
		wg_set_peer_endpoint(unit, iface, pubkey, endpoint, port);

	/* add routes (also default route if any) */
	wg_route_peer_allowed_ips(unit, iface, allowed_ips, fwmark, 1); /* 1 = add */
}

static inline int decode_base64(const char src[static 4])
{
	int val = 0;
	unsigned int i;

	for (i = 0; i < 4; ++i)
		val |= (-1
			   + ((((('A' - 1) - src[i]) & (src[i] - ('Z' + 1))) >> 8) & (src[i] - 64))
			   + ((((('a' - 1) - src[i]) & (src[i] - ('z' + 1))) >> 8) & (src[i] - 70))
			   + ((((('0' - 1) - src[i]) & (src[i] - ('9' + 1))) >> 8) & (src[i] + 5))
			   + ((((('+' - 1) - src[i]) & (src[i] - ('+' + 1))) >> 8) & 63)
			   + ((((('/' - 1) - src[i]) & (src[i] - ('/' + 1))) >> 8) & 64)
			) << (18 - 6 * i);
	return val;
}

static inline void encode_base64(char dest[static 4], const uint8_t src[static 3])
{
	const uint8_t input[] = { (src[0] >> 2) & 63, ((src[0] << 4) | (src[1] >> 4)) & 63, ((src[1] << 2) | (src[2] >> 6)) & 63, src[2] & 63 };
	unsigned int i;

	for (i = 0; i < 4; ++i)
		dest[i] = input[i] + 'A'
			  + (((25 - input[i]) >> 8) & 6)
			  - (((51 - input[i]) >> 8) & 75)
			  - (((61 - input[i]) >> 8) & 15)
			  + (((62 - input[i]) >> 8) & 3);
}

static void key_to_base64(char base64[static WG_KEY_LEN_BASE64], const uint8_t key[static WG_KEY_LEN])
{
	unsigned int i;

	for (i = 0; i < WG_KEY_LEN / 3; ++i)
		encode_base64(&base64[i * 4], &key[i * 3]);

	encode_base64(&base64[i * 4], (const uint8_t[]){ key[i * 3 + 0], key[i * 3 + 1], 0 });
	base64[WG_KEY_LEN_BASE64 - 2] = '=';
	base64[WG_KEY_LEN_BASE64 - 1] = '\0';
}

static bool key_from_base64(uint8_t key[static WG_KEY_LEN], const char *base64)
{
	unsigned int i;
	volatile uint8_t ret = 0;
	int val;

	if ((strlen(base64) != WG_KEY_LEN_BASE64 - 1) || (base64[WG_KEY_LEN_BASE64 - 2] != '='))
		return FALSE;

	for (i = 0; i < WG_KEY_LEN / 3; ++i) {
		val = decode_base64(&base64[i * 4]);
		ret |= (uint32_t)val >> 31;
		key[i * 3 + 0] = (val >> 16) & 0xff;
		key[i * 3 + 1] = (val >> 8) & 0xff;
		key[i * 3 + 2] = val & 0xff;
	}
	val = decode_base64((const char[]){ base64[i * 4 + 0], base64[i * 4 + 1], base64[i * 4 + 2], 'A' });
	ret |= ((uint32_t)val >> 31) | (val & 0xff);
	key[i * 3 + 0] = (val >> 16) & 0xff;
	key[i * 3 + 1] = (val >> 8) & 0xff;

	return 1 & ((ret - 1) >> 8);
}

static void wg_pubkey(const char *privkey, char *pubkey)
{
	uint8_t key[WG_KEY_LEN] __attribute__((aligned(sizeof(uintptr_t))));

	key_from_base64(key, privkey);
	curve25519_generate_public(key, key);
	key_to_base64(pubkey, key);
}

static void wg_add_peer_privkey(const int unit, char *iface, const char *privkey, char *allowed_ips, const char *presharedkey, char *keepalive, const char *endpoint, const char *fwmark)
{
	char pubkey[BUF_SIZE_64];

	memset(pubkey, 0, BUF_SIZE_64);
	wg_pubkey(privkey, pubkey);

	wg_add_peer(unit, iface, pubkey, allowed_ips, presharedkey, keepalive, endpoint, fwmark, port);
}

static void wg_remove_peer(const int unit, char *iface, char *pubkey, char *allowed_ips, const char *fwmark)
{
	if (eval("wg", "set", iface, "peer", pubkey, "remove"))
		logmsg(LOG_WARNING, "unable to remove peer %s from wireguard interface %s!", pubkey, iface);
	else
		logmsg(LOG_DEBUG, "peer %s has been removed from wireguard interface %s", pubkey, iface);

	/* remove routes (also default route if any) */
	wg_route_peer_allowed_ips(unit, iface, allowed_ips, fwmark, 0); /* 0 = remove */
}

static void wg_remove_peer_privkey(const int unit, char *iface, char *privkey, char *allowed_ips, const char *fwmark)
{
	char pubkey[BUF_SIZE_64];
	memset(pubkey, 0, BUF_SIZE_64);

	wg_pubkey(privkey, pubkey);

	wg_remove_peer(unit, iface, pubkey, allowed_ips, fwmark);
}

static int wg_set_iface_down(char *iface)
{
	/* check if interface exists */
	if (wg_if_exist(iface)) {
		if (eval("ip", "link", "set", "down", "dev", iface)) {
			logmsg(LOG_WARNING, "failed to bring down wireGuard interface %s", iface);
			return -1;
		}
		else
			logmsg(LOG_DEBUG, "wireguard interface %s has been brought down", iface);
	}

	return 0;
}

static int wg_remove_iface(char *iface)
{
	/* check if interface exists */
	if (wg_if_exist(iface)) {
		/* delete wireguard interface */
		if (eval("ip", "link", "del", "dev", iface)) {
			logmsg(LOG_WARNING, "unable to delete wireguard interface %s!", iface);
			return -1;
		}
		logmsg(LOG_DEBUG, "wireguard interface %s has been deleted", iface);
		return 0;
	}

	/* interface doesn't exist - not an error, just log */
	logmsg(LOG_DEBUG, "no such interface: %s", iface);

	return 0;
}

void start_wg_eas(void)
{
	int unit;
	int externalall_mode = 0;

	for (unit = 0; unit < WG_INTERFACE_MAX; unit++) {
		if (atoi(getNVRAMVar("wg%d_enable", unit)) == 1) {
			if (atoi(getNVRAMVar("wg%d_com", unit)) == 3 && atoi(getNVRAMVar("wg%d_rgwr", unit)) == VPN_RGW_ALL) { /* check for 'External - VPN Provider' mode with "Redirect Internet traffic" set to "All" on this unit */
				if (externalall_mode == 0) { /* no previous unit is in this mode - allow */
					start_wireguard(unit);
					externalall_mode++;
				}
				else
					logmsg(LOG_WARNING, "only one wireguard instance can be run in 'External - VPN Provider' mode with 'Redirect Internet traffic' set to 'All' (currently up: wg%d)! Aborting ...", unit);
			}
			else
				start_wireguard(unit);
		}
	}
}
/*
void stop_wg_eas(void)
{
	int unit;

	for (unit = 0; unit < WG_INTERFACE_MAX; unit++) {
		stop_wireguard(unit);
	}
}
*/
void stop_wg_all(void)
{
	int unit;
	char iface[IF_SIZE];

	for (unit = 0; unit < WG_INTERFACE_MAX; unit++) {
		memset(iface, 0, IF_SIZE);
		snprintf(iface, IF_SIZE, "wg%d", unit);
		if (wg_if_exist(iface))
			stop_wireguard(unit);
		else
			logmsg(LOG_DEBUG, "no such wg instance to stop: %s", iface);
	}
	wg_cleanup_dirs();

	/* remove tunnel interface module */
	modprobe_r("wireguard");
}

void start_wireguard(const int unit)
{
	char *nv, *nvp, *rka, *b;
	char *priv, *name, *key, *psk, *ip, *ka, *aip, *ep;
	char iface[IF_SIZE];
	char buffer[BUF_SIZE];
	int mode;

	memset(buffer, 0, BUF_SIZE);
	snprintf(buffer, BUF_SIZE, "wireguard%d", unit);
	if (serialize_restart(buffer, 1))
		return;

	/* determine interface */
	memset(iface, 0, IF_SIZE);
	snprintf(iface, IF_SIZE, "wg%d", unit);

	/* prepare port value */
	wg_find_port(unit, port);

	/* set up directories for later use */
	wg_setup_dirs();

	/* check if file is specified */
	if (getNVRAMVar("wg%d_file", unit)[0] != '\0') {
		if (wg_quick_iface(iface, getNVRAMVar("wg%d_file", unit), 1))
			goto out;
	}
	else {
		/* create interface */
		if (wg_create_iface(iface))
			goto out;

		/* set interface address */
		if (wg_set_iface_addr(iface, getNVRAMVar("wg%d_ip", unit)))
			goto out;

		/* set interface port */
		if (wg_set_iface_port(iface, port))
			goto out;

		/* set interface private key */
		if (wg_set_iface_privkey(iface, getNVRAMVar("wg%d_key", unit)))
			goto out;

		/* set interface fwmark */
		wg_find_fwmark(unit, port, fwmark);
		if (wg_set_iface_fwmark(iface, fwmark))
			goto out;

		/* set interface mtu */
		if (wg_set_iface_mtu(iface, getNVRAMVar("wg%d_mtu", unit)))
			goto out;

		/* check for keepalives from the router */
		rka = getNVRAMVar("wg%d_ka", unit);

		/* bring up interface */
		wg_iface_pre_up(unit);
		if (wg_set_iface_up(iface))
			goto out;

		/* add stored peers */
		nvp = nv = strdup(getNVRAMVar("wg%d_peers", unit));
		if (nv) {
			mode = atoi(getNVRAMVar("wg%d_com", unit));

			while ((b = strsep(&nvp, ">")) != NULL) {
				if (vstrsep(b, "<", &priv, &name, &ep, &key, &psk, &ip, &aip, &ka) < 8)
					continue;

				/* build peer allowed ips */
				memset(buffer, 0, BUF_SIZE);
				if (aip[0] == '\0')
					snprintf(buffer, BUF_SIZE, "%s", ip);
				else if (ip[0] == '\0')
					snprintf(buffer, BUF_SIZE, "%s", aip);
				else
					snprintf(buffer, BUF_SIZE, "%s,%s", ip, aip);

				/* add peer to interface (and route) */
				if (priv[0] == '1') /* peer has private key? */
					wg_add_peer_privkey(unit, iface, key, buffer, psk, (mode == 3 ? ka : rka), ep, fwmark);
				else
					wg_add_peer(unit, iface, key, buffer, psk, (mode == 3 ? ka : rka), ep, fwmark, port);
			}
			free(nv);
		}

		eval("ip", "route", "flush", "cache");

		/* run post up scripts */
		wg_iface_post_up(unit);
	}

	/* create firewall script & DNS rules */
	wg_build_firewall(unit, port);

	/* firewall + dns rules */
	memset(buffer, 0, BUF_SIZE);
	snprintf(buffer, BUF_SIZE, WG_FW_DIR"/%s-fw.sh", iface);

	/* first remove existing firewall rule(s) */
	simple_lock("firewall");
	run_del_firewall_script(buffer, WG_DIR_DEL_SCRIPT);

	/* then add firewall rule(s) */
	if (eval(buffer))
		logmsg(LOG_WARNING, "unable to add iptable rules for wireguard interface %s on port %s!", iface, port);
	else
		logmsg(LOG_DEBUG, "iptable rules have been added for wireguard interface %s on port %s", iface, port);

	/* the same for routing rule(s) file, if exists */
	memset(buffer, 0, BUF_SIZE);
	snprintf(buffer, BUF_SIZE, WG_FW_DIR"/%s-fw-routing.sh", iface);
	if (f_exists(buffer)) {
		/* first remove all existing routing rule(s) */
		run_del_firewall_script(buffer, WG_DIR_DEL_SCRIPT);

		if (eval(buffer))
			logmsg(LOG_WARNING, "unable to add route rules for wireguard interface %s on port %s!", iface, port);
		else
			logmsg(LOG_DEBUG, "route rules have been added for wireguard interface %s on port %s", iface, port);
	}
	simple_unlock("firewall");

	wg_setup_watchdog(unit);

	logmsg(LOG_INFO, "wireguard (%s) started", iface);

	return;

out:
	stop_wireguard(unit);
}

void stop_wireguard(const int unit)
{
	char *nv, *nvp, *b;
	char *priv, *name, *key, *psk, *ip, *ka, *aip, *ep;
	char iface[IF_SIZE];
	char buffer[BUF_SIZE];
	int is_dev;

	memset(buffer, 0, BUF_SIZE);
	snprintf(buffer, BUF_SIZE, "wireguard%d", unit);
	if (serialize_restart(buffer, 0))
		return;

	/* remove cron job */
	memset(buffer, 0, BUF_SIZE);
	snprintf(buffer, BUF_SIZE, "CheckWireguard%d", unit);
	eval("cru", "d", buffer);

	/* remove watchdog file */
	memset(buffer, 0, BUF_SIZE);
	snprintf(buffer, BUF_SIZE, WG_SCRIPTS_DIR"/watchdog%d.sh", unit);
	eval("rm", "-rf", buffer);

	/* determine interface */
	memset(iface, 0, IF_SIZE);
	snprintf(iface, IF_SIZE, "wg%d", unit);

	is_dev = wg_if_exist(iface);

	if (getNVRAMVar("wg%d_file", unit)[0] != '\0')
		wg_quick_iface(iface, getNVRAMVar("wg%d_file", unit), 0);
	else {
		/* prepare port value */
		wg_find_port(unit, port);

		/* prepare fwmark value */
		wg_find_fwmark(unit, port, fwmark);

		wg_iface_pre_down(unit);

		/* remove peers */
		nvp = nv = strdup(getNVRAMVar("wg%d_peers", unit));
		if (nv) {
			while ((b = strsep(&nvp, ">")) != NULL) {
				if (vstrsep(b, "<", &priv, &name, &ep, &key, &psk, &ip, &aip, &ka) < 8)
					continue;

				/* build peer allowed ips */
				memset(buffer, 0, BUF_SIZE);
				if (aip[0] == '\0')
					snprintf(buffer, BUF_SIZE, "%s", ip);
				else if (ip[0] == '\0')
					snprintf(buffer, BUF_SIZE, "%s", aip);
				else
					snprintf(buffer, BUF_SIZE, "%s,%s", ip, aip);

				/* remove peer from interface / remove routing */
				if (priv[0] == '1') /* peer has private key? */
					wg_remove_peer_privkey(unit, iface, key, buffer, fwmark);
				else
					wg_remove_peer(unit, iface, key, buffer, fwmark);
			}
			free(nv);
		}

		eval("ip", "rule", "delete", "table", fwmark, "fwmark", fwmark);
		eval("ip", "route", "flush", "table", fwmark);
		eval("ip", "route", "flush", "cache");

		/* remove interface */
		wg_set_iface_down(iface);
		wg_remove_iface(iface);
		wg_iface_post_down(unit);
	}

	/* remove firewall rules */
	simple_lock("firewall");
	memset(buffer, 0, BUF_SIZE);
	snprintf(buffer, BUF_SIZE, WG_FW_DIR"/%s-fw.sh", iface);
	run_del_firewall_script(buffer, WG_DIR_DEL_SCRIPT);
	eval("rm", "-rf", buffer);
	simple_unlock("firewall");

	if (is_dev)
		logmsg(LOG_INFO, "wireguard (%s) stopped", iface);
}

void write_wg_dnsmasq_config(FILE* f)
{
	char buf[BUF_SIZE], device[BUF_SIZE_32];
	char *pos, *fn;
	DIR *dir;
	struct dirent *file;
	int cur;

	/* add interface(s) to dns config */
	strlcpy(buf, nvram_safe_get("wg_adns"), BUF_SIZE);
	for (pos = strtok(buf, ","); pos != NULL; pos = strtok(NULL, ",")) {
		cur = atoi(pos);
		if (cur || cur == 0) {
			logmsg(LOG_DEBUG, "*** %s: adding server wg%d interface to Dnsmasq config", __FUNCTION__, cur);
			fprintf(f, "interface=wg%d\n", cur);
		}
	}

	dir = opendir(WG_DNS_DIR);
	if (!dir)
		return;

	while ((file = readdir(dir)) != NULL) {
		fn = file->d_name;

		if (fn[0] == '.')
			continue;

		if (sscanf(fn, "%s.conf", device) == 1) {
			memset(buf, 0, BUF_SIZE);
			snprintf(buf, BUF_SIZE, "%s/%s", WG_DNS_DIR, fn);
			if (fappend(f, buf) == -1) {
				logmsg(LOG_WARNING, "fappend failed for %s (%s)", buf, strerror(errno));
				continue;
			}

			logmsg(LOG_DEBUG, "*** %s: adding Dnsmasq config from %s", __FUNCTION__, fn);
		}
	}
	closedir(dir);
}
