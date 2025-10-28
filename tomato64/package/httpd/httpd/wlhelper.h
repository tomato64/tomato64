/*
 * wlhelper.h - WiFi helper functions for TOMATO64
 *
 * Tomato Firmware
 * Copyright (C) 2025
 *
 * This file contains helper functions for working with WiFi interfaces
 * in TOMATO64, replacing shell script implementations with native C code.
 */

#ifndef __WLHELPER_H__
#define __WLHELPER_H__

#ifdef TOMATO64

#include <stddef.h>

/*
 * Get noise level for a given interface using iwinfo
 *
 * @param ifname: Interface name (e.g., "phy0-ap0")
 * @return: Noise level in dBm, or -99 if unknown/error
 */
int wlhelper_get_noise_level(const char *ifname);

/*
 * Get interface name for a given PHY and interface index
 *
 * Reads from nvram wifi_phy{phy}iface{iface}_ifname or uses default
 * format phy{phy}-ap{iface}
 *
 * @param phy: PHY index (0, 1, 2, ...)
 * @param iface: Interface index (0, 1, 2, ...)
 * @param ifname: Output buffer for interface name
 * @param size: Size of output buffer
 */
void wlhelper_get_ifname(int phy, int iface, char *ifname, size_t size);

/*
 * Count number of WiFi PHYs by checking nvram
 *
 * First checks wifi_phy_count (set by wlconfig during hotplug).
 * If not set, falls back to device-specific defaults (matches defaults.c).
 *
 * @return: Number of PHYs (device-specific: MT6000/BPI-R3/BPI-R3Mini=2, unknown=0)
 */
int wlhelper_count_phys(void);

/*
 * Check if an interface is enabled
 *
 * @param phy: PHY index
 * @param iface: Interface index
 * @return: 1 if enabled, 0 otherwise
 */
int wlhelper_is_iface_enabled(int phy, int iface);

/*
 * Get the mode of an interface (ap, sta, etc.)
 *
 * @param phy: PHY index
 * @param iface: Interface index
 * @return: Mode string from nvram, or NULL if not set
 */
const char* wlhelper_get_iface_mode(int phy, int iface);

/*
 * Get number of interfaces for a given PHY
 *
 * @param phy: PHY index
 * @return: Number of interfaces configured for this PHY
 */
int wlhelper_get_iface_count(int phy);

/*
 * Execute iwinfo command and get output
 *
 * This is a helper function to call iwinfo commands and parse output.
 * Used internally by other wlhelper functions.
 *
 * @param ifname: Interface name
 * @param command: iwinfo command (e.g., "info", "scan")
 * @param output: Buffer to store output
 * @param output_size: Size of output buffer
 * @return: 0 on success, -1 on error
 */
int wlhelper_exec_iwinfo(const char *ifname, const char *command,
                         char *output, size_t output_size);

/*
 * Get a specific field from iwinfo info output
 *
 * Parses iwinfo <iface> info output for a specific field.
 * Example: wlhelper_get_iwinfo_field("phy0-ap0", "PHY name:", value, sizeof(value))
 *
 * @param ifname: Interface name
 * @param field_name: Field to search for (e.g., "PHY name:", "Channel:")
 * @param value: Output buffer for field value
 * @param value_size: Size of value buffer
 * @return: 0 on success, -1 on error or field not found
 */
int wlhelper_get_iwinfo_field(const char *ifname, const char *field_name,
                               char *value, size_t value_size);

/*
 * Get MAC address for an interface from sysfs
 *
 * Reads /sys/class/net/<ifname>/address
 *
 * @param ifname: Interface name
 * @param mac: Output buffer for MAC address (e.g., "AA:BB:CC:DD:EE:FF")
 * @param mac_size: Size of mac buffer (should be at least 18 bytes)
 * @return: 0 on success, -1 on error
 */
int wlhelper_get_mac_address(const char *ifname, char *mac, size_t mac_size);

