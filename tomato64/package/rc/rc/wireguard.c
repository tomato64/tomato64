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
#include <stdarg.h>
#include <libgen.h>

/* needed by logmsg() */
#define LOGMSG_DISABLE		DISABLE_SYSLOG_OSM
#define LOGMSG_NVDEBUG		"wireguard_debug"

#define IF_SIZE			8
#define PEER_COUNT		3
#define MAX_LINE		1024
#define WG_SCRIPT_START		0
#define WG_SCRIPT_STOP		1
/* uncomment to add default routing (also in patches/wireguard-tools/101-tomato-specific.patch line 412 - 414) after kernel fix */
#define KERNEL_WG_FIX


/* interfaces that we want to ignore in standard PRB mode */
static const char *vpn_ifaces[] = { "wg0", "wg1", "wg2", "tun11", "tun12", "tun13", NULL };

/* structure for storing a dynamic array of domains */
typedef struct {
	char **domains;
	int count;
	int capacity;
} domain_list_t;

/* per-unit WG routing script context */
typedef struct {
	FILE *start_fp;
	char start_path[BUF_SIZE_64];
	char stop_path[BUF_SIZE_64];

	char stop_buf[8192];
	size_t stop_len;

	char routing_ipset_name[BUF_SIZE_16];

	char port[BUF_SIZE_8];
	char fwmark[BUF_SIZE_16];

	int policy_configured;
	int default_configured;
	char epilog_buf[512];
} wg_script_ctx_t;

static wg_script_ctx_t wg_script_ctx[WG_INTERFACE_MAX];
unsigned int restart_dnsmasq = 0;
unsigned int restart_fw = 0;
const char *pid_path = WG_DIR"/child_wg%d.pid";


static int wg_execute_script(const char *path)
{
	if (!f_exists(path))
		return -1;

	return eval("sh", (char *)path);
}

/* initialize start/stop scripts */
static int wg_script_init(const int unit)
{
	wg_script_ctx_t *ctx = &wg_script_ctx[unit];

	memset(ctx, 0, sizeof(wg_script_ctx_t));
	snprintf(ctx->start_path, sizeof(ctx->start_path), WG_SCRIPTS_DIR"/wg%d-start.sh", unit);
	snprintf(ctx->stop_path, sizeof(ctx->stop_path), WG_SCRIPTS_DIR"/wg%d-stop.sh", unit);

	/* START file */
	if (!(ctx->start_fp = fopen(ctx->start_path, "w"))) {
		logmsg(LOG_ERR, "cannot open file for writing: %s (%s)", ctx->start_path, strerror(errno));
		return -1;
	}

	/* START header */
	fprintf(ctx->start_fp, "#!/bin/sh\n\n"
	                       "set -e\n\n"
	                       "WG_TAG=\"wg%d-start.sh\"\n"
	                       "run_cmd() {\n"
	                       " set +e\n"
	                       " output=$(\"$@\" 2>&1)\n"
	                       " rc=$?\n"
	                       " set -e\n"
	                       " if [ $rc -ne 0 ]; then\n"
	                       "  case \"$output\" in\n"
	                       "   *\"File exists\"*|*\"already exists\"*)\n"
	                       "    logger -p WARNING -t $WG_TAG \"ignored: $*\"\n"
	                       "    return 0\n"
	                       "    ;;\n"
	                       "   *)\n"
	                       "    logger -p ERROR -t $WG_TAG \"command failed: $* rc=$rc msg=$output\"\n"
	                       "    return $rc\n"
	                       "    ;;\n"
	                       "  esac\n"
	                       " fi\n",
	                       unit);
	if (nvram_get_int("wireguard_debug")) {
		fprintf(ctx->start_fp, " logger -p DEBUG -t $WG_TAG \"command added: $*\"\n");
	}
	fprintf(ctx->start_fp, "}\n\n");

	/* STOP uses buffer only */
	ctx->stop_len = 0;

	return 0;
}

/* add a line to start/stop scripts */
static void wg_script_add(const int unit, const int which, const char *fmt, ...)
{
	wg_script_ctx_t *ctx = &wg_script_ctx[unit];
	char line[BUF_SIZE];
	va_list ap;
	size_t len;

	if (!ctx->start_fp) /* wg-quick */
		return;

	va_start(ap, fmt);
	vsnprintf(line, BUF_SIZE, fmt, ap);
	va_end(ap);

	if (which == WG_SCRIPT_START) {
		fprintf(ctx->start_fp, "%s\n", line);
	}
	else if (which == WG_SCRIPT_STOP) {
		len = strlen(line);

		if (ctx->stop_len + len + 1 >= sizeof(ctx->stop_buf)) {
			logmsg(LOG_ERR, "wg%d STOP buffer overflow! used=%zu/%zu tried=%zu -- stop.sh will be INCOMPLETE, routing cleanup may fail!", unit, ctx->stop_len, sizeof(ctx->stop_buf), len);
			return;
		}

		/* shift entire existing buffer forward */
		memmove(ctx->stop_buf + len + 1, ctx->stop_buf, ctx->stop_len + 1);
		ctx->stop_buf[ctx->stop_len + len + 1] = '\0';

		/* write new line at beginning */
		memcpy(ctx->stop_buf, line, len);
		ctx->stop_buf[len] = '\n';

		ctx->stop_len += len + 1;
	}
}

/* apply start/stop scripts */
static int wg_script_apply(const int unit)
{
	wg_script_ctx_t *ctx = &wg_script_ctx[unit];
	size_t written;
	FILE *fp;

	if (!ctx->start_fp) /* wg-quick */
		return 0;

	/* add epilog */
	if (ctx->epilog_buf[0] != '\0')
		fprintf(ctx->start_fp, "%s", ctx->epilog_buf);

	/* close START */
	fsync(fileno(ctx->start_fp));
	fclose(ctx->start_fp);
	chmod(ctx->start_path, S_IRUSR | S_IWUSR | S_IXUSR);
	ctx->start_fp = NULL;

	/* write STOP file */
	if ((fp = fopen(ctx->stop_path, "w"))) {
		fprintf(fp, "#!/bin/sh\n\n"
		            "set +e\n"
		            "WG_TAG=\"wg%d-stop.sh\"\n"
		            "run_cmd() {\n"
		            " \"$@\"\n"
		            " if [ $? -ne 0 ]; then\n"
		            "  logger -p ERROR -t $WG_TAG command failed: $*\n",
		            unit);
		if (nvram_get_int("wireguard_debug")) {
			fprintf(fp, " else\n"
			            "  logger -p DEBUG -t $WG_TAG command added: $*\n");
		}
		fprintf(fp, " fi\n"
		            "}\n\n");

		/* STOP commands (already reversed in buffer) */
		written = fwrite(ctx->stop_buf, 1, ctx->stop_len, fp);
		if (written != ctx->stop_len)
			logmsg(LOG_ERR, "wg%d: failed writing stop script (%zu/%zu)", unit, written, ctx->stop_len);

		fsync(fileno(fp));
		fclose(fp);
		chmod(ctx->stop_path, S_IRUSR | S_IWUSR | S_IXUSR);
		logmsg(LOG_DEBUG, "wg%d: generated %zu bytes stop script", unit, ctx->stop_len);
	}
	else
		logmsg(LOG_ERR, "wg%d: cannot open stop script %s", unit, ctx->stop_path);

	/* execute START */
	return wg_execute_script(ctx->start_path);
}

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
static void add_domain(domain_list_t *list, const char *domain)
{
	char **newdomains;
	int newcap;

	if (!domain || !*domain)
		return;

	/* normalize dnsmasq wildcard format (.example.com -> example.com) */
	if (domain[0] == '.')
		domain++;

	/* check if increase the array size is needed */
	if (list->count >= list->capacity - 1) { /* -1 for NULL at the end */
		newcap = list->capacity * 2;
		newdomains = realloc(list->domains, newcap * sizeof(char*));
		if (!newdomains) {
			logmsg(LOG_ERR, "%s: realloc failed (capacity=%d, domain='%s') - skipping domain (out of memory)", __FUNCTION__, newcap, domain);
			return;
		}
		list->domains = newdomains;
		list->capacity = newcap;
		logmsg(LOG_DEBUG, "%s: capacity increased to %d", __FUNCTION__, list->capacity);
	}

	/* allocate memory for the new domain */
	list->domains[list->count] = (char*)malloc((strlen(domain) + 1) * sizeof(char));
	if (!list->domains[list->count]) {
		logmsg(LOG_ERR, "%s: malloc failed for domain '%s' (out of memory) - skipping", __FUNCTION__, domain);
		return;
	}

	/* copy domain */
	strlcpy(list->domains[list->count], domain, strlen(domain) + 1);
	list->count++;

	if (list->count < list->capacity)
		list->domains[list->count] = NULL;
}

