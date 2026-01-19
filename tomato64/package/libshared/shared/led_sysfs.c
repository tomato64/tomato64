/*
 * Tomato64 LED Control via Linux sysfs interface
 * Copyright (C) 2024 Tomato64 Project
 *
 * LED control using the standard Linux LED subsystem.
 * Each device has an explicit list of managed LEDs - only these LEDs
 * are controlled by this code. Driver-managed LEDs (like mt76-phy*)
 * are left untouched.
 */

#ifdef TOMATO64

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <dirent.h>
#include <errno.h>
#include <syslog.h>
#include <ctype.h>

#include <bcmnvram.h>
#include "shutils.h"
#include "shared.h"
#include "led_sysfs.h"

/* needed by logmsg() */
#define LOGMSG_DISABLE	DISABLE_SYSLOG_OS
#define LOGMSG_NVDEBUG	"led_debug"

#define SYSFS_LEDS_PATH "/sys/class/leds"

/*
 * ============================================================================
 * Per-device LED definitions
 *
 * Each device lists only the LEDs we want to manage. This keeps us from
 * accidentally touching driver-controlled LEDs (like mt76-phy0/phy1).
 *
 * Format: NULL-terminated array of LED sysfs names
 * ============================================================================
 */

#if defined(TOMATO64_R6S)
/*
 * NanoPi R6S LEDs:
 * - red:power    = System power indicator
 * - green:wan    = WAN port activity
 * - green:lan-1  = LAN port 1 activity
 * - green:lan-2  = LAN port 2 activity
 */
static const char * const managed_leds[] = {
	"red:power",
	"green:wan",
	"green:lan-1",
	"green:lan-2",
	NULL
};

#elif defined(TOMATO64_MT6000)
/*
 * GL.iNet GL-MT6000 LEDs:
 * - blue:run     = Boot/activity indicator
 * - white:system = System status
 */
static const char * const managed_leds[] = {
	"blue:run",
	"white:system",
	NULL
};

#elif defined(TOMATO64_BPIR3)
/*
 * Banana Pi BPI-R3 LEDs:
 * - green:power  = Power indicator
 * - blue:status  = Status indicator
 * Note: mt76-phy0/phy1 LEDs are controlled by the WiFi driver
 */
static const char * const managed_leds[] = {
	"green:power",
	"blue:status",
	NULL
};

#elif defined(TOMATO64_BPIR3MINI)
/*
 * Banana Pi BPI-R3 Mini LEDs:
 * - green:status = Status indicator
 * - blue:wlan-1  = 2.4GHz WiFi indicator (GPIO, not driver-controlled)
 * - blue:wlan-2  = 5GHz WiFi indicator (GPIO, not driver-controlled)
 */
static const char * const managed_leds[] = {
	"green:status",
	"blue:wlan-1",
	"blue:wlan-2",
	NULL
};

#elif defined(TOMATO64_RPI4)
/*
 * Raspberry Pi 4 LEDs:
 * - ACT = Activity LED (directly named, no color prefix)
 * - PWR = Power LED (directly named, no color prefix)
 */
static const char * const managed_leds[] = {
	"ACT",
	"PWR",
	NULL
};

#else
/* x86_64 and unknown platforms - no LEDs */
static const char * const managed_leds[] = {
	NULL
};
#endif

/*
 * ============================================================================
 * Sysfs helper functions
 * ============================================================================
 */

/*
 * Helper: write string to sysfs file
 */
static int sysfs_write(const char *path, const char *value)
{
	int fd, ret = 0;
	size_t len;

	fd = open(path, O_WRONLY);
	if (fd < 0) {
		logmsg(LOG_DEBUG, "*** %s: open %s: %s", __FUNCTION__, path, strerror(errno));
		return -1;
	}

	len = strlen(value);
	if (write(fd, value, len) != (ssize_t)len) {
		logmsg(LOG_DEBUG, "*** %s: write %s: %s", __FUNCTION__, path, strerror(errno));
		ret = -1;
	}

	close(fd);
	return ret;
}