/*
 * Check if a network interface exists
 *
 * Checks if /sys/class/net/<ifname>/address exists
 *
 * @param ifname: Interface name
 * @return: 1 if exists, 0 if not
 */
int wlhelper_iface_exists(const char *ifname);

/*
 * Get channel statistics from iwinfo
 *
 * Parses iwinfo info output to extract channel, mhz, nbw, noise, and rate.
 *
 * @param ifname: Interface name
 * @param channel: Output for channel number
 * @param mhz: Output for frequency in MHz (e.g., 2412, 5180)
 * @param nbw: Output for channel bandwidth (20, 40, 80, 160, or 0 for NOHT)
 * @param noise: Output for noise level in dBm (-99 if unknown)
 * @param rate: Output for bit rate in Mbit/s (0.0 if unknown, can be decimal like 864.6)
 * @return: 0 on success, -1 on error
 */
int wlhelper_get_channel_stats(const char *ifname, int *channel, int *mhz,
                                 int *nbw, int *noise, float *rate);

/*
 * Station (client) information structure
 */
struct wlhelper_station_info {
	char mac[18];           /* MAC address in uppercase (AA:BB:CC:DD:EE:FF) */
	int signal;             /* Signal strength in dBm */
	int tx_bitrate;         /* TX bitrate in kbit/s (multiplied by 1000) */
	int rx_bitrate;         /* RX bitrate in kbit/s (multiplied by 1000) */
	int connected_time;     /* Connected time in seconds */
};

/*
 * Callback function type for processing each station
 *
 * @param ifname: Interface name
 * @param phy: PHY index
 * @param station: Station information
 * @param user_data: User-provided data pointer
 * @return: 0 to continue, non-zero to stop iteration
 */
typedef int (*wlhelper_station_callback)(const char *ifname, int phy,
                                          const struct wlhelper_station_info *station,
                                          void *user_data);

/*
 * Iterate through all connected stations on an interface
 *
 * Parses output from 'iw <ifname> station dump' and calls the callback
 * for each connected station.
 *
 * @param ifname: Interface name
 * @param phy: PHY index (passed to callback)
 * @param callback: Function to call for each station
 * @param user_data: User data to pass to callback
 * @return: Number of stations found, or -1 on error
 */
int wlhelper_foreach_station(const char *ifname, int phy,
                               wlhelper_station_callback callback,
                               void *user_data);

/*
 * Filter flags for wlhelper_foreach_interface()
 */
#define WLHELPER_FILTER_NONE     0
#define WLHELPER_FILTER_ENABLED  (1 << 0)  /* Only enabled interfaces */
#define WLHELPER_FILTER_AP_MODE  (1 << 1)  /* Only AP mode interfaces */
#define WLHELPER_FILTER_STA_MODE (1 << 2)  /* Only STA mode interfaces */

/*
 * Callback function type for iterating through interfaces
 *
 * @param phy: PHY index (0, 1, 2, ...)
 * @param iface: Interface index (0, 1, 2, ...)
 * @param ifname: Interface name (e.g., "phy0-ap0")
 * @param user_data: User-provided data pointer
 * @return: 0 to continue iteration, non-zero to stop
 */
typedef int (*wlhelper_iface_callback)(int phy, int iface, const char *ifname, void *user_data);

/*
 * Iterate through WiFi interfaces with optional filtering
 *
 * This function eliminates the need for duplicate PHY/interface iteration
 * loops by providing a single callback-based iterator with filtering.
 *
 * Example usage:
 *   wlhelper_foreach_interface(WLHELPER_FILTER_ENABLED | WLHELPER_FILTER_AP_MODE,
 *                               my_callback, &my_data);
 *
 * @param filter_flags: Bitmask of WLHELPER_FILTER_* flags
 * @param callback: Function to call for each matching interface
 * @param user_data: User data to pass to callback
 * @return: Number of interfaces processed (callback called), or -1 on error
 */
int wlhelper_foreach_interface(int filter_flags, wlhelper_iface_callback callback, void *user_data);

#endif /* TOMATO64 */

#endif /* __WLHELPER_H__ */
