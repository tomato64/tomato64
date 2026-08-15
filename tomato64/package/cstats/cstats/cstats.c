/*
 *
 * cstats
 * Copyright (C) 2011-2012 Augusto Bott
 *
 * based on rstats
 * Copyright (C) 2006-2009 Jonathan Zarate
 *
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * Fixes/updates (C) 2018 - 2026 pedro
 * https://freshtomato.org/
 *
 */


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <time.h>
#include <sys/types.h>
#include <sys/sysinfo.h>
#include <sys/stat.h>
#include <stdint.h>
#include <syslog.h>

#include <bcmnvram.h>
#include <shutils.h>
#include <shared.h>
#include <tree.h>
#ifdef USE_ZLIB
 #include <zlib.h>
#endif

#include "cstats.h"


long save_utime;
char save_path[96];
long uptime;

volatile int gothup = 0;
volatile int gotuser = 0;
volatile int gotterm = 0;
static int node_count = 0; /* active node counter */

Tree tree = TREE_INITIALIZER(Node_compare);

#ifdef DEBUG_CSTATS
void Node_print(Node *self, FILE *stream)
{
	fprintf(stream, "%s", self->ipaddr);
}

void Node_printer(Node *self, void *stream)
{
	Node_print(self, (FILE *)stream);
	fprintf((FILE *)stream, " ");
}

void Tree_info(void)
{
	TREE_FORWARD_APPLY(&tree, _Node, linkage, Node_printer, stdout);
	logmsg(LOG_DEBUG, "Tree depth = %d", TREE_DEPTH(&tree, linkage));
}
#endif

static void free_all_nodes(void)
{
	Node *node;
	while (tree.th_root != NULL) {
		node = tree.th_root;
		TREE_REMOVE(&tree, _Node, linkage, node);
		free(node);
		node_count--;
	}
	node_count = 0;
}

Node *Node_new(char *ipaddr)
{
	Node *self;
	if ((self = malloc(sizeof(Node))) != NULL) {
		memset(self, 0, sizeof(Node));
		self->id = CURRENT_ID;
		strncpy(self->ipaddr, ipaddr, INET_ADDRSTRLEN);

		if (node_count >= MAX_NODES) {
			logmsg(LOG_WARNING, "*** %s: max nodes reached (%d), skipping %s", __FUNCTION__, MAX_NODES, ipaddr);
			free(self);
			return NULL;
		}
		node_count++;

		logmsg(LOG_DEBUG, "*** %s: new node ip=%s, version=%d, sizeof(Node)=%d (bytes)", __FUNCTION__, self->ipaddr, self->id, sizeof(Node));
	}

	return self;
}

int Node_compare(Node *lhs, Node *rhs)
{
	return strncmp(lhs->ipaddr, rhs->ipaddr, INET_ADDRSTRLEN);
}

void Node_save(Node *self, void *t)
{
	node_print_mode_t *info = (node_print_mode_t *)t;
#ifdef USE_ZLIB
	int written, errnum;
	const char *errmsg;

	written = gzwrite(info->gzstream, self, sizeof(Node));
	if (written != sizeof(Node)) {
		errmsg = gzerror(info->gzstream, &errnum);
		logmsg(LOG_ERR, "*** %s: gzwrite failed for Node %s: %s (errnum=%d)", __FUNCTION__, self->ipaddr, errmsg, errnum);
		info->kn = -1;
		return;
	}
	info->kn++;
#else
	if (fwrite(self, sizeof(Node), 1, info->stream) > 0)
		info->kn++;
#endif
}

static int get_stime(void)
{
	int t;
	t = nvram_get_int("cstats_stime");
	if (t < 1)
		t = 1;
	else if (t > 8760)
		t = 8760;

	return t * SHOUR;
}

