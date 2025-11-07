/*
 * wireguard.c
 *
 * Copyright (C) 2025 FreshTomato
 * https://freshtomato.org/
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston,
 * MA 02111-1307 USA
 *
 */


#include "tomato.h"

#define LOGMSG_DISABLE	DISABLE_SYSLOG_OSM
#define LOGMSG_NVDEBUG	"wg_debug"


static int wg_status(char *iface)
{
	FILE *fp;
	char buffer[BUF_SIZE_64];
	int status = 0;

	memset(buffer, 0, BUF_SIZE_64);
	snprintf(buffer, BUF_SIZE_64, "/sys/class/net/%s/operstate", iface);

	if ((fp = fopen(buffer, "r"))) {
		fgets(buffer, BUF_SIZE_64, fp);
		buffer[strcspn(buffer, "\n")] = 0;
		if ((strcmp(buffer, "unknown") == 0) || (strcmp(buffer, "up") == 0))
			status = 1;

		fclose(fp);
	}

	return status;
}

void asp_wgstat(int argc, char **argv)
{
	if (argc == 1)
		web_printf("%d", wg_status(argv[0]));
}
