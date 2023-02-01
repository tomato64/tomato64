/*
 *
 * Tomato Firmware
 * Copyright (C) 2006-2009 Jonathan Zarate
 *
 */


#include "tomato.h"

#include <ctype.h>
#include <wlutils.h>
#include <sys/ioctl.h>
#include <wlscan.h>

#ifndef WL_BSS_INFO_VERSION
#error WL_BSS_INFO_VERSION
#endif
#if WL_BSS_INFO_VERSION < 108
#error WL_BSS_INFO_VERSION < 108
#endif
#define WLC_IOCTL_MAXLEN_ADDON	2048
#define MAX_WLIF_SCAN		3 /* allow to scan using up to MAX_WLIF_SCAN wireless ifaces */
#define WLC_SCAN_MAX_RETRY	6 /* 6 * 500 ms retry time */
#define WLC_SCAN_TIME_EXT	40
/* needed by logmsg() */
#define LOGMSG_DISABLE		DISABLE_SYSLOG_OSM
#define LOGMSG_NVDEBUG		"wl_debug"

#if defined(TCONFIG_BLINK) || defined(TCONFIG_BCMARM) /* RT-N+ */
#define WLC_SCAN_RESULT_BUF_LEN_TOMATO WLC_SCAN_RESULT_BUF_LEN /* 32 * 1024 */
#else
#define WLC_SCAN_RESULT_BUF_LEN_TOMATO WLC_IOCTL_MAXLEN /* 8192 */
#endif

static int unit = 0;
static int subunit = 0;

typedef struct {
	int unit_filter;
	char comma;
	struct {
		int ap;
		int radio;
		int scan_time;
	} wif[MAX_WLIF_SCAN];
} scan_list_t;

/*
 * Country names and abbreviations from ISO 3166
 */
typedef struct {
	const char *name;	/* Name for FreshTomato */
	const char *abbrev;	/* Abbreviation */
} cntry_name_t;

cntry_name_t cntry_names[] = {
{"#a (debug)",		"#a"},
{"#r (debug)",		"#r"},
{"AFGHANISTAN",		"AF"},
{"ALBANIA",		"AL"},
{"ALGERIA",		"DZ"},
{"ARGENTINA",		"AR"},
{"AUSTRALIA",		"AU"},
{"AUSTRIA",		"AT"},
{"BAHRAIN",		"BH"},
{"BELGIUM",		"BE"},
{"BRAZIL",		"BR"},
{"BULGARIA",		"BG"},
{"CANADA",		"CA"},
{"CHILE",		"CL"},
{"CHINA",		"CN"},
{"COLOMBIA",		"CO"},
{"CONGO",		"CG"},
{"CROATIA",		"HR"},
{"CUBA",		"CU"},
{"CZECH REPUBLIC",	"CZ"},
{"DENMARK",		"DK"},
{"DOMINICAN REP.",	"DO"},
{"EUROPE",		"EU"},
{"ECUADOR",		"EC"},
{"EGYPT",		"EG"},
{"ESTONIA",		"EE"},
{"ETHIOPIA",		"ET"},
{"FINLAND",		"FI"},
{"FRANCE",		"FR"},
{"GERMANY",		"DE"},
{"GREECE",		"GR"},
{"GREENLAND",		"GL"},
{"GUAM",		"GU"},
{"GUATEMALA",		"GT"},
{"HAITI",		"HT"},
{"VATICAN CITY",	"VA"},
{"HONDURAS",		"HN"},
{"HONG KONG",		"HK"},
{"HUNGARY",		"HU"},
{"ICELAND",		"IS"},
{"INDIA",		"IN"},
{"INDONESIA",		"ID"},
{"IRAN",		"IR"},
{"IRAQ",		"IQ"},
{"IRELAND",		"IE"},
{"ISRAEL",		"IL"},
{"ITALY",		"IT"},
{"JAMAICA",		"JM"},
{"JAPAN",		"JP"},
{"KOREA1",		"KP"},
{"KOREA2",		"KR"},
{"KUWAIT",		"KW"},
{"LIECHTENSTEIN",	"LI"},
{"LITHUANIA",		"LT"},
{"LUXEMBOURG",		"LU"},
{"MACAO",		"MO"},
{"MACEDONIA",		"MK"},
{"MADAGASCAR",		"MG"},
{"MALAWI",		"MW"},
{"MALAYSIA",		"MY"},
{"MEXICO",		"MX"},
{"MICRONESIA",		"FM"},
{"MOLDOVA",		"MD"},
{"MONGOLIA",		"MN"},
{"MONTENEGRO",		"ME"},
{"MOROCCO",		"MA"},
{"NAMIBIA",		"NA"},
{"NAURU",		"NR"},
{"NEPAL",		"NP"},
{"NETHERLANDS",		"NL"},
{"NEW ZEALAND",		"NZ"},
{"NIGERIA",		"NG"},
{"NORWAY",		"NO"},
{"OMAN",		"OM"},
{"PAKISTAN",		"PK"},
{"PALAU",		"PW"},
{"PALESTINIAN",		"PS"},
{"PANAMA",		"PA"},
{"PARAGUAY",		"PY"},
{"PERU",		"PE"},
{"PHILIPPINES",		"PH"},
{"PITCAIRN",		"PN"},
{"POLAND",		"PL"},
{"PORTUGAL",		"PT"},
{"PUERTO RICO",		"PR"},
{"QATAR",		"QA"},
{"ROMANIA",		"RO"},
{"RUSSIA",		"RU"},
{"RWANDA",		"RW"},
{"SAN MARINO",		"SM"},
{"SAUDI ARABIA",	"SA"},
{"SENEGAL",		"SN"},
{"SERBIA",		"RS"},
{"SEYCHELLES",		"SC"},
{"SIERRA LEONE",	"SL"},
{"SINGAPORE",		"SG"},
{"SLOVAKIA",		"SK"},
{"SLOVENIA",		"SI"},
{"SOMALIA",		"SO"},
{"SOUTH AFRICA",	"ZA"},
{"SPAIN",		"ES"},
{"SRI LANKA",		"LK"},
{"SUDAN",		"SD"},
{"SWEDEN",		"SE"},
{"SWITZERLAND",		"CH"},
{"SYRIAN",		"SY"},
{"TAIWAN",		"TW"},
{"THAILAND",		"TH"},
{"TUNISIA",		"TN"},
{"TURKEY",		"TR"},
{"TURKMENISTAN",	"TM"},
{"TURKS",		"TC"},
{"TUVALU",		"TV"},
{"UGANDA",		"UG"},
{"UKRAINE",		"UA"},
{"U.AR. EMIRATES",	"AE"},
{"GREAT BRITAIN",	"GB"},
{"USA",			"US"},
{"URUGUAY",		"UY"},
{"UZBEKISTAN",		"UZ"},
{"VENEZUELA",		"VE"},
{"YEMEN",		"YE"},
{"YUGOSLAVIA",		"YU"},
{"ZAMBIA",		"ZM"},
{"ZIMBABWE",		"ZW"},
{NULL,			NULL}
};

static void check_wl_unit(const char *unitarg)
{
	char ifname[12], *wlunit;
	unit = 0; subunit = 0;

	wlunit = (unitarg && *unitarg) ? (char *)unitarg : webcgi_safeget("_wl_unit", nvram_safe_get("wl_unit"));
	snprintf(ifname, sizeof(ifname), "wl%s", wlunit);
	get_ifname_unit(ifname, &unit, &subunit);

	logmsg(LOG_DEBUG, "*** %s: unitarg: %s, _wl_unit: %s, ifname: %s, unit: %d, subunit: %d", __FUNCTION__, unitarg, webcgi_safeget("_wl_unit", nvram_safe_get("wl_unit")), ifname, unit, subunit);
}

