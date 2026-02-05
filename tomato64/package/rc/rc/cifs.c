/*
 *
 * Tomato Firmware
 * Copyright (C) 2006-2009 Jonathan Zarate
 *
 * Fixes/updates (C) 2018 - 2026 pedro
 * https://freshtomato.org/
 *
 */


#include "rc.h"

#include <sys/mount.h>
#include <sys/stat.h>
#include <sys/statfs.h>

#ifndef MNT_DETACH
#define MNT_DETACH	0x00000002
#endif


void start_cifs(void)
{
	xstart("mount-cifs", "-m");
}

void stop_cifs(void)
{
	killall("mount-cifs", SIGTERM);
	eval("mount-cifs", "-u");
}

int mount_cifs_main(int argc, char *argv[])
{
	char s[512];
	char opt[512];
	char mpath[32];
	int i, j;
	int try;
	int first;
	char *on, *unc, *user, *pass, *dom, *exec, *servern, *sec, *custom;
	int done[3];
	struct statfs sf;

	if (argc == 2) {
		if (strcmp(argv[1], "-m") == 0) {
			done[1] = 0;
			done[2] = 0;
			first = 1;
			try = 0;
			while (1) {
				for (i = 1; i <= 2; ++i) {
					if (done[i])
						continue;

					done[i] = 2;

					snprintf(s, sizeof(s), "cifs%d", i);
					strlcpy(s, nvram_safe_get(s), sizeof(s));
					if ((vstrsep(s, "<", &on, &unc, &user, &pass, &dom, &exec, &servern, &sec) < 8) || (*on != '1'))
						continue;

					custom = nvram_safe_get("cifs_opts");

					done[i] = 0;

					if (first) {
						notice_set("cifs", "Mounting...");
						modprobe("cifs");
						first = 0;
						if (nvram_get_int("cifs_dbg") > 0)
							f_write_string("/proc/fs/cifs/cifsFYI", nvram_safe_get("cifs_dbg"), 0, 0);
					}

#ifndef TOMATO64
					j = snprintf(opt, sizeof(opt), "sep=<unc=%s", unc);
					if (*user)
						j += snprintf(opt + j, sizeof(opt) - j, "<user=%s", user);
					if (*pass)
						j += snprintf(opt + j, sizeof(opt) - j, "<pass=%s", pass);
					if (*dom)
						j += snprintf(opt + j, sizeof(opt) - j, "<dom=%s", dom);
					if (*servern)
						j += snprintf(opt + j, sizeof(opt) - j, "<servern=%s", servern);
					if (*sec)
						j += snprintf(opt + j, sizeof(opt) - j, "<sec=%s", sec);
					if (*custom)
						j += snprintf(opt + j, sizeof(opt) - j, "<%s", custom);
#else
					j = snprintf(opt, sizeof(opt), "vers=1.0");
					if (*user)
						j += snprintf(opt + j, sizeof(opt) - j, ",user=%s", user);
					if (*pass)
						j += snprintf(opt + j, sizeof(opt) - j, ",pass=%s", pass);
					if (*dom)
						j += snprintf(opt + j, sizeof(opt) - j, ",dom=%s", dom);
					if (*servern)
						j += snprintf(opt + j, sizeof(opt) - j, ",servern=%s", servern);
					if (*sec)
						j += snprintf(opt + j, sizeof(opt) - j, ",sec=%s", sec);
					if (*custom)
						j += snprintf(opt + j, sizeof(opt) - j, ",%s", custom);
#endif /* TOMATO64 */

					snprintf(mpath, sizeof(mpath), "/cifs%d", i);
					umount(mpath);
#ifndef TOMATO64
					if (mount("-", mpath, "cifs", (MS_NOATIME | MS_NODIRATIME), opt) != 0)
						continue;
#else
					if (mount(unc, mpath, "cifs", (MS_NOATIME | MS_NODIRATIME), opt) != 0)
						continue;
#endif /* TOMATO64 */

					done[i] = 1;
					if (try > 12)
						try = 12; /* 1 min */

					if (*exec) {
						chdir(mpath);
						system(exec);
					}
					run_userfile(mpath, ".autorun", mpath, 3);
				}
				if ((done[1]) && (done[2])) {
					notice_set("cifs", "");
					return 0;
				}

				sleep(5 * ++try);
				if (try == 24) { /* 2 min */
					snprintf(s, sizeof(s), "Error mounting CIFS #%s. Still trying... ", (done[1] == done[2]) ? "1 and #2" : ((done[1] == 0) ? "1" : "2"));
					notice_set("cifs", s);
				}
				else if (try > 180) /* 15 mins */
					try = 180;
			}

			return 1;
		}
		if (strcmp(argv[1], "-u") == 0) {
			for (i = 1; i <= 2; ++i) {
				snprintf(mpath, sizeof(mpath), "/cifs%d", i);
				if ((statfs(mpath, &sf) == 0) && (sf.f_type != 0x73717368
#if defined(TCONFIG_BLINK) || defined(CONFIG_BCMWL6)
				    && sf.f_type != 0x71736873
#endif
				)) {
					/* is mounted */
					run_userfile(mpath, ".autostop", mpath, 5);
					run_nvscript("script_autostop", mpath, 5);
				}
				umount2(mpath, MNT_DETACH);
			}
			modprobe_r("cifs");
			notice_set("cifs", "");
			return 0;
		}
	}

	usage_exit(argv[0], "-m|-u");
	return 1;
}
