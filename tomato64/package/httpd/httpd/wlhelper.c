/*
 * wlhelper.c - WiFi helper functions for TOMATO64
 *
 * Tomato Firmware
 * Copyright (C) 2025
 *
 * This file contains helper functions for working with WiFi interfaces
 * in TOMATO64, replacing shell script implementations with native C code.
 */

#ifdef TOMATO64

#include "tomato.h"
#include "wlhelper.h"

#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include <sys/stat.h>
#include <net/if.h>

/* Buffer sizes */
#define NVRAM_KEY_SIZE 64
#define CMD_BUFFER_SIZE 256
#define LINE_BUFFER_SIZE 512

/*
 * Execute iwinfo command and get output
 */
int wlhelper_exec_iwinfo(const char *ifname, const char *command,
                         char *output, size_t output_size)
{
	FILE *fp;
	char cmd[CMD_BUFFER_SIZE];

	if (!ifname || !command || !output || output_size == 0)
		return -1;

	/* Build command */
	snprintf(cmd, sizeof(cmd), "iwinfo %s %s 2>/dev/null", ifname, command);

	fp = popen(cmd, "r");
	if (!fp)
		return -1;

	/* Read output */
	size_t total_read = 0;
	while (total_read < output_size - 1 &&
	       fgets(output + total_read, output_size - total_read, fp) != NULL) {
		total_read = strlen(output);
	}

	pclose(fp);
	return 0;
}

/*
 * Get noise level for a given interface using iwinfo
 */
int wlhelper_get_noise_level(const char *ifname)
{
	FILE *fp;
	char cmd[CMD_BUFFER_SIZE];
	char line[LINE_BUFFER_SIZE];
	int noise = -99;

	if (!ifname)
		return -99;

	/* Execute: iwinfo <ifname> info (parse output in C, no grep) */
	snprintf(cmd, sizeof(cmd), "iwinfo %s info 2>/dev/null", ifname);
	fp = popen(cmd, "r");
	if (!fp)
		return -99;

	/* Read through output looking for Noise line */
	while (fgets(line, sizeof(line), fp) != NULL) {
		/* Parse line like: "Noise: -95 dBm" or "Noise: unknown" */
		char *p = strstr(line, "Noise:");
		if (p) {
			p += 6; /* Skip "Noise:" */

			/* Skip whitespace */
			while (*p == ' ' || *p == '\t')
				p++;

			/* Check if it's "unknown" */
			if (strncmp(p, "unknown", 7) == 0) {
				noise = -99;
			}
			else {
				/* Parse the number */
				noise = atoi(p);

				/* Sanity check: noise should be negative and reasonable */
				if (noise > 0 || noise < -120)
					noise = -99;
			}
			break; /* Found it, stop reading */
		}
	}

	pclose(fp);
	return noise;
}

/*
 * Get interface name for a given PHY and interface index
 */
void wlhelper_get_ifname(int phy, int iface, char *ifname, size_t size)
{
	char nvram_key[NVRAM_KEY_SIZE];
	char *val;

	if (!ifname || size == 0)
		return;

	/* Try to get custom interface name from nvram */
	snprintf(nvram_key, sizeof(nvram_key), "wifi_phy%diface%d_ifname", phy, iface);
	val = nvram_get(nvram_key);

	if (val && val[0] != '\0') {
		/* Use custom name */
		snprintf(ifname, size, "%s", val);
	}
	else {
		/*
		 * Default naming, matching the uci interface names start_wifi.sh
		 * writes: phy{phy}-ap{iface}, phy{phy}-sta{iface} or
		 * phy{phy}-mesh{iface} depending on the configured mode.
		 */
		const char *mode = wlhelper_get_iface_mode(phy, iface);
		const char *kind = "ap";

		if (mode) {
			if (strcmp(mode, "sta") == 0 || strcmp(mode, "bridge") == 0)
				kind = "sta";
			else if (strcmp(mode, "mesh") == 0)
				kind = "mesh";
		}

		snprintf(ifname, size, "phy%d-%s%d", phy, kind, iface);
	}
}

/*
 * Count number of WiFi PHYs by checking nvram
 */
int wlhelper_count_phys(void)
{
	int count = 0;

	/* First, try to get the cached PHY count set by wlconfig */
	count = nvram_get_int("wifi_phy_count");
	if (count > 0)
		return count;

	/*
	 * Fallback: Use expected PHY count from nvram
	 * This is a constant set per-device in libshared/shared/defaults.c
	 */
	return nvram_get_int("wifi_phy_count_expected");
}

/*
 * Check if an interface is enabled
 */
int wlhelper_is_iface_enabled(int phy, int iface)
{
	char nvram_key[NVRAM_KEY_SIZE];

	snprintf(nvram_key, sizeof(nvram_key), "wifi_phy%diface%d_enable", phy, iface);
	return (nvram_get_int(nvram_key) == 1);
}

