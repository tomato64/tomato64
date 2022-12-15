/*
 *
 * Tomato Firmware
 * Copyright (C) 2006-2009 Jonathan Zarate
 *
 */


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <bcmnvram.h>
#include <syslog.h>
#include <sys/ioctl.h>
#include <linux_gpio.h>

#include "utils.h"
#include "shutils.h"
#include "shared.h"

/* needed by logmsg() */
#define LOGMSG_DISABLE	DISABLE_SYSLOG_OS
#define LOGMSG_NVDEBUG	"led_debug"


#ifdef TCONFIG_AC3200
const char *led_names[] = {"wlan", "diag", "white", "amber", "dmz", "aoss", "bridge", "usb", "usb3", "5g", "52g"};
#else
const char *led_names[] = {"wlan", "diag", "white", "amber", "dmz", "aoss", "bridge", "usb", "usb3", "5g"};
#endif

static int _gpio_ioctl(int f, int gpioreg, unsigned int mask, unsigned int val)
{
	struct gpio_ioctl gpio;

	gpio.val = val;
	gpio.mask = mask;

	if (ioctl(f, gpioreg, &gpio) < 0) {
		logmsg(LOG_DEBUG, "*** %s: invalid gpioreg %d", __FUNCTION__, gpioreg);
		return -1;
	}

	return (gpio.val);
}

static int _gpio_open()
{
	int f = open("/dev/gpio", O_RDWR);
	if (f < 0)
		logmsg(LOG_DEBUG, "*** %s: failed to open /dev/gpio", __FUNCTION__);

	return f;
}

int gpio_open(uint32_t mask)
{
	uint32_t bit = 0;
	int i = 0 ;
	int f = _gpio_open();

	if ((f >= 0) && mask) {
		for (i = TOMATO_GPIO_MIN; i <= TOMATO_GPIO_MAX; i++) {
			bit = 1 << i;
			if ((mask & bit) == bit) {
				_gpio_ioctl(f, GPIO_IOC_RESERVE, bit, bit);
				_gpio_ioctl(f, GPIO_IOC_OUTEN, bit, 0);
			}
		}
		close(f);
		f = _gpio_open();
	}

	return f;
}

void gpio_write(uint32_t bit, int en)
{
	int f;

	if ((f = gpio_open(0)) < 0)
		return;

	_gpio_ioctl(f, GPIO_IOC_RESERVE, bit, bit);
	_gpio_ioctl(f, GPIO_IOC_OUTEN, bit, bit);
	_gpio_ioctl(f, GPIO_IOC_OUT, bit, en ? bit : 0);
	close(f);
}

uint32_t _gpio_read(int f)
{
	uint32_t r;
	r = _gpio_ioctl(f, GPIO_IOC_IN, 0x07FF, 0);
	if (r < 0)
		r = ~0;

	return r;
}

uint32_t gpio_read(void)
{
	int f;
	uint32_t r;

	if ((f = gpio_open(0)) < 0)
		return ~0;

	r = _gpio_read(f);
	close(f);

	return r;
}

uint32_t set_gpio(uint32_t gpio, uint32_t value)
{
	uint32_t bit = 0;

	if ( gpio <= TOMATO_GPIO_MAX && gpio >= TOMATO_GPIO_MIN ) {
		bit = 1 << gpio;
		logmsg(LOG_DEBUG, "*** %s: set_gpio: %d %d\n", __FUNCTION__, bit, value);
		gpio_write(bit, value);
		return 0;
	}
	else return 1;
}

int nvget_gpio(const char *name, int *gpio, int *inv)
{
	char *p;
	uint32_t n;

	if (((p = nvram_get(name)) != NULL) && (*p)) {
		n = strtoul(p, NULL, 0);
		if ((n & 0xFFFFFF60) == 0) {		/* bin 0110 0000 */
			*gpio = (n & TOMATO_GPIO_MAX);	/* bin 0001 1111 */
			*inv = ((n & 0x80) != 0);	/* bin 1000 0000 */
			/* 0x60 + 0x1F (dec 31) + 0x80 = 0xFF */
			return 1;
		}
	}
	return 0;
}