static void wl_restore(char *wif, int unit, int ap, int radio, int scan_time)
{
	int cur_radio_status = get_radio(unit); /* check radio status (ON/OFF) */

#if !defined(CONFIG_BCMWL6) && !defined(TCONFIG_BLINK) /* only mips RT branch */
	if (ap > 0) {
		wl_ioctl(wif, WLC_SET_AP, &ap, sizeof(ap));

		if (!radio)
			set_radio(1, unit);

		eval("wl", "-i", wif, "up"); /* without this the router may reboot */

#if WL_BSS_INFO_VERSION >= 108
		/* no idea why this voodoo sequence works to wake up wl */
		eval("wl", "-i", wif, "ssid", "");
		eval("wl", "-i", wif, "ssid", nvram_safe_get(wl_nvname("ssid", unit, 0)));
#endif
	}
#endif /* only mips RT branch */

	if (scan_time > 0) {
		/* restore original scan channel time */
		wl_ioctl(wif, WLC_SET_SCAN_CHANNEL_TIME, &scan_time, sizeof(scan_time));
	}


	if (cur_radio_status != radio) {
		set_radio(radio, unit);
	}
}

static int start_scan(int idx, int unit, int subunit, void *param)
{
	scan_list_t *rp = param;
	wl_scan_params_t sp;
	char *wif;
#if !defined(CONFIG_BCMWL6) && !defined(TCONFIG_BLINK) /* only mips RT branch */
	int zero = 0;
#endif /* only mips RT branch */
	int retry = WLC_SCAN_MAX_RETRY;
	int scan_time = WLC_SCAN_TIME_EXT;

	if ((idx >= MAX_WLIF_SCAN) || (rp->unit_filter >= 0 && rp->unit_filter != unit))
		return 0;

	wif = nvram_safe_get(wl_nvname("ifname", unit, 0));

	/* init scan params */
	memset(&sp, 0, sizeof(sp)); /* clean-up */
	memcpy(&sp.bssid, &ether_bcast, ETHER_ADDR_LEN);
	sp.bss_type = DOT11_BSSTYPE_INFRASTRUCTURE;
	sp.nprobes = -1; /* default */
	sp.active_time = -1; /* default */
	sp.passive_time = -1; /* default */
	sp.home_time = -1; /* default */
	sp.channel_num = 0; /* use all available channels */

	if (wl_ioctl(wif, WLC_GET_AP, &(rp->wif[idx].ap), sizeof(rp->wif[idx].ap)) < 0) /* unable to get AP mode */
		return 0;

#if !defined(CONFIG_BCMWL6) && !defined(TCONFIG_BLINK) /* only mips RT branch */
	if (rp->wif[idx].ap > 0)
		wl_ioctl(wif, WLC_SET_AP, &zero, sizeof(zero));
#endif /* only mips RT branch */

	/* set scan type based on the ap mode; AP --> passive scan, Media Bridge / Wireless Ethernet Bridge / WL Client --> active scan */
	sp.scan_type = rp->wif[idx].ap ? DOT11_SCANTYPE_PASSIVE : DOT11_SCANTYPE_ACTIVE /* default */;

	/* extend scan channel time to get more AP probe resp */
	wl_ioctl(wif, WLC_GET_SCAN_CHANNEL_TIME, &(rp->wif[idx].scan_time), sizeof(rp->wif[idx].scan_time));
	if (rp->wif[idx].scan_time < scan_time)
		wl_ioctl(wif, WLC_SET_SCAN_CHANNEL_TIME, &scan_time, sizeof(scan_time));

	rp->wif[idx].radio = get_radio(unit);
	if (!(rp->wif[idx].radio))
		set_radio(1, unit);

	while (retry--) { /* retry if needed */
		if (wl_ioctl(wif, WLC_SCAN, &sp, WL_SCAN_PARAMS_FIXED_SIZE) == 0)
			return 1;

		if (retry)
			usleep(500 * 1000); /* wait 500 ms */
	}

	/* unable to start scan */
	wl_restore(wif, unit, rp->wif[idx].ap, rp->wif[idx].radio, rp->wif[idx].scan_time);

	return 0;
}

int wpa_selector_to_bitfield(const unsigned char *s)
{
	if (memcmp(s, WPA_CIPHER_SUITE_NONE, WPA_SELECTOR_LEN) == 0)
		return WPA_CIPHER_NONE_;
	if (memcmp(s, WPA_CIPHER_SUITE_WEP40, WPA_SELECTOR_LEN) == 0)
		return WPA_CIPHER_WEP40_;
	if (memcmp(s, WPA_CIPHER_SUITE_TKIP, WPA_SELECTOR_LEN) == 0)
		return WPA_CIPHER_TKIP_;
	if (memcmp(s, WPA_CIPHER_SUITE_CCMP, WPA_SELECTOR_LEN) == 0)
		return WPA_CIPHER_CCMP_;
	if (memcmp(s, WPA_CIPHER_SUITE_WEP104, WPA_SELECTOR_LEN) == 0)
		return WPA_CIPHER_WEP104_;

	return 0;
}

int rsn_selector_to_bitfield(const unsigned char *s)
{
	if (memcmp(s, RSN_CIPHER_SUITE_NONE, RSN_SELECTOR_LEN) == 0)
		return WPA_CIPHER_NONE_;
	if (memcmp(s, RSN_CIPHER_SUITE_WEP40, RSN_SELECTOR_LEN) == 0)
		return WPA_CIPHER_WEP40_;
	if (memcmp(s, RSN_CIPHER_SUITE_TKIP, RSN_SELECTOR_LEN) == 0)
		return WPA_CIPHER_TKIP_;
	if (memcmp(s, RSN_CIPHER_SUITE_CCMP, RSN_SELECTOR_LEN) == 0)
		return WPA_CIPHER_CCMP_;
	if (memcmp(s, RSN_CIPHER_SUITE_WEP104, RSN_SELECTOR_LEN) == 0)
		return WPA_CIPHER_WEP104_;

	return 0;
}

int wpa_key_mgmt_to_bitfield(const unsigned char *s)
{
	if (memcmp(s, WPA_AUTH_KEY_MGMT_UNSPEC_802_1X, WPA_SELECTOR_LEN) == 0)
		return WPA_KEY_MGMT_IEEE8021X_;
	if (memcmp(s, WPA_AUTH_KEY_MGMT_PSK_OVER_802_1X, WPA_SELECTOR_LEN) == 0)
		return WPA_KEY_MGMT_PSK_;
	if (memcmp(s, WPA_AUTH_KEY_MGMT_NONE, WPA_SELECTOR_LEN) == 0)
		return WPA_KEY_MGMT_WPA_NONE_;

	return 0;
}

int rsn_key_mgmt_to_bitfield(const unsigned char *s)
{
	if (memcmp(s, RSN_AUTH_KEY_MGMT_UNSPEC_802_1X, RSN_SELECTOR_LEN) == 0)
		return WPA_KEY_MGMT_IEEE8021X2_;
	if (memcmp(s, RSN_AUTH_KEY_MGMT_PSK_OVER_802_1X, RSN_SELECTOR_LEN) == 0)
		return WPA_KEY_MGMT_PSK2_;

	return 0;
}

