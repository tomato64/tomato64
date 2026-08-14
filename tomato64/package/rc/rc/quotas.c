/*
 * Bandwidth quotas for Tomato64
 *
 * Generates the mangle-table rules that drive the xt_bandwidth match
 * (see board/common/linux-patches/0007-xt-bandwidth.patch). Ported from
 * Gargoyle router firmware's restore_quotas (gargoyle-router.com), with two
 * substantial changes:
 *
 *  1. Gargoyle shells out "iptables -A ..." once per rule. Tomato64 rebuilds
 *     the whole ruleset and applies it with iptables-restore, so rules added
 *     out of band would be destroyed on the next firewall restart. Everything
 *     here is emitted into the same restore file as the rest of the firewall.
 *
 *  2. Gargoyle's marks collide with every mark Tomato64 already uses, and it
 *     clears them with destructive whole-word ANDs (e.g. "--set-mark
 *     0x0/0xFF000000") which would smash the bandwidth limiter's tc handle.
 *     Quotas here live entirely in the top nibble, which qos.c documents as
 *     unused, and every write is masked so QoS / PBR / bwlimit bits survive.
 *
 * Mark usage (see the mark map comment in qos.c):
 *   0x10000000  quota exceeded ("death" mark) - traffic gets dropped
 *   0x20000000  transient: combined (up+down) path flag
 *   0x40000000  transient: an explicit per-host quota matched
 *   0x80000000  transient: an address-scoped cap's list matched (see quota_emit)
 *   0x0ff00000  throttle band - which tc penalty class an over-quota rule's
 *               traffic is steered into instead of being dropped (see below)
 *
 * The throttle band reuses the bandwidth limiter's per-host field (bits 20-27,
 * masked 0x0ff00000). That is only safe because a throttle band is written
 * exclusively while the bandwidth limiter is off (see the resolution in
 * ipt_quotas): with bwl_enable set every over-quota rule falls back to the
 * death mark, so the two never write these bits on the same boot. When we do
 * own the band the entry-point clear widens to take the byte with it.
 *
 * The death/flag bits are packet marks (MARK), never connection marks. That is
 * deliberate and load bearing: a quota decision is only valid for the instant
 * the packet is seen. Held on the conntrack entry, an exceeded verdict would
 * outlive the thing that caused it - the counters would roll over at the top
 * of the hour (or be cleared from the usage page) and every connection that
 * was open at the time would stay blocked until conntrack expired it, because
 * the rules below deliberately skip a packet already carrying the death mark.
 * As packet marks the whole decision is simply retaken on every packet.
 *
 * Nothing else can see these bits: every tc "fw" filter in qos.c and
 * bwlimit.c is masked (/0xf, /0xf0, /0xff00000), cake reads "fwmark 0xf",
 * every CONNMARK --restore-mark passes --mask, and the PBR ip rules match
 * with /0xf00.
 */

#include "rc.h"

#include <arpa/inet.h>
#include <sys/stat.h>
#include <unistd.h>
#include <time.h>
#include <dirent.h>

/*
 * The whole feature is gated on TCONFIG_QUOTAS (BR2_PACKAGE_RC_QUOTAS). Without
 * it the xt_bandwidth match, timerange match and libiptbwctl are not built, so
 * these entry points collapse to no-ops - the call sites in firewall.c /
 * services.c stay unconditional, and a stray quota_enable can't make rc emit
 * "-m bandwidth" rules for a match that isn't in the kernel. See the stubs at
 * the foot of this file.
 */
#ifdef TCONFIG_QUOTAS

/*
 * Counter ids we generated rules for, one per line, written as the rules are
 * built. quota_backup reads this to know what to save and restore.
 *
 * It is deliberately produced here rather than re-derived from quota_rules:
 * a record this code skips (malformed, no caps set, unknown reset interval)
 * would shift every later index, and a second parser that disagreed even
 * slightly would restore counters into the wrong quota.
 */
#ifndef QUOTA_IDS_DIR
#define QUOTA_IDS_DIR		"/var/lib/quotas"
#endif
#ifndef QUOTA_IDS_FILE
#define QUOTA_IDS_FILE		QUOTA_IDS_DIR "/ids"
#endif

/*
 * Below this the wall clock has obviously not been set (no usable RTC, NTP not
 * yet synced) - it is years before this firmware existed. Quota accounting is
 * time-based, so nothing quota-related may run until the clock passes it. Well
 * clear of both the epoch and any plausible build date. Shared with the guard
 * in quota_backup.c; keep the two in step.
 */
#define QUOTA_CLOCK_VALID_MIN	1600000000L	/* 2020-09-13 UTC */

/* quota mark bits - top nibble only, see comment above */
#define QUOTA_MARK_DEATH	"0x10000000"
#define QUOTA_MARK_COMBINED	"0x20000000"
#define QUOTA_MARK_EXPLICIT	"0x40000000"
#define QUOTA_MARK_HIT		"0x80000000"	/* transient: this cap's address list matched */
#define QUOTA_MARK_ALL		"0xF0000000"	/* the flag nibble, for the entry-point clear */

/*
 * Throttle band (bits 20-27). An over-quota rule that limits speed rather than
 * blocking steers its traffic into a tc penalty class by writing its rule index
 * here; the class is built in /etc/quota_shaping (see start_quota_shaping). Band
 * = idx << QUOTA_MARK_BAND_SHIFT, so idx must stay <= 255 to fit the byte.
 */
#define QUOTA_MARK_BAND_MASK	"0x0ff00000"
#define QUOTA_MARK_BAND_SHIFT	20
#define QUOTA_MARK_BAND_MAX	255
#define QUOTA_MARK_ALL_BAND	"0xFFF00000"	/* flag nibble + band byte, when we own the band */

/* sentinels understood in the "ip" field of a quota record */
#define QUOTA_IP_ALL		"ALL"
#define QUOTA_IP_OTHERS_COMB	"ALL_OTHERS_COMBINED"
#define QUOTA_IP_OTHERS_INDIV	"ALL_OTHERS_INDIVIDUAL"
/*
 * A shared-pool-of-hosts record stores its address list prefixed with this, so
 * "SHARED:192.168.3.0/24,192.168.4.0/24" is one 20 GB pot across those subnets
 * rather than a per-host allowance. No address can start with a letter, so the
 * prefix can't collide with a real entry.
 */
#define QUOTA_IP_HOSTS_SHARED	"SHARED:"

/*
 * Minimum fields in a quota record: enabled<ip<dlimit<ulimit<climit<reset<rday<
 * rhour<active<action<desc. Two optional throttle-speed fields (tdl, tul, in
 * kbit/s) may follow; a config saved before they existed simply lacks them and
 * is read as block-on-exceed. A 14th field carries the rule's id (see below).
 */
#define QUOTA_FIELDS		11