static int save_history_from_tree(const char *fname)
{
	node_print_mode_t info;
	char hgz[256];

	info.kn = 0;
	logmsg(LOG_DEBUG, "*** %s: fname=%s", __FUNCTION__, fname);

	snprintf(hgz, sizeof(hgz), "%s.gz", fname);

#ifdef USE_ZLIB
	gzFile f;
	int err;

	if (!(f = gzopen(hgz, "wb8"))) {
		logmsg(LOG_ERR, "*** %s: cannot open %s for writing (%s)", __FUNCTION__, hgz, strerror(errno));
		return -1;
	}
	info.mode = 0;
	info.gzstream = f;
	TREE_FORWARD_APPLY(&tree, _Node, linkage, Node_save, &info);
	if (info.kn == -1) {
		gzclose(f);
		return -1;
	}
	err = gzclose(f);
		if (err != Z_OK) {
		logmsg(LOG_ERR, "*** %s: gzclose failed for %s: error %d", __FUNCTION__, hgz, err);
		return -1;
	}
#else
	FILE *f;
	char cmd[256];

	unlink(uncomp_fn);

	if (!(f = fopen(uncomp_fn, "wb"))) {
		logmsg(LOG_ERR, "*** %s: cannot open %s for writing (%s)", __FUNCTION__, uncomp_fn, strerror(errno));
		return -1;
	}
	info.mode = 0;
	info.stream = f;
	TREE_FORWARD_APPLY(&tree, _Node, linkage, Node_save, &info);
	fclose(f);

	/* direct compression to .gz file – safer */
	snprintf(cmd, sizeof(cmd), "gzip -c %s > %s", uncomp_fn, hgz);
	if (system(cmd) == 0) {
		unlink(uncomp_fn);
	}
	else {
		logmsg(LOG_ERR, "*** %s: gzip failed for %s", __FUNCTION__, fname);
		/* optional: leave uncompressed as backup */
		// rename(uncomp_fn, fname);
		unlink(uncomp_fn);
		return -1;
	}
#endif

	return info.kn;
}

static void save(int quick)
{
	int i, n, b;
	char hgz[256], tmp[256], bak[256], bkp[256];
	time_t now;
	struct tm *tms;
	static int lastbak = -1;

	logmsg(LOG_DEBUG, "*** %s: quick=%d", __FUNCTION__, quick);

	f_write(stime_fn, &save_utime, sizeof(save_utime), 0, 0);

	n = save_history_from_tree(history_fn);
	logmsg(LOG_DEBUG, "*** %s: saved %d records from tree on file %s", __FUNCTION__, n, history_fn);

	logmsg(LOG_DEBUG, "*** %s: write source=%s", __FUNCTION__, save_path);
	f_write_string(source_fn, save_path, 0, 0);

	if (quick)
		return;

	snprintf(hgz, sizeof(hgz), "%s.gz", history_fn);

	if (save_path[0] != 0) {
		strlcpy(tmp, save_path, sizeof(tmp));
		strlcat(tmp, ".tmp", sizeof(tmp));

		for (i = 15; i > 0; --i) {
			if (!wait_action_idle(10))
				logmsg(LOG_DEBUG, "*** %s: busy, not saving", __FUNCTION__);
			else {
				logmsg(LOG_DEBUG, "*** %s: cp %s %s", __FUNCTION__, hgz, tmp);
				if (eval("cp", hgz, tmp) == 0) {
					logmsg(LOG_DEBUG, "*** %s: copy ok", __FUNCTION__);

					if (!nvram_match("cstats_bak", "0")) {
						now = time(0);
						tms = localtime(&now);
						if (lastbak != tms->tm_yday) {
							strlcpy(bak, save_path, sizeof(bak));
							n = strlen(bak);
							if ((n > 3) && (strcmp(bak + (n - 3), ".gz") == 0))
								n -= 3;

							strlcpy(bkp, bak, sizeof(bkp));
							for (b = HI_BACK-1; b > 0; --b) {
								snprintf(bkp + n, sizeof(bkp) - n, "_%d.bak", b + 1);
								snprintf(bak + n, sizeof(bak) - n, "_%d.bak", b);
								rename(bak, bkp);
							}
							if (eval("cp", "-p", save_path, bak) == 0)
								lastbak = tms->tm_yday;
						}
					}

					logmsg(LOG_DEBUG, "*** %s: rename %s %s", __FUNCTION__, tmp, save_path);
					if (rename(tmp, save_path) == 0) {
						logmsg(LOG_DEBUG, "*** %s: rename ok", __FUNCTION__);
						break;
					}
				}
			}

			/* might not be ready */
			sleep(3);
			if (gotterm)
				break;
		}
	}
}