/*
 * Helper: read string from sysfs file
 */
static int sysfs_read(const char *path, char *buf, size_t buflen)
{
	int fd;
	ssize_t n;

	if (buflen == 0)
		return -1;

	fd = open(path, O_RDONLY);
	if (fd < 0)
		return -1;

	n = read(fd, buf, buflen - 1);
	close(fd);

	if (n < 0)
		return -1;

	if ((size_t)n >= buflen)
		n = buflen - 1;

	buf[n] = '\0';

	/* Strip trailing newlines */
	while (n > 0 && (buf[n-1] == '\n' || buf[n-1] == '\r'))
		buf[--n] = '\0';

	return 0;
}

/*
 * ============================================================================
 * LED name parsing
 * ============================================================================
 */

/*
 * Parse LED name into color, function, and enumerator
 * Format: "color:function" or "color:function-N"
 * Examples: "green:wan", "green:lan-1", "blue:wlan-2", "ACT", "PWR"
 */
static void parse_led_name(const char *name, char *color, size_t clen,
                           char *func, size_t flen, int *enumerator)
{
	const char *colon, *dash;
	size_t len;

	color[0] = '\0';
	func[0] = '\0';
	*enumerator = -1;

	colon = strchr(name, ':');
	if (colon) {
		/* Has color:function format */
		len = colon - name;
		if (len >= clen) len = clen - 1;
		memcpy(color, name, len);
		color[len] = '\0';

		colon++;
		dash = strrchr(colon, '-');
		if (dash && isdigit((unsigned char)dash[1])) {
			/* Has enumerator: function-N */
			len = dash - colon;
			if (len >= flen) len = flen - 1;
			memcpy(func, colon, len);
			func[len] = '\0';
			*enumerator = atoi(dash + 1);
		} else {
			strncpy(func, colon, flen - 1);
			func[flen - 1] = '\0';
		}
	} else {
		/* No color, whole name is function (e.g., "ACT", "PWR") */
		strncpy(func, name, flen - 1);
		func[flen - 1] = '\0';
	}
}

/*
 * ============================================================================
 * LED access functions
 * ============================================================================
 */

/*
 * Check if LED is in our managed list
 */
static int led_is_managed(const char *name)
{
	int i;

	if (!name || !*name)
		return 0;

	for (i = 0; managed_leds[i] != NULL; i++) {
		if (strcmp(managed_leds[i], name) == 0)
			return 1;
	}
	return 0;
}

/*
 * Check if LED exists in sysfs
 */
int led_exists(const char *name)
{
	char path[256];
	struct stat st;

	if (!name || !*name)
		return 0;

	snprintf(path, sizeof(path), "%s/%s", SYSFS_LEDS_PATH, name);
	return (stat(path, &st) == 0 && S_ISDIR(st.st_mode));
}

/*
 * Set LED on/off
 */
int led_set(const char *name, int on)
{
	char path[256];

	if (!name || !*name)
		return -1;

	snprintf(path, sizeof(path), "%s/%s/brightness", SYSFS_LEDS_PATH, name);
	return sysfs_write(path, on ? "255" : "0");
}

/*
 * Set LED brightness (0-255)
 */
int led_set_brightness(const char *name, int brightness)
{
	char path[256], value[16];

	if (!name || !*name)
		return -1;

	if (brightness < 0) brightness = 0;
	if (brightness > 255) brightness = 255;

	snprintf(path, sizeof(path), "%s/%s/brightness", SYSFS_LEDS_PATH, name);
	snprintf(value, sizeof(value), "%d", brightness);
	return sysfs_write(path, value);
}

/*
 * Get LED brightness
 */
int led_get_brightness(const char *name)
{
	char path[256], buf[16];

	if (!name || !*name)
		return -1;

	snprintf(path, sizeof(path), "%s/%s/brightness", SYSFS_LEDS_PATH, name);
	if (sysfs_read(path, buf, sizeof(buf)) < 0)
		return -1;

	return atoi(buf);
}

