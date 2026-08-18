/*
 * Tomato64 nDPI protocol table
 * Copyright (C) 2026 Tomato64 Project
 *
 * Everything that needs to know which protocols the nDPI library in this
 * build understands: the table itself, validation of a stored protocol
 * value, and bringing rules written against an older library up to date.
 *
 * The xt_ndpi kernel module publishes the protocol table of the nDPI library
 * it was built against as /proc/net/xt_ndpi/proto. libxt_ndpi.so has no
 * compiled in list of its own - it reads that same file to decide whether it
 * accepts "--proto <name>" - so the file is the authoritative answer for the
 * build that is actually running, and it follows the library across version
 * bumps for free.
 *
 * Format, as of nDPI 5.1.0:
 *
 *   #id     mark ~mask     name   # count #version 5.1.0
 *   00         0/0000ffff unknown          # 0 debug=0
 *   07         7/0000ffff http             # 0 debug=0 dpi
 *   fa        fa/0000ffff teams            # 0 debug=0
 *   1d9           disabled custom473       # 0
 *
 * Bitmap slots with no protocol assigned carry a mark of "disabled" and a
 * synthetic "customN" name. libxt_ndpi.so rejects those exactly the way it
 * rejects an unknown name, so they are not usable here either.
 *
 * The trailing " dpi" flag marks a protocol nDPI recognises from the payload,
 * as opposed to one it only knows by port or address. That distinction is
 * what "--inprogress" needs: libxt_ndpi.so refuses the option for a protocol
 * without a dissector, and a refused option fails the whole ruleset.
 *
 * The table cannot be read before the module is loaded - the whole
 * /proc/net/xt_ndpi/ directory is created by the module at netns init - so
 * callers must modprobe xt_ndpi first.
 */

#ifdef TOMATO64

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <syslog.h>

#include <bcmnvram.h>
#include "shutils.h"
#include "shared.h"

#define NDPI_PROTO_PATH		"/proc/net/xt_ndpi/proto"
#define NDPI_PROTO_MAX		1024	/* NDPI_MAX_NUM_STATIC_BITMAP is 512 today */
#define NDPI_NAME_SIZE		32	/* kernel side prints through a char[32] */

static char **ndpi_protos = NULL;	/* NULL terminated, kernel index order */
static char *ndpi_dissector = NULL;	/* one flag per ndpi_protos entry, same order */
static const char **ndpi_display = NULL;/* NULL terminated, sorted, for a UI list */

/*
 * Does a protocol table line end with the " dpi" flag?
 *
 * The kernel prints the flag as the last field of the trailing comment and
 * then adds one more space before the newline. libxt_ndpi.so reads it at a
 * fixed offset from the end of the line, which only works while that spacing
 * holds, so the field is matched here after the line has been trimmed.
 */
static int ndpi_line_dissector(const char *line)
{
	size_t n = strlen(line);

	while ((n > 0) && ((line[n - 1] == '\n') || (line[n - 1] == '\r') || (line[n - 1] == ' ')))
		n--;

	return ((n >= 4) && (strncmp(line + n - 4, " dpi", 4) == 0));
}

/*
 * Read the protocol table into ndpi_protos, once per process.
 *
 * Only entries the match can actually be used with are kept, which is the
 * same set libxt_ndpi.so will accept: everything that is named and not
 * disabled. The " dpi" flag of each kept entry is recorded alongside it.
 * Returns 1 when the table is available, 0 when it is not.
 */
static int ndpi_proto_load(void)
{
	FILE *fp;
	char buf[192], mark[NDPI_NAME_SIZE], name[NDPI_NAME_SIZE];
	char **list;
	char *dpi;
	unsigned int idx;
	int n, hdr;

	if (ndpi_protos)
		return 1;

	if ((fp = fopen(NDPI_PROTO_PATH, "r")) == NULL) {
		syslog(LOG_DEBUG, "nDPI: %s is not readable, is xt_ndpi loaded?", NDPI_PROTO_PATH);
		return 0;
	}

	if ((list = calloc(NDPI_PROTO_MAX + 1, sizeof(char *))) == NULL) {
		fclose(fp);
		return 0;
	}

	if ((dpi = calloc(NDPI_PROTO_MAX + 1, sizeof(char))) == NULL) {
		free(list);
		fclose(fp);
		return 0;
	}

	n = 0;
	hdr = 0;
	while (fgets(buf, sizeof(buf), fp)) {
		if (buf[0] == '#') {
			/* "#id mark ~mask name # count #version <release>" */
			if (strstr(buf, "#version"))
				hdr = 1;
			continue;
		}
		if (sscanf(buf, "%x %31s %31s", &idx, mark, name) != 3)
			continue;
		if (strncmp(mark, "disable", 7) == 0)	/* unassigned slot */
			continue;
		if (n >= NDPI_PROTO_MAX)
			break;
		if ((list[n] = strdup(name)) == NULL)
			break;
		dpi[n] = ndpi_line_dissector(buf);
		n++;
	}
	fclose(fp);

	/*
	 * No version header means this is not the file we know how to read.
	 * Throw the parse away rather than reject protocols over it.
	 */
	if (!hdr || n == 0) {
		syslog(LOG_WARNING, "nDPI: could not parse %s", NDPI_PROTO_PATH);
		while (--n >= 0)
			free(list[n]);
		free(list);
		free(dpi);

		return 0;
	}

	ndpi_protos = list;
	ndpi_dissector = dpi;

	return 1;
}

