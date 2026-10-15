/*
 * wlmlo.c - Wi-Fi 7 Multi-Link Operation (MLO) support for the status pages
 *
 * Tomato Firmware
 * Copyright (C) 2026
 *
 * An MLO access point is one netdev (e.g. phy0-ap0) with a link on each of
 * several radios of one wiphy. iwinfo only knows the netdev, so it reports a
 * single channel for the lot. Everything here reads one link at a time
 * instead: 'iw dev <ifname> info' for the link's channel, the survey for its
 * noise, and hostapd's per-link control socket for its clients.
 */

#ifdef TOMATO64

#include "tomato.h"
#include "wlhelper.h"

#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include <strings.h>

#define LINE_BUFFER_SIZE 256
#define MLO_MAX_STATIONS 128

/*
 * Written by start_wifi.sh, one logical phy per line:
 *	<lphy>|<board phy>|<radio index|->|<bands>|<path>
 */
#define LPHY_MAP "/tmp/.lphy_map"

static int mlo_freq2chan(int mhz)
{
	if (mhz == 2484)
		return 14;
	if (mhz < 2484)
		return (mhz - 2407) / 5;
	if (mhz < 5925)
		return (mhz - 5000) / 5;
	if (mhz == 5935)
		return 2;

	return (mhz - 5950) / 5;
}

/* The lphy map line for logical phy 'lphy', split into board phy and radio index */
static int mlo_lphy_lookup(int lphy, char *board, size_t board_size, int *ridx)
{
	FILE *fp;
	char line[LINE_BUFFER_SIZE];
	int found = 0;

	if ((fp = fopen(LPHY_MAP, "r")) == NULL)
		return 0;

	while (fgets(line, sizeof(line), fp) != NULL) {
		char *f_lphy = strtok(line, "|");
		char *f_board = strtok(NULL, "|");
		char *f_ridx = strtok(NULL, "|");

		if (!f_lphy || !f_board || !f_ridx || (atoi(f_lphy) != lphy))
			continue;

		snprintf(board, board_size, "%s", f_board);
		*ridx = (f_ridx[0] == '-') ? -1 : atoi(f_ridx);
		found = 1;
		break;
	}

	fclose(fp);
	return found;
}

/*
 * Logical phy of the radio an MLO link runs on
 *
 * hostapd is given mld_link_id = the radio index (OpenWrt's ap.uc), so the
 * link is the logical phy that is that radio of the anchor's wiphy.
 */
int wlhelper_mlo_link_radio(int phy, int link_id)
{
	FILE *fp;
	char line[LINE_BUFFER_SIZE];
	char board[32];
	int ridx, radio = -1;

	if (!mlo_lphy_lookup(phy, board, sizeof(board), &ridx))
		return -1;

	if ((fp = fopen(LPHY_MAP, "r")) == NULL)
		return -1;

	while (fgets(line, sizeof(line), fp) != NULL) {
		char *f_lphy = strtok(line, "|");
		char *f_board = strtok(NULL, "|");
		char *f_ridx = strtok(NULL, "|");

		if (!f_lphy || !f_board || !f_ridx || (f_ridx[0] == '-'))
			continue;

		if ((strcmp(f_board, board) == 0) && (atoi(f_ridx) == link_id)) {
			radio = atoi(f_lphy);
			break;
		}
	}

	fclose(fp);
	return radio;
}

/*
 * Parse the "MLD with links:" block of 'iw dev <ifname> info':
 *
 *	MLD with links:
 *	 - link ID  1 link addr 8c:88:2b:00:08:9e
 *	   channel 100 (5500 MHz), width: 160 MHz, center1: 5570 MHz
 *	   txpower 30.00 dBm
 *
 * A link that is not beaconing yet has no channel line.
 */
int wlhelper_mlo_links(int phy, int iface, const char *ifname,
                       struct wlhelper_mlo_link *links, int max)
{
	FILE *fp;
	char key[64], cmd[96], line[LINE_BUFFER_SIZE];
	int n = 0, in_mld = 0;

	/* Only an AP set up for MLO can be one, which saves the fork for every other interface */
	snprintf(key, sizeof(key), "wifi_phy%diface%d_mlo", phy, iface);
	if (nvram_get_int(key) != 1)
		return 0;

	snprintf(key, sizeof(key), "wifi_phy%diface%d_mode", phy, iface);
	if (strcmp(nvram_safe_get(key), "ap") != 0)
		return 0;

	if (!wlhelper_iface_exists(ifname))
		return 0;

	snprintf(cmd, sizeof(cmd), "iw dev %s info 2>/dev/null", ifname);
	if ((fp = popen(cmd, "r")) == NULL)
		return 0;

	while (fgets(line, sizeof(line), fp) != NULL) {
		char *p;
		int center_mhz;

		if (strstr(line, "MLD with links:")) {
			in_mld = 1;
			continue;
		}
		if (!in_mld)
			continue;

		if ((p = strstr(line, "link ID")) != NULL) {
			if (n >= max)
				break;

			memset(&links[n], 0, sizeof(links[n]));
			if (sscanf(p, "link ID %d link addr %17s", &links[n].id, links[n].addr) >= 1)
				n++;
			continue;
		}

		if (n && ((p = strstr(line, "channel ")) != NULL)) {
			struct wlhelper_mlo_link *l = &links[n - 1];

			sscanf(p, "channel %d (%d MHz)", &l->channel, &l->mhz);
			if ((p = strstr(line, "width:")) != NULL)
				sscanf(p, "width: %d", &l->width);
			if (((p = strstr(line, "center1:")) != NULL) && (sscanf(p, "center1: %d", &center_mhz) == 1))
				l->center = mlo_freq2chan(center_mhz);
		}
	}

