/*
 *
 * Tomato Firmware
 * Copyright (C) 2006-2009 Jonathan Zarate
 * Fixes/updates (C) 2018 - 2024 pedro
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

#ifdef TOMATO64
			eval("mount_nvram");
#endif /* TOMATO64 */

			led(LED_DIAG, 1);

			parse_asp("reboot-default.asp");
			web_close();

			if (nvram_get_int("remote_upgrade")) {
				killall("xl2tpd", SIGTERM);
				killall("pppd", SIGTERM);
			}
			sleep(2);

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

	if ((fd = mkstemp(file)) < 0)
		exit(1);

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

	close(fd);
	unlink(file);
	unlink(msg + 1);
}

void wi_restore(char *url, int len, char *boundary)
{
	char *buf;
	const char *error = "Error reading file";
	int n, fd;
	static char *args[] = { "nvram", "restore", NULL, NULL };
	char file[] = "/tmp/restoreXXXXXX";
	char msg[64];

	buf = NULL;
	check_id(url);

	if ((fd = mkstemp(file)) < 0) {
		error = "Error creating file";
		goto ERROR;
	}
	args[2] = file;
	snprintf(msg, sizeof(msg), ">%s.msg", file);

	if (!skip_header(&len))
		goto ERROR;

	if ((len < 64) || (len > (NVRAM_SPACE * 2))) {
		error = "Invalid file";
		goto ERROR;
	}

	if ((buf = malloc(len)) == NULL) {
		error = "Not enough memory";
		goto ERROR;
	}

	n = web_read(buf, len);
	len -= n;

	if (f_write(file, buf, n, 0, 0600) != n) {
		error = "Error writing temporary file";
		goto ERROR;
	}

	rboot = 1;

	prepare_upgrade();

#ifdef TOMATO64
	eval("mount_nvram");
#endif /* TOMATO64 */

	if (_eval(args, msg, 0, NULL) != 0)
		resmsg_fread(msg + 1);

#ifdef TCONFIG_BCMARM
	nvram_commit();
#endif

	close(fd);
	unlink(msg + 1);

	error = NULL;

ERROR:
	if (buf)
		free(buf);

	if (file[0])
		unlink(file);

	if (error)
		resmsg_set(error);

	web_eat(len);
}

void wo_restore(char *url)
{
	if (rboot) {
		parse_asp("reboot.asp");
		web_close();

		if (nvram_get_int("remote_upgrade")) {
			killall("xl2tpd", SIGTERM);
			killall("pppd", SIGTERM);
		}
		sleep(2);

		set_action(ACT_REBOOT);
		//kill(1, SIGTERM);
		sync();
		reboot(RB_AUTOBOOT);

		exit(0);
	}

	parse_asp("error.asp");
}
