/*
 *
 * Tomato Firmware
 *
 * Fixes/updates (C) 2018 - 2023 pedro
 *
 */


#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include "tomato.h"


static float g_cpu_used;
static unsigned int cpu_num;

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

static void cal_occupy(struct occupy *, struct occupy *);
static void get_occupy(struct occupy *);

void trim(char *str)
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
int strncmp_ex(char *str1, char *str2)
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
#ifdef TCONFIG_BCMARM
			if (strncmp_ex(title, "Processor") == 0) {
#else
			if (strncmp_ex(title, "system type") == 0) {
#endif
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
			if (strncmp_ex(title, "cpu MHz") == 0)
					strlcpy(cpuclk, value, buf_cpuclk_sz);

			if (strncmp_ex(title, "cpu Temp") == 0)
				strlcpy(cputemp, value, buf_cputemp_sz);
		}
		fclose(fd);
	}
#endif
}

float get_cpupercent()
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
	double scale;

#ifdef TCONFIG_BCMARM
	od = (double) (o->user + o->nice + o->system + o->idle + o->io + o->irq + o->sirq);
	nd = (double) (n->user + n->nice + n->system + n->idle + n->io + n->irq + n->sirq);
#else
	od = (double) (o->user + o->nice + o->system + o->idle);
	nd = (double) (n->user + n->nice + n->system + n->idle);
#endif

	scale = 100.0 / (float)(nd-od);
	id = (double) (n->user - o->user);
	sd = (double) (n->system - o->system);
	g_cpu_used = ((sd+id) * 100.0) / (nd-od);
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
