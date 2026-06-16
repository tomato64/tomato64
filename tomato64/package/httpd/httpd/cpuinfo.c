/*
 *
 * Tomato Firmware
 *
 * Fixes/updates (C) 2018 - 2026 pedro
 * https://freshtomato.org/
 *
 */


#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#ifdef TOMATO64
#include <sys/sysinfo.h>
#endif /* TOMATO64 */
#include "tomato.h"


#if 0
static float g_cpu_used;
static unsigned int cpu_num;
#endif /* 0 */

struct occupy
{
	char name[20];
	unsigned int user;
	unsigned int nice;
	unsigned int system;
	unsigned int idle;
#ifdef TCONFIG_BCMARM
	unsigned int io;
	unsigned int irq;
	unsigned int sirq;
#endif
};

#if 0
static void cal_occupy(struct occupy *, struct occupy *);
static void get_occupy(struct occupy *);
#endif /* 0 */

static void trim(char *str)
{
	char *copied, *tail = NULL;
	if (str == NULL)
		return ;

	for (copied = str; *str; str++) {
		if ((unsigned char)*str > 0x20) {
			*copied++ = *str;
			tail = copied;
		}
		else {
			if (tail)
				*copied++ = *str;
		}
	}

	if (tail)
		*tail = 0;
	else
		*copied = 0;
	return;
}

/*
 * - ARM:
 * Processor               : ARMv7 Processor rev 0 (v7l
 * - MIPS:
 * system type             : Broadcom BCM5354 chip rev 2 pkg 0
 *
 * processor               : 0
 * cpu model               : BCM3302 V2.9
 * BogoMIPS                : 238.38
 */
static int strncmp_ex(char *str1, char *str2)
{
	return strncmp(str1, str2, strlen(str2));
}

