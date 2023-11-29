/*
 * Frontend command-line utility for Linux NVRAM layer
 *
 * Copyright (C) 2012, Broadcom Corporation. All Rights Reserved.
 * 
 * Permission to use, copy, modify, and/or distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY
 * SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN ACTION
 * OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN
 * CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 *
 * $Id: main.c 325698 2012-04-04 12:40:07Z $
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <typedefs.h>
#include <bcmnvram.h>

#define PROFILE_HEADER		"HDR1"
#define PROFILE_HEADER_NEW	"HDR2"

unsigned char get_rand()
{
	unsigned char buf[1];
	FILE *fp;

	fp = fopen("/dev/urandom", "r");
	if (fp == NULL)
		return 0;

	fread(buf, 1, 1, fp);
	fclose(fp);

	return buf[0];
}

int nvram_save_new(char *file, char *buf)
{
	FILE *fp;
	char *name;
	unsigned long count, filelen, i;
	unsigned char rand = 0, temp;

	if ((fp = fopen(file, "w")) == NULL)
		return -1;

	count = 0;
	for (name = buf; *name; name += strlen(name) + 1) {
		count = count + strlen(name) + 1;
	}

	filelen = count + (1024 - count % 1024);
	do {
		rand = get_rand() % 30;
	}
	while (rand > 7 && rand < 14);

	fwrite(PROFILE_HEADER_NEW, 1, 4, fp);
	fwrite(&filelen, 1, 3, fp);
	fwrite(&rand, 1, 1, fp);
	for (i = 0; i < count; i++) {
		if (buf[i] == 0x0)
			buf[i] = 0xfd + get_rand() % 3;
		else
			buf[i] = 0xff - buf[i] + rand;
	}
	fwrite(buf, 1, count, fp);
	for (i = count; i < filelen; i++) {
		temp = 0xfd + get_rand() % 3;
		fwrite(&temp, 1, 1, fp);
	}
	fclose(fp);

	return 0;
}

int nvram_restore_new(char *file, char *buf)
{
	FILE *fp;
	char header[8], *p, *v;
	unsigned long count, filelen, *filelenptr, i;
	unsigned char rand, *randptr;

	if ((fp = fopen(file, "r+")) == NULL)
		return -1;

	count = fread(header, 1, 8, fp);
	if (count >= 8 && strncmp(header, PROFILE_HEADER, 4) == 0) {
		filelenptr = (unsigned long *)(header + 4);
		fread(buf, 1, *filelenptr, fp);
	}
	else if (count >= 8 && strncmp(header, PROFILE_HEADER_NEW, 4) == 0) {
		filelenptr = (unsigned long *)(header + 4);
		filelen = *filelenptr & 0xffffff;
		randptr = (unsigned char *)(header + 7);
		rand = *randptr;
		count = fread(buf, 1, filelen, fp);

		for (i = 0; i < count; i++) {
			if ((unsigned char) buf[i] > (0xfd - 0x1)) {
				/* e.g.: to skip the case: 0x61 0x62 0x63 0x00 0x00 0x61 0x62 0x63 */
				if (i > 0 && buf[i - 1] != 0x0)
					buf[i] = 0x0;
			}
			else
				buf[i] = 0xff + rand - buf[i];
		}
	}
	else {
		fclose(fp);
		return 0;
	}
	fclose(fp);

	p = buf;
	while (*p) {
		/* e.g.: to skip the case: 00 2e 30 2e 32 38 00 ff 77 61 6e */
		if (*p == '\0' || *p < 32 || *p > 127) {
			p = p + 1;
			continue;
		}

		v = strchr(p, '=');

		if (v != NULL) {
			*v++ = '\0';
			nvram_set(p, v);
			p = v + strlen(v) + 1;
		}
		else {
			nvram_unset(p);
			p = p + 1;
		}
	}

	return 0;
}

void usage(void)
{
	fprintf(stderr, "NVRAM Utility\n"
	                "Usage: nvram set <key=value> | get <key> | unset <key> |\n"
	                "commit | erase | show | save <filename> | restore <filename>\n");
//	                "convert <infilename.cfg> <outfilename.txt>\n");

	exit(0);
}

/* NVRAM utility */
int main(int argc, char **argv)
{
	char *name, *value, buf[MAX_NVRAM_SPACE];
	int size;

	/* skip program name */
	--argc;
	++argv;

	if (!*argv)
		usage();

	/* Process the arguments */
	for (; *argv; ++argv) {
		if (!strcmp(*argv, "get")) {
			if (*++argv) {
				if ((value = nvram_get(*argv)) && (*value))
					puts(value);
			}
		}
		else if (!strcmp(*argv, "set")) {
			if (*++argv) {
				strncpy(value = buf, *argv, sizeof(buf));
				name = strsep(&value, "=");
				nvram_set(name, value);
			}
		}
		else if (!strcmp(*argv, "unset")) {
			if (*++argv)
				nvram_unset(*argv);
		}
		else if (!strcmp(*argv, "commit")) {
			nvram_commit();
		}
		else if (!strcmp(*argv, "save")) {
			if (*++argv) {
				nvram_getall(buf, NVRAM_SPACE);
				nvram_save_new(*argv, buf);
			}
		}
		else if (!strcmp(*argv, "restore")) {
			if (*++argv) 
				nvram_restore_new(*argv, buf);
		}
		else if (!strcmp(*argv, "erase")) {
			system("nvram_erase");
		}
		else if (!strcmp(*argv, "show") || !strcmp(*argv, "dump")) {
			nvram_getall(buf, sizeof(buf));
			for (name = buf; *name; name += strlen(name) + 1)
				puts(name);

			size = sizeof(struct nvram_header) + (int)name - (int)buf;
			if (**argv != 'd')
				fprintf(stderr, "size: %d bytes (%d left)\n", size, MAX_NVRAM_SPACE - size);
		}
		else
			usage();
	}

	return 0;
}