/* add/remove domains in dnsmasq config file */
static void update_dnsmasq_ipset(const char *tag, domain_list_t *list, const int add)
{
	FILE *fp_read = NULL, *fp_write = NULL;
	char line[MAX_LINE], new_line[MAX_LINE], line_backup[MAX_LINE];
	char temp_file[BUF_SIZE_64], dirbuf[BUF_SIZE_64], domain_entry[BUF_SIZE_128];
	char *pos, *tag_pos, *saveptr;
	int found, tag_found, i, domain_count = 0, truncated, dfd;
	int *domain_seen = NULL;

	if (!tag || !*tag)
		return;

	/* list and domains are required only when adding entries */
	if (add) {
		if ((!list) || (!list->domains) || (list->count == 0))
			return;

		domain_count = list->count;
		domain_seen = calloc(domain_count, sizeof(int));
		if (!domain_seen) {
			logmsg(LOG_ERR, "domain_seen allocation failed");
			return;
		}
	}

	/* lock */
	simple_lock("dnsmasq");

	if (!f_exists(dmipset))
		f_write(dmipset, NULL, 0, 0, 0);

	if (!(fp_read = fopen(dmipset, "r"))) {
		logmsg(LOG_ERR, "cannot open file for reading: %s (%s)", dmipset, strerror(errno));
		goto cleanup;
	}

	/* create temporary file path */
	snprintf(temp_file, BUF_SIZE_64, "%s.tmp", dmipset);
	if (!(fp_write = fopen(temp_file, "w"))) {
		logmsg(LOG_ERR, "cannot open file for writing: %s (%s)", temp_file, strerror(errno));
		goto cleanup;
	}

	/* process existing file */
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
		memset(domain_entry, 0, BUF_SIZE_128);
		strlcpy(domain_entry, line + 7, BUF_SIZE_128);

		/* normalize dnsmasq wildcard entries (.example.com -> example.com) */
		if (domain_entry[0] == '.')
			memmove(domain_entry, domain_entry + 1, strlen(domain_entry) + 1);

		*pos = '/';

		/* note: found is intentionally not checked when removing (add=0).
		 * tag is removed from ALL matching lines in the file, regardless of
		 * the domain list. list/domains are used only to scope additions,
		 * not removals - removing a tag cleans it globally across all domains.
		 */
		found = 0;
		if (add && list->domains) {
			/* no rewind needed — list->domains is an in-memory array */
			for (i = 0; i < domain_count; i++) {
				if (strcmp(domain_entry, list->domains[i]) == 0) {
					found = 1;
					domain_seen[i] = 1;
					break;
				}
			}
		}

		/* look for our tag in the tags part */
		tag_pos = pos + 1;
		tag_found = 0;

		/* backup original line before strtok_r destroys it */
		strlcpy(line_backup, line, MAX_LINE);

		/* create a copy to work with */
		truncated = 0;
		memset(new_line, 0, MAX_LINE);
		if ((strlcpy(new_line, "ipset=/", MAX_LINE) >= MAX_LINE) || (strlcat(new_line, domain_entry, MAX_LINE) >= MAX_LINE) || (strlcat(new_line, "/", MAX_LINE) >= MAX_LINE)) {
			logmsg(LOG_WARNING, "ipset line too long for domain: %s, keeping original", domain_entry);
			fprintf(fp_write, "%s\n", line_backup);
			continue;
		}

		/* parse and rebuild tags */
		pos = strtok_r(tag_pos, ",", &saveptr);
		while (pos && !truncated) {
			while (*pos == ' ')
				pos++;

			if (strcmp(pos, tag) == 0) {
				tag_found = 1;
				if (!add) {
					/* intentional: remove tag globally, not scoped to domain list */
					pos = strtok_r(NULL, ",", &saveptr);
					continue;
				}
			}

			/* add tag to new line */
			if (strlen(new_line) > strlen("ipset=/") + strlen(domain_entry) + 1) {
				if (strlcat(new_line, ",", MAX_LINE) >= MAX_LINE) {
					logmsg(LOG_WARNING, "ipset line too long for domain: %s, keeping original", domain_entry);
					fprintf(fp_write, "%s\n", line_backup);
					truncated = 1;
					break;
				}
			}

			if (strlcat(new_line, pos, MAX_LINE) >= MAX_LINE) {
				logmsg(LOG_WARNING, "ipset line too long for domain: %s, keeping original", domain_entry);
				fprintf(fp_write, "%s\n", line_backup);
				truncated = 1;
				break;
			}

			pos = strtok_r(NULL, ",", &saveptr);
		}

		if (truncated)
			continue;

		/* add our tag if adding and not found, and this domain is in our list */
		if (add && found && !tag_found) {
			if (strlen(new_line) > strlen("ipset=/") + strlen(domain_entry) + 1) {
				if (strlcat(new_line, ",", MAX_LINE) >= MAX_LINE) {
					logmsg(LOG_WARNING, "ipset line too long for domain: %s, keeping original", domain_entry);
					fprintf(fp_write, "%s\n", line_backup);
					continue;
				}
			}

			if (strlcat(new_line, tag, MAX_LINE) >= MAX_LINE) {
				logmsg(LOG_WARNING, "ipset line too long for domain: %s, keeping original", domain_entry);
				fprintf(fp_write, "%s\n", line_backup);
				continue;
			}
		}

		/* write line if it has tags after the domain */
		if (strlen(new_line) > strlen("ipset=/") + strlen(domain_entry) + 1)
			fprintf(fp_write, "%s\n", new_line);
	}

	/* add new entries for domains not found in file */
	if (add && list->domains) {
		for (i = 0; i < domain_count; i++) {
			if (!domain_seen[i])
				/* add new entry if domain not found */
				fprintf(fp_write, "ipset=/%s/%s\n", list->domains[i], tag);
		}
	}

	/* replace original file with temporary file */
	if (rename(temp_file, dmipset) != 0) {
		logmsg(LOG_ERR, "cannot rename file: %s (%s)", dmipset, strerror(errno));
		unlink(temp_file);
	}
	else {
		strlcpy(dirbuf, dmipset, BUF_SIZE_64);
		dfd = open(dirname(dirbuf), O_DIRECTORY);
		if (dfd >= 0) {
			fsync(dfd);
			close(dfd);
		}
	}

cleanup:
	if (fp_read)
		fclose(fp_read);
	if (fp_write) {
		fsync(fileno(fp_write));
		fclose(fp_write);
	}

	free(domain_seen);

	/* unlock */
	simple_unlock("dnsmasq");
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
		logmsg(LOG_ERR, "cannot open file for reading: %s (%s)", filename, strerror(errno));
		return -1;
	}

	snprintf(tmp_filename, FILENAME_MAX, "%s.tmp", filename);
	if (!(fp_out = fopen(tmp_filename, "w"))) {
		logmsg(LOG_ERR, "could not create temporary file: %s (%s)", tmp_filename, strerror(errno));
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
		logmsg(LOG_ERR, "failed to overwrite %s: %s", filename, strerror(errno));
		unlink(tmp_filename);
		return -1;
	}

	return 0;
}

static int wg_detect_routing_mode(const int unit, const int is_default_route)
{
	int rgwr;

	rgwr = atoi(getNVRAMVar("wg%d_rgwr", unit));

	if (!is_default_route)
		return 0; /* standard */

	if (rgwr == VPN_RGW_ALL)
		return 1; /* default */

	if (rgwr == VPN_RGW_POLICY)
		return 2; /* policy */

	if (rgwr == VPN_RGW_POLICY_STRICT)
		return 3; /* strict */

	return 0;
}