/*
 * Can the running xt_ndpi module match on this protocol name?
 *
 * Returns 1 yes, 0 no, -1 when the protocol table is unavailable and the
 * question cannot be answered. Callers must not reject a protocol on -1.
 *
 * The comparison is case insensitive to match libxt_ndpi.so, which resolves
 * names with strcasecmp().
 */
int ndpi_proto_valid(const char *name)
{
	char **p;

	if ((name == NULL) || (*name == 0))
		return -1;

	if (!ndpi_proto_load())
		return -1;

	for (p = ndpi_protos; *p; ++p) {
		if (strcasecmp(*p, name) == 0)
			return 1;
	}

	return 0;
}

static int ndpi_name_cmp(const void *a, const void *b)
{
	return strcmp(*(const char **)a, *(const char **)b);
}

/*
 * The protocol names of the running module, for building a selection list.
 *
 * Sorted, NULL terminated, and owned by the library - do not free it. NULL
 * when the table is unavailable. Filtered the way libxt_ndpi.so filters its
 * own --help output: "unknown" is reached through its own match option rather
 * than by name, and the "badproto_"/"free" slots are internal. The "customN"
 * slots are disabled and so were already dropped when the table was read.
 */
const char *const *ndpi_proto_names(void)
{
	const char **list;
	char **p;
	int n;

	if (ndpi_display)
		return (const char *const *)ndpi_display;

	if (!ndpi_proto_load())
		return NULL;

	for (n = 0, p = ndpi_protos; *p; ++p)
		n++;

	if ((list = calloc(n + 1, sizeof(char *))) == NULL)
		return NULL;

	for (n = 0, p = ndpi_protos; *p; ++p) {
		if ((strcmp(*p, "unknown") == 0) ||
		    (strncmp(*p, "badproto_", 9) == 0) ||
		    (strncmp(*p, "free", 4) == 0))
			continue;

		list[n++] = *p;
	}
	qsort(list, n, sizeof(list[0]), ndpi_name_cmp);
	ndpi_display = list;

	return (const char *const *)ndpi_display;
}

/*
 * Is a stored protocol value one the running xt_ndpi can be given?
 *
 * The GUI stores a single name, but the match itself takes a comma separated
 * list, an optional leading '-' to exclude a protocol, and the special name
 * "all", so a hand written value is checked the same way libxt_ndpi.so checks
 * it. Returns 1 when the whole value is usable, 0 when it is not, and copies
 * the offending name into bad. Unusable protocols are what an nDPI bump
 * leaves behind: skype_teams and skype_teamscall became teams and teamscall,
 * and a rule naming the old spelling takes the entire ruleset down with it,
 * since iptables-restore is all or nothing.
 */
int ndpi_proto_list_valid(const char *v, char *bad, const size_t bad_sz)
{
	char list[256];
	char *p, *n;
	int r;

	strlcpy(list, v, sizeof(list));

	p = list;
	while ((n = strsep(&p, ",")) != NULL) {
		if (*n == '-') /* exclude this protocol */
			n++;
		if ((*n == 0) || (strcmp(n, "all") == 0))
			continue;

		r = ndpi_proto_valid(n);
		if (r < 0) /* no protocol table, leave the rule alone */
			break;
		if (r == 0) {
			strlcpy(bad, n, bad_sz);
			return 0;
		}
	}

	return 1;
}

/*
 * Is a protocol one nDPI detects from the payload?
 *
 * Returns 1 yes, 0 no or unknown name, -1 when the protocol table is
 * unavailable. A protocol without a dissector is recognised from the port or
 * the address of the very first packet, so the answer also says whether the
 * classification of a flow can lag behind its first packet at all.
 */