#ifdef TCONFIG_BCMARM
void get_cpuinfo(char *system_type, const size_t buf_system_type_sz, char *cpuclk, const size_t buf_cpuclk_sz, char *cputemp, const size_t buf_cputemp_sz)
#else
void get_cpuinfo(char *system_type, const size_t buf_system_type_sz, char *cpuclk, const size_t buf_cpuclk_sz)
#endif
{
	FILE *fd;
	char *next;
	char buff[1024];
	char title[128], value[512];
#ifdef TOMATO64_X86_64
	double mhz_sum = 0.0;
	int mhz_count = 0;
#endif /* TOMATO64_X86_64 */

	memset(buff, 0, sizeof(buff));
	if ((fd = fopen("/proc/cpuinfo", "r"))) {
		while (fgets(buff, sizeof(buff), fd)) {
			next = buff;
			memset(title, 0, sizeof(title));
			strlcpy(title, strsep(&next, ":"), sizeof(title));
			if (next == NULL)
				continue;

			memset(value, 0, sizeof(value));
			strlcpy(value, next, sizeof(value));
			trim(value);
#ifndef TOMATO64
#ifdef TCONFIG_BCMARM
			if (strncmp_ex(title, "Processor") == 0) {
#else
			if (strncmp_ex(title, "system type") == 0) {
#endif
#endif /* TOMATO64 */
#ifdef TOMATO64
			if (strncmp_ex(title, "vendor_id") == 0) {
#endif /* TOMATO64 */
#ifndef TCONFIG_BCMARM
				if (strncmp_ex(value, "Broadcom BCM5354") == 0)
					strlcpy(system_type, "Broadcom BCM5354", buf_system_type_sz);
				else
#endif
					strlcpy(system_type, value, buf_system_type_sz);
			}
#ifndef TCONFIG_BCMARM
			if (strncmp_ex(title, "cpu MHz") == 0)
				strlcpy(cpuclk, value, buf_cpuclk_sz);
#endif
#ifdef TOMATO64
			if (strncmp_ex(title, "cpu MHz") == 0) {
				strlcpy(cpuclk, value, buf_cpuclk_sz);
#ifdef TOMATO64_X86_64
				mhz_sum += atof(value);
				mhz_count++;
#endif /* TOMATO64_X86_64 */
			}
#endif /* TOMATO64 */
		}
		fclose(fd);
	}

#ifdef TCONFIG_BCMARM
	memset(buff, 0, sizeof(buff));
	system("/usr/sbin/sysinfo-helper");
	if ((fd = fopen("/tmp/sysinfo-helper", "r"))) {
		while (fgets(buff, sizeof(buff), fd)) {
			next = buff;
			memset(title, 0, sizeof(title));
			strlcpy(title, strsep(&next, ":"), sizeof(title));
			if (next == NULL)
				continue;

			memset(value, 0, sizeof(value));
			strlcpy(value, next, sizeof(value));
			trim(value);
#ifndef TOMATO64
			if (strncmp_ex(title, "cpu MHz") == 0)
					strlcpy(cpuclk, value, buf_cpuclk_sz);
#endif /* TOMATO64 */

			if (strncmp_ex(title, "cpu Temp") == 0)
				strlcpy(cputemp, value, buf_cputemp_sz);
		}
		fclose(fd);
	}
#endif
#ifdef TOMATO64_X86_64
	/* Report the average of the per-core "cpu MHz" values from /proc/cpuinfo */
	if (mhz_count > 0)
		snprintf(cpuclk, buf_cpuclk_sz, "%d", (int)(mhz_sum / mhz_count + 0.5));

	/* CPU package temperature straight from hwmon sysfs (coretemp on Intel, k10temp on AMD) */
	cputemp[0] = '\0';
	{
		int i;
		char path[128], namebuf[32];

		for (i = 0; i < 32; i++) {
			FILE *nf;

			snprintf(path, sizeof(path), "/sys/class/hwmon/hwmon%d/name", i);
			if (!(nf = fopen(path, "r")))
				continue;	/* numbering may have gaps; keep scanning */

			namebuf[0] = '\0';
			if (fgets(namebuf, sizeof(namebuf), nf))
				namebuf[strcspn(namebuf, "\n")] = 0;
			fclose(nf);

			if (strcmp(namebuf, "coretemp") == 0 || strcmp(namebuf, "k10temp") == 0) {
				FILE *tf;

				snprintf(path, sizeof(path), "/sys/class/hwmon/hwmon%d/temp1_input", i);
				if ((tf = fopen(path, "r"))) {
					char tbuf[16];
					if (fgets(tbuf, sizeof(tbuf), tf)) {
						long milli = atol(tbuf);
						if (milli > 0)
							snprintf(cputemp, buf_cputemp_sz, "%ld", (milli + 500) / 1000);
					}
					fclose(tf);
				}
				break;
			}
		}
	}
#endif /* TOMATO64_X86_64 */
#ifdef TOMATO64_ARM64
#if TOMATO64_RPI4
	strlcpy(system_type, "Broadcom BCM2711", buf_system_type_sz);
	strlcpy(cpuclk, "1800", buf_cpuclk_sz);
#elif TOMATO64_R6S
	strlcpy(system_type, "Rockchip RK3588S", buf_system_type_sz);
	strlcpy(cpuclk, "2400", buf_cpuclk_sz);
#elif TOMATO64_R5S
	strlcpy(system_type, "Rockchip RK3568", buf_system_type_sz);
	strlcpy(cpuclk, "2000", buf_cpuclk_sz);
#elif TOMATO64_R76S
	strlcpy(system_type, "Rockchip RK3576", buf_system_type_sz);
	strlcpy(cpuclk, "2200", buf_cpuclk_sz);
#elif TOMATO64_MT3600BE
	strlcpy(system_type, "MediaTek MT7987A", buf_system_type_sz);
	strlcpy(cpuclk, "2000", buf_cpuclk_sz);
#elif TOMATO64_ARMSR
	strlcpy(system_type, "ARM64 SystemReady", buf_system_type_sz);
	/* arm64 has no "cpu MHz" in /proc/cpuinfo; read the live clock from
	 * cpufreq when a driver is present (real HW / CPPC-capable VMs). Leave
	 * empty otherwise (e.g. a plain VM) - the status page hides the row. */
	cpuclk[0] = '\0';
	{
		FILE *cf = fopen("/sys/devices/system/cpu/cpu0/cpufreq/scaling_cur_freq", "r");
		if (cf) {
			char fbuf[32];
			if (fgets(fbuf, sizeof(fbuf), cf)) {
				long khz = atol(fbuf);
				if (khz > 0)
					snprintf(cpuclk, buf_cpuclk_sz, "%ld", khz / 1000);
			}
			fclose(cf);
		}
	}
#else
	strlcpy(system_type, "MediaTek Filogic 830", buf_system_type_sz);
	strlcpy(cpuclk, "2000", buf_cpuclk_sz);
#endif
	FILE *f;
	char buffer[8];
	int temp;

#if defined(TOMATO64_R6S) || defined(TOMATO64_R76S)
	const char cmd[] = "sensors -A package_thermal-virtual-0 | grep 'temp1' | awk '{print $2}' | sed 's/+//; s/°C//'";
#else
	const char cmd[] = "sensors -A cpu_thermal-virtual-0 | grep 'temp1' | awk '{print $2}' | sed 's/+//; s/°C//'";
#endif

	if ((f = popen(cmd, "r"))) {
		if (fgets(buffer, sizeof(buffer), f) != NULL) {
			buffer[strcspn(buffer, "\n")] = 0;
			temp = (int)(atof(buffer) + 0.5);
			snprintf(cputemp, buf_cputemp_sz, "%d", temp);
		}
		pclose(f);
	}
	else {
		snprintf(cputemp, buf_cputemp_sz, "");
	}
#endif /* TOMATO64_ARM64 */
#ifdef TOMATO64_ARM
	strlcpy(system_type, "Broadcom BCM47xx/53xx", buf_system_type_sz);
	strlcpy(cpuclk, "1000", buf_cpuclk_sz);

	FILE *f;
	char buffer[8];
	int temp;
	const char cmd[] = "sensors -A cpu_thermal-virtual-0 | grep 'temp1' | awk '{print $2}' | sed 's/+//; s/\xc2\xb0C//'";
	if ((f = popen(cmd, "r"))) {
		if (fgets(buffer, sizeof(buffer), f) != NULL) {
			buffer[strcspn(buffer, "\n")] = 0;
			temp = (int)(atof(buffer) + 0.5);
			snprintf(cputemp, buf_cputemp_sz, "%d", temp);
		}
		pclose(f);
	}
	else {
		snprintf(cputemp, buf_cputemp_sz, "");
	}
#endif /* TOMATO64_ARM */
}

#ifdef TOMATO64
void get_cpumodel(char *cpumodel, const size_t buf_cpumodel_sz)
{
#ifdef TOMATO64_X86_64
	FILE *fd;
	char *next;
	char buff[1024];
	char title[128], value[512];

	memset(buff, 0, sizeof(buff));
	if ((fd = fopen("/proc/cpuinfo", "r"))) {
		while (fgets(buff, sizeof(buff), fd)) {
			next = buff;
			memset(title, 0, sizeof(title));
			strlcpy(title, strsep(&next, ":"), sizeof(title));
			if (next == NULL)
				continue;

			memset(value, 0, sizeof(value));
			strlcpy(value, next, sizeof(value));
			trim(value);

			if (strncmp_ex(title, "model name") == 0) {
				strlcpy(cpumodel, value, buf_cpumodel_sz);
			}
		}
		fclose(fd);
	}
#endif /* TOMATO64_X86_64 */
#ifdef TOMATO64_ARM64
#if TOMATO64_RPI4
	strlcpy(cpumodel, "ARM Cortex-A72", buf_cpumodel_sz);
#elif TOMATO64_R6S
	strlcpy(cpumodel, "ARM Cortex-A76 / A55", buf_cpumodel_sz);
#elif TOMATO64_R5S
	strlcpy(cpumodel, "ARM Cortex-A55", buf_cpumodel_sz);
#elif TOMATO64_R76S
	strlcpy(cpumodel, "ARM Cortex-A72 / A53", buf_cpumodel_sz);
#elif TOMATO64_ARMSR
	{
		/* armsr runs on VMs and varied UEFI hardware; read the real core
		 * name from lscpu, fall back to a generic label when unknown */
		FILE *fcpu;
		char line[128];

		cpumodel[0] = '\0';
		if ((fcpu = popen("lscpu | sed -n 's/^Model name:[[:space:]]*//p' | head -1", "r"))) {
			if (fgets(line, sizeof(line), fcpu) != NULL) {
				line[strcspn(line, "\n")] = 0;
				if (line[0] != '\0' && strcmp(line, "-") != 0)
					strlcpy(cpumodel, line, buf_cpumodel_sz);
			}
			pclose(fcpu);
		}
		if (cpumodel[0] == '\0')
			strlcpy(cpumodel, "Generic ARM64", buf_cpumodel_sz);
	}
#else
	strlcpy(cpumodel, "MediaTek MT7986AV (Cortex-A53)", buf_cpumodel_sz);
#endif
#endif /* TOMATO64_ARM64 */
#ifdef TOMATO64_ARM
	strlcpy(cpumodel, "ARM Cortex-A9", buf_cpumodel_sz);
#endif /* TOMATO64_ARM */
}
#endif /* TOMATO64 */

#if 0
float get_cpupercent(void)
{
	struct occupy ocpu[10];
	struct occupy ncpu[10];
	unsigned int i;

	//cpu_num = sysconf(_SC_NPROCESSORS_ONLN);
	cpu_num = 1;
	get_occupy(ocpu);
	sleep(1);
	get_occupy(ncpu);
	for (i = 0; i < cpu_num; i++) {
		cal_occupy(&ocpu[i], &ncpu[i]);
	}

	return g_cpu_used;
}

static void cal_occupy (struct occupy *o, struct occupy *n)
{
	double od, nd;
	double id, sd;

#ifdef TCONFIG_BCMARM
	od = (double) (o->user + o->nice + o->system + o->idle + o->io + o->irq + o->sirq);
	nd = (double) (n->user + n->nice + n->system + n->idle + n->io + n->irq + n->sirq);
#else
	od = (double) (o->user + o->nice + o->system + o->idle);
	nd = (double) (n->user + n->nice + n->system + n->idle);
#endif

	id = (double) (n->user - o->user);
	sd = (double) (n->system - o->system);
	g_cpu_used = ((sd + id) * 100.0) / (nd - od);
}

static void get_occupy (struct occupy *o)
{
	FILE *fd;
	unsigned int n;
	char buff[1024];

	if ((fd = fopen("/proc/stat", "r"))) {
		fgets(buff, sizeof(buff), fd);

		for (n = 0; n < cpu_num; n++) {
			fgets(buff, sizeof(buff), fd);
#ifdef TCONFIG_BCMARM
			sscanf(buff, "%s %u %u %u %u %u %u %u", o[n].name, &o[n].user, &o[n].nice, &o[n].system, &o[n].idle, &o[n].io, &o[n].irq, &o[n].sirq);
#else
			sscanf(buff, "%s %u %u %u %u", o[n].name, &o[n].user, &o[n].nice, &o[n].system, &o[n].idle);
#endif
		}
		fclose(fd);
	}
}
#endif /* 0 */
