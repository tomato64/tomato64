/*

	Copyright 2005, Broadcom Corporation
	All Rights Reserved.

	THIS SOFTWARE IS OFFERED "AS IS", AND BROADCOM GRANTS NO WARRANTIES OF ANY
	KIND, EXPRESS OR IMPLIED, BY STATUTE, COMMUNICATION OR OTHERWISE. BROADCOM
	SPECIFICALLY DISCLAIMS ANY IMPLIED WARRANTIES OF MERCHANTABILITY, FITNESS
	FOR A SPECIFIC PURPOSE OR NONINFRINGEMENT CONCERNING THIS SOFTWARE.

*/
/*

	Modified for Tomato Firmware
	Portions, Copyright (C) 2006-2009 Jonathan Zarate

*/


#include "rc.h"

#include <limits.h>
#include <sys/sysmacros.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#include <error.h>
#include <sys/ioctl.h>
#include <sys/sysinfo.h>
#include <linux/compiler.h>
#include <mtd/mtd-user.h>
#include <stdint.h>
#include <trxhdr.h>
#include <bcmutils.h>

#if defined(TCONFIG_BLINK) || defined(TCONFIG_BCMARM) /* RT-N+ */
#include <bcmendian.h>
#endif
#ifdef TCONFIG_BCMARM
#include <sys/mman.h>
#include <bcmnvram.h>
#include <shutils.h>
#endif /* TCONFIG_BCMARM */
//#define DEBUG_SIMULATE


struct code_header {
	char magic[4];
	char res1[4];
	char fwdate[3];
	char fwvern[3];
	char id[4];
	char hw_ver;
	char res2;
	unsigned short flags;
	unsigned char res3[10];
};

#ifdef TCONFIG_BLINK /* RT-N/RTAC */
/*
 * Netgear CHK Header -> contains needed checksum information (for Netgear)
 * Information here is stored big endian (vs. later information in TRX header and linux / rootfs, which is little endian)
 * First two entries are commented out, as they are read manually to determine the file format and header length (and rewind is not working?)
 */
struct chk_header {
	//uint32_t magic;
	//uint32_t header_len;
	uint8_t  reserved[8];
	uint32_t kernel_chksum;
	uint32_t rootfs_chksum;
	uint32_t kernel_len;
	uint32_t rootfs_len;
	uint32_t image_chksum;
	uint32_t header_chksum;
	/* char board_id[] - upto MAX_BOARD_ID_LEN, not NULL terminated! */
};
#endif /* TCONFIG_BLINK */

static uint32 *crc_table = NULL;

static void crc_done(void)
{
	free(crc_table);
	crc_table = NULL;
}

#ifndef TCONFIG_BCMARM
static int crc_init(void)
{
	uint32 c;
	int i, j;

	if (crc_table == NULL) {
		if ((crc_table = malloc(sizeof(uint32) * 256)) == NULL)
			return 0;

		for (i = 255; i >= 0; --i) {
			c = i;
			for (j = 8; j > 0; --j) {
				if (c & 1)
					c = (c >> 1) ^ 0xEDB88320L;
				else
					c >>= 1;
			}
			crc_table[i] = c;
		}
	}
	return 1;
}

static uint32 crc_calc(uint32 crc, char *buf, int len)
{
	while (len-- > 0) {
		crc = crc_table[(crc ^ *((char *)buf)) & 0xFF] ^ (crc >> 8);
		buf++;
	}
	return crc;
}
#endif /* !TCONFIG_BCMARM */

#ifdef TCONFIG_BCMARM
static int mtd_open_old(const char *mtdname, mtd_info_t *mi)
#else
static int mtd_open(const char *mtdname, mtd_info_t *mi)
#endif
{
	char path[256];
	int part;
	int size;
	int f;

	if (mtd_getinfo(mtdname, &part, &size)) {
		sprintf(path, MTD_DEV(%d), part);
		if ((f = open(path, O_RDWR|O_SYNC)) >= 0) {
			if ((mi) && ioctl(f, MEMGETINFO, mi) != 0) {
				close(f);
				return -1;
			}
			return f;
		}
	}

	return -1;
}