/*
 * Rule identity - the "id" field, 14th in a record.
 *
 * Counter ids are "quota_<id>_<d|u|c>", where <id> comes from the record itself
 * rather than from the rule's position in the list. That id is what names the
 * on-disk usage file and what the GUI resets, so it has to survive every edit
 * that leaves the rule meaning the same thing.
 *
 * Position cannot do that. Deleting, reordering or merely disabling a rule
 * renumbers everything after it, and each of those rules would then adopt its
 * neighbour's saved usage on the next restore; changing a rule's address in
 * place would silently hand the new host the old host's consumption.
 *
 * So quota-settings.asp issues an id from the monotonic quota_nextid counter when a
 * rule is created, and a fresh one whenever the address changes (a repurposed
 * rule starts from zero). Ids are never reused, which is what lets stale usage
 * files be pruned by name - see quota_prune_backups().
 *
 * The fallback below only fires on a record written before ids existed. Such a
 * rule keeps behaving as it did until the page is saved once, at which point
 * the GUI backfills an id. Ids are allocated from 1 upwards, so a backfilled
 * record can in principle collide with a legacy record's position for that one
 * save; not worth more machinery than this comment.
 */

/* needed by logmsg() */
#define LOGMSG_DISABLE		DISABLE_SYSLOG_OS
#define LOGMSG_NVDEBUG		"quota_debug"

/* "action" field: what happens to a rule's traffic once it is over quota */
#define QUOTA_ACT_BLOCK		0
#define QUOTA_ACT_THROTTLE	1

/* per-rule throttle speeds for start_quota_shaping, written as ipt_quotas runs */
#ifndef QUOTA_SHAPE_FILE
#define QUOTA_SHAPE_FILE	QUOTA_IDS_DIR "/shaping"
#endif

/* how a quota record's address field is being applied */
enum {
	QUOTA_SCOPE_ALL,		/* whole LAN, one shared counter */
	QUOTA_SCOPE_OTHERS_COMB,	/* unmetered hosts, one shared counter */
	QUOTA_SCOPE_OTHERS_INDIV,	/* unmetered hosts, counter per host */
	QUOTA_SCOPE_HOSTS,		/* explicitly listed hosts, counter per host */
	QUOTA_SCOPE_HOSTS_SHARED	/* explicitly listed hosts, one shared counter */
};

/* has the wall clock been set this boot? see QUOTA_CLOCK_VALID_MIN */
int quota_clock_valid(void)
{
	return time(NULL) >= QUOTA_CLOCK_VALID_MIN;
}

static int quota_valid_reset(const char *s)
{
	return (!strcmp(s, "hour") || !strcmp(s, "day") ||
	        !strcmp(s, "week") || !strcmp(s, "month"));
}

static int quota_scope_of(const char *ip)
{
	if (!*ip || !strcmp(ip, QUOTA_IP_ALL))
		return QUOTA_SCOPE_ALL;
	if (!strcmp(ip, QUOTA_IP_OTHERS_COMB))
		return QUOTA_SCOPE_OTHERS_COMB;
	if (!strcmp(ip, QUOTA_IP_OTHERS_INDIV))
		return QUOTA_SCOPE_OTHERS_INDIV;
	if (!strncmp(ip, QUOTA_IP_HOSTS_SHARED, strlen(QUOTA_IP_HOSTS_SHARED)))
		return QUOTA_SCOPE_HOSTS_SHARED;

	return QUOTA_SCOPE_HOSTS;
}

/* the address list of a hosts record, past the shared-pool prefix if present */
static const char *quota_hosts_addrs(const char *ip)
{
	if (!strncmp(ip, QUOTA_IP_HOSTS_SHARED, strlen(QUOTA_IP_HOSTS_SHARED)))
		return ip + strlen(QUOTA_IP_HOSTS_SHARED);

	return ip;
}

/* has an explicit address list (per-host or shared pool) */
static int quota_scope_has_addrs(int scope)
{
	return scope == QUOTA_SCOPE_HOSTS || scope == QUOTA_SCOPE_HOSTS_SHARED;
}

/* metered by one shared counter rather than per host - uses the "combined" type */
static int quota_scope_shared(int scope)
{
	return scope == QUOTA_SCOPE_ALL ||
	       scope == QUOTA_SCOPE_OTHERS_COMB ||
	       scope == QUOTA_SCOPE_HOSTS_SHARED;
}

/*
 * Address test for one entry of the address list.
 *
 * dir is 's' for traffic leaving the host (upload) and 'd' for traffic
 * arriving at it (download) - the LAN host is the source of an upload and the
 * destination of a download, so the same quota needs a different test in each
 * direction.
 */
/*
 * Expand the last-octet range shorthand the web UI stores into the full form
 * iptables' iprange match needs. v_macip normalises "192.168.1.3-192.168.1.4"
 * down to "192.168.1.3-4" before saving; passed through verbatim, iprange
 * reads the bare "4" as 0.0.0.4, yielding a reversed range that "will never
 * match" (so the quota silently never fires). Turn "A.B.C.x-y" back into
 * "A.B.C.x-A.B.C.y". bwlimit.c does the same in address_checker().
 *
 * A full-form range (a dot after the dash) or a plain address is copied through
 * untouched.
 */
static void quota_range_expand(char *buf, size_t bufsz, const char *addr)
{
	const char *dash = strchr(addr, '-');
	const char *dot;

	if (dash == NULL || strchr(dash + 1, '.') != NULL) {
		strlcpy(buf, addr, bufsz);
		return;
	}

	/* last dot before the dash = end of the shared network prefix */
	for (dot = dash; dot > addr && *dot != '.'; dot--)
		;
	if (*dot != '.') {		/* no prefix to borrow - leave as-is */
		strlcpy(buf, addr, bufsz);
		return;
	}

	/* "<low>" "-" "<prefix incl. dot>" "<high last octet>" */
	snprintf(buf, bufsz, "%.*s-%.*s%s",
	         (int)(dash - addr), addr,
	         (int)(dot - addr) + 1, addr,
	         dash + 1);
}

static void quota_addr_test(char *buf, size_t bufsz, const char *addr, char dir)
{
	if (strchr(addr, '-') != NULL) {	/* range: a.b.c.d-e.f.g.h or shorthand a.b.c.d-h */
		char full[64];

		quota_range_expand(full, sizeof(full), addr);
		snprintf(buf, bufsz, " -m iprange --%s-range %s", (dir == 's') ? "src" : "dst", full);
	}
	else				/* single address or CIDR */
		snprintf(buf, bufsz, " -%c %s", dir, addr);
}

/*
 * "reset_interval" / "reset_time" for the xt_bandwidth match. The match wants
 * reset_time as an offset in seconds from the start of the interval, which is
 * how the GUI's "reset day" + "reset hour" are stored.
 */
static void quota_reset_args(char *buf, size_t bufsz, const char *reset, const char *rday, const char *rhour)
{
	long offset = 0;
	int day = (rday && *rday) ? atoi(rday) : 0;
	int hour = (rhour && *rhour) ? atoi(rhour) : 0;

	if (hour < 0 || hour > 23)
		hour = 0;

	if (!strcmp(reset, "week")) {
		if (day < 0 || day > 6)
			day = 0;
		offset = (day * 86400L) + (hour * 3600L);
	}
	else if (!strcmp(reset, "month")) {
		/* GUI is 1-31, the match counts from the start of the month */
		if (day < 1 || day > 31)
			day = 1;
		offset = ((day - 1) * 86400L) + (hour * 3600L);
	}
	else if (!strcmp(reset, "day")) {
		offset = hour * 3600L;
	}
	/* "hour" resets on the hour, offset stays 0 */

	snprintf(buf, bufsz, " --reset_interval %s --reset_time %ld", reset, offset);
}