int wpa_parse_wpa_ie_wpa(const unsigned char *wpa_ie, size_t wpa_ie_len, struct wpa_ie_data *data)
{
	const struct wpa_ie_hdr *hdr;
	const unsigned char *pos;
	int i, left, count;

	data->proto = WPA_PROTO_WPA_;
	data->pairwise_cipher = WPA_CIPHER_TKIP_;
	data->group_cipher = WPA_CIPHER_TKIP_;
	data->key_mgmt = WPA_KEY_MGMT_IEEE8021X_;
	data->capabilities = 0;
	data->pmkid = NULL;
	data->num_pmkid = 0;

	if (wpa_ie_len == 0) /* No WPA IE - fail silently */
		return -1;

	if (wpa_ie_len < sizeof(struct wpa_ie_hdr))
		return -1;

	hdr = (const struct wpa_ie_hdr *) wpa_ie;

	if ((hdr->elem_id != DOT11_MNG_WPA_ID) || (hdr->len != wpa_ie_len - 2) || (memcmp(&hdr->oui, WPA_OUI_TYPE_ARR, WPA_SELECTOR_LEN) != 0) || (WPA_GET_LE16(hdr->version) != WPA_VERSION_))
		return -1;

	pos = (const unsigned char *) (hdr + 1);
	left = wpa_ie_len - sizeof(*hdr);

	if (left >= WPA_SELECTOR_LEN) {
		data->group_cipher = wpa_selector_to_bitfield(pos);
		pos += WPA_SELECTOR_LEN;
		left -= WPA_SELECTOR_LEN;
	}
	else if (left > 0)
		return -1;

	if (left >= 2) {
		data->pairwise_cipher = 0;
		count = WPA_GET_LE16(pos);
		pos += 2;
		left -= 2;
		if ((count == 0) || (left < count * WPA_SELECTOR_LEN))
			return -1;

		for (i = 0; i < count; i++) {
			data->pairwise_cipher |= wpa_selector_to_bitfield(pos);
			pos += WPA_SELECTOR_LEN;
			left -= WPA_SELECTOR_LEN;
		}
	}
	else if (left == 1)
		return -1;

	if (left >= 2) {
		data->key_mgmt = 0;
		count = WPA_GET_LE16(pos);
		pos += 2;
		left -= 2;
		if ((count == 0) || (left < count * WPA_SELECTOR_LEN))
			return -1;

		for (i = 0; i < count; i++) {
			data->key_mgmt |= wpa_key_mgmt_to_bitfield(pos);
			pos += WPA_SELECTOR_LEN;
			left -= WPA_SELECTOR_LEN;
		}
	}
	else if (left == 1)
		return -1;

	if (left >= 2) {
		data->capabilities = WPA_GET_LE16(pos);
		pos += 2;
		left -= 2;
	}

	if (left > 0)
		return -1;

	return 0;
}

int wpa_parse_wpa_ie_rsn(const unsigned char *rsn_ie, size_t rsn_ie_len, struct wpa_ie_data *data)
{
	const struct rsn_ie_hdr *hdr;
	const unsigned char *pos;
	int i, left, count;

	data->proto = WPA_PROTO_RSN_;
	data->pairwise_cipher = WPA_CIPHER_CCMP_;
	data->group_cipher = WPA_CIPHER_CCMP_;
	data->key_mgmt = WPA_KEY_MGMT_IEEE8021X2_;
	data->capabilities = 0;
	data->pmkid = NULL;
	data->num_pmkid = 0;

	if (rsn_ie_len == 0)
		return -1;

	if (rsn_ie_len < sizeof(struct rsn_ie_hdr))
		return -1;

	hdr = (const struct rsn_ie_hdr *) rsn_ie;

	if ((hdr->elem_id != DOT11_MNG_RSN_ID) || (hdr->len != rsn_ie_len - 2) || (WPA_GET_LE16(hdr->version) != RSN_VERSION_))
		return -1;

	pos = (const unsigned char *) (hdr + 1);
	left = rsn_ie_len - sizeof(*hdr);

	if (left >= RSN_SELECTOR_LEN) {
		data->group_cipher = rsn_selector_to_bitfield(pos);
		pos += RSN_SELECTOR_LEN;
		left -= RSN_SELECTOR_LEN;
	}
	else if (left > 0)
		return -1;

	if (left >= 2) {
		data->pairwise_cipher = 0;
		count = WPA_GET_LE16(pos);
		pos += 2;
		left -= 2;
		if ((count == 0) || (left < count * RSN_SELECTOR_LEN))
			return -1;

		for (i = 0; i < count; i++) {
			data->pairwise_cipher |= rsn_selector_to_bitfield(pos);
			pos += RSN_SELECTOR_LEN;
			left -= RSN_SELECTOR_LEN;
		}
	}
	else if (left == 1)
		return -1;

	if (left >= 2) {
		data->key_mgmt = 0;
		count = WPA_GET_LE16(pos);
		pos += 2;
		left -= 2;
		if ((count == 0) || (left < count * RSN_SELECTOR_LEN))
			return -1;

		for (i = 0; i < count; i++) {
			data->key_mgmt |= rsn_key_mgmt_to_bitfield(pos);
			pos += RSN_SELECTOR_LEN;
			left -= RSN_SELECTOR_LEN;
		}
	}
	else if (left == 1)
		return -1;

	if (left >= 2) {
		data->capabilities = WPA_GET_LE16(pos);
		pos += 2;
		left -= 2;
	}

	if (left >= 2) {
		data->num_pmkid = WPA_GET_LE16(pos);
		pos += 2;
		left -= 2;
		if (left < data->num_pmkid * PMKID_LEN)
			data->num_pmkid = 0;
		else {
			data->pmkid = pos;
			pos += data->num_pmkid * PMKID_LEN;
			left -= data->num_pmkid * PMKID_LEN;
		}
	}

	return 0;
}

int wpa_parse_wpa_ie(const unsigned char *wpa_ie, size_t wpa_ie_len,
		     struct wpa_ie_data *data)
{
	if (wpa_ie_len >= 1 && wpa_ie[0] == DOT11_MNG_RSN_ID)
		return wpa_parse_wpa_ie_rsn(wpa_ie, wpa_ie_len, data);
	else
		return wpa_parse_wpa_ie_wpa(wpa_ie, wpa_ie_len, data);
}