/*
 * Get the mode of an interface (ap, sta, etc.)
 */
const char* wlhelper_get_iface_mode(int phy, int iface)
{
	char nvram_key[NVRAM_KEY_SIZE];

	snprintf(nvram_key, sizeof(nvram_key), "wifi_phy%diface%d_mode", phy, iface);
	return nvram_get(nvram_key);
}

/*
 * Get number of interfaces for a given PHY
 */
int wlhelper_get_iface_count(int phy)
{
	char nvram_key[NVRAM_KEY_SIZE];

	snprintf(nvram_key, sizeof(nvram_key), "wifi_phy%d_ifaces", phy);
	return nvram_get_int(nvram_key);
}

/*
 * Get a specific field from iwinfo info output
 */
int wlhelper_get_iwinfo_field(const char *ifname, const char *field_name,
                               char *value, size_t value_size)
{
	FILE *fp;
	char cmd[CMD_BUFFER_SIZE];
	char line[LINE_BUFFER_SIZE];
	size_t field_len;

	if (!ifname || !field_name || !value || value_size == 0)
		return -1;

	field_len = strlen(field_name);
	value[0] = '\0';

	/* Execute: iwinfo <ifname> info */
	snprintf(cmd, sizeof(cmd), "iwinfo %s info 2>/dev/null", ifname);
	fp = popen(cmd, "r");
	if (!fp)
		return -1;

	/* Search for the field in output */
	while (fgets(line, sizeof(line), fp) != NULL) {
		char *field_start = strstr(line, field_name);
		if (field_start) {
			char *value_start = field_start + field_len;

			/* Skip whitespace after field name */
			while (*value_start == ' ' || *value_start == '\t')
				value_start++;

			/* Copy value, removing trailing whitespace/newline */
			size_t len = 0;
			while (*value_start && *value_start != '\n' && *value_start != '\r' &&
			       len < value_size - 1) {
				value[len++] = *value_start++;
			}
			value[len] = '\0';

			/* Trim trailing whitespace */
			while (len > 0 && (value[len-1] == ' ' || value[len-1] == '\t')) {
				value[--len] = '\0';
			}

			pclose(fp);
			return (len > 0) ? 0 : -1;
		}
	}

	pclose(fp);
	return -1; /* Field not found */
}

/*
 * Get MAC address for an interface from sysfs
 */
int wlhelper_get_mac_address(const char *ifname, char *mac, size_t mac_size)
{
	char path[256];
	FILE *fp;

	if (!ifname || !mac || mac_size < 18)
		return -1;

	/* Read from /sys/class/net/<ifname>/address */
	snprintf(path, sizeof(path), "/sys/class/net/%s/address", ifname);
	fp = fopen(path, "r");
	if (!fp)
		return -1;

	/* Read MAC address */
	if (fgets(mac, mac_size, fp) == NULL) {
		fclose(fp);
		return -1;
	}

	/* Remove trailing newline */
	size_t len = strlen(mac);
	if (len > 0 && mac[len-1] == '\n')
		mac[len-1] = '\0';

	fclose(fp);
	return 0;
}

/*
 * Check if a network interface exists
 */
int wlhelper_iface_exists(const char *ifname)
{
	char path[256];

	if (!ifname)
		return 0;

	/* Check if /sys/class/net/<ifname>/address exists */
	snprintf(path, sizeof(path), "/sys/class/net/%s/address", ifname);
	return (access(path, F_OK) == 0) ? 1 : 0;
}

/*
 * Get channel statistics from iwinfo
 */