/* leading set bits of a host-order netmask = prefix length */
static int quota_mask_bits(unsigned int mask)
{
	int bits = 0;

	while (mask & 0x80000000u) {
		bits++;
		mask <<= 1;
	}

	return bits;
}

/*
 * A network/prefix for individual_local to recognise a LAN host by.
 *
 * The xt_bandwidth "individual_local" type keys the counter on whichever end
 * of the packet sits inside this subnet, so it has to contain every LAN host.
 * Tomato has up to BRIDGE_COUNT bridges (br0..brN), each its own subnet, so a
 * single primary-LAN subnet would leave hosts on the other bridges unmetered
 * for combined caps. Widen to the smallest CIDR that covers all configured
 * bridges.
 *
 * Guard: if the bridges sit in far-apart ranges (say 192.168/16 and 10/8) that
 * smallest CIDR balloons toward 0.0.0.0/0 and would start treating WAN
 * addresses as "local". If it gets broader than /8 fall back to the primary
 * LAN - a combined cap that only covers br0 is far better than one that
 * miscounts the WAN. (Contiguous bridges, the normal case, stay tight: three
 * 192.168.x.0/24s collapse to a /22.)
 */
static void quota_lan_subnet(char *buf, size_t bufsz)
{
	char nv[24];
	struct in_addr in;
	unsigned int super_net = 0, super_mask = 0;
	unsigned int ip, mask, net;
	int have = 0;
	int i, bits;

	for (i = 0; i < BRIDGE_COUNT; i++) {
		char ipbuf[24];

		if (i == 0)
			strlcpy(nv, "lan_ipaddr", sizeof(nv));
		else
			snprintf(nv, sizeof(nv), "lan%d_ipaddr", i);
		strlcpy(ipbuf, nvram_safe_get(nv), sizeof(ipbuf));
		if (!*ipbuf)		/* bridge not configured (empty nvram) */
			continue;

		if (i == 0)
			strlcpy(nv, "lan_netmask", sizeof(nv));
		else
			snprintf(nv, sizeof(nv), "lan%d_netmask", i);

		ip = ntohl((unsigned int)inet_addr(ipbuf));
		mask = ntohl((unsigned int)inet_addr(nvram_safe_get(nv)));
		if (mask == 0 || mask == 0xFFFFFFFFu)	/* missing/degenerate netmask */
			continue;
		net = ip & mask;

		if (!have) {
			super_net = net;
			super_mask = mask;
			have = 1;
			continue;
		}

		/* widen so the mask keeps only the bits both networks agree on */
		mask &= super_mask;
		while ((super_net ^ net) & mask)
			mask <<= 1;
		super_mask = mask;
		super_net &= mask;
	}

	if (have && quota_mask_bits(super_mask) < 8) {
		/* too broad - fall back to the primary LAN only */
		ip = ntohl((unsigned int)inet_addr(nvram_safe_get("lan_ipaddr")));
		super_mask = ntohl((unsigned int)inet_addr(nvram_safe_get("lan_netmask")));
		super_net = ip & super_mask;
	}
	else if (!have) {		/* no LAN at all - nothing sensible to emit */
		super_net = 0;
		super_mask = 0;
	}

	bits = quota_mask_bits(super_mask);
	in.s_addr = htonl(super_net);
	snprintf(buf, bufsz, "%s/%d", inet_ntoa(in), bits);
}

/*
 * Emit the bandwidth rule(s) for one cap of one quota.
 *
 * dirs is the direction(s) the cap meters, as a string of quota_addr_test
 * codes: "d" (download), "s" (upload) or "sd" (combined - both).
 *
 * Crucial constraint: the xt_bandwidth match keeps a global map of every --id
 * and rejects a second rule that re-uses an id within one IP family ("duplicate
 * id in this IP family"). So a cap can be backed by exactly ONE bandwidth rule.
 * A whole-LAN/catch-all cap already is one rule. But an address-scoped cap has
 * to match a list of addresses, and a combined cap has to match both
 * directions - more matches than a single -s/-d rule can express. Emitting one
 * bandwidth rule per address/direction (all sharing the id) is what made
 * per-host and multi-subnet caps fail to load and take the whole table down.
 *
 * Instead: flag every packet the address list matches with a transient mark,
 * then run a single bandwidth rule off that mark. The bandwidth match still
 * reads the packet, so individual_* keeps keying per host; combined pools it.
 *
 * over_mark/over_mask is what to write when the rule is over its cap: the death
 * mark (block) or a throttle band (limit speed). ipt_quotas resolves it per cap
 * and direction; quota_emit just stamps it on.
 */
static void quota_emit(const char *chain, const char *id, const char *type, const char *subnet,
                       const char *limit, const char *resetargs, const char *skip_test,
                       char **addrs, int naddrs, const char *dirs,
                       const char *over_mark, const char *over_mask)
{
	char addr_test[128];
	char match[512];
	const char *d;
	int i;

	snprintf(match, sizeof(match), " -m bandwidth --id \"%s\" --type %s%s --greater_than %s%s",
	         id, type, subnet, limit, resetargs);

	if (naddrs == 0) {	/* whole-LAN or catch-all quota, no address test */
		/*
		 * A --subnet (individual_local, i.e. a per-host combined cap) carries
		 * an IPv4 network; the single --subnet option is parsed per table
		 * family, so an IPv4 value fed to ip6tables fails and drops the whole
		 * v6 table. Keep those v4-only. The plain "combined" type has no
		 * subnet and stays dual-stack so it still meters IPv6.
		 */
		if (*subnet)
			ipt_write("-A %s%s%s -j MARK --set-mark %s/%s\n",
			          chain, skip_test, match, over_mark, over_mask);
		else
			ip46t_write(ipv6_enabled, "-A %s%s%s -j MARK --set-mark %s/%s\n",
			            chain, skip_test, match, over_mark, over_mask);
		return;
	}

	/*
	 * Address-scoped, so IPv4-only (the host field holds IPv4 literals, and an
	 * IPv4 -s/-d or --iprange in ip6tables drops the whole v6 table). Mark the
	 * matching packets, one bandwidth rule off the mark, then clear the mark so
	 * it can't leak into the next cap in this chain.
	 */
	for (i = 0; i < naddrs; i++) {
		for (d = dirs; *d; d++) {
			quota_addr_test(addr_test, sizeof(addr_test), addrs[i], *d);
			ipt_write("-A %s%s -j MARK --set-mark %s/%s\n",
			          chain, addr_test, QUOTA_MARK_HIT, QUOTA_MARK_HIT);
		}
	}
	ipt_write("-A %s -m mark --mark %s/%s%s%s -j MARK --set-mark %s/%s\n",
	          chain, QUOTA_MARK_HIT, QUOTA_MARK_HIT, skip_test, match,
	          over_mark, over_mask);
	ipt_write("-A %s -j MARK --set-mark 0x0/%s\n", chain, QUOTA_MARK_HIT);
}

