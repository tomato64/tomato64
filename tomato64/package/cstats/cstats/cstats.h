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


/* needed by logmsg() */
#define LOGMSG_DISABLE	DISABLE_SYSLOG_OSM
#define LOGMSG_NVDEBUG	"cstats_debug"
//#define DEBUG_CSTATS

#define K 		1024
#define M 		(1024 * 1024)
#define G 		(1024 * 1024 * 1024)

#define SMIN		60
#define	SHOUR		(60 * 60)
#define	SDAY		(60 * 60 * 24)

#define INTERVAL	60

#define MAX_NSPEED	((24 * SHOUR) / INTERVAL)
#define MAX_NDAILY	62
#define MAX_NMONTHLY	25
#define MAX_ROLLOVER	(3750ULL * M) /* 3750 MByte - new rollover limit */

#define MAX_COUNTER	2
#define RX 		0
#define TX 		1

#define DAILY		0
#define MONTHLY		1

#define ID_V0		0x30305352
#define ID_V1		0x31305352
#define ID_V2		0x32305352
#define CURRENT_ID	ID_V2

#define HI_BACK		5
#define MAX_NODES	2000 /* maximum number of IPs tracked – memory protection */


const char history_fn[]       = "/var/lib/misc/cstats-history";
const char uncomp_fn[]        = "/var/tmp/cstats-uncomp";
const char source_fn[]        = "/var/lib/misc/cstats-source";
const char historyjs_fn[]     = "/var/spool/cstats-history.js";
const char historyjs_tmp_fn[] = "/var/tmp/cstats-history.js";
const char speedjs_fn[]       = "/var/spool/cstats-speed.js";
const char speedjs_tmp_fn[]   = "/var/tmp/cstats-speed.js";
const char load_fn[]          = "/var/tmp/cstats-load";
const char stime_fn[]         = "/var/lib/misc/cstats-stime";

typedef struct {
	int mode;
	int kn;
	FILE *stream;
} node_print_mode_t;

typedef struct {
	uint32_t xtime;
	uint64_t counter[MAX_COUNTER];
} data_t;

typedef struct _Node {
	char ipaddr[INET_ADDRSTRLEN];

	uint32_t id;

	data_t daily[MAX_NDAILY];
	int dailyp;
	data_t monthly[MAX_NMONTHLY];
	int monthlyp;

	long utime;
	uint64_t speed[MAX_NSPEED][MAX_COUNTER];
	uint64_t last[MAX_COUNTER];
	int tail;
	int sync;

	TREE_ENTRY(_Node) linkage;
} Node;

typedef TREE_HEAD(_Tree, _Node) Tree;

TREE_DEFINE(_Node, linkage);

int Node_compare(Node *lhs, Node *rhs);
Node *Node_new(char *ipaddr);
