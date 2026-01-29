/*
 *
 * rstats
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
#include <sys/ioctl.h>
#include <stdint.h>
#include <syslog.h>
#ifdef USE_ZLIB
 #include <zlib.h>
#endif

#include <bcmnvram.h>
#include <shutils.h>
#include <shared.h>

/* needed by logmsg() */
#define LOGMSG_DISABLE	DISABLE_SYSLOG_OSM
#define LOGMSG_NVDEBUG	"rstats_debug"

#define K		1024
#define M		(1024 * 1024)
#define G		(1024 * 1024 * 1024)

#define SMIN		60
#define	SHOUR		(60 * 60)
#define	SDAY		(60 * 60 * 24)

#define INTERVAL	60

#define MAX_NSPEED	((24 * SHOUR) / INTERVAL)
#define MAX_NDAILY	62
#ifndef TOMATO64
#define MAX_NMONTHLY	25
#define MAX_SPEED_IF	32
#endif /* TOMATO64 */
#ifdef TOMATO64
#define MAX_NMONTHLY	121
#define MAX_SPEED_IF	64
#endif /* TOMATO64 */
#define MAX_ROLLOVER	(3750ULL * M) /* 3750 MByte - new rollover limit */

#define MAX_COUNTER	2
#define RX 		0
#define TX 		1

#define DAILY		0
#define MONTHLY		1

#define CURRENT_ID	0x31305352

#define HI_BACK		5

typedef struct {
	uint32_t xtime;
	uint64_t counter[MAX_COUNTER];
} data_t;

typedef struct {
	uint32_t id;

	data_t daily[MAX_NDAILY];
	int dailyp;

	data_t monthly[MAX_NMONTHLY];
	int monthlyp;
} history_t;

typedef struct {
	char ifname[12];
	long utime;
	unsigned long speed[MAX_NSPEED][MAX_COUNTER];
	uint64_t last[MAX_COUNTER];
	int tail;
	signed char sync;
} speed_t;

typedef struct {
	int16_t wan_unit;
	uint16_t from_ifaceX;
} speed_runtime_t;

history_t history;
speed_t speed[MAX_SPEED_IF];
speed_runtime_t speed_rtd[MAX_SPEED_IF];
int speed_count;
long save_utime;
char save_path[96];
long uptime;

volatile int gothup = 0;
volatile int gotuser = 0;
volatile int gotterm = 0;
volatile int restarted = 1;

const char history_fn[]       = "/var/lib/misc/rstats-history";
const char speed_fn[]         = "/var/lib/misc/rstats-speed";
#ifndef USE_ZLIB
 const char uncomp_fn[]        = "/var/tmp/rstats-uncomp";
#endif
const char source_fn[]        = "/var/lib/misc/rstats-source";
const char historyjs_fn[]     = "/var/spool/rstats-history.js";
const char historyjs_tmp_fn[] = "/var/tmp/rstats-history.js";
const char speedjs_fn[]       = "/var/spool/rstats-speed.js";
const char speedjs_tmp_fn[]   = "/var/tmp/rstats-speed.js";
const char load_fn[]          = "/var/tmp/rstats-load";
const char stime_fn[]         = "/var/lib/misc/rstats-stime";


static int get_stime(void)
{
	int t;
	t = nvram_get_int("rstats_stime");
	if (t < 1)
		t = 1;
	else if (t > 8760)
		t = 8760;

	return t * SHOUR;
}

