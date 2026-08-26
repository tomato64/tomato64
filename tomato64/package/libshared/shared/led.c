/*
 *
 * Tomato Firmware
 * Copyright (C) 2006-2009 Jonathan Zarate
 *
 * Fixes/updates (C) 2018 - 2026 pedro
 * https://freshtomato.org/
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

#include "shutils.h"
#include "shared.h"

/* needed by logmsg() */
#define LOGMSG_DISABLE	DISABLE_SYSLOG_OS
#define LOGMSG_NVDEBUG	"led_debug"

#ifdef TCONFIG_BCMARM
 #ifdef TCONFIG_AC3200
const char *led_names[] = {"wlan", "diag", "white", "amber", "dmz", "aoss", "bridge", "usb", "usb3", "5g", "52g"};
 #else
const char *led_names[] = {"wlan", "diag", "white", "amber", "dmz", "aoss", "bridge", "usb", "usb3", "5g"};
 #endif
#elif defined(TCONFIG_RTNPLUS)
const char *led_names[] = {"wlan", "diag", "white", "amber", "dmz", "aoss", "bridge", "usb", "5g"};
#else
const char *led_names[] = {"wlan", "diag", "white", "amber", "dmz", "aoss", "bridge", "usb"};
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

#ifdef TCONFIG_BCMARM
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
#endif /* TCONFIG_BCMARM */

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

#if !defined(TCONFIG_BCMARM) && defined(TCONFIG_RTNPLUS)
/* Routine to write to shift register
 * Note that the controls are active low, but input as high = on
 */
void gpio_write_shiftregister(unsigned int led_status, int clk, int data, int max_shifts)
{
	int i;

	gpio_write(1 << data, 1);	/* set data to 1 to start (disable) */
	gpio_write(1 << clk, 0);	/* and clear clock ... */
	for (i = max_shifts; i >= 0; i--) {
		if (led_status & (1 << i))
			gpio_write(1 << data, 0);	/* on, pull low (active low) */
		else
			gpio_write(1 << data, 1);	/* off, pull high (active low) */

		gpio_write(1 << clk, 1);	/* pull high to trigger */
		gpio_write(1 << clk, 0);	/* reset to low -> finish clock cycle*/
	}
}

/* strBits:  convert binary value to string (binary file representation) */
char strConvert[33];
char * strBits(int input, int binarySize)
{

	int i;
	if (binarySize > 0) {
		if (binarySize > 32)
			binarySize = 32;

		for(i = 0; i < binarySize ; i++) {
			if (input & (1 << ((binarySize-1)-i)))
				strConvert[i] = '1';
			else
				strConvert[i] = '0';
		}

		strConvert[binarySize] = '\0';
		return (char *)strConvert;
	} else
		return (char *)NULL;
}

void led_bit(int b, int mode)
{
	FILE *fileExtGPIOstatus;		/* For WNDR4000, keep track of extended bit status (shift register), as cannot read from HW! */
	unsigned int intExtendedLEDStatus;	/* Status of Extended LED's (shift register on WNDR4000) ... and WNDR3700v3, it's the same! */
	if ((mode == LED_ON) || (mode == LED_OFF)) {
		if (b < 16) {
			/* Read bit-mask from file, for tracking / updates (as this process is called clean each LED update, so cannot use static variable!) */
			if ((fileExtGPIOstatus = fopen("/tmp/.ext_led_value", "rb"))) {
				fscanf(fileExtGPIOstatus, "Shift Register Status: 0x%x\n", &intExtendedLEDStatus);
				fclose(fileExtGPIOstatus);
			} else {
				/* Read Error (tracking file) - set all LED's to off */
				intExtendedLEDStatus = 0x00;
			}
			if (mode == LED_ON) {
				/* Bitwise OR, turn corresponding bit on */
				intExtendedLEDStatus |= (1 << b);
			} else {
				/* Bitwise AND, with bitwise inverted shift ... so turn bit off */
				intExtendedLEDStatus &= (~(1 << b));
			}
			/* And write to LEDs (Shift Register) */
			gpio_write_shiftregister(intExtendedLEDStatus, 7, 6, 7);
			/* Write bit-mask to file, for tracking / updates (as this process is called clean each LED update, so cannot use static variable!) */
			if ((fileExtGPIOstatus = fopen("/tmp/.ext_led_value", "wb"))) {
				fprintf(fileExtGPIOstatus, "Shift Register Status: 0x%x\n", intExtendedLEDStatus);
				fprintf(fileExtGPIOstatus, "Shift Register Status: 0b%s\n", strBits(intExtendedLEDStatus, 8));
				fclose(fileExtGPIOstatus);
			}
		}
	}
}
#endif /* !TCONFIG_BCMARM && TCONFIG_RTNPLUS */