static void wg_build_firewall(const int unit)
{
	FILE *fp;
	wg_script_ctx_t *ctx = &wg_script_ctx[unit];
	char buffer[BUF_SIZE_64];
	char tmp[BUF_SIZE_16];
	char *dns;
	int nvi;

	snprintf(buffer, BUF_SIZE_64, WG_FW_DIR"/wg%d-fw.sh", unit);

	/* script with firewall rules (port, unit) */
	/* (..., open wireguard port, accept packets from wireguard internal subnet, set up forwarding) */
	if ((fp = fopen(buffer, "w"))) {
		chains_log_detection();

		fprintf(fp, "#!/bin/sh\n"
		            "\n# FW\n"
		            "STATUS=0\n"
		            "run() {\n"
		            " \"$@\" || STATUS=1\n"
		            "}\n");

		nvi = atoi(getNVRAMVar("wg%d_fw", unit));

		/* Handle firewall rules if appropriate */
		snprintf(tmp, BUF_SIZE_16, "wg%d_firewall", unit);
		if (atoi(getNVRAMVar("wg%d_com", unit)) == 3 && !nvram_contains_word(tmp, "custom")) { /* 'External - VPN Provider' & auto */
			fprintf(fp, "run iptables -I INPUT -i wg%d -m state --state NEW -j %s\n"
			            "run iptables -I FORWARD -i wg%d -m state --state NEW -j %s\n"
			            "run iptables -I FORWARD -o wg%d -j ACCEPT\n"
			            "echo 1 > /proc/sys/net/ipv4/conf/all/src_valid_mark\n",
			            unit, (nvi ? chain_in_drop : chain_in_accept),
			            unit, (nvi ? "DROP" : "ACCEPT"),
			            unit);

			/* masquerade all peer outbound traffic regardless of source subnet */
			if (atoi(getNVRAMVar("wg%d_nat", unit)) == 1)
				fprintf(fp, "run iptables -t nat -I POSTROUTING -o wg%d -j MASQUERADE\n", unit);

			if (atoi(getNVRAMVar("wg%d_rgwr", unit)) >= VPN_RGW_POLICY) {
				/* Disable rp_filter when in policy mode */
				fprintf(fp, "echo 0 > /proc/sys/net/ipv4/conf/wg%d/rp_filter\n"
				            "echo 0 > /proc/sys/net/ipv4/conf/all/rp_filter\n",
				            unit);
			}
		}
		else if (atoi(getNVRAMVar("wg%d_com", unit)) != 3) { /* other */
			fprintf(fp, "run iptables -t nat -I PREROUTING -p udp --dport %s -j ACCEPT\n"
			            "run iptables -I INPUT -p udp --dport %s -j %s\n"
			            "run iptables -I INPUT -i wg%d -j %s\n"
			            "run iptables -I FORWARD -i wg%d -j ACCEPT\n",
			            ctx->port,
			            ctx->port, chain_in_accept,
			            unit, chain_in_accept,
			            unit);
		}

		if (!nvram_get_int("ctf_disable")) { /* bypass CTF if enabled */
			fprintf(fp, "run iptables -t mangle -I PREROUTING -i wg%d -j MARK --set-mark 0x01/0x7\n"
			            "run iptables -t mangle -I POSTROUTING -o wg%d -j MARK --set-mark 0x01/0x7\n",
			            unit, unit);
#ifdef TCONFIG_IPV6
			if (ipv6_enabled()) {
				fprintf(fp, "run ip6tables -t mangle -I PREROUTING -i wg%d -j MARK --set-mark 0x01/0x7\n"
				            "run ip6tables -t mangle -I POSTROUTING -o wg%d -j MARK --set-mark 0x01/0x7\n",
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
			            "\niptables -nL \"$DNS_CHAIN\" &>/dev/null && {\n"
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
			            " }\n"
			            "}\n",
			            unit,
			            WG_DNS_DIR, unit,
			            dns,
			            dns,
			            unit,
			            unit);
		}
		fprintf(fp, "exit \"${STATUS:-0}\"\n");
		fclose(fp);
		chmod(buffer, (S_IRUSR | S_IWUSR | S_IXUSR));
	}
}

static void wg_build_routing(const int unit, const char *fwmark_mask, const char *wgrouting_mark)
{
	wg_script_ctx_t *ctx = &wg_script_ctx[unit];
	domain_list_t my_domains;
	FILE *fp;
	char *enable, *type, *value, *kswitch;
	char *nv, *nvp, *b;
	char buffer[BUF_SIZE_64];
	int policy, rules_count;

	snprintf(buffer, BUF_SIZE_64, WG_FW_DIR"/wg%d-fw-routing.sh", unit);

	if (!(fp = fopen(buffer, "w"))) {
		logmsg(LOG_ERR, "cannot open file for writing: %s (%s)", buffer, strerror(errno));
		return;
	}

	/* script with routing policy rules */
	fprintf(fp, "#!/bin/sh\n"
	            "\n# Routing\n"
	            "iptables -t mangle -A PREROUTING -m set --match-set %s dst,src -j MARK --set-mark %s\n",
	            wgrouting_mark, fwmark_mask);

	if (init_domain_list(&my_domains) != 0) {
		logmsg(LOG_ERR, "cannot initialize domain list");
		fclose(fp);
		eval("rm", "-f", buffer);
		return;
	}

	logmsg(LOG_INFO, "start adding routing rules for wg%d (if any) ...", unit);
	rules_count = 0;

	/* example of routing_val: 1<2<8.8.8.8<1>1<1<1.2.3.4<0>1<3<domain.com<0> (enabled<type<domain_or_IP<kill_switch>) */
	nv = nvp = strdup(getNVRAMVar("wg%d_routing_val", unit));
	if (!nv) {
		logmsg(LOG_ERR, "%s: strdup failed for wg%d routing_val (out of memory)", __FUNCTION__, unit);
		fclose(fp);
		eval("rm", "-f", buffer);
		free_domain_list(&my_domains);
		return;
	}
	while ((b = strsep(&nvp, ">")) != NULL) {
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

			rules_count++;
			break;
		case 2: /* to destination */
			logmsg(LOG_INFO, "type: %d - add %s (wg%d)", policy, value, unit);
			fprintf(fp, "iptables -t mangle -A PREROUTING -d %s -j MARK --set-mark %s\n", value, fwmark_mask);
			rules_count++;
			break;
		case 3: /* to domain */
			logmsg(LOG_INFO, "type: %d - add %s (wg%d)", policy, value, unit);
			add_domain(&my_domains, value);
			restart_dnsmasq = 1;
			rules_count++;
			break;
		default:
			continue;
		}
	}
	if (nv)
		free(nv);

	if (rules_count > 0)
		logmsg(LOG_INFO, "added %d routing rule(s) for wg%d", rules_count, unit);

	fclose(fp);
	chmod(buffer, (S_IRUSR | S_IWUSR | S_IXUSR));

	if (my_domains.count) {
		strlcpy(ctx->routing_ipset_name, wgrouting_mark, sizeof(ctx->routing_ipset_name));
		logmsg(LOG_DEBUG, "wg%d: created ipset '%s' with %d domain(s)", unit, wgrouting_mark, my_domains.count);
		update_dnsmasq_ipset(ctx->routing_ipset_name, &my_domains, 1);
	}
	free_domain_list(&my_domains);

	restart_fw = 1;
}

static int wg_quick_iface(char *iface, const char *file, const int up)
{
	char buffer[BUF_SIZE_32];
	char *up_down = (up == 1 ? "up" : "down");
	FILE *fp;

	/* copy config to wg dir with proper name */
	snprintf(buffer, BUF_SIZE_32, WG_DIR"/%s.conf", iface);

	if (!(fp = fopen(buffer, "w"))) {
		logmsg(LOG_ERR, "unable to open wireguard configuration file %s for interface %s!", file, iface);
		return -1;
	}

	if (fappend(fp, file) < 0) {
		logmsg(LOG_ERR, "fappend failed for %s", buffer);
		fclose(fp);
		return -1;
	}
	fclose(fp);

	/* set up/down wireguard IF */
	if (eval("wg-quick", up_down, buffer, iface, "--norestart")) {
		logmsg(LOG_ERR, "unable to set %s wireguard interface %s from file %s!", up_down, iface, file);
		return -1;
	}
	else
		logmsg(LOG_DEBUG, "wireguard interface %s from file %s set %s", iface, file, up_down);

	restart_dnsmasq = 1;

	return 0;
}

