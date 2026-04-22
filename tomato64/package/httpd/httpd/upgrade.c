/*
 *
 * Tomato Firmware
 * Copyright (C) 2006-2009 Jonathan Zarate
 *
 * Fixes/updates (C) 2018 - 2026 pedro
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
#include <dirent.h>

/* Maximum firmware image size: 64MB. Rejects absurdly large uploads
 * before allocating memory or touching flash.
 */
#ifndef TOMATO64
#define FIRMWARE_MAX_SIZE (64 * 1024 * 1024)
#else
#define FIRMWARE_MAX_SIZE (1024 * 1024 * 1024)
#endif /* TOMATO64 */


void copy_css_files(void)
{
	DIR *d;
	struct dirent *de;
	char src[128];
	const char *name;
	int len;

	if ((d = opendir("/www")) == NULL)
		return;

	while ((de = readdir(d)) != NULL) {
		name = de->d_name;

		if (de->d_type == DT_DIR)
			continue;

		snprintf(src, sizeof(src), "/www/%s", name);

		if (de->d_type == DT_UNKNOWN) {
			struct stat st;
			if ((stat(src, &st) != 0) || (!S_ISREG(st.st_mode)))
				continue;
		}

		len = strlen(name);
		if (len > 4 && strcmp(name + len - 4, ".css") == 0) {
			eval("cp", src, "/tmp");
		}
	}
	closedir(d);
}

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
	pid_t pid = -1;
	int fd = -1, m, retries = 100;
	int status;
	unsigned int reset;
	const char *error = "Error reading file";
	int complete = 1;
#ifdef TOMATO64
#ifndef TOMATO64_BCM53XX
	struct statvfs disk;
	statvfs("/", &disk);
	float f_bavail = disk.f_bavail;
	float f_frsize = disk.f_frsize;
	float available_space = f_bavail * f_frsize;
#endif /* TOMATO64_BCM53XX */
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


	/* validate session */
	check_id(url);

	reset = (strcmp(webcgi_safeget("_reset", "0"), "1") == 0);

	/* Skip HTTP headers */
	if (!skip_header(&len))
		goto ERROR;

	/* sanity check file size: must be between 1MB and FIRMWARE_MAX_SIZE */
	if (len < (1 * 1024 * 1024)) {
		error = "Invalid file: too small";
		goto ERROR;
	}
	if (len > FIRMWARE_MAX_SIZE) {
		error = "Invalid file: too large";
		goto ERROR;
	}

#ifdef TOMATO64
#ifndef TOMATO64_BCM53XX
	if ((float) len > available_space) {
		error = "Insufficient disk space to extract update";
		goto ERROR;
	}
#endif /* TOMATO64_BCM53XX */
#endif /* TOMATO64 */

	/*
	 * avoid large malloc just to test memory availability.
	 * Instead, rely on streaming and enforce a reasonable upper bound if needed.
	 */

	/* from this point forward, system will reboot */
	rboot = 1;

	/* ignore signals during upgrade */
	signal(SIGTERM, SIG_IGN);
	signal(SIGINT, SIG_IGN);
	signal(SIGHUP, SIG_IGN);
	signal(SIGQUIT, SIG_IGN);

	/* stop services and prepare system */
	prepare_upgrade();

#ifdef TOMATO64_X86_64
	eval("mount_nvram");
#endif /* TOMATO64_X86_64 */

	/* copy required UI assets to tmpfs (survive upgrade process) */
	eval("cp", "/www/reboot.asp", "/www/favicon.ico", "/www/tomatousb_bg.png", "/tmp");
	eval("cp", "/www/asus-bg.png", "/tmp");
	copy_css_files();
#ifdef TOMATO64_X86_64
	eval("cp", "/www/reboot-fast.asp", "/tmp");
#endif /* TOMATO64_X86_64 */

	led(LED_DIAG, 1);
#ifdef TOMATO64
	led_state_upgrade();
#endif /* TOMATO64 */

	/*
	 * create unique temporary path.
	 * mkstemp creates a file - we immediately unlink it and reuse path for FIFO.
	 */
	if ((fd = mkstemp(fifo)) < 0) {
		error = "Unable to create file";
		goto ERROR2;
	}
	close(fd);
	fd = -1;
	unlink(fifo);

	/* create FIFO for streaming firmware to mtd-write */
	if (mkfifo(fifo, S_IRWXU) < 0) {
		error = "Unable to create fifo";
		goto ERROR2;
	}

	/*
	 * start flashing process asynchronously.
	 * mtd-write will open FIFO for reading.
	 */
	if (_eval(args, ">/tmp/.mtd-write", 0, &pid) != 0) {
		error = "Unable to start flash program";
		goto ERROR2;
	}

	/*
	 * open FIFO for writing.
	 * this can block until reader is ready, so retry with timeout.
	 */
	while (retries-- > 0) {
		f = fopen(fifo, "w");
		if (f)
			break;

		usleep(10000); /* 10ms */
	}

	if (!f) {
		error = "Unable to open fifo";
		goto ERROR2;
	}

	/*
	 * stream POST body directly into FIFO.
	 * note: boundary is included, but mtd-write uses trx length.
	 */
	while (len > 0) {
		m = web_read(buf, MIN((unsigned int)len, sizeof(buf)));

		if (m <= 0) {
			complete = 0;
			goto ERROR2;
		}

		len -= m;

		if (safe_fwrite(buf, 1, m, f) != m) {
			complete = 0;
			error = "Error writing to pipe";
			goto ERROR2;
		}
	}

	error = NULL;

ERROR2:
	/* close FIFO stream */
	if (f)
		fclose(f);

	/* wait for flashing process */
	if (pid != -1) {
		while (waitpid(pid, &status, 0) < 0) {
			if (errno != EINTR)
				break;
		}

		/* if transfer completed but flashing failed, propagate error */
		if (error == NULL) {
			if (!complete)
				error = "Incomplete upload";
			else if (!WIFEXITED(status) || WEXITSTATUS(status) != 0)
				error = "Flash failed";
		}
	}

	/* optional NVRAM erase after successful flash */
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

	/* mtd-write output takes precedence over generic error */
	if (resmsg_fread("/tmp/.mtd-write"))
		error = NULL;

ERROR:
	/* cleanup FIFO */
	if (fifo[0])
		unlink(fifo);

	/* report error to GUI */
	if (error)
		resmsg_set(error);

	if (reset)
		webcgi_set("resreset", "1");

	/* consume any remaining POST data */
	web_eat(len);
}

void wo_flash(char *url)
{
#ifdef TOMATO64_X86_64
	unsigned int fastreboot;
	fastreboot = (strcmp(webcgi_safeget("_fastreboot", "0"), "1") == 0);
#endif /* TOMATO64_X86_64 */

	if (rboot) {
		set_action(ACT_REBOOT);
		sync();
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

#ifdef TOMATO64
		sync();
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