/* split a comma/space separated address list in place; returns the count */
static int quota_split_addrs(char *list, char **out, int max)
{
	char *tok;
	int n = 0;

	for (tok = strtok(list, ", "); tok && n < max; tok = strtok(NULL, ", ")) {
		if (*tok)
			out[n++] = tok;
	}

	return n;
}

/*
 * Bandwidth quotas.
 *
 * Called from mangle_table() after ipt_qos()/ipt_bwlimit() so that a quota
 * that is over its cap gets the last word on the packet.
 */
void ipt_quotas(void)
{
	char *buf, *g, *p;
	char *enabled, *ip, *dlimit, *ulimit, *climit, *reset, *rday, *rhour, *active, *action, *desc, *tdl, *tul, *ruleid;
	char idstr[24];
	char *addrs[64];
	char addr_test[128];
	char idbuf[64];
	char resetargs[128];
	char subnet[64];
	char lansub[32];
	char skip_test[64];
	char lanlist[512];
	char band[16];
	char wanface[MWAN_MAX][IFNAMSIZ];
	char s[16];
	int wanup_q[MWAN_MAX];
	int i, n, idx, naddrs, scope, pass, shared;
	size_t idlen;
	int have_wan = 0;
	FILE *idf = NULL;
	FILE *shapef = NULL;
	/*
	 * Whether an over-quota rule can be throttled rather than blocked comes down
	 * to who owns the tc root qdisc on the egress device. The bandwidth limiter
	 * owns the LAN bridges (download) and, with QoS off, the WAN (upload); QoS
	 * owns the WAN. So download shaping needs the limiter off, upload shaping
	 * needs both off. Anything unshapeable falls back to the death mark.
	 */
	int bwl_on = nvram_get_int("bwl_enable");
	int qos_on = nvram_get_int("qos_enable");
	int dl_free = !bwl_on;			/* LAN bridge egress available */
	int ul_free = !bwl_on && !qos_on;	/* WAN egress available */
	const char *clearmask = dl_free ? QUOTA_MARK_ALL_BAND : QUOTA_MARK_ALL;

	/*
	 * Drop any id list from a previous configuration first, so that turning
	 * quotas off (or removing every rule) doesn't leave quota_backup saving
	 * counters for rules that no longer exist.
	 */
	unlink(QUOTA_IDS_FILE);
	unlink(QUOTA_SHAPE_FILE);

	if (!nvram_get_int("quota_enable"))
		return;

	/*
	 * Do not build quota rules until the wall clock is set.
	 *
	 * The xt_bandwidth match fixes each rule's next reset boundary from the
	 * current time when the rule is inserted (checkentry). Built while the
	 * clock still reads seconds-past-the-epoch - no usable RTC, NTP not yet
	 * synced, which is the normal state for the first minutes of boot - that
	 * boundary lands in 1970. The moment the clock jumps to real time the
	 * match sees next_reset < now and zeroes the counter, and any save in
	 * between writes those bogus counters over the good on-disk backup.
	 *
	 * So defer. Leaving the id file unlinked above means quotas_pre_restore()
	 * finds nothing to save or flush, so the backup is untouched. start_-
	 * firewall() is re-run from the NTP "clock set" hook (ntpd_synced_main),
	 * which rebuilds these rules against a correct clock and then restores.
	 */
	if (!quota_clock_valid()) {
		syslog(LOG_INFO, "quotas: clock not set yet, deferring quota rules until time sync");
		return;
	}

	/*
	 * Quotas and the bandwidth limiter can run at the same time. Block-on-exceed
	 * quotas only add a DROP in the top nibble, which is disjoint from every bit
	 * the limiter and QoS use, so they never collide. Only speed-limit quotas
	 * contend for a resource - the tc root qdisc on the egress device - and that
	 * is handled per rule by falling back to a block when the device is busy
	 * (see dl_free/ul_free above), not by suppressing quotas wholesale.
	 */
	if (!nvram_invmatch("quota_rules", ""))
		return;

	/* collect the WAN interfaces quotas are measured against */
	memset(wanface, 0, sizeof(wanface));
	for (i = 1; i <= MWAN_MAX; i++) {
		memset(s, 0, sizeof(s));
		snprintf(s, sizeof(s), "wan%d", i);
		wanup_q[i - 1] = (i == 1) ? 1 : check_wanup(s);

		for (n = 0; n < wanfaces[i - 1].count; ++n) {
			if (*(wanfaces[i - 1].iface[n].name) && wanup_q[i - 1]) {
				strlcpy(wanface[i - 1], wanfaces[i - 1].iface[n].name, IFNAMSIZ);
				have_wan = 1;
				break;
			}
		}
	}
	if (!have_wan)
		return;

	quota_lan_subnet(lansub, sizeof(lansub));
	snprintf(subnet, sizeof(subnet), " --subnet %s", lansub);

	/*
	 * Chain layout, following Gargoyle:
	 *
	 *   quota_fwd  - hooked into FORWARD, splits LAN<->WAN traffic into the
	 *                direction chains and then runs the combined chain once
	 *   quota_in   - download (ingress from WAN)
	 *   quota_out  - upload (egress to WAN)
	 *   quota_comb - combined up+down
	 */
	ip46t_write(ipv6_enabled,
	            ":quota_fwd - [0:0]\n"
	            ":quota_in - [0:0]\n"
	            ":quota_out - [0:0]\n"
	            ":quota_comb - [0:0]\n");

	/*
	 * Clear all of our bits once, at the entry points, before any quota
	 * decision has been taken. Nothing else in the firewall writes the top
	 * nibble so a packet should never arrive carrying them, but a stray mark
	 * would mean silent drops - cheap insurance against an expensive symptom.
	 *
	 * When we own the throttle band (limiter off) the clear takes the band byte
	 * too. With the limiter on we must NOT - it stamps that byte in PREROUTING,
	 * before FORWAD, and its WAN egress filter still needs it downstream; a wider
	 * clear here would wipe the limiter's own upload classification.
	 *
	 * It has to happen here rather than at the head of the direction chains:
	 * quota_comb runs after quota_in/quota_out on the same packet, so a clear
	 * there would erase the verdict they just reached.
	 */
	ip46t_write(ipv6_enabled,
	            "-A FORWARD -j MARK --set-mark 0x0/%s\n"
	            "-A INPUT -j MARK --set-mark 0x0/%s\n"
	            "-A OUTPUT -j MARK --set-mark 0x0/%s\n",
	            clearmask, clearmask, clearmask);

	ip46t_write(ipv6_enabled, "-A FORWARD -j quota_fwd\n");
	for (i = 1; i <= MWAN_MAX; i++) {
		if (!wanup_q[i - 1] || !*wanface[i - 1])
			continue;

		/* forwarded LAN <-> WAN traffic */
		ip46t_write(ipv6_enabled,
		            "-A quota_fwd -o %s -m mark --mark 0x0/%s -j quota_out\n"
		            "-A quota_fwd -i %s -m mark --mark 0x0/%s -j quota_in\n",
		            wanface[i - 1], QUOTA_MARK_DEATH,
		            wanface[i - 1], QUOTA_MARK_DEATH);

		/* flag both directions so the combined chain runs exactly once */
		ip46t_write(ipv6_enabled,
		            "-A quota_fwd -o %s -m mark --mark 0x0/%s -j MARK --set-mark %s/%s\n"
		            "-A quota_fwd -i %s -m mark --mark 0x0/%s -j MARK --set-mark %s/%s\n",
		            wanface[i - 1], QUOTA_MARK_DEATH, QUOTA_MARK_COMBINED, QUOTA_MARK_COMBINED,
		            wanface[i - 1], QUOTA_MARK_DEATH, QUOTA_MARK_COMBINED, QUOTA_MARK_COMBINED);

		/* the router's own traffic to/from the WAN */
		ip46t_write(ipv6_enabled,
		            "-A INPUT -i %s -m mark --mark 0x0/%s -j quota_in\n"
		            "-A INPUT -i %s -m mark --mark 0x0/%s -j quota_comb\n"
		            "-A OUTPUT -o %s -m mark --mark 0x0/%s -j quota_out\n"
		            "-A OUTPUT -o %s -m mark --mark 0x0/%s -j quota_comb\n",
		            wanface[i - 1], QUOTA_MARK_DEATH,
		            wanface[i - 1], QUOTA_MARK_DEATH,
		            wanface[i - 1], QUOTA_MARK_DEATH,
		            wanface[i - 1], QUOTA_MARK_DEATH);
	}
	ip46t_write(ipv6_enabled,
	            "-A quota_fwd -m mark --mark %s/%s -j quota_comb\n"
	            "-A quota_fwd -j MARK --set-mark 0x0/%s\n",
	            QUOTA_MARK_COMBINED, QUOTA_MARK_COMBINED, QUOTA_MARK_COMBINED);

	/*
	 * Two passes over the rule list.
	 *
	 * Pass 0 flags every host that has an explicit quota, pass 1 emits the
	 * quota rules themselves. Rules are appended in emission order, so the
	 * flags have to be laid down first for "hosts without an explicit quota"
	 * to mean anything regardless of how the user ordered the list.
	 */
	for (pass = 0; pass < 2; pass++) {
		g = buf = strdup(nvram_safe_get("quota_rules"));
		idx = 0;

		if (pass == 1) {
			mkdir(QUOTA_IDS_DIR, 0755);
			idf = fopen(QUOTA_IDS_FILE, "w");
			shapef = fopen(QUOTA_SHAPE_FILE, "w");
		}

		while (g) {
			int is_throttle, tdl_kbit, tul_kbit;
			int over_dl_band, over_ul_band;	/* this cap's direction gets a band, not a drop */
			int shape_dl, shape_ul;		/* speeds to write to the shaping spec */
			char dmark[16], umark[16];
			const char *dmask, *umask;

			if ((p = strsep(&g, ">")) == NULL)
				break;

			/*
			 * tdl/tul/ruleid are optional - a config saved before throttle
			 * speeds, or before rule ids, simply lacks them. vstrsep NULLs the
			 * first field it can't fill and leaves the rest untouched, so seed
			 * them and re-empty any NULL it wrote, to keep the atoi()s and
			 * string reads below off a null pointer.
			 */
			tdl = tul = ruleid = "";
			i = vstrsep(p, "<", &enabled, &ip, &dlimit, &ulimit, &climit,
			            &reset, &rday, &rhour, &active, &action, &desc, &tdl, &tul, &ruleid);
			if (i < QUOTA_FIELDS)
				continue;
			if (!tdl)
				tdl = "";
			if (!tul)
				tul = "";
			if (!ruleid)
				ruleid = "";
			if (*enabled && atoi(enabled) != 1)
				continue;
			if (!quota_valid_reset(reset))
				continue;
			/* a quota with no caps at all would match everything */
			if (!*dlimit && !*ulimit && !*climit)
				continue;

			scope = quota_scope_of(ip);
			idx++;

			/*
			 * idx is still the throttle band number - that one genuinely is
			 * positional, since it only has to be unique among the rules of
			 * this ruleset and has to fit a byte. Identity is the record's
			 * own id; see the rule identity note by QUOTA_FIELDS.
			 *
			 * The id lands in both an iptables argument and a backup file
			 * name, so accept it only in the form the GUI writes it: digits,
			 * short enough to copy whole. Anything else is treated as absent
			 * rather than sanitised or truncated, so a hand-edited record
			 * degrades to the old positional behaviour instead of quietly
			 * addressing some other rule's counter - or colliding with it,
			 * which xt_bandwidth answers by rejecting the whole mangle table.
			 */
			idlen = strlen(ruleid);
			if (idlen > 0 && idlen < sizeof(idstr) && strspn(ruleid, "0123456789") == idlen)
				strlcpy(idstr, ruleid, sizeof(idstr));
			else
				snprintf(idstr, sizeof(idstr), "%d", idx);

			naddrs = 0;
			if (quota_scope_has_addrs(scope)) {
				strlcpy(lanlist, quota_hosts_addrs(ip), sizeof(lanlist));
				naddrs = quota_split_addrs(lanlist, addrs, 64);
				if (naddrs == 0)
					continue;
			}

			if (pass == 0) {
				/*
				 * Flag explicitly metered hosts in both directions so the
				 * catch-all quotas below can exclude them.
				 *
				 * IPv4-only (ipt_write), for the same reason the host rules
				 * in quota_emit() are: the tests carry IPv4 addresses. The
				 * flag exists purely so a v4 "without a quota" rule skips a
				 * host that already has an explicit v4 quota; in v6 that host
				 * has no explicit quota (it can't - the quota is keyed on an
				 * IPv4 address), so it correctly stays unflagged and its v6
				 * traffic falls under the v6 catch-all.
				 *
				 * A shared-pool host is metered too, so flag it as well.
				 */
				if (!quota_scope_has_addrs(scope))
					continue;

				for (n = 0; n < naddrs; n++) {
					quota_addr_test(addr_test, sizeof(addr_test), addrs[n], 's');
					ipt_write("-A quota_out%s -j MARK --set-mark %s/%s\n",
					          addr_test, QUOTA_MARK_EXPLICIT, QUOTA_MARK_EXPLICIT);
					ipt_write("-A quota_comb%s -j MARK --set-mark %s/%s\n",
					          addr_test, QUOTA_MARK_EXPLICIT, QUOTA_MARK_EXPLICIT);

					quota_addr_test(addr_test, sizeof(addr_test), addrs[n], 'd');
					ipt_write("-A quota_in%s -j MARK --set-mark %s/%s\n",
					          addr_test, QUOTA_MARK_EXPLICIT, QUOTA_MARK_EXPLICIT);
					ipt_write("-A quota_comb%s -j MARK --set-mark %s/%s\n",
					          addr_test, QUOTA_MARK_EXPLICIT, QUOTA_MARK_EXPLICIT);
				}
				continue;
			}

			quota_reset_args(resetargs, sizeof(resetargs), reset, rday, rhour);

			/* catch-all quotas only apply to hosts nothing else metered */
			memset(skip_test, 0, sizeof(skip_test));
			if (scope == QUOTA_SCOPE_OTHERS_COMB || scope == QUOTA_SCOPE_OTHERS_INDIV)
				snprintf(skip_test, sizeof(skip_test), " -m mark --mark 0x0/%s", QUOTA_MARK_EXPLICIT);

			/*
			 * Each cap gets its own --id and its own single bandwidth rule
			 * (quota_emit marks then meters). "combined" keeps one shared
			 * counter for the whole rule; the individual_* types keep one per
			 * host inside the match - which is what makes the per-host and
			 * "each host" scopes work. A shared pool of hosts is "combined"
			 * with an address list, so every listed host feeds one counter.
			 */
			shared = quota_scope_shared(scope);

			/*
			 * Resolve what an over-quota packet gets: the death mark (block) or
			 * this rule's throttle band (limit speed). A band is only written
			 * when the rule asks to throttle, has a speed for that direction,
			 * and the egress device is free (see dl_free/ul_free); otherwise the
			 * direction blocks. The band is the rule index, so it must fit the
			 * byte. A combined cap meters both directions through one counter,
			 * so it can only throttle if BOTH directions are shapeable - if just
			 * one is, throttling half a shared pool while blocking the other is
			 * incoherent, so it blocks. Use a separate download cap if you want
			 * download to keep flowing while QoS or the limiter owns the WAN.
			 */
			is_throttle = (atoi(action) == QUOTA_ACT_THROTTLE);
			tdl_kbit = atoi(tdl);
			tul_kbit = atoi(tul);
			over_dl_band = is_throttle && dl_free && tdl_kbit > 0 && idx <= QUOTA_MARK_BAND_MAX;
			over_ul_band = is_throttle && ul_free && tul_kbit > 0 && idx <= QUOTA_MARK_BAND_MAX;
			snprintf(band, sizeof(band), "0x%x", idx << QUOTA_MARK_BAND_SHIFT);
			shape_dl = shape_ul = 0;

			if (*dlimit) {
				dmask = over_dl_band ? QUOTA_MARK_BAND_MASK : QUOTA_MARK_DEATH;
				strlcpy(dmark, over_dl_band ? band : QUOTA_MARK_DEATH, sizeof(dmark));
				if (over_dl_band)
					shape_dl = tdl_kbit;

				snprintf(idbuf, sizeof(idbuf), "quota_%s_d", idstr);
				if (idf)
					fprintf(idf, "%s\n", idbuf);
				quota_emit("quota_in", idbuf,
				           shared ? "combined" : "individual_dst",
				           "", dlimit, resetargs, skip_test, addrs, naddrs, "d",
				           dmark, dmask);
			}

			if (*ulimit) {
				umask = over_ul_band ? QUOTA_MARK_BAND_MASK : QUOTA_MARK_DEATH;
				strlcpy(umark, over_ul_band ? band : QUOTA_MARK_DEATH, sizeof(umark));
				if (over_ul_band)
					shape_ul = tul_kbit;

				snprintf(idbuf, sizeof(idbuf), "quota_%s_u", idstr);
				if (idf)
					fprintf(idf, "%s\n", idbuf);
				quota_emit("quota_out", idbuf,
				           shared ? "combined" : "individual_src",
				           "", ulimit, resetargs, skip_test, addrs, naddrs, "s",
				           umark, umask);
			}

			if (*climit) {
				/*
				 * A combined cap meters both directions in one counter, so its
				 * one bandwidth rule matches "sd" (both). The subnet is only
				 * needed by individual_local (it identifies the local host); a
				 * shared pool uses plain combined and needs none.
				 */
				const char *ctype = shared ? "combined" : "individual_local";
				const char *csub = shared ? "" : subnet;
				int comb_band = over_dl_band && over_ul_band;
				const char *cmark = comb_band ? band : QUOTA_MARK_DEATH;
				const char *cmask = comb_band ? QUOTA_MARK_BAND_MASK : QUOTA_MARK_DEATH;

				if (comb_band) {
					shape_dl = tdl_kbit;
					shape_ul = tul_kbit;
				}

				snprintf(idbuf, sizeof(idbuf), "quota_%s_c", idstr);
				if (idf)
					fprintf(idf, "%s\n", idbuf);
				quota_emit("quota_comb", idbuf, ctype, csub,
				           climit, resetargs, skip_test, addrs, naddrs, "sd",
				           cmark, cmask);
			}

			/*
			 * Record the penalty speeds so start_quota_shaping can build the tc
			 * class for this band: "<idx> <download kbit> <upload kbit>", 0 for a
			 * direction that blocks rather than throttles.
			 */
			if (shapef && (shape_dl || shape_ul))
				fprintf(shapef, "%d %d %d\n", idx, shape_dl, shape_ul);
		}
		free(buf);

		if (idf) {
			fclose(idf);
			idf = NULL;
		}
		if (shapef) {
			fclose(shapef);
			shapef = NULL;
		}
	}

	/*
	 * Enforcement. Anything carrying the death mark is dropped; doing it in
	 * one place rather than per quota rule keeps a single choke point.
	 */
	ip46t_write(ipv6_enabled,
	            "-A FORWARD -m mark --mark %s/%s -j DROP\n"
	            "-A INPUT -m mark --mark %s/%s -j DROP\n"
	            "-A OUTPUT -m mark --mark %s/%s -j DROP\n",
	            QUOTA_MARK_DEATH, QUOTA_MARK_DEATH,
	            QUOTA_MARK_DEATH, QUOTA_MARK_DEATH,
	            QUOTA_MARK_DEATH, QUOTA_MARK_DEATH);
}