int ndpi_proto_dissector(const char *name)
{
	char **p;
	int i;

	if ((name == NULL) || (*name == 0))
		return -1;

	if (!ndpi_proto_load())
		return -1;

	for (i = 0, p = ndpi_protos; *p; ++p, ++i) {
		if (strcasecmp(*p, name) == 0)
			return ndpi_dissector[i];
	}

	return 0;
}

/*
 * Can the whole stored value be handed to "--inprogress"?
 *
 * libxt_ndpi.so walks the protocol bitmask the value expands to and fails the
 * command for the first protocol without a dissector, so every name has to
 * carry one. Exclusions and "all" are answered no rather than expanded here:
 * they resolve against the running library rather than against the value, and
 * this has to fail closed. Unlike ndpi_proto_list_valid(), an unreadable
 * protocol table is also a no - that one keeps a rule the caller already
 * built, this one decides whether to add an option that takes the whole
 * ruleset down when it turns out to be wrong.
 *
 * Returns 1 when the value is usable, 0 when it is not, and copies the name
 * that decided it into bad, which is left empty when no single name did.
 */
int ndpi_proto_list_dissector(const char *v, char *bad, const size_t bad_sz)
{
	char list[256];
	char *p, *n;
	int r;

	*bad = 0;
	strlcpy(list, v, sizeof(list));

	p = list;
	while ((n = strsep(&p, ",")) != NULL) {
		if (*n == 0)
			continue;

		if ((*n == '-') || (strcmp(n, "all") == 0)) {
			strlcpy(bad, n, bad_sz);
			return 0;
		}

		r = ndpi_proto_dissector(n);
		if (r < 0) /* no protocol table, nothing can be vouched for */
			return 0;
		if (r == 0) {
			strlcpy(bad, n, bad_sz);
			return 0;
		}
	}

	return 1;
}

/*
 * nDPI protocols that were renamed by the library rather than dropped.
 *
 * Only renames belong here. A protocol that genuinely went away is left
 * alone: ipt_ndpi() skips the rule and logs it, which loses one rule, while
 * guessing a replacement or blanking the field would quietly turn a narrow
 * rule into one that matches everything.
 */
static const struct {
	const char *old;
	const char *new;
} ndpi_renamed[] = {
	{ "skype_teams",	"teams"		},	/* nDPI 4.x -> 5.x */
	{ "skype_teamscall",	"teamscall"	},
	{ NULL,			NULL		}
};

#define NDPI_MAX_NRULES		50	/* MAX_NRULES in restrict.c */

static const char *ndpi_rename_of(const char *name)
{
	int i;

	for (i = 0; ndpi_renamed[i].old; ++i) {
		if (strcasecmp(name, ndpi_renamed[i].old) != 0)
			continue;
		/*
		 * Only rename into something this build can actually match on,
		 * and only away from something it cannot. That keeps the table
		 * harmless if the library ever renames back.
		 */
		if ((ndpi_proto_valid(ndpi_renamed[i].old) == 0) && (ndpi_proto_valid(ndpi_renamed[i].new) == 1))
			return ndpi_renamed[i].new;

		break;
	}

	return NULL;
}

/*
 * Rewrite field <field> of every '>' separated record in src.
 *
 * Records are field lists separated by '<', which is how both qos_orules and
 * the match list inside an rrule are stored. Only the one field is examined,
 * so a rule described as "skype_teams" in its free text description is not
 * touched. Returns 1 when something was rewritten.
 */
static int ndpi_rename_field(char *dst, const size_t dst_sz, const char *src, const int field)
{
	char name[64];
	const char *p, *s, *r;
	size_t n, len;
	int idx, changed;

	p = src;
	idx = 0;
	len = 0;
	changed = 0;

	while (*p) {
		if (idx == field) {
			for (s = p; *p && (*p != '<') && (*p != '>'); ++p)
				;

			n = p - s;
			if (n >= sizeof(name)) /* not a protocol name, pass it through */
				r = NULL;
			else {
				memcpy(name, s, n);
				name[n] = 0;
				if ((r = ndpi_rename_of(name)) != NULL) {
					n = strlen(r);
					changed = 1;
				}
			}

			if ((len + n) >= dst_sz)
				return 0; /* would not fit, leave the value alone */

			memcpy(dst + len, r ? r : s, n);
			len += n;

			if (*p == 0)
				break;
		}

		if (*p == '<')
			idx++;
		else if (*p == '>')
			idx = 0;

		if ((len + 1) >= dst_sz)
			return 0;

		dst[len++] = *p++;
	}
	dst[len] = 0;

	return changed;
}