static int get_scan_results(int idx, int unit, int subunit, void *param)
{
	scan_list_t *rp = param;

	if ((idx >= MAX_WLIF_SCAN) || (rp->unit_filter >= 0 && rp->unit_filter != unit))
		return 0;

	char *wif;
	wl_scan_results_t *results;
	wl_bss_info_t *bssi;
	struct bss_ie_hdr *ie;
	int r, retry;
	int chan_bw;
#ifdef CONFIG_BCMWL6
	int chanspec = 0, ctr_channel = 0;
#endif

	/* get results */
	wif = nvram_safe_get(wl_nvname("ifname", unit, 0));

	results = malloc(WLC_SCAN_RESULT_BUF_LEN_TOMATO + sizeof(*results));
	if (!results) {
		/* not enough memory */
		wl_restore(wif, unit, rp->wif[idx].ap, rp->wif[idx].radio, rp->wif[idx].scan_time);
		return 0;
	}

	results->buflen = WLC_SCAN_RESULT_BUF_LEN_TOMATO;
	results->version = WL_BSS_INFO_VERSION;

	/* Keep trying to get scan results for up to 3 secs (6 * 500 ms) */
	retry = WLC_SCAN_MAX_RETRY;
	r = -1;
	while (retry--) {
		r = wl_ioctl(wif, WLC_SCAN_RESULTS, results, WLC_SCAN_RESULT_BUF_LEN_TOMATO);

		if (r >= 0)
			break;

		usleep(500 * 1000); /* wait 500 ms */
	}

	wl_restore(wif, unit, rp->wif[idx].ap, rp->wif[idx].radio, rp->wif[idx].scan_time);

	if (r < 0) {
		free(results);
		/* unable to obtain scan results */
		return 0;
	}

	/* format for javascript */
	unsigned int i, k;
	int left;
	char macstr[18];
	NDIS_802_11_NETWORK_TYPE NetWorkType;
	unsigned char *bssidp;
	unsigned char rate;

	bssi = &results->bss_info[0];
	for (i = 0; i < results->count; ++i) {

		bssidp = (unsigned char *)&bssi->BSSID;
		snprintf(macstr, sizeof(macstr), "%02X:%02X:%02X:%02X:%02X:%02X", (unsigned char)bssidp[0], (unsigned char)bssidp[1], (unsigned char)bssidp[2], (unsigned char)bssidp[3], (unsigned char)bssidp[4], (unsigned char)bssidp[5]);

		strcpy(apinfos[0].BSSID, macstr);
		memset(apinfos[0].SSID, 0x0, 33);
		memcpy(apinfos[0].SSID, bssi->SSID, bssi->SSID_len);
		apinfos[0].channel = (uint8)(bssi->chanspec & WL_CHANSPEC_CHAN_MASK);

		if (bssi->ctl_ch == 0) /* check control channel number; if 0 calc/replace it --> backup! */
		{
			apinfos[0].ctl_ch = apinfos[0].channel;
#ifdef CONFIG_BCMWL6
			chanspec = bssi->chanspec;
			ctr_channel = CHSPEC_CHANNEL(chanspec);
			if (CHSPEC_IS40(chanspec))
				ctr_channel = ctr_channel + (CHSPEC_SB_LOWER(chanspec) ? -2 : 2);
			else if (CHSPEC_IS80(chanspec))
				ctr_channel += (((chanspec & WL_CHANSPEC_CTL_SB_MASK) == WL_CHANSPEC_CTL_SB_LLU) ? -2 : -6 ); /* upper is actually LLU */

			apinfos[0].ctl_ch = ctr_channel; /* use calculated value */
#endif
		}
		else { /* default */
			apinfos[0].ctl_ch = bssi->ctl_ch; /* get control channel number */
		}

		if (bssi->RSSI >= -50)
			apinfos[0].RSSI_Quality = 100;
		else if (bssi->RSSI >= -80)	/* between -50 ~ -80dbm */
			apinfos[0].RSSI_Quality = (int)(24 + ((bssi->RSSI + 80) * 26)/10);
		else if (bssi->RSSI >= -90)	/* between -80 ~ -90dbm */
			apinfos[0].RSSI_Quality = (int)(((bssi->RSSI + 90) * 26)/10);
		else				/* < -84 dbm */
			apinfos[0].RSSI_Quality = 0;

		if ((bssi->capability & 0x10) == 0x10)
			apinfos[0].wep = 1;
		else
			apinfos[0].wep = 0;

		apinfos[0].wpa = 0;

		NetWorkType = Ndis802_11DS;
		if ((uint8)(bssi->chanspec & WL_CHANSPEC_CHAN_MASK) <= 14) {
			for (k = 0; k < bssi->rateset.count; k++) {
				rate = bssi->rateset.rates[k] & 0x7f; /* Mask out basic rate set bit */
				if ((rate == 2) || (rate == 4) || (rate == 11) || (rate == 22))
					continue;
				else {
					NetWorkType = Ndis802_11OFDM24;
					break;
				}
			}
		}
		else
			NetWorkType = Ndis802_11OFDM5;

		if (bssi->n_cap) {
			if (NetWorkType == Ndis802_11OFDM5) {
#ifdef CONFIG_BCMWL6
				if (bssi->vht_cap)
					NetWorkType = Ndis802_11OFDM5_VHT;
				else
#endif
				NetWorkType = Ndis802_11OFDM5_N;
			}
			else
				NetWorkType = Ndis802_11OFDM24_N;
		}

		apinfos[0].NetworkType = NetWorkType;

		ie = (struct bss_ie_hdr *) ((unsigned char *) bssi + bssi->ie_offset);
		for (left = bssi->ie_length; left > 0; /* look for RSN IE first */
		     left -= (ie->len + 2), ie = (struct bss_ie_hdr *) ((unsigned char *) ie + 2 + ie->len))
		{
			if (ie->elem_id != DOT11_MNG_RSN_ID)
				continue;

			if (wpa_parse_wpa_ie(&ie->elem_id, ie->len + 2, &apinfos[0].wid) == 0) {
				apinfos[0].wpa = 1;
				goto next_info;
			}
		}

		ie = (struct bss_ie_hdr *) ((unsigned char *) bssi + bssi->ie_offset);
		for (left = bssi->ie_length; left > 0; /* then look for WPA IE */
		     left -= (ie->len + 2), ie = (struct bss_ie_hdr *) ((unsigned char *) ie + 2 + ie->len))
		{
			if (ie->elem_id != DOT11_MNG_WPA_ID)
				continue;

			if (wpa_parse_wpa_ie(&ie->elem_id, ie->len + 2, &apinfos[0].wid) == 0) {
				apinfos[0].wpa = 1;
				break;
			}
		}

next_info:
#ifdef CONFIG_BCMWL6
		if (CHSPEC_IS8080(bssi->chanspec))
			chan_bw = 8080;
		else if (CHSPEC_IS160(bssi->chanspec))
			chan_bw = 160;
		else if (CHSPEC_IS80(bssi->chanspec))
			chan_bw = 80;
		else if (CHSPEC_IS40(bssi->chanspec))
			chan_bw = 40;
		else if (CHSPEC_IS20(bssi->chanspec))
			chan_bw = 20;
		else
			chan_bw = 10;
#else /* for SDK5 */
		if (CHSPEC_IS40(bssi->chanspec))
			chan_bw = 40;
		else if (CHSPEC_IS20(bssi->chanspec))
			chan_bw = 20;
		else
			chan_bw = 10;
#endif
		/* note: provide/use control channel and not the actual channel because we use it for wireless survey and scan button at basic-network.asp */
		web_printf("%c['%s','%s',%d,%d,%d,%d,", rp->comma,
		           apinfos[0].BSSID, apinfos[0].SSID, bssi->RSSI, apinfos[0].ctl_ch,
		           chan_bw, apinfos[0].RSSI_Quality);
		rp->comma = ',';

		if ((apinfos[0].NetworkType == Ndis802_11FH) || (apinfos[0].NetworkType == Ndis802_11DS))
				web_printf("'%s',", "11b");
		else if (apinfos[0].NetworkType == Ndis802_11OFDM5)
				web_printf("'%s',", "11a");
		else if (apinfos[0].NetworkType == Ndis802_11OFDM5_VHT)
				web_printf("'%s',", "11ac");
		else if (apinfos[0].NetworkType == Ndis802_11OFDM5_N)
				web_printf("'%s',", "11a/n");
		else if (apinfos[0].NetworkType == Ndis802_11OFDM24)
				web_printf("'%s',", "11b/g");
		else if (apinfos[0].NetworkType == Ndis802_11OFDM24_N)
				web_printf("'%s',", "11b/g/n");
		else
				web_printf("'%s',", "unknown");

		if (apinfos[0].wpa == 1) {
			if (apinfos[0].wid.key_mgmt == WPA_KEY_MGMT_IEEE8021X_)
				web_printf("'%s',", "WPA-Enterprise");
			else if (apinfos[0].wid.key_mgmt == WPA_KEY_MGMT_IEEE8021X2_)
				web_printf("'%s',", "WPA2-Enterprise");
			else if (apinfos[0].wid.key_mgmt == WPA_KEY_MGMT_PSK_)
				web_printf("'%s',", "WPA-Personal");
			else if (apinfos[0].wid.key_mgmt == WPA_KEY_MGMT_PSK2_)
				web_printf("'%s',", "WPA2-Personal");
			else if (apinfos[0].wid.key_mgmt == WPA_KEY_MGMT_NONE_)
				web_printf("'%s',", "NONE");
			else if (apinfos[0].wid.key_mgmt == WPA_KEY_MGMT_IEEE8021X_NO_WPA_)
				web_printf("'%s',", "IEEE 802.1X");
			else
				web_printf("'%s',", "Unknown");
		}
		else if (apinfos[0].wep == 1)
			web_printf("'%s',", "Unknown");
		else
			web_printf("'%s',", "Open System");

		if (apinfos[0].wpa == 1) {
			if (apinfos[0].wid.pairwise_cipher == WPA_CIPHER_NONE_)
				web_printf("'%s',", "NONE");
			else if (apinfos[0].wid.pairwise_cipher == WPA_CIPHER_WEP40_)
				web_printf("'%s',", "WEP");
			else if (apinfos[0].wid.pairwise_cipher == WPA_CIPHER_WEP104_)
				web_printf("'%s',", "WEP");
			else if (apinfos[0].wid.pairwise_cipher == WPA_CIPHER_TKIP_)
				web_printf("'%s',", "TKIP");
			else if (apinfos[0].wid.pairwise_cipher == WPA_CIPHER_CCMP_)
				web_printf("'%s',", "AES");
			else if (apinfos[0].wid.pairwise_cipher == (WPA_CIPHER_TKIP_ | WPA_CIPHER_CCMP_))
				web_printf("'%s',", "TKIP+AES");
			else
				web_printf("'%s',", "Unknown");
		}
		else if (apinfos[0].wep == 1)
			web_printf("'%s',", "WEP");
		else
			web_printf("'%s',", "NONE");

		web_printf("'%s']", CHSPEC_IS2G(bssi->chanspec) ? "2.4" : "5");

		bssi = (wl_bss_info_t*)((uint8*)bssi + bssi->length);
	}
	free(results);

	return 1;
}