	pclose(fp);
	return n;
}

int wlhelper_wif_channel_stats(const struct wlhelper_wif *w, int *channel, int *mhz,
                               int *nbw, int *noise, float *rate,
                               int *center, char *proto, size_t proto_size,
                               int *signal)
{
	if (w->link < 0)
		return wlhelper_get_channel_stats(w->ifname, channel, mhz, nbw, noise, rate,
		                                  center, proto, proto_size, signal);

	*channel = w->mlo.channel;
	*mhz = w->mlo.mhz;
	*nbw = w->mlo.width;
	*noise = wlhelper_wif_noise(w);
	/* iwinfo's rate is per netdev, so there is none to give a single link */
	*rate = 0.0;
	if (center)
		*center = w->mlo.center;
	/* MLO needs 802.11be on every link, start_wifi.sh drops any other */
	if (proto && proto_size)
		snprintf(proto, proto_size, "%s", "11be");
	if (signal)
		*signal = 0;

	return (w->mlo.mhz) ? 0 : -1;
}

/*
 * Noise of the channel an MLO link is on, from the survey:
 *
 *	Survey data from phy0-ap0
 *		frequency:			5500 MHz [in use]
 *		noise:				-92 dBm
 */
int wlhelper_wif_noise(const struct wlhelper_wif *w)
{
	FILE *fp;
	char cmd[96], line[LINE_BUFFER_SIZE];
	int freq = 0, v, noise = -99;

	if (w->link < 0)
		return wlhelper_get_noise_level(w->ifname);

	if (!w->mlo.mhz)
		return -99;

	snprintf(cmd, sizeof(cmd), "iw dev %s survey dump 2>/dev/null", w->ifname);
	if ((fp = popen(cmd, "r")) == NULL)
		return -99;

	while (fgets(line, sizeof(line), fp) != NULL) {
		char *p;

		if ((p = strstr(line, "frequency:")) != NULL)
			freq = atoi(p + 10);
		else if ((freq == w->mlo.mhz) && ((p = strstr(line, "noise:")) != NULL)) {
			v = atoi(p + 6);
			if ((v < 0) && (v >= -120))
				noise = v;
			break;
		}
	}

	pclose(fp);
	return noise;
}

int wlhelper_wif_bssid(const struct wlhelper_wif *w, char *mac, size_t mac_size)
{
	if ((w->link < 0) || !w->mlo.addr[0])
		return wlhelper_get_mac_address(w->ifname, mac, mac_size);

	snprintf(mac, mac_size, "%s", w->mlo.addr);
	return 0;
}

struct mlo_sta_filter {
	char macs[MLO_MAX_STATIONS][18];
	int n;
	wlhelper_station_callback callback;
	void *user_data;
};

static int mlo_sta_filter_cb(const char *ifname, int phy,
                             const struct wlhelper_station_info *station,
                             void *user_data)
{
	struct mlo_sta_filter *f = (struct mlo_sta_filter *)user_data;
	int i;

	for (i = 0; i < f->n; i++) {
		if (strcasecmp(f->macs[i], station->mac) == 0)
			return f->callback(ifname, phy, station, f->user_data);
	}

	return 0; /* on another link */
}

/*
 * 'iw station dump' on the MLD lists every client of every link without
 * saying which link it is on, so ask hostapd's control socket for that link.
 */
int wlhelper_wif_foreach_station(const struct wlhelper_wif *w,
                                 wlhelper_station_callback callback,
                                 void *user_data)
{
	struct mlo_sta_filter *f;
	FILE *fp;
	char cmd[128], line[LINE_BUFFER_SIZE];
	int ret;

	if (w->link < 0)
		return wlhelper_foreach_station(w->ifname, w->phy, callback, user_data);

	if ((f = calloc(1, sizeof(*f))) == NULL)
		return -1;

	snprintf(cmd, sizeof(cmd), "hostapd_cli -p /var/run/hostapd -i %s -l %d list_sta 2>/dev/null",
	         w->ifname, w->mlo.id);
	if ((fp = popen(cmd, "r")) != NULL) {
		while ((f->n < MLO_MAX_STATIONS) && (fgets(line, sizeof(line), fp) != NULL)) {
			char mac[18];

			if ((sscanf(line, "%17s", mac) == 1) && (strlen(mac) == 17) && (mac[2] == ':'))
				snprintf(f->macs[f->n++], sizeof(f->macs[0]), "%s", mac);
		}
		pclose(fp);
	}

	if (!f->n) {
		free(f);
		return 0;
	}

	f->callback = callback;
	f->user_data = user_data;
	ret = wlhelper_foreach_station(w->ifname, w->phy, mlo_sta_filter_cb, f);

	free(f);
	return ret;
}

#endif /* TOMATO64 */
