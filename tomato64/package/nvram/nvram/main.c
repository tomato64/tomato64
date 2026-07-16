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
/*
 *
 * Fixes/updates (C) 2018 - 2026 pedro
 * https://freshtomato.org/
 *
 */


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <typedefs.h>

#include <bcmnvram.h>
#include <shutils.h>
#include <shared.h>

#define PROFILE_HEADER		"HDR1"
#define PROFILE_HEADER_NEW	"HDR2"
#define INFILE_NOT_READABLE	-1
#define OUTFILE_NOT_WRITABLE	-2
#define INVALID_CFG_FORMAT	-3

#define PROFILE_ALIGN		1024

static uint32_t read_le32(const unsigned char *p)
{
	return ((uint32_t)p[0]) |
	       ((uint32_t)p[1] << 8) |
	       ((uint32_t)p[2] << 16) |
	       ((uint32_t)p[3] << 24);
}

static uint32_t read_le24(const unsigned char *p)
{
	return ((uint32_t)p[0]) |
	       ((uint32_t)p[1] << 8) |
	       ((uint32_t)p[2] << 16);
}

static int print_error(int rv, const char *infile, const char *outfile)
{
	switch(rv) {
		case INFILE_NOT_READABLE:
			fprintf(stderr, "Cannot read input file \"%s\"\n", infile);
			return 1;
		case OUTFILE_NOT_WRITABLE:
			fprintf(stderr, "Cannot write to output file \"%s\"\n", outfile);
			return 1;
		case INVALID_CFG_FORMAT:
			fprintf(stderr, "Invalid cfg file format for \"%s\"\n", infile);
			return 1;
		default:
			fprintf(stderr, "Wrote %d bytes to %s\n", rv, outfile);
			return 0;
	}
}

static unsigned char get_rand(void)
{
	unsigned char b = 0;

	if (f_read("/dev/urandom", &b, 1) != 1)
		b = 0;

	return b;
}

static int nvram_save_new(const char *file, const char *buf)
{
	FILE *fp;
	const char *name;
	size_t count, i, len;
	unsigned long filelen;
	unsigned char rand = 0, temp;
	unsigned char outbuf[MAX_NVRAM_SPACE];

	if ((fp = fopen(file, "wb")) == NULL)
		return OUTFILE_NOT_WRITABLE;

	count = 0;
	for (name = buf; *name; ) {
		len = strlen(name);
		if (count > sizeof(outbuf) - (len + 1)) {
			fclose(fp);
			return INVALID_CFG_FORMAT;
		}
		count += len + 1;
		name += len + 1;
	}

	filelen = (count + (PROFILE_ALIGN - 1)) & ~(PROFILE_ALIGN - 1);

	do {
		rand = get_rand() % 30;
	} while (rand > 7 && rand < 14);

	if (safe_fwrite(PROFILE_HEADER_NEW, 1, 4, fp) != 4) {
		fclose(fp);
		return OUTFILE_NOT_WRITABLE;
	}

	temp = (unsigned char)(filelen & 0xff);
	if (safe_fwrite(&temp, 1, 1, fp) != 1) {
		fclose(fp);
		return OUTFILE_NOT_WRITABLE;
	}
	temp = (unsigned char)((filelen >> 8) & 0xff);
	if (safe_fwrite(&temp, 1, 1, fp) != 1) {
		fclose(fp);
		return OUTFILE_NOT_WRITABLE;
	}
	temp = (unsigned char)((filelen >> 16) & 0xff);
	if (safe_fwrite(&temp, 1, 1, fp) != 1) {
		fclose(fp);
		return OUTFILE_NOT_WRITABLE;
	}
	if (safe_fwrite(&rand, 1, 1, fp) != 1) {
		fclose(fp);
		return OUTFILE_NOT_WRITABLE;
	}

	for (i = 0; i < count; i++) {
		if (buf[i] == '\0')
			outbuf[i] = 0xfd + get_rand() % 3;
		else
			outbuf[i] = (unsigned char)(0xff - (unsigned char)buf[i] + rand);
	}

	if ((count > 0) && (safe_fwrite(outbuf, 1, count, fp) != count)) {
		fclose(fp);
		return OUTFILE_NOT_WRITABLE;
	}

	for (i = count; i < filelen; i++) {
		temp = 0xfd + get_rand() % 3;
		if (safe_fwrite(&temp, 1, 1, fp) != 1) {
			fclose(fp);
			return OUTFILE_NOT_WRITABLE;
		}
	}

	if (fclose(fp) != 0)
		return OUTFILE_NOT_WRITABLE;

	return 0;
}

