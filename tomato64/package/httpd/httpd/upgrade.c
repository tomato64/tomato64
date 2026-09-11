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
#ifdef TOMATO64
#include <sys/wait.h>
#endif /* TOMATO64 */
#include <typedefs.h>
#include <sys/reboot.h>
#ifdef TOMATO64
#include <dirent.h>
#endif /* TOMATO64 */

/* Maximum firmware image size: 64MB. Rejects absurdly large uploads
 * before allocating memory or touching flash.
 */
#ifndef TOMATO64
#define FIRMWARE_MAX_SIZE	(64 * 1024 * 1024)
#define FIRMWARE_TMP_RESERVE	(1 * 1024 * 1024)
#else
#define FIRMWARE_MAX_SIZE	(1024 * 1024 * 1024)
#endif /* TOMATO64 */

/* needed by logmsg() */
#define LOGMSG_DISABLE		DISABLE_SYSLOG_OSM
#define LOGMSG_NVDEBUG		"upgrade_debug"

#ifndef TOMATO64
static char upgrade_file[64];
static unsigned int upgrade_reset;
#endif /* TOMATO64 */


static int wait_upgrade_service(const char *action, int timeout)
{
	exec_service(action);

	while (timeout-- > 0) {
		if (nvram_match("action_service", ""))
			return 1;

		sleep(1);
	}

	return nvram_match("action_service", "");
}

#ifndef TOMATO64
static int firmware_tmp_space_ok(unsigned long image_len)
{
	struct statfs fs;
	unsigned long long available;
	unsigned long long needed;

	if (statfs("/tmp", &fs) != 0)
		return 0;

	available = (unsigned long long)fs.f_bavail * (unsigned long long)fs.f_bsize;
	needed = (unsigned long long)image_len + FIRMWARE_TMP_RESERVE;

	return (available >= needed);
}

static int validate_firmware(const char *file)
{
	int status;

#ifdef TCONFIG_BCMARM
	char *args[] = { "mtd-write2", "-c", (char *)file, "linux", NULL };
#else
	char *args[] = { "mtd-write", "-c", "-i", (char *)file, "-d", "linux", NULL };
#endif

	unlink("/tmp/.mtd-check");
	status = _eval(args, ">/tmp/.mtd-check", 0, NULL);

	return status;
}
#else
void copy_css_files(void)
{
	DIR *d;
	struct dirent *de;
	struct stat st;
	char src[128];
	const char *name;
	int len;

	if ((d = opendir("/www")) == NULL)
		return;

	while ((de = readdir(d)) != NULL) {
		name = de->d_name;

		if (de->d_type == DT_DIR)
			continue;

		len = strlen(name);
		if ((len <= 4) || (strcmp(name + len - 4, ".css") != 0))
			continue;

		snprintf(src, sizeof(src), "/www/%.122s", name);

		if (de->d_type == DT_UNKNOWN) {
			if ((stat(src, &st) != 0) || (!S_ISREG(st.st_mode)))
				continue;
		}

		eval("cp", src, "/tmp");
	}

	closedir(d);
}
#endif /* TOMATO64 */

int prepare_upgrade(void)
{
	int clean;

	/* stop non-essential stuff & free up some memory */
	clean = wait_upgrade_service("upgrade-start", 60);
	if (!clean)
		logmsg(LOG_WARNING, "upgrade-start did not complete before timeout; continuing");

	nvram_set("os_version_last", tomato_shortver);
	nvram_commit();

	sync();

	return clean;
}

int finalize_upgrade(void)
{
	/*
	 * Stop the listening httpd master without killing this request worker:
	 * it still has to run the validated MTD write after finalization.
	 * Drop the worker's inherited web_dir cwd as well so /opt or another
	 * USB-backed web root cannot keep storage busy during unmount.
	 */
	kill_pidfile_s("/var/run/httpd.pid", SIGTERM);
	chdir("/");

	return wait_upgrade_service("upgradefinalize-start", 60);
}