int wlhelper_get_channel_stats(const char *ifname, int *channel, int *mhz,
                                 int *nbw, int *noise, float *rate,
                                 int *center, char *proto, size_t proto_size)
{
	char cmd[CMD_BUFFER_SIZE];
	char line[LINE_BUFFER_SIZE];
	FILE *fp;
	int found_master = 0, found_noise = 0, found_rate = 0;

	if (!ifname || !channel || !mhz || !nbw || !noise || !rate)
		return -1;

	/* Initialize defaults */
	*channel = 0;
	*mhz = 0;
	*nbw = 0;
	*noise = -99;
	*rate = 0.0;
	if (center)
		*center = 0;
	if (proto && proto_size)
		proto[0] = '\0';

	/* Execute iwinfo info command */
	snprintf(cmd, sizeof(cmd), "iwinfo %s info 2>/dev/null", ifname);
	fp = popen(cmd, "r");
	if (!fp)
		return -1;

	/* Parse output line by line */
	while (fgets(line, sizeof(line), fp) != NULL) {
		/* Parse Master line for channel, mhz, and nbw */
		/* Format: "Mode: Master  Channel: 1 (2.412 GHz)  HT Mode: HE40" */
		/* Note: Must check for "Master" first to avoid collision with "Center Channel" line */
		if (!found_master && strstr(line, "Master") != NULL) {
			char *channel_ptr = strstr(line, "Channel:");
			char *ht_mode_ptr = strstr(line, "HT Mode:");

			/* Ensure this is the Master line, not "Center Channel" line */
			/* Parse channel and frequency */
			if (channel_ptr && strstr(line, "Center Channel") == NULL) {
				char freq_str[32];
				/* Extract channel number and frequency: "Channel: 1 (2.412" */
				if (sscanf(channel_ptr, "Channel: %d %s", channel, freq_str) == 2) {
					/* Parse frequency like "(2.412" or "(5.180" */
					/* Shell script does: ${mhz:1:1}${mhz:3} which extracts "2" + "412" = "2412" */
					if (strlen(freq_str) >= 5 && freq_str[0] == '(' && freq_str[2] == '.') {
						char mhz_str[16];
						/* Extract char at position 1, then concatenate from position 3 onward */
						snprintf(mhz_str, sizeof(mhz_str), "%c%s", freq_str[1], &freq_str[3]);
						*mhz = atoi(mhz_str);
					}
				}
			}

			/* Parse HT Mode (bandwidth) */
			if (ht_mode_ptr) {
				char bw_str[32];
				/* Extract bandwidth: "HT Mode: HE40" or "HT Mode: NOHT" */
				if (sscanf(ht_mode_ptr, "HT Mode: %s", bw_str) == 1) {
					/*
					 * The prefix of the same field names the generation
					 * the radio is actually running: EHT160, HE80, VHT80,
					 * HT40 or NOHT.
					 */
					if (proto && proto_size) {
						if (strncmp(bw_str, "EHT", 3) == 0)
							snprintf(proto, proto_size, "%s", "11be");
						else if (strncmp(bw_str, "VHT", 3) == 0)
							snprintf(proto, proto_size, "%s", "11ac");
						else if (strncmp(bw_str, "HE", 2) == 0)
							snprintf(proto, proto_size, "%s", "11ax");
						else if (strncmp(bw_str, "HT", 2) == 0)
							snprintf(proto, proto_size, "%s", "11n");
					}

					if (strcmp(bw_str, "NOHT") == 0) {
						*nbw = 0;
					} else {
						/* Extract last 2 or 3 chars for bandwidth (e.g., "HE40" -> 40, "HE160" -> 160) */
						size_t len = strlen(bw_str);
						if (len >= 2) {
							/* Check if last 3 chars are digits (for 160) */
							if (len >= 3 && isdigit(bw_str[len-3]))
								*nbw = atoi(&bw_str[len - 3]);
							else
								*nbw = atoi(&bw_str[len - 2]);
						}
					}
				}
			}

			/* Mark as found if we got at least the channel */
			if (channel_ptr)
				found_master = 1;
		}

		/* Parse the centre channel, only printed when the driver reports one */
		if (center && !*center) {
			char *center_ptr = strstr(line, "Center Channel 1:");

			if (center_ptr)
				sscanf(center_ptr, "Center Channel 1: %d", center);
		}

		/* Parse Noise line (format: "Signal: ... Noise: -92 dBm" or "Signal: unknown  Noise: -92 dBm") */
		if (!found_noise && strstr(line, "Noise:") != NULL) {
			char *noise_ptr = strstr(line, "Noise:");
			if (noise_ptr) {
				char noise_str[32];
				/* Skip past "Noise:" and extract the value */
				if (sscanf(noise_ptr, "Noise: %s", noise_str) == 1) {
					if (strcmp(noise_str, "unknown") == 0) {
						*noise = -99;
					} else {
						*noise = atoi(noise_str);
					}
					found_noise = 1;
				}
			}
		}

		/* Parse Bit Rate line (format: "Bit Rate: unknown" or "Bit Rate: 2401.9 MBit/s") */
		if (!found_rate && strstr(line, "Bit Rate:") != NULL) {
			char *rate_ptr = strstr(line, "Bit Rate:");
			if (rate_ptr) {
				char rate_str[32];
				/* Skip past "Bit Rate:" and extract the value */
				if (sscanf(rate_ptr, "Bit Rate: %s", rate_str) == 1) {
					if (strcmp(rate_str, "unknown") == 0) {
						*rate = 0.0;
					} else {
						*rate = atof(rate_str);
					}
					found_rate = 1;
				}
			}
		}

		/* Stop if we found all fields */
		if (found_master && found_noise && found_rate && (!center || *center))
			break;
	}

	pclose(fp);

	if (proto && proto_size && !proto[0])
		snprintf(proto, proto_size, "%s", (*mhz && (*mhz < 2500)) ? "11g" : "11a");

	return (found_master) ? 0 : -1;
}