static int load_history_to_tree(const char *fname)
{
	int n;
#ifdef USE_ZLIB
	gzFile f;
	int read_bytes, errnum, err;
	const char *errmsg;
#else
	FILE *f;
	char cmd[256];
#endif
	Node tmp;
	Node *ptr;
	char *exclude;

	exclude = nvram_safe_get("cstats_exclude");
	logmsg(LOG_DEBUG, "*** %s: cstats_exclude='%s' fname=%s", __FUNCTION__, exclude, fname);
	unlink(uncomp_fn);

	n = -1;

#ifdef USE_ZLIB
	if (!(f = gzopen(fname, "rb"))) {
		logmsg(LOG_ERR, "*** %s: cannot open %s for reading (%s)", __FUNCTION__, fname, strerror(errno));
		return -1;
	}
	n = 0;
	while (1) {
		read_bytes = gzread(f, &tmp, sizeof(Node));
		if (read_bytes == 0)
			break; /* EOF */

		if (read_bytes != sizeof(Node)) {
			errmsg = gzerror(f, &errnum);
			logmsg(LOG_ERR, "*** %s: gzread failed: %s (errnum=%d, read_bytes=%d)", __FUNCTION__, errmsg, errnum, read_bytes);
			gzclose(f);
			return -1;
		}

		if (find_word(exclude, tmp.ipaddr)) {
			logmsg(LOG_DEBUG, "*** %s: not loading excluded ip '%s'", __FUNCTION__, tmp.ipaddr);
			continue;
		}

		if (tmp.id == CURRENT_ID) {
			logmsg(LOG_DEBUG, "*** %s: found data for ip %s", __FUNCTION__, tmp.ipaddr);

			ptr = TREE_FIND(&tree, _Node, linkage, &tmp);
			if (ptr) {
				logmsg(LOG_DEBUG, "*** %s: removing/reloading new data for ip %s", __FUNCTION__, ptr->ipaddr);
				TREE_REMOVE(&tree, _Node, linkage, ptr);
				free(ptr);
				node_count--;
				ptr = NULL;
			}

			TREE_INSERT(&tree, _Node, linkage, Node_new(tmp.ipaddr));

			ptr = TREE_FIND(&tree, _Node, linkage, &tmp);

			if (ptr) { /* Node_new could return NULL at limit */
				memcpy(ptr->daily, &tmp.daily, sizeof(data_t) * MAX_NDAILY);
				ptr->dailyp = tmp.dailyp;
				memcpy(ptr->monthly, &tmp.monthly, sizeof(data_t) * MAX_NMONTHLY);
				ptr->monthlyp = tmp.monthlyp;

				ptr->utime = tmp.utime;
				memcpy(ptr->speed, &tmp.speed, sizeof(uint64_t) * MAX_NSPEED * MAX_COUNTER);
				memcpy(ptr->last, &tmp.last, sizeof(uint64_t) * MAX_COUNTER);
				ptr->tail = tmp.tail;
				ptr->sync = -1;

				if (ptr->utime > uptime) {
					ptr->utime = uptime;
					ptr->sync = 1;
				}

				++n;
			}
		}
		else
			logmsg(LOG_DEBUG, "*** %s: data for ip '%s' version %d not loaded (current version is %d)", __FUNCTION__, tmp.ipaddr, tmp.id, CURRENT_ID);
	}
	err = gzclose(f);
	if (err != Z_OK) {
		logmsg(LOG_ERR, "*** %s: gzclose failed for %s: error %d", __FUNCTION__, fname, err);
		return -1;
	}
#else
	snprintf(cmd, sizeof(cmd), "gzip -dc %s > %s", fname, uncomp_fn);
	if (system(cmd) != 0) {
		logmsg(LOG_ERR, "*** %s: decompress failed: %s", __FUNCTION__, cmd);
		return -1;
	}

	if ((f = fopen(uncomp_fn, "rb"))) {
		n = 0;
		while (fread(&tmp, sizeof(Node), 1, f) > 0) {
			if (find_word(exclude, tmp.ipaddr)) {
				logmsg(LOG_DEBUG, "*** %s: not loading excluded ip '%s'", __FUNCTION__, tmp.ipaddr);
				continue;
			}

			if (tmp.id == CURRENT_ID) {
				logmsg(LOG_DEBUG, "*** %s: found data for ip %s", __FUNCTION__, tmp.ipaddr);

				ptr = TREE_FIND(&tree, _Node, linkage, &tmp);
				if (ptr) {
					logmsg(LOG_DEBUG, "*** %s: removing/reloading new data for ip %s", __FUNCTION__, ptr->ipaddr);
					TREE_REMOVE(&tree, _Node, linkage, ptr);
					free(ptr);
					node_count--;
					ptr = NULL;
				}

				TREE_INSERT(&tree, _Node, linkage, Node_new(tmp.ipaddr));

				ptr = TREE_FIND(&tree, _Node, linkage, &tmp);

				if (ptr) {  /* Node_new could return NULL at limit */
					memcpy(ptr->daily, &tmp.daily, sizeof(data_t) * MAX_NDAILY);
					ptr->dailyp = tmp.dailyp;
					memcpy(ptr->monthly, &tmp.monthly, sizeof(data_t) * MAX_NMONTHLY);
					ptr->monthlyp = tmp.monthlyp;

					ptr->utime = tmp.utime;
					memcpy(ptr->speed, &tmp.speed, sizeof(uint64_t) * MAX_NSPEED * MAX_COUNTER);
					memcpy(ptr->last, &tmp.last, sizeof(uint64_t) * MAX_COUNTER);
					ptr->tail = tmp.tail;
					ptr->sync = -1;

					if (ptr->utime > uptime) {
						ptr->utime = uptime;
						ptr->sync = 1;
					}

					++n;
				}
			}
			else
				logmsg(LOG_DEBUG, "*** %s: data for ip '%s' version %d not loaded (current version is %d)", __FUNCTION__, tmp.ipaddr, tmp.id, CURRENT_ID);
		}

		fclose(f);
	}

	unlink(uncomp_fn);
#endif

	if (n == -1)
		logmsg(LOG_DEBUG, "*** %s: Failed to parse the data file!", __FUNCTION__);
	else
		logmsg(LOG_DEBUG, "*** %s: Loaded %d records", __FUNCTION__, n);

	return n;
}