#ifndef TOMATO64
void wi_upgrade(char *url, int len, char *boundary)
{
	FILE *f = NULL;
	struct stat st;
	char end_boundary[256];
	char recv_boundary[256];
	char tail[2];
	uint8 buf[4096];
	unsigned long image_len;
	unsigned long remaining;
	int m;
	int fd = -1;
	int end_len;
	int status;
	unsigned int reset;
	const char *error = "Error reading file";

	upgrade_file[0] = '\0';
	upgrade_reset = 0;
	rboot = 0;

	/* validate session */
	check_id(url);

	reset = (strcmp(webcgi_safeget("_reset", "0"), "1") == 0);

	/* Skip multipart headers and leave len at the firmware payload. */
	if (!skip_header(&len))
		goto ERROR;

	if ((boundary == NULL) || (*boundary == '\0')) {
		error = "Invalid upload boundary";
		goto ERROR;
	}

	end_len = snprintf(end_boundary, sizeof(end_boundary), "\r\n--%s--", boundary);
	if ((end_len <= 0) || (end_len >= (int)sizeof(end_boundary)) || (len <= (end_len + 2))) {
		error = "Invalid upload boundary";
		goto ERROR;
	}

	/*
	 * The multipart body ends with:
	 *   firmware data + "\\r\\n--" + boundary + "--\\r\\n"
	 * Stage only the firmware bytes so the validator sees the exact image.
	 */
	image_len = (unsigned long)(len - end_len - 2);

	if (image_len < (1 * 1024 * 1024)) {
		error = "Invalid file: too small";
		goto ERROR;
	}
	if (image_len > FIRMWARE_MAX_SIZE) {
		error = "Invalid file: too large";
		goto ERROR;
	}

	if (!firmware_tmp_space_ok(image_len)) {
		error = "Not enough free memory to stage firmware image";
		goto ERROR;
	}

	strlcpy(upgrade_file, "/tmp/firmwareXXXXXX", sizeof(upgrade_file));
	if ((fd = mkstemp(upgrade_file)) < 0) {
		error = "Unable to create temporary firmware file";
		goto ERROR;
	}

	if ((f = fdopen(fd, "w")) == NULL) {
		error = "Unable to open temporary firmware file";
		goto ERROR;
	}
	fd = -1; /* owned by f */

	remaining = image_len;
	while (remaining > 0) {
		m = web_read(buf, MIN(remaining, (unsigned long)sizeof(buf)));
		if (m <= 0) {
			error = "Incomplete firmware upload";
			goto ERROR;
		}

		if (safe_fwrite(buf, 1, m, f) != (size_t)m) {
			error = "Error writing temporary firmware file";
			goto ERROR;
		}

		remaining -= (unsigned long)m;
		len -= m;
	}

	if ((fflush(f) != 0) || (fsync(fileno(f)) != 0)) {
		error = "Error flushing temporary firmware file";
		goto ERROR;
	}

	if (fclose(f) != 0) {
		f = NULL;
		error = "Error closing temporary firmware file";
		goto ERROR;
	}
	f = NULL;

	/* Consume and verify the multipart terminator separately from the image. */
	if (web_read_x(recv_boundary, end_len) != end_len) {
		error = "Incomplete upload boundary";
		goto ERROR;
	}
	len -= end_len;

	if (memcmp(recv_boundary, end_boundary, end_len) != 0) {
		error = "Invalid upload boundary";
		goto ERROR;
	}

	if (web_read_x(tail, (int)sizeof(tail)) != (int)sizeof(tail)) {
		error = "Incomplete upload boundary";
		goto ERROR;
	}
	len -= (int)sizeof(tail);

	if ((tail[0] != '\r') || (tail[1] != '\n')) {
		error = "Invalid upload boundary";
		goto ERROR;
	}

	if ((stat(upgrade_file, &st) != 0) || ((unsigned long)st.st_size != image_len)) {
		error = "Incomplete firmware upload";
		goto ERROR;
	}

	/*
	 * Validate while the router is still fully operational. A bad header,
	 * size or CRC is reported to the browser without touching flash or
	 * stopping services.
	 */
	status = validate_firmware(upgrade_file);
	if (status != 0) {
		if (!resmsg_fread("/tmp/.mtd-check"))
			error = "Firmware image validation failed";
		else
			error = NULL;
		goto ERROR;
	}
	unlink("/tmp/.mtd-check");

	/* From this point forward the validated image is committed for upgrade. */
	signal(SIGTERM, SIG_IGN);
	signal(SIGINT, SIG_IGN);
	signal(SIGHUP, SIG_IGN);
	signal(SIGQUIT, SIG_IGN);

	if (!prepare_upgrade())
		resmsg_set("Warning: service shutdown timed out after 60 seconds; firmware upgrade will continue. If this repeats, check enabled service configuration.");

	upgrade_reset = reset;
	rboot = 1;
	led(LED_DIAG, 1);

	if (reset)
		webcgi_set("resreset", "1");

	return;

ERROR:
	if (f)
		fclose(f);
	else if (fd >= 0)
		close(fd);

	if (!rboot && upgrade_file[0]) {
		unlink(upgrade_file);
		upgrade_file[0] = '\0';
	}
	unlink("/tmp/.mtd-check");

	if (error)
		resmsg_set(error);

	/* consume any remaining unread POST data */
	if (len > 0)
		web_eat(len);
}