int do_led(int which, int mode)
{
/*
 * valid GPIO values: 0 to 31 (default active LOW, inverted or active HIGH with -[value])
 * value 255: not known / disabled / not possible
 * value -99: special case for -0 substitute (active HIGH for GPIO 0)
 * value 254: non GPIO LED (special case, to show there is something!)
 */
//				   WLAN  DIAG  WHITE AMBER   DMZ  AOSS  BRIDGE USB2 USB3    5G   52G
//				   ----  ----  ----- -----   ---  ----  ------ ---- ----    --   ---
#ifdef TCONFIG_AC3200
#ifdef TCONFIG_AC5300
	static int ac5300[]	= { 254,   -4,     5,  255,   19,    3,   21,   16,   17,  254,  254 };
#endif
	static int ac3200[]	= { 254,  -15,     5,  255,   14,    3,  254,  255,  255,  254,  254 };
	static int r8000[]	= {  13,    3,     8,  255,  -14,  -15,  254,   18,   17,   12,   16 };
#elif defined(CONFIG_BCMWL6A)
#ifdef TCONFIG_BCM714
	static int ac3100[]	= { 254,   -4,     5,  255,   19,    3,   21,   16,   17,  254}; /* also for RT-AC88U */
#endif /* TCONFIG_BCM714 */
	static int ac67u[]	= { 254,  255,     5,  255,  255,    0,  254,  255,  255,  254};
	static int dslac68u[]	= { 254,  255,     4,  255,  255,    3,  254,    0,   14,  254};
	static int ac68u[]	= { 254,  255,     4,  255,  255,    3,  254,    0,   14,  254};
	static int ac68u_v3[]	= { 254,  255,     4,  255,  255,    3,  254,    0,   14,  254};
	static int ac1450[]	= {  11,    3,    10,  255,  255,    1,  255,    8,  255,  255};
	static int ac1900p[]	= { 254,  255,     4,  255,  255,    3,  254,    0,   14,  254};
	static int ac66u_b1[]	= { 254,  255,     5,  255,  255,    0,  254,  255,  255,  254};
	static int ac56u[]	= { 254,  255,     1,  255,  255,    3,    2,   14,    0,    6};
	static int n18u[]	= { 254,  255,     6,  255,  255,    0,    9,    3,   14,  255};
	static int r6250[]	= {  11,    3,    15,  255,  255,   -1,  255,    8,  255,  255};
	static int r6300v2[]	= {  11,    3,    10,  255,  255,   -1,  255,    8,  255,  255};
	static int r6400[]	= {   9,    2,     7,  255,  -10,  -11,  254,   12,   13,    8};
	static int r6400v2[]	= {   9,    2,     7,  255,  -10,  -11,  254,   12,   13,    8};
	static int r6700v1[]	= {  13,    3,     9,  255,  -14,  -15,  254,   18,   17,   12};
	static int r6700v3[]	= {   9,    2,     7,  255,  -10,  -11,  254,   12,   13,    8};
	static int r6900[]	= {  13,    3,     9,  255,  -14,  -15,  254,   18,   17,   12};
	static int xr300[]	= {   9,    2,     7,  255,  -10,  -11,  254,   12,   13,    8};
	static int r7000[]	= {  13,    3,     9,  255,  -14,  -15,  254,   18,   17,   12};
	static int ac15[]	= { 254,  -99,   255,  255,  255,   -6,  254,  -14,  255,   -2};
	static int ac18[]	= { 254,  -99,   255,  255,  255,   -6,  254,  -14,  255,   -2};
	static int f9k1113v2[]	= { 255,   14,    12,  255,  255,   15,  255,   0,     1,  255};
	static int dir868[]	= { 255,    0,     3,  255,  255,  255,  255,  255,  255,  255};
	static int ea6350v1[]	= { 255,  255,    -8,  255,  255,  255,  254,  255,  255,  255};
	static int ea6400[]	= { 255,  255,    -8,  255,  255,  255,  254,  255,  255,  255};
	static int ea6500v2[]	= { 255,  255,     6,  255,  255,  255,  254,  255,  255,  255};
	static int ea6700[]	= { 255,  255,    -8,  255,  255,  255,  254,  255,  255,  255};
	static int ea6900[]	= { 255,  255,    -8,  255,  255,  255,  254,  255,  255,  255};
	static int ws880[]	= { 255,    6,   -12,  255,  255,    0,    1,   14,  255,  255};
	static int r1d[]	= { 255,    1,   255,  255,  255,  255,   -8,  255,  255,  255};
#if 0
	static int wzr1750[]	= {  -6,   -1,    -5,  255,  255,   -4,  255,  -99,  255,   -7}; /* tbd. 8 bit shift register (SPI GPIO 0 to 7), active HIGH M_ars*/
#endif
#endif /* TCONFIG_AC3200 */
//				   ----  ----  ----- -----   ---  ----  ------ ---- ----    --   ---
//				   WLAN  DIAG  WHITE AMBER   DMZ  AOSS  BRIDGE USB2 USB3    5G   52G

	char s[16];
	int n;
	int b = 255, c = 255;
	int ret = 255;
	static int model = 0; /* initialize with 0 / MODEL_UNKNOWN */

	if ((which < 0) || (which >= LED_COUNT))
		return ret;

	if (model == 0) { /* router model unknown OR detect router model for the first time at function do_led(). */
		/* get router model */
		model = get_model();
	}

	/* stealth mode ON ? */
	if (nvram_match("stealth_mode", "1")) {
		/* turn off WLAN LEDs for some Asus/Tenda Router: AC15, AC18, RT-N18U, RT-AC56U, RT-AC66U_B1, RT-AC67U, RT-AC68U (V3), RT-AC1900P, RT-AC3200, RT-AC3100, RT-AC88U, RT-AC5300 */
		switch (model) {
#ifdef TCONFIG_AC3200
			case MODEL_RTAC3200:
#ifdef TCONFIG_AC5300
			case MODEL_RTAC5300:
#endif /* TCONFIG_AC5300 */
#elif defined(CONFIG_BCMWL6A)
#ifdef TCONFIG_BCM714
			case MODEL_RTAC3100:
			case MODEL_RTAC88U:
#endif /* TCONFIG_BCM714 */
			case MODEL_AC15:
			case MODEL_AC18:
			case MODEL_RTN18U:
			case MODEL_RTAC56U:
			case MODEL_RTAC66U_B1:
			case MODEL_RTAC67U:
			case MODEL_DSLAC68U:
			case MODEL_RTAC68U:
			case MODEL_RTAC68UV3:
			case MODEL_RTAC1900P:
#endif /* TCONFIG_AC3200 */

#if defined(CONFIG_BCMWL6A) || defined(TCONFIG_BCM7)
				do_led_nongpio(model, which, LED_OFF);
				break;
#endif /* CONFIG_BCMWL6A OR TCONFIG_BCM7 */
			default:
				/* nothing to do right now */
				break;
		}

		if (nvram_match("stealth_iled", "1") && which == LED_WHITE) { /* do not disable WAN / INTERNET LED and set LED_WHITE */
			/* nothing to do right now */
		}
		else {
			return ret; /* stealth mode ON: no LED work to do, set return value to 255 / disabled */
		}
	}

	switch (nvram_match("led_override", "1") ? MODEL_UNKNOWN : model) {
#ifdef TCONFIG_AC3200
#ifdef TCONFIG_AC5300
	case MODEL_RTAC5300:
		b = ac5300[which];
		if ((which == LED_WLAN) ||
		    (which == LED_5G) ||
		    (which == LED_52G)) { /* non GPIO LED */
			do_led_nongpio(model, which, mode);
		}
		else if (which == LED_BRIDGE) { /* special case: non GPIO LED and turn on second WAN LED (red is GPIO 5) */
			do_led_bridge(mode);
		}
		else if (which == LED_WHITE) { /* WAN LED ; Keep it simple: With Media Bridge ON on any module, disable second WAN LED */
			if (nvram_match("wl0_mode", "psta") ||
			    nvram_match("wl1_mode", "psta") ||
			    nvram_match("wl2_mode", "psta")) {
				b = 255; /* disabled */
			}
		}
		break;
#endif /* TCONFIG_AC5300 */
	case MODEL_RTAC3200:
		b = ac3200[which];
		if ((which == LED_WLAN) ||
		    (which == LED_5G) ||
		    (which == LED_52G)) { /* non GPIO LED */
			do_led_nongpio(model, which, mode);
		}
		else if (which == LED_BRIDGE) { /* non GPIO LED */
			do_led_bridge(mode);
		}
		else if (which == LED_WHITE) { /* WAN LED ; Keep it simple: With Media Bridge ON on any module, disable second WAN LED */
			if (nvram_match("wl0_mode", "psta") ||
			    nvram_match("wl1_mode", "psta") ||
			    nvram_match("wl2_mode", "psta")) {
				b = 255; /* disabled */
			}
		}
		break;
	case MODEL_R8000:
		if (which == LED_DIAG) {
			b = 3; /* color amber gpio 3 (active LOW) */
			c = 2; /* color white gpio 2 (active LOW) */
		}
		else if (which == LED_WHITE) {
			b = -8; /* color white gpio 8 (active LOW) */
			c = 9; /* color amber gpio 9 (active HIGH) */
			/*
			 * GPIO | 8 | 9 | WAN leds (logic)
			 * -----------------------------------
			 *      | 1 | 1 | Both OFF
			 * -----------------------------------
			 *      | 1 | 0 | WAN White, Amber OFF
			 * -----------------------------------
			 *      | 0 | 1 | Wan amber, White OFF
			 * -----------------------------------
			 *      | 0 | 0 | WAN amber, White OFF
			 */
		}
		else if (which == LED_BRIDGE) { /* non GPIO LED */
			do_led_bridge(mode);
			b = r8000[which];
		}
		else {
			b = r8000[which];
		}
		break;
#elif defined(CONFIG_BCMWL6A)
#ifdef TCONFIG_BCM714
	case MODEL_RTAC3100:
	case MODEL_RTAC88U:
		b = ac3100[which];
		if ((which == LED_WLAN) ||
		    (which == LED_5G)) { /* non GPIO LED */
			do_led_nongpio(model, which, mode);
		}
		else if (which == LED_BRIDGE) { /* special case: non GPIO LED and turn on second WAN LED (red is GPIO 5) */
			do_led_bridge(mode);
		}
		else if (which == LED_WHITE) { /* WAN LED ; Keep it simple: With Media Bridge ON on any module, disable second WAN LED */
			if (nvram_match("wl0_mode", "psta") ||
			    nvram_match("wl1_mode", "psta")) {
				b = 255; /* disabled */
			}
		}
		break;
#endif /* TCONFIG_BCM714 */
	case MODEL_RTAC67U:
		b = ac67u[which];
		if ((which == LED_WLAN) ||
		    (which == LED_5G)) { /* non GPIO LED */
			do_led_nongpio(model, which, mode);
		}
		else if (which == LED_BRIDGE) { /* non GPIO LED */
			do_led_bridge(mode);
		}
		break;
	case MODEL_DSLAC68U:
		b = dslac68u[which];
		if ((which == LED_WLAN) ||
		    (which == LED_5G)) { /* non GPIO LED */
			do_led_nongpio(model, which, mode);
		}
		else if (which == LED_BRIDGE) { /* non GPIO LED */
			do_led_bridge(mode);
		}
		break;
	case MODEL_RTAC68U:
		b = ac68u[which];
		if ((which == LED_WLAN) ||
		    (which == LED_5G)) { /* non GPIO LED */
			do_led_nongpio(model, which, mode);
		}
		else if (which == LED_BRIDGE) { /* non GPIO LED */
			do_led_bridge(mode);
		}
		break;
	case MODEL_RTAC68UV3:
		b = ac68u_v3[which];
		if ((which == LED_WLAN) ||
		    (which == LED_5G)) { /* non GPIO LED */
			do_led_nongpio(model, which, mode);
		}
		else if (which == LED_BRIDGE) { /* non GPIO LED */
			do_led_bridge(mode);
		}
		break;
	case MODEL_RTAC1900P:
		b = ac1900p[which];
		if ((which == LED_WLAN) ||
		    (which == LED_5G)) { /* non GPIO LED */
			do_led_nongpio(model, which, mode);
		}
		else if (which == LED_BRIDGE) { /* non GPIO LED */
			do_led_bridge(mode);
		}
		break;
	case MODEL_RTAC66U_B1:
		b = ac66u_b1[which];
		if ((which == LED_WLAN) ||
		    (which == LED_5G)) { /* non GPIO LED */
			do_led_nongpio(model, which, mode);
		}
		else if (which == LED_BRIDGE) { /* non GPIO LED */
			do_led_bridge(mode);
		}
		break;
	case MODEL_RTAC56U:
		b = ac56u[which];
		if (which == LED_WLAN) { /* non GPIO LED */
			do_led_nongpio(model, which, mode);
		}
		break;
	case MODEL_RTN18U:
		b = n18u[which];
		if (which == LED_WLAN) { /* non GPIO LED */
			do_led_nongpio(model, which, mode);
		}
		break;
	case MODEL_R6250:
		if (which == LED_DIAG) {
			b = 3; /* color amber gpio 3 (active LOW) */
			c = 2; /* color green gpio 2 (active LOW) */
		}
		else {
			b = r6250[which];
		}
		break;
	case MODEL_AC1450:
		if (which == LED_DIAG) {
			b = 3; /* color amber gpio 3 (active LOW) */
			c = 2; /* color green gpio 2 (active LOW) */
		}
		else {
			b = ac1450[which];
		}
		break;
	case MODEL_R6300v2:
		if (which == LED_DIAG) {
			b = 3; /* color amber gpio 3 (active LOW) */
			c = 2; /* color green gpio 2 (active LOW) */
		}
		else {
			b = r6300v2[which];
		}
		break;
	case MODEL_R6400:
		if (which == LED_DIAG) {
			b = 2; /* color amber gpio 2 (active LOW) */
			c = 1; /* color white gpio 1 (active LOW) */
		}
		else if (which == LED_WHITE) {
			b = 7; /* color white gpio 7 (active LOW) */
			c = 6; /* color amber gpio 6 (active LOW) */
		}
		else if (which == LED_BRIDGE) { /* non GPIO LED */
			do_led_bridge(mode);
			b = r6400[which];
		}
		else {
			b = r6400[which];
		}
		break;
	case MODEL_R6400v2:
		if (which == LED_DIAG) {
			b = 2; /* color amber gpio 2 (active LOW) */
			c = 1; /* color white gpio 1 (active LOW) */
		}
		else if (which == LED_WHITE) {
			b = 7; /* color white gpio 7 (active LOW) */
			c = 6; /* color amber gpio 6 (active LOW) */
		}
		else if (which == LED_BRIDGE) { /* non GPIO LED */
			do_led_bridge(mode);
			b = r6400v2[which];
		}
		else {
			b = r6400v2[which];
		}
		break;
	case MODEL_R6700v1:
		if (which == LED_DIAG) {
			b = 3; /* color amber gpio 3 (active LOW) */
			c = 2; /* color white gpio 2 (active LOW) */
		}
		else if (which == LED_WHITE) {
			b = 9; /* color white gpio 9 (active LOW) */
			c = 8; /* color amber gpio 8 (active LOW) */
		}
		else if (which == LED_BRIDGE) { /* non GPIO LED */
			do_led_bridge(mode);
			b = r6700v1[which];
		}
		else {
			b = r6700v1[which];
		}
		break;
	case MODEL_R6700v3:
		if (which == LED_DIAG) {
			b = 2; /* color amber gpio 2 (active LOW) */
			c = 1; /* color white gpio 1 (active LOW) */
		}
		else if (which == LED_WHITE) {
			b = 7; /* color white gpio 7 (active LOW) */
			c = 6; /* color amber gpio 6 (active LOW) */
		}
		else if (which == LED_BRIDGE) { /* non GPIO LED */
			do_led_bridge(mode);
			b = r6700v3[which];
		}
		else {
			b = r6700v3[which];
		}
		break;
	case MODEL_XR300:
		if (which == LED_DIAG) {
			b = 2; /* color amber gpio 2 (active LOW) */
			c = 1; /* color white gpio 1 (active LOW) */
		}
		else if (which == LED_WHITE) {
			b = 7; /* color white gpio 7 (active LOW) */
			c = 6; /* color amber gpio 6 (active LOW) */
		}
		else if (which == LED_BRIDGE) { /* non GPIO LED */
			do_led_bridge(mode);
			b = xr300[which];
		}
		else {
			b = xr300[which];
		}
		break;
	case MODEL_R6900:
		if (which == LED_DIAG) {
			b = 3; /* color amber gpio 3 (active LOW) */
			c = 2; /* color white gpio 2 (active LOW) */
		}
		else if (which == LED_WHITE) {
			b = 9; /* color white gpio 9 (active LOW) */
			c = 8; /* color amber gpio 8 (active LOW) */
		}
		else if (which == LED_BRIDGE) { /* non GPIO LED */
			do_led_bridge(mode);
			b = r6900[which];
		}
		else {
			b = r6900[which];
		}
		break;
	case MODEL_R7000:
		if (which == LED_DIAG) {
			b = 3; /* color amber gpio 3 (active LOW) */
			c = 2; /* color white gpio 2 (active LOW) */
		}
		else if (which == LED_WHITE) {
			b = 9; /* color white gpio 9 (active LOW) */
			c = 8; /* color amber gpio 8 (active LOW) */
		}
		else if (which == LED_BRIDGE) { /* non GPIO LED */
			do_led_bridge(mode);
			b = r7000[which];
		}
		else {
			b = r7000[which];
		}
		break;
	case MODEL_AC15:
		b = ac15[which];
		if (which == LED_WLAN) { /* non GPIO LED */
			do_led_nongpio(model, which, mode);
		}
		else if (which == LED_BRIDGE) { /* non GPIO LED */
			do_led_bridge(mode);
		}
		break;
	case MODEL_AC18:
		b = ac18[which];
		if (which == LED_WLAN) { /* non GPIO LED */
			do_led_nongpio(model, which, mode);
		}
		else if (which == LED_BRIDGE) { /* non GPIO LED */
			do_led_bridge(mode);
		}
		break;
	case MODEL_F9K1113v2_20X0:
	case MODEL_F9K1113v2:
		if (which == LED_WHITE) {
			b = 12; /* color blue gpio 12 (active LOW) */
			c = 13; /* color orange gpio 13 (active LOW) */
		}
		else {
			b = f9k1113v2[which];
		}
		break;
	case MODEL_DIR868L:
		if (which == LED_DIAG) {
			b = 0; /* color amber gpio 0 (active LOW) */
			c = 2; /* color green gpio 2 (active LOW) */
		}
		else if (which == LED_WHITE) {
			b = 3; /* color green gpio 3 (active LOW) */
			c = 1; /* color amber gpio 1 (active LOW) */
		}
		else {
			b = dir868[which];
		}
		break;
	case MODEL_WS880:
		b = ws880[which];
		break;
	case MODEL_R1D:
		if (which == LED_WHITE) {
			b = 3; /* color blue gpio 3 (active LOW) */
			c = 2; /* color orange gpio 2 (active LOW) */
		}
		else {
			b = r1d[which];
		}
		break;
	case MODEL_EA6350v1:
	case MODEL_EA6350v2:
		b = ea6350v1[which];
		if (which == LED_BRIDGE) { /* non GPIO LED */
			do_led_bridge(mode);
		}
		break;
	case MODEL_EA6400:
		b = ea6400[which];
		if (which == LED_BRIDGE) { /* non GPIO LED */
			do_led_bridge(mode);
		}
		break;
	case MODEL_EA6700:
		if (strstr(nvram_safe_get("modelNumber"), "EA6500") != NULL) { /* check for ea6500v2 --> same boardtype/num/rev like EA6700! */
			b = ea6500v2[which];
			if (which == LED_BRIDGE) { /* non GPIO LED */
				do_led_bridge(mode);
			}
		}
		else {
			b = ea6700[which];
			if (which == LED_BRIDGE) { /* non GPIO LED */
				do_led_bridge(mode);
			}
		}
		break;
	case MODEL_EA6900:
		b = ea6900[which];
		if (which == LED_BRIDGE) { /* non GPIO LED */
			do_led_bridge(mode);
		}
		break;
	case MODEL_WZR1750:
		/* tbd.: no support yet for 8-Bit Shift Registers at arm branch */
		b = 255; /* disabled */
		c = 255;
#if 0 /* tbd. 8-Bit Shift Registers at arm branch M_ars */
		if (which == LED_DIAG) {
			b = -1; /* color red gpio 1 (active HIGH) */
			c = 2; /* color white gpio 2 (active HIGH, inverted) */
		}
		else if (which == LED_AOSS) {
			b = -3; /* color blue gpio 3 (active HIGH) */
			c = 4; /* color amber gpio 4 (active HIGH, inverted) */
		}
		else {
			b = wzr1750[which];
		}
#endif  /* tbd. 8-Bit Shift Registers at arm branch M_ars */
		break;
#endif /* TCONFIG_AC3200 */
	default:
		sprintf(s, "led_%s", led_names[which]);
		if (nvget_gpio(s, &b, &n)) {
			if ((mode != LED_PROBE) && (n)) mode = !mode;
			ret = (n) ? b : ((b) ? -b : -99);
			goto SET;
		}
		return ret;
	}

	ret = b;
	if (b < TOMATO_GPIO_MIN) {
		if (b == -99)
			b = TOMATO_GPIO_MIN; /* -0 substitute */
		else
			b = -b;
	}
	else if (mode != LED_PROBE) {
		mode = !mode;
	}

SET:
	if (b <= TOMATO_GPIO_MAX) {
		if (mode != LED_PROBE) {
			gpio_write(1 << b, mode);

			if (c < TOMATO_GPIO_MIN) {
				if (c == -99)
					c = TOMATO_GPIO_MIN;
				else
					c = -c;
			}
			else
				mode = !mode;

			if (c <= TOMATO_GPIO_MAX)
				gpio_write(1 << c, mode);
		}
	}

	return ret;
}

