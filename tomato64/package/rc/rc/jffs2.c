/*

	Tomato Firmware
	Copyright (C) 2006-2009 Jonathan Zarate

*/


#include "rc.h"

#include <sys/mount.h>
#include <sys/stat.h>
#include <sys/statfs.h>
#include <errno.h>

#ifndef MNT_DETACH
#define MNT_DETACH	0x00000002
#endif

#ifdef TCONFIG_JFFSV1
#define JFFS_NAME	"jffs"
#else
#define JFFS_NAME	"jffs2"
#endif

#ifdef TCONFIG_BRCM_NAND_JFFS2
#define JFFS2_PARTITION	"brcmnand"
#else
#ifdef TCONFIG_BCMARM
#define JFFS2_PARTITION	"brcmnand"
#else
#define JFFS2_PARTITION	"jffs2"
#endif /* TCONFIG_BCMARM */
#endif /* TCONFIG_BRCM_NAND_JFFS2 */

//#define TEST_INTEGRITY


static void error(const char *message)
{
	char s[512];

	snprintf(s, sizeof(s), "Error %s JFFS. Check the logs to see if they contain more details about this error.", message);
	notice_set("jffs", s);
}

void start_jffs2(void)
{
	int format = 0;
	int i = 0;
	int size;
	int part;
	char s[256];
	const char *p;
	struct statfs sf;

#ifdef TCONFIG_BCMARM
	int model = get_model();

	if (model == MODEL_UNKNOWN) { /* check router */
		/* router model unknown! Stop here to avoid overwriting board_data for some router */
		error("because router model unknown - can not enable");
		return;
	}
#endif

	if (!nvram_match("jffs2_on", "1")) {
		notice_set("jffs", "");
		return;
	}

	while(1) {
		if (wait_action_idle(10))
			break;
		else
			i++;

		if (i >= 10) {
			error("busy");
			return;
		}
	}

	if (!mtd_getinfo(JFFS2_PARTITION, &part, &size)) {
		error("getting info from");
		return;
	}

	if (nvram_match("jffs2_format", "1")) {
		nvram_set("jffs2_format", "0");
		nvram_commit_x();
		if (mtd_erase(JFFS2_PARTITION)) {
			error("formatting");
			return;
		}
		format = 1;
	}

	memset(s, 0, 256);
	snprintf(s, sizeof(s), "%d", size);
	p = nvram_safe_get("jffs2_size");
	if ((!*p) || (strcmp(p, s) != 0)) {
		if (format) {
			nvram_set("jffs2_size", s);
			nvram_commit_x();
		}
		else if ((p != NULL) && (*p != 0)) {
			error("verifying known size of (unformatted?)");
			return;
		}
	}

	if ((statfs("/jffs", &sf) == 0) && (sf.f_type != 0x73717368)
#if defined(TCONFIG_BCMARM) || defined(TCONFIG_BLINK)
	    && (sf.f_type != 0x71736873)
#endif
	) {
		/* already mounted */
		notice_set("jffs", format ? "Formatted" : "Loaded");
		return;
	}

	if (!mtd_unlock(JFFS2_PARTITION)) {
		error("unlocking");
		return;
	}

	modprobe(JFFS_NAME);

	memset(s, 0, 256);
	snprintf(s, sizeof(s), MTD_BLKDEV(%d), part);

	if (mount(s, "/jffs", JFFS_NAME, MS_NOATIME, "") != 0) {
		if (mtd_erase(JFFS2_PARTITION)) {
			error("formatting");
			return;
		}
		format = 1;

		if (mount(s, "/jffs", JFFS_NAME, MS_NOATIME, "") != 0) {
			modprobe_r(JFFS_NAME);
			error("mounting 2nd time");
			return;
		}
	}

#ifdef TEST_INTEGRITY
	int test;

	if (format) {
		if (f_write("/jffs/.tomato_do_not_erase", &size, sizeof(size), 0, 0) != sizeof(size)) {
			stop_jffs2();
			error("setting integrity test for");
			return;
		}
	}

	if ((f_read("/jffs/.tomato_do_not_erase", &test, sizeof(test)) != sizeof(test)) || (test != size)) {
		stop_jffs2();
		error("testing integrity of");
		return;
	}
#endif

	notice_set("jffs", format ? "Formatted" : "Loaded");

	if ((p = nvram_safe_get("jffs2_exec")) && (*p)) {
		chdir("/jffs");
		system(p);
		chdir("/");
	}
	run_userfile("/jffs", ".autorun", "/jffs", 3);
}

void stop_jffs2(void)
{
	struct statfs sf;

	if (!wait_action_idle(10))
		return;

	if ((statfs("/jffs", &sf) == 0) && (sf.f_type != 0x73717368)
#if defined(TCONFIG_BCMARM) || defined(TCONFIG_BLINK)
	    && (sf.f_type != 0x71736873)
#endif
	) {
		/* is mounted */
		run_userfile("/jffs", ".autostop", "/jffs", 5);
		run_nvscript("script_autostop", "/jffs", 5);
	}

	notice_set("jffs", "Stopped");
	umount2("/jffs", MNT_DETACH);
	modprobe_r(JFFS_NAME);
}