static int load_history(const char *fname)
{
	logmsg(LOG_DEBUG, "*** %s: fname=%s", __FUNCTION__, fname);

	return load_history_to_tree(fname);
}

/* Try loading from the backup versions.
 * We'll try from oldest to newest, then
 * retry the requested one again last. In case the drive mounts while
 * we are trying to find a good version.
 */
static int try_hardway(const char *fname)
{
	char fn[256];
	int n, b, found = 0;

	strlcpy(fn, fname, sizeof(fn));
	n = strlen(fn);
	if ((n > 3) && (strcmp(fn + (n - 3), ".gz") == 0))
		n -= 3;

	for (b = HI_BACK; b > 0; --b) {
		snprintf(fn + n, sizeof(fn) - n, "_%d.bak", b);
		found |= load_history(fn);
	}
	found |= load_history(fname);

	return found;
}

static void load_new(void)
{
	char hgz[256];

	snprintf(hgz, sizeof(hgz), "%s.gz.new", history_fn);
	if (load_history(hgz) >= 0)
		save(0);

	unlink(hgz);
}

static void load(int new)
{
	int i, n;
	long t;
	char hgz[256];
	unsigned char mac[6];

	if (new) {
		free_all_nodes(); /* clear old data with --new */
		node_count = 0;
	}

	uptime = get_uptime();

	logmsg(LOG_DEBUG, "*** %s: new=%d, uptime=%lu", __FUNCTION__, new, uptime);

	strlcpy(save_path, nvram_safe_get("cstats_path"), sizeof(save_path) - 32);
	if (((n = strlen(save_path)) > 0) && (save_path[n - 1] == '/')) {
		ether_atoe(nvram_safe_get("lan_hwaddr"), mac);
		snprintf(save_path + n, sizeof(save_path) - n, "tomato_cstats_%02x%02x%02x%02x%02x%02x.gz",
		         mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
	}

	if (f_read(stime_fn, &save_utime, sizeof(save_utime)) != sizeof(save_utime))
		save_utime = 0;

	t = uptime + get_stime();
	if ((save_utime < uptime) || (save_utime > t))
		save_utime = t;

	logmsg(LOG_DEBUG, "*** %s: uptime = %lum, save_utime = %lum", __FUNCTION__, uptime / 60, save_utime / 60);

	snprintf(hgz, sizeof(hgz), "%s.gz", history_fn);

	if (new) {
		unlink(hgz);
		save_utime = 0;
		return;
	}

	if (save_path[0] != 0) {
		i = 1;
		while (1) {
			if (wait_action_idle(10)) {
				/* cifs quirk: try forcing refresh */
				eval("ls", save_path);

				/* If we can't access the path, keep trying - maybe it isn't mounted yet.
				 * If we can, and we can sucessfully load it, oksy.
				 * If we can, and we cannot load it, then maybe it has been deleted, or
				 * maybe it's corrupted (like 0 bytes long).
				 * In these cases, try the backup files.
				 */
				if ((load_history(save_path) >= 0) || (try_hardway(save_path) >= 0)) {
					f_write_string(source_fn, save_path, 0, 0);
					break;
				}
			}

			/* not ready... */
			sleep(i);
			if ((i *= 2) > 900)
				i = 900; /* 15m */

			if (gotterm) {
				save_path[0] = 0;
				return;
			}

			if (i > (3 * 60))
				logmsg(LOG_WARNING, "Problem loading %s. Still trying...", save_path);
		}
	}
}

int speed_empty(Node *node)
{
	int i, j;
	/* iterate over the entire speed[i][j] two-dimensional array
	 * and look for any entries (counters) that are non-zero.
	 * If such an entry is found, return 0 (false)
	 */
	for (i = 0; i < MAX_NSPEED; ++i) {
		for (j = 0; j < MAX_COUNTER; ++j) {
			if (node->speed[i][j])
				return 0;
		}
	}

	return 1;
}

void Node_print_speedjs(Node *self, void *t)
{
	int j, k, p;
	uint64_t total, tmax;
	uint64_t n;
	char c;

	/* hide hosts with no traffic on IP Traffic - Last 24 Hours */
	if (speed_empty(self))
		return;

	node_print_mode_t *info = (node_print_mode_t *)t;

	fprintf(info->stream, "%s'%s': {\n", info->kn ? " },\n" : "", self->ipaddr);
	for (j = 0; j < MAX_COUNTER; ++j) {
		total = tmax = 0;
		fprintf(info->stream, "%sx: [", j ? ",\n t" : " r");
		p = self->tail;
		for (k = 0; k < MAX_NSPEED; ++k) {
			p = (p + 1) % MAX_NSPEED;
			n = self->speed[p][j];
			fprintf(info->stream, "%s%llu", k ? "," : "", n);
			total += n;
			if (n > tmax)
				tmax = n;
		}
		fprintf(info->stream, "],\n");

		c = j ? 't' : 'r';
		fprintf(info->stream, " %cx_avg: %llu,\n %cx_max: %llu,\n %cx_total: %llu", c, total / MAX_NSPEED, c, tmax, c, total);
	}
	info->kn++;
}

static void save_speedjs(long next)
{
	FILE *f;

	if (!(f = fopen(speedjs_tmp_fn, "w"))) {
		logmsg(LOG_ERR, "*** %s: cannot open %s for writing (%s)", __FUNCTION__, speedjs_tmp_fn, strerror(errno));
		return;
	}

	node_print_mode_t info = {0};
	info.stream = f;

	fprintf(f, "\nspeed_history = {\n");
	TREE_FORWARD_APPLY(&tree, _Node, linkage, Node_print_speedjs, &info);
	fprintf(f, "%s_next: %ld};\n", info.kn ? "},\n" : "", ((next >= 1) ? next : 1));

	fclose(f);
	rename(speedjs_tmp_fn, speedjs_fn);
}

void Node_print_datajs(Node *self, void *t)
{
	data_t *data;
	int p, max, k;

	node_print_mode_t *info = (node_print_mode_t *)t;

	if (info->mode == DAILY) {
		data = self->daily;
		p = self->dailyp;
		max = MAX_NDAILY;
	}
	else {
		data = self->monthly;
		p = self->monthlyp;
		max = MAX_NMONTHLY;
	}

	for (k = max; k > 0; --k) {
		p = (p + 1) % max;
		if (data[p].xtime == 0)
			continue;

		fprintf(info->stream, "%s[0x%lx,'%s',%llu,%llu]", info->kn ? "," : "", (unsigned long)data[p].xtime, self->ipaddr, data[p].counter[RX] / K, data[p].counter[TX] / K);
		info->kn++;
	}
}

static void save_datajs(FILE *f, int mode)
{
	node_print_mode_t info = {0};
	info.mode = mode;
	info.stream = f;

	fprintf(f, "\n%s_history = [\n", (mode == DAILY) ? "daily" : "monthly");
	TREE_FORWARD_APPLY(&tree, _Node, linkage, Node_print_datajs, &info);
	fprintf(f, "\n];\n");
}

static void save_histjs(void)
{
	FILE *f;

	if (!(f = fopen(historyjs_tmp_fn, "w"))) {
		logmsg(LOG_ERR, "*** %s: cannot open %s for writing (%s)", __FUNCTION__, historyjs_tmp_fn, strerror(errno));
		return;
	}

	save_datajs(f, DAILY);
	save_datajs(f, MONTHLY);
	fclose(f);
	rename(historyjs_tmp_fn, historyjs_fn);
}

static void bump(data_t *data, int *tail, int max, uint32_t xnow, uint64_t *counter)
{
	int t, i;

	t = *tail;
	if (data[t].xtime != xnow) {
		for (i = max - 1; i >= 0; --i) {
			if (data[i].xtime == xnow) {
				t = i;
				break;
			}
		}
		if (i < 0) {
			*tail = t = (t + 1) % max;
			data[t].xtime = xnow;
			memset(data[t].counter, 0, sizeof(data[0].counter));
		}
	}
	for (i = 0; i < MAX_COUNTER; ++i) {
		data[t].counter[i] += counter[i];
	}
}

void Node_housekeeping(Node *self, void *info)
{
	if (self) {
		if (self->sync == -1)
			self->sync = 0;
		else
			self->sync = 1;
	}
}

static void calc(void)
{
	FILE *f;
	char buf[512];
	char ip[INET_ADDRSTRLEN];
	char name[64];
	char *ipaddr = NULL;
	uint64_t counter[MAX_COUNTER];
	int i, j, n;
	time_t now, mon;
	struct tm *tms;
	uint64_t tx, rx;
	uint64_t c, sc, diff;
	long tick;
	char *exclude = NULL;
	char *include = NULL;
	char prefix[] = "wan"; /* not yet mwan ready, assume wan */
	char br;
	Node *ptr = NULL;
	Node test;

	int wanup = 0; /* 0 = FALSE, 1 = TRUE */
	long wanuptime = 0; /* wanuptime in seconds */

	now = time(0);

	exclude = strdup(nvram_safe_get("cstats_exclude"));
	include = strdup(nvram_safe_get("cstats_include"));

	logmsg(LOG_DEBUG, "*** %s: cstats_exclude=[%s] cstats_include=[%s]", __FUNCTION__, exclude, include);

	for (br = 0 ; br < BRIDGE_COUNT; br++) {
		char bridge[2] = "0";
		if (br != 0)
			bridge[0] += br;
		else
			strlcpy(bridge, "", sizeof(bridge));

		snprintf(name, sizeof(name), "/proc/net/ipt_account/lan%s", bridge);

		if (!(f = fopen(name, "r")))
			continue;

		while (fgets(buf, sizeof(buf), f)) {
			if (sscanf(buf, "ip = %s bytes_src = %llu %*u %*u %*u %*u packets_src = %*u %*u %*u %*u %*u bytes_dst = %llu %*u %*u %*u %*u packets_dst = %*u %*u %*u %*u %*u time = %*u", ip, &rx, &tx) != 3)
				continue;

			logmsg(LOG_DEBUG, "*** %s: ip=[%s] tx=[%llu] rx=[%llu]", __FUNCTION__, ip, tx, rx);

			if (find_word(exclude, ip))
				continue;

			counter[RX] = tx;
			counter[TX] = rx;
			ipaddr = ip;

			strncpy(test.ipaddr, ipaddr, INET_ADDRSTRLEN);
			ptr = TREE_FIND(&tree, _Node, linkage, &test);

			if ((tx < 1) && (rx < 1) && (!ptr))
				continue;

			if ((ptr) || (nvram_get_int("cstats_all")) || (find_word(include, ipaddr))) {
				if (!ptr) {
					logmsg(LOG_DEBUG, "*** %s: new ip: %s", __FUNCTION__, ipaddr);
					TREE_INSERT(&tree, _Node, linkage, Node_new(ipaddr));
					ptr = TREE_FIND(&tree, _Node, linkage, &test);
					if (ptr) {
						ptr->sync = 1;
						ptr->utime = uptime;
					}
				}
#ifdef DEBUG_CSTATS
				Tree_info();
#endif
				logmsg(LOG_DEBUG, "*** %s: sync[%s]=%d", __FUNCTION__, ptr->ipaddr, ptr->sync);
				if (ptr && ptr->sync) {
					logmsg(LOG_DEBUG, "*** %s: sync[%s] changed to -1", __FUNCTION__, ptr->ipaddr);
					ptr->sync = -1;
#ifdef DEBUG_CSTATS
					for (i = 0; i < MAX_COUNTER; ++i) {
						logmsg(LOG_DEBUG, "*** %s: counter[%d]=%llu ptr->last[%d]=%llu", __FUNCTION__, i, counter[i], i, ptr->last[i]);
					}
#endif
					memcpy(ptr->last, counter, sizeof(ptr->last));
					memset(counter, 0, sizeof(counter));
				}
				else if (ptr) {
#ifdef DEBUG_CSTATS
					logmsg(LOG_DEBUG, "*** %s: sync[%s] = %d ", __FUNCTION__, ptr->ipaddr, ptr->sync);
#endif
					ptr->sync = -1;
					tick = uptime - ptr->utime;
					n = tick / INTERVAL;
					if (n < 1) {
						logmsg(LOG_DEBUG, "*** %s: %s is a little early... %lu < %d", __FUNCTION__, ipaddr, tick, INTERVAL);
						continue;
					}
					else {
						ptr->utime += (n * INTERVAL);
						logmsg(LOG_DEBUG, "*** %s: %s n=%d tick=%lu utime=%lu ptr->utime=%lu", __FUNCTION__, ipaddr, n, tick, uptime, ptr->utime);

						for (i = 0; i < MAX_COUNTER; ++i) {
							c = counter[i];
							sc = ptr->last[i];
#ifdef DEBUG_CSTATS
							logmsg(LOG_DEBUG, "*** %s: counter[%d]=%llu ptr->last[%d]=%llu c=%llu sc=%llu", __FUNCTION__, i, counter[i], i, ptr->last[i], c, sc);
#endif
							if (c < sc) {
								wanup = check_wanup(prefix); /* router/shared/misc.c */
								wanuptime = check_wanup_time(prefix); /* router/shared/misc.c */
								diff = ((0xFFFFFFFFFFFFFFFFULL) - sc + 1ULL) + c; /* rollover calculation */
								if (diff > ((uint64_t)MAX_ROLLOVER))
									diff = 0ULL; /* 3750 MByte / 60 sec => 500 MBit/s maximum limit with roll-over! Try to catch unknown/unwanted traffic peaks - Part 1/2 */
								if (wanup && (wanuptime < (long)(INTERVAL + 10)))
									diff = 0ULL; /* Try to catch traffic peaks at connection startup/reconnect (xDSL/PPPoE) - Part 2/2 */

								/* see https://www.linksysinfo.org/index.php?threads/tomato-toastmans-releases.36106/page-39#post-281722 */
							}
							else
								diff = c - sc;

							ptr->last[i] = c;
							counter[i] = diff;
							logmsg(LOG_DEBUG, "*** %s: counter[%d]=%llu ptr->last[%d]=%llu c=%llu sc=%llu diff=%llu", __FUNCTION__, i, counter[i], i, ptr->last[i], c, sc, diff);
						}
						logmsg(LOG_DEBUG, "*** %s: ip=%s n=%d ptr->tail=%d", __FUNCTION__, ptr->ipaddr, n, ptr->tail);

						for (j = 0; j < n; ++j) {
							ptr->tail = (ptr->tail + 1) % MAX_NSPEED;
#ifdef DEBUG_CSTATS
							logmsg(LOG_DEBUG, "*** %s: ip=%s j=%d n=%d ptr->tail=%d", __FUNCTION__, ptr->ipaddr, j, n, ptr->tail);
#endif
							for (i = 0; i < MAX_COUNTER; ++i) {
								ptr->speed[ptr->tail][i] = counter[i] / n;
							}
						}
						logmsg(LOG_DEBUG, "*** %s: ip=%s j=%d n=%d ptr->tail=%d", __FUNCTION__, ptr->ipaddr, j, n, ptr->tail);
					}
				}

				if (ptr && nvram_get_int("ntp_ready")) { /* Skip this if the time&date is not set yet */
#ifdef DEBUG_CSTATS
					logmsg(LOG_DEBUG, "*** %s: calling bump %s ptr->dailyp=%d", __FUNCTION__, ptr->ipaddr, ptr->dailyp);
#endif
					tms = localtime(&now);
					bump(ptr->daily, &ptr->dailyp, MAX_NDAILY, (tms->tm_year << 16) | ((uint32_t)tms->tm_mon << 8) | tms->tm_mday, counter);

#ifdef DEBUG_CSTATS
					logmsg(LOG_DEBUG, "*** %s: calling bump %s ptr->monthlyp=%d", __FUNCTION__, ptr->ipaddr, ptr->monthlyp);
#endif
					n = nvram_get_int("cstats_offset");
					if ((n < 1) || (n > 31))
						n = 1;

					mon = now + ((1 - n) * (60 * 60 * 24));
					tms = localtime(&mon);
					bump(ptr->monthly, &ptr->monthlyp, MAX_NMONTHLY, (tms->tm_year << 16) | ((uint32_t)tms->tm_mon << 8), counter);
				}
			}
		}
		fclose(f);
	}

	/* remove/exclude history (if we still have any data previously stored) */
	char *nvp, *b;
	nvp = exclude;
	if (nvp) {
		while ((b = strsep(&nvp, ",")) != NULL) {
			logmsg(LOG_DEBUG, "*** %s: check exclude='%s'", __FUNCTION__, b);
			strncpy(test.ipaddr, b, INET_ADDRSTRLEN);
			ptr = TREE_FIND(&tree, _Node, linkage, &test);
			if (ptr) {
				logmsg(LOG_DEBUG, "*** %s: excluding '%s'", __FUNCTION__, ptr->ipaddr);
				TREE_REMOVE(&tree, _Node, linkage, ptr);
				free(ptr);
				node_count--;
			}
		}
	}

	/* cleanup remaining entries for next time */
	TREE_FORWARD_APPLY(&tree, _Node, linkage, Node_housekeeping, NULL);

	/* todo: total > user ??? */
	if (uptime >= save_utime) {
		save(0);
		save_utime = uptime + get_stime();
		logmsg(LOG_DEBUG, "*** %s: uptime = %lum, save_utime = %lum", __FUNCTION__, uptime / 60, save_utime / 60);
	}

	logmsg(LOG_DEBUG, "*** %s: ====================================", __FUNCTION__);

	if (exclude) free(exclude);
	if (include) free(include);
}

static void sig_handler(int sig)
{
	switch (sig) {
	case SIGTERM:
	case SIGINT:
		gotterm = 1;
		break;
	case SIGHUP:
		gothup = 1;
		break;
	case SIGUSR1:
		gotuser = 1;
		break;
	case SIGUSR2:
		gotuser = 2;
		break;
	}
}

int main(int argc, char *argv[])
{
	struct sigaction sa;
	long z;
	int new = 0;

	if (fork() != 0)
		return 0;

	/* proper daemonization */
	setsid();
	chdir("/");
	close(0); close(1); close(2);

	openlog("cstats", LOG_PID, LOG_USER);

	//logmsg(LOG_INFO, "cstats - Copyright (C) 2011-2012 Augusto Bott");
	//logmsg(LOG_INFO, "based on rstats - Copyright (C) 2006-2009 Jonathan Zarate");

	if (argc > 1 && strcmp(argv[1], "--new") == 0) {
		new = 1;
		logmsg(LOG_DEBUG, "*** %s: new=1", __FUNCTION__);
	}

	unlink(load_fn);

	sa.sa_handler = sig_handler;
	sa.sa_flags = 0;
	sigemptyset(&sa.sa_mask);
	sigaction(SIGUSR1, &sa, NULL);
	sigaction(SIGUSR2, &sa, NULL);
	sigaction(SIGHUP, &sa, NULL);
	sigaction(SIGTERM, &sa, NULL);
	sigaction(SIGINT, &sa, NULL);

	load(new);

	z = uptime = get_uptime();
	while (1) {
		while (uptime < z) {
			sleep(z - uptime);
			if (gothup) {
				if (unlink(load_fn) == 0)
					load_new();
				else
					save(0);

				gothup = 0;
			}
			if (gotterm) {
				save(!nvram_match("cstats_sshut", "1"));
				free_all_nodes();
				exit(0);
			}
			if (gotuser == 1) {
				save_speedjs(z - get_uptime());
				gotuser = 0;
			}
			else if (gotuser == 2) {
				save_histjs();
				gotuser = 0;
			}
			uptime = get_uptime();
		}
		calc();
		z += INTERVAL;
	}

	return 0;
}
