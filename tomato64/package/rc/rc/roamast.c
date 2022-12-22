/*
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
 * Copyright 2012, ASUSTeK Inc.
 * All Rights Reserved.
 *
 * THIS SOFTWARE IS OFFERED "AS IS", AND ASUS GRANTS NO WARRANTIES OF ANY
 * KIND, EXPRESS OR IMPLIED, BY STATUTE, COMMUNICATION OR OTHERWISE. BROADCOM
 * SPECIFICALLY DISCLAIMS ANY IMPLIED WARRANTIES OF MERCHANTABILITY, FITNESS
 * FOR A SPECIFIC PURPOSE OR NONINFRINGEMENT CONCERNING THIS SOFTWARE.
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <signal.h>
#include <unistd.h>
#include <shared.h>
#include <rc.h>
#include <wlioctl.h>
#include <wlutils.h>

#define RAST_COUNT_IDLE 20
#define RAST_COUNT_RSSI 6
#define RAST_POLL_INTV_NORMAL 5
#define RAST_TIMEOUT_STA 10
#define RAST_DFT_IDLE_RATE	2 /* Kbps */
#define	MAX_IF_NUM 3
#define MAX_SUBIF_NUM 4
#define MAX_STA_COUNT 128
#define ETHER_ADDR_STR_LEN 18

/* for FreshTomato */
#define uptime() get_uptime()

/* needed by logmsg() */
#define LOGMSG_DISABLE		DISABLE_SYSLOG_OSM
#define LOGMSG_NVDEBUG		"roamast_debug"

#ifndef TCONFIG_BCMWL6 /* only for RT and RT-N */
/* compare two ethernet addresses - assumes the pointers can be referenced as shorts */
#define eacmp(a, b)	((((const uint16 *)(a))[0] ^ ((const uint16 *)(b))[0]) | \
                        (((const uint16 *)(a))[1] ^ ((const uint16 *)(b))[1]) | \
                        (((const uint16 *)(a))[2] ^ ((const uint16 *)(b))[2]))

#endif