/* where usage counters are persisted; see quota_path in defaults.c */
static void quota_backup_dir(char *buf, size_t bufsz)
{
	const char *p = nvram_safe_get("quota_path");

	/*
	 * With no path configured the counters land on tmpfs. That still earns
	 * its keep - the firewall is rebuilt on every WAN transition and config
	 * change, and this is what carries usage across those - but it is lost
	 * on reboot. Pointing quota_path at jffs/USB is what makes it survive,
	 * exactly as rstats_path/cstats_path work.
	 */
	if (*p)
		snprintf(buf, bufsz, "%s", p);
	else
		snprintf(buf, bufsz, "%s", QUOTA_IDS_DIR);
}

/* penalty-class shaper, mirrors bwlimit's /etc/bwlimit */
#ifndef QUOTA_SHAPING_SCRIPT
#define QUOTA_SHAPING_SCRIPT	"/etc/quota_shaping"
#endif
static const char *quota_shaping_script = QUOTA_SHAPING_SCRIPT;
#ifdef TCONFIG_BCMARM
static const char *quota_leaf_qdisc = "fq_codel";
#else
static const char *quota_leaf_qdisc = "sfq perturb 10";
#endif

/*
 * One throttle band, read back from QUOTA_SHAPE_FILE (written by ipt_quotas).
 * dl/ul are the penalty speeds in kbit/s; 0 means that direction blocks rather
 * than throttles, so no class is built for it.
 */
