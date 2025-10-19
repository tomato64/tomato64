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
#include <sys/wait.h>
#include <typedefs.h>
#include <sys/reboot.h>


void wi_uploadsplash(char *url, int len, char *boundary)
{
	char tmp[255];
	char *buf, *p;
	const char *error;
	int n;

	//check_id();

	tmp[0] = 0;
	buf = NULL;
	error = "Error reading file";

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
	n = n - strlen(boundary)-6;
	syslog(LOG_INFO, "boundary %s, len %d", boundary, strlen(boundary));

	if ((p = nvram_get("NC_DocumentRoot")) == NULL)
		p = "/tmp/splashd";

	snprintf(tmp, sizeof(tmp), "%s/splash.html", p);

	if (f_write(tmp, buf, n, 0, 0600) != n) {
		error = "Error writing temporary file";
		goto ERROR;
	}

	nvram_set_file("NC_SplashFile", tmp, 8192);
	nvram_commit();
	rboot = 1;

	error = NULL;

ERROR:
	free(buf);
	if (error != NULL)
		resmsg_set(error);

	web_eat(len);
}

void wo_uploadsplash(char *url)
{
	if (rboot) {
		redirect("/splashd.asp");
		exit(0);
	}
	else
		parse_asp("error.asp");
}
