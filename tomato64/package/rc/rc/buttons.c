/*

	Tomato Firmware
	Copyright (C) 2006-2009 Jonathan Zarate

*/


#include "rc.h"

#include <sys/reboot.h>
#include <wlutils.h>
#include <wlioctl.h>

//#define DEBUG_TEST

/* needed by logmsg() */
#define LOGMSG_DISABLE	DISABLE_SYSLOG_OS
#define LOGMSG_NVDEBUG	"buttons_debug"

#define BUTTON_SAMPLE_RATE 500000 /* 500 ms */

static int gf;


static int get_btn(const char *name, uint32_t *bit, uint32_t *pushed)
{
	int gpio;
	int inv;
	
	if (nvget_gpio(name, &gpio, &inv)) {
		*bit = 1 << gpio;
		*pushed = inv ? 0 : *bit;
		return 1;
	}

	return 0;
}

int buttons_main(int argc, char *argv[])
{
	long gpio;
	long last;
	uint32_t mask;
	uint32_t ses_mask;
	uint32_t ses_pushed;
	uint32_t reset_mask;
	uint32_t reset_pushed;
	int count;
	char s[16];
	char *p;
	int n;
	int ses_led;
	int model;
#ifdef CONFIG_BCMWL6A
	uint32_t wlan_mask;
	uint32_t wlan_pushed;
#else
	uint32_t brau_mask;
	long brau_state;
	int brau_count_stable;
	int brau_flag;
#endif

	/* get Router model */
	model = get_model();

	ses_mask = 0;
	ses_pushed = 0;
	reset_pushed = 0;
	ses_led = LED_DIAG;
	last = 0;
#ifdef CONFIG_BCMWL6A
	reset_mask = 1 << 4;
	wlan_mask = 0;
	wlan_pushed = 0;
#else
	brau_mask = 0;
	brau_state = ~0;;
	brau_count_stable = 0;
	brau_flag = 0;
#endif

	switch (nvram_get_int("btn_override") ? MODEL_UNKNOWN : model) {

#ifndef CONFIG_BCMWL6A

	case MODEL_WRT54G:
	case MODEL_WRTSL54GS:
		reset_mask = 1 << 6;
		ses_mask = 1 << 4;
		ses_led = LED_DMZ;
		break;
	case MODEL_WTR54GS:
		reset_mask = 1 << 3;
		ses_mask = 1 << 2;
		break;
	case MODEL_WHRG54S:
	case MODEL_WHRHPG54:
	case MODEL_WHR2A54G54:
	case MODEL_WHR3AG54:
	case MODEL_WHRG125:
		reset_mask = reset_pushed = 1 << 4;
		ses_mask = 1 << 0;
		brau_mask = 1 << 5;
		break;
	case MODEL_WBRG54:
		reset_mask = reset_pushed = 1 << 4;
		break;
	case MODEL_WBR2G54:
		reset_mask = reset_pushed = 1 << 7;
		ses_mask = ses_pushed = 1 << 4;
		ses_led = LED_AOSS;
		break;
	case MODEL_WZRG54:
	case MODEL_WZRHPG54:
	case MODEL_WZRRSG54:
	case MODEL_WZRRSG54HP:
	case MODEL_WVRG54NF:
		reset_mask = reset_pushed = 1 << 4;
		ses_mask = 1 << 0;
		ses_led = LED_AOSS;
		break;
	case MODEL_WZRG108:
		reset_mask = reset_pushed = 1 << 7;
		ses_mask = 1 << 0;
		ses_led = LED_AOSS;
		break;
	case MODEL_WR850GV1:
		reset_mask = 1 << 0;
		break;
	case MODEL_WR850GV2:
	case MODEL_WR100:
		reset_mask = 1 << 5;
		break;
	case MODEL_WL500GP:
		reset_mask = reset_pushed = 1 << 0;
		ses_mask = ses_pushed = 1 << 4;
		break;
	case MODEL_WL500W:
		reset_mask = reset_pushed = 1 << 6;
		ses_mask = ses_pushed = 1 << 7;
		break;
	case MODEL_DIR320:
	case MODEL_H618B:
		reset_mask = 1 << 7;
		ses_mask = 1 << 6; /* WLAN button on H618B */
		break;
	case MODEL_WL500GPv2:
	case MODEL_WL520GU:
	case MODEL_WL330GE:
		reset_mask = 1 << 2;
		ses_mask = 1 << 3;
		break;
	case MODEL_WLA2G54L:
		reset_mask = reset_pushed = 1 << 7;
		break;
	case MODEL_WL1600GL:
		reset_mask = 1 << 3;
		ses_mask = 1 << 4;
		ses_led = LED_AOSS;
		break;
	case MODEL_RTN10:
		reset_mask = 1 << 3;
		ses_mask = 1 << 2;
		break;
	case MODEL_RTN16:
		reset_mask = 1 << 6;
		ses_mask = 1 << 8;
		break;
	case MODEL_WNR3500L:
#ifdef TCONFIG_BLINK /* RT-N/RTAC */
	case MODEL_WNR3500LV2:
#endif
		reset_mask = 1 << 4;
		ses_mask = 1 << 6;
		ses_led = LED_AOSS;
		break;
	case MODEL_WNR2000v2:
		reset_mask = 1 << 1;
		ses_mask = 1 << 0;
		ses_led = LED_AOSS;
		break;
	case MODEL_F7D3301:
	case MODEL_F7D3302:
	case MODEL_F7D4301:
	case MODEL_F7D4302:
	case MODEL_F5D8235v3:
		reset_mask = 1 << 6;
		ses_mask = 1 << 8;
		ses_led = LED_AOSS;
		break;
	case MODEL_WRT160Nv3:
		reset_mask = 1 << 6;
		ses_mask = 1 << 5;
		break;
	case MODEL_WRT320N:
		reset_mask = 1 << 8;
		ses_mask = 1 << 5;
		ses_led = LED_AMBER;
		break;
	case MODEL_WRT610Nv2:
		reset_mask = 1 << 6;
		ses_mask = 1 << 4;
		ses_led = LED_AMBER;
		break;
	case MODEL_E4200:
		reset_mask = 1 << 6;
		ses_mask = 1 << 4;
		ses_led = LED_WHITE;
		break;
	case MODEL_WRT160Nv1:
	case MODEL_WRT300N:
		reset_mask = 1 << 6;
		ses_mask = 1 << 4;
		break;
	case MODEL_WRT310Nv1:
		reset_mask = 1 << 6;
		ses_mask = 1 << 8;
		break;
#ifdef TCONFIG_BLINK /* RT-N/RTAC */
	case MODEL_RTN12A1:
#else
	case MODEL_RTN12:
#endif
		reset_mask = 1 << 1;
		ses_mask = 1 << 0;
		brau_mask = (1 << 4) | (1 << 5) | (1 << 6);
		break;
#ifdef TCONFIG_BLINK /* RT-N/RTAC */
	case MODEL_RTN10U:
		reset_mask = 1 << 21;
		ses_mask = 1 << 20;
		ses_led = LED_AOSS;
		break;
	case MODEL_RTN10P:
		reset_mask = 1 << 20;
		ses_mask = 1 << 21;
		ses_led = LED_AOSS;
		break;
	case MODEL_RTN12B1:
	case MODEL_RTN12C1:
	case MODEL_RTN12D1:
	case MODEL_RTN12VP:
	case MODEL_RTN12K:
	case MODEL_RTN12HP:
		reset_mask = 1 << 22; /* reset button (active LOW) */
		ses_mask = 1 << 23; /* wps button (active LOW) */
		ses_led = LED_AOSS; /* Use LED AOSS (Power LED for Asus Router) for feedback if a button is pushed */
		break;
	case MODEL_RTN15U:
		reset_mask = 1 << 5;
		ses_mask = 1 << 8;
		break;
	case MODEL_RTN53:
		reset_mask = 1 << 3;
		ses_mask = 1 << 7;
		break;
	case MODEL_RTN53A1:
		reset_mask = 1 << 7;
		ses_mask = 1 << 3;
		break;
	case MODEL_RTN66U:
		reset_mask = 1 << 9;
		ses_mask = 1 << 4;
		break;
	case MODEL_R6300V1:
	case MODEL_WNDR4500:
	case MODEL_WNDR4500V2:
		reset_mask = 1 << 6;
		ses_mask = 1 << 5; /* gpio 5, inversed */
		break;
	case MODEL_DIR865L:
		reset_mask = 1 << 5;
		ses_mask = 1 << 14;
		ses_led = LED_AOSS;
		break;
	case MODEL_EA6500V1:
		reset_mask = 1 << 3;
		ses_mask = 1 << 4;
		break;
	case MODEL_TDN80:
		reset_mask = 1 << 14;
		break;
	case MODEL_W1800R:
		reset_mask = 1 << 14;
		break;
	case MODEL_D1800H:
		reset_mask = 1 << 5;
		break;
	case MODEL_F9K1102:
		reset_mask = 1 << 3;
		ses_mask = 1 << 7;
		ses_led = LED_AOSS;
		break;
	case MODEL_E900:
	case MODEL_E1000v2:
	case MODEL_E1500:
	case MODEL_E1550:
	case MODEL_E2500:
		reset_mask = 1 << 10;
		ses_mask = 1 << 9;
		break;
	case MODEL_E3200:
		reset_mask = 1 << 5;
		ses_mask = 1 << 8;
		break;
	case MODEL_L600N:
		reset_mask = 1 << 21;
		ses_mask = 1 << 20;
		// wlan button = 1 >> 10
		break;
	case MODEL_DIR620C1:
		reset_mask = 1 << 21;
		ses_mask = 1 << 20;
		break;
	case MODEL_RG200E_CA:
	case MODEL_H218N:
		reset_mask = 1 << 30;
		ses_mask = 1 << 28;
		break;
	case MODEL_HG320:
		reset_mask = 1 << 30;
		ses_mask = 1 << 29;
		break;
	case MODEL_TDN60:
		reset_mask = 1 << 8;
		break;
	case MODEL_TDN6:
		reset_mask = 1 << 20;
		break;
	case MODEL_WNDR4000:
	case MODEL_WNDR3700v3:
		reset_mask = 1 << 3;
		ses_mask = 1 << 2;
		ses_led = LED_AOSS;
		break;
	case MODEL_WNDR3400:
		reset_mask = 1 << 4;
		ses_mask = 1 << 8;
		break;
	case MODEL_WNDR3400v2:
	case MODEL_WNDR3400v3:
		reset_mask = 1 << 12;
		ses_mask = 1 << 23;
		ses_led = LED_AOSS; /* Use LED AOSS for feedback if a button is pushed */
		break;
#endif /* TCONFIG_BLINK */

#else /* !CONFIG_BCMWL6A */

	case MODEL_RTN18U:
		reset_mask = 1 << 7; /* reset button (active LOW) */
		ses_mask = 1 << 11; /* wps button (active LOW) */
		ses_led = LED_AOSS;
		break;
	case MODEL_RTAC56U:
		reset_mask = 1 << 11; /* reset button (active LOW) */
		ses_mask = 1 << 15; /* wps button (active LOW) */
		wlan_mask = 1 << 7;  /* wifi button (active LOW) */
		ses_led = LED_AOSS;
		break;
	case MODEL_RTAC67U:
		reset_mask = 1 << 11; /* reset button (active LOW) */
		ses_mask = 1 << 7; /* wps button (active LOW) */
		ses_led = LED_AOSS;
		break;
	case MODEL_DSLAC68U:
	case MODEL_RTAC68U:
	case MODEL_RTAC68UV3:
		reset_mask = 1 << 11; /* reset button (active LOW) */
		ses_mask = 1 << 7; /* wps button (active LOW) */
		wlan_mask = 1 << 15;  /* wifi button (active LOW) */
		ses_led = LED_AOSS;
		break;
	case MODEL_RTAC1900P:
		reset_mask = 1 << 11; /* reset button (active LOW) */
		ses_mask = 1 << 7; /* wps button (active LOW) */
		wlan_mask = 1 << 15;  /* wifi button (active LOW) */
		ses_led = LED_AOSS;
		break;
	case MODEL_RTAC66U_B1:
		reset_mask = 1 << 11; /* reset button (active LOW) */
		ses_mask = 1 << 7; /* wps button (active LOW) */
		ses_led = LED_AOSS;
		break;
	case MODEL_DIR868L:
		reset_mask = 1 << 11; /* reset button (active LOW) */
		ses_mask = 1 << 7; /* wps button (active LOW) */
		ses_led = LED_DIAG; /* Use LED Diag for feedback if a button is pushed */
		break;
	case MODEL_AC15:
		reset_mask = 1 << 11; /* reset button (active LOW) */
		ses_mask = 1 << 7; /* wps button (active LOW) */
		wlan_mask = 1 << 15; /* wifi button (active LOW) */
		ses_led = LED_AOSS; /* Use LED AOSS for feedback if a button is pushed */
		break;
	case MODEL_AC18:
		reset_mask = 1 << 11; /* reset button (active LOW) */
		ses_mask = 1 << 7; /* wps button (active LOW) */
		wlan_mask = 1 << 15; /* wifi button (active LOW) */
		ses_led = LED_AOSS; /* Use LED AOSS for feedback if a button is pushed */
		break;
	case MODEL_F9K1113v2_20X0:
	case MODEL_F9K1113v2:
		reset_mask = 1 << 8; /* reset button (active LOW) */
		ses_mask = 1 << 7; /* wps button (active LOW) */
		ses_led = LED_AOSS; /* Use LED AOSS for feedback if a button is pushed */
		break;
	case MODEL_WS880:
		reset_mask = 1 << 2; /* reset button (active LOW) */
		ses_mask = 1 << 3; /* wps button (active LOW) */
		wlan_mask = 1 << 15; /* power button (active LOW) --> use it to toggle wifi */
		ses_led = LED_AMBER; /* Use LED Amber (only Dummy) for feedback if a button is pushed. Do not interfere with LED_AOSS --> used for WLAN SUMMARY LED */
		break;
	case MODEL_EA6350v1:
	case MODEL_EA6350v2:
		ses_mask = 1 << 7;
		reset_mask = 1 << 11;
		ses_led = LED_AOSS;
		break;
	case MODEL_EA6400:
		ses_mask = 1 << 7;
		reset_mask = 1 << 11;
		ses_led = LED_AOSS;
		break;
	case MODEL_EA6700:
		ses_mask = 1 << 7;
		reset_mask = 1 << 11;
		ses_led = LED_AOSS;
		break;
	case MODEL_EA6900:
		ses_mask = 1 << 7;
		reset_mask = 1 << 11;
		ses_led = LED_AOSS;
		break;
	case MODEL_R1D:
		reset_mask = 1 << 17;
		ses_led = LED_AOSS;
		break;
	case MODEL_AC1450:
	case MODEL_R6200v2:
	case MODEL_R6250:
	case MODEL_R6300v2:
		reset_mask = 1 << 6; /* reset button (active LOW) */
		ses_mask = 1 << 4; /* wps button (active LOW) */
		wlan_mask = 1 << 5;  /* wifi button (active LOW) */
		ses_led = LED_AOSS;
		break;
	case MODEL_R6400:
	case MODEL_R6400v2:
	case MODEL_R6700v3:
	case MODEL_XR300:
		reset_mask = 1 << 5; /* reset button (active LOW) */
		ses_mask = 1 << 3; /* wps button (active LOW) */
		wlan_mask = 1 << 4; /* wifi button (active LOW) */
		ses_led = LED_DIAG; /* Use LED Diag for feedback if a button is pushed. Do not interfere with LED_AOSS --> used for WLAN SUMMARY LED */
		break;
	case MODEL_R6700v1:
	case MODEL_R6900:
	case MODEL_R7000:
		reset_mask = 1 << 6; /* reset button (active LOW) */
		ses_mask = 1 << 4; /* wps button (active LOW) */
		wlan_mask = 1 << 5;  /* wifi button (active LOW) */
		ses_led = LED_DIAG; /* Use LED Diag for feedback if a button is pushed. Do not interfere with LED_AOSS --> used for WLAN SUMMARY LED */
		break;
	case MODEL_WZR1750:
		reset_mask = 1 << 11; /* reset button (active LOW) */
		ses_mask = 1 << 12; /* wps button (active LOW) */
		ses_led = LED_DIAG; /* Use LED Diag for feedback if a button is pushed. */
		break;
#ifdef TCONFIG_AC3200
	case MODEL_RTAC3200:
		reset_mask = 1 << 11; /* reset button (active LOW) */
		ses_mask = 1 << 7; /* wps button (active LOW) */
		wlan_mask = 1 << 4;  /* wifi button (active LOW) */
		ses_led = LED_AOSS; /* Use LED AOSS for feedback if a button is pushed. */
		break;
	case MODEL_R8000:
		reset_mask = 1 << 6; /* reset button (active LOW) */
		ses_mask = 1 << 5; /* wps button (active LOW) */
		wlan_mask = 1 << 4; /* wifi button (active LOW) */
		ses_led = LED_DIAG; /* Use LED Diag for feedback if a button is pushed. Do not interfere with LED_AOSS --> used for WLAN SUMMARY LED */
		break;
#endif /* TCONFIG_AC3200 */
#ifdef TCONFIG_BCM714
	case MODEL_RTAC3100:
	case MODEL_RTAC88U:
		reset_mask = 1 << 11; /* reset button (active LOW) */
		ses_mask = 1 << 20; /* wps button (active LOW) */
		wlan_mask = 1 << 18;  /* wifi button (active LOW) */
		ses_led = LED_AOSS; /* Use LED AOSS for feedback if a button is pushed. */
		break;
#ifdef TCONFIG_AC5300
	case MODEL_RTAC5300:
		reset_mask = 1 << 11; /* reset button (active LOW) */
		ses_mask = 1 << 18; /* wps button (active LOW) */
		wlan_mask = 1 << 20;  /* wifi button (active LOW) */
		ses_led = LED_AOSS; /* Use LED AOSS for feedback if a button is pushed. */
		break;
#endif /* TCONFIG_AC5300 */
#endif /* TCONFIG_BCM714 */
#endif /* !CONFIG_BCMWL6A */

	default:
		get_btn("btn_ses", &ses_mask, &ses_pushed);
		if (!get_btn("btn_reset", &reset_mask, &reset_pushed))
			return 1;
		break;
	}
#ifdef CONFIG_BCMWL6A
	mask = reset_mask | ses_mask | wlan_mask;
#else
	mask = reset_mask | ses_mask | brau_mask;
#endif

#ifdef DEBUG_TEST
	cprintf("reset_mask=0x%X reset_pushed=0x%X\n", reset_mask, reset_pushed);
#ifdef CONFIG_BCMWL6A
	cprintf("wlan_mask=0x%X wlan_pushed=0x%X\n", wlan_mask, wlan_pushed);
#else
	cprintf("brau_mask=0x%X\n", brau_mask);
#endif
	cprintf("ses_mask=0x%X\n", ses_mask);
	cprintf("ses_led=%d\n", ses_led);
#else /* DEBUG_TEST */
	if (fork() != 0)
		return 0;

	setsid();
#endif /* DEBUG_TEST */

	signal(SIGCHLD, chld_reap);

	if ((gf = gpio_open(mask)) < 0)
		return 1;

	while (1) {
		if (((gpio = _gpio_read(gf)) == ~0) || (last == (gpio &= mask)
#ifndef CONFIG_BCMWL6A
		                                        && !brau_flag
#endif
		                                       ) || (check_action() != ACT_IDLE)) {
#ifdef DEBUG_TEST
			cprintf("gpio = %X\n", gpio);
#endif
			usleep(BUTTON_SAMPLE_RATE); /* wait 500 ms */
			continue;
		}

		if ((gpio & reset_mask) == reset_pushed) {
#ifdef DEBUG_TEST
			cprintf("reset down\n");
#endif

			led(LED_DIAG, LED_OFF);

			count = 0;
			do {
				usleep(BUTTON_SAMPLE_RATE); /* wait 500 ms */
				if (++count == 6)
					led(LED_DIAG, LED_ON);
			} while (((gpio = _gpio_read(gf)) != ~0) && ((gpio & reset_mask) == reset_pushed));
			gpio &= mask;

#ifdef DEBUG_TEST
			cprintf("reset count = %d\n", count);
#else
			if (count >= 6) { /* after 3 sec */
#ifndef CONFIG_BCMWL6A
				eval("mtd-erase", "-d", "nvram");
#else
				eval("mtd-erase2", "nvram");
#endif
				sync();
				reboot(RB_AUTOBOOT);
				exit(0);
			}
			else if (count >= 2) {  /* after 1 sec */
				led(LED_DIAG, LED_ON);
				set_action(ACT_REBOOT);
				kill(1, SIGTERM);
				exit(0);
			}
			else {
				logmsg(LOG_INFO, "reset button pushed for %d ms", count * 500);
				/* nothing to do right now! */
			}
#endif /* DEBUG_TEST */
		}

		if ((ses_mask) && ((gpio & ses_mask) == ses_pushed)) {
			count = 0;
			do {
				logmsg(LOG_DEBUG, "*** %s: ses-pushed: gpio=x%ld, pushed=x%X, mask=x%X, count=%d", __FUNCTION__, gpio, ses_pushed, ses_mask, count);
				led(ses_led, LED_ON);
				usleep(BUTTON_SAMPLE_RATE/2);
				led(ses_led, LED_OFF);
				usleep(BUTTON_SAMPLE_RATE/2);
				++count;
			} while (((gpio = _gpio_read(gf)) != ~0) && ((gpio & ses_mask) == ses_pushed));
			gpio &= mask;

#ifdef TCONFIG_BLINK /* RT-N/RTAC */
			/* for WNDR3400/3700v3/4000 */
			if ((model == MODEL_WNDR3400) || (model == MODEL_WNDR3700v3) || (model == MODEL_WNDR4000))
				led(ses_led, LED_ON);

			/* turn LED_AOSS (WPS LED for WNDR3400v2/v3) back on if used for feedback (WPS Button); Check Startup LED setting (bit 2 used for LED_AOSS) */
			if ((ses_led == LED_AOSS) && (nvram_get_int("sesx_led") & 0x04) && ((model == MODEL_WNDR3400v2) || (model == MODEL_WNDR3400v3)))
				led(ses_led, LED_ON);
#endif /* TCONFIG_BLINK */

			if ((ses_led == LED_DMZ) && (nvram_get_int("dmz_enable") > 0))
				led(LED_DMZ, LED_ON); /* turn LED_DMZ back on if used for feedback */

#ifdef CONFIG_BCMWL6A
			/* turn LED_AOSS (Power LED for Asus Router; WPS LED for Tenda Router AC15/AC18) back on if used for feedback (WPS Button); Check Startup LED setting (bit 2 used for LED_AOSS) */
			if ((ses_led == LED_AOSS) && (nvram_get_int("sesx_led") & 0x04) &&
			    ((model == MODEL_AC15)
			     || (model == MODEL_AC18)
			     || (model == MODEL_RTN18U)
			     || (model == MODEL_RTAC56U)
			     || (model == MODEL_RTAC66U_B1)
			     || (model == MODEL_RTAC1900P)
			     || (model == MODEL_RTAC67U)
			     || (model == MODEL_DSLAC68U)
			     || (model == MODEL_RTAC68U)
			     || (model == MODEL_RTAC68UV3)
#ifdef TCONFIG_BCM714
			     || (model == MODEL_RTAC3100)
			     || (model == MODEL_RTAC88U)
#endif /* TCONFIG_BCM714 */
#ifdef TCONFIG_AC3200
#ifdef TCONFIG_AC5300
			     || (model == MODEL_RTAC5300)
#endif			     
			     || (model == MODEL_RTAC3200)
#endif
			)) {
				led(ses_led, LED_ON);
			}
#endif /* CONFIG_BCMWL6A */

			logmsg(LOG_DEBUG, "*** %s: ses-released: gpio=x%ld, pushed=x%X, mask=x%X, count=%d", __FUNCTION__, gpio, ses_pushed, ses_mask, count);
			logmsg(LOG_INFO, "SES button pushed for %d ms", count * 500);

			n = -1;
			/*
			count 2-4   = func0 (1-2 sec)
			count 8-12  = func1 (4-6 sec)
			count 16-20 = func2 (8-10 sec)
			count 24+   = func3 (12+ sec)
			*/
			if (count > 1 && count < 5)
				n = 0;
			else if (count > 7 && count < 13)
				n = 1;
			else if (count > 15 && count < 21)
				n = 2;
			else if (count > 23)
				n = 3;

			if (n != -1) {
#ifdef DEBUG_TEST
				cprintf("ses func=%d\n", n);
#else
				snprintf(s, sizeof(s), "sesx_b%d", n);
				logmsg(LOG_DEBUG, "*** %s: ses-func: count=%d %s='%s'", __FUNCTION__, count, s, nvram_safe_get(s));
				if ((p = nvram_get(s)) != NULL) {
					switch (*p) {
					case '1': /* toggle wl */
						nvram_set("rrules_radio", "-1");
						eval("radio", "toggle");
						break;
					case '2': /* reboot */
						kill(1, SIGTERM);
						break;
					case '3': /* shutdown */
						kill(1, SIGQUIT);
						break;
					case '4': /* run a script */
						snprintf(s, sizeof(s), "%d", count/2);
						run_nvscript("sesx_script", s, 2);
						break;
#ifdef TCONFIG_USB
					case '5': /* unmount all USB drives */
						add_remove_usbhost("-2", 0);
						break;
#endif /* TCONFIG_USB */
					}
				}
#endif /* DEBUG_TEST */

			}
		}

#ifndef CONFIG_BCMWL6A
		if (brau_mask) {
			if (last == gpio)
				usleep(BUTTON_SAMPLE_RATE); /* wait 500 ms */

			last = (gpio & brau_mask);
			if (brau_state != last) {
				brau_flag = (brau_state != ~0); /* set to 1 to run at startup */
				brau_state = last;
				brau_count_stable = 0;
			}
			else if (brau_flag && ++brau_count_stable > 4) { /* stable for 2+ seconds */
				brau_flag = 0;

				switch (nvram_get_int("btn_override") ? MODEL_UNKNOWN : model) {
#ifdef TCONFIG_BLINK /* RT-N/RTAC */
				case MODEL_RTN12A1:
#else
				case MODEL_RTN12:
#endif
					p = (brau_state & (1 << 4)) ? "ap" : (brau_state & (1 << 5)) ? "repeater" : "router";
					break;
				default:
					p = brau_state ? "auto" : "bridge";
					break;
				}

				nvram_set("brau_state", p);
#ifdef DEBUG_TEST
				cprintf("bridge/auto state = %s\n", p);
#else
				run_nvscript("script_brau", p, 2);
#endif /* DEBUG_TEST */
			}
		}
#else /* !CONFIG_BCMWL6A */
		if ((wlan_mask) && ((gpio & wlan_mask) == wlan_pushed)) {
			count = 0;
			do {
				logmsg(LOG_DEBUG, "*** %s: wlan-pushed: gpio=x%ld, pushed=x%X, mask=x%X, count=%d", __FUNCTION__, gpio, wlan_pushed, wlan_mask, count);
				led(ses_led, LED_ON);
				usleep(BUTTON_SAMPLE_RATE/2);
				led(ses_led, LED_OFF);
				usleep(BUTTON_SAMPLE_RATE/2);
				++count;
			} while (((gpio = _gpio_read(gf)) != ~0) && ((gpio & wlan_mask) == wlan_pushed));
			gpio &= mask;

			/* turn LED_AOSS (Power LED for Asus Router; WPS LED for Tenda Router AC15/AC18) back on if used for feedback (WLAN Button); Check Startup LED setting (bit 2 used for LED_AOSS) */
			if ((ses_led == LED_AOSS) && (nvram_get_int("sesx_led") & 0x04) &&
			    ((model == MODEL_AC15)
			     || (model == MODEL_AC18)
			     || (model == MODEL_RTAC56U)
			     || (model == MODEL_RTAC1900P)
			     || (model == MODEL_DSLAC68U)
			     || (model == MODEL_RTAC68U)
			     || (model == MODEL_RTAC68UV3)
#ifdef TCONFIG_BCM714
			     || (model == MODEL_RTAC3100)
			     || (model == MODEL_RTAC88U)
#endif /* TCONFIG_BCM714 */
#ifdef TCONFIG_AC3200
#ifdef TCONFIG_AC5300
			     || (model == MODEL_RTAC5300)
#endif
			     || (model == MODEL_RTAC3200)
#endif
			)) {
				led(ses_led, LED_ON);
			}

			logmsg(LOG_DEBUG, "*** %s: wlan-released: gpio=x%ld, pushed=x%X, mask=x%X, count=%d", __FUNCTION__, gpio, wlan_pushed, wlan_mask, count);

			if (count >= 2) {  /* after 1 sec */
				logmsg(LOG_INFO, "WLAN button pushed for %d ms - toggle radio", count * 500);
				nvram_set("rrules_radio", "-1");
				eval("radio", "toggle");
			}
			else {
				logmsg(LOG_INFO, "WLAN button pushed for %d ms", count * 500);
				/* nothing to do right now! */
			}
		}
#endif /* !CONFIG_BCMWL6A */

		last = gpio;
	}

	return 0;
}