struct quota_band {
	int idx;
	int dl;
	int ul;
};

static int quota_read_bands(struct quota_band *b, int max, int *has_dl, int *has_ul)
{
	FILE *f;
	int n = 0;

	*has_dl = *has_ul = 0;

	if ((f = fopen(QUOTA_SHAPE_FILE, "r")) == NULL)
		return 0;

	while (n < max && fscanf(f, "%d %d %d", &b[n].idx, &b[n].dl, &b[n].ul) == 3) {
		if (b[n].idx < 1 || b[n].idx > QUOTA_MARK_BAND_MAX)
			continue;
		if (b[n].dl > 0)
			*has_dl = 1;
		if (b[n].ul > 0)
			*has_ul = 1;
		n++;
	}

	fclose(f);
	return n;
}

/*
 * Build the tc penalty classes for speed-limited quotas.
 *
 * A rule that is over its cap and set to limit-speed writes its band (its rule
 * index) into the packet mark instead of the death mark; the classes here catch
 * that mark and rate-limit it. Same shaping model as bwlimit.c, so no IFB is
 * needed: download is shaped on the client's LAN bridge egress, upload on the
 * WAN egress, and the mark set in mangle FORWARD rides through to both.
 *
 * Which side we may shape is decided when the bands are generated (dl_free/
 * ul_free in ipt_quotas): download needs the bandwidth limiter off, upload needs
 * the limiter and QoS off, because they own those root qdiscs. So by the time a
 * band reaches the shaping file its device is guaranteed free, and re-checking
 * here would only risk disagreeing with what the marks already committed to.
 */