/* returns: ['bssid','ssid',channel,capabilities,rssi,noise,[rates,]],  or  [null,'error message'] */
void asp_wlscan(int argc, char **argv)
{
	scan_list_t rp;

	memset(&rp, 0, sizeof(rp));
	rp.comma = ' ';
	rp.unit_filter = (argc > 0) ? atoi(argv[0]) : (-1);

	web_puts("\nwlscandata = [");

	/* scan */
	if (foreach_wif(0, &rp, start_scan) == 0) {
		web_puts("[null,'Unable to start scan.']];\n");
		return;
	}

#if defined(TCONFIG_BLINK) || defined(TCONFIG_BCMARM) /* RT-N+ */
	sleep(3); /* dual-/tri-band router - scan result for 5 GHz survey after ~3 sec available - we need to wait... */
#else
	sleep(1); /* only 2,4 GHz - scan result after ~1 sec available */
#endif

	/* get results */
	if (foreach_wif(0, &rp, get_scan_results) == 0) {
		web_puts("[null,'Unable to obtain scan results.']];\n");
		return;
	}

	web_puts("];\n");
}

void wo_wlradio(char *url)
{
	char *enable;
	char sunit[10];

	check_wl_unit(NULL);

	parse_asp("saved.asp");

	if (nvram_get_int(wl_nvname("radio", unit, 0))) {
		if ((enable = webcgi_get("enable")) != NULL) {
			web_close();
			sleep(2);
			snprintf(sunit, sizeof(sunit), "%d", unit);
			eval("radio", atoi(enable) ? "on" : "off", sunit);
			return;
		}
	}
}

static int read_noise(int unit)
{
	int v;

	/* WLC_GET_PHY_NOISE = 135 */
	if (wl_ioctl(nvram_safe_get(wl_nvname("ifname", unit, 0)), 135, &v, sizeof(v)) == 0) {
		char s[32];
		snprintf(s, sizeof(s), "%d", v);
		nvram_set(wl_nvname("tnoise", unit, 0), s);
		return v;
	}

	return -99;
}

static int get_wlnoise(int client, int unit)
{
	int v;

	if (client)
		v = read_noise(unit);
	else {
		/* added read_noise here, does not take AP down (just reads from register inside Broadcom ASIC). Read keeps value current */
		v = read_noise(unit);
		v = nvram_get_int(wl_nvname("tnoise", unit, 0));
		if ((v >= 0) || (v < -100))
			v = -99;
	}

	return v;
}

static int print_wlnoise(int idx, int unit, int subunit, void *param)
{
	web_printf("%c%d", (idx == 0) ? ' ' : ',', get_wlnoise(wl_client(unit, 0), unit));

	return 0;
}

void asp_wlnoise(int argc, char **argv)
{
	web_puts("\nwlnoise = [");
	foreach_wif(0, NULL, print_wlnoise);
	web_puts(" ];\n");
}

static int wl_chanfreq(uint ch, int band)
{
	if ((band == WLC_BAND_2G && (ch < 1 || ch > 14)) || (ch > 200))
		return -1;
	else if ((band == WLC_BAND_2G) && (ch == 14))
		return 2484;
	else
		return ch * 5 + ((band == WLC_BAND_2G) ? 4814 : 10000) / 2;
}

static int not_wlclient(int idx, int unit, int subunit, void *param)
{
	return (!wl_client(unit, subunit));
}

/* returns '1' if all wireless interfaces are in client mode, '0' otherwise */
void asp_wlclient(int argc, char **argv)
{
	web_puts(foreach_wif(1, NULL, not_wlclient) ? "0" : "1");
}

static int print_wlstats(int idx, int unit, int subunit, void *param)
{
	int phytype;
	int rate, client, nbw;
	int chanspec, channel, mhz, band, scan;
	int chanim_enab;
	int interference;
	char retbuf[WLC_IOCTL_SMLEN];
	scb_val_t rssi;
	char *ifname, *ctrlsb;

	ifname = nvram_safe_get(wl_nvname("ifname", unit, 0));
	client = wl_client(unit, 0);

	/* get configured phy type */
	wl_ioctl(ifname, WLC_GET_PHYTYPE, &phytype, sizeof(phytype));

	if (wl_ioctl(ifname, WLC_GET_RATE, &rate, sizeof(rate)) < 0)
		rate = 0;

	if (wl_ioctl(ifname, WLC_GET_BAND, &band, sizeof(band)) < 0)
		band = nvram_get_int(wl_nvname("nband", unit, 0));

	channel = nvram_get_int(wl_nvname("channel", unit, 0));
	scan = 0;
	interference = -1;

	if (wl_phytype_n(phytype)) {
		if (wl_iovar_getint(ifname, "chanspec", &chanspec) != 0) {
			ctrlsb = nvram_safe_get(wl_nvname("nctrlsb", unit, 0));
			nbw = nvram_get_int(wl_nvname("nbw", unit, 0));
		}
		else {
			channel = CHSPEC_CHANNEL(chanspec);
			if (CHSPEC_IS40(chanspec))
				channel = channel + (CHSPEC_SB_LOWER(chanspec) ? -2 : 2);
#ifdef CONFIG_BCMWL6
			else if (CHSPEC_IS80(chanspec))
				channel += (((chanspec & WL_CHANSPEC_CTL_SB_MASK) == WL_CHANSPEC_CTL_SB_LLU) ? -2 : -6); /* upper is actually LLU */

			ctrlsb = (chanspec & WL_CHANSPEC_CTL_SB_MASK) == WL_CHANSPEC_CTL_SB_LOWER ? "lower" : ((chanspec & WL_CHANSPEC_CTL_SB_MASK) == WL_CHANSPEC_CTL_SB_LLU ? "upper" : "none");
			nbw = CHSPEC_IS80(chanspec) ? 80 : (CHSPEC_IS40(chanspec) ? 40 : 20);
#else
			ctrlsb = CHSPEC_SB_LOWER(chanspec) ? "lower" : (CHSPEC_SB_UPPER(chanspec) ? "upper" : "none");
			nbw = CHSPEC_IS40(chanspec) ? 40 : 20;
#endif
		}
	}
	else {
		channel_info_t ch;
		if (wl_ioctl(ifname, WLC_GET_CHANNEL, &ch, sizeof(ch)) == 0) {
			scan = (ch.scan_channel > 0);
			channel = (scan) ? ch.scan_channel : ch.hw_channel;
		}
		ctrlsb = "";
		nbw = 20;
	}

	mhz = (channel) ? wl_chanfreq(channel, band) : 0;
	if (wl_iovar_getint(ifname, "chanim_enab", (int*)(void*)&chanim_enab) != 0)
		chanim_enab = 0;

	if (chanim_enab) {
		if (wl_iovar_getbuf(ifname, "chanim_state", &chanspec, sizeof(chanspec), retbuf, WLC_IOCTL_SMLEN) == 0)
			interference = *(int*)retbuf;
	}

	memset(&rssi, 0, sizeof(rssi));
	if (client) {
		if (wl_ioctl(ifname, WLC_GET_RSSI, &rssi, sizeof(rssi)) != 0)
			rssi.val = -100;
	}

	/* [ radio, is_client, channel, freq (mhz), rate, nctrlsb, nbw, rssi, noise, interference ] */
	web_printf("%c{ radio: %d, client: %d, channel: %c%d, mhz: %d, rate: %d, ctrlsb: '%s', nbw: %d, rssi: %d, noise: %d, intf: %d }\n",
		(idx == 0 ? ' ' : ','),
		get_radio(unit), client, (scan ? '-' : ' '), channel, mhz, rate, ctrlsb, nbw, rssi.val, get_wlnoise(client, unit), interference);

	return 0;
}