#if 0
#define RAST_INFO(fmt, arg...) \
	do {	\
		_dprintf("RAST %lu: "fmt, uptime(), ##arg); \
	} while (0)
#define RAST_DBG(fmt, arg...) \
	do {	\
		if (rast_dbg) \
		_dprintf("RAST %lu: "fmt, uptime(), ##arg); \
	} while (0)
#endif

#define MACF	"%02x:%02x:%02x:%02x:%02x:%02x"
#define ETHERP_TO_MACF(ea)	((struct ether_addr *) (ea))->octet[0], \
                                ((struct ether_addr *) (ea))->octet[1], \
                                ((struct ether_addr *) (ea))->octet[2], \
                                ((struct ether_addr *) (ea))->octet[3], \
                                ((struct ether_addr *) (ea))->octet[4], \
                                ((struct ether_addr *) (ea))->octet[5]
#define ETHER_TO_MACF(ea)	(ea).octet[0], \
                                (ea).octet[1], \
                                (ea).octet[2], \
                                (ea).octet[3], \
                                (ea).octet[4], \
                                (ea).octet[5]

typedef struct rast_sta_info {
	time_t timestamp;
	time_t active;
	struct ether_addr addr;
	struct rast_sta_info *prev, *next;
#ifdef TCONFIG_BCMARM
	float datarate; /* Kbps */
	time_t idle_start;
	uint8 idle_state;
#endif
	int32 rssi_hit_count;
	int32 rssi;
} rast_sta_info_t;

typedef struct rast_bss_info {
	char wlif_name[32];
	char prefix[32];
	int user_low_rssi;
#ifdef TCONFIG_BCMARM
	int32 idle_rate;
#endif
	rast_sta_info_t *assoclist[MAX_SUBIF_NUM];
} rast_bss_info_t;

rast_bss_info_t bssinfo[MAX_IF_NUM];
uint8 init = 1;
uint8 wlif_count = 0;
uint32 ticks = 0;
static struct itimerval itv;
int rast_dbg = 0;

static void
alarmtimer(unsigned long sec, unsigned long usec)
{
	itv.it_value.tv_sec  = sec;
	itv.it_value.tv_usec = usec;
	itv.it_interval = itv.it_value;
	setitimer(ITIMER_REAL, &itv, NULL);
}

void rast_init_bssinfo(void)
{
	char ifname[128], *next;
	char usr_rssi[128];
	int idx = 0, idxList;
	memset(bssinfo, 0, sizeof(rast_bss_info_t));

#ifdef TCONFIG_BCMARM
	int rate = nvram_get_int("rast_idlrt");
	if (rate <= 0)
		rate = RAST_DFT_IDLE_RATE;
#endif

	foreach(ifname, nvram_safe_get("wl_ifnames"), next) {
		strncpy(bssinfo[idx].wlif_name, ifname, 32);
		bssinfo[idx].user_low_rssi = 0;
#ifdef TCONFIG_BCMARM
		bssinfo[idx].idle_rate = rate;
#endif
		strncpy(bssinfo[idx].prefix, "", 32);
		for (idxList=0; idxList < MAX_SUBIF_NUM; idxList++) bssinfo[idx].assoclist[idxList] = NULL;

		int ret, unit;

		ret = wl_ioctl(bssinfo[idx].wlif_name, WLC_GET_INSTANCE, &unit, sizeof(unit));
		if (ret < 0) {
#if 0
			RAST_INFO("[WARNING] get instance %s error!!!\n", bssinfo[idx].wlif_name);
#else
			logmsg(LOG_INFO, "[WARNING] get instance %s error!!!\n", bssinfo[idx].wlif_name);
#endif
		}
		else {
			snprintf(bssinfo[idx].prefix, sizeof(bssinfo[idx].prefix), "wl%d_", unit);
			bssinfo[idx].user_low_rssi = nvram_get_int(strcat_r(bssinfo[idx].prefix, "user_rssi", usr_rssi));
		}
		idx++;
	}
	wlif_count = idx;
#if 0
	RAST_DBG("[%s]: TotalWI[%d] \n\n", __FUNCTION__, wlif_count);
#else
	logmsg(LOG_DEBUG, "[%s]: TotalWI[%d] \n\n", __FUNCTION__, wlif_count);
#endif
}

rast_sta_info_t *rast_add_to_assoclist(int bssidx, int vifidx, struct ether_addr *addr)
{
	rast_sta_info_t *sta, *head;

	sta = bssinfo[bssidx].assoclist[vifidx];
	while (sta) {
		/* find sta in assoclist */
		if (eacmp(&(sta->addr), addr) == 0) {
			break;
		}
		sta = sta->next;
	}

	if (!sta) {
		sta = malloc(sizeof(rast_sta_info_t));
		if (!sta) {
#if 0
			RAST_INFO("[%s]: Malloc failure!\n", __FUNCTION__);
#else
			logmsg(LOG_INFO, "[%s]: Malloc failure!\n", __FUNCTION__);
#endif
			return NULL;
		}

		memset(sta, 0, sizeof(rast_sta_info_t));
		memcpy(&sta->addr, addr, sizeof(struct ether_addr));
		sta->timestamp = uptime();
		sta->active = uptime();
		sta->rssi = 0;
		sta->rssi_hit_count = 0;
#ifdef TCONFIG_BCMARM
		sta->datarate = 0;
		sta->idle_state = 0;
#endif

		/* insert sta in the front of assoclist */
		head = bssinfo[bssidx].assoclist[vifidx];
		if (head)
			head->prev = sta;

		sta->next = head;
		sta->prev = (struct rast_sta_info *)&(bssinfo[bssidx].assoclist[vifidx]);
		bssinfo[bssidx].assoclist[vifidx] = sta;
#if 0
		RAST_DBG("add sta ["MACF"] to assoclist\n", ETHERP_TO_MACF(addr));
#else
		logmsg(LOG_DEBUG, "add sta ["MACF"] to assoclist\n", ETHERP_TO_MACF(addr));
#endif
	}

	return sta;
}

rast_sta_info_t *rast_remove_from_assoclist(int bssidx, int vifidx, struct ether_addr *addr)
{
	bool found = FALSE;
	rast_sta_info_t *assoclist = NULL, *prev, *head, *next_sta = NULL;

	assoclist = bssinfo[bssidx].assoclist[vifidx];
	if (assoclist == NULL) {
		return next_sta;
	}

	/* found at 1st element, update pointer */
	if (eacmp(&(assoclist->addr), addr) == 0) {
		head = assoclist->next;
		bssinfo[bssidx].assoclist[vifidx] = head;
		if (head) {
			head->prev = (struct rast_sta_info *)&(bssinfo[bssidx].assoclist[vifidx]);
			next_sta = head;
		}
		found = TRUE;
	}
	else {
		prev = assoclist;
		assoclist = prev->next;

		while (assoclist) {
			if (eacmp(&(assoclist->addr), addr) == 0) {
				head = assoclist->next;
				prev->next = head;
				if (head) {
					head->prev = prev;
					next_sta = head;
				}

				found = TRUE;
				break;
			}

			prev = assoclist;
			assoclist = prev->next;
		}
	}

	if (found) {
		free(assoclist);
	}

	return next_sta;
}

void rast_deauth_sta(int bssidx, int vifidx, rast_sta_info_t *sta)
{
	int ret;
	scb_val_t scb_val;
	char wlif_name[32];

	if (vifidx > 0)
		sprintf(wlif_name, "wl%d.%d", bssidx, vifidx);
	else
		strcpy(wlif_name, bssinfo[bssidx].wlif_name);

#if 0
	RAST_INFO("DEAUTHENTICATE ["MACF"] from %s\n", ETHER_TO_MACF(sta->addr), wlif_name);
#else
	logmsg(LOG_INFO, "DEAUTHENTICATE ["MACF"] from %s\n", ETHER_TO_MACF(sta->addr), wlif_name);
#endif
	memcpy(&scb_val.ea, &sta->addr, ETHER_ADDR_LEN);
	scb_val.val = 8; /* reason code: Disassociated because sending STA is leaving BSS */

	ret = wl_ioctl(wlif_name, WLC_SCB_DEAUTHENTICATE_FOR_REASON, &scb_val, sizeof(scb_val));
	if (ret < 0) {
#if 0
		RAST_INFO("[WARNING] error to deauthticate ["MACF"] !!!\n", ETHER_TO_MACF(sta->addr));
#else
		logmsg(LOG_INFO, "[WARNING] error to deauthticate ["MACF"] !!!\n", ETHER_TO_MACF(sta->addr));
#endif
	}
}

void rast_check_criteria(int bssidx, int vifidx)
{
	int idx=0;
	int flag_rssi;
#ifdef TCONFIG_BCMARM
	int flag_idle;
	time_t now = uptime();
#endif
	rast_sta_info_t *sta = bssinfo[bssidx].assoclist[vifidx];

	while (sta) {
		flag_rssi = 0;
#ifdef TCONFIG_BCMARM
		flag_idle = 0;
#endif
		idx++;

		if (sta->rssi < bssinfo[bssidx].user_low_rssi) {
			sta->rssi_hit_count++;
			if (sta->rssi_hit_count >= RAST_COUNT_RSSI)
				flag_rssi = 1;
		}

#ifdef TCONFIG_BCMARM
		if (sta->datarate < bssinfo[bssidx].idle_rate) {
			if (!sta->idle_state) {
				sta->idle_state = 1;
				sta->idle_start = now;
			}
			if ((now - sta->idle_start) >= RAST_COUNT_IDLE)
				flag_idle = 1;

		}
		else {
			sta->idle_state = 0;
		}
#endif
#if 0
		RAST_DBG("[%d]sta ["MACF"]: rssi = %d dBm [Hit=%d],datarate = %.1f Kbps, active = %lu [idle start = %lu, period = %lu] %s\n",
			idx,
			ETHER_TO_MACF(sta->addr),
			sta->rssi,
			sta->rssi_hit_count,
			sta->datarate,
			sta->active,
			sta->idle_state ? sta->idle_start : 0,
			sta->idle_state ? now - sta->idle_start : 0,
			(flag_rssi & flag_idle) ? " *** Deauthenticate *** " : "");
#else
#ifdef TCONFIG_BCMARM
		logmsg(LOG_DEBUG, "[%d]sta ["MACF"]: rssi = %d dBm [Hit=%d],datarate = %.1f Kbps, active = %lu [idle start = %lu, period = %lu] %s\n",
			idx,
			ETHER_TO_MACF(sta->addr),
			sta->rssi,
			sta->rssi_hit_count,
			sta->datarate,
			sta->active,
			sta->idle_state ? sta->idle_start : 0,
			sta->idle_state ? now - sta->idle_start : 0,
			(flag_rssi & flag_idle) ? " *** Deauthenticate *** " : "");
#else /* MIPS */
		logmsg(LOG_DEBUG, "[%d]sta ["MACF"]: rssi = %d dBm [Hit=%d], active = %lu %s\n",
			idx,
			ETHER_TO_MACF(sta->addr),
			sta->rssi,
			sta->rssi_hit_count,
			sta->active,
			flag_rssi ? " *** Deauthenticate *** " : "");
#endif
#endif

		if (flag_rssi
#ifdef TCONFIG_BCMARM
		    & flag_idle
#endif
		    ) {
			rast_deauth_sta(bssidx, vifidx, sta);
			sta = rast_remove_from_assoclist(bssidx, vifidx, &sta->addr);
			continue;
		}

		sta = sta->next;
	}

	return;
}

#ifdef TCONFIG_BCMARM
void rast_retrieve_bs_data(int bssidx, int vifidx)
{
	int ret;
	int argn;
	char ioctl_buf[4096];
	iov_bs_data_struct_t *data = (iov_bs_data_struct_t *)ioctl_buf;
	iov_bs_data_record_t *rec;
	iov_bs_data_counters_t *ctr;
	float datarate;
	rast_sta_info_t *sta = NULL;
	char wlif_name[32];

	if (vifidx > 0)
		sprintf(wlif_name, "wl%d.%d", bssidx, vifidx);
	else
		strcpy(wlif_name, bssinfo[bssidx].wlif_name);

	memset(ioctl_buf, 0, sizeof(ioctl_buf));
	strcpy(ioctl_buf, "bs_data");
	ret = wl_ioctl(wlif_name, WLC_GET_VAR, ioctl_buf, sizeof(ioctl_buf));
	if (ret < 0) {
		return;
	}

	for (argn = 0; argn < data->structure_count; argn++) {
		rec = &data->structure_record[argn];
		ctr = &rec->station_counters;

		if (ctr->acked == 0)
			continue;

		datarate = (ctr->time_delta) ?
			(float)ctr->throughput * 8000.0 / (float)ctr->time_delta : 0.0;

		sta = rast_add_to_assoclist(bssidx, vifidx, &(rec->station_address));
		if (sta) {
			sta->datarate = datarate;
		}
	}
	return;
}
#endif

void rast_timeout_sta(int bssidx, int vifidx)
{
	time_t now = uptime();
	rast_sta_info_t *sta, *prev, *next, *head;
	sta = bssinfo[bssidx].assoclist[vifidx];
	head = NULL;
	prev = NULL;

	while (sta) {
		if (now - sta->active > RAST_TIMEOUT_STA) {
#if 0
			RAST_DBG("free ["MACF"] from assoclist [now = %lu][active = %lu]\n",
					ETHER_TO_MACF(sta->addr),
					now,
					sta->active);
#else
			logmsg(LOG_DEBUG, "free ["MACF"] from assoclist [now = %lu][active = %lu]\n",
					ETHER_TO_MACF(sta->addr),
					now,
					sta->active);
#endif
			next = sta->next;
			free(sta);
			sta = next;
			if (prev)
				prev->next = sta;
			continue;
		}

		if (head == NULL)
			head = sta;

		prev = sta;
		sta = sta->next;
	}
	bssinfo[bssidx].assoclist[vifidx] = head;
}

void rast_update_sta_info(int bssidx, int vifidx)
{
	struct maclist *mac_list;
	int mac_list_size;
	scb_val_t scb_val;
	unsigned int mcnt;
	char wlif_name[32];
	int32 rssi;
	rast_sta_info_t *sta = NULL;

	if (vifidx > 0)
		sprintf(wlif_name, "wl%d.%d", bssidx, vifidx);
	else
		strcpy(wlif_name, bssinfo[bssidx].wlif_name);

	mac_list_size = sizeof(mac_list->count) + MAX_STA_COUNT * sizeof(struct ether_addr);
	mac_list = malloc(mac_list_size);

	if (!mac_list)
		goto exit;

	memset(mac_list, 0, mac_list_size);

	/* query authentication sta list */
	strcpy((char*) mac_list, "authe_sta_list");
	if (wl_ioctl(wlif_name, WLC_GET_VAR, mac_list, mac_list_size))
		goto exit;

	for (mcnt=0; mcnt < mac_list->count; mcnt++) {
		memcpy(&scb_val.ea, &mac_list->ea[mcnt], ETHER_ADDR_LEN);

		if (wl_ioctl(wlif_name, WLC_GET_RSSI, &scb_val, sizeof(scb_val_t)))
			continue;

		rssi = scb_val.val;

		/* add to assoclist */
		sta = rast_add_to_assoclist(bssidx, vifidx, &(mac_list->ea[mcnt]));
		sta->rssi = rssi;
		sta->active = uptime();
	}

#ifdef TCONFIG_BCMARM
	rast_retrieve_bs_data(bssidx, vifidx);
#endif
	rast_timeout_sta(bssidx, vifidx);
	rast_check_criteria(bssidx, vifidx);

exit:
	if (mac_list)
		free(mac_list);

	return;
}

static void rast_watchdog(int sig)
{
	int idx,vidx;
	char prefix[32];
	char tmp[32];
	const char *ifnames = NULL;

#if 0
	if (!nvram_get_int("wlready"))
		return;
#else /* for FreshTomato: if NTP isn't set yet */
	if (!nvram_get_int("ntp_ready"))
		return;
#endif

	/* for FreshTomato: catch dirty upgrade OR router target(s) not ready OR something else! */
	ifnames = nvram_get("wl_ifnames");
	if ((ifnames == NULL) ||
	    (ifnames && !strlen(ifnames)))
		return;

	if (init) {
		rast_init_bssinfo();
		init = 0;
	}

	for (idx=0; idx < wlif_count; idx++) {
		int val;

		if (!bssinfo[idx].user_low_rssi)
			continue;
#ifdef TCONFIG_PROXYSTA
		if (psta_exist_except(idx) || psr_exist_except(idx))
			continue;
		else if (is_psta(idx) || is_psr(idx))
			continue;
#endif

		wl_ioctl(bssinfo[idx].wlif_name, WLC_GET_RADIO, &val, sizeof(val));
		val &= WL_RADIO_SW_DISABLE | WL_RADIO_HW_DISABLE;
		if (val) {
#if 0
			RAST_DBG("%s radio is disabled!\n", bssinfo[idx].wlif_name);
#else
			logmsg(LOG_DEBUG, "%s radio is disabled!\n", bssinfo[idx].wlif_name);
#endif
			continue;
		}

		for (vidx = 0; vidx < MAX_SUBIF_NUM; vidx++) {

			if (vidx > 0) {
				sprintf(prefix, "wl%d.%d", idx, vidx);
				if (!nvram_match(strcat_r(prefix, "_bss_enabled", tmp), "1"))
					continue;
			}

			/* for FreshTomato: check for wireless client, wireless ehternet bridge and media bridge mode --> skip! */
			if (wl_client(idx, vidx)) {
				logmsg(LOG_DEBUG, "%s radio is in client Mode - skipping!\n", bssinfo[idx].wlif_name);
				continue;
			}

#if 0
			RAST_DBG("### Check stainfo [%s][rssi criteria = %d] ###\n",
				vidx > 0 ? prefix : bssinfo[idx].wlif_name,
				bssinfo[idx].user_low_rssi);
#else
			logmsg(LOG_DEBUG, "### Check stainfo [%s][rssi criteria = %d] ###\n",
				vidx > 0 ? prefix : bssinfo[idx].wlif_name,
				bssinfo[idx].user_low_rssi);
#endif

			rast_update_sta_info(idx,vidx);
		}
	}
#if 0
	RAST_DBG("\n");
#endif
}

static void rast_exit(int sig)
{
	/* free assoclist */
	int i, vi;
	rast_sta_info_t *assoclist, *next;

	alarmtimer(0, 0);

	for (i = 0; i < wlif_count; i++) {
		for (vi = 0; vi < MAX_SUBIF_NUM; vi++) {
			assoclist = bssinfo[i].assoclist[vi];
			while (assoclist) {
				next = assoclist->next;
				free(assoclist);
				assoclist = next;
			}
		}
	}
#if 0
	RAST_INFO("ROAMAST Exit...\n");
#else
	logmsg(LOG_DEBUG, "ROAMAST Exit...\n");
#endif
	remove("/var/run/roamast.pid");
	exit(0);
}

int roam_assistant_main(int argc, char *argv[])
{
	FILE *fp;
	sigset_t sigs_to_catch;

	/* write pid */
	if ((fp = fopen("/var/run/roamast.pid", "w")) != NULL) {
		fprintf(fp, "%d", getpid());
		fclose(fp);
	}

#if 1
	rast_dbg = nvram_get_int("rast_dbg");
#else /* Open DBG_INFO msg */
	rast_dbg = 1;
#endif
#if 0
	RAST_INFO("ROAMAST Start...\n");
#else
	logmsg(LOG_DEBUG, "ROAMAST Start...\n");
#endif

	/* set the signal handler */
	sigemptyset(&sigs_to_catch);
	sigaddset(&sigs_to_catch, SIGALRM);
	sigaddset(&sigs_to_catch, SIGTERM);
	sigprocmask(SIG_UNBLOCK, &sigs_to_catch, NULL);

	signal(SIGALRM, rast_watchdog);
	signal(SIGTERM, rast_exit);

	alarmtimer(RAST_POLL_INTV_NORMAL, 0);
#if 0
	RAST_INFO("%s \n", __FUNCTION__);
#endif
	/* Most of time it goes to sleep */
	while (1) {
		pause();
	}

	return 0;
}
