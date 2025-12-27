/*
 *
 * Tomato Firmware
 * Copyright (C) 2006-2009 Jonathan Zarate
 *
 * Fixes/updates (C) 2018 - 2025 pedro
 * https://freshtomato.org/
 *
 */


#include "tomato.h"

#include <fcntl.h>
#include <errno.h>
#include <sys/stat.h>
#include <sys/statfs.h>
#include <sys/wait.h>
#include <typedefs.h>
#include <sys/reboot.h>


void prepare_upgrade(void)
{
	int n;

	/* stop non-essential stuff & free up some memory */
	exec_service("upgrade-start");
	for (n = 60; n > 0; --n) { /* wait 60 seconds for completion */
		sleep(1);

		if (nvram_match("action_service", "")) /* this is cleared at the end */
			break;
	}

	nvram_set("os_version_last", tomato_shortver);
	nvram_commit();

	unlink("/var/log/messages");
	unlink("/var/log/messages.0");
	sync();
}

void wi_upgrade(char *url, int len, char *boundary)
{
	FILE *f = NULL;
	char fifo[] = "/tmp/flashXXXXXX";
	uint8 buf[1024];
	char *tmp;
	pid_t pid = -1;
	int fd, m;
	unsigned int reset;
	const char *error = "Error reading file";
#ifdef TOMATO64
	struct statvfs disk;
	statvfs("/", &disk);
	float f_bavail = disk.f_bavail;
	float f_frsize = disk.f_frsize;
	float available_space = f_bavail * f_frsize;
#endif /* TOMATO64 */
#ifndef TOMATO64
#ifdef TCONFIG_BCMARM
	char *args[] = { "mtd-write2", fifo, "linux", NULL };
#else
	char *args[] = { "mtd-write", "-w", "-i", fifo, "-d", "linux", NULL };
#endif
#else /* TOMATO64 */
	char *args[] = { "upgrade", fifo, NULL };
#endif /* TOMATO64 */


	check_id(url);
	reset = (strcmp(webcgi_safeget("_reset", "0"), "1") == 0);
	memset(buf, 0, sizeof(buf)); /* reset */

	/* skip the rest of the header */
	if (!skip_header(&len))
		goto ERROR;

	if (len < (1 * 1024 * 1024)) {
		error = "Invalid file";
		goto ERROR;
	}

#ifdef TOMATO64
	if ((float) len > available_space) {
		error = "Insufficient disk space to extract update";
		goto ERROR;
	}
#endif /* TOMATO64 */

	if ((tmp = malloc(len)) == NULL) {
		error = "Not enough memory";
		goto ERROR;
	}
	free(tmp);

	/* -- anything after here ends in a reboot -- */

	rboot = 1;

	signal(SIGTERM, SIG_IGN);
	signal(SIGINT, SIG_IGN);
	signal(SIGHUP, SIG_IGN);
	signal(SIGQUIT, SIG_IGN);

	prepare_upgrade();

#ifdef TOMATO64_X86_64
	eval("mount_nvram");
#endif /* TOMATO64_X86_64 */

	/* copy to memory */
	system("cp /www/reboot.asp /tmp");
	system("cp /www/*.css /tmp");
	system("cp /www/favicon.ico /tmp");
	system("cp /www/asus-bg.png /tmp");
	system("cp /www/tomatousb_bg.png /tmp");
#ifdef TOMATO64_X86_64
	system("cp /www/reboot-fast.asp /tmp");
#endif /* TOMATO64_X86_64 */

	led(LED_DIAG, 1);

	/* create unique file */
	if ((fd = mkstemp(fifo) < 0)) {
		error = "Unable to create file";
		goto ERROR2;
	}
	unlink(fifo);

	/* create fifo */
	if (mkfifo(fifo, S_IRWXU) < 0) {
		error = "Unable to create fifo";
		goto ERROR2;
	}

	/* start mtd-write with the fifo */
	if (_eval(args, ">/tmp/.mtd-write", 0, &pid) != 0) {
		error = "Unable to start flash program";
		goto ERROR2;
	}

	/* open fifo for write */
	if ((f = fopen(fifo, "w")) == NULL) {
		error = "Unable to start pipe for mtd-write";
		goto ERROR2;
	}

	/* this will actually write the boundary, but since mtd-write uses trx length... */
	while (len > 0) {
		if ((m = web_read(buf, MIN((unsigned int)len, sizeof(buf)))) <= 0)
			goto ERROR2;

		len -= m;
		if (safe_fwrite(buf, 1, m, f) != m) {
			error = "Error writing to pipe";
			goto ERROR2;
		}
	}

	error = NULL;

ERROR2:
	if (f)
		fclose(f);

	if (fd != -1)
		close(fd);

	if (pid != -1)
		waitpid(pid, &m, 0);

	/* clear nvram? */
	if (error == NULL && reset) {
		set_action(ACT_IDLE);
#ifndef TOMATO64
#ifdef TCONFIG_BCMARM
		eval("mtd-erase2", "nvram");
#else
		eval("mtd-erase", "-d", "nvram");
#endif
#else /* TOMATO64 */
		system("rm /nvram/*");
#endif /* TOMATO64 */

	}
	set_action(ACT_REBOOT);

	/* display info on reboot page given by mtd-write (takes priority over regular error) */
	if (resmsg_fread("/tmp/.mtd-write"))
		error = NULL;

ERROR:
	/* erase flash file and free memory */
	if (fifo[0])
		unlink(fifo);

	if (error)
		resmsg_set(error);

	if (reset)
		webcgi_set("resreset", "1");

	web_eat(len);
}

void wo_flash(char *url)
{
#ifdef TOMATO64_X86_64
	unsigned int fastreboot;
	fastreboot = (strcmp(webcgi_safeget("_fastreboot", "0"), "1") == 0);
#endif /* TOMATO64_X86_64 */

	if (rboot) {
		sleep(1);
#ifdef TOMATO64_X86_64
		if (fastreboot)
			parse_asp("/tmp/reboot-fast.asp");
		else
#endif /* TOMATO64_X86_64 */
		parse_asp("/tmp/reboot.asp");
		web_close();

		if (nvram_get_int("remote_upgrade")) {
			killall("xl2tpd", SIGTERM);
			killall("pppd", SIGTERM);
		}

		sleep(2);

		sync();
		//kill(1, SIGTERM);
#ifdef TOMATO64
		system("/bin/umount -a -d -r");
#endif /* TOMATO64 */
#ifdef TOMATO64_X86_64
		if (fastreboot) {
			system("kexec -l /boot/bzImage --reuse-cmdline");
			system("kexec -e");
		} else {
			reboot(RB_AUTOBOOT);
		}
#else
		reboot(RB_AUTOBOOT);
#endif /* TOMATO64_X86_64 */

		exit(0);
	}

	parse_asp("error.asp");
}