void asp_wlstats(int argc, char **argv)
{
	int include_vifs = (argc > 0) ? atoi(argv[0]) : 0;

	web_puts("\nwlstats = [");
	foreach_wif(include_vifs, NULL, print_wlstats);
	web_puts("];\n");
}

static void web_print_wlchan(uint chan, int band)
{
	int mhz;
	if ((mhz = wl_chanfreq(chan, band)) > 0)
		web_printf(",[%d, %d]", chan, mhz);
	else
		web_printf(",[%d, 0]", chan);
}

#ifdef TCONFIG_BCM714
static const uint8 wf_chspec_bw_mhz[] = {5, 10, 20, 40, 80, 160, 160};

#define WF_NUM_BW \
	(sizeof(wf_chspec_bw_mhz)/sizeof(uint8))

/* convert bandwidth from chanspec to MHz */
static uint bw_chspec_to_mhz(chanspec_t chspec)
{
	uint bw;

	bw = (chspec & WL_CHANSPEC_BW_MASK) >> WL_CHANSPEC_BW_SHIFT;
	return (bw >= WF_NUM_BW ? 0 : wf_chspec_bw_mhz[bw]);
}

/* 
 * bw in MHz, return the channel count from the center channel to the
 * the channel at the edge of the band
 */
static int center_chan_to_edge(uint bw)
{
	/* edge channels separated by BW - 10MHz on each side
	 * delta from cf to edge is half of that,
	 * MHz to channel num conversion is 5MHz/channel
	 */
	return (((bw - 20) / 2) / 5);
}

/* 
 * return channel number of the low edge of the band
 * given the center channel and BW
 */
static int channel_low_edge(uint center_ch, uint bw)
{
	return (center_ch - center_chan_to_edge(bw));
}

/* return control channel given center channel and side band */
static int channel_to_ctl_chan(uint center_ch, uint bw, uint sb)
{
	return (channel_low_edge(center_ch, bw) + sb * 4);
}

/*
 * This function returns the channel number that control traffic is being sent on
 */
static int chspec_ctlchan(chanspec_t chspec)
{
	uint center_chan;
	uint bw_mhz;
	uint sb;

	/* Is there a sideband ? */
	if (CHSPEC_IS20(chspec)) {
		return CHSPEC_CHANNEL(chspec);
	} else {
		sb = CHSPEC_CTL_SB(chspec) >> WL_CHANSPEC_CTL_SB_SHIFT;

		bw_mhz = bw_chspec_to_mhz(chspec);
		center_chan = CHSPEC_CHANNEL(chspec) >> WL_CHANSPEC_CHAN_SHIFT;

		return (channel_to_ctl_chan(center_chan, bw_mhz, sb));
	}
}
#endif /* TCONFIG_BCM714 */

static int _wlchanspecs(char *ifname, char *country, int band, int bw, int ctrlsb)
{
	chanspec_t c = 0;
#ifndef TCONFIG_BCM714
	chanspec_t *chanspec;
	int buflen;
#endif
	wl_uint32_list_t *list;
	unsigned int count, i = 0;

	char *buf = (char *)malloc(WLC_IOCTL_MAXLEN);
	if (!buf)
		return 0;

#ifdef TCONFIG_BCM714
	memset(buf, 0, WLC_IOCTL_MAXLEN);
	if (wl_iovar_getbuf(ifname, "chanspecs", &c, sizeof(chanspec_t), buf, WLC_IOCTL_MAXLEN) < 0) {
		free((void *)buf);
		return 0;
	}

	count = 0;
	list = (wl_uint32_list_t *)buf;
	logmsg(LOG_DEBUG, "*** %s: list->count = %d ifname = %s", __FUNCTION__, list->count, ifname);
	for (i = 0; i < list->count; i++) {
		c = (chanspec_t)list->element[i];
		/* create the actual control channel */
		int chan = chspec_ctlchan(c);

		/* country & band are already set at this time ... no need to check! ifname is enough */

		/* filter the list (no support yet for FT) */
		if (CHSPEC_IS8080(c) || CHSPEC_IS160(c)) {
			continue;
		}

		/* filter the list (only upper or lower - no support for the other ones yet ... ) for 40 MHz BW and up! */
		if ((bw >= 40) && CHSPEC_CTL_SB(c) != ctrlsb) {
			continue;
		}
		
		if ((bw == 80)) {
			if (CHSPEC_IS80(c)) {
				web_print_wlchan(chan, band);
				count++;
				continue;
			}
		}
		else if ((bw == 40)) {
			if (CHSPEC_IS40(c)) {
				web_print_wlchan(chan, band);
				count++;
				continue;
			}
		}
		else if ((bw == 20)) {
			if (CHSPEC_IS20(c)) {
				web_print_wlchan(chan, band);
				count++;
				continue;
			}
		}
	}	
#else /* SDK5 + SDK6.37 + SDK7 - keep "old" way for now! */
	strcpy(buf, "chanspecs");
	buflen = strlen(buf) + 1;

	c |= (band == WLC_BAND_5G) ? WL_CHANSPEC_BAND_5G : WL_CHANSPEC_BAND_2G;
#ifdef CONFIG_BCMWL6
	c |= (bw == 20) ? WL_CHANSPEC_BW_20 : (bw == 40 ? WL_CHANSPEC_BW_40 : WL_CHANSPEC_BW_80);
#else
	c |= (bw == 20) ? WL_CHANSPEC_BW_20 : WL_CHANSPEC_BW_40;
#endif

	chanspec = (chanspec_t *)(buf + buflen);
	*chanspec = c;
	buflen += (sizeof(chanspec_t));
	strncpy(buf + buflen, country, WLC_CNTRY_BUF_SZ);
	buflen += WLC_CNTRY_BUF_SZ;

	/* add list */
	list = (wl_uint32_list_t *)(buf + buflen);
	list->count = WL_NUMCHANSPECS;
	buflen += sizeof(uint32)*(WL_NUMCHANSPECS + 1);

	if (wl_ioctl(ifname, WLC_GET_VAR, buf, buflen) < 0) {
		free((void *)buf);
		return 0;
	}

	count = 0;
	list = (wl_uint32_list_t *)buf;
	for (i = 0; i < list->count; i++) {
		c = (chanspec_t)list->element[i];
		/* skip upper.. (take only one of the lower or upper) */
#ifdef CONFIG_BCMWL6
		if (bw >= 40 && (CHSPEC_CTL_SB(c) != ctrlsb))
#else
		if (bw == 40 && (CHSPEC_CTL_SB(c) != ctrlsb))
#endif
			continue;
		/* create the actual control channel number from sideband */
		int chan = CHSPEC_CHANNEL(c);
		if (bw == 40)
			chan += ((ctrlsb == WL_CHANSPEC_CTL_SB_UPPER) ? 2 : -2);
#ifdef CONFIG_BCMWL6
		else if (bw == 80)
			chan += ((ctrlsb == WL_CHANSPEC_CTL_SB_UPPER) ? -2 : -6 ); /* upper is actually LLU */
#endif
		web_print_wlchan(chan, band);
		count++;
	}
#endif /* TCONFIG_BCM714 */

	free((void *)buf);

	return count;
}

