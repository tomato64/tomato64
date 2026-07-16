/*
 * bb_uuid.c
 *
 * Copyright (C) 2025 - 2026 FreshTomato
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
/*
 * Compatibility shim for BusyBox >= 1.38.
 *
 * BusyBox commit 3e8010196325 moved UUID DCE formatting from
 * util-linux/volume_id/util*.c to libbb/xfuncs_printf.c.
 *
 * libshared.so links selected volume_id objects directly, without linking
 * BusyBox libbb. Provide the tiny formatter here so the volume_id objects
 * can be linked into libshared.so.
 */

#include <stdint.h>
#include <stdio.h>

void __attribute__((visibility("hidden")))
format_uuid_DCE_37_chars(char *dst37, const uint8_t *buf)
{
	/* 37 = 16*2 hexdigits + 4 dashes + 1 NUL */
	sprintf(dst37,
		"%02x%02x%02x%02x-%02x%02x-%02x%02x-%02x%02x-%02x%02x%02x%02x%02x%02x",
		buf[0], buf[1], buf[2], buf[3],
		buf[4], buf[5],
		buf[6], buf[7],
		buf[8], buf[9],
		buf[10], buf[11], buf[12], buf[13], buf[14], buf[15]
	);
}
