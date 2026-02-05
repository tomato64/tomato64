/*
 * tomatoanon.c
 *
 * Copyright (C) 2012 shibby
 *
 * Fixes/updates (C) 2018 - 2026 pedro
 * https://freshtomato.org/
 *
 */


#include "rc.h"

#include <sys/stat.h>


void start_tomatoanon(void)
{
	/* only if enabled... */
	if (nvram_match("tomatoanon_enable", "1"))
		xstart("tomatoanon", "anonupdate");

	if (nvram_match("tomatoanon_notify", "1"))
		xstart("tomatoanon", "checkver");
}

void stop_tomatoanon(void)
{
	xstart("cru", "d", "anonupdate");
	xstart("cru", "d", "checkver");
}