static int _unlock_erase(const char *mtdname, int erase)
{
	int mf;
	mtd_info_t mi;
	erase_info_t ei;
	int r;
#if defined(TCONFIG_BLINK) || defined(TCONFIG_BCMARM) /* RT-N+ */
	int ret, skipbb = 0;
#endif

	if (!wait_action_idle(5))
		return 0;

	set_action(ACT_ERASE_NVRAM);

#ifndef TCONFIG_BCMARM
	if (erase)
		led(LED_DIAG, 1);
#endif

	r = 0;

#ifdef TCONFIG_BCMARM
 	if ((mf = mtd_open_old(mtdname, &mi)) >= 0) {
#else
	if ((mf = mtd_open(mtdname, &mi)) >= 0) {
#endif
			r = 1;
#if 1
			ei.length = mi.erasesize;
			for (ei.start = 0; ei.start < mi.size; ei.start += mi.erasesize) {
				printf("%sing 0x%x - 0x%x\n", erase ? "Eras" : "Unlock", ei.start, (ei.start + ei.length) - 1);
				fflush(stdout);

#if defined(TCONFIG_BLINK) || defined(TCONFIG_BCMARM) /* RT-N+ */
				if (!skipbb) {
					loff_t offset = ei.start;

					if ((ret = ioctl(mf, MEMGETBADBLOCK, &offset)) > 0) {
						printf("Skipping bad block at 0x%08x\n", ei.start);
						continue;
					}
					else if (ret < 0) {
						if (errno == EOPNOTSUPP) {
							skipbb = 1; /* not supported by this device */
						}
						else {
							perror("MEMGETBADBLOCK");
							r = 0;
							break;
						}
					}
				}
#endif /* TCONFIG_BLINK || TCONFIG_BCMARM */
				if (ioctl(mf, MEMUNLOCK, &ei) != 0) {
					//perror("MEMUNLOCK");
					//r = 0;
					//break;
				}
				if (erase) {
					if (ioctl(mf, MEMERASE, &ei) != 0) {
						perror("MEMERASE");
						r = 0;
						break;
					}
				}
			}
#else /* 1 */
			ei.start = 0;
			ei.length = mi.size;

			printf("%sing 0x%x - 0x%x\n", erase ? "Eras" : "Unlock", ei.start, ei.length - 1);
			fflush(stdout);

			if (ioctl(mf, MEMUNLOCK, &ei) != 0) {
				perror("MEMUNLOCK");
				r = 0;
			}
			else if (erase) {
				if (ioctl(mf, MEMERASE, &ei) != 0) {
					perror("MEMERASE");
					r = 0;
				}
			}
#endif /* 1 */

			/* checkme: */
			char buf[2];
			read(mf, &buf, sizeof(buf));
			close(mf);
	}

#ifndef TCONFIG_BCMARM
	if (erase)
		led(LED_DIAG, 0);
#endif

	set_action(ACT_IDLE);

	if (r)
		printf("\"%s\" successfully %s.\n", mtdname, erase ? "erased" : "unlocked");
	else
		printf("\nError %sing MTD\n", erase ? "eras" : "unlock");

	sleep(1);

	return r;
}

int mtd_unlock(const char *mtdname)
{
	return _unlock_erase(mtdname, 0);
}

#ifdef TCONFIG_BCMARM
int mtd_erase_old(const char *mtdname)
#else
int mtd_erase(const char *mtdname)
#endif
{
	return !_unlock_erase(mtdname, 1);
}

#ifdef TCONFIG_BCMARM
int mtd_unlock_erase_main_old(int argc, char *argv[])
#else
int mtd_unlock_erase_main(int argc, char *argv[])
#endif
{
	char c;
	char *dev = NULL;

	while ((c = getopt(argc, argv, "d:")) != -1) {
		switch (c) {
		case 'd':
			dev = optarg;
			break;
		}
	}

	if (!dev) {
		usage_exit(argv[0], "-d part");
	}

	return _unlock_erase(dev, strstr(argv[0], "erase") ? 1 : 0);
}

#ifdef TCONFIG_BCMARM
int mtd_write_main_old(int argc, char *argv[])
{
	int mf = -1;
	mtd_info_t mi;
	erase_info_t ei;
#ifdef TCONFIG_BCMARM
	struct code_header;
#endif
	FILE *f;
	unsigned char *buf = NULL, *p, *bounce_buf = NULL;
	const char *error;
	long wlen, n;
	unsigned long filelen = 0, unit_len;
	struct sysinfo si;
	uint32 ofs;
	char c;
	int web = 0;
	char *iname = NULL;
	char *dev = NULL;
	char msg_buf[2048];
	int alloc = 0, bounce = 0, fd;
#ifdef DEBUG_SIMULATE
	FILE *of;
#endif

	while ((c = getopt(argc, argv, "i:d:w")) != -1) {
		switch (c) {
		case 'i':
			iname = optarg;
			break;
		case 'd':
			dev = optarg;
			break;
		case 'w':
			web = 1;
			break;
		}
	}

	if ((iname == NULL) || (dev == NULL)) {
		usage_exit(argv[0], "-i file -d part");
	}

	if (!wait_action_idle(10)) {
		printf("System is busy\n");
		return 1;
	}

	set_action(ACT_WEB_UPGRADE);

	if ((f = fopen(iname, "r")) == NULL) {
		error = "Error opening input file";
		goto ERROR;
	}

	fd = fileno(f);
	fseek( f, 0, SEEK_END);
	filelen = ftell(f);
	fseek( f, 0, SEEK_SET);
	_dprintf("*** %s: file len=0x%lu\n", __FUNCTION__, filelen);

#ifdef TCONFIG_BCMARM
	if ((mf = mtd_open_old(dev, &mi)) < 0) {
#else
	if ((mf = mtd_open(dev, &mi)) < 0) {
#endif
		snprintf(msg_buf, sizeof(msg_buf), "Error opening MTD device. (errno %d (%s))", errno, strerror(errno));
		error = msg_buf;
		goto ERROR;
	}

	if (mi.erasesize < sizeof(struct trx_header)) {
		error = "Error obtaining MTD information";
		goto ERROR;
	}

	_dprintf("*** %s: mtd size=%x, erasesize=%x, writesize=%x, type=%x\n", __FUNCTION__, mi.size, mi.erasesize, mi.writesize, mi.type);

	unit_len = ROUNDUP(filelen, mi.erasesize);
	if (unit_len > mi.size) {
		error = "File is too big to fit in MTD";
		goto ERROR;
	}

	if ((buf = mmap(0, filelen, PROT_READ, MAP_SHARED, fd, 0)) == (unsigned char*)MAP_FAILED) {
		_dprintf("*** %s: mmap %lu bytes fail!. errno %d (%s)\n", __FUNCTION__, filelen, errno, strerror(errno));
		alloc = 1;
	}

	sysinfo(&si);
	if (alloc) {
		if ((si.freeram * si.mem_unit) <= (unit_len + (4096 * 1024)))
			unit_len = mi.erasesize;
	}

	if (mi.type == MTD_UBIVOLUME) {
		if (!(bounce_buf = malloc(mi.writesize))) {
			error = "Not enough memory";
			goto ERROR;
		}
	}
	_dprintf("*** %s: freeram=%lx unit_len=%lx filelen=%lx mi.erasesize=%x mi.writesize=%x\n", __FUNCTION__, si.freeram, unit_len, filelen, mi.erasesize, mi.writesize);

	if (alloc && !(buf = malloc(unit_len))) {
		error = "Not enough memory";
		goto ERROR;
	}

#ifdef DEBUG_SIMULATE
	if ((of = fopen("/mnt/out.bin", "w")) == NULL) {
		error = "Error creating test file";
		goto ERROR;
	}
#endif

	for (ei.start = ofs = 0, ei.length = unit_len, n = 0, error = NULL, p = buf;
	     ofs < filelen;
	     ofs += n, ei.start += unit_len)
	{
		wlen = n = MIN(unit_len, filelen - ofs);
		if (mi.type == MTD_UBIVOLUME) {
			if ((unsigned long) n >= mi.writesize) {
				n &= ~(mi.writesize - 1);
				wlen = n;
			} else {
				if (!alloc)
					memcpy(bounce_buf, p, n);
				bounce = 1;
				p = bounce_buf;
				wlen = ROUNDUP(n, mi.writesize);
			}
		}

		if (alloc && (safe_fread(p, 1, n, f) != n)) {
			error = "Error reading file";
			break;
		}

		_dprintf("*** %s: ofs=%x n=%lx/%lx ei.start=%x ei.length=%x\n", __FUNCTION__, ofs, n, wlen, ei.start, ei.length);

#ifdef DEBUG_SIMULATE
		if (fwrite(p, 1, wlen, of) != wlen) {
			fclose(of);
			error = "Error writing to test file";
			break;
		}
#else
		if (ei.start == ofs) {
			ioctl(mf, MEMUNLOCK, &ei);
			if (ioctl(mf, MEMERASE, &ei) != 0) {
				snprintf(msg_buf, sizeof(msg_buf), "Error erasing MTD block. (errno %d (%s))", errno, strerror(errno));
				error = msg_buf;
				break;
			}
		}
		if (write(mf, p, wlen) != wlen) {
			snprintf(msg_buf, sizeof(msg_buf), "Error writing to MTD device. (errno %d (%s))", errno, strerror(errno));
			error = msg_buf;
			break;
		}
#endif

		if (!(alloc || bounce))
			p += n;
	}

#ifdef DEBUG_SIMULATE
	fclose(of);
#endif

ERROR:
	if (!alloc)
		munmap((void*) buf, filelen);
	else
		free(buf);

	if (bounce_buf)
		free(bounce_buf);

	if (mf >= 0) {
		/* dummy read to ensure chip(s) are out of lock/suspend state */
		read(mf, &n, sizeof(n));
		close(mf);
	}
	if (f)
		fclose(f);

	crc_done();

	set_action(ACT_IDLE);

	_dprintf("*** %s: %s\n", __FUNCTION__, error ? error : "image successfully flashed");

	return (error ? 1 : 0);
}
#else /* TCONFIG_BCMARM */
int mtd_write_main(int argc, char *argv[])
{
	int mf = -1;
	mtd_info_t mi;
	erase_info_t ei;
	uint32 sig;
	struct trx_header trx;
	struct code_header cth;
#ifdef TCONFIG_BLINK /* RT-N/RTAC */
	struct chk_header netgear_hdr;
	uint32 netgear_chk_len;
#endif
	uint32 crc;
	FILE *f;
	char *buf = NULL;
	const char *error;
	uint32 total;
	uint32 n;
	struct sysinfo si;
	uint32 ofs;
	char c;
	int web = 0;
	char *iname = NULL;
	char *dev = NULL;
	int model;

	while ((c = getopt(argc, argv, "i:d:w")) != -1) {
		switch (c) {
		case 'i':
			iname = optarg;
			break;
		case 'd':
			dev = optarg;
			break;
		case 'w':
			web = 1;
			break;
		}
	}

	if ((iname == NULL) || (dev == NULL)) {
		usage_exit(argv[0], "-i file -d part");
	}

	if (!wait_action_idle(10)) {
		printf("System is busy\n");
		return 1;
	}
	set_action(ACT_WEB_UPGRADE);

	if ((f = fopen(iname, "r")) == NULL) {
		error = "Error opening input file";
		goto ERROR;
	}

	error = "File contains an invalid header";

	if (safe_fread(&sig, 1, sizeof(sig), f) != sizeof(sig)) {
		goto ERROR;
	}

	switch (sig) {
	case 0x47343557: /* W54G	G, GL */
	case 0x53343557: /* W54S	GS */
	case 0x73343557: /* W54s	GS v4 */
	case 0x55343557: /* W54U	SL */
	case 0x31345257: /* WR41	WRH54G */
	case 0x4E303233: /* 320N	WRT320N */
	case 0x4E583233: /* 32XN	E2000 */
	case 0x4E303136: /* 610N	WRT610N v2 */
	case 0x4E583136: /* 61XN	E3000 */
	case 0x30303145: /* E100	E1000 */
	case 0x3031304D: /* M010	M10 */
	case 0x3032304D: /* M020	M20 */
	case 0x3036314E: /* N160	WRT160N */
	case 0x42435745: /* EWCB	WRT300N v1 */
	case 0x4E303133: /* 310N	WRT310N v1/v2 */
//	case 0x32435745: /* EWC2	WRT300N? */
	case 0x3035314E: /* N150	WRT150N */
	case 0x30303234: /* 4200	E4200 */
#ifdef TCONFIG_BLINK /* RT-N/RTAC */
	case 0x30303845: /* E800	E800 */
	case 0x30303945: /* E900	E900 */
	case 0x30323145: /* E120	E1200v1 */
	case 0x32323145: /* E122	E1200v2 */
	case 0x30353145: /* E150	E1500 */
	case 0x30353531: /* 1550	E1550 */
	case 0x58353245: /* E25X	E2500 */
	case 0x33563532: /* 25V3	E2500v3 */
	case 0x30303233: /* 3200	E3200 */
#endif /* TCONFIG_BLINK */
		if (safe_fread(((char *)&cth) + 4, 1, sizeof(cth) - 4, f) != (sizeof(cth) - 4)) {
			goto ERROR;
		}
		if (memcmp(cth.id, "U2ND", 4) != 0) {
			goto ERROR;
		}

		/* trx should be next... */
		if (safe_fread(&sig, 1, sizeof(sig), f) != sizeof(sig)) {
			goto ERROR;
		}
		break;
	case 0x5E24232A: /* Netgear */
		/* get the Netgear header length */
		if (safe_fread(&n, 1, sizeof(n), f) != sizeof(n)) {
			goto ERROR;
		}
#ifdef TCONFIG_BLINK /* RT-N/RTAC */
		else {
			/* and Byte Swap, Netgear header is big endian, machine is little endian */
			n = BCMSWAP32(n);
			_dprintf("*** %s: read Netgear header length: 0x%x\n", __FUNCTION__, n);
		}

		/* read (formatted) Netgear CHK header (now that we know how long it is) */
		// rewind(f); /* disabled, not working for some reason? Adjust structure above to account for this */
		if (safe_fread(&netgear_hdr, 1, n-sizeof(sig)-sizeof(n), f) != (int) (n-sizeof(sig)-sizeof(n))) {
			goto ERROR;
		}
		else
			_dprintf("*** %s: read Netgear header, magic=0x%x, length=0x%x\n", __FUNCTION__, sig, n);
#else
		/* skip the header - we can't use seek() for fifo, so read the rest of the header */
		n = ntohl(n) - sizeof(sig) - sizeof(n);
		if ((buf = malloc(n + 1)) == NULL) {
			error = "Not enough memory";
			goto ERROR;
		}
		if (safe_fread(buf, 1, n, f) != (int) n) {
			goto ERROR;
		}
		free(buf);
		buf = NULL;
#endif /* TCONFIG_BLINK */
		/* TRX (MAGIC) should be next... */
		if (safe_fread(&sig, 1, sizeof(sig), f) != sizeof(sig)) {
			goto ERROR;
		}
		else
			_dprintf("*** %s: read TRX header, magic=0x%x\n", __FUNCTION__, sig);
		break;
	case TRX_MAGIC:
		break;
#ifndef CONFIG_BCMWL6
	case TRX_MAGIC_F7D3301:
	case TRX_MAGIC_F7D3302:
	case TRX_MAGIC_F7D4302:
	case TRX_MAGIC_F5D8235V3:
	case TRX_MAGIC_QA:
		sig = TRX_MAGIC;
		break;
#endif /* !CONFIG_BCMWL6 */
	default:
		/* moto */
		if (safe_fread(&sig, 1, sizeof(sig), f) != sizeof(sig)) {
			goto ERROR;
		}
		switch (sig) {
		case 0x50705710: /* WR850G */
			/* trx */
			if (safe_fread(&sig, 1, sizeof(sig), f) != sizeof(sig)) {
				goto ERROR;
			}
			break;
		default:
			goto ERROR;
		}
		break;
	}

	if (sig != TRX_MAGIC) {
		goto ERROR;
	}
	if ((safe_fread(((char *)&trx) + 4, 1, sizeof(trx) - 4, f) != (sizeof(trx) - 4)) || (trx.len <= sizeof(trx))) {
		goto ERROR;
	}

	model = get_model();

	switch (model) {
#ifndef CONFIG_BCMWL6
	case MODEL_F7D3301:
		trx.magic = TRX_MAGIC_F7D3301;
		break;
	case MODEL_F7D3302:
		trx.magic = TRX_MAGIC_F7D3302;
		break;
	case MODEL_F7D4302:
		trx.magic = TRX_MAGIC_F7D4302;
		break;
	case MODEL_F5D8235v3:
		trx.magic = TRX_MAGIC_F5D8235V3;
		break;
#endif /* !CONFIG_BCMWL6 */
	default:
		trx.magic = sig;
		break;
	}

	if (!crc_init()) {
		error = "Not enough memory";
		goto ERROR;
	}
	crc = crc_calc(0xFFFFFFFF, (char *)&trx.flag_version, sizeof(struct trx_header) - OFFSETOF(struct trx_header, flag_version));

	if (trx.flag_version & TRX_NO_HEADER) {
		trx.len -= sizeof(struct trx_header);
		_dprintf("*** %s: don't write header\n", __FUNCTION__);
	}

	_dprintf("*** %s: trx len=%db 0x%x\n", __FUNCTION__, trx.len, trx.len);

	if ((mf = mtd_open(dev, &mi)) < 0) {
		error = "Error opening MTD device";
		goto ERROR;
	}

	if (mi.erasesize < sizeof(struct trx_header)) {
		error = "Error obtaining MTD information";
		goto ERROR;
	}

	_dprintf("*** %s: mtd size=%6x, erasesize=%6x\n", __FUNCTION__, mi.size, mi.erasesize);

	total = ROUNDUP(trx.len, mi.erasesize);
	if (total > mi.size) {
		error = "File is too big to fit in MTD";
		goto ERROR;
	}

	sysinfo(&si);
	if ((si.freeram * si.mem_unit) > (total + (256 * 1024))) {
		ei.length = total;
	}
	else {
		// ei.length = ROUNDUP((si.freeram - (256 * 1024)), mi.erasesize);
		ei.length = mi.erasesize;
	}
	_dprintf("*** %s: freeram=%ld ei.length=%d total=%u\n", __FUNCTION__, si.freeram, ei.length, total);

	if ((buf = malloc(ei.length)) == NULL) {
		error = "Not enough memory";
		goto ERROR;
	}

#ifdef DEBUG_SIMULATE
	FILE *of;
	if ((of = fopen("/mnt/out.bin", "w")) == NULL) {
		error = "Error creating test file";
		goto ERROR;
	}
#endif /* DEBUG_SIMULATE */

	if (trx.flag_version & TRX_NO_HEADER) {
		ofs = 0;
	}
	else {
		memcpy(buf, &trx, sizeof(trx));
		ofs = sizeof(trx);
	}
	_dprintf("*** %s: trx.len=%ub 0x%x ofs=%ub 0x%x\n", __FUNCTION__, trx.len, trx.len, ofs, ofs);
#ifdef TCONFIG_BLINK /* RT-N/RTAC */
	netgear_chk_len = trx.len;
#endif

	error = NULL;

	for (ei.start = 0; ei.start < total; ei.start += ei.length) {
		n = MIN(ei.length, trx.len) - ofs;
		if (safe_fread(buf + ofs, 1, n, f) != (int) n) {
			error = "Error reading file";
			break;
		}
		trx.len -= (n + ofs);

		crc = crc_calc(crc, buf + ofs, n);

		if (trx.len == 0) {
			_dprintf("*** %s: crc=%8x  trx.crc=%8x\n", __FUNCTION__, crc, trx.crc32);
			if (crc != trx.crc32) {
				error = "Image is corrupt";
				break;
			}
		}

		if (!web) {
			printf("Writing %x-%x\r", ei.start, (ei.start + ei.length) - 1);
		}

		_dprintf("*** %s: ofs=%ub  n=%ub 0x%x  trx.len=%ub  ei.start=0x%x  ei.length=0x%x  mi.erasesize=0x%x\n", __FUNCTION__, ofs, n, n, trx.len, ei.start, ei.length, mi.erasesize);

		n += ofs;

		_dprintf(" erase start=%x len=%x\n", ei.start, ei.length);
		_dprintf(" write %x\n", n);

#ifdef DEBUG_SIMULATE
		if (fwrite(buf, 1, n, of) != n) {
			fclose(of);
			error = "Error writing to test file";
			break;
		}
#else
		ioctl(mf, MEMUNLOCK, &ei);
		if (ioctl(mf, MEMERASE, &ei) != 0
#ifdef TCONFIG_BLINK /* RT-N/RTAC */
		    && model != MODEL_WNR3500LV2
#endif
		) {
			error = "Error erasing MTD block";
			break;
		}
		if (write(mf, buf, n) != (int) n) {
			error = "Error writing to MTD device";
			break;
		}
#endif /* DEBUG_SIMULATE */
		ofs = 0;
	}

	/* Netgear WNR3500L: write fake len and checksum at the end of mtd */
	/* Netgear WNDR4000, WNDR3700v3, WNDR3400, WNDR3400v2, WNDR3400v3 - write real len and checksum */
	char *tmp;
	char imageInfo[8];

	switch (model) {
	case MODEL_WNR3500L:
	case MODEL_WNR2000v2:
#ifdef TCONFIG_BLINK /* RT-N/RTAC */
	case MODEL_WNDR4000:
	case MODEL_WNDR3700v3:
	case MODEL_WNDR3400:
	case MODEL_WNDR3400v2:
	case MODEL_WNDR3400v3:
#endif /* TCONFIG_BLINK */
		error = "Error writing Netgear CRC";

		/*
		 * Netgear CFE has the offset of the checksum hardcoded as
		 * 0x78FFF8 on 8MB flash, and 0x38FFF8 on 4MB flash - in both
		 * cases this is 8 last bytes in the block exactly 6 blocks to the end.
		 * We rely on linux partition to be sized correctly by the kernel,
		 * so the checksum area doesn't fall outside of the linux partition,
		 * and doesn't override the rootfs.
		 * Note: For WNDR4000/WNDR3700v3/WNDR3400v2/WNDR3400v2/WNDR3400v3, the target address (offset) inside Linux is 0x6FFFF8 (displayed by CFE when programmed via tftp)
		 * Note: For WNDR3400, the target address (offset) inside Linux is 0x6CFFF8 (displayed by CFE when programmed via tftp)
		 */
#ifdef TCONFIG_BLINK /* RT-N/RTAC */
		if ((model == MODEL_WNDR4000) || (model == MODEL_WNDR3700v3) || (model == MODEL_WNDR3400) || (model == MODEL_WNDR3400v2) || (model == MODEL_WNDR3400v3)) {
			if (model == MODEL_WNDR3400)
				ofs = 0x6CFFF8;
			else
				ofs = 0x6FFFF8;
			/* Endian "convert" - machine is little endian, but header is big endian */
			crc = BCMSWAP32(netgear_hdr.kernel_chksum);
			n = netgear_chk_len;
		}
		else {
#endif /* TCONFIG_BLINK */
			ofs = (mi.size > (4 *1024 * 1024) ? 0x78FFF8 : 0x38FFF8) - 0x040000;
			n   = 0x00000004; /* fake length - little endian */
			crc = 0x02C0010E; /* fake crc - little endian */
#ifdef TCONFIG_BLINK /* RT-N/RTAC */
		}
#endif

		_dprintf("*** %s: Netgear CRC, data to write: CRC=0x%x, len=0x%x, offset=0x%x\n", __FUNCTION__, crc, n, ofs);
		memcpy(&imageInfo[0], (char *)&n,   4);
		memcpy(&imageInfo[4], (char *)&crc, 4);

		ei.start = (ofs / mi.erasesize) * mi.erasesize;
		ei.length = mi.erasesize;
		_dprintf("*** %s: Netgear CRC: erase start=0x%x, length=0x%x\n", __FUNCTION__, ei.start, ei.length);

		if (lseek(mf, ei.start, SEEK_SET) < 0) {
			_dprintf("*** %s: Netgear CRC: lseek() error\n", __FUNCTION__);
			goto ERROR2;
		}
		if (buf) free(buf);
		if (!(buf = malloc(mi.erasesize))) {
			_dprintf("*** %s: Netgear CRC: malloc() error\n", __FUNCTION__);
			goto ERROR2;
		}
		if (read(mf, buf, mi.erasesize) != (int) mi.erasesize) {
			_dprintf("*** %s: Netgear CRC: read() error\n", __FUNCTION__);
			goto ERROR2;
		}
		if (lseek(mf, ei.start, SEEK_SET) < 0) {
			_dprintf("*** %s: Netgear CRC: lseed() error\n", __FUNCTION__);
			goto ERROR2;
		}

		tmp = buf + (ofs % mi.erasesize);
		memcpy(tmp, imageInfo, sizeof(imageInfo));

#ifdef DEBUG_SIMULATE
		if (fseek(of, ei.start, SEEK_SET) < 0)
			goto ERROR2;
		if (fwrite(buf, 1, mi.erasesize, of) != n)
			goto ERROR2;
		error = NULL;
#else
		ioctl(mf, MEMUNLOCK, &ei);
		if (ioctl(mf, MEMERASE, &ei) != 0) {
			_dprintf("*** %s: Netgear CRC: ioctl() error\n", __FUNCTION__);
			goto ERROR2;
		}
		if (write(mf, buf, mi.erasesize) != (int) mi.erasesize) {
			_dprintf("*** %s: Netgear CRC: write() error\n", __FUNCTION__);
			goto ERROR2;
		}
		error = NULL;
#endif /* DEBUG_SIMULATE */

ERROR2:
		_dprintf("*** %s: %s\n", __FUNCTION__, error ? : "write Netgear fake len/crc completed");
		/* ignore crc write errors */
		error = NULL;
		break;
	}

#ifdef DEBUG_SIMULATE
	fclose(of);
#endif

ERROR:
	if (buf)
		free(buf);
	if (mf >= 0) {
		/* dummy read to ensure chip(s) are out of lock/suspend state */
		read(mf, &n, sizeof(n));
		close(mf);
	}
	if (f)
		fclose(f);

	crc_done();

#ifdef DEBUG_SIMULATE
	set_action(ACT_IDLE);
#endif

	printf("%s\n",  error ? error : "Image successfully flashed");
	_dprintf("*** %s: %s\n", __FUNCTION__, error ? error : "image successfully flashed");

	return (error ? 1 : 0);
}
#endif /* TCONFIG_BCMARM */

#ifdef TCONFIG_BCMARM
/*
 * Check for bad block on MTD device
 * @param	fd	file descriptor for MTD device
 * @param	offset	offset of block to check
 * @return		>0 if bad block, 0 if block is ok or not supported, <0 check failed
 */
int mtd_block_is_bad(int fd, int offset)
{
	int r;
	loff_t o = offset;
	r = ioctl(fd, MEMGETBADBLOCK, &o);
	if (r < 0) {
		if (errno == EOPNOTSUPP) {
			return 0;
		}
	}

	return r;
}

/*
 * Open an MTD device
 * @param       mtd     path to or partition name of MTD device
 * @param       flags   open() flags
 * @return      return value of open()
 */
int mtd_open(const char *mtd, int flags)
{
	FILE *fp;
	char dev[PATH_MAX];
	int i;

	if ((fp = fopen("/proc/mtd", "r"))) {
		while (fgets(dev, sizeof(dev), fp)) {
			if (sscanf(dev, "mtd%d:", &i) && strstr(dev, mtd)) {
				snprintf(dev, sizeof(dev), "/dev/mtd%d", i);
				fclose(fp);
				return open(dev, flags);
			}
		}
		fclose(fp);
	}

	return open(mtd, flags);
}

/*
 * Erase an MTD device
 * @param       mtd     path to or partition name of MTD device
 * @return      0 on success and errno on failure
 */
int mtd_erase(const char *mtd)
{
	int mtd_fd, ret;
	mtd_info_t mtd_info;
	erase_info_t erase_info;

	/* Open MTD device */
	if ((mtd_fd = mtd_open(mtd, O_RDWR)) < 0) {
		perror(mtd);
		return errno;
	}

	/* Get sector size */
	if (ioctl(mtd_fd, MEMGETINFO, &mtd_info) != 0) {
		perror(mtd);
		close(mtd_fd);
		return errno;
	}

	erase_info.length = mtd_info.erasesize;

	printf("Erase MTD %s\n", mtd);
	for (erase_info.start = 0; erase_info.start < mtd_info.size; erase_info.start += mtd_info.erasesize) {
		if ((ret = mtd_block_is_bad(mtd_fd, erase_info.start)) != 0) {
			if (ret > 0) {
				printf("Skipping bad block at 0x%08x\n", erase_info.start);
				continue;
			}
			else {
				printf("Cannot get bad block status at 0x%08x (errno %d (%s))\n", erase_info.start, errno, strerror(errno));
			}
		}
		else {
			(void) ioctl(mtd_fd, MEMUNLOCK, &erase_info);
			if (ioctl(mtd_fd, MEMERASE, &erase_info) != 0) {
				perror(mtd);
				close(mtd_fd);
				return errno;
			}
			else {
				_dprintf("*** %s: erased block at 0x%08x\n", __FUNCTION__, erase_info.start);
			}
		}
	}

	close(mtd_fd);
	printf("Erase MTD %s OK!\n", mtd);

	return 0;
}

static char *base64enc(const char *p, char *buf, int len)
{
	char al[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz"
	            "0123456789+/";
	char *s = buf;

	while (*p) {
		if (s >= buf+len-4)
			break;

		*(s++) = al[(*p >> 2) & 0x3F];
		*(s++) = al[((*p << 4) & 0x30) | ((*(p+1) >> 4) & 0x0F)];
		*s = *(s+1) = '=';
		*(s+2) = 0;
		if (! *(++p))
			break;
		*(s++) = al[((*p << 2) & 0x3C) | ((*(p+1) >> 6) & 0x03)];
		if (! *(++p))
			break;
		*(s++) = al[*(p++) & 0x3F];
	}

	return buf;
}

enum {
	METHOD_GET,
	METHOD_POST
};

static int wget(int method, const char *server, char *buf, size_t count, off_t offset)
{
	char url[PATH_MAX] = { 0 }, *s;
	char *host = url, *path = "", auth[128] = { 0 }, line[512];
	unsigned short port = 80;
	int fd;
	FILE *fp;
	struct sockaddr_in sin;
	int chunked = 0;
	unsigned len = 0;

	if (server == NULL || !strcmp(server, "")) {
		_dprintf("*** %s: wget: null server input\n", __FUNCTION__);
		return (0);
	}

	strncpy(url, server, sizeof(url));

	/* Parse URL */
	if (!strncmp(url, "http://", 7)) {
		port = 80;
		host = url + 7;
	}
	if ((s = strchr(host, '/'))) {
		*s++ = '\0';
		path = s;
	}
	if ((s = strchr(host, '@'))) {
		*s++ = '\0';
		base64enc(host, auth, sizeof(auth));
		host = s;
	}
	if ((s = strchr(host, ':'))) {
		*s++ = '\0';
		port = atoi(s);
	}

	/* Open socket */
	if (!inet_aton(host, &sin.sin_addr))
		return 0;

	sin.sin_family = AF_INET;
	sin.sin_port = htons(port);
	_dprintf("*** %s: Connecting to %s:%u...\n", __FUNCTION__, host, port);

	if ((fd = socket(AF_INET, SOCK_STREAM, 0)) < 0 || connect(fd, (struct sockaddr *) &sin, sizeof(sin)) < 0 || !(fp = fdopen(fd, "r+"))) {
		perror(host);
		if (fd >= 0)
			close(fd);

		return 0;
	}
	_dprintf("*** %s: connected!\n", __FUNCTION__);

	/* Send HTTP request */
	fprintf(fp, "%s /%s HTTP/1.1\r\n", method == METHOD_POST ? "POST" : "GET", path);
	fprintf(fp, "Host: %s\r\n", host);
	fprintf(fp, "User-Agent: wget\r\n");
	if (strlen(auth))
		fprintf(fp, "Authorization: Basic %s\r\n", auth);
	if (offset)
		fprintf(fp, "Range: bytes=%ld-\r\n", offset);
	if (method == METHOD_POST) {
		fprintf(fp, "Content-Type: application/x-www-form-urlencoded\r\n");
		fprintf(fp, "Content-Length: %d\r\n\r\n", (int) strlen(buf));
		fputs(buf, fp);
	} else
		fprintf(fp, "Connection: close\r\n\r\n");

	/* Check HTTP response */
	_dprintf("*** %s: HTTP request sent, awaiting response...\n", __FUNCTION__);
	if (fgets(line, sizeof(line), fp)) {
		_dprintf("*** %s: %s\n", __FUNCTION__, line);
		for (s = line; *s && !isspace((int)*s); s++);
		for (; isspace((int)*s); s++);
		switch (atoi(s)) {
			case 200: if (offset) goto done; else break;
			case 206: if (offset) break; else goto done;
			default: goto done;
		}
	}
	/* Parse headers */
	while (fgets(line, sizeof(line), fp)) {
		_dprintf("*** %s: %s\n", __FUNCTION__, line);
		for (s = line; *s == '\r'; s++);
		if (*s == '\n')
			break;
		if (!strncasecmp(s, "Content-Length:", 15)) {
			for (s += 15; isblank(*s); s++);
			chomp(s);
			len = atoi(s);
		}
		else if (!strncasecmp(s, "Transfer-Encoding:", 18)) {
			for (s += 18; isblank(*s); s++);
			chomp(s);
			if (!strncasecmp(s, "chunked", 7))
				chunked = 1;
		}
	}

	if (chunked && fgets(line, sizeof(line), fp))
		len = strtol(line, NULL, 16);

	len = (len > count) ? count : len;
	len = fread(buf, 1, len, fp);

done:
	/* Close socket */
	fflush(fp);
	fclose(fp);

	return len;
}

int http_get(const char *server, char *buf, size_t count, off_t offset)
{
	return wget(METHOD_GET, server, buf, count, offset);
}

/*
 * Write a file to an MTD device
 * @param       path    file to write or a URL
 * @param       mtd     path to or partition name of MTD device
 * @return      0 on success and errno on failure
 */
int mtd_write(const char *path, const char *mtd)
{
	int mtd_fd = -1;
	mtd_info_t mtd_info;
	erase_info_t erase_info;

	struct sysinfo info;
	struct trx_header trx;
	unsigned long crc;

#ifdef CONFIG_FAILSAFE_UPGRADE
	int model = get_model();

	if (model == MODEL_EA6700 || model == MODEL_EA6400 || model == MODEL_EA6350v1 || model == MODEL_EA6350v2) {
		if (nvram_match("bootpartition", "1")) {
			mtd = "linux";
			nvram_set("bootpartition", "0");
			nvram_commit();
		}
		else {
			mtd = "linux2";
			nvram_set("bootpartition", "1");
			nvram_commit();
		}
	}
#endif
	FILE *fp;
	char *buf = NULL;
	unsigned long count, len, off;
	int ret = -1;

	if ((fp = fopen(path, "r")))
		count = safe_fread(&trx, 1, sizeof(struct trx_header), fp);
	else
		count = http_get(path, (char *) &trx, sizeof(struct trx_header), 0);

	if (count < sizeof(struct trx_header)) {
		fprintf(stderr, "%s: File is too small (%ld bytes)\n", path, count);
		goto fail;
	}

	/* Open MTD device and get sector size */
	if ((mtd_fd = mtd_open(mtd, O_RDWR)) < 0 || ioctl(mtd_fd, MEMGETINFO, &mtd_info) != 0 || mtd_info.erasesize < sizeof(struct trx_header)) {
		perror(mtd);
		goto fail;
	}

	if (trx.magic != TRX_MAGIC || trx.len > mtd_info.size || trx.len < sizeof(struct trx_header)) {
		if (trx.magic != TRX_MAGIC)
			fprintf(stderr, "Trx magic %d != 30524448 Expected TRX_MAGIC \n", trx.magic);
		if (trx.len > mtd_info.size)
			fprintf(stderr, "trx size %d > memory size %d \n", trx.len, mtd_info.size);
		if (trx.len < sizeof(struct trx_header))
			fprintf(stderr, "trx size %d < header size %d \n", trx.len, sizeof(struct trx_header));

		goto fail;
	}

	/* Allocate temporary buffer */
	/* See if we have enough memory to store the whole file */
	sysinfo(&info);
	if (info.freeram >= trx.len) {
		erase_info.length = ROUNDUP(trx.len, mtd_info.erasesize);
		if (!(buf = malloc(erase_info.length)))
			erase_info.length = mtd_info.erasesize;
	}
	/* fallback to smaller buffer */
	else {
		erase_info.length = mtd_info.erasesize;
		buf = NULL;
	}
	if (!buf && (!(buf = malloc(erase_info.length)))) {
		perror("malloc");
		goto fail;
	}

	/* Calculate CRC over header */
	crc = hndcrc32((uint8 *) &trx.flag_version, sizeof(struct trx_header) - OFFSETOF(struct trx_header, flag_version), CRC32_INIT_VALUE);

	if (trx.flag_version & TRX_NO_HEADER)
		trx.len -= sizeof(struct trx_header);

	/* Write file or URL to MTD device */
	for (erase_info.start = 0; erase_info.start < trx.len; erase_info.start += count) {
		len = MIN(erase_info.length, trx.len - erase_info.start);
		if ((trx.flag_version & TRX_NO_HEADER) || erase_info.start)
			count = off = 0;
		else {
			count = off = sizeof(struct trx_header);
			memcpy(buf, &trx, sizeof(struct trx_header));
		}
		if (fp)
			count += safe_fread(&buf[off], 1, len - off, fp);
		else
			count += http_get(path, &buf[off], len - off, erase_info.start + off);

		if (count < len) {
			fprintf(stderr, "%s: Truncated file (actual %ld expect %ld)\n", path, count - off, len - off);
			goto fail;
		}
		/* Update CRC */
		crc = hndcrc32((uint8 *)&buf[off], count - off, crc);
		/* Check CRC before writing if possible */
		if (count == trx.len) {
			if (crc != trx.crc32) {
				fprintf(stderr, "%s: Bad CRC\n", path);
				goto fail;
			}
		}
		/* Do it */
		(void) ioctl(mtd_fd, MEMUNLOCK, &erase_info);
		if (ioctl(mtd_fd, MEMERASE, &erase_info) != 0 || (unsigned long) write(mtd_fd, buf, count) != count) {
			perror(mtd);
			goto fail;
		}
	}

#ifdef PLC
	eval("gigle_util restart");
	nvram_set("plc_pconfig_state", "2");
	nvram_commit();
#endif

	printf("%s: CRC OK - Image successfully flashed\n", mtd);
	ret = 0;

fail:
	if (buf) {
		/* Dummy read to ensure chip(s) are out of lock/suspend state */
		(void) read(mtd_fd, buf, 2);
		free(buf);
	}

	if (mtd_fd >= 0)
		close(mtd_fd);
	if (fp)
		fclose(fp);

	return ret;
}
#endif /* TCONFIG_BCMARM */
