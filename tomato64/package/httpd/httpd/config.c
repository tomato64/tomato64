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
#include <sys/wait.h>
#include <typedefs.h>
#include <sys/reboot.h>


void wo_defaults(char *url)
{
	const char *v;
	int mode;

	if ((v = webcgi_get("mode")) != NULL) {
		mode = atoi(v);
		if ((mode == 1) || (mode == 2)) {
			prepare_upgrade();

#ifdef TOMATO64_X86_64
			eval("mount_nvram");
#endif /* TOMATO64_X86_64 */

			led(LED_DIAG, 1);

			webcgi_set("resreset", "1");
			parse_asp("reboot.asp");
			web_close();

			if (mode == 1) {
				nvram_set("restore_defaults", "1");
				nvram_commit();
			}
			else
#ifndef TOMATO64
			{
#ifdef TCONFIG_BCMARM
				eval("mtd-erase2", "nvram");
#else
				eval("mtd-erase", "-d", "nvram");
#endif
			}
#else
			{
				system("rm /nvram/*");
			}
#endif /* TOMATO64 */

			if (nvram_get_int("remote_upgrade")) {
				killall("xl2tpd", SIGTERM);
				killall("pppd", SIGTERM);
			}
			sleep(2);

			set_action(ACT_REBOOT);
			sync();
			//kill(1, SIGTERM);
			reboot(RB_AUTOBOOT);

			exit(0);
		}
	}

	redirect("/admin-config.asp");
}

void wo_backup(char *url)
{
#ifdef TCONFIG_BCMARM
	static char *args[] = { "nvram", "save", NULL, NULL };
#else
	static char *args[] = { "nvram", "backup", NULL, NULL };
#endif
	char file[] = "/tmp/backupXXXXXX";
	char msg[64];
	int fd;

	/* create secure temporary file */
	if ((fd = mkstemp(file)) < 0)
		exit(1);

	/*
	 * ensure fd is not inherited across exec (extra safety)
	 * even though we close it below, this protects against future changes.
	 */
	fcntl(fd, F_SETFD, FD_CLOEXEC);

	/*
	 * close immediately - we only need the filename.
	 * prevents descriptor leak into _eval() child.
	 */
	close(fd);

	args[2] = file;
	snprintf(msg, sizeof(msg), ">%s.msg", file);

	if (_eval(args, msg, 0, NULL) == 0) {
		send_header(200, NULL, mime_binary, 0);
		do_file(file);
	}
	else {
		resmsg_fread(msg + 1);
		send_header(200, NULL, mime_html, 0);
		parse_asp("error.asp");
	}

	/* cleanup */
	unlink(file);
	unlink(msg + 1);
}

void wi_restore(char *url, int len, char *boundary)
{
	char *buf = NULL;
	const char *error = "Error reading file";
	int n, fd;
	static char *args[] = { "nvram", "restore", NULL, NULL };
	char file[] = "/tmp/restoreXXXXXX";
	char msg[64];
	int total = 0;

	/* validate session */
	check_id(url);

	if ((fd = mkstemp(file)) < 0) {
		error = "Error creating file";
		goto ERROR;
	}
	args[2] = file;
	snprintf(msg, sizeof(msg), ">%s.msg", file);

	/* skip HTTP headers (not multipart headers!) */
	if (!skip_header(&len))
		goto ERROR;

	/* basic sanity check for payload size */
	if ((len < 64) || (len > (NVRAM_SPACE * 2))) {
		error = "Invalid file";
		goto ERROR;
	}

	/* allocate buffer for incoming data */
	if ((buf = malloc(len)) == NULL) {
		error = "Not enough memory";
		goto ERROR;
	}

	/* read full POST body (web_read() may return partial data) */
	while (total < len) {
		n = web_read(buf + total, len - total);
		if (n <= 0) {
			error = "Error reading file";
			goto ERROR;
		}
		total += n;
	}

	/*
	 * write data directly using mkstemp() file descriptor.
	 * this avoids:
	 *  - double open()
	 *  - TOCTOU race window
	 *  - file descriptor leaks
	 */
	if (write(fd, buf, total) != total) {
		error = "Error writing temporary file";
		goto ERROR;
	}

	/* ensure data is flushed to disk */
	fsync(fd);

	/* close file descriptor early */
	close(fd);
	fd = -1;

	rboot = 1;

	/* stop services and prepare system for restore */
	prepare_upgrade();

#ifdef TOMATO64_X86_64
	eval("mount_nvram");
#endif /* TOMATO64_X86_64 */

	/*
	 * execute: nvram restore <file>
	 * output redirected to msg file.
	 */
	if (_eval(args, msg, 0, NULL) != 0)
		resmsg_fread(msg + 1);

#ifdef TCONFIG_BCMARM
	nvram_commit();
#endif

	/* remove temporary message file */
	unlink(msg + 1);

	error = NULL;

ERROR:
	/* cleanup file descriptor if still open */
	if (fd >= 0)
		close(fd);

	/* free allocated buffer */
	free(buf);

	/* remove temporary restore file */
	if (file[0])
		unlink(file);

	/* set error message for GUI if needed */
	if (error)
		resmsg_set(error);

	/* consume any remaining unread POST data to keep connection in consistent state */
	web_eat(len);
}

void wo_restore(char *url)
{
	if (rboot) {
		set_action(ACT_REBOOT);
		sync();
		parse_asp("reboot.asp");
		web_close();

		if (nvram_get_int("remote_upgrade")) {
			killall("xl2tpd", SIGTERM);
			killall("pppd", SIGTERM);
		}

		sleep(2);

		reboot(RB_AUTOBOOT);

		exit(0);
	}

	parse_asp("error.asp");
}