void start_quota_shaping(void)
{
	struct quota_band bands[QUOTA_MARK_BAND_MAX + 1];
	char brdev[BRIDGE_COUNT][IFNAMSIZ];
	int nbr = 0;
	int nband, has_dl, has_ul;
	int dl_free, ul_free;
	int i, k, classid, handle;
	unsigned int mark;
	const char *waniface, *ibw, *obw;
	FILE *tc;

	unlink(quota_shaping_script);

	if (!nvram_get_int("quota_enable"))
		return;

	nband = quota_read_bands(bands, QUOTA_MARK_BAND_MAX + 1, &has_dl, &has_ul);
	if (nband == 0)
		return;			/* no speed-limit rules, nothing to shape */

	dl_free = !nvram_get_int("bwl_enable");
	ul_free = dl_free && !nvram_get_int("qos_enable");

	/* the LAN bridges that carry throttled download traffic */
	if (dl_free && has_dl) {
		for (i = 0; i < BRIDGE_COUNT; i++) {
			char nv[24];

			if (i == 0)
				strlcpy(nv, "lan_ipaddr", sizeof(nv));
			else
				snprintf(nv, sizeof(nv), "lan%d_ipaddr", i);
			if (!*nvram_safe_get(nv))
				continue;

			snprintf(brdev[nbr], IFNAMSIZ, "br%d", i);
			nbr++;
		}
	}

	if (nbr == 0 && !(ul_free && has_ul))
		return;			/* everything the bands wanted is unshapeable here */

	waniface = nvram_safe_get("wan_iface");
	ibw = nvram_safe_get("wan_qos_ibw");	/* download link speed, kbit */
	obw = nvram_safe_get("wan_qos_obw");	/* upload link speed, kbit */
	if (!*ibw || !strcmp(ibw, "0"))
		ibw = "1000000";
	if (!*obw || !strcmp(obw, "0"))
		obw = "1000000";

	if ((tc = fopen(quota_shaping_script, "w")) == NULL) {
		logerr(__FUNCTION__, __LINE__, quota_shaping_script);
		return;
	}

	fprintf(tc, "#!/bin/sh\n"
	            "case \"$1\" in\n"
	            "start)\n");

	/* one htb root per download bridge, and one on the WAN for upload */
	for (k = 0; k < nbr; k++)
		fprintf(tc, "\ttc qdisc del dev %s root 2>/dev/null\n"
		            "\ttc qdisc add dev %s root handle 1: htb\n"
		            "\ttc class add dev %s parent 1: classid 1:1 htb rate %skbit\n",
		            brdev[k], brdev[k], brdev[k], ibw);

	if (ul_free && has_ul && *waniface)
		fprintf(tc, "\ttc qdisc del dev %s root 2>/dev/null\n"
		            "\ttc qdisc add dev %s root handle 1: htb\n"
		            "\ttc class add dev %s parent 1: classid 1:1 htb rate %skbit\n",
		            waniface, waniface, waniface, obw);

	for (i = 0; i < nband; i++) {
		/* classid can't be 1:1 (the root class), so offset the band by 1 */
		classid = bands[i].idx + 1;
		handle = classid;
		mark = (unsigned int)bands[i].idx << QUOTA_MARK_BAND_SHIFT;

		if (bands[i].dl > 0) {
			for (k = 0; k < nbr; k++)
				fprintf(tc, "\ttc class add dev %s parent 1:1 classid 1:%d htb rate %dkbit ceil %dkbit\n"
				            "\ttc qdisc add dev %s parent 1:%d handle %d: %s\n"
				            "\ttc filter add dev %s parent 1:0 prio 1 protocol all handle 0x%x/%s fw flowid 1:%d\n",
				            brdev[k], classid, bands[i].dl, bands[i].dl,
				            brdev[k], classid, handle, quota_leaf_qdisc,
				            brdev[k], mark, QUOTA_MARK_BAND_MASK, classid);
		}

		if (bands[i].ul > 0 && ul_free && *waniface) {
			fprintf(tc, "\ttc class add dev %s parent 1:1 classid 1:%d htb rate %dkbit ceil %dkbit\n"
			            "\ttc qdisc add dev %s parent 1:%d handle %d: %s\n"
			            "\ttc filter add dev %s parent 1:0 prio 1 protocol all handle 0x%x/%s fw flowid 1:%d\n",
			            waniface, classid, bands[i].ul, bands[i].ul,
			            waniface, classid, handle, quota_leaf_qdisc,
			            waniface, mark, QUOTA_MARK_BAND_MASK, classid);
		}
	}

	fprintf(tc, "\tlogger -t quotas \"speed limits started\"\n"
	            "\t;;\n"
	            "stop)\n");

	for (k = 0; k < nbr; k++)
		fprintf(tc, "\ttc qdisc del dev %s root 2>/dev/null\n", brdev[k]);
	if (ul_free && has_ul && *waniface)
		fprintf(tc, "\ttc qdisc del dev %s root 2>/dev/null\n", waniface);

	fprintf(tc, "\t;;\n"
	            "esac\n");

	fclose(tc);
	chmod(quota_shaping_script, 0700);

	eval((char *)quota_shaping_script, "start");
}

/* tear down the penalty classes; safe if the script was never written */
void stop_quota_shaping(void)
{
	if (f_exists(quota_shaping_script))
		eval((char *)quota_shaping_script, "stop");
}

/*
 * Save the live counters and release the xt_bandwidth ids the current rules
 * are holding. Must run before the new ruleset is generated and applied.
 *
 * xt_bandwidth keeps a global map of every --id in use and its checkentry
 * rejects an id that is already registered:
 *
 *     "%s is a duplicate id in this IP family"   ->  -EINVAL
 *
 * That never bites Gargoyle, which installs quota rules with individual
 * "iptables -A" calls: those round-trip the existing rules through userspace
 * with the kernel's ref_count pointer still embedded in the match data, so
 * checkentry takes the refcount path and never re-registers the id. Tomato64
 * rebuilds everything with iptables-restore, where every rule is parsed from
 * text and ref_count is NULL, so each restore tries to register ids that the
 * outgoing rules still hold. checkentry runs before the old table is torn
 * down, so the duplicate is real and the *entire mangle table* is rejected -
 * the first restore after boot succeeds and every one after it fails.
 *
 * Flushing the quota chains tears the old rules down in a transaction of its
 * own, which releases the ids before the restore asks for them again. That
 * also frees their counters, hence the save here and the restore in
 * start_quotas() - the same shape as save_webmon()/ipt_webmon() for the other
 * Gargoyle-derived match.
 *
 * The id file still describes the *live* rules at this point; ipt_quotas()
 * rewrites it later while generating the new ones.
 */
