/*
 * Tomato64 LED Control via Linux sysfs interface
 * Copyright (C) 2024 Tomato64 Project
 *
 * LED control using the standard Linux LED subsystem.
 * Each device has an explicit list of managed LEDs defined at compile time.
 */

#ifndef _LED_SYSFS_H_
#define _LED_SYSFS_H_

#ifdef TOMATO64

#include <stddef.h>

/*
 * Standard LED function names (from Linux kernel dt-bindings/leds/common.h)
 * These are the function part of LED names like "green:wan", "red:power"
 */
#define LED_FUNC_POWER    "power"
#define LED_FUNC_STATUS   "status"
#define LED_FUNC_SYSTEM   "system"
#define LED_FUNC_RUN      "run"
#define LED_FUNC_WAN      "wan"
#define LED_FUNC_LAN      "lan"
#define LED_FUNC_WLAN     "wlan"
#define LED_FUNC_USB      "usb"
#define LED_FUNC_ACTIVITY "activity"

/*
 * LED trigger types (standard Linux LED triggers)
 */
#define LED_TRIGGER_NONE       "none"
#define LED_TRIGGER_TIMER      "timer"
#define LED_TRIGGER_HEARTBEAT  "heartbeat"
#define LED_TRIGGER_NETDEV     "netdev"
#define LED_TRIGGER_DEFAULT_ON "default-on"

/*
 * Check if LED exists in sysfs
 */
int led_exists(const char *name);

/*
 * Basic LED control by sysfs name
 */
int led_set(const char *name, int on);
int led_set_brightness(const char *name, int brightness);
int led_get_brightness(const char *name);

/*
 * LED trigger control
 */
int led_set_trigger(const char *name, const char *trigger);
int led_get_trigger(const char *name, char *trigger, size_t len);

/*
 * Timer trigger (blinking)
 */
int led_set_timer(const char *name, int delay_on_ms, int delay_off_ms);

/*
 * Network device activity trigger
 */
int led_set_netdev(const char *name, const char *ifname, int link, int tx, int rx);

/*
 * LED lookup - find LED by function name from managed list
 * Returns sysfs name or NULL if not found
 */
const char *led_find(const char *function);
const char *led_find_enum(const char *function, int enumerator);

/*
 * LED list access
 */
const char *led_get_by_index(int index);
int led_get_count(void);

/*
 * Global LED control (only affects managed LEDs)
 */
int led_all_off(void);
int led_all_on(void);

/*
 * Stealth mode - turns off ALL LEDs in system (not just managed ones)
 */
void led_stealth(int enable);

/*
 * Boot sequence LED control
 */
void led_boot_init(void);
void led_boot_done(void);

/*
 * System state LED indication
 */
void led_state_upgrade(void);
void led_state_shutdown(void);

/*
 * Main LED setup entry point - called after boot
 */
void led_setup_sysfs(void);

#endif /* TOMATO64 */

#endif /* _LED_SYSFS_H_ */
