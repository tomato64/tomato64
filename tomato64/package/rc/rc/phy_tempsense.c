/*
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston,
 * MA 02111-1307 USA
 *
 * Copyright 2011, ASUSTeK Inc.
 * All Rights Reserved.
 * 
 * THIS SOFTWARE IS OFFERED "AS IS", AND ASUS GRANTS NO WARRANTIES OF ANY
 * KIND, EXPRESS OR IMPLIED, BY STATUTE, COMMUNICATION OR OTHERWISE. BROADCOM
 * SPECIFICALLY DISCLAIMS ANY IMPLIED WARRANTIES OF MERCHANTABILITY, FITNESS
 * FOR A SPECIFIC PURPOSE OR NONINFRINGEMENT CONCERNING THIS SOFTWARE.
 *
 */
/*
 *
 * Fixes/updates (C) 2018 - 2023 pedro
 *
 */


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <syslog.h>
#include <signal.h>
#include <sys/time.h>
#include <unistd.h>
#include <bcmnvram.h>
#include <shared.h>
#include <shutils.h>
#include <wlutils.h>

#define FAN_NORMAL_PERIOD	2.5 * 1000	/* microseconds */
#define TEMP_MAX		90.000		/* note: this not degrees */
#define TEMP_4			84.000		/* actual degrees math is */
#define TEMP_3			78.000		/* Celsius: val / 2 + 20, */
#define TEMP_2			72.000		/* 72.000 = 56 C          */
#define TEMP_MIN		66.000		/* 66.000 = 53 C          */
#define TEMP_H			3.000		/* temp delta             */

#define max(a,b)  (((a) > (b)) ? (a) : (b))
#define min(a,b)  (((a) < (b)) ? (a) : (b))

static int count = -2;
static int status = -1;
static int duty_cycle = 0;
static int status_old = 0;
static double tempavg_1 = 0.000;
static double tempavg_2 = 0.000;
static double tempavg_max = 0.000;
static struct itimerval itv;
static int count_timer = 0;
static int base = 1;


static void alarmtimer(unsigned long sec, unsigned long usec)
{
	itv.it_value.tv_sec  = sec;
	itv.it_value.tv_usec = usec;
	itv.it_interval = itv.it_value;
	setitimer(ITIMER_REAL, &itv, NULL);
}

static int fan_status()
{
	int idx;

	if (!base)
		return 1;
	else if (base == 1)
		return 0;
	else
		idx = count_timer % base;

	if (!idx)
		return 0;
	else
		return 1;
}

static void phy_tempsense_exit(int sig)
{
	alarmtimer(0, 0);
	//led(LED_BRIDGE, LED_OFF);
	remove("/var/run/phy_tempsense.pid");
	exit(0);
}

static void phy_tempsense_mon()
{
	char buf1[WLC_IOCTL_SMLEN];
	char buf2[WLC_IOCTL_SMLEN];
	char w[8];
	int ret, tmin, tmp2, tmp3, tmp4, tmax;
	unsigned int r0 = 1; /* fake val r0, in case interface is down */
	unsigned int *ret_int1 = NULL;
	unsigned int *ret_int2 = NULL;

	strlcpy(buf1, "phy_tempsense", WLC_IOCTL_SMLEN);
	strlcpy(buf2, "phy_tempsense", WLC_IOCTL_SMLEN);

	if ((ret = wl_ioctl("eth1", WLC_GET_VAR, buf1, sizeof(buf1))) == 0)
		ret_int1 = (unsigned int *)buf1;
	else
		ret_int1 = &r0; /* fake val r0 */

	if ((ret = wl_ioctl("eth2", WLC_GET_VAR, buf2, sizeof(buf2))) == 0)
		ret_int2 = (unsigned int *)buf2;
	else
		ret_int2 = &r0; /* fake val r0 */

	if (count == -2) {
		count++;
		tempavg_1 = *ret_int1;
		tempavg_2 = *ret_int2;
	}
	else {
		tempavg_1 = (tempavg_1 * 4 + *ret_int1) / 5;
		tempavg_2 = (tempavg_2 * 4 + *ret_int2) / 5;
	}

	/* use highest, not average val (better in case only one radio enabled) */
	tempavg_max = (((tempavg_1) > (tempavg_2)) ? (tempavg_1) : (tempavg_2));

	duty_cycle = nvram_get_int("fanctrl_dutycycle");

	if ((duty_cycle < 0) || (duty_cycle > 10))
		duty_cycle = 0;

	/* allow user redefine values via nvram */
	tmin = (nvram_get_int("fanctrl_t1") ? ((nvram_get_int("fanctrl_t1") - 20) * 2) : TEMP_MIN);
	tmp2 = (nvram_get_int("fanctrl_t2") ? ((nvram_get_int("fanctrl_t2") - 20) * 2) : TEMP_2);
	tmp3 = (nvram_get_int("fanctrl_t3") ? ((nvram_get_int("fanctrl_t3") - 20) * 2) : TEMP_3);
	tmp4 = (nvram_get_int("fanctrl_t4") ? ((nvram_get_int("fanctrl_t4") - 20) * 2) : TEMP_4);
	tmax = (nvram_get_int("fanctrl_t5") ? ((nvram_get_int("fanctrl_t5") - 20) * 2) : TEMP_MAX);

	/* some failsafe checks (revert to defaults on wrong / incomplete user settings) */
	if ((tmp2 <= tmin) || (tmin < 0)) {
		tmin = TEMP_MIN;
		tmp2 = TEMP_2;
	}
	if (tmp3 <= tmp2)
		tmp3 = TEMP_3;
	if (tmp4 <= tmp3)
		tmp4 = TEMP_4;
	if (tmax <= tmp4)
		tmax = TEMP_MAX;

	if (duty_cycle && (tempavg_max < tmax))
		base = duty_cycle;
	else {
		if (tempavg_max <= tmin - TEMP_H)
			base = 1;
		else if ((tempavg_max > tmin) && (tempavg_max < tmp2 - TEMP_H))
			base = 2;
		else if ((tempavg_max > tmp2) && (tempavg_max < tmp3 - TEMP_H))
			base = 3;
		else if ((tempavg_max > tmp3) && (tempavg_max < tmp4 - TEMP_H))
			base = 4;
		else if ((tempavg_max > tmp4) && (tempavg_max < tmax - TEMP_H))
			base = 5;
		else if (tempavg_max > tmax) /* overheat! */
			base = 0; /* max revs, no pwm */
	}

	/* this param do nothing, just for user info */
	if (!base)
		nvram_set("fanctrl_dutycycle_ex", "0");
	else {
		memset(w, 0, sizeof(w));
		snprintf(w, sizeof(w), "%d", base);
		nvram_set("fanctrl_dutycycle_ex", w);
	}
}

