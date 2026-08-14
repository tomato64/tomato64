/*
 * Bandwidth quota usage for Tomato64
 *
 * Reads the xt_bandwidth counters through libiptbwctl and hands them to
 * quota-usage.asp. The counters live only in kernel memory and are reachable
 * solely through the match's get/setsockopt interface, so unlike IP Traffic
 * (which parses /proc/net/ipt_account) there is no file to read here.
 *
 * The set of counter ids comes from the id file rc writes while generating the
 * quota rules - see rc/quotas.c for why it is produced there rather than
 * re-derived from NVRAM.
 */

#include "tomato.h"

#include <ipt_bwctl.h>

#define QUOTA_IDS_FILE		"/var/lib/quotas/ids"
#define QUOTA_MAX_WAIT_MS	3000
#define QUOTA_MAX_IDS		128

/*
 * Zero one counter.
 *
 * The obvious call - set with zero_unset and an empty ip list - does not work.
 * It clears the match's per-ip map, but a "combined" rule is also gated on
 * info->current_bandwidth, a scalar the kernel only ever assigns from a set
 * entry whose address is 0.0.0.0 (see set_single_ip_data in xt_bandwidth.c),
 * and the match tests it unconditionally:
 *
 *     match_found = info->current_bandwidth > info->bandwidth_cutoff ? 1 : ...
 *
 * So an emptied map left whole-LAN quotas reading zero on this page while the
 * kernel went on blocking. Read the entries back, zero them and write them
 * out again, exactly as Gargoyle's bw_get | awk | bw_set does: the 0.0.0.0
 * entry a combined rule keeps is then part of the set, and current_bandwidth
 * is reset with it.
 *
 * last_backup is deliberately 0. It exists so a restore can assert that saved
 * data belongs to the current accounting interval, and the kernel fails the
 * whole set when that assertion doesn't hold - there is nothing to assert when
 * zeroing, and no reason to risk a silent no-op.
 */
static void quota_reset_id(const char *id)
{
	unsigned long num_ips = 0;
	unsigned long j;
	ip_bw *data = NULL;

	if (!get_all_bandwidth_usage_for_rule_id((char *)id, &num_ips, &data, QUOTA_MAX_WAIT_MS))
		return;

	for (j = 0; j < num_ips; j++)
		data[j].bw = 0;

	set_bandwidth_usage_for_rule_id((char *)id, 1, num_ips, 0, data, QUOTA_MAX_WAIT_MS);
	free(data);
}

/*
 * Load the ids rc generated. Returns the count; ids are NUL-terminated strings
 * in a caller-supplied array.
 */
static int quota_load_ids(char ids[][BANDWIDTH_MAX_ID_LENGTH + 1], int max)
{
	FILE *f;
	char line[BANDWIDTH_MAX_ID_LENGTH + 8];
	int n = 0;

	if ((f = fopen(QUOTA_IDS_FILE, "r")) == NULL)
		return 0;

	while (n < max && fgets(line, sizeof(line), f) != NULL) {
		char *p = line;
		char *nl;

		while (*p == ' ' || *p == '\t')
			p++;
		if ((nl = strpbrk(p, "\r\n")) != NULL)
			*nl = '\0';
		if (*p == '\0' || *p == '#')
			continue;

		strlcpy(ids[n], p, BANDWIDTH_MAX_ID_LENGTH + 1);
		n++;
	}
	fclose(f);

	return n;
}

static void quota_ip_str(ip_bw *e, char *buf, size_t bufsz)
{
	if (e->family == AF_INET6) {
		struct in6_addr a;
		memcpy(&a, e->ip, sizeof(a));
		if (inet_ntop(AF_INET6, &a, buf, bufsz) == NULL)
			strlcpy(buf, "::", bufsz);
	}
	else {
		struct in_addr a;
		a.s_addr = e->ip[0];
		if (inet_ntop(AF_INET, &a, buf, bufsz) == NULL)
			strlcpy(buf, "0.0.0.0", bufsz);
	}
}

/*
 * asp: quotas
 *
 *   update.cgi?exec=quotas                     - dump current usage
 *   update.cgi?exec=quotas&arg0=reset&arg1=ID  - zero ID, then dump
 *   update.cgi?exec=quotas&arg0=reset&arg1=*   - zero every counter
 *
 * Output is a flat list so the page can group it however it likes:
 *   quota_usage=[['<id>','<ip>',<bytes>], ...];
 */
void asp_quotas(int argc, char **argv)
{
	static char ids[QUOTA_MAX_IDS][BANDWIDTH_MAX_ID_LENGTH + 1];
	char ipstr[INET6_ADDRSTRLEN];
	unsigned long num_ips;
	ip_bw *data;
	int nids, i;
	unsigned long j;
	char comma;

	nids = quota_load_ids(ids, QUOTA_MAX_IDS);

	if (argc >= 2 && strcmp(argv[0], "reset") == 0) {
		int all = (strcmp(argv[1], "*") == 0);

		/*
		 * Only ids rc actually generated may be reset. argv comes straight
		 * from the query string, and there is no reason to let it address
		 * bandwidth rules belonging to anything else.
		 */
		for (i = 0; i < nids; i++) {
			if (!all && strcmp(ids[i], argv[1]) != 0)
				continue;

			quota_reset_id(ids[i]);
		}
	}

	web_puts("\n\nquota_usage=[");
	comma = ' ';

	for (i = 0; i < nids; i++) {
		num_ips = 0;
		data = NULL;

		/* a rule that isn't loaded yet simply has nothing to report */
		if (!get_all_bandwidth_usage_for_rule_id(ids[i], &num_ips, &data, QUOTA_MAX_WAIT_MS))
			continue;

		for (j = 0; j < num_ips; j++) {
			quota_ip_str(&data[j], ipstr, sizeof(ipstr));
			web_printf("%c['%s','%s',%llu]", comma, ids[i], ipstr,
			           (unsigned long long)data[j].bw);
			comma = ',';
		}
		free(data);
	}

	web_puts("];\n");
}