static void _wlchannels(char *ifname, char *country, int band)
{
	unsigned int i;
	wl_channels_in_country_t *cic;

	cic = (wl_channels_in_country_t *)malloc(WLC_IOCTL_MAXLEN);
	if (cic) {
		cic->buflen = WLC_IOCTL_MAXLEN;
		strcpy(cic->country_abbrev, country);
		cic->band = band;

		if (wl_ioctl(ifname, WLC_GET_CHANNELS_IN_COUNTRY, cic, cic->buflen) == 0) {
			for (i = 0; i < cic->count; i++) {
				web_print_wlchan(cic->channel[i], band);
			}
		}
		free((void *)cic);
	}
}

void asp_wlchannels(int argc, char **argv)
{
	char buf[WLC_CNTRY_BUF_SZ];
	int band, phytype, nphy;
	int bw, ctrlsb, chanspec;
	char *ifname;

	/* args: unit, nphy[1|0], bw, band, ctrlsb */

	check_wl_unit(argc > 0 ? argv[0] : NULL);

	ifname = nvram_safe_get(wl_nvname("ifname", unit, 0));
	wl_ioctl(ifname, WLC_GET_COUNTRY, buf, sizeof(buf));
	if (wl_ioctl(ifname, WLC_GET_BAND, &band, sizeof(band)) != 0)
		band = nvram_get_int(wl_nvname("nband", unit, 0));

	wl_iovar_getint(ifname, "chanspec", &chanspec);

	if (argc > 1)
		nphy = atoi(argv[1]);
	else {
		wl_ioctl(ifname, WLC_GET_PHYTYPE, &phytype, sizeof(phytype));
		nphy = wl_phytype_n(phytype);
	}

	bw = (argc > 2) ? atoi(argv[2]) : 0;
#ifdef CONFIG_BCMWL6
	bw = bw ? : CHSPEC_IS80(chanspec) ? 80 : (CHSPEC_IS40(chanspec) ? 40 : 20);
#else
	bw = bw ? : CHSPEC_IS40(chanspec) ? 40 : 20;
#endif
	if (argc > 3) band = atoi(argv[3]) ? : band;

	if (argc > 4) {
		if (strcmp(argv[4], "upper") == 0)
			ctrlsb = WL_CHANSPEC_CTL_SB_UPPER;
		else
			ctrlsb = WL_CHANSPEC_CTL_SB_LOWER;
	}
	else
		ctrlsb = CHSPEC_CTL_SB(chanspec);

	web_puts("\nwl_channels = [\n[0, 0]");
	if (nphy) {
		if (!_wlchanspecs(ifname, buf, band, bw, ctrlsb) && band == WLC_BAND_2G && bw == 40)
			_wlchanspecs(ifname, buf, band, 20, ctrlsb);
	}
	else
		_wlchannels(ifname, buf, band);

	web_puts("];\n");
}

static int print_wlbands(int idx, int unit, int subunit, void *param)
{
	char *phytype, *phylist, *ifname;
	char comma = ' ';
	int list[WLC_BAND_ALL];
	unsigned int i;

	ifname = nvram_safe_get(wl_nvname("ifname", unit, 0));
	phytype = nvram_safe_get(wl_nvname("phytype", unit, 0));

	web_printf("%c[", (idx == 0) ? ' ' : ',');

	if ((phytype[0] == 'n') ||
	    (phytype[0] == 'l') ||
	    (phytype[0] == 's') ||
	    (phytype[0] == 'c') ||
#if defined(CONFIG_BCMWL6) || defined(TCONFIG_BLINK)
	    (phytype[0] == 'v') ||
#endif
	    (phytype[0] == 'h'))
	{
		/* get band list, assume both the bands in case of error */
		if (wl_ioctl(ifname, WLC_GET_BANDLIST, list, sizeof(list)) < 0) {
			for (i = WLC_BAND_5G; i <= WLC_BAND_2G; i++) {
				web_printf("%c'%d'", comma, i);
				comma = ',';
			}
		}
		else {
			if (list[0] > 2)
				list[0] = 2;

#if !defined(TCONFIG_BCMWL6) && defined(TCONFIG_BLINK) /* only RT-N */
			if (list[0] == 2) { /* two bands for wl interface ethX possible (2,4 GHz & 5 GHz) */
				/* get router model */
				int model = get_model();

				if ((model == MODEL_E4200) ||
				    (model == MODEL_F9K1102)) {
					list[0] = 1; /* allow (or use) only the first band from the list! (see GUI: basic-network) */
				}
			}
#endif

			for (i = 1; i <= (unsigned int) list[0]; i++) {
				web_printf("%c'%d'", comma, list[i]);
				comma = ',';
			}
		}
	}
	else {
		/* get available phy types of the currently selected wireless interface */
		phylist = nvram_safe_get(wl_nvname("phytypes", unit, 0));
		for (i = 0; i < strlen(phylist); i++) {
			web_printf("%c'%d'", comma, phylist[i] == 'a' ? WLC_BAND_5G : WLC_BAND_2G);
			comma = ',';
		}
	}

	web_puts("]");

	return 0;
}

void asp_wlbands(int argc, char **argv)
{
	int include_vifs = (argc > 0) ? atoi(argv[0]) : 0;

	web_puts("\nwl_bands = [");
	foreach_wif(include_vifs, NULL, print_wlbands);
	web_puts(" ];\n");
}

static int print_wif(int idx, int unit, int subunit, void *param)
{
	struct ifreq ifr;
	struct ether_addr bssid;
	char unit_str[] = "000000";
	char *ifname, *ssid, *next;
	char cap[WLC_IOCTL_SMLEN];
	char caps[WLC_IOCTL_SMLEN];
	int sfd, max_no_vifs = 0;
	unsigned int up = 0;

	if (subunit > 0)
		snprintf(unit_str, sizeof(unit_str), "%d.%d", unit, subunit);
	else {
		snprintf(unit_str, sizeof(unit_str), "%d", unit);

		max_no_vifs = 1;
		wl_iovar_get(nvram_safe_get(wl_nvname("ifname", unit, 0)), "cap", (void *)caps, WLC_IOCTL_SMLEN);
		foreach(cap, caps, next) {
			if (!strcmp(cap, "mbss16"))
				max_no_vifs = 16;
			if (!strcmp(cap, "mbss8"))
				max_no_vifs = 8;
			if (!strcmp(cap, "mbss4"))
				max_no_vifs = 4;
		}
	}

	if ((sfd = socket(AF_INET, SOCK_RAW, IPPROTO_RAW)) < 0)
		logmsg(LOG_ERR, "[%s %d]: error opening socket %m\n", __FUNCTION__, __LINE__);

	if (sfd >= 0) {
		strcpy(ifr.ifr_name, nvram_safe_get(wl_nvname("ifname", unit, subunit)));
		if (ioctl(sfd, SIOCGIFFLAGS, &ifr) == 0)
			if (ifr.ifr_flags & (IFF_UP | IFF_RUNNING))
				up = 1;
		close(sfd);
	}

	ifname = nvram_safe_get(wl_nvname("ifname", unit, subunit));
	wl_ioctl(ifname, WLC_GET_BSSID, &bssid, ETHER_ADDR_LEN);

	/* [ifname, unitstr, unit, subunit, ssid, hwaddr, up, max_no_vifs, wmode, bssid] */
	ssid = utf8_to_js_string(nvram_safe_get(wl_nvname("ssid", unit, subunit)));

	web_printf("%c['%s','%s',%d,%d,'%s','%s',%d,%d,'%s','%02X:%02X:%02X:%02X:%02X:%02X']",
	           (idx == 0 ? ' ' : ','),
	           nvram_safe_get(wl_nvname("ifname", unit, subunit)), unit_str, unit, subunit, ssid, nvram_safe_get(wl_nvname("hwaddr", unit, subunit)), up, max_no_vifs,
	           nvram_safe_get(wl_nvname("mode", unit, subunit)), bssid.octet[0], bssid.octet[1], bssid.octet[2], bssid.octet[3], bssid.octet[4], bssid.octet[5]
	);

	free(ssid);

	return 0;
}