void quotas_pre_restore(void)
{
	static const char *chains[] = { "quota_in", "quota_out", "quota_comb", "quota_fwd", NULL };
	char dir[80];
	char *argv[6];
	int i;

	/* nothing was ever generated, so nothing is holding an id */
	if (!f_exists(QUOTA_IDS_FILE))
		return;

	quota_backup_dir(dir, sizeof(dir));
	mkdir(dir, 0755);
	eval("/usr/sbin/quota_backup", "save", dir);

	for (i = 0; chains[i] != NULL; i++) {
		argv[0] = "iptables";
		argv[1] = "-t";
		argv[2] = "mangle";
		argv[3] = "-F";
		argv[4] = (char *)chains[i];
		argv[5] = NULL;

		/*
		 * The chains are gone whenever the previous restore failed, so a
		 * miss here is expected rather than exceptional - silence it.
		 */
		_eval(argv, ">/dev/null", 0, NULL);

#ifdef TCONFIG_IPV6
		if (ipv6_enabled) {
			argv[0] = "ip6tables";
			_eval(argv, ">/dev/null", 0, NULL);
		}
#endif
	}
}

/* is name one of the ids in the file rc just wrote? */
static int quota_id_is_live(const char *name)
{
	char line[80];		/* comfortably over the "quota_<digits>_x" rc writes */
	FILE *f;
	int live = 0;

	if ((f = fopen(QUOTA_IDS_FILE, "r")) == NULL)
		return 0;

	while (!live && fgets(line, sizeof(line), f) != NULL) {
		char *p = line;
		char *nl;

		while (*p == ' ' || *p == '\t')
			p++;
		if ((nl = strpbrk(p, "\r\n")) != NULL)
			*nl = '\0';
		if (*p == '\0' || *p == '#')
			continue;

		live = (strcmp(p, name) == 0);
	}

	fclose(f);

	return live;
}

/*
 * Drop usage files belonging to rules that no longer exist.
 *
 * Ids are never reused (see the rule identity note by QUOTA_FIELDS), so a file
 * whose id isn't in the current id list belongs to a deleted rule, or to one
 * whose address changed and was reissued. Without this they accumulate forever
 * on a persistent quota_path - and, more importantly, nothing would stop a
 * later rule from being handed one of them.
 *
 * Two things keep this from deleting more than it should:
 *
 *  - it only ever considers names of the form "quota_<digits>_<d|u|c>", the
 *    ones this feature creates. quota_path is user-supplied and may well point
 *    at a directory holding other things (a jffs or USB mount root), so an
 *    unconditional sweep of it would be destructive.
 *
 *  - the caller must have confirmed the id file exists. ipt_quotas() unlinks
 *    it whenever it declines to build rules - quotas off, no WAN up yet, clock
 *    not yet synced - and an absent file read as "no rule is live" would wipe
 *    every saved counter on a perfectly ordinary boot.
 */
static void quota_prune_backups(const char *dir)
{
	char path[512];
	struct dirent *de;
	DIR *d;

	if ((d = opendir(dir)) == NULL)
		return;

	while ((de = readdir(d)) != NULL) {
		const char *n = de->d_name;
		size_t len = strlen(n);
		size_t ndigits;

		if (strncmp(n, "quota_", 6) != 0)
			continue;

		ndigits = strspn(n + 6, "0123456789");
		if (ndigits == 0 || len != 6 + ndigits + 2 || n[6 + ndigits] != '_')
			continue;
		if (n[len - 1] != 'd' && n[len - 1] != 'u' && n[len - 1] != 'c')
			continue;

		if (quota_id_is_live(n))
			continue;

		snprintf(path, sizeof(path), "%s/%s", dir, n);
		if (unlink(path) == 0)
			logmsg(LOG_DEBUG, "*** %s: dropped stale usage file %s", __FUNCTION__, n);
	}

	closedir(d);
}

/*
 * Restore saved usage into the rules ipt_quotas() just created, and arrange
 * for it to be written back periodically.
 *
 * Called after the firewall ruleset has been applied - the counters cannot be
 * populated until the bandwidth rules actually exist in the kernel.
 */
void start_quotas(void)
{
	char dir[80];
	char sched[160];
	int stime;

	/* leave no cron job behind if quotas got turned off */
	eval("cru", "d", "quota_backup");

	if (!nvram_get_int("quota_enable")) {
		stop_quota_shaping();		/* clear any classes a prior config left */
		return;
	}

	/* build the speed-limit penalty classes for any throttle rules */
	start_quota_shaping();

	quota_backup_dir(dir, sizeof(dir));
	mkdir(dir, 0755);

	/*
	 * Only safe once the id file is known to describe the rules that were just
	 * built - see quota_prune_backups(). ipt_quotas() unlinks it when it builds
	 * nothing, and pruning against a missing list would delete every counter.
	 */
	if (f_exists(QUOTA_IDS_FILE))
		quota_prune_backups(dir);

	eval("/usr/sbin/quota_backup", "restore", dir);

	stime = nvram_get_int("quota_stime");
	if (stime < 1 || stime > 24)
		stime = 4;

	snprintf(sched, sizeof(sched), "0 */%d * * * /usr/sbin/quota_backup save %s", stime, dir);
	eval("cru", "a", "quota_backup", sched);
}

/*
 * Save usage on the way down so a clean reboot or shutdown doesn't lose
 * whatever has accumulated since the last periodic save. Called from
 * stop_services(), which runs while the bandwidth rules are still loaded and
 * before USB storage is unmounted.
 *
 * Note this only reaches persistent storage when quota_path points at one.
 * Left empty the counters are written to tmpfs and are gone with the reboot -
 * see quota_backup_dir().
 */
void stop_quotas(void)
{
	char dir[80];

	eval("cru", "d", "quota_backup");
	stop_quota_shaping();

	/*
	 * The id file is what says rules were actually generated - quota_enable can
	 * be true while a deferred clock, or no throttle-free device, meant nothing
	 * was emitted.
	 */
	if (!f_exists(QUOTA_IDS_FILE))
		return;

	quota_backup_dir(dir, sizeof(dir));
	mkdir(dir, 0755);
	eval("/usr/sbin/quota_backup", "save", dir);
}

#else /* !TCONFIG_QUOTAS - quotas not built into this image */

/*
 * No-op stubs so the unconditional call sites in firewall.c / services.c link.
 * (The internal helpers, quota_clock_valid and the shaping functions, have no
 * external callers, so they need no stub.)
 */
void ipt_quotas(void) { }
void quotas_pre_restore(void) { }
void start_quotas(void) { }
void stop_quotas(void) { }

#endif /* TCONFIG_QUOTAS */
