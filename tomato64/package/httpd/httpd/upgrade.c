/*
 *
 * Tomato Firmware
 * Copyright (C) 2006-2009 Jonathan Zarate
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
	for (n = 30; n > 0; --n) {
		sleep(1);
		if (nvram_match("action_service", ""))
			break; /* this is cleared at the end */
	}
	unlink("/var/log/messages");
	unlink("/var/log/messages.0");
	sync();
}

void wi_upgrade(char *url, int len, char *boundary)
{
	FILE *f = NULL;
	char fifo[] = "/tmp/flashXXXXXX";
	const char *error = "Error reading file";
	int pid = -1;
	int n;
	unsigned int reset, m;
	uint8 buf[1024];

	check_id(url);
	reset = (strcmp(webcgi_safeget("_reset", "0"), "1") == 0);

	/* skip the rest of the header */
	if (!skip_header(&len))
		goto ERROR;

	if (len < (1 * 1024 * 1024)) {
		error = "Invalid file";
		goto ERROR;
	}

	/* -- anything after here ends in a reboot -- */

	rboot = 1;

	signal(SIGTERM, SIG_IGN);
	signal(SIGINT, SIG_IGN);
	signal(SIGHUP, SIG_IGN);
	signal(SIGQUIT, SIG_IGN);

	prepare_upgrade();

	/* copy to memory */
	system("cp /www/reboot.asp /tmp");
	system("cp /www/*.css /tmp");
	system("cp /www/*.png /tmp");

	led(LED_DIAG, 1);

	if ((mktemp(fifo) == NULL) || (mkfifo(fifo, S_IRWXU) < 0)) {
		error = "Unable to create a fifo";
		goto ERROR2;
	}

#ifdef TCONFIG_BCMARM
	char *args[] = { "mtd-write2", fifo, "linux", NULL };
#else
	char *args[] = { "mtd-write", "-w", "-i", fifo, "-d", "linux", NULL };
#endif
	if (_eval(args, ">/tmp/.mtd-write", 0, &pid) != 0) {
		error = "Unable to start flash program";
		goto ERROR2;
	}

	if ((f = fopen(fifo, "w")) == NULL) {
		error = "Unable to start pipe for mtd-write";
		goto ERROR2;
	}

	/* this will actually write the boundary, but since mtd-write uses trx length... */
	while (len > 0) {
		if ((m = web_read(buf, MIN((unsigned int) len, sizeof(buf)))) <= 0)
			goto ERROR2;

		len -= m;
		if (safe_fwrite(buf, 1, m, f) != (int) m) {
			error = "Error writing to pipe";
			goto ERROR2;
		}
	}

	error = NULL;

ERROR2:
	rboot = 1;

	if (f)
		fclose(f);
	if (pid != -1)
		waitpid(pid, &n, 0);

	if (error == NULL && reset) {
		set_action(ACT_IDLE);
#ifdef TCONFIG_BCMARM
		eval("mtd-erase2", "nvram");
#else
		eval("mtd-erase", "-d", "nvram");
#endif
	}
	set_action(ACT_REBOOT);

	if (resmsg_fread("/tmp/.mtd-write"))
		error = NULL;

ERROR:
	if (error)
		resmsg_set(error);

	web_eat(len);

	/* erase flash file and free memory */
	if (fifo[0])
		unlink(fifo);
}

void wo_flash(char *url)
{
	if (rboot) {
		parse_asp("/tmp/reboot.asp");
		web_close();

		if (nvram_get_int("remote_upgrade")) {
			killall("xl2tpd", SIGTERM);
			killall("pppd", SIGTERM);
		}
		sleep(2);

		//kill(1, SIGTERM);
		sync();
		reboot(RB_AUTOBOOT);

		exit(0);
	}

	parse_asp("error.asp");
}