/*
 * Iterate through all connected stations on an interface
 */
int wlhelper_foreach_station(const char *ifname, int phy,
                               wlhelper_station_callback callback,
                               void *user_data)
{
	char cmd[CMD_BUFFER_SIZE];
	char line[LINE_BUFFER_SIZE];
	FILE *fp;
	int station_count = 0;
	struct wlhelper_station_info current_station;
	int field_index = 0;
	int has_station = 0;

	if (!ifname || !callback)
		return -1;

	memset(&current_station, 0, sizeof(current_station));

	/* Execute: iw <ifname> station dump */
	snprintf(cmd, sizeof(cmd), "iw %s station dump 2>/dev/null", ifname);
	fp = popen(cmd, "r");
	if (!fp)
		return -1;

	/* Parse output line by line */
	/* Looking for: Station, signal:, tx bitrate:, rx bitrate:, connected time: */
	while (fgets(line, sizeof(line), fp) != NULL) {
		/* Look for "Station" line (new client) */
		if (strstr(line, "Station ") != NULL) {
			/* If we already have a complete station, process it */
			if (has_station && field_index == 5) {
				if (callback(ifname, phy, &current_station, user_data) != 0) {
					pclose(fp);
					return station_count;
				}
				station_count++;
			}

			/* Start new station */
			memset(&current_station, 0, sizeof(current_station));
			field_index = 0;
			has_station = 1;

			/* Extract MAC address: "Station AA:BB:CC:DD:EE:FF (on phyX-apY)" */
			char mac_lower[18];
			if (sscanf(line, "Station %17s", mac_lower) == 1) {
				/* Convert to uppercase */
				for (int i = 0; mac_lower[i] && i < sizeof(current_station.mac) - 1; i++) {
					current_station.mac[i] = toupper(mac_lower[i]);
				}
				current_station.mac[17] = '\0';
				field_index++;
			}
			continue;
		}

		if (!has_station)
			continue;

		/* Look for "signal:" line - use label-first approach */
		if (field_index == 1 && strstr(line, "signal:") != NULL) {
			char *signal_ptr = strstr(line, "signal:");
			if (signal_ptr) {
				char signal_str[32];
				/* Extract signal value */
				if (sscanf(signal_ptr, "signal: %s", signal_str) == 1) {
					current_station.signal = atoi(signal_str);
					field_index++;
				}
			}
			continue;
		}

		/* Look for "tx bitrate:" line - use label-first approach */
		if (field_index == 2 && strstr(line, "tx bitrate:") != NULL) {
			char *tx_ptr = strstr(line, "tx bitrate:");
			if (tx_ptr) {
				float tx_rate;
				/* Extract tx bitrate value */
				if (sscanf(tx_ptr, "tx bitrate: %f", &tx_rate) == 1) {
					current_station.tx_bitrate = (int)(tx_rate * 1000);
					field_index++;
				}
			}
			continue;
		}

		/* Look for "rx bitrate:" line - use label-first approach */
		if (field_index == 3 && strstr(line, "rx bitrate:") != NULL) {
			char *rx_ptr = strstr(line, "rx bitrate:");
			if (rx_ptr) {
				float rx_rate;
				/* Extract rx bitrate value */
				if (sscanf(rx_ptr, "rx bitrate: %f", &rx_rate) == 1) {
					current_station.rx_bitrate = (int)(rx_rate * 1000);
					field_index++;
				}
			}
			continue;
		}

		/* Look for "connected time:" line - use label-first approach */
		if (field_index == 4 && strstr(line, "connected time:") != NULL) {
			char *time_ptr = strstr(line, "connected time:");
			if (time_ptr) {
				int connected_sec;
				/* Extract connected time value */
				if (sscanf(time_ptr, "connected time: %d", &connected_sec) == 1) {
					current_station.connected_time = connected_sec;
					field_index++;
				}
			}
			continue;
		}
	}

	/* Process last station if we have one */
	if (has_station && field_index == 5) {
		callback(ifname, phy, &current_station, user_data);
		station_count++;
	}

	pclose(fp);
	return station_count;
}

/*
 * Iterate through WiFi interfaces with optional filtering
 */