static void wg_set_port(const int unit)
{
	wg_script_ctx_t *ctx = &wg_script_ctx[unit];
	char *b;

	b = getNVRAMVar("wg%d_port", unit);
	if (b[0] == '\0')
		snprintf(ctx->port, sizeof(ctx->port), "%d", 51820 + unit);
	else
		snprintf(ctx->port, sizeof(ctx->port), "%s", b);
}

static void wg_set_fwmark(const int unit)
{
	wg_script_ctx_t *ctx = &wg_script_ctx[unit];
	char *b;

	/* ensure port initialized first */
	if (!ctx->port[0])
		wg_set_port(unit);

	b = getNVRAMVar("wg%d_fwmark", unit);
	/* fwmark=0 disables mark in WG */
	if ((b[0] == '\0') || (b[0] == '0'))
		snprintf(ctx->fwmark, sizeof(ctx->fwmark), "%s", ctx->port);
	else
		snprintf(ctx->fwmark, sizeof(ctx->fwmark), "%s", b);
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
	const char *val, *ipchk;
	int nvi;

	if ((nvi = atoi(getNVRAMVar("wg%d_poll", unit))) > 0) {
		snprintf(buffer, BUF_SIZE_64, WG_SCRIPTS_DIR"/watchdog-wg%d.sh", unit);

		val = getNVRAMVar("wg%d_tunchk", unit);
		ipchk = (val && *val) ? val : nvram_safe_get("wan_checker");

		if ((fp = fopen(buffer, "w"))) {
			fprintf(fp, "#!/bin/sh\n"
			            "pingme() {\n"
			            " [ \"%d\" = \"0\" ] && return 0\n"
			            " local i=1 delay=3\n"
			            " while [ $i -le 4 ]; do\n"
			            "  ping -qc1 -W2 -I wg%d %s &>/dev/null && return 0\n"
			            "  sleep $delay\n"
			            "  delay=$((delay * 2))\n"
			            "  i=$((i + 1))\n"
			            " done\n"
			            " return 1\n"
			            "}\n"
			            "[ \"$(nvram get g_upgrade)\" = \"1\" -o \"$(nvram get g_reboot)\" = \"1\" ] && exit 0\n"
			            "ip route | grep -q \"^default\" || exit 0\n"
			            "[ -r /sys/class/net/wg%d/operstate ] || {\n"
			            " logger -t wg-watchdog wg%d interface missing? Restarting ...\n"
			            " service wireguard%d restart\n"
			            " exit 0\n"
			            "}\n"
			            "ISUP=$(cat /sys/class/net/wg%d/operstate)\n"
			            "[ \"$ISUP\" = \"unknown\" -o \"$ISUP\" = \"up\" ] && pingme && exit 0\n"
			            "logger -t wg-watchdog wg%d stopped? Restarting ...\n"
			            "service wireguard%d restart\n",
			            atoi(getNVRAMVar("wg%d_tchk", unit)),
			            unit, ipchk,
			            unit,
			            unit,
			            unit,
			            unit,
			            unit,
			            unit);
			fclose(fp);
			chmod(buffer, (S_IRUSR | S_IWUSR | S_IXUSR));

			snprintf(taskname, BUF_SIZE_32,"CheckWireguard%d", unit);
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
		logmsg(LOG_WARNING, "command failed: ip link add dev %s type wireguard", iface);
		return -1;
	}
	else
		logmsg(LOG_DEBUG, "command added: ip link add dev %s type wireguard", iface);

	return 0;
}

static int wg_set_iface_addr(char *iface, const char *addr)
{
	char *nv, *nvp, *b;

	/* Flush all addresses from interface */
	if (eval("ip", "addr", "flush", "dev", iface)) {
		logmsg(LOG_WARNING, "command failed: ip addr flush dev %s", iface);
		return -1;
	}
	else
		logmsg(LOG_DEBUG, "command added: ip addr flush dev %s", iface);

	/* Set wireguard interface address(es) */
	nv = nvp = strdup(addr);
	if (!nv) {
		logmsg(LOG_ERR, "%s: strdup failed for %s (out of memory)", __FUNCTION__, iface);
		return -1;
	}
	while ((b = strsep(&nvp, ",")) != NULL) {
		if (eval("ip", "addr", "add", b, "dev", iface)) {
			logmsg(LOG_WARNING, "command failed: ip addr add %s dev %s", b, iface);
			free(nv);
			return -1;
		}
		else
			logmsg(LOG_DEBUG, "command added: ip addr add %s dev %s", b, iface);
	}
	free(nv);

	return 0;
}

static int wg_set_iface_port(const int unit, char *iface)
{
	wg_script_ctx_t *ctx = &wg_script_ctx[unit];

	if (eval("wg", "set", iface, "listen-port", ctx->port)) {
		logmsg(LOG_WARNING, "command failed: wg set %s listen-port %s", iface, ctx->port);
		return -1;
	}
	else
		logmsg(LOG_DEBUG, "command added: wg set %s listen-port %s", iface, ctx->port);

	return 0;
}

static int wg_set_iface_fwmark(const int unit, char *iface)
{
	wg_script_ctx_t *ctx = &wg_script_ctx[unit];

	if (eval("wg", "set", iface, "fwmark", ctx->fwmark)) {
		logmsg(LOG_WARNING, "command failed: wg set %s fwmark %s", iface, ctx->fwmark);
		return -1;
	}
	else
		logmsg(LOG_DEBUG, "command added: wg set %s fwmark %s", iface, ctx->fwmark);

	return 0;
}

static int wg_set_iface_privkey(char *iface, const char *privkey)
{
	FILE *fp;
	char buffer[BUF_SIZE];

	/* write private key to file */
	snprintf(buffer, BUF_SIZE, WG_KEYS_DIR"/%s", iface);

	if (!(fp = fopen(buffer, "w"))) {
		logmsg(LOG_ERR, "cannot open file for writing: %s (%s)", buffer, strerror(errno));
		return -1;
	}
	fprintf(fp, "%s", privkey);
	fclose(fp);

	chmod(buffer, (S_IRUSR | S_IWUSR));

	/* set interface private key */
	if (eval("wg", "set", iface, "private-key", buffer)) {
		logmsg(LOG_WARNING, "command failed: wg set %s private-key xxxxx", iface);
		return -1;
	}
	else
		logmsg(LOG_DEBUG, "command added: wg set %s private-key xxxxx", iface);

	/* remove file for security */
	remove(buffer);

	return 0;
}

static int wg_set_iface_mtu(char *iface, char *mtu)
{
	if (eval("ip", "link", "set", "dev", iface, "mtu", mtu)) {
		logmsg(LOG_WARNING, "command failed: ip link set dev %s mtu %s", iface, mtu);
		return -1;
	}
	else
		logmsg(LOG_DEBUG, "command added: ip link set dev %s mtu %s", iface, mtu);

	return 0;
}

static int wg_set_iface_up(char *iface)
{
	int retry = 0;

	while (retry < 5) {
		if (!(eval("ip", "link", "set", "up", "dev", iface))) {
			logmsg(LOG_DEBUG, "command added: ip link set up dev %s", iface);
			return 0;
		}
		else if (retry < 4) {
			logmsg(LOG_WARNING, "command failed: ip link set up dev %s (%d) ...", iface, retry + 1);
			sleep(4);
		}
		retry += 1;
	}

	logmsg(LOG_WARNING, "command failed: ip link set up dev %s", iface);

	return -1;
}

static void wg_iface_script(const int unit, const char *script_name)
{
	char *script;
	char buffer[BUF_SIZE_32];
	char path[FILENAME_MAX];
	FILE *fp;

	snprintf(buffer, BUF_SIZE_32, "wg%d_%s", unit, script_name);
	script = nvram_safe_get(buffer);

	if (strcmp(script, "") != 0) {
		/* build path */
		snprintf(path, FILENAME_MAX, WG_SCRIPTS_DIR"/wg%d-%s.sh", unit, script_name);

		if (!(fp = fopen(path, "w"))) {
			logmsg(LOG_ERR, "unable to open %s for writing!", path);
			return;
		}
		fprintf(fp, "#!/bin/sh\n%s\n", script); /* write the content */
		fclose(fp);

		/* replace %i with interface */
		snprintf(buffer, BUF_SIZE_32, "wg%d", unit);
		if (replace_in_file(path, "%i", buffer) != 0) {
			logmsg(LOG_WARNING, "unable to substitute interface name in %s script for wireguard interface wg%d!", script_name, unit);
			return;
		}
		else
			logmsg(LOG_DEBUG, "interface substitution in %s script for wireguard interface wg%d has executed successfully", script_name, unit);

		/* run script */
		chmod(path, (S_IRUSR | S_IWUSR | S_IXUSR));
		if (wg_execute_script(path) != 0) {
			logmsg(LOG_WARNING, "unable to execute %s script for wireguard interface wg%d!", script_name, unit);
			return;
		}
		else
			logmsg(LOG_DEBUG, "%s script for wireguard interface wg%d has executed successfully", script_name, unit);
	}
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

static void wg_set_peer_psk(char *iface, char *pubkey, const char *presharedkey)
{
	FILE *fp;
	char buffer[BUF_SIZE];

	/* write preshared key to file */
	snprintf(buffer, BUF_SIZE, WG_KEYS_DIR"/%s.psk", iface);

	if (!(fp = fopen(buffer, "w"))) {
		logmsg(LOG_ERR, "cannot open file for writing: %s (%s)", buffer, strerror(errno));
		return;
	}
	fprintf(fp, "%s", presharedkey);
	fclose(fp);

	if (eval("wg", "set", iface, "peer", pubkey, "preshared-key", buffer)) {
		logmsg(LOG_WARNING, "command failed: wg set %s peer %s preshared-key xxx", iface, pubkey);
	}
	else
		logmsg(LOG_DEBUG, "command added: wg set %s peer %s preshared-key xxx", iface, pubkey);

	/* remove file for security */
	remove(buffer);
}

static int wg_set_peer_keepalive(char *iface, char *pubkey, char *keepalive)
{
	if (eval("wg", "set", iface, "peer", pubkey, "persistent-keepalive", keepalive)) {
		logmsg(LOG_WARNING, "command failed: wg set %s peer %s persistent-keepalive %s", iface, pubkey, keepalive);
		return -1;
	}
	else
		logmsg(LOG_DEBUG, "command added: wg set %s peer %s persistent-keepalive %s", iface, pubkey, keepalive);

	return 0;
}

static int wg_set_peer_endpoint(const int unit, char *iface, char *pubkey, const char *endpoint)
{
	wg_script_ctx_t *ctx = &wg_script_ctx[unit];
	char buffer[BUF_SIZE_64];

	if (atoi(getNVRAMVar("wg%d_com", unit)) == 3) /* 'External - VPN Provider' */
		snprintf(buffer, BUF_SIZE_64, "%s", endpoint);
	else
		snprintf(buffer, BUF_SIZE_64, "%s:%s", endpoint, ctx->port);

	if (eval("wg", "set", iface, "peer", pubkey, "endpoint", buffer)) {
		logmsg(LOG_WARNING, "command failed: wg set %s peer %s endpoint %s", iface, pubkey, buffer);
		return -1;
	}
	else
		logmsg(LOG_DEBUG, "command added: wg set %s peer %s endpoint %s", iface, pubkey, buffer);

	return 0;
}

static void wg_route_peer(const int unit, char *iface, char *route, char *table, const int add)
{
	if (table != NULL) {
		if (add)
			/* On error: when using mask, check if the entry is correct (for the /24-31 mask the last IP octet must be 0, for the /9-16 mask the last two octets must be 0, etc.) */
			wg_script_add(unit, WG_SCRIPT_START, "run_cmd ip route replace %s dev %s table %s", route, iface, table);
		else
			wg_script_add(unit, WG_SCRIPT_STOP,  "run_cmd ip route delete %s dev %s table %s", route, iface, table);
	}
	else {
		if (add)
			/* On error: when using mask, check if the entry is correct (for the /24-31 mask the last IP octet must be 0, for the /9-16 mask the last two octets must be 0, etc.) */
			wg_script_add(unit, WG_SCRIPT_START, "run_cmd ip route replace %s dev %s", route, iface);
		else
			wg_script_add(unit, WG_SCRIPT_STOP,  "run_cmd ip route delete %s dev %s", route, iface);
	}
}

static void wg_route_bridges(const int unit)
{
	wg_script_ctx_t *ctx = &wg_script_ctx[unit];
	FILE *fp;
	int i;
	char cmd[BUF_SIZE];
	char line[BUF_SIZE];
	char *field, *saveptr;

	for (i = 0; i < BRIDGE_COUNT; i++) { /* todo: add to GUI the option to select which bridge wg should route traffic to */
		snprintf(cmd, BUF_SIZE, "ip route show dev br%d", i);

		if ((fp = popen(cmd, "r"))) {
			while (fgets(line, BUF_SIZE, fp)) {
				line[strcspn(line, "\n")] = '\0';

				field = strtok_r(line, " \t", &saveptr);
				if (field && strlen(field) > 1) {
					/* START (we don't need it for STOP) */
					wg_script_add(unit, WG_SCRIPT_START, "run_cmd ip route replace %s dev br%d table %s", field, i, ctx->fwmark);
				}
			}
			pclose(fp);
		}
	}
}

static void wg_init_table(const int unit, char *iface)
{
	wg_script_ctx_t *ctx = &wg_script_ctx[unit];
	FILE *fp;
	char route[BUF_SIZE];
	char cmd[BUF_SIZE_64];
	unsigned int i, n_ifaces, skip;
	char *nl;
	int routing = atoi(getNVRAMVar("wg%d_rgwr", unit));

	wg_script_add(unit, WG_SCRIPT_START, "logger -p INFO -t \"$WG_TAG\" \"creating wireguard (wg%d) routing table %s (mode %d)\" || true", unit, ctx->fwmark, routing);

	/* strict - copy routes from main routing table only for this interface */
	if (routing == VPN_RGW_POLICY_STRICT) {
		snprintf(cmd, BUF_SIZE_64, "ip route show table main dev %s", iface);

		if ((fp = popen(cmd, "r")) != NULL) {
			while (fgets(route, BUF_SIZE, fp)) {
				route[strcspn(route, "\n")] = '\0';

				/* START (we don't need it for STOP) */
				wg_script_add(unit, WG_SCRIPT_START, "run_cmd ip route replace table %s %s dev %s", ctx->fwmark, route, iface);
			}
			pclose(fp);
		}
	}
	/* standard mode - copy routes from main routing table (exclude vpns and all default gateways) */
	else if (routing == VPN_RGW_POLICY) {
		if ((fp = popen("ip route show table main", "r"))) {
			n_ifaces = ASIZE(vpn_ifaces);

			while (fgets(route, BUF_SIZE, fp)) {
				skip = 0;
				nl = strchr(route, '\n');

				if (nl)
					*nl = '\0';

				/* skip default and other uneeded routes */
				if ((strncmp(route, "default ", 8) == 0) || (strncmp(route, "0.0.0.0/1 ", 10) == 0) ||
				    (strncmp(route, "128.0.0.0/1 ", 12) == 0) || (strstr(route, "proto mwwatchdog")))
					continue;

				/* skip vpn ifaces */
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

				/* START (we don't need it for STOP) */
				wg_script_add(unit, WG_SCRIPT_START, "run_cmd ip route replace table %s %s", ctx->fwmark, route);
			}
			pclose(fp);
		}
	}
}

static void wg_snapshot_standard(const int unit, char *iface, char *route, char *table)
{
	wg_route_peer(unit, iface, route, table, 0);
	wg_route_peer(unit, iface, route, table, 1);
}

static void wg_snapshot_default(const int unit, char *iface, char *route)
{
	wg_script_ctx_t *ctx = &wg_script_ctx[unit];
	int add = !ctx->default_configured;

#ifdef KERNEL_WG_FIX
	/* STOP - reverse order */
	if (add) wg_script_add(unit, WG_SCRIPT_STOP, "run_cmd ip route flush cache");
	if (add) wg_script_add(unit, WG_SCRIPT_STOP, "run_cmd ip route flush table %s", ctx->fwmark);
	if (add) wg_script_add(unit, WG_SCRIPT_STOP, "run_cmd ip rule delete table main suppress_prefixlength 0");
	if (add) wg_script_add(unit, WG_SCRIPT_STOP, "run_cmd ip rule delete not fwmark %s table %s", ctx->fwmark, ctx->fwmark);
	wg_route_peer(unit, iface, route, ctx->fwmark, 0);

	/* START */
	/* pre-cleanup: idempotent removal of stale rules from previous crash (no logs) */
	if (add) wg_script_add(unit, WG_SCRIPT_START, "ip rule delete table main suppress_prefixlength 0 2>/dev/null || true");

	wg_route_peer(unit, iface, route, ctx->fwmark, 1);
	if (add)
		snprintf(ctx->epilog_buf, sizeof(ctx->epilog_buf),
		         "run_cmd ip rule add not fwmark %s table %s\n"
		         "run_cmd ip rule add table main suppress_prefixlength 0\n",
		         ctx->fwmark, ctx->fwmark);
#else
	/* STOP - reverse order */
	if (add) wg_script_add(unit, WG_SCRIPT_STOP, "run_cmd ip route flush cache");
	if (add) wg_script_add(unit, WG_SCRIPT_STOP, "run_cmd ip route flush table %s", ctx->fwmark);
	if (add) wg_script_add(unit, WG_SCRIPT_STOP, "run_cmd ip rule delete not fwmark %s table %s", ctx->fwmark, ctx->fwmark);
	wg_route_peer(unit, iface, route, ctx->fwmark, 0);

	/* START */
	/* pre-cleanup: idempotent removal of stale rules from previous crash (no logs) */
	if (add) wg_script_add(unit, WG_SCRIPT_START, "ip rule delete not fwmark %s table %s 2>/dev/null || true", ctx->fwmark, ctx->fwmark);
	if (add) wg_script_add(unit, WG_SCRIPT_START, "ip route flush table %s 2>/dev/null || true", ctx->fwmark);

	/* route for bridges (start) */
	if (add) wg_route_bridges(unit);
	wg_route_peer(unit, iface, route, ctx->fwmark, 1);
	if (add)
		snprintf(ctx->epilog_buf, sizeof(ctx->epilog_buf),
		         "run_cmd ip rule add not fwmark %s table %s\n",
		         ctx->fwmark, ctx->fwmark);
#endif
	ctx->default_configured = 1;
}

static void wg_snapshot_policy(const int unit, char *iface, char *route)
{
	wg_script_ctx_t *ctx = &wg_script_ctx[unit];
	char fwmark_mask[BUF_SIZE_16];
	char wgrouting_mark[BUF_SIZE_16];
	char prio_buf[BUF_SIZE_16];
	char *priority;

	snprintf(fwmark_mask, sizeof(fwmark_mask), "%s/0xf00", ctx->fwmark);
	snprintf(wgrouting_mark, BUF_SIZE_16, "wgrouting%s", ctx->fwmark);

	/* policy infrastructure saved only once - there can be multiple allowed_ips.
	 * No per-peer ip route add needed: traffic is steered by iptables mark
	 * into table fwmark which already has a default route via this interface.
	 */
	if (ctx->policy_configured)
		return;

	ctx->policy_configured = 1;

	/* needed clean-up - no logs */
	wg_script_add(unit, WG_SCRIPT_START, "ip rule delete fwmark %s table %s 2>/dev/null || true", fwmark_mask, ctx->fwmark);
	wg_script_add(unit, WG_SCRIPT_START, "ip route flush table %s 2>/dev/null || true", ctx->fwmark);

	/* build routing */
	wg_build_routing(unit, fwmark_mask, wgrouting_mark);

	/* reset interface mark - we don't want to mark packets for PBR */
	wg_script_add(unit, WG_SCRIPT_START, "run_cmd wg set %s fwmark 0", iface);

	wg_script_add(unit, WG_SCRIPT_START, "logger -p INFO -t \"$WG_TAG\" \"starting routing policy for wireguard%d - interface %s - table %s\" || true", unit, iface, ctx->fwmark);

	/* ---------- START ---------- */

	/* load modules - no logs */
	wg_script_add(unit, WG_SCRIPT_START, "modprobe ip_set || true");
	wg_script_add(unit, WG_SCRIPT_START, "modprobe xt_set || true");
	wg_script_add(unit, WG_SCRIPT_START, "modprobe ip_set_hash_ip || true");

	/* default route */
	wg_script_add(unit, WG_SCRIPT_START, "run_cmd ip route replace default dev %s table %s", iface, ctx->fwmark);

	/* priority */
	priority = getNVRAMVar("wg%d_prio", unit);
	if (priority[0] == '\0')
		snprintf(prio_buf, BUF_SIZE_16, "%d", 100 + unit); /* default: 100, 101, 102 ... */
	else
		snprintf(prio_buf, BUF_SIZE_16, "%s", priority);

	wg_script_add(unit, WG_SCRIPT_START, "run_cmd ip rule add fwmark %s table %s priority %s", fwmark_mask, ctx->fwmark, prio_buf);

	/* copy routes */
	wg_init_table(unit, iface);

	/* route for bridges */
	wg_route_bridges(unit);

	wg_script_add(unit, WG_SCRIPT_START, "run_cmd ipset create %s hash:ip", wgrouting_mark);
	wg_script_add(unit, WG_SCRIPT_START, "run_cmd ip route flush cache");

	/* ---------- STOP (LIFO) ---------- */

	if (ctx->routing_ipset_name[0] != '\0')
		wg_script_add(unit, WG_SCRIPT_STOP, "run_cmd ipset destroy %s", ctx->routing_ipset_name);

	wg_script_add(unit, WG_SCRIPT_STOP, "run_cmd ip route flush cache");
	wg_script_add(unit, WG_SCRIPT_STOP, "run_cmd ip route flush table %s", ctx->fwmark);
	wg_script_add(unit, WG_SCRIPT_STOP, "run_cmd ip rule delete fwmark %s table %s", fwmark_mask, ctx->fwmark);

	wg_script_add(unit, WG_SCRIPT_START, "logger -p INFO -t \"$WG_TAG\" \"completed routing policy configuration for wireguard - interface %s - table %s\" || true", iface, ctx->fwmark);
}

static void wg_snapshot_route_for_entry(const int unit, char *iface, char *route, char *table, const int is_default_route)
{
	int mode;

	mode = wg_detect_routing_mode(unit, is_default_route);

	switch (mode) {
		case 1: /* default route */
			wg_snapshot_default(unit, iface, route);
		break;

		case 2: /* policy */
		case 3: /* strict */
			wg_snapshot_policy(unit, iface, route);
		break;

		default:
			wg_snapshot_standard(unit, iface, route, table);
		break;
	}
}

static void wg_route_peer_allowed_ips(const int unit, char *iface, const char *allowed_ips)
{
	char *aip, *aipp, *b, *table, *rt, *tp, *ip, *nm;
	int parsed, is_default, route_type = 1;
	char buffer[BUF_SIZE_32];
	char table_buf[BUF_SIZE_32];

	memset(table_buf, 0, BUF_SIZE_32);
	table = rt = NULL;

	tp = b = strdup(getNVRAMVar("wg%d_route", unit));
	if (!tp) {
		logmsg(LOG_ERR, "%s: strdup failed for wg%d route (out of memory)", __FUNCTION__, unit);
		return;
	}
	parsed = vstrsep(b, "|", &rt, &table);

	if ((!rt) || (rt[0] == '\0')) {
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

	logmsg(LOG_DEBUG, "*** %s: routing: iface=[%s] route_type=[%d] table=[%s]", __FUNCTION__, iface, route_type, table);

	/* check which routing type the user specified */
	if (route_type <= 0)
		return; /* !off */

	aip = aipp = strdup(allowed_ips);
	if (!aip) {
		logmsg(LOG_ERR, "%s: strdup failed for wg%d allowed_ips (out of memory)", __FUNCTION__, unit);
		return;
	}
	while ((b = strsep(&aipp, ",")) != NULL) {
		snprintf(buffer, BUF_SIZE_32, "%s", b);

		if (vstrsep(b, "/", &ip, &nm) == 2) {
			is_default = (atoi(nm) == 0);
			wg_snapshot_route_for_entry(unit, iface, buffer, (route_type == 1) ? NULL : table, is_default);
		}
	}
	free(aip);
}

static void wg_set_peer_allowed_ips(char *iface, char *pubkey, char *allowed_ips)
{
	if (eval("wg", "set", iface, "peer", pubkey, "allowed-ips", allowed_ips))
		logmsg(LOG_WARNING, "command failed: wg set %s peer %s allowed-ips %s", iface, pubkey, allowed_ips);
	else
		logmsg(LOG_DEBUG, "command added: wg set %s peer %s allowed-ips %s", iface, pubkey, allowed_ips);
}

static void wg_add_peer(const int unit, char *iface, char *pubkey, char *allowed_ips, const char *presharedkey, char *keepalive, const char *endpoint)
{
	/* set allowed ips / create peer */
	wg_set_peer_allowed_ips(iface, pubkey, allowed_ips);

	/* set peer psk */
	if (presharedkey[0] != '\0')
		wg_set_peer_psk(iface, pubkey, presharedkey);

	/* set peer keepalive */
	if (atoi(keepalive) > 0)
		wg_set_peer_keepalive(iface, pubkey, keepalive);

	/* set peer endpoint */
	if (endpoint[0] != '\0')
		wg_set_peer_endpoint(unit, iface, pubkey, endpoint);

	/* add routes (also default route if any) */
	wg_route_peer_allowed_ips(unit, iface, allowed_ips);
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

static void wg_add_peer_privkey(const int unit, char *iface, const char *privkey, char *allowed_ips, const char *presharedkey, char *keepalive, const char *endpoint)
{
	char pubkey[BUF_SIZE_64];

	memset(pubkey, 0, BUF_SIZE_64);
	wg_pubkey(privkey, pubkey);

	wg_add_peer(unit, iface, pubkey, allowed_ips, presharedkey, keepalive, endpoint);
}

static int wg_set_iface_down(char *iface)
{
	/* check if interface exists */
	if (wg_if_exist(iface)) {
		if (eval("ip", "link", "set", "down", "dev", iface)) {
			logmsg(LOG_WARNING, "command failed: ip link set down dev %s", iface);
			return -1;
		}
		else
			logmsg(LOG_DEBUG, "command added: ip link set down dev %s", iface);
	}

	return 0;
}

static int wg_remove_iface(char *iface)
{
	/* check if interface exists */
	if (wg_if_exist(iface)) {
		/* delete wireguard interface */
		if (eval("ip", "link", "del", "dev", iface)) {
			logmsg(LOG_WARNING, "command failed: ip link del dev %s", iface);
			return -1;
		}
		logmsg(LOG_DEBUG, "command added: ip link del dev %s", iface);
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
	char wg_child_pid[BUF_SIZE_32];
	int mode, n, use_routing_snapshot;
	pid_t pidof_child = 0;

	snprintf(buffer, BUF_SIZE, "wireguard%d", unit);
	if (serialize_restart(buffer, 1))
		return;

	snprintf(wg_child_pid, BUF_SIZE_32, pid_path, unit); /* add no of unit to pid file */

	memset(buffer, 0, BUF_SIZE);
	if (f_read_string(wg_child_pid, buffer, BUF_SIZE_32) > 0 && atoi(buffer) > 0 && ppid(atoi(buffer)) > 0) { /* fork is still up */
		logmsg(LOG_WARNING, "%s: another process (PID: %s) still up, aborting ...", __FUNCTION__, buffer);
		return;
	}

	/* determine interface */
	snprintf(iface, IF_SIZE, "wg%d", unit);

	/* set up directories for later use */
	wg_setup_dirs();

	/* fork new process */
	if (fork() != 0) /* foreground process */
		return;

	pidof_child = getpid();

	/* write child pid to a file */
	snprintf(buffer, BUF_SIZE, "%d", pidof_child);
	f_write_string(wg_child_pid, buffer, 0, 0);

	/* wait a given time */
	n = atoi(getNVRAMVar("wg%d_sleep", unit));
	if (n > 0 && n < 60) {
		logmsg(LOG_INFO, "wg%d - delaying start by %d seconds ...", unit, n);
		sleep(n);
	}

	/* forked children each have their own instance */
	restart_dnsmasq = 0;
	restart_fw = 0;

	/* mode */
	use_routing_snapshot = (getNVRAMVar("wg%d_file", unit)[0] == '\0');

	/* check if file is specified */
	if (!use_routing_snapshot) {
		if (wg_quick_iface(iface, getNVRAMVar("wg%d_file", unit), 1))
			goto out;
	}
	else {
		/* init bash start/stop scripts */
		if (wg_script_init(unit))
			goto out;

		/* prepare port value */
		wg_set_port(unit); /* ctx->port */

		/* prepare fwmark value */
		wg_set_fwmark(unit); /* ctx->fwmark */

		/* create interface */
		if (wg_create_iface(iface))
			goto out;

		/* set interface address */
		if (wg_set_iface_addr(iface, getNVRAMVar("wg%d_ip", unit)))
			goto out;

		/* set interface port */
		if (wg_set_iface_port(unit, iface))
			goto out;

		/* set interface fwmark */
		if (wg_set_iface_fwmark(unit, iface))
			goto out;

		/* set interface private key */
		if (wg_set_iface_privkey(iface, getNVRAMVar("wg%d_key", unit)))
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
		nv = nvp = strdup(getNVRAMVar("wg%d_peers", unit));
		if (!nv) {
			logmsg(LOG_ERR, "%s: strdup failed for wg%d peers (out of memory)", __FUNCTION__, unit);
			goto out;
		}
		mode = atoi(getNVRAMVar("wg%d_com", unit));

		while ((b = strsep(&nvp, ">")) != NULL) {
			if (vstrsep(b, "<", &priv, &name, &ep, &key, &psk, &ip, &aip, &ka) < 8)
				continue;

			/* build peer allowed ips */
			if (aip[0] == '\0')
				snprintf(buffer, BUF_SIZE, "%s", ip);
			else if (ip[0] == '\0')
				snprintf(buffer, BUF_SIZE, "%s", aip);
			else
				snprintf(buffer, BUF_SIZE, "%s,%s", ip, aip);

			/* add peer to interface (and route) */
			if (priv[0] == '1') /* peer has private key? */
				wg_add_peer_privkey(unit, iface, key, buffer, psk, (mode == 3 ? ka : rka), ep);
			else
				wg_add_peer(unit, iface, key, buffer, psk, (mode == 3 ? ka : rka), ep);
		}
		free(nv);

		/* run post up scripts */
		wg_iface_post_up(unit);
	}

	/* create firewall script & DNS rules */
	wg_build_firewall(unit);

	/* start script */
	if (use_routing_snapshot) {
		if (wg_script_apply(unit) != 0) {
			logmsg(LOG_WARNING, "routing script failed - rolling back wg%d", unit);
			goto out;
		}
		else
			logmsg(LOG_DEBUG, "wg%d: start routing script OK", unit);
	}

	/* lock */
	simple_lock("firewall");

	/* firewall + dns rules */
	snprintf(buffer, BUF_SIZE, WG_FW_DIR"/wg%d-fw.sh", unit);

	/* first remove existing firewall rule(s) */
	run_del_firewall_script(buffer, WG_DIR_DEL_SCRIPT);

	/* then add firewall rule(s) */
	if (eval(buffer)) {
		logmsg(LOG_WARNING, "firewall script failed - rolling back wg%d", unit);
		simple_unlock("firewall");
		goto out;
	}
	else
		logmsg(LOG_DEBUG, "wg%d: start firewall script OK", unit);

	/* the same for routing rule(s) file, if exists */
	snprintf(buffer, BUF_SIZE, WG_FW_DIR"/wg%d-fw-routing.sh", unit);
	if (f_exists(buffer)) {
		/* first remove all existing routing rule(s) */
		run_del_firewall_script(buffer, WG_DIR_DEL_SCRIPT);

		if (eval(buffer)) {
			logmsg(LOG_WARNING, "route rules script failed - rolling back wg%d", unit);
			simple_unlock("firewall");
			goto out;
		}
		else
			logmsg(LOG_DEBUG, "wg%d: start route rules script OK", unit);
	}
	/* unlock */
	simple_unlock("firewall");

	wg_setup_watchdog(unit);

	logmsg(LOG_INFO, "wireguard (%s) started", iface);

	/* requests PID 1 to restart*/
	if (restart_dnsmasq) {
		logmsg(LOG_DEBUG, "*** %s: requesting dnsmasq restart from init", __FUNCTION__);
		stop_service("dnsmasq");
		start_service("dnsmasq");
		restart_dnsmasq = 0;
	}
	if (restart_fw) {
		logmsg(LOG_DEBUG, "*** %s: requesting firewall restart from init", __FUNCTION__);
		simple_lock("firewall");
		stop_service("firewall");
		start_service("firewall");
		simple_unlock("firewall");
		restart_fw = 0;
	}

	eval("rm", "-f", wg_child_pid);

	/* terminate the child */
	_exit(0);

out:
	if (wg_script_ctx[unit].start_fp)
		fclose(wg_script_ctx[unit].start_fp);

	eval("rm", "-f", wg_child_pid);

	stop_wireguard(unit);

	/* terminate the child */
	_exit(0);
}

void stop_wireguard(const int unit)
{
	wg_script_ctx_t *ctx = &wg_script_ctx[unit];
	char iface[IF_SIZE];
	char buffer[BUF_SIZE];
	char wg_child_pid[BUF_SIZE_32];
	char fwmark_mask[BUF_SIZE_16];
	int is_dev, m;

	snprintf(buffer, BUF_SIZE, "wireguard%d", unit);
	if (serialize_restart(buffer, 0))
		return;

	/* prepare variables */
	snprintf(wg_child_pid, BUF_SIZE_32, pid_path, unit); /* add no of unit to pid file */
	m = atoi(getNVRAMVar("wg%d_sleep", unit)) + 10;

	/* wait for child of start_wireguard to finish (if any) */
	memset(buffer, 0, BUF_SIZE);
	if (f_read_string(wg_child_pid, buffer, BUF_SIZE) > 0 && atoi(buffer) > 0 && ppid(atoi(buffer)) > 0 && (m-- > 0)) {
		logmsg(LOG_DEBUG, "*** %s: waiting for child process of start_wireguard to end, %d secs left ...", __FUNCTION__, m);
		sleep(1);
	}

	/* remove cron job */
	snprintf(buffer, BUF_SIZE, "CheckWireguard%d", unit);
	eval("cru", "d", buffer);

	/* remove watchdog file */
	snprintf(buffer, BUF_SIZE, WG_SCRIPTS_DIR"/watchdog-wg%d.sh", unit);
	eval("rm", "-f", buffer);

	/* determine interface, set flag */
	snprintf(iface, IF_SIZE, "wg%d", unit);
	is_dev = wg_if_exist(iface);

	/* lock */
	simple_lock("firewall");

	/* remove firewall rules/script */
	snprintf(buffer, BUF_SIZE, WG_FW_DIR"/wg%d-fw.sh", unit);
	if (f_exists(buffer)) {
		run_del_firewall_script(buffer, WG_DIR_DEL_SCRIPT);
		eval("rm", "-f", buffer);
	}

	/* remove routing rules/script */
	snprintf(buffer, BUF_SIZE, WG_FW_DIR"/wg%d-fw-routing.sh", unit);
	if (f_exists(buffer)) {
		run_del_firewall_script(buffer, WG_DIR_DEL_SCRIPT);
		eval("rm", "-f", buffer);
	}

	/* unlock */
	simple_unlock("firewall");

	if (getNVRAMVar("wg%d_file", unit)[0] != '\0')
		wg_quick_iface(iface, getNVRAMVar("wg%d_file", unit), 0);
	else {
		wg_iface_pre_down(unit);

		/* remove start script */
		snprintf(buffer, BUF_SIZE, WG_SCRIPTS_DIR"/wg%d-start.sh", unit);
		eval("rm", "-f", buffer);

		/* run/remove stop script */
		snprintf(buffer, BUF_SIZE, WG_SCRIPTS_DIR"/wg%d-stop.sh", unit);
		if (f_exists(buffer)) {
			if (wg_execute_script(buffer) != 0)
				logmsg(LOG_WARNING, "wg%d: stop routing script failed", unit);
			else
				logmsg(LOG_DEBUG, "wg%d: stop routing script executed", unit);

			eval("rm", "-f", buffer);
		}
		else {
			/* emergency stop without snapshot - reconstruct from the current NVRAM */
			wg_set_port(unit);
			wg_set_fwmark(unit);

			/* safety net — no logs */
			if (atoi(getNVRAMVar("wg%d_rgwr", unit)) >= VPN_RGW_POLICY) {
				snprintf(fwmark_mask, BUF_SIZE_16, "%s/0xf00", ctx->fwmark);
				eval("ip", "rule", "delete", "fwmark", fwmark_mask, "table", ctx->fwmark);
			}
			else if (atoi(getNVRAMVar("wg%d_rgwr", unit)) == VPN_RGW_ALL) {
				eval("ip", "rule", "delete", "not", "fwmark", ctx->fwmark, "table", ctx->fwmark);
#ifdef KERNEL_WG_FIX
				eval("ip", "rule", "delete", "table", "main", "suppress_prefixlength", "0");
#endif
			}
			eval("ip", "route", "flush", "table", ctx->fwmark);
			eval("ip", "route", "flush", "cache");
		}

		/* remove interface */
		wg_set_iface_down(iface);
		wg_remove_iface(iface);
		wg_iface_post_down(unit);
	}

	/* clean dnsmasq ipset entries for policy routing */
	if (ctx->routing_ipset_name[0]) {
		if (f_exists(dmipset)) {
			update_dnsmasq_ipset(ctx->routing_ipset_name, NULL, 0);
			restart_dnsmasq = 1;
		}
	}

	/* restart if needed; PID 1 can restart directly, child must send request to pid 1 */
	if (restart_dnsmasq) {
		if (getpid() == 1) {
			stop_dnsmasq();
			start_dnsmasq();
		}
		else {
			stop_service("dnsmasq");
			start_service("dnsmasq");
		}
		restart_dnsmasq = 0;
	}
	if (restart_fw) {
		simple_lock("firewall");
		if (getpid() == 1) {
			restart_firewall();
		}
		else {
			stop_service("firewall");
			start_service("firewall");
		}
		simple_unlock("firewall");
		restart_fw = 0;
	}

	eval("rm", "-f", wg_child_pid);

	if (is_dev)
		logmsg(LOG_INFO, "wireguard (%s) stopped", iface);
}

void write_wg_dnsmasq_config(FILE* fp)
{
	char buf[BUF_SIZE];
	char *pos, *fn, *saveptr, *endptr, ch;
	DIR *dir;
	struct dirent *file;
	int cur, num;

	/* add interface(s) to dns config */
	strlcpy(buf, nvram_safe_get("wg_adns"), BUF_SIZE);
	for (pos = strtok_r(buf, ",", &saveptr); pos != NULL; pos = strtok_r(NULL, ",", &saveptr)) {
		errno = 0;
		cur = (int)strtol(pos, &endptr, 10);
		if (errno == 0 && endptr != pos && *endptr == '\0' && cur >= 0) {
			logmsg(LOG_DEBUG, "*** %s: adding server wg%d interface to Dnsmasq config", __FUNCTION__, cur);
			fprintf(fp, "interface=wg%d\n", cur);
		}
	}

	dir = opendir(WG_DNS_DIR);
	if (!dir)
		return;

	while ((file = readdir(dir)) != NULL) {
		fn = file->d_name;

		if (fn[0] == '.')
			continue;

		if (sscanf(fn, "wg%d.con%c", &num, &ch) == 2 && ch == 'f') {
			snprintf(buf, BUF_SIZE, "%s/%s", WG_DNS_DIR, fn);
			if (fappend(fp, buf) == -1) {
				logmsg(LOG_WARNING, "fappend failed for %s (%s)", buf, strerror(errno));
				continue;
			}

			logmsg(LOG_DEBUG, "*** %s: adding Dnsmasq config from %s", __FUNCTION__, fn);
		}
	}
	closedir(dir);
}