void disable_led_wanlan(void)
{
	system("/usr/sbin/et robowr 0x0 0x18 0x0100"); /* turn off all LAN and WAN LEDs Part 1/2 */
	system("/usr/sbin/et robowr 0x0 0x1a 0x0100"); /* turn off all LAN and WAN LEDs Part 2/2 */
}

void enable_led_wanlan(void)
{
	system("/usr/sbin/et robowr 0x0 0x18 0x01ff"); /* turn on all LAN and WAN LEDs Part 1/2 */
	system("/usr/sbin/et robowr 0x0 0x1a 0x01ff"); /* turn on all LAN and WAN LEDs Part 2/2 */
}

void do_led_bridge(int mode)
{
	if (mode == LED_ON) {
		enable_led_wanlan();
	}
	else if (mode == LED_OFF) {
		disable_led_wanlan();
	}
	else if (mode == LED_PROBE) {
		return;
	}
}

void led_setup(void)
{
	int model;

	/* get router model */
	model = get_model();

	/* stealth mode on ? */
	if (nvram_match("stealth_mode", "1")) {

		/* the following router do have LEDs for WLAN, WAN and LAN - see at the ethernet connectors or at the front panel / case */
		/* turn off non GPIO LEDs and some special cases like power LED - - do_led(...) will take care of the other ones */
		switch (model) {
#ifdef TCONFIG_AC3200
#ifdef TCONFIG_AC5300
		case MODEL_RTAC5300:
			set_gpio(GPIO_03, T_HIGH); /* disable power led */
			set_gpio(GPIO_04, T_LOW); /* disable button led */
			disable_led_wanlan();
			break;
#endif /* TCONFIG_AC5300 */
		case MODEL_R8000:
			set_gpio(GPIO_03, T_HIGH); /* disable power led color amber */
			disable_led_wanlan();
			break;
		case MODEL_RTAC3200:
			set_gpio(GPIO_03, T_HIGH); /* disable power led */
			set_gpio(GPIO_15, T_LOW); /* disable button led */
			disable_led_wanlan();
			break;
#elif defined(CONFIG_BCMWL6A)
#ifdef TCONFIG_BCM714
		case MODEL_RTAC3100:
		case MODEL_RTAC88U:
			set_gpio(GPIO_03, T_HIGH); /* disable power led */
			set_gpio(GPIO_04, T_LOW); /* disable button led */
			disable_led_wanlan();
			break;
#endif /* TCONFIG_BCM714 */
		case MODEL_DIR868L:
			set_gpio(GPIO_00, T_HIGH); /* disable power led color amber */
			break;
		case MODEL_AC15:
			set_gpio(GPIO_00, T_LOW); /* disable sys led */
			disable_led_wanlan();
			break;
		case MODEL_AC18:
			set_gpio(GPIO_00, T_LOW); /* disable sys led */
			disable_led_wanlan();
			break;
		case MODEL_F9K1113v2_20X0:
		case MODEL_F9K1113v2:
			set_gpio(GPIO_12, T_HIGH); /* disable sys led */
			set_gpio(GPIO_15, T_HIGH); /* disable wps led */
			break;
    		case MODEL_AC1450:
		case MODEL_R6250:
		case MODEL_R6300v2:
			set_gpio(GPIO_03, T_HIGH); /* disable power led color amber */
			break;
		case MODEL_R6400:
		case MODEL_R6400v2:
		case MODEL_R6700v3:
		case MODEL_XR300:
			set_gpio(GPIO_02, T_HIGH); /* disable power led color amber */
			disable_led_wanlan();
			break;
		case MODEL_R6700v1:
		case MODEL_R6900:
		case MODEL_R7000:
			set_gpio(GPIO_03, T_HIGH); /* disable power led color amber */
			disable_led_wanlan();
			break;
		case MODEL_RTN18U:
			set_gpio(GPIO_00, T_HIGH); /* disable power led color blue */
			break;
		case MODEL_RTAC56U:
			set_gpio(GPIO_03, T_HIGH); /* disable power led color blue */
			disable_led_wanlan();
			break;
		case MODEL_RTAC66U_B1:
		case MODEL_RTAC67U:
			set_gpio(GPIO_00, T_HIGH); /* disable power led */
			disable_led_wanlan();
			break;
		case MODEL_DSLAC68U:
		case MODEL_RTAC68U:
		case MODEL_RTAC68UV3:
		case MODEL_RTAC1900P:
			set_gpio(GPIO_03, T_HIGH); /* disable power led */
			set_gpio(GPIO_04, T_HIGH); /* disable asus logo led */
			disable_led_wanlan();
			break;
		case MODEL_EA6400:
		case MODEL_EA6900:
			set_gpio(GPIO_08, T_LOW); /* disable LOGO led */
			disable_led_wanlan();
			break;
		case MODEL_EA6700:
			if (strstr(nvram_safe_get("modelNumber"), "EA6500") != NULL) { /* check for ea6500v2 --> same boardtype/num/rev like EA6700! */
				set_gpio(GPIO_06, T_HIGH); /* disable LOGO led for EA6500 */
			}
			else {
				set_gpio(GPIO_08, T_LOW); /* disable LOGO led for EA6700 */
			}
			disable_led_wanlan();
			break;
		case MODEL_R1D:
			set_gpio(GPIO_01, T_HIGH); /* disable red */
			set_gpio(GPIO_02, T_HIGH); /* disable orange */
			set_gpio(GPIO_03, T_HIGH); /* disable blue */
			break;
		case MODEL_WZR1750:
#if 0 /* tbd. 8-Bit Shift Registers at arm branch M_ars */
			set_gpio(GPIO_01, T_LOW); /* disable power led color red */
#endif /* tbd. 8-Bit Shift Registers at arm branch M_ars */
			break;
#endif /* TCONFIG_AC3200 */
		default:
			/* nothing to do right now */
			break;
		}
	}
	else {
		/* LED setup/config/preparation for some router models */
		switch (model) {
#ifdef TCONFIG_AC3200
		case MODEL_R8000:
			/* activate WAN port led */
			set_gpio(GPIO_08, T_HIGH);
			set_gpio(GPIO_09, T_LOW); /* R8000: enable LED_WHITE / WAN LED with color amber (GPIO 9, active LOW) if ethernet cable is connected; switch to color white (GPIO 8, active HIGH) with WAN up */
			break;
#elif defined(CONFIG_BCMWL6A)
		case MODEL_DIR868L:
			/* activate WAN port led */
			set_gpio(GPIO_01, T_LOW); /* DIR868L: enable LED_WHITE / WAN LED with color amber (1); switch to color green (3) with WAN up */
			break;
		case MODEL_RTAC56U:
			set_gpio(GPIO_04, T_LOW); /* enable power supply for all LEDs, except for PowerLED */
			break;
		case MODEL_R6400:
		case MODEL_R6400v2:
		case MODEL_R6700v3:
		case MODEL_XR300:
			/* activate WAN port led */
			set_gpio(GPIO_06, T_HIGH); /* R6400: enable LED_WHITE / WAN LED with color amber (6) if ethernet cable is connected; switch to color white (7) with WAN up */
			set_gpio(GPIO_07, T_LOW);
			break;
		case MODEL_R6700v1:
		case MODEL_R6900:
		case MODEL_R7000:
			/* activate WAN port led */
			set_gpio(GPIO_08, T_HIGH); /* R6700v1, R6900 and R7000: enable LED_WHITE / WAN LED with color amber (8) if ethernet cable is connected; switch to color white (9) with WAN up */
			set_gpio(GPIO_09, T_LOW);
			break;
#endif /* TCONFIG_AC3200 */
		default:
			/* nothing to do right now */
			break;
		}
	}
}