int wlhelper_foreach_interface(int filter_flags, wlhelper_iface_callback callback, void *user_data)
{
	int phycount;
	int processed_count = 0;

	if (!callback)
		return -1;

	/* Get number of PHYs */
	phycount = wlhelper_count_phys();
	if (phycount <= 0)
		return 0;

	/* Iterate through all PHYs */
	for (int phy = 0; phy < phycount; phy++) {
		int iface_count = wlhelper_get_iface_count(phy);

		/* Iterate through all interfaces for this PHY */
		for (int iface = 0; iface < iface_count && iface < 16; iface++) {
			char ifname[64];

			/* Apply ENABLED filter */
			if ((filter_flags & WLHELPER_FILTER_ENABLED) &&
			    !wlhelper_is_iface_enabled(phy, iface)) {
				continue;
			}

			/* Apply mode filters */
			if (filter_flags & (WLHELPER_FILTER_AP_MODE | WLHELPER_FILTER_STA_MODE)) {
				const char *mode = wlhelper_get_iface_mode(phy, iface);

				if (!mode) {
					continue;
				}

				/* Check AP mode filter */
				if ((filter_flags & WLHELPER_FILTER_AP_MODE) &&
				    strcmp(mode, "ap") != 0) {
					continue;
				}

				/* Check STA mode filter */
				if ((filter_flags & WLHELPER_FILTER_STA_MODE) &&
				    strcmp(mode, "sta") != 0) {
					continue;
				}
			}

			/* Get interface name */
			wlhelper_get_ifname(phy, iface, ifname, sizeof(ifname));

			/* Call callback */
			if (callback(phy, iface, ifname, user_data) != 0) {
				/* Callback requested stop */
				return processed_count;
			}

			processed_count++;
		}
	}

	return processed_count;
}

/*
 * Site survey support
 *
 * Everything below parses the output of 'iwinfo <ifname> scan', which looks
 * like this (field order and the two-space separators are fixed by
 * iwinfo_cli.c):
 *
 *	Cell 01 - Address: AA:BB:CC:DD:EE:FF
 *	          ESSID: "example"
 *	          Mode: Master  Frequency: 2.437 GHz  Band: 2.4 GHz  Channel: 6
 *	          Signal: -55 dBm  Quality: 55/70
 *	          Encryption: WPA2 PSK (CCMP)
 *	          HT Operation:
 *	                    Primary Channel: 6
 *	                    Secondary Channel Offset: below
 *	                    Channel Width: 40 MHz or higher
 *	          VHT Operation:
 *	                    Center Frequency 1: 0
 *	                    Center Frequency 2: 0
 *	                    Channel Width: 20 or 40 MHz
 *
 * The HT/VHT/HE/EHT blocks are only printed when the BSS advertises them, so
 * their presence doubles as the 802.11 generation.
 */

/* Which operation block the indented lines currently belong to */
#define SCAN_SEC_NONE 0
#define SCAN_SEC_HT   1
#define SCAN_SEC_VHT  2
#define SCAN_SEC_HE   3
#define SCAN_SEC_EHT  4

struct scan_parse_state {
	struct wlhelper_scan_entry e;
	int have_cell;      /* a Cell header has been seen, entry is being filled */
	int section;        /* SCAN_SEC_* */
	int ht_primary;     /* HT Operation / Primary Channel */
	int ht_offset;      /* +1 above, -1 below, 0 none */
	int ht_width;       /* resolved width of each block, 0 when absent */
	int vht_width;
	int he_width;
	int eht_width;
	int vht_center;     /* Center Frequency 1 of each block, 0 when absent */
	int he_center;
	int eht_center;
};

/*
 * Skip leading blanks
 */
static char *scan_skip_ws(char *p)
{
	while (*p == ' ' || *p == '\t')
		p++;

	return p;
}

/*
 * Trim trailing newline and blanks in place
 */
static void scan_rtrim(char *p)
{
	size_t n = strlen(p);

	while (n > 0 && (p[n - 1] == '\n' || p[n - 1] == '\r' ||
	                 p[n - 1] == ' ' || p[n - 1] == '\t'))
		p[--n] = '\0';
}

/*
 * Return the text following a "<label>" prefix, or NULL when the line does not
 * start with that label
 */
static char *scan_field(char *line, const char *label)
{
	size_t n = strlen(label);

	if (strncmp(line, label, n) != 0)
		return NULL;

	return scan_skip_ws(line + n);
}

/*
 * Copy the value of an inline "<label>" field up to the two-space separator
 * that iwinfo puts between fields on the same line
 */
static void scan_inline_field(const char *line, const char *label, char *out, size_t size)
{
	const char *p, *end;
	size_t n;

	out[0] = '\0';

	if ((p = strstr(line, label)) == NULL)
		return;

	p = scan_skip_ws((char *)p + strlen(label));

	for (end = p; *end != '\0' && *end != '\n'; end++)
		if (end[0] == ' ' && end[1] == ' ')
			break;

	n = end - p;
	if (n >= size)
		n = size - 1;

	memcpy(out, p, n);
	out[n] = '\0';
}

/*
 * Resolve one "Channel Width:" string
 *
 * HT prints "20 MHz" or "40 MHz or higher"; VHT prints "20 or 40 MHz" (meaning
 * "look at HT"), "80 MHz", "80+80 MHz" or "160 MHz"; HE and EHT print a plain
 * "<n> MHz" up to 320. Returns 0 when the width is deferred or unknown.
 */