/*
 * Set LED trigger
 */
int led_set_trigger(const char *name, const char *trigger)
{
	char path[256];

	if (!name || !*name || !trigger)
		return -1;

	snprintf(path, sizeof(path), "%s/%s/trigger", SYSFS_LEDS_PATH, name);
	return sysfs_write(path, trigger);
}

/*
 * Get current LED trigger
 */
int led_get_trigger(const char *name, char *trigger, size_t len)
{
	char path[256], buf[512];
	char *start, *end;

	if (!name || !*name || !trigger || len == 0)
		return -1;

	snprintf(path, sizeof(path), "%s/%s/trigger", SYSFS_LEDS_PATH, name);
	if (sysfs_read(path, buf, sizeof(buf)) < 0)
		return -1;

	/* Active trigger enclosed in brackets: none [timer] heartbeat */
	start = strchr(buf, '[');
	if (start) {
		start++;
		end = strchr(start, ']');
		if (end) {
			size_t tlen = end - start;
			if (tlen >= len) tlen = len - 1;
			memcpy(trigger, start, tlen);
			trigger[tlen] = '\0';
			return 0;
		}
	}

	/* No brackets, first word is trigger */
	strncpy(trigger, buf, len - 1);
	trigger[len - 1] = '\0';
	end = strchr(trigger, ' ');
	if (end) *end = '\0';

	return 0;
}

/*
 * Set timer trigger (blinking)
 */
int led_set_timer(const char *name, int delay_on_ms, int delay_off_ms)
{
	char path[256], value[16];

	if (!name || !*name)
		return -1;

	if (led_set_trigger(name, LED_TRIGGER_TIMER) < 0)
		return -1;

	snprintf(path, sizeof(path), "%s/%s/delay_on", SYSFS_LEDS_PATH, name);
	snprintf(value, sizeof(value), "%d", delay_on_ms);
	sysfs_write(path, value);

	snprintf(path, sizeof(path), "%s/%s/delay_off", SYSFS_LEDS_PATH, name);
	snprintf(value, sizeof(value), "%d", delay_off_ms);
	sysfs_write(path, value);

	return 0;
}

/*
 * Set netdev trigger for network activity
 */
int led_set_netdev(const char *name, const char *ifname, int link, int tx, int rx)
{
	char path[256];

	if (!name || !*name || !ifname || !*ifname)
		return -1;

	if (led_set_trigger(name, LED_TRIGGER_NETDEV) < 0)
		return -1;

	snprintf(path, sizeof(path), "%s/%s/device_name", SYSFS_LEDS_PATH, name);
	if (sysfs_write(path, ifname) < 0)
		return -1;

	snprintf(path, sizeof(path), "%s/%s/link", SYSFS_LEDS_PATH, name);
	sysfs_write(path, link ? "1" : "0");

	snprintf(path, sizeof(path), "%s/%s/tx", SYSFS_LEDS_PATH, name);
	sysfs_write(path, tx ? "1" : "0");

	snprintf(path, sizeof(path), "%s/%s/rx", SYSFS_LEDS_PATH, name);
	sysfs_write(path, rx ? "1" : "0");

	return 0;
}

/*
 * ============================================================================
 * LED lookup functions
 * ============================================================================
 */

/*
 * Find managed LED by function name
 * Returns sysfs name or NULL if not found
 */
const char *led_find(const char *function)
{
	return led_find_enum(function, -1);
}

/*
 * Find managed LED by function name and optional enumerator
 * enumerator = -1 means match any/no enumerator
 * Only searches within the managed_leds list
 */
const char *led_find_enum(const char *function, int enumerator)
{
	char color[16], func[32];
	int led_enum;
	int i;

	if (!function || !*function)
		return NULL;

	for (i = 0; managed_leds[i] != NULL; i++) {
		parse_led_name(managed_leds[i], color, sizeof(color),
		               func, sizeof(func), &led_enum);

		if (strcmp(func, function) == 0) {
			if (enumerator < 0 || led_enum == enumerator) {
				return managed_leds[i];
			}
		}
	}

	return NULL;
}