/* control non GPIO LEDs for some Asus/Tenda Router: AC15, AC18, RT-N18U, RT-AC56U, RT-AC66U_B1, RT-AC67U, RT-AC68U (V3), DSL-AC68U, RT-AC1900P, RT-AC3200 */
void do_led_nongpio(int model, int which, int mode)
{
	switch (model) {
#ifdef TCONFIG_AC3200
#ifdef TCONFIG_AC5300
	case MODEL_RTAC5300:
		if (which == LED_WLAN) {
			if (mode == LED_ON) system("/usr/sbin/wl -i eth1 ledbh 9 1"); /* 2.4 GHz - eth1, see Asus SRC */
			else if (mode == LED_OFF) system("/usr/sbin/wl -i eth1 ledbh 9 0");
			else if (mode == LED_PROBE) return;
		}
		else if (which == LED_5G) {
			if (mode == LED_ON) system("/usr/sbin/wl -i eth2 ledbh 9 1"); /* 5 GHz - eth2, see Asus SRC */
			else if (mode == LED_OFF) system("/usr/sbin/wl -i eth2 ledbh 9 0");
			else if (mode == LED_PROBE) return;
		}
		else if (which == LED_52G) {
			if (mode == LED_ON) system("/usr/sbin/wl -i eth3 ledbh 9 1"); /* second 5 GHz - eth3, see Asus SRC */
			else if (mode == LED_OFF) system("/usr/sbin/wl -i eth3 ledbh 9 0");
			else if (mode == LED_PROBE) return;
		}
		break;
#endif /* TCONFIG_AC5300 */
	case MODEL_RTAC3200:
		if (which == LED_WLAN) {
			if (mode == LED_ON) system("/usr/sbin/wl -i eth2 ledbh 10 1"); /* 2.4 GHz - eth2, see Asus SRC */
			else if (mode == LED_OFF) system("/usr/sbin/wl -i eth2 ledbh 10 0");
			else if (mode == LED_PROBE) return;
		}
		else if (which == LED_5G) {
			if (mode == LED_ON) system("/usr/sbin/wl -i eth1 ledbh 10 1"); /* 5 GHz - eth1, see Asus SRC */
			else if (mode == LED_OFF) system("/usr/sbin/wl -i eth1 ledbh 10 0");
			else if (mode == LED_PROBE) return;
		}
		else if (which == LED_52G) {
			if (mode == LED_ON) system("/usr/sbin/wl -i eth3 ledbh 10 1"); /* second 5 GHz - eth3, see Asus SRC */
			else if (mode == LED_OFF) system("/usr/sbin/wl -i eth3 ledbh 10 0");
			else if (mode == LED_PROBE) return;
		}
		break;
#elif defined(CONFIG_BCMWL6A)
#ifdef TCONFIG_BCM714
	case MODEL_RTAC3100:
	case MODEL_RTAC88U:
		if (which == LED_WLAN) {
			if (mode == LED_ON) system("/usr/sbin/wl -i eth1 ledbh 9 1"); /* 2.4 GHz - eth1, see Asus SRC */
			else if (mode == LED_OFF) system("/usr/sbin/wl -i eth1 ledbh 9 0");
			else if (mode == LED_PROBE) return;
		}
		else if (which == LED_5G) {
			if (mode == LED_ON) system("/usr/sbin/wl -i eth2 ledbh 9 1"); /* 5 GHz - eth2, see Asus SRC */
			else if (mode == LED_OFF) system("/usr/sbin/wl -i eth2 ledbh 9 0");
			else if (mode == LED_PROBE) return;
		}
		break;
#endif /* TCONFIG_BCM714 */
	case MODEL_AC15:
	case MODEL_AC18:
	case MODEL_RTN18U:
		if (which == LED_WLAN) {
			if (mode == LED_ON) system("/usr/sbin/wl -i eth1 ledbh 10 7");
			else if (mode == LED_OFF) system("/usr/sbin/wl -i eth1 ledbh 10 0");
			else if (mode == LED_PROBE) return;
		}
		break;
	case MODEL_RTAC56U:
		if (which == LED_WLAN) {
			if (mode == LED_ON) system("/usr/sbin/wl -i eth1 ledbh 3 1");
			else if (mode == LED_OFF) system("/usr/sbin/wl -i eth1 ledbh 3 0");
			else if (mode == LED_PROBE) return;
		}
		break;
	case MODEL_RTAC66U_B1:
	case MODEL_RTAC67U:
	case MODEL_DSLAC68U:
	case MODEL_RTAC68U:
	case MODEL_RTAC68UV3:
	case MODEL_RTAC1900P:
		if (which == LED_WLAN) {
			if (mode == LED_ON) system("/usr/sbin/wl -i eth1 ledbh 10 1");
			else if (mode == LED_OFF) system("/usr/sbin/wl -i eth1 ledbh 10 0");
			else if (mode == LED_PROBE) return;
		}
		else if (which == LED_5G) {
			if (mode == LED_ON) system("/usr/sbin/wl -i eth2 ledbh 10 1");
			else if (mode == LED_OFF) system("/usr/sbin/wl -i eth2 ledbh 10 0");
			else if (mode == LED_PROBE) return;
		}
		break;
#endif /* TCONFIG_AC3200 */
	default:
		/* nothing to do right now */
		break;
	}

}