static int scan_parse_width(int section, const char *s)
{
	if (section == SCAN_SEC_HT)
		return (strncmp(s, "20 MHz", 6) == 0) ? 20 : (strncmp(s, "40 MHz", 6) == 0) ? 40 : 0;

	if (section == SCAN_SEC_VHT) {
		if (strncmp(s, "80+80 MHz", 9) == 0)
			return 80;
		if (strncmp(s, "80 MHz", 6) == 0)
			return 80;
		if (strncmp(s, "160 MHz", 7) == 0)
			return 160;

		return 0; /* "20 or 40 MHz" defers to the HT block */
	}

	/* HE and EHT are printed unambiguously */
	return atoi(s);
}

/*
 * Map a band name to the string the web UI expects
 */
static const char *scan_band_name(const char *band, int mhz)
{
	if (strncmp(band, "2.4", 3) == 0)
		return "2.4";
	if (strncmp(band, "6 ", 2) == 0)
		return "6";
	if (strncmp(band, "5 ", 2) == 0)
		return "5";

	/* "unknown", or a band iwinfo grew after this was written: use the frequency */
	if (mhz >= 2400 && mhz < 2500)
		return "2.4";
	if (mhz >= 5925 && mhz < 7200)
		return "6";

	return "5";
}

/*
 * Split an iwinfo encryption string into a security mode and a cipher
 *
 * "none"                             -> "NONE" / "NONE"
 * "WPA2 PSK (CCMP)"                  -> "WPA2-Personal" / "AES"
 * "mixed WPA/WPA2 PSK (TKIP, CCMP)"  -> "WPA/WPA2-Personal" / "TKIP+AES"
 * "WPA3 SAE (CCMP)"                  -> "WPA3-Personal" / "AES"
 * "WEP Open System (WEP-40)"         -> "WEP" / "WEP-40"
 */
static void scan_parse_encryption(const char *enc, char *sec, size_t sec_size,
                                  char *cipher, size_t cipher_size)
{
	char mode[80];
	const char *open_paren, *close_paren;
	size_t n;

	snprintf(sec, sec_size, "%s", "NONE");
	snprintf(cipher, cipher_size, "%s", "NONE");

	if (!enc || !enc[0] || strcmp(enc, "none") == 0 || strcmp(enc, "unknown") == 0)
		return;

	/* Everything up to " (" is the mode, the parenthesised tail is the cipher list */
	open_paren = strstr(enc, " (");
	if (open_paren) {
		n = open_paren - enc;
		if (n >= sizeof(mode))
			n = sizeof(mode) - 1;

		memcpy(mode, enc, n);
		mode[n] = '\0';

		open_paren += 2;
		close_paren = strrchr(open_paren, ')');
		n = close_paren ? (size_t)(close_paren - open_paren) : strlen(open_paren);

		/* Prefer the names Tomato has always shown for the common ciphers */
		if (strstr(open_paren, "TKIP") && strstr(open_paren, "CCMP"))
			snprintf(cipher, cipher_size, "%s", "TKIP+AES");
		else if (strstr(open_paren, "CCMP-256"))
			snprintf(cipher, cipher_size, "%s", "AES-256");
		else if (strstr(open_paren, "CCMP"))
			snprintf(cipher, cipher_size, "%s", "AES");
		else
			snprintf(cipher, cipher_size, "%.*s", (int)n, open_paren);
	}
	else {
		snprintf(mode, sizeof(mode), "%s", enc);
	}

	if (strncmp(mode, "WEP", 3) == 0) {
		snprintf(sec, sec_size, "%s", "WEP");
		return;
	}

	/*
	 * iwinfo prints "[mixed ]WPA[/WPA2[/WPA3]] <suites>". Turn the key
	 * management suites into the Personal/Enterprise wording the rest of the
	 * GUI uses, and drop the redundant "mixed " prefix.
	 */
	{
		const char *v = mode;
		const char *suite;
		char versions[32];

		if (strncmp(v, "mixed ", 6) == 0)
			v += 6;

		suite = strchr(v, ' ');
		if (!suite) {
			snprintf(sec, sec_size, "%s", v);
			return;
		}

		n = suite - v;
		if (n >= sizeof(versions))
			n = sizeof(versions) - 1;

		memcpy(versions, v, n);
		versions[n] = '\0';
		suite = scan_skip_ws((char *)suite);

		if (strstr(suite, "802.1X"))
			snprintf(sec, sec_size, "%s-Enterprise", versions);
		else if (strstr(suite, "OWE"))
			snprintf(sec, sec_size, "%s", "OWE");
		else if (strstr(suite, "PSK") || strstr(suite, "SAE"))
			snprintf(sec, sec_size, "%s-Personal", versions);
		else
			snprintf(sec, sec_size, "%s %s", versions, suite);
	}
}