void asp_wlifaces(int argc, char **argv)
{
	int include_vifs = (argc > 0) ? atoi(argv[0]) : 0;

	web_puts("\nwl_ifaces = [");
	foreach_wif(include_vifs, NULL, print_wif);
	web_puts("];\n");
}

static cntry_name_t *wlc_cntry_abbrev_to_country(const char *abbrev)
{
	cntry_name_t *cntry;
	if ((!*abbrev) || (strlen(abbrev) > 3) || (strlen(abbrev) < 2))
		return (NULL);

	for (cntry = cntry_names; cntry->name &&
		strncmp(abbrev, cntry->abbrev, strlen(abbrev)); cntry++);

	return (!cntry->name ? NULL : cntry);
}

void asp_wlcountries(int argc, char **argv)
{
	unsigned int i;
	wl_country_list_t *cl = (wl_country_list_t *)malloc(WLC_IOCTL_MAXLEN + WLC_IOCTL_MAXLEN_ADDON);
	cntry_name_t *cntry = NULL;
	char *abbrev = NULL;
	char *ifname = (strcmp(nvram_safe_get("wl_ifname"),"") != 0) ? nvram_safe_get("wl_ifname") : "eth1"; /* keep it easy - take the first interface */
	int band = WLC_BAND_ALL; /* all bands == 3 */

	web_puts("\nwl_countries = [");

	if (cl) {
		cl->buflen = WLC_IOCTL_MAXLEN + WLC_IOCTL_MAXLEN_ADDON;
		cl->band_set = FALSE;
		cl->band = band;

		if (wl_ioctl(ifname, WLC_GET_COUNTRY_LIST, cl, cl->buflen) == 0) {
			for (i = 0; i < cl->count; i++) {
				abbrev = &cl->country_abbrev[i*WLC_CNTRY_BUF_SZ];
				cntry = wlc_cntry_abbrev_to_country(abbrev);
				web_printf("%c['%s','%s']", (i ? ',' : ' '), abbrev, (cntry ? cntry->name : abbrev));
			}

		}
		free((void *)cl);
	}
	else { /* something went wrong ... provide a basic list at least */
#ifdef TCONFIG_BCMARM
		web_puts(" ['#a', '#a (wildcard)'],['EU', 'EUROPE'],['CZ', 'CZECH REPUBLIC'],['DE', 'GERMANY'],['US', 'UNITED STATES'],['SE', 'SWEDEN'],['SG', 'SINGAPORE'],['LU', 'LUXEMBOURG']");
#else
		web_puts(" ['BR', 'BRAZIL'],['EU', 'EUROPE'],['CZ', 'CZECH REPUBLIC'],['DE', 'GERMANY'],['US', 'UNITED STATES'],['SE', 'SWEDEN'],['SG', 'SINGAPORE'],['LU', 'LUXEMBOURG']");
#endif
	}

	web_puts("];\n");
}

#if defined(TCONFIG_BLINK) || defined(TCONFIG_BCMARM) /* RT-N+ */
int mround(float val)
{
	return (int)(val + 0.5);
}

/*
 * Get temperature of wireless chip
 * bwq518. Copyright 2013
*/
char* get_wl_tempsense(char *buf)
{
	char *lan_ifnames;
	char *p;
	char *ifname;
	char s[WLC_IOCTL_SMLEN], tempC[128], tempF[128], band[WLC_IOCTL_SMLEN];
	int b5G, b2G;
	unsigned *cur_temp;
	int  ret = 0, len, i;

	strcpy(buf, "");
	if ((lan_ifnames = strdup(nvram_safe_get("wl_ifnames"))) != NULL) {
		p = lan_ifnames;
		while ((ifname = strsep(&p, " ")) != NULL) {
			while (*ifname == ' ')
				++ifname;

			trimstr(ifname);
			if ((*ifname == 0) || (strncasecmp(ifname, "eth",3) != 0))
				continue;

			bzero(s, sizeof(s));
			bzero(tempC, sizeof(tempC));
			bzero(tempF, sizeof(tempF));
			strcpy(s, "phy_tempsense");
			if ((ret = wl_ioctl(ifname, WLC_GET_VAR, s, sizeof(s))) == 0) {
				cur_temp = (unsigned int*) s;
				snprintf(tempC, sizeof(tempC), "%d", *cur_temp / 2 + 20);

				if ((atoi(tempC) <=0) || (atoi(tempC)>= 120)) {
					strcpy(tempC, "--");
					strcpy(tempF, "--");
				}
				else
					snprintf(tempF, sizeof(tempF), "%d", mround((*cur_temp / 2 + 20) * 1.8 + 32));
			}
			else {
				strcpy(tempC, "--");
				strcpy(tempF, "--");
			}

			/* get band of ifname */
			bzero(s, sizeof(s));
			bzero(band, sizeof(band));
			int bandlist[WLC_BAND_ALL];
			if (wl_ioctl(ifname, WLC_GET_BANDLIST, bandlist, sizeof(bandlist)) == 0) {
				if (bandlist[0] == 0)
					strcpy(band, "--");
				else {
					b5G = 0;
					b2G = 0;
					for (i = 1; i <= bandlist[0]; i++) {
						switch (bandlist[i]) {
							case WLC_BAND_5G:
								b5G = 1;
								break;
							case WLC_BAND_2G:
								b2G = 1;
								break;
							default:
								break;
						}
					}
					if (b5G == 1 && b2G == 1)
						strcpy(band, "2.4G/5G");
					else if (b5G == 0 && b2G == 1)
						strcpy(band, "2.4G");
					else if (b5G == 1 && b2G == 0)
						strcpy(band, "5G");
					else /* b5G == 0 && b2G == 0 */
						strcpy(band, "--");
				}
			}
			else
				strcpy(band,"--");

			if ((strlen(tempC) > 0) && (strlen(band) > 0)) {
				if ((strcmp(tempC, "--") != 0) || (strcmp(band, "--") != 0)) {
					strcat(buf, ifname);
					strcat(buf, ": ");
					strcat(buf, band);
					strcat(buf, " - ");
					strcat(buf, tempC);
					strcat(buf, "&#176;C&nbsp;/&nbsp;");
					strcat(buf, tempF);
					strcat(buf, "&#176;F&nbsp;&nbsp;&nbsp;&nbsp;");
				}
			}
		}
		free(lan_ifnames);
	}
	/* remove spaces from end */
	len = strlen(buf);
	if (len >= 24)
		buf[len - 24] = '\0';

	if (len == 0)
		strcpy(buf, "--");
	else
		trimstr(buf);

	return buf;
}
#endif /* TCONFIG_BLINK || TCONFIG_BCMARM */