static int nvram_restore_new(const char *file, char *buf, size_t buflen, FILE *ofp)
{
	FILE *fp;
	unsigned char header[8];
	char *p, *v;
	size_t count, i;
	uint32_t filelen;
	unsigned char rand;
	unsigned long nbytes = 0;

	if ((file == NULL) || (buf == NULL) || (buflen < 2))
		return INVALID_CFG_FORMAT;

	if ((fp = fopen(file, "rb")) == NULL)
		return INFILE_NOT_READABLE;

	count = safe_fread(header, 1, sizeof(header), fp);
	if (count < sizeof(header)) {
		fclose(fp);
		return INVALID_CFG_FORMAT;
	}

	if (memcmp(header, PROFILE_HEADER, 4) == 0) {
		filelen = read_le32(header + 4);

		if ((filelen == 0) || (filelen >= buflen)) {
			fclose(fp);
			return INVALID_CFG_FORMAT;
		}

		count = safe_fread(buf, 1, filelen, fp);
		if (count != filelen) {
			fclose(fp);
			return INVALID_CFG_FORMAT;
		}

		buf[count] = '\0';
	}
	else if (memcmp(header, PROFILE_HEADER_NEW, 4) == 0) {
		filelen = read_le24(header + 4);
		rand = header[7];

		if ((filelen == 0) || (filelen >= buflen)) {
			fclose(fp);
			return INVALID_CFG_FORMAT;
		}

		count = safe_fread(buf, 1, filelen, fp);
		if (count != filelen) {
			fclose(fp);
			return INVALID_CFG_FORMAT;
		}

		for (i = 0; i < count; i++) {
			if ((unsigned char)buf[i] >= 0xfd) {
				/* e.g.: to skip the case: 0x61 0x62 0x63 0x00 0x00 0x61 0x62 0x63 */
				if ((i > 0) && (buf[i - 1] != '\0'))
					buf[i] = '\0';
			}
			else {
				buf[i] = (char)((unsigned char)(0xff + rand - (unsigned char)buf[i]));
			}
		}

		buf[count] = '\0';
	}
	else {
		fclose(fp);
		return INVALID_CFG_FORMAT;
	}

	fclose(fp);

	p = buf;
	while (*p) {
		/* e.g.: to skip the case: 00 2e 30 2e 32 38 00 ff 77 61 6e */
		if (((unsigned char)*p < 32) || ((unsigned char)*p > 127)) {
			p++;
			continue;
		}

		v = strchr(p, '=');

		if (v != NULL) {
			*v++ = '\0';

			if (ofp != NULL) {
				nbytes += fprintf(ofp, "%s", p);
				nbytes += fprintf(ofp, "=%s\n", v);
			}
			else
				nvram_set(p, v);

			p = v + strlen(v) + 1;
		}
		else {
			if (ofp != NULL)
				nbytes += fprintf(ofp, "%s=\n", p);
			else
				nvram_unset(p);

			p += strlen(p) + 1;
		}
	}

	return (int)nbytes;
}

static int nvram_restore_to_file(const char *file, const char *outfile, char *buf, size_t buflen)
{
	FILE *ofp;
	int rv;

	if ((ofp = fopen(outfile, "w")) == NULL)
		return OUTFILE_NOT_WRITABLE;

	rv = nvram_restore_new(file, buf, buflen, ofp);
	fclose(ofp);

	return rv;
}

void usage(void)
{
	fprintf(stderr, "NVRAM Utility\n"
	                "Usage: nvram set <key=value> | get <key> | unset <key> |\n"
	                "commit | erase | show | save <filename> | restore <filename> |\n"
	                "convert <infile.cfg> <outfile.txt>\n");

	exit(1);
}

/* NVRAM utility */
int main(int argc, char **argv)
{
	char *name, *value, buf[MAX_NVRAM_SPACE];
	int size, res, ret = 0;

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
				strlcpy(buf, *argv, sizeof(buf));
				value = buf;
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
				res = nvram_save_new(*argv, buf);
				if (res != 0) {
					ret = print_error(res, *argv, *argv);
					break;
				}
			}
		}
		else if (!strcmp(*argv, "restore")) {
			if (*++argv) {
				res = nvram_restore_new(*argv, buf, sizeof(buf), NULL);
				if (res < 0) {
					ret = print_error(res, *argv, NULL);
					break;
				}
			}
		}
		else if (!strcmp(*argv, "erase")) {
			ret = eval("nvram_erase");
			if (ret)
				fprintf(stderr, "Error: nvram_erase failed\n");
		}
		else if ((!strcmp(*argv, "show")) || (!strcmp(*argv, "dump"))) {
			nvram_getall(buf, sizeof(buf));
			for (name = buf; *name; name += strlen(name) + 1)
				puts(name);

			size = (int)(sizeof(struct nvram_header) + (size_t)(name - buf));
			if (**argv != 'd')
				fprintf(stderr, "size: %d bytes (%d left)\n", size, MAX_NVRAM_SPACE - size);
		}
		else if (!strcmp(*argv, "convert")) {
			if (*++argv) {
				name = *argv;
				if (*++argv) {
					res = nvram_restore_to_file(name, *argv, buf, sizeof(buf));
					ret = print_error(res, name, (const char *)*argv);
				}
			}
		}
		else
			usage();
	}

	return ret;
}