#ifdef TOMATO64
/* Generate speed path from save_path by appending _speed */
static void get_speed_path(char *speed_path, int size)
{
	int n;
	char *ext;

	speed_path[0] = 0;

	if (save_path[0] == 0 || strcmp(save_path, "*nvram") == 0)
		return;

	n = strlen(save_path);
	if (n == 0)
		return;

	/* If path ends with /, use same directory with different filename */
	if (save_path[n - 1] == '/') {
		unsigned char mac[6];
		ether_atoe(nvram_safe_get("lan_hwaddr"), mac);
		snprintf(speed_path, size, "%stomato_rstats_speed_%02x%02x%02x%02x%02x%02x.gz",
			save_path, mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
	}
	else {
		/* File path - insert _speed before extension or at end */
		ext = strrchr(save_path, '.');
		if (ext && (strcmp(ext, ".gz") == 0)) {
			/* Has .gz extension - insert _speed before it */
			n = ext - save_path;
			snprintf(speed_path, size, "%.*s_speed%s", n, save_path, ext);
		}
		else {
			/* No extension - append _speed */
			snprintf(speed_path, size, "%s_speed", save_path);
		}
	}
}
#endif /* TOMATO64 */

static int comp(const char *path, void *buffer, int size)
{
#ifdef USE_ZLIB
	char gzpath[256];
	int written, err;

	snprintf(gzpath, sizeof(gzpath), "%s.gz", path);
	gzFile gf = gzopen(gzpath, "wb9");
	if (!gf)
		return 0;

	written = gzwrite(gf, buffer, size);
	if (written != size) {
		gzclose(gf);
		return 0;
	}

	err = gzclose(gf);
	return (err == Z_OK);
#else
	char cmd[256];

	if (f_write(path, buffer, size, 0, 0) != size)
		return 0;

	snprintf(cmd, sizeof(cmd), "gzip -f %s", path); /* -f to overwrite existing .gz */

	return system(cmd) == 0;
#endif
}

static void save(int quick)
{
	int i;
	char *bi, *bo;
	int n;
	int b;
	char hgz[256];
#ifdef TOMATO64
	char sgz[256];
#endif /* TOMATO64 */
	char tmp[256];
	char bak[256];
	char bkp[256];
	time_t now;
	struct tm *tms;
	static int lastbak = -1;
#ifdef TOMATO64
	static int lastbak_speed = -1;
#endif /* TOMATO64 */

	logmsg(LOG_DEBUG, "*** %s: quick=%d", __FUNCTION__, quick);

	f_write(stime_fn, &save_utime, sizeof(save_utime), 0, 0);

	comp(speed_fn, speed, sizeof(speed[0]) * speed_count);
	comp(history_fn, &history, sizeof(history));

	logmsg(LOG_DEBUG, "*** %s: write source=%s", __FUNCTION__, save_path);
	f_write_string(source_fn, save_path, 0, 0);

	if (quick)
		return;

	snprintf(hgz, sizeof(hgz), "%s.gz", history_fn);

	if (strcmp(save_path, "*nvram") == 0) {
		if (!wait_action_idle(10)) {
			logmsg(LOG_DEBUG, "*** %s: busy, not saving", __FUNCTION__);
			return;
		}

		if ((n = f_read_alloc(hgz, &bi, 20 * 1024)) > 0) {
			if ((bo = malloc(base64_encoded_len(n) + 1)) != NULL) {
				n = base64_encode((unsigned char *)bi, bo, n);
				bo[n] = 0;
				nvram_set("rstats_data", bo);
				if (!nvram_match("debug_nocommit", "1"))
					nvram_commit();

				logmsg(LOG_DEBUG, "*** %s: nvram commit", __FUNCTION__);

				free(bo);
			}
			free(bi);
		}
	}
	else if (save_path[0] != 0) {
		strlcpy(tmp, save_path, sizeof(tmp));
		strlcat(tmp, ".tmp", sizeof(tmp));

		for (i = 15; i > 0; --i) {
			if (!wait_action_idle(10))
				logmsg(LOG_DEBUG, "*** %s: busy, not saving", __FUNCTION__);
			else {
				logmsg(LOG_DEBUG, "*** %s: cp %s %s", __FUNCTION__, hgz, tmp);
				if (eval("cp", hgz, tmp) == 0) {
					logmsg(LOG_DEBUG, "*** %s: copy ok", __FUNCTION__);

					if (!nvram_match("rstats_bak", "0")) {
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

#ifdef TOMATO64
	snprintf(sgz, sizeof(sgz), "%s.gz", speed_fn);

	if (save_path[0] != 0 && strcmp(save_path, "*nvram") != 0) {
		char speed_path[256];
		get_speed_path(speed_path, sizeof(speed_path));

		if (speed_path[0] != 0) {
			strcpy(tmp, speed_path);
			strcat(tmp, ".tmp");

			for (i = 15; i > 0; --i) {
				if (!wait_action_idle(10))
					logmsg(LOG_DEBUG, "*** %s: busy, not saving speed", __FUNCTION__);
				else {
					logmsg(LOG_DEBUG, "*** %s: cp %s %s", __FUNCTION__, sgz, tmp);
					if (eval("cp", sgz, tmp) == 0) {
						logmsg(LOG_DEBUG, "*** %s: copy ok (speed)", __FUNCTION__);

						if (!nvram_match("rstats_bak", "0")) {
							now = time(0);
							tms = localtime(&now);
							if (lastbak_speed != tms->tm_yday) {
								strcpy(bak, speed_path);
								n = strlen(bak);
								if ((n > 3) && (strcmp(bak + (n - 3), ".gz") == 0))
									n -= 3;

								strcpy(bkp, bak);
								for (b = HI_BACK-1; b > 0; --b) {
									snprintf(bkp + n, sizeof(bkp) - n, "_%d.bak", b + 1);
									snprintf(bak + n, sizeof(bak) - n, "_%d.bak", b);
									rename(bak, bkp);
								}
								if (eval("cp", "-p", speed_path, bak) == 0)
									lastbak_speed = tms->tm_yday;
							}
						}
						logmsg(LOG_DEBUG, "*** %s: rename %s %s", __FUNCTION__, tmp, speed_path);

						if (rename(tmp, speed_path) == 0) {
							logmsg(LOG_DEBUG, "*** %s: rename ok (speed)", __FUNCTION__);
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
#endif /* TOMATO64 */
}

static int decomp(const char *fname, void *buffer, int size, int max)
{
	int n = 0;

	logmsg(LOG_DEBUG, "*** %s: fname=%s", __FUNCTION__, fname);
#ifdef USE_ZLIB
	gzFile gf = gzopen(fname, "rb");
	if (gf) {
		n = gzread(gf, buffer, (unsigned)(size * max));
		gzclose(gf);

		if (n <= 0)
			n = 0;
		else
			n /= size;
	}
#else
	char cmd[256];

	unlink(uncomp_fn);

	snprintf(cmd, sizeof(cmd), "gzip -dc %s > %s", fname, uncomp_fn);
	if (system(cmd) == 0) {
		n = f_read(uncomp_fn, buffer, size * max);
		_dprintf("%s: n=%d\n", __FUNCTION__, n);
		if (n <= 0)
			n = 0;
		else
			n /= size;
	}
	else
		logmsg(LOG_DEBUG, "*** %s: %s != 0", __FUNCTION__, cmd);

	unlink(uncomp_fn);
#endif
	memset((char *)buffer + (size * n), 0, (max - n) * size);

	return n;
}

static void clear_history(void)
{
	memset(&history, 0, sizeof(history));
	history.id = CURRENT_ID;
}

static int load_history(const char *fname)
{
	history_t hist;

	logmsg(LOG_DEBUG, "*** %s: fname=%s", __FUNCTION__, fname);

	if (decomp(fname, &hist, sizeof(hist), 1) != 1) {
		logmsg(LOG_DEBUG, "*** %s: load failed", __FUNCTION__);
		return 0;
	}

	memcpy(&history, &hist, sizeof(history));

	logmsg(LOG_DEBUG, "*** %s: dailyp=%d monthlyp=%d", __FUNCTION__, history.dailyp, history.monthlyp);

	return 1;
}

#ifdef TOMATO64
static int load_speed(const char *fname)
{
	int count;

	logmsg(LOG_DEBUG, "*** %s: fname=%s", __FUNCTION__, fname);

	count = decomp(fname, speed, sizeof(speed[0]), MAX_SPEED_IF);
	if (count <= 0) {
		logmsg(LOG_DEBUG, "*** %s: load failed", __FUNCTION__);
		return 0;
	}

	logmsg(LOG_DEBUG, "*** %s: speed_count=%d", __FUNCTION__, count);
	speed_count = count;

	return 1;
}
#endif /* TOMATO64 */

/* Try loading from the backup versions.
 * We'll try from oldest to newest, then
 * retry the requested one again last.  In case the drive mounts while
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

#ifdef TOMATO64
static int try_hardway_speed(const char *fname)
{
	char fn[256];
	int n, b, found = 0;

	strcpy(fn, fname);
	n = strlen(fn);
	if ((n > 3) && (strcmp(fn + (n - 3), ".gz") == 0))
		n -= 3;

	for (b = HI_BACK; b > 0; --b) {
		snprintf(fn + n, sizeof(fn) - n, "_%d.bak", b);
		found |= load_speed(fn);
	}
	found |= load_speed(fname);

	return found;
}
#endif /* TOMATO64 */

static void load_new(void)
{
	char hgz[256];

	snprintf(hgz, sizeof(hgz), "%s.gz.new", history_fn);
	if (load_history(hgz))
		save(0);

	unlink(hgz);
}

static void load(int new)
{
	int i;
	long t;
	char *bi, *bo;
	int n;
	char hgz[256];
#ifdef TOMATO64
	char sgz[256];
#endif /* TOMATO64 */
	char sp[sizeof(save_path)];
	unsigned char mac[6];

	uptime = get_uptime();

	strlcpy(save_path, nvram_safe_get("rstats_path"), sizeof(save_path) - 32);
	if (((n = strlen(save_path)) > 0) && (save_path[n - 1] == '/')) {
		ether_atoe(nvram_safe_get("lan_hwaddr"), mac);
		snprintf(save_path + n, sizeof(save_path) - n, "tomato_rstats_%02x%02x%02x%02x%02x%02x.gz",
		         mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
	}

	if (f_read(stime_fn, &save_utime, sizeof(save_utime)) != sizeof(save_utime))
		save_utime = 0;

	t = uptime + get_stime();
	if ((save_utime < uptime) || (save_utime > t))
		save_utime = t;

	logmsg(LOG_DEBUG, "*** %s: uptime = %ldm, save_utime = %ldm", __FUNCTION__, uptime / 60, save_utime / 60);

#ifndef TOMATO64
	snprintf(hgz, sizeof(hgz), "%s.gz", speed_fn);
	speed_count = decomp(hgz, speed, sizeof(speed[0]), MAX_SPEED_IF);
#endif /* TOMATO64 */
#ifdef TOMATO64
	snprintf(sgz, sizeof(sgz), "%s.gz", speed_fn);
	speed_count = decomp(sgz, speed, sizeof(speed[0]), MAX_SPEED_IF);
#endif /* TOMATO64 */
	logmsg(LOG_DEBUG, "*** %s: speed_count = %d", __FUNCTION__, speed_count);

#ifndef TOMATO64
	for (i = 0; i < speed_count; ++i) {
		if (speed[i].utime > uptime) {
			speed[i].utime = uptime;
			speed[i].sync = 1;
		}
	}
#endif /* TOMATO64 */

	snprintf(hgz, sizeof(hgz), "%s.gz", history_fn);

	if (new) {
		unlink(hgz);
		save_utime = 0;
		return;
	}

	f_read_string(source_fn, sp, sizeof(sp)); /* always terminated */
	logmsg(LOG_DEBUG, "*** %s: read source=%s save_path=%s", __FUNCTION__, sp, save_path);

	if ((strcmp(sp, save_path) == 0) && (load_history(hgz))) {
		logmsg(LOG_DEBUG, "*** %s: using local file", __FUNCTION__);
		return;
	}

	if (save_path[0] != 0) {
		if (strcmp(save_path, "*nvram") == 0) {
			if (!wait_action_idle(60))
				exit(0);

			bi = nvram_safe_get("rstats_data");
			if ((n = strlen(bi)) > 0) {
				if ((bo = malloc(base64_decoded_len(n))) != NULL) {
					n = base64_decode(bi, (unsigned char *)bo, n);
					logmsg(LOG_DEBUG, "*** %s: nvram n=%d", __FUNCTION__, n);
					f_write(hgz, bo, n, 0, 0);
					free(bo);
					load_history(hgz);
				}
			}
		}
		else {
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
					if (load_history(save_path) || try_hardway(save_path)) {
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
					syslog(LOG_WARNING, "Problem loading %s. Still trying...", save_path);
			}
		}
	}

#ifdef TOMATO64
	/* Load speed from custom path if configured (not nvram - too large for libnvram 16KB read limit) */
	if (save_path[0] != 0 && strcmp(save_path, "*nvram") != 0) {
		char speed_path[256];

		get_speed_path(speed_path, sizeof(speed_path));

		if (speed_path[0] != 0) {
			/* Try to load speed file once - if it doesn't exist, start fresh immediately */
			if (load_speed(speed_path) || try_hardway_speed(speed_path)) {
				logmsg(LOG_DEBUG, "*** %s: loaded speed from %s", __FUNCTION__, speed_path);
			} else {
				/* Speed file doesn't exist - start fresh (normal for new installs or after migration) */
				logmsg(LOG_INFO, "*** %s: no speed file found, starting fresh", __FUNCTION__);
				speed_count = 0;
				memset(speed, 0, sizeof(speed));
			}
		}
	}

	/* Ensure speed data is loaded and synced */
	for (i = 0; i < speed_count; ++i) {
		if (speed[i].utime > uptime) {
			speed[i].utime = uptime;
			speed[i].sync = 1;
		}
	}
#endif /* TOMATO64 */
}

static void save_speedjs(long next)
{
	int i, j, k;
	speed_t *sp;
	int p;
	FILE *f;
	uint64_t total;
	uint64_t tmax;
	unsigned long n;
	char c;
	int up;
	int sfd;
	struct ifreq ifr;

	if ((f = fopen(speedjs_tmp_fn, "w")) == NULL)
		return;

	logmsg(LOG_DEBUG, "*** %s: speed_count = %d", __FUNCTION__, speed_count);

	if ((sfd = socket(AF_INET, SOCK_RAW, IPPROTO_RAW)) < 0)
		logmsg(LOG_DEBUG, "*** %s: %d: error opening socket %m", __FUNCTION__, __LINE__);

	fprintf(f, "\nspeed_history = {\n");

	for (i = 0; i < speed_count; ++i) {
		sp = &speed[i];

		up = 0;
		if (sfd >= 0) {
			strlcpy(ifr.ifr_name, sp->ifname, sizeof(ifr.ifr_name));
			if (ioctl(sfd, SIOCGIFFLAGS, &ifr) == 0)
				up = (ifr.ifr_flags & IFF_UP);
		}

		fprintf(f, "%s'%s': { up: %d", i ? " },\n" : "", sp->ifname, up);

		for (j = 0; j < MAX_COUNTER; ++j) {
			total = tmax = 0;
			c = j ? 't' : 'r';
			fprintf(f, ",\n %cx: [", c);
			p = sp->tail;
			for (k = 0; k < MAX_NSPEED; ++k) {
				p = (p + 1) % MAX_NSPEED;
				n = sp->speed[p][j];
				fprintf(f, "%s%lu", k ? "," : "", n);
				total += n;
				if (n > tmax)
					tmax = n;
			}
			fprintf(f, "],\n");

			fprintf(f, " %cx_avg: %llu,\n %cx_max: %llu,\n %cx_total: %llu", c, total / MAX_NSPEED, c, tmax, c, total);
		}
	}
	fprintf(f, "%s_next: %ld};\n", speed_count ? "},\n" : "", ((next >= 1) ? next : 1));

	if (sfd >= 0)
		close(sfd);

	fclose(f);

	rename(speedjs_tmp_fn, speedjs_fn);
}

static void save_datajs(FILE *f, int mode)
{
	data_t *data;
	int p;
	int max;
	int k, kn;

	fprintf(f, "\n%s_history = [\n", (mode == DAILY) ? "daily" : "monthly");

	if (mode == DAILY) {
		data = history.daily;
		p = history.dailyp;
		max = MAX_NDAILY;
	}
	else {
		data = history.monthly;
		p = history.monthlyp;
		max = MAX_NMONTHLY;
	}
	kn = 0;
	for (k = max; k > 0; --k) {
		p = (p + 1) % max;
		if (data[p].xtime == 0)
			continue;

		fprintf(f, "%s[0x%lx,0x%llx,0x%llx]", kn ? "," : "", (unsigned long)data[p].xtime, data[p].counter[RX] / K, data[p].counter[TX] / K);
		++kn;
	}
	fprintf(f, "];\n");
}

static void save_histjs(void)
{
	FILE *f;

	if ((f = fopen(historyjs_tmp_fn, "w")) != NULL) {
		save_datajs(f, DAILY);
		save_datajs(f, MONTHLY);
		fclose(f);
		rename(historyjs_tmp_fn, historyjs_fn);
	}
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

static void calc(void)
{
	FILE *f;
	char buf[256], sbuf[16];
	char prefix[] = "wanXX";
	char *ifname;
	char *p;
	uint64_t counter[MAX_COUNTER];
	speed_t *sp;
	speed_runtime_t *sp_rtd;
	int i, j;
	time_t now;
	time_t mon;
	struct tm *tms;
	uint64_t c;
	uint64_t sc;
	uint64_t diff;
	long tick;
	int n;
	char *exclude;

	int wanup = 0; /* 0 = FALSE, 1 = TRUE */
	long wanuptime = 0; /* wanuptime in seconds */

	now = time(0);
	exclude = nvram_safe_get("rstats_exclude");

	if ((f = fopen("/proc/net/dev", "r")) == NULL)
		return;

	fgets(buf, sizeof(buf), f); /* header */
	fgets(buf, sizeof(buf), f); /* " */
	while (fgets(buf, sizeof(buf), f)) {
		if ((p = strchr(buf, ':')) == NULL)
			continue;

		*p = 0;
		if ((ifname = strrchr(buf, ' ')) == NULL)
			ifname = buf;
		else
			++ifname;

		if ((strcmp(ifname, "lo") == 0) || (find_word(exclude, ifname)))
			continue;

		/* <rx bytes, packets, errors, dropped, fifo errors, frame errors, compressed, multicast><tx ...> */
		if (sscanf(p + 1, "%llu%*u%*u%*u%*u%*u%*u%*u%llu", &counter[RX], &counter[TX]) != 2)
			continue;

		sp = speed;
		sp_rtd = speed_rtd;
		for (i = speed_count; i > 0; --i) {
			if (strcmp(sp->ifname, ifname) == 0)
				break;

			++sp;
			++sp_rtd;
		}
		if (i == 0) {
			if (speed_count >= MAX_SPEED_IF)
				continue;

			logmsg(LOG_DEBUG, "*** %s: add %s as #%d", __FUNCTION__, ifname, speed_count);

			i = speed_count++;
			sp = &speed[i];
			sp_rtd = &speed_rtd[i];
			memset(sp, 0, sizeof(*sp));
			strlcpy(sp->ifname, ifname, sizeof(sp->ifname));
			sp->sync = 1;
			sp->utime = uptime;
		}
		if (sp_rtd->wan_unit == 0) {
			sp_rtd->wan_unit = get_wan_unit_with_value("_iface", ifname);
			if (sp_rtd->wan_unit < 0) {
				sp_rtd->wan_unit = get_wan_unit_with_value("_ifnameX", ifname);
				sp_rtd->from_ifaceX = sp_rtd->wan_unit > 0;
			}

			logmsg(LOG_DEBUG, "*** %s: mapped ifname %s to wan unit %d", __FUNCTION__, ifname, sp_rtd->wan_unit);
		}
		get_wan_prefix(sp_rtd->wan_unit, prefix); /* Note this will default to 'wan' for anything but the primary interface for wanXX */

		if (sp->sync) {
			logmsg(LOG_DEBUG, "*** %s: sync %s", __FUNCTION__, ifname);
			sp->sync = -1;

			memcpy(sp->last, counter, sizeof(sp->last));
			memset(counter, 0, sizeof(counter));
		}
		else {
			sp->sync = -1;
			/* reset previous counters on first calc() to prevent wrong rollover */
			if (restarted > 0) {
				memcpy(sp->last, counter, sizeof(sp->last));
			}

			tick = uptime - sp->utime;
			n = tick / INTERVAL;
			if (n < 1) {
				logmsg(LOG_DEBUG, "*** %s: %s is a little early... %ld < %d", __FUNCTION__, ifname, tick, INTERVAL);
				continue;
			}

			sp->utime += (n * INTERVAL);
			logmsg(LOG_DEBUG, "*** %s: %s n=%d tick=%ld", __FUNCTION__, ifname, n, tick);

			for (i = 0; i < MAX_COUNTER; ++i) {
				c = counter[i];
				sc = sp->last[i];
				if (c < sc) { /* TX/RX bytes went backwards - figure out why */
					diff = (0xFFFFFFFFFFFFFFFFULL - sc + 1ULL) + c; /* rollover calculation */
					if (diff > MAX_ROLLOVER) {
						diff = 0ULL; /* 3750 MByte / 60 sec => 500 MBit/s maximum limit with roll-over! Try to catch unknown/unwanted traffic peaks - Part 1/2 */
					}
					else {
						wanup = check_wanup(prefix); /* see router/shared/misc.c */
						wanuptime = check_wanup_time(prefix); /* see router/shared/misc.c */

						/* see https://www.linksysinfo.org/index.php?threads/tomato-toastmans-releases.36106/post-281722 */
						if (wanup && (wanuptime < (long)(INTERVAL + 10)))
							diff = 0ULL; /* Try to catch traffic peaks at connection startup/reconnect (xDSL/PPPoE) - Part 2/2 */
					}
				}
				else
					diff = c - sc;

				sp->last[i] = c;
				counter[i] = diff;
			}

			for (j = 0; j < n; ++j) {
				sp->tail = (sp->tail + 1) % MAX_NSPEED;
				for (i = 0; i < MAX_COUNTER; ++i) {
					sp->speed[sp->tail][i] = (unsigned long)(counter[i] / n);
				}
			}
		}

		/* todo: split, delay */

		if (sp_rtd->wan_unit > 0 && nvram_get_int("ntp_ready")) { /* Skip if this is not a primary wan interface or the time&date is not set yet */
			if (get_wanx_proto(prefix) == WP_DISABLED) {
				if (!sp_rtd->from_ifaceX || (nvram_get_int(strlcat_r(prefix, "_islan", sbuf, sizeof(sbuf))) == 0)) {
					continue;
				}
			}
			else if (sp_rtd->from_ifaceX) {
				continue;
			}

			tms = localtime(&now);
			bump(history.daily, &history.dailyp, MAX_NDAILY, (tms->tm_year << 16) | ((uint32_t)tms->tm_mon << 8) | tms->tm_mday, counter);

			n = nvram_get_int("rstats_offset");
			if ((n < 1) || (n > 31))
				n = 1;

			mon = now + ((1 - n) * (60 * 60 * 24));
			tms = localtime(&mon);
			bump(history.monthly, &history.monthlyp, MAX_NMONTHLY, (tms->tm_year << 16) | ((uint32_t)tms->tm_mon << 8), counter);
		}
	}
	fclose(f);

	/* cleanup stale entries + save if needed */
	for (i = 0; i < speed_count; ++i) {
		sp = &speed[i];
		if (sp->sync == -1) {
			sp->sync = 0;
			continue;
		}
		if (((uptime - sp->utime) > (10 * SMIN)) || (find_word(exclude, sp->ifname))) {
			logmsg(LOG_DEBUG, "*** %s: #%d removing. > time limit or excluded", __FUNCTION__, i);
			--speed_count;
			memmove(sp, sp + 1, (speed_count - i) * sizeof(speed[0]));
		}
		else {
			logmsg(LOG_DEBUG, "*** %s: %s not found setting sync=1 #%d", __FUNCTION__, sp->ifname, i);
			sp->sync = 1;
		}
	}

	/* todo: total > user */
	if (uptime >= save_utime) {
		save(0);
		save_utime = uptime + get_stime();
		logmsg(LOG_DEBUG, "*** %s: uptime = %ldm, save_utime = %ldm", __FUNCTION__, uptime / 60, save_utime / 60);
	}

	if (restarted > 0)
		restarted = 0;
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
	int new;

	if (fork() != 0)
		return 0;

	/* proper daemonization */
	setsid();
	chdir("/");
	close(0); close(1); close(2);

	openlog("rstats", LOG_PID, LOG_USER);

	//logmsg(LOG_INFO, "rstats - Copyright (C) 2006-2009 Jonathan Zarate");

	new = 0;
	if (argc > 1) {
		if (strcmp(argv[1], "--new") == 0) {
			new = 1;
			logmsg(LOG_DEBUG, "*** %s: new=1", __FUNCTION__);
		}
	}

	clear_history();
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
				save(!nvram_match("rstats_sshut", "1"));
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