#ifdef TCONFIG_BCMARM
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
	static int r6200v2[]	= {  11,    3,    15,  255,  255,   -1,  255,    8,  255,  255};
	static int r6250[]	= {  11,    3,    15,  255,  255,   -1,  255,    8,  255,  255};
	static int r6300v2[]	= {  11,    3,    10,  255,  255,   -1,  255,    8,  255,  255};
	static int r6400[]	= {   9,    2,     7,  255,  -10,  -11,  254,   12,   13,    8};
	static int r6400v2[]	= {   9,    2,     7,  255,  -10,  -11,  254,   12,   13,    8};
	static int r6700v1[]	= {  13,    3,     9,  255,  -14,  -15,  254,   18,   17,   12};
	static int r6700v3[]	= {   9,    2,     7,  255,  -10,  -11,  254,   12,   13,    8};
	static int r6900[]	= {  13,    3,     9,  255,  -14,  -15,  254,   18,   17,   12};
	static int xr300[]	= {   9,    2,     7,  255,  -10,  -11,  254,   12,   13,    8};
	static int r7000[]	= {  13,    3,     9,  255,  -14,  -15,  254,   18,   17,   12};
	static int ex7000[]	= {   8,  255,    12,  255,  255,   -1,  254,    5,  255,   10};
	static int ex6200[]	= {  12,  255,     8,  255,  255,   -1,  254,    5,  255,   10};
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
	case MODEL_R6200v2:
		if (which == LED_DIAG) {
			b = 3; /* color amber gpio 3 (active LOW) */
			c = 2; /* color green gpio 2 (active LOW) */
		}
		else {
			b = r6200v2[which];
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
	case MODEL_EX7000:
		if (which == LED_WHITE) {
			b = 12; /* color green gpio 12 (active LOW) */
			c = 13; /* color red gpio 13 (active LOW) */
		}
		else if (which == LED_BRIDGE) { /* non GPIO LED */
			do_led_bridge(mode);
			b = ex7000[which];
		}
		else {
			b = ex7000[which];
		}
		break;
	case MODEL_EX6200:
		if (which == LED_WHITE) {
			b = 8; /* color green gpio 8 (active LOW) */
			c = 9; /* color red gpio 9 (active LOW) */
		}
		else if (which == LED_BRIDGE) { /* non GPIO LED */
			do_led_bridge(mode);
			b = ex6200[which];
		}
		else {
			b = ex6200[which];
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
#else /* MIPS */
int do_led(int which, int mode)
{
  /*
   * valid GPIO values: 0 to 31 (default active LOW, inverted or active HIGH with -[value])
   * value 255: not known / disabled / not possible
   * value -99: special case for -0 substitute (active HIGH for GPIO 0)
   * value 254: non GPIO LED (special case, to show there is something!)
   */
#ifdef TCONFIG_RTNPLUS
//				    WLAN  DIAG  WHITE AMBER  DMZ  AOSS  BRIDGE MYST/USB 5G
//				    ----- ----- ----- -----  ---  ----  ------ -----    --
	static int wrt54g[]	= { 255,    1,     2,    3,    7,  255,  255,  255,    255};
	static int wrtsl[]	= { 255,    1,     5,    7,    0,  255,  255,  255,    255};
	static int whrg54[]	= {   2,    7,   255,  255,  255,    6,    1,    3,    255};
	static int wbr2g54[]	= { 255,   -1,   255,  255,  255,   -6,  255,  255,    255};
	static int wzrg54[]	= {   2,    7,   255,  255,  255,    6,  255,  255,    255};
	static int wr850g1[]	= {   7,    3,   255,  255,  255,  255,  255,  255,    255};
	static int wr850g2[]	= {   0,    1,   255,  255,  255,  255,  255,  255,    255};
	static int wtr54gs[]	= {   1,   -1,   255,  255,  255,  255,  255,  255,    255};
	static int dir320[]	= { -99,    1,     4,    3,  255,  255,  255,   -5,    255};
	static int h618b[]	= { 255,   -1,   255,  255,  255,   -5,   -3,   -4,    255};
	static int wl1600gl[]	= {   1,   -5,     0,  255,  255,    2,  255,  255,    255};
	static int wrt310nv1[]	= { 255,    1,     9,    3,  255,  255,  255,  255,    255};
	static int wrt160nv1[]	= { 255,    1,     5,    3,  255,  255,  255,  255,    255};
#ifdef CONFIG_BCMWL5
	static int wnr3500[]	= { 255,  255,    -2,  255,  255,   -1,  255,  255,    255};
	static int wnr2000v2[]	= { 255,  255,   255,  255,  255,   -7,  255,  255,    255};
	static int wndr4000[]	= {   3,    1,     0,    1,  255,    6,  255,    5,      4};
	static int wndr3400[]	= {  -9,   -7,    -3,   -7,  255,  255,  255,    2,    -99}; /* Note: 5 = Switch, 4 = Reset button, 8 = SES button */
	static int wndr3400v2[]	= {  17,   14,   255,  255,  255,  -22,  255,   20,     18}; /* Note: 12 = Reset button, 23 = WPS button */
	static int wndr3400v3[]	= { -17,   14,   255,  255,  255,  -22,  255,  -20,    -18}; /* Note: 12 = Reset button, 23 = WPS button */
	static int f7d[]	= { 255,  255,   255,  255,   12,   13,  255,   14,    255};
	static int f9k[]	= { 255,    1,     0,  255,  255,  255,  255,  255,    255};
	static int wrt160nv3[]	= { 255,    1,     4,    2,  255,  255,  255,  255,    255};
	static int e900[]	= { 255,   -6,     8,  255,  255,  255,  255,  255,    255};
	static int e1000v2[]	= { 255,   -6,     8,    7,  255,  255,  255,  255,    255};
	static int e3200[]	= { 255,   -3,   255,  255,  255,  255,  255,  255,    255};
	static int wrt320n[]	= { 255,    2,     3,    4,  255,  255,  255,  255,    255};
	static int wrt610nv2[]	= { 255,    5,     3,    0,  255,  255,  255,   -7,    255};
	static int e4200[]	= { 255,    3,     5,  255,  255,  255,  255,  255,    255};
	static int rtn10u[]	= { 255,  255,   255,  255,  255,   -7,  255,   -8,    255};
	static int rtn10p[]	= { 255,   -6,   255,  255,  255,   -7,  255,  255,    255};
	static int rtn12a1[]	= { 255,  255,   255,  255,  255,   -2,  255,  225,    255};
	static int rtn12b1[]	= {  -5,  255,     4,  255,  255,  -18,  255,  225,    255};
	static int rtn12c1[]	= {  -4,  255,     5,  255,  255,  -18,  255,  225,    255};
	static int rtn12d1[]	= {  -5,  255,   255,  255,  255,  -18,  255,  225,    255};
	static int rtn15u[]	= {  -1,  255,     3,  255,  255,   -6,    4,   -9,    255};
	static int rtn53[]	= {   0,  -17,   255,  255,  255,  255,  255,  255,    255};
	static int l600n[]	= { 255,  255,   255,  255,  255,   -7,  255,   -8,    255};
	static int dir620c1[]	= {  -6,   -8,   255,  255,  255,   -7,  255,  255,    255};
	static int rtn66u[]	= { 255,  -12,   255,  255,  255,  255,  255,   15,     13};
	static int w1800r[]	= { 255,  -13,   255,  255,  255,  255,  255,  -12,     -5};
	static int d1800h[]	= { -12,  -13,     8,  255,  255,  -10,  255,   15,     11};
	static int tdn6[]	= { 255,   -6,     8,  255,  255,  255,  255,  255,    255};
	static int tdn60[]	= { 255,   -6,   255,  255,  255,  255,  255,    9,    255};
	static int dir865l[]	= { 255,  -99,     2,    1,  255,    3,  255,  255,    255};
	static int r6300v1[]	= {  11,    3,   255,  255,  255,  255,  255,    8,     11};
	static int wndr4500[]	= {   9,    3,     2,    3,  255,  255,  255,   14,     11};
#endif
//				    ----- ----- ----- -----  ---  ----  ------ -----    --
//				    WLAN  DIAG  WHITE AMBER  DMZ  AOSS  BRIDGE MYST/USB 5G
#else
//				    WLAN  DIAG  WHITE AMBER  DMZ  AOSS  BRIDGE MYST/USB
//				    ----- ----- ----- -----  ---  ----  ------ -----
	static int wrt54g[]	= { 255,    1,     2,    3,    7,  255,  255,  255};
	static int wrtsl[]	= { 255,    1,     5,    7,    0,  255,  255,  255};
	static int whrg54[]	= {   2,    7,   255,  255,  255,    6,    1,    3};
	static int wbr2g54[]	= { 255,   -1,   255,  255,  255,   -6,  255,  255};
	static int wzrg54[]	= {   2,    7,   255,  255,  255,    6,  255,  255};
	static int wr850g1[]	= {   7,    3,   255,  255,  255,  255,  255,  255};
	static int wr850g2[]	= {   0,    1,   255,  255,  255,  255,  255,  255};
	static int wtr54gs[]	= {   1,   -1,   255,  255,  255,  255,  255,  255};
	static int dir320[]	= { -99,    1,     4,    3,  255,  255,  255,   -5};
	static int h618b[]	= { 255,   -1,   255,  255,  255,   -5,   -3,   -4};
	static int wl1600gl[]	= {   1,   -5,     0,  255,  255,    2,  255,  255};
	static int wrt310nv1[]	= { 255,    1,     9,    3,  255,  255,  255,  255};
	static int wrt160nv1[]	= { 255,    1,     5,    3,  255,  255,  255,  255};
#ifdef CONFIG_BCMWL5
	static int wnr3500[]	= { 255,  255,     2,  255,  255,   -1,  255,  255};
	static int wnr2000v2[]	= { 255,  255,   255,  255,  255,   -7,  255,  255};
	static int f7d[]	= { 255,  255,   255,  255,   12,   13,  255,   14};
	static int wrt160nv3[]	= { 255,    1,     4,    2,  255,  255,  255,  255};
	static int wrt320n[]	= { 255,    2,     3,    4,  255,  255,  255,  255};
	static int wrt610nv2[]	= { 255,    5,     3,    0,  255,  255,  255,   -7};
	static int e4200[]	= { 255,    3,     5,  255,  255,  255,  255,  255};
#endif
//				    ----- ----- ----- -----  ---  ----  ------ -----
//				    WLAN  DIAG  WHITE AMBER  DMZ  AOSS  BRIDGE MYST/USB
#endif /* TCONFIG_RTNPLUS */

	char s[16];
	int n;
	int b = 255, c = 255;
	int ret = 255;
	static int model = 0; /* initialize with 0 / MODEL_UNKNOWN */

	if ((which < 0) || (which >= LED_COUNT)) return ret;

	if (model == 0) { /* router model unknown OR detect router model for the first time at function do_led(). */
		/* get router model */
		model = get_model();
	}

	switch (nvram_match("led_override", "1") ? MODEL_UNKNOWN : model) {
	case MODEL_WRT54G:
		if (check_hw_type() == HW_BCM4702) {
			/* G v1.x */
			if ((which != LED_DIAG) && (which != LED_DMZ)) return ret;
			b = (which == LED_DMZ) ? 1 : 4;
			if (mode != LED_PROBE) {
				if (f_read_string("/proc/sys/diag", s, sizeof(s)) > 0) {
					n = atoi(s);
					sprintf(s, "%u", mode ? (n | b) : (n & ~b));
					f_write_string("/proc/sys/diag", s, 0, 0);
				}
			}
			return b;
		}
		switch (which) {
		case LED_AMBER:
		case LED_WHITE:
			if (!supports(SUP_WHAM_LED)) return ret;
			break;
		}
		b = wrt54g[which];
		break;
	case MODEL_WTR54GS:
		b = wtr54gs[which];
		break;
	case MODEL_WRTSL54GS:
		b = wrtsl[which];
		break;
	case MODEL_WHRG54S:
	case MODEL_WHRHPG54:
	case MODEL_WHRG125:
		b = whrg54[which];
		break;
	case MODEL_WZRG54:
	case MODEL_WZRHPG54:
	case MODEL_WZRRSG54:
	case MODEL_WZRRSG54HP:
	case MODEL_WVRG54NF:
	case MODEL_WHR2A54G54:
	case MODEL_WHR3AG54:
	case MODEL_WZRG108:
		b = wzrg54[which];
		break;
/*
	case MODEL_WHR2A54G54:
		if (which != LED_DIAG) return ret;
		b = 7;
		break;
*/
	case MODEL_WBRG54:
		if (which != LED_DIAG) return ret;
		b = 7;
		break;
	case MODEL_WBR2G54:
		b = wbr2g54[which];
		break;
	case MODEL_WR850GV1:
		b = wr850g1[which];
		break;
	case MODEL_WR850GV2:
	case MODEL_WR100:
		b = wr850g2[which];
		break;
	case MODEL_WL500GP:
		if (which != LED_DIAG) return ret;
		b = -1;	/* power light */
		break;
	case MODEL_WL500W:
		if (which != LED_DIAG) return ret;
		b = -5;	/* power light */
		break;
	case MODEL_DIR320:
		b = dir320[which];
		break;
	case MODEL_H618B:
		b = h618b[which];
		break;
	case MODEL_WL1600GL:
		b = wl1600gl[which];
		break;
	case MODEL_WL500GPv2:
	case MODEL_WL500GD:
	case MODEL_WL520GU:
	case MODEL_WL330GE:
		if (which != LED_DIAG) return ret;
		b = -99;	/* Invert power light as diag indicator */
		break;
#ifdef CONFIG_BCMWL5
#ifdef TCONFIG_RTNPLUS
	case MODEL_RTN10:
	case MODEL_RTN16:
		if (which != LED_DIAG) return ret;
		b = -1;	/* power light */
		break;
	case MODEL_RTN10U:
		b = rtn10u[which];
		break;
	case MODEL_RTN10P:
		b = rtn10p[which];
		break;
	case MODEL_RTN12A1:
		b = rtn12a1[which];
		break;
	case MODEL_RTN12B1:
	case MODEL_RTN12HP:
		b = rtn12b1[which];
		break;
	case MODEL_RTN12C1:
		b = rtn12c1[which];
		break;
	case MODEL_RTN12D1:
	case MODEL_RTN12VP:
		b = rtn12d1[which];
		break;
	case MODEL_RTN15U:
		b = rtn15u[which];
		break;
	case MODEL_RTN53:
	case MODEL_RTN53A1:
		b = rtn53[which];
		break;
	case MODEL_RTN66U:
		b = rtn66u[which];
		break;
	case MODEL_DIR865L:
		b = dir865l[which];
		break;
	case MODEL_W1800R:
	case MODEL_TDN80:
		b = w1800r[which];
		break;
	case MODEL_D1800H:
		if (which == LED_DIAG) {
			/* power led gpio: 0x02 - white, 0x13 - red */
			b = (mode) ? 13 : 2;
			c = (mode) ? 2 : 13;
		} else
			b = d1800h[which];
		break;
	case MODEL_WNR3500L:
	case MODEL_WNR3500LV2:
		if (which == LED_DIAG) {
			b = 3; /* gpio 3 actice HIGH AND gpio 7 active LOW  ==> result: Power LED on green; for amber --> inverted */
			c = 7;
		} else
			b = wnr3500[which];
		break;
	case MODEL_WNDR4500:
	case MODEL_WNDR4500V2:
		if (which == LED_DIAG) {
			/* power led gpio: 0x102 - green, 0x103 - amber */
			b = (mode) ? 3 : -2;
			c = (mode) ? -2 : 3;
		} else {
			b = wndr4500[which];
		}
		break;
	case MODEL_R6300V1:
		b = r6300v1[which];
		break;
	case MODEL_WNR2000v2:
		if (which == LED_DIAG) {
			/* power led gpio: 0x01 - green, 0x02 - amber */
			b = (mode) ? 2 : 1;
			c = (mode) ? 1 : 2;
		} else
			b = wnr2000v2[which];
		break;
	case MODEL_WNDR4000:
	case MODEL_WNDR3700v3:
		/* Special Case, shift register control ... so write accordingly. */
		b = wndr4000[which];
		led_bit(b, mode);
		return b;
		break;
	case MODEL_WNDR3400:
		b = wndr3400[which];
		break;
	case MODEL_WNDR3400v2:
		b = wndr3400v2[which];
		break;
	case MODEL_WNDR3400v3:
		b = wndr3400v3[which];
		break;
	case MODEL_F7D3301:
	case MODEL_F7D3302:
	case MODEL_F7D4301:
	case MODEL_F7D4302:
	case MODEL_F5D8235v3:
		if (which == LED_DIAG) {
			/* power led gpio: 10 - green, 11 - red */
			b = (mode) ? 11 : -10;
			c = (mode) ? -10 : 11;
		} else
			b = f7d[which];
		break;
	case MODEL_F9K1102:
		if (which == LED_AOSS) {
			b = 5; /* color orange gpio 5 (active LOW) */
			c = 4; /* color blue gpio 4 (active LOW) */
		}
		else {
			b = f9k[which];
		}
		break;
	case MODEL_E1000v2:
		b = e1000v2[which];
		break;
	case MODEL_E900:
	case MODEL_E1500:
	case MODEL_E1550:
	case MODEL_E2500:
		b = e900[which];
		break;
	case MODEL_E3200:
		b = e3200[which];
		break;
	case MODEL_WRT160Nv3:
		b = wrt160nv3[which];
		break;
	case MODEL_WRT320N:
		b = wrt320n[which];
		break;
	case MODEL_WRT610Nv2:
		b = wrt610nv2[which];
		break;
	case MODEL_E4200:
		b = e4200[which];
		break;
	case MODEL_L600N:
		b = l600n[which];
		break;
	case MODEL_DIR620C1:
		b = dir620c1[which];
		break;
	case MODEL_TDN60:
		b = tdn60[which];
		break;
	case MODEL_TDN6:
		b = tdn6[which];
		break;
#else
	case MODEL_RTN12:
		if (which != LED_DIAG) return ret;
		b = -2;	/* power light */
		break;
	case MODEL_RTN10:
	case MODEL_RTN16:
		if (which != LED_DIAG) return ret;
		b = -1;	/* power light */
		break;
	case MODEL_WNR3500L:
		if (which == LED_DIAG) {
			/* power led gpio: 0x03 - green, 0x07 - amber */
			b = (mode) ? 7 : 3;
			c = (mode) ? 3 : 7;
		} else
			b = wnr3500[which];
		break;
	case MODEL_WNR2000v2:
		if (which == LED_DIAG) {
			/* power led gpio: 0x01 - green, 0x02 - amber */
			b = (mode) ? 2 : 1;
			c = (mode) ? 1 : 2;
		} else
			b = wnr2000v2[which];
		break;
	case MODEL_F7D3301:
	case MODEL_F7D3302:
	case MODEL_F7D4301:
	case MODEL_F7D4302:
	case MODEL_F5D8235v3:
		if (which == LED_DIAG) {
			/* power led gpio: 10 - green, 11 - red */
			b = (mode) ? 11 : -10;
			c = (mode) ? -10 : 11;
		} else
			b = f7d[which];
		break;
	case MODEL_WRT160Nv3:
		b = wrt160nv3[which];
		break;
	case MODEL_WRT320N:
		b = wrt320n[which];
		break;
	case MODEL_WRT610Nv2:
		b = wrt610nv2[which];
		break;
	case MODEL_E4200:
		b = e4200[which];
		break;
#endif /* TCONFIG_RTNPLUS */
#endif /* CONFIG_BCMWL5 */
/*
	case MODEL_RT390W:
		break;
*/
	case MODEL_MN700:
		if (which != LED_DIAG) return ret;
		b = 6;
		break;
	case MODEL_WLA2G54L:
		if (which != LED_DIAG) return ret;
		b = 1;
		break;
	case MODEL_WRT300N:
		if (which != LED_DIAG) return ret;
		b = 1;
		break;
	case MODEL_WRT310Nv1:
		b = wrt310nv1[which];
		break;
	case MODEL_WRT160Nv1:
		b = wrt160nv1[which];
		break;
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
			b = TOMATO_GPIO_MIN;	/* -0 substitute */
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

			if (c <= TOMATO_GPIO_MAX) gpio_write(1 << c, mode);
		}
	}

	return ret;
}
#endif /* TCONFIG_BCMARM */

#ifdef TCONFIG_BCMARM
void disable_led_wanlan(void)
{
	eval("et", "robowr", "0x0", "0x18", "0x0100"); /* turn off all LAN and WAN LEDs Part 1/2 */
	eval("et", "robowr", "0x0", "0x1a", "0x0100"); /* turn off all LAN and WAN LEDs Part 2/2 */
}

void enable_led_wanlan(void)
{
	eval("et", "robowr", "0x0", "0x18", "0x01ff"); /* turn on all LAN and WAN LEDs Part 1/2 */
	eval("et", "robowr", "0x0", "0x1a", "0x01ff"); /* turn on all LAN and WAN LEDs Part 2/2 */
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
		case MODEL_R6200v2:
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
		case MODEL_EX7000:
			set_gpio(GPIO_01, T_LOW); /* disable Netgear LOGO_LED */
			set_gpio(GPIO_05, T_HIGH); /* disable Netgear USB_LED */
			set_gpio(GPIO_09, T_HIGH); /* disable WLAN_2G_LED_RED */
			set_gpio(GPIO_08, T_HIGH); /* disable WLAN_2G_LED_GREEN */
			set_gpio(GPIO_11, T_HIGH); /* disable WLAN_5G_LED_RED */
			set_gpio(GPIO_10, T_HIGH); /* disable WLAN_5G_LED_GREEN */
			set_gpio(GPIO_13, T_HIGH); /* disable FT LED WHITE (Internet) - Device to Extender LED_RED */
			set_gpio(GPIO_12, T_HIGH); /* disable FT LED WHITE (Internet) - Device to Extender LED_GREEN */
			disable_led_wanlan();
			break;
		case MODEL_EX6200:
			set_gpio(GPIO_01, T_LOW); /* disable Netgear LOGO_LED */
			set_gpio(GPIO_05, T_HIGH); /* disable Netgear USB_LED */
			set_gpio(GPIO_13, T_HIGH); /* disable WLAN_2G_LED_RED */
			set_gpio(GPIO_12, T_HIGH); /* disable WLAN_2G_LED_GREEN */
			set_gpio(GPIO_11, T_HIGH); /* disable WLAN_5G_LED_RED */
			set_gpio(GPIO_10, T_HIGH); /* disable WLAN_5G_LED_GREEN */
			set_gpio(GPIO_09, T_HIGH); /* disable FT LED WHITE (Internet) - Device to Extender LED_RED */
			set_gpio(GPIO_08, T_HIGH); /* disable FT LED WHITE (Internet) - Device to Extender LED_GREEN */
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
		case MODEL_EX7000:
			/* activate WAN port led (Device to Extender LED in case EX7000) */
			set_gpio(GPIO_12, T_HIGH); /* LED green off */
			set_gpio(GPIO_13, T_LOW); /* LED red on */

			set_gpio(GPIO_09, T_HIGH); /* disable WLAN_2G_LED_RED */
			set_gpio(GPIO_11, T_HIGH); /* disable WLAN_5G_LED_RED */
			if (nvram_match("wl0_radio", "1")) {
				set_gpio(GPIO_08, T_LOW); /* enable WLAN_2G_LED_GREEN */
			}
			else {
				set_gpio(GPIO_08, T_HIGH); /* disable WLAN_2G_LED_GREEN */
			}
			if (nvram_match("wl1_radio", "1")) {
				set_gpio(GPIO_10, T_LOW); /* enable WLAN_5G_LED_GREEN */
			}
			else {
				set_gpio(GPIO_10, T_HIGH); /* disable WLAN_5G_LED_GREEN */
			}
			break;
		case MODEL_EX6200:
			/* activate WAN port led (Device to Extender LED in case EX6200) */
			set_gpio(GPIO_08, T_HIGH); /* LED green off */
			set_gpio(GPIO_09, T_LOW); /* LED red on */

			set_gpio(GPIO_13, T_HIGH); /* disable WLAN_2G_LED_RED */
			set_gpio(GPIO_11, T_HIGH); /* disable WLAN_5G_LED_RED */
			if (nvram_match("wl0_radio", "1")) {
				set_gpio(GPIO_12, T_LOW); /* enable WLAN_2G_LED_GREEN */
			}
			else {
				set_gpio(GPIO_12, T_HIGH); /* disable WLAN_2G_LED_GREEN */
			}
			if (nvram_match("wl1_radio", "1")) {
				set_gpio(GPIO_10, T_LOW); /* enable WLAN_5G_LED_GREEN */
			}
			else {
				set_gpio(GPIO_10, T_HIGH); /* disable WLAN_5G_LED_GREEN */
			}
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
			if      (mode == LED_ON)    eval("wl", "-i", "eth1", "ledbh", "9", "1"); /* 2.4 GHz - eth1, see Asus SRC */
			else if (mode == LED_OFF)   eval("wl", "-i", "eth1", "ledbh", "9", "0");
			else if (mode == LED_PROBE) return;
		}
		else if (which == LED_5G) {
			if      (mode == LED_ON)    eval("wl", "-i", "eth2", "ledbh", "9", "1"); /* 5 GHz - eth2, see Asus SRC */
			else if (mode == LED_OFF)   eval("wl", "-i", "eth2", "ledbh", "9", "0");
			else if (mode == LED_PROBE) return;
		}
		else if (which == LED_52G) {
			if      (mode == LED_ON)    eval("wl", "-i", "eth3", "ledbh", "9", "1"); /* second 5 GHz - eth3, see Asus SRC */
			else if (mode == LED_OFF)   eval("wl", "-i", "eth3", "ledbh", "9", "0");
			else if (mode == LED_PROBE) return;
		}
		break;
 #endif /* TCONFIG_AC5300 */
	case MODEL_RTAC3200:
		if (which == LED_WLAN) {
			if      (mode == LED_ON)    eval("wl", "-i", "eth2", "ledbh", "10", "1"); /* 2.4 GHz - eth2, see Asus SRC */
			else if (mode == LED_OFF)   eval("wl", "-i", "eth2", "ledbh", "10", "0");
			else if (mode == LED_PROBE) return;
		}
		else if (which == LED_5G) {
			if      (mode == LED_ON)    eval("wl", "-i", "eth1", "ledbh", "10", "1"); /* 5 GHz - eth1, see Asus SRC */
			else if (mode == LED_OFF)   eval("wl", "-i", "eth1", "ledbh", "10", "0");
			else if (mode == LED_PROBE) return;
		}
		else if (which == LED_52G) {
			if      (mode == LED_ON)    eval("wl", "-i", "eth3", "ledbh", "10", "1"); /* second 5 GHz - eth3, see Asus SRC */
			else if (mode == LED_OFF)   eval("wl", "-i", "eth3", "ledbh", "10", "0");
			else if (mode == LED_PROBE) return;
		}
		break;
#elif defined(CONFIG_BCMWL6A)
 #ifdef TCONFIG_BCM714
	case MODEL_RTAC3100:
	case MODEL_RTAC88U:
		if (which == LED_WLAN) {
			if      (mode == LED_ON)    eval("wl", "-i", "eth1", "ledbh", "9", "1"); /* 2.4 GHz - eth1, see Asus SRC */
			else if (mode == LED_OFF)   eval("wl", "-i", "eth1", "ledbh", "9", "0");
			else if (mode == LED_PROBE) return;
		}
		else if (which == LED_5G) {
			if      (mode == LED_ON)    eval("wl", "-i", "eth2", "ledbh", "9", "1"); /* 5 GHz - eth2, see Asus SRC */
			else if (mode == LED_OFF)   eval("wl", "-i", "eth2", "ledbh", "9", "0");
			else if (mode == LED_PROBE) return;
		}
		break;
 #endif /* TCONFIG_BCM714 */
	case MODEL_AC15:
	case MODEL_AC18:
	case MODEL_RTN18U:
		if (which == LED_WLAN) {
			if      (mode == LED_ON)    eval("wl", "-i", "eth1", "ledbh", "10", "7");
			else if (mode == LED_OFF)   eval("wl", "-i", "eth1", "ledbh", "10", "0");
			else if (mode == LED_PROBE) return;
		}
		break;
	case MODEL_RTAC56U:
		if (which == LED_WLAN) {
			if      (mode == LED_ON)    eval("wl", "-i", "eth1", "ledbh", "3", "1");
			else if (mode == LED_OFF)   eval("wl", "-i", "eth1", "ledbh", "3", "0");
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
			if      (mode == LED_ON)    eval("wl", "-i", "eth1", "ledbh", "10", "1");
			else if (mode == LED_OFF)   eval("wl", "-i", "eth1", "ledbh", "10", "0");
			else if (mode == LED_PROBE) return;
		}
		else if (which == LED_5G) {
			if      (mode == LED_ON)    eval("wl", "-i", "eth2", "ledbh", "10", "1");
			else if (mode == LED_OFF)   eval("wl", "-i", "eth2", "ledbh", "10", "0");
			else if (mode == LED_PROBE) return;
		}
		break;
#endif /* TCONFIG_AC3200 */
	default:
		/* nothing to do right now */
		break;
	}

}
#endif /* TCONFIG_BCMARM */