static void phy_tempsense(int sig)
{
	int count_local = count_timer % 400; /* monitor period */
	if (!count_local)
		phy_tempsense_mon();

	status_old = status;
	status = fan_status();

	if (status != status_old) {
		if (status)
			led(LED_BRIDGE, LED_ON);
		else
			led(LED_BRIDGE, LED_OFF);
	}

	count_timer = (count_timer + 1) % 800; /* monitor period */

	alarmtimer(0, FAN_NORMAL_PERIOD); /* fan pwm period */
}

static void update_dutycycle(int sig)
{
	alarmtimer(0, 0);
	count = -1;
	status = -1;
	count_timer = 0;

	duty_cycle = nvram_get_int("fanctrl_dutycycle");

	if ((duty_cycle < 0) || (duty_cycle > 10))
		duty_cycle = 0; /* switch to auto */

	phy_tempsense(sig);
}

int phy_tempsense_main(int argc, char *argv[])
{
	FILE *fp;
	sigset_t sigs_to_catch;
	int t1, t2, t3, t4, t5;

	/* write pid */
	if ((fp = fopen("/var/run/phy_tempsense.pid", "w")) != NULL) {
		fprintf(fp, "%d", getpid());
		fclose(fp);
	}

	/* set the signal handler */
	sigemptyset(&sigs_to_catch);
	sigaddset(&sigs_to_catch, SIGALRM);
	sigaddset(&sigs_to_catch, SIGTERM);
	sigaddset(&sigs_to_catch, SIGUSR1);
	sigprocmask(SIG_UNBLOCK, &sigs_to_catch, NULL);

	signal(SIGALRM, phy_tempsense);
	signal(SIGTERM, phy_tempsense_exit);
	signal(SIGUSR1, update_dutycycle);

	duty_cycle = nvram_get_int("fanctrl_dutycycle");

	if ((duty_cycle < 0) || (duty_cycle > 10))
		duty_cycle = 0; // switch to auto

	/* note user about fan control in syslog on start */
	if (duty_cycle)
		syslog(LOG_INFO, "Manual mode set for fan control!!! Possible router overheat!!! Use: 'nvram unset fanctrl_dutycycle; nvram commit' to reset. Fan speed (fanctrl_dutycycle) = %d", duty_cycle);
	else {
		t1 = (nvram_get_int("fanctrl_t1") ? nvram_get_int("fanctrl_t1") : TEMP_MIN / 2 + 20);
		t2 = (nvram_get_int("fanctrl_t2") ? nvram_get_int("fanctrl_t2") : TEMP_2 / 2 + 20);
		t3 = (nvram_get_int("fanctrl_t3") ? nvram_get_int("fanctrl_t3") : TEMP_3 / 2 + 20);
		t4 = (nvram_get_int("fanctrl_t4") ? nvram_get_int("fanctrl_t4") : TEMP_4 / 2 + 20);
		t5 = (nvram_get_int("fanctrl_t5") ? nvram_get_int("fanctrl_t5") : TEMP_MAX / 2 + 20);

		if ((t2 <= t1) || (t1 < 20)) {
			t1 = TEMP_MIN / 2 + 20;
			t2 = TEMP_2 / 2 + 20;
		}
		if (t3 <= t2)
			t3 = TEMP_3 / 2 + 20;
		if (t4 <= t3)
			t4 = TEMP_4 / 2 + 20;
		if (t5 <= t4)
			t5 = TEMP_MAX / 2 + 20;

		syslog(LOG_INFO, "Fan works in auto mode, switch wl temps (Celsius): %d %d %d %d %d", t1, t2, t3, t4, t5);
	}

	alarmtimer(0, FAN_NORMAL_PERIOD);

	/* most of time it goes to sleep */
	while (1) {
		pause();
	}

	return 0;
}

void restart_fanctrl()
{
	kill_pidfile_s("/var/run/phy_tempsense.pid", SIGUSR1);
}
