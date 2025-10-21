/*
 * wlhelper.c - WiFi helper functions for TOMATO64
 *
 * Tomato Firmware
 * Copyright (C) 2025
 *
 * This file contains helper functions for working with WiFi interfaces
 * in TOMATO64, replacing shell script implementations with native C code.
 */

#ifdef TOMATO64_WIFI

#include "tomato.h"
#include "wlhelper.h"

#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include <unistd.h>

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
		/* Use default naming: phy{phy}-ap{iface} */
		snprintf(ifname, size, "phy%d-ap%d", phy, iface);
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
                                 int *nbw, int *noise, float *rate)
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
		if (found_master && found_noise && found_rate)
			break;
	}

	pclose(fp);
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

#endif /* TOMATO64_WIFI */