/*
 * Get LED by index from managed list
 * Returns NULL if index out of bounds
 */
const char *led_get_by_index(int index)
{
	int i;

	for (i = 0; managed_leds[i] != NULL; i++) {
		if (i == index)
			return managed_leds[i];
	}
	return NULL;
}

/*
 * Get count of managed LEDs
 */
int led_get_count(void)
{
	int i;

	for (i = 0; managed_leds[i] != NULL; i++)
		;
	return i;
}

/*
 * ============================================================================
 * LED control functions
 * ============================================================================
 */

/*
 * Turn off all managed LEDs
 */
int led_all_off(void)
{
	int i;

	for (i = 0; managed_leds[i] != NULL; i++) {
		if (led_exists(managed_leds[i])) {
			led_set_trigger(managed_leds[i], LED_TRIGGER_NONE);
			led_set(managed_leds[i], 0);
		}
	}
	return 0;
}

/*
 * Turn on all managed LEDs
 */
int led_all_on(void)
{
	int i;

	for (i = 0; managed_leds[i] != NULL; i++) {
		if (led_exists(managed_leds[i])) {
			led_set_trigger(managed_leds[i], LED_TRIGGER_NONE);
			led_set(managed_leds[i], 1);
		}
	}
	return 0;
}

/*
 * ============================================================================
 * Boot sequence LED control
 * ============================================================================
 */

/*
 * Initialize LEDs for boot sequence
 * Sets a suitable LED to heartbeat during boot
 */
void led_boot_init(void)
{
	const char *led = NULL;

	/* Try to find a suitable LED for boot indication */
	led = led_find(LED_FUNC_POWER);
	if (!led) led = led_find(LED_FUNC_STATUS);
	if (!led) led = led_find(LED_FUNC_SYSTEM);
	if (!led) led = led_find(LED_FUNC_RUN);

	/* Fallback: use first LED in list */
	if (!led)
		led = led_get_by_index(0);

	if (led && led_exists(led))
		led_set_trigger(led, LED_TRIGGER_HEARTBEAT);
}

/*
 * Finalize LED setup after boot
 * Sets power/system LEDs to steady on
 */
void led_boot_done(void)
{
	const char *led;

	/* Power LED - steady on */
	led = led_find(LED_FUNC_POWER);
	if (led && led_exists(led)) {
		led_set_trigger(led, LED_TRIGGER_NONE);
		led_set(led, 1);
	}

	/* Status/system LED - steady on */
	led = led_find(LED_FUNC_STATUS);
	if (!led) led = led_find(LED_FUNC_SYSTEM);
	if (!led) led = led_find(LED_FUNC_RUN);
	if (led && led_exists(led)) {
		led_set_trigger(led, LED_TRIGGER_NONE);
		led_set(led, 1);
	}
}

/*
 * Turn off ALL LEDs in the system (not just managed ones)
 * Used by stealth mode to ensure everything is dark
 */
static void led_all_off_system(void)
{
	DIR *dir;
	struct dirent *entry;
	char path[256];

	dir = opendir(SYSFS_LEDS_PATH);
	if (!dir) {
		logmsg(LOG_DEBUG, "*** %s: cannot open %s", __FUNCTION__, SYSFS_LEDS_PATH);
		return;
	}

	while ((entry = readdir(dir)) != NULL) {
		/* Skip . and .. */
		if (entry->d_name[0] == '.')
			continue;

		/* Turn off this LED */
		snprintf(path, sizeof(path), "%s/%s/trigger", SYSFS_LEDS_PATH, entry->d_name);
		sysfs_write(path, "none");

		snprintf(path, sizeof(path), "%s/%s/brightness", SYSFS_LEDS_PATH, entry->d_name);
		sysfs_write(path, "0");
	}

	closedir(dir);
}