/*
 * Rewrite qos_orules, which is nothing but a record list.
 */
static int ndpi_migrate_qos(void)
{
	char *src, *dst;
	size_t sz;
	int changed;

	if (((src = nvram_get("qos_orules")) == NULL) || (*src == 0))
		return 0;

	sz = (strlen(src) * 2) + 1; /* a rename may lengthen the value */
	if ((dst = malloc(sz)) == NULL)
		return 0;

	/* addr_type<addr<proto<port_type<port<ndpi<bcount<dscp<class_prio<desc */
	changed = ndpi_rename_field(dst, sz, src, 5);
	if (changed) {
		nvram_set("qos_orules", dst);
		syslog(LOG_INFO, "nDPI: updated renamed protocols in qos_orules");
	}
	free(dst);

	return changed;
}

/*
 * Rewrite one rruleN, whose record list is the sixth of eight '|' separated
 * fields. The record list is cut out and put back rather than scanned in
 * place, so that a '<' anywhere in the free text http field cannot shift the
 * field counting onto the wrong token.
 */
static int ndpi_migrate_rrule(const char *name)
{
	char *src, *dst, *old, *new;
	const char *head, *tail;
	size_t sz;
	int i, changed;

	if (((src = nvram_get(name)) == NULL) || (*src == 0))
		return 0;

	/* enabled|time|-|-|comps|matches|http|- */
	for (head = src, i = 0; i < 5; ++i) {
		if ((head = strchr(head, '|')) == NULL)
			return 0;
		head++;
	}
	if ((tail = strchr(head, '|')) == NULL)
		return 0;

	sz = ((size_t)(tail - head) * 2) + 1; /* a rename may lengthen the value */
	if ((old = malloc((size_t)(tail - head) + 1)) == NULL)
		return 0;
	if ((new = malloc(sz)) == NULL) {
		free(old);
		return 0;
	}
	strlcpy(old, head, (size_t)(tail - head) + 1);

	/* pproto<dir<pport<ndpi<addr_type<addr */
	changed = ndpi_rename_field(new, sz, old, 3);
	if (changed) {
		sz = strlen(src) + strlen(new) + 1;
		if ((dst = malloc(sz)) != NULL) {
			snprintf(dst, sz, "%.*s%s%s", (int)(head - src), src, new, tail);
			nvram_set(name, dst);
			syslog(LOG_INFO, "nDPI: updated renamed protocols in %s", name);
			free(dst);
		}
		else
			changed = 0;
	}
	free(new);
	free(old);

	return changed;
}

/*
 * Bring stored rules up to date with the nDPI library in this build.
 *
 * Defaults only ever reach nvram that has never been written, so a router
 * configured before a protocol was renamed - or restored from a backup that
 * old - keeps the dead name forever. One dead name is enough to make
 * iptables-restore reject the whole ruleset, which reads as a WAN outage
 * rather than as a QoS problem, so the rename is applied once at boot.
 */
void ndpi_migrate_rules(void)
{
	char rrule[32];
	char *v;
	int i, n, dirty;

	/*
	 * Cheap gate first: nothing to do on the overwhelming majority of
	 * routers, and this must not load xt_ndpi just to find that out.
	 */
	dirty = 0;
	for (i = 0; ndpi_renamed[i].old && !dirty; ++i) {
		if (((v = nvram_get("qos_orules")) != NULL) && (strstr(v, ndpi_renamed[i].old) != NULL))
			dirty = 1;

		for (n = 0; (n < NDPI_MAX_NRULES) && !dirty; ++n) {
			snprintf(rrule, sizeof(rrule), "rrule%d", n);
			if (((v = nvram_get(rrule)) != NULL) && (strstr(v, ndpi_renamed[i].old) != NULL))
				dirty = 1;
		}
	}
	if (!dirty)
		return;

	/* ndpi_rename_of() needs the protocol table, which needs the module */
	eval("modprobe", "-s", "xt_ndpi");

	dirty = ndpi_migrate_qos();

	for (n = 0; n < NDPI_MAX_NRULES; ++n) {
		snprintf(rrule, sizeof(rrule), "rrule%d", n);
		dirty |= ndpi_migrate_rrule(rrule);
	}

	if (dirty)
		nvram_commit();
}

#endif /* TOMATO64 */