/*
 * Finish the entry currently being parsed and hand it to the callback
 */
static int scan_flush(struct scan_parse_state *st, wlhelper_scan_callback callback, void *user_data)
{
	struct wlhelper_scan_entry *e = &st->e;

	if (!st->have_cell || !e->bssid[0])
		return 0;

	/* Width: most capable operation block wins, HT resolves VHT's "20 or 40 MHz" */
	if (st->eht_width)
		e->width = st->eht_width;
	else if (st->he_width)
		e->width = st->he_width;
	else if (st->vht_width)
		e->width = st->vht_width;
	else if (st->ht_width)
		e->width = st->ht_width;
	else
		e->width = 20;

	/* Control channel: HT states it explicitly, otherwise the Mode line has it */
	if (st->ht_primary)
		e->channel = st->ht_primary;

	/* Centre channel: likewise, and HT40 derives it from the secondary offset */
	if (st->eht_center)
		e->center_chan = st->eht_center;
	else if (st->he_center)
		e->center_chan = st->he_center;
	else if (st->vht_center)
		e->center_chan = st->vht_center;
	else if (st->ht_width == 40 && st->ht_offset)
		e->center_chan = e->channel + (2 * st->ht_offset);
	else
		e->center_chan = e->channel;

	if (st->eht_center)
		e->proto = "11be";
	else if (st->he_center)
		e->proto = "11ax";
	else if (st->vht_center)
		e->proto = "11ac";
	else if (st->ht_primary)
		e->proto = "11n";
	else if (strcmp(e->band, "2.4") == 0)
		e->proto = "11g";
	else
		e->proto = "11a";

	return callback(e, user_data);
}

/*
 * Reset the parser for a new Cell block
 */
static void scan_reset(struct scan_parse_state *st)
{
	memset(st, 0, sizeof(*st));
	st->e.quality = -1;
	st->e.band = "5";
	st->e.proto = "11a";
	snprintf(st->e.security, sizeof(st->e.security), "%s", "NONE");
	snprintf(st->e.cipher, sizeof(st->e.cipher), "%s", "NONE");
}

/*
 * Parse an already-open stream of 'iwinfo scan' output
 */
static int scan_parse_stream(FILE *fp, FILE *cache, wlhelper_scan_callback callback,
                             void *user_data, int *refused)
{
	struct scan_parse_state st;
	char raw[LINE_BUFFER_SIZE];
	char field[128];
	char *line, *p;
	int count = 0;

	scan_reset(&st);

	while (fgets(raw, sizeof(raw), fp) != NULL) {
		if (cache)
			fputs(raw, cache);

		scan_rtrim(raw);
		line = scan_skip_ws(raw);

		if (!*line)
			continue;

		/*
		 * The scan never ran: mac80211 refused to leave the operating
		 * channel. Distinct from "No scan results", which just means the
		 * radio heard nothing.
		 */
		if (refused && (strncmp(line, "Scanning not possible", 21) == 0)) {
			*refused = 1;
			continue;
		}

		/* New BSS: emit whatever was collected for the previous one */
		if (strncmp(line, "Cell ", 5) == 0) {
			if (scan_flush(&st, callback, user_data) != 0)
				return count + 1;

			if (st.have_cell && st.e.bssid[0])
				count++;

			scan_reset(&st);

			if ((p = strstr(line, "Address:")) != NULL) {
				p = scan_skip_ws(p + 8);
				snprintf(st.e.bssid, sizeof(st.e.bssid), "%s", p);
				st.have_cell = 1;
			}
			continue;
		}

		if (!st.have_cell)
			continue;

		if ((p = scan_field(line, "ESSID:")) != NULL) {
			/* Quoted, or the literal word "unknown" for a hidden BSS */
			if (*p == '"') {
				char *end = strrchr(p + 1, '"');

				if (end)
					*end = '\0';

				snprintf(st.e.ssid, sizeof(st.e.ssid), "%s", p + 1);
			}
			continue;
		}

		if (strncmp(line, "Mode:", 5) == 0) {
			scan_inline_field(line, "Frequency:", field, sizeof(field));
			st.e.mhz = (int)(atof(field) * 1000.0);

			scan_inline_field(line, "Channel:", field, sizeof(field));
			st.e.channel = atoi(field);

			scan_inline_field(line, "Band:", field, sizeof(field));
			st.e.band = scan_band_name(field, st.e.mhz);
			continue;
		}

		if (strncmp(line, "Signal:", 7) == 0) {
			scan_inline_field(line, "Signal:", field, sizeof(field));
			st.e.signal = atoi(field);

			scan_inline_field(line, "Quality:", field, sizeof(field));
			if ((p = strchr(field, '/')) != NULL) {
				int q = atoi(field);
				int qmax = atoi(p + 1);

				st.e.quality = (q > 0 && qmax > 0) ? ((100 * q) / qmax) : 0;
				if (st.e.quality > 100)
					st.e.quality = 100;
			}
			continue;
		}

		if ((p = scan_field(line, "Encryption:")) != NULL) {
			scan_parse_encryption(p, st.e.security, sizeof(st.e.security),
			                      st.e.cipher, sizeof(st.e.cipher));
			continue;
		}

		if (strncmp(line, "HT Operation:", 13) == 0) {
			st.section = SCAN_SEC_HT;
			continue;
		}
		if (strncmp(line, "VHT Operation:", 14) == 0) {
			st.section = SCAN_SEC_VHT;
			continue;
		}
		if (strncmp(line, "HE Operation:", 13) == 0) {
			st.section = SCAN_SEC_HE;
			continue;
		}
		if (strncmp(line, "EHT Operation:", 14) == 0) {
			st.section = SCAN_SEC_EHT;
			continue;
		}

		if ((p = scan_field(line, "Primary Channel:")) != NULL) {
			st.ht_primary = atoi(p);
			continue;
		}

		if ((p = scan_field(line, "Secondary Channel Offset:")) != NULL) {
			st.ht_offset = (strncmp(p, "above", 5) == 0) ? 1 : (strncmp(p, "below", 5) == 0) ? -1 : 0;
			continue;
		}

		if ((p = scan_field(line, "Center Frequency 1:")) != NULL) {
			int c = atoi(p);

			if (st.section == SCAN_SEC_EHT)
				st.eht_center = c;
			else if (st.section == SCAN_SEC_HE)
				st.he_center = c;
			else if (st.section == SCAN_SEC_VHT)
				st.vht_center = c;
			continue;
		}

		if ((p = scan_field(line, "Channel Width:")) != NULL) {
			int w = scan_parse_width(st.section, p);

			if (st.section == SCAN_SEC_EHT)
				st.eht_width = w;
			else if (st.section == SCAN_SEC_HE)
				st.he_width = w;
			else if (st.section == SCAN_SEC_VHT)
				st.vht_width = w;
			else if (st.section == SCAN_SEC_HT)
				st.ht_width = w;
			continue;
		}
	}

	/* Last BSS in the list has no following Cell header to trigger the flush */
	if (scan_flush(&st, callback, user_data) == 0 && st.have_cell && st.e.bssid[0])
		count++;

	return count;
}