/*
 * Enable or disable stealth mode (all LEDs off)
 * When enabled, turns off ALL LEDs in the system, not just managed ones
 */
void led_stealth(int enable)
{
	if (enable) {
		led_all_off_system();
	} else {
		led_boot_done();
	}
}

/*
 * ============================================================================
 * System state LED indication
 * ============================================================================
 */

/*
 * Set LEDs to indicate upgrade in progress
 * Uses fast blink pattern (200ms on/off like OpenWrt preinit_regular)
 */
void led_state_upgrade(void)
{
	const char *led = NULL;

	/* Turn off all LEDs first */
	led_all_off();

	/* Find status/power LED for upgrade indication */
	led = led_find(LED_FUNC_POWER);
	if (!led) led = led_find(LED_FUNC_STATUS);
	if (!led) led = led_find(LED_FUNC_SYSTEM);
	if (!led) led = led_find(LED_FUNC_RUN);
	if (!led)
		led = led_get_by_index(0);

	if (led && led_exists(led))
		led_set_timer(led, 200, 200);
}

/*
 * Set LEDs to indicate shutdown/reboot in progress
 * Uses faster blink pattern (100ms on/off) to indicate urgent operation
 */
void led_state_shutdown(void)
{
	const char *led = NULL;

	/* Turn off all LEDs first */
	led_all_off();

	/* Find status/power LED for shutdown indication */
	led = led_find(LED_FUNC_POWER);
	if (!led) led = led_find(LED_FUNC_STATUS);
	if (!led) led = led_find(LED_FUNC_SYSTEM);
	if (!led) led = led_find(LED_FUNC_RUN);
	if (!led)
		led = led_get_by_index(0);

	if (led && led_exists(led))
		led_set_timer(led, 100, 100);
}

/*
 * ============================================================================
 * Network LED configuration
 * ============================================================================
 */

/*
 * Platform-specific network LED configuration
 */
static void led_setup_network(void)
{
#if defined(TOMATO64_R6S)
	/*
	 * NanoPi R6S:
	 * - eth0 = WAN
	 * - eth1 = LAN port 1
	 * - eth2 = LAN port 2
	 */
	if (led_exists("green:wan"))
		led_set_netdev("green:wan", "eth0", 1, 1, 1);

	if (led_exists("green:lan-1"))
		led_set_netdev("green:lan-1", "eth1", 1, 1, 1);

	if (led_exists("green:lan-2"))
		led_set_netdev("green:lan-2", "eth2", 1, 1, 1);

#elif defined(TOMATO64_BPIR3MINI)
	/*
	 * BPI-R3 Mini: GPIO-based WiFi indicator LEDs
	 * These are NOT driver-controlled, need manual setup
	 */
	if (led_exists("blue:wlan-1"))
		led_set_netdev("blue:wlan-1", "phy0-ap0", 1, 1, 1);

	if (led_exists("blue:wlan-2"))
		led_set_netdev("blue:wlan-2", "phy1-ap0", 1, 1, 1);

#elif defined(TOMATO64_BPIR3) || defined(TOMATO64_MT6000) || defined(TOMATO64_RPI4)
	/*
	 * BPI-R3: WiFi LEDs (mt76-phy*) controlled by driver
	 * MT6000: No network LEDs
	 * RPI4: No network LEDs
	 */

#else
	/* Generic/x86_64: No network LEDs */

#endif
}

/*
 * ============================================================================
 * Main entry point
 * ============================================================================
 */

/*
 * Main LED setup entry point
 * Called after boot is complete
 */
void led_setup_sysfs(void)
{
	/* Check stealth mode */
	if (nvram_match("stealth_mode", "1")) {
		led_stealth(1);
		return;
	}

	/* Set LEDs to normal running state */
	led_boot_done();

	/* Configure network activity LEDs */
	led_setup_network();
}

#endif /* TOMATO64 */