void wo_flash(char *url)
{
	int status;

#ifdef TCONFIG_BCMARM
	char *args[] = { "mtd-write2", upgrade_file, "linux", NULL };
#else
	char *args[] = { "mtd-write", "-w", "-i", upgrade_file, "-d", "linux", NULL };
#endif

	if (!rboot || !upgrade_file[0]) {
		parse_asp("error.asp");
		return;
	}

	/*
	 * Deliver reboot.asp while the client-facing network is still available.
	 * Keep httpd/networking alive briefly so the browser can fetch the linked
	 * stylesheets before upgrade-finalize tears the interface down.
	 */
	parse_asp("reboot.asp");
	web_close();
	sleep(2);

	/*
	 * The destructive phase starts only now. This stops wireless and any
	 * networking/storage kept alive for the upload response.
	 */
	if (!finalize_upgrade())
		logmsg(LOG_WARNING, "upgrade-finalize did not complete before timeout; continuing");

	sync();
	unlink("/tmp/.mtd-write");
	status = _eval(args, ">/tmp/.mtd-write", 0, NULL);

	if ((status == 0) && upgrade_reset) {
		set_action(ACT_IDLE);
#ifdef TCONFIG_BCMARM
		eval("mtd-erase2", "nvram");
#else
		eval("mtd-erase", "-d", "nvram");
#endif
	}

	unlink(upgrade_file);
	upgrade_file[0] = '\0';

	/*
	 * Once flashing has started the old root filesystem can no longer be
	 * trusted. Reboot even if the writer reports an I/O error, matching the
	 * historical web-upgrade recovery behaviour.
	 */
	set_action(ACT_REBOOT);
	sync();
	sleep(2);
	reboot(RB_AUTOBOOT);
	exit(0);
}
#else
void wi_upgrade(char *url, int len, char *boundary)
{
	FILE *f = NULL;
	char fifo[] = "/tmp/flashXXXXXX";
	uint8 buf[1024];
	pid_t pid = -1;
	int fd = -1, retries = 100;
	size_t m;
	int status;
	unsigned int reset;
	const char *error = "Error reading file";
	int complete = 1;
#ifdef TOMATO64
#if !defined(TOMATO64_BCM53XX) && !defined(TOMATO64_MT3600BE)
	struct statvfs disk;
	statvfs("/", &disk);
	float f_bavail = disk.f_bavail;
	float f_frsize = disk.f_frsize;
	float available_space = f_bavail * f_frsize;
#endif /* !TOMATO64_BCM53XX && !TOMATO64_MT3600BE */
#endif /* TOMATO64 */
#ifndef TOMATO64
#ifdef TCONFIG_BCMARM
	char *args[] = { "mtd-write2", fifo, "linux", NULL };
#else
	char *args[] = { "mtd-write", "-w", "-i", fifo, "-d", "linux", NULL };
#endif
#else /* TOMATO64 */
	/* args[2] = factory-reset flag, filled in below from _reset. Only
	 * MT3600BE's /sbin/upgrade reads it; other devices ignore extra args
	 * and rely on httpd's post-flash nvram_clear(). */
	char *args[] = { "upgrade", fifo, "0", NULL };
#endif /* TOMATO64 */


	/* validate session */
	check_id(url);

	reset = (strcmp(webcgi_safeget("_reset", "0"), "1") == 0);

#ifdef TOMATO64
	if (reset)
		args[2] = "1";
#endif /* TOMATO64 */

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
#if !defined(TOMATO64_BCM53XX) && !defined(TOMATO64_MT3600BE)
	if ((float) len > available_space) {
		error = "Insufficient disk space to extract update";
		goto ERROR;
	}
#endif /* !TOMATO64_BCM53XX && !TOMATO64_MT3600BE */
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
#if !defined(TOMATO64_BCM53XX) && !defined(TOMATO64_MT3600BE)
		nvram_clear();
#endif /* !TOMATO64_BCM53XX && !TOMATO64_MT3600BE */
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
#if defined(TOMATO64_MT3600BE) || defined(TOMATO64_BCM53XX)
		parse_asp("/tmp/reboot.asp");
		web_close();
		return;
#endif /* TOMATO64_MT3600BE || TOMATO64_BCM53XX */
#ifdef TOMATO64_X86_64
		if (fastreboot)
			parse_asp("/tmp/reboot-fast.asp");
		else
#endif /* TOMATO64_X86_64 */
		parse_asp("/tmp/reboot.asp");
		web_close();

		/* Give the browser time to request linked reboot page assets. */
		sleep(2);

		if (!finalize_upgrade())
			logmsg(LOG_WARNING, "upgrade-finalize did not complete before timeout; continuing");

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
#endif /* TOMATO64 */