/*
 * Check that an interface name is a plain name
 *
 * wifi_phy<p>iface<i>_ifname is only length checked when it is saved from the
 * web UI, and the name is pasted into a shell command below and into the cache
 * path, so reject anything that is not a bare interface name.
 */
static int scan_valid_ifname(const char *ifname)
{
	const char *p;

	for (p = ifname; *p; p++)
		if (!isalnum((unsigned char)*p) && *p != '-' && *p != '_' && *p != '.')
			return 0;

	return (p != ifname) && ((size_t)(p - ifname) < IFNAMSIZ);
}

/*
 * Run a site survey on an interface and iterate over the results
 */
int wlhelper_foreach_scan_result(const char *ifname, int force,
                                  wlhelper_scan_callback callback, void *user_data)
{
	FILE *fp, *cache = NULL;
	struct stat st;
	char path[128];
	char tmp[160];
	char cmd[CMD_BUFFER_SIZE];
	int count, refused = 0;

	if (!ifname || !callback || !scan_valid_ifname(ifname))
		return -1;

	snprintf(path, sizeof(path), "/var/tmp/iwscan.%s", ifname);

	/*
	 * A scan blocks for several seconds per radio, which is far too long to
	 * repeat for every refresh of the survey page, so re-use a recent result.
	 */
	if (!force &&
	    stat(path, &st) == 0 &&
	    (time(NULL) - st.st_mtime) < WLHELPER_SCAN_CACHE_SEC &&
	    (fp = fopen(path, "r")) != NULL) {
		count = scan_parse_stream(fp, NULL, callback, user_data, NULL);
		fclose(fp);
		return count;
	}

	snprintf(cmd, sizeof(cmd), "iwinfo %s scan 2>/dev/null", ifname);
	fp = popen(cmd, "r");
	if (!fp)
		return -1;

	snprintf(tmp, sizeof(tmp), "%s.new", path);
	cache = fopen(tmp, "w");

	count = scan_parse_stream(fp, cache, callback, user_data, &refused);
	pclose(fp);

	if (cache) {
		fclose(cache);
		/* Only publish a cache entry that actually holds results */
		if (count > 0)
			rename(tmp, path);
		else
			unlink(tmp);
	}

	return (refused && (count == 0)) ? WLHELPER_SCAN_REFUSED : count;
}

#endif /* TOMATO64 */
