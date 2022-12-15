/*
 *
 * Tomato Firmware
 * USB Support Module
 * Fixes/updates (C) 2018 - 2022 pedro
 *
 */

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <ctype.h>
#include <sys/stat.h>
#include <stdarg.h>
#include <sys/ioctl.h>
#include <net/if.h>
#include <dirent.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <sys/sysinfo.h>
#include <sys/types.h>

#include <bcmnvram.h>
#include <bcmdevs.h>
#include <wlutils.h>

#include "shutils.h"
#include "shared.h"


/* Execute a function for each disc partition on the specified controller.
 *
 * Directory /dev/discs/ looks like this:
 * disc0 -> ../scsi/host0/bus0/target0/lun0/
 * disc1 -> ../scsi/host1/bus0/target0/lun0/
 * disc2 -> ../scsi/host2/bus0/target0/lun0/
 * disc3 -> ../scsi/host2/bus0/target0/lun1/
 *
 * Scsi host 2 supports multiple drives.
 * Scsi host 0 & 1 support one drive.
 *
 * For attached drives, like this.  If not attached, there is no "part#" item.
 * Here, only one drive, with 2 partitions, is plugged in.
 * /dev/discs/disc0/disc
 * /dev/discs/disc0/part1
 * /dev/discs/disc0/part2
 * /dev/discs/disc1/disc
 * /dev/discs/disc2/disc
 *
 * Which is the same as:
 * /dev/scsi/host0/bus0/target0/lun0/disc
 * /dev/scsi/host0/bus0/target0/lun0/part1
 * /dev/scsi/host0/bus0/target0/lun0/part2
 * /dev/scsi/host1/bus0/target0/lun0/disc
 * /dev/scsi/host2/bus0/target0/lun0/disc
 * /dev/scsi/host2/bus0/target0/lun1/disc
 *
 * Implementation notes:
 * Various mucking about with a disc that just got plugged in or unplugged
 * will make the scsi subsystem try a re-validate, and read the partition table of the disc.
 * This will make sure the partitions show up.
 *
 * It appears to try to do the revalidate and re-read & update the partition
 * information when this code does the "readdir of /dev/discs/disc0/?".  If the
 * disc has any mounted partitions the revalidate will be rejected.  So the
 * current partition info will remain.  On an unplug event, when it is doing the
 * readdir's, it will try to do the revalidate as we are doing the readdir's.
 * But luckily they'll be rejected, otherwise the later partitions will disappear as
 * soon as we get the first one.
 * But be very careful!  If something goes not exactly right, the partition entries
 * will disappear before we've had a chance to unmount from them.
 *
 * To avoid this automatic revalidation, we go through /proc/partitions looking for the partitions
 * that /dev/discs point to.  That will avoid the implicit revalidate attempt.
 * 
 * If host < 0, do all hosts.   If >= 0, it is the host number to do.
 *
 */

/* check if the block device has no partition */
int is_no_partition(const char *discname)
{
	FILE *procpt;
	char line[128], ptname[32];
	int ma, mi, sz;
	int count = 0;

	if ((procpt = fopen("/proc/partitions", "r"))) {
		while (fgets(line, sizeof(line), procpt)) {
			if (sscanf(line, " %d %d %d %[^\n ]", &ma, &mi, &sz, ptname) != 4)
				continue;
			if (strstr(ptname, discname))
				count++;
		}
	}

	return (count == 1);
}

int exec_for_host(int host, int obsolete, uint flags, host_exec func)
{
	DIR *usb_dev_disc;
	DIR *dir_host;
	char bfr[256];		/* Will be: /dev/discs/disc# */
	char ptname[32];	/* Will be: discDN_PN */
	char dsname[16];	/* Will be: discDN */
	int host_no;		/* SCSI controller/host */
	struct dirent *dp;
	FILE *prt_fp;
	int siz;
	char line[256];
	char hostbuf[16];
	int result = 0;

	flags |= EFH_1ST_HOST;

	/*
	 * Scsi block devices in kernel 2.6 (for attached devices) can be found as
	 * /sys/bus/scsi/devices/<host_no>:x:x:x/block:[sda|sdb|...]
	 */
	if ((usb_dev_disc = opendir("/sys/bus/scsi/devices"))) {
		sprintf(hostbuf, "%d:", host);

		while ((dp = readdir(usb_dev_disc))) {
			if (host >= 0 && strncmp(dp->d_name, hostbuf, strlen(hostbuf)) != 0)
				continue;
			if (sscanf(dp->d_name, "%d:%*s:%*s:%*s", &host_no) != 1)
				continue;
			sprintf(bfr, "/sys/bus/scsi/devices/%s", dp->d_name);
			if ((dir_host = opendir(bfr))) {
				while ((dp = readdir(dir_host))) {
					if (strncmp(dp->d_name, "block:", 6) != 0)
						continue;
					strncpy(dsname, dp->d_name + 6, sizeof(dsname));
					siz = strlen(dsname);

					flags |= EFH_1ST_DISC;
					if (func && (prt_fp = fopen("/proc/partitions", "r"))) {
						while (fgets(line, sizeof(line) - 2, prt_fp)) {
							if (sscanf(line, " %*s %*s %*s %s", ptname) == 1) {
								if (strncmp(ptname, dsname, siz) == 0) {
									if ((strcmp(ptname, dsname) == 0) && !is_no_partition(dsname))
										continue;
									sprintf(line, "/dev/%s", ptname);
									result = (*func)(line, host_no, dsname, ptname, flags) || result;
									flags &= ~(EFH_1ST_HOST | EFH_1ST_DISC);
								}
							}
						}
						fclose(prt_fp);
					}
				}
				closedir(dir_host);
			}
		}
		closedir(usb_dev_disc);
	}

	return result;
}

/* Concept taken from the e2fsprogs/ismounted.c.
 * Find wherever 'file' (actually: device) is mounted.
 * Either the exact same device-name, or another device-name.
 * The latter is detected by comparing the rdev or dev&inode.
 * So aliasing won't fool us---we'll still find if it's mounted.
 * Return its mnt entry.
 * In particular, the caller would look at the mnt->mountpoint.
 *
 * Find the matching devname(s) in mounts or swaps.
 * If func is supplied, call it for each match.  If not, return mnt on the first match.
 */
static inline int is_same_device(char *fsname, dev_t file_rdev, dev_t file_dev, ino_t file_ino)
{
	struct stat st_buf;

	if (stat(fsname, &st_buf) == 0) {
		if (S_ISBLK(st_buf.st_mode)) {
			if (file_rdev && (file_rdev == st_buf.st_rdev))
				return 1;
		}
		else {
			if (file_dev && ((file_dev == st_buf.st_dev) &&
				(file_ino == st_buf.st_ino)))
				return 1;
			/* Check for [swap]file being on the device. */
			if (file_dev == 0 && file_ino == 0 && file_rdev == st_buf.st_dev)
				return 1;
		}
	}
	return 0;
}

struct mntent *findmntents(char *file, int swp, int (*func)(struct mntent *mnt, uint flags), uint flags)
{
	struct mntent	*mnt;
	struct stat	st_buf;
	dev_t		file_dev=0, file_rdev=0;
	ino_t		file_ino=0;
	FILE		*f;

	if ((f = setmntent(swp ? "/proc/swaps": "/proc/mounts", "r")) == NULL)
		return NULL;

	if (stat(file, &st_buf) == 0) {
		if (S_ISBLK(st_buf.st_mode)) {
			file_rdev = st_buf.st_rdev;
		}
		else {
			file_dev = st_buf.st_dev;
			file_ino = st_buf.st_ino;
		}
	}
	while ((mnt = getmntent(f)) != NULL) {
		/* Always ignore rootfs mount */
		if (strcmp(mnt->mnt_fsname, "rootfs") == 0)
			continue;

		if (strcmp(file, mnt->mnt_fsname) == 0 ||
		    strcmp(file, mnt->mnt_dir) == 0 ||
		    is_same_device(mnt->mnt_fsname, file_rdev , file_dev, file_ino)) {
			if (func == NULL)
				break;
			(*func)(mnt, flags);
		}
	}

	endmntent(f);
	return mnt;
}

/* Simulate a hotplug event, as if a USB storage device
 * got plugged or unplugged.
 */
void add_remove_usbhost(char *host, int add)
{
	setenv("ACTION", add ? "add" : "remove", 1);
	setenv("SCSI_HOST", host, 1);
	setenv("PRODUCT", host, 1);
	setenv("INTERFACE", "TOMATO/0", 1);

	/* don't use value from /proc/sys/kernel/hotplug 
	 * since it may be overriden by a user
	 */
	system("/sbin/hotplug usb");

	unsetenv("INTERFACE");
	unsetenv("PRODUCT");
	unsetenv("SCSI_HOST");
	unsetenv("ACTION");
}

/****************************************************/
/* Use busybox routines to get labels for fat & ext */
/* Probe for label the same way that mount does.    */
/****************************************************/

#define VOLUME_ID_LABEL_SIZE		64
#define VOLUME_ID_UUID_SIZE		36
#define SB_BUFFER_SIZE			0x11000

struct volume_id
{
	int		fd;
	int		error;
	size_t		sbbuf_len;
	size_t		seekbuf_len;
	uint8_t		*sbbuf;
	uint8_t		*seekbuf;
	uint64_t	seekbuf_off;
	char		label[VOLUME_ID_LABEL_SIZE+1];
	char		uuid[VOLUME_ID_UUID_SIZE+1];
};

extern void *volume_id_get_buffer();
extern void volume_id_free_buffer();
extern int volume_id_probe_ext();
extern int volume_id_probe_vfat();
extern int volume_id_probe_ntfs();
#ifdef HFS
extern int volume_id_probe_hfs_hfsplus();
#endif
extern int volume_id_probe_exfat();
extern int volume_id_probe_linux_swap();

/* magic for ext2/3/4 detection */
int check_magic(char *buf, char *magic)
{
	if (!strncmp(magic, "ext3_chk", 8)) {
		if (!((*buf)&4))
			return 0;
		if (*(buf+4) >= 0x40)
			return 0;
		if (*(buf+8) >= 8)
			return 0;
		return 1;
	}

	if (!strncmp(magic, "ext4_chk", 8)) {
		if (!((*buf)&4))
			return 0;
		if (*(buf+4) > 0x3F)
			return 1;
		if (*(buf+4) >= 0x40)
			return 0;
		if (*(buf+8) <= 7)
			return 0;
		return 1;
	}

	return 0;
}

/* Put the label in *label and uuid in *uuid.
 * Return fstype if determined.
 */
char *find_label_or_uuid(char *dev_name, char *label, char *uuid)
{
	struct volume_id id;
	char *fstype = NULL;

	memset(&id, 0x00, sizeof(id));
	if (label) *label = 0;
	if (uuid) *uuid = 0;
	if ((id.fd = open(dev_name, O_RDONLY)) < 0)
		return NULL;

	volume_id_get_buffer(&id, 0, SB_BUFFER_SIZE);

	/* detect swap */
	if (!id.error && volume_id_probe_linux_swap(&id) == 0)
		fstype = "swap";
	/* detect vfat */
	else if (!id.error && volume_id_probe_vfat(&id) == 0)
		fstype = "vfat";
	/* detect ext2/3/4 */
	else if (!id.error && volume_id_probe_ext(&id) == 0) {
		if (id.sbbuf[0x438] == 0x53 && id.sbbuf[0x439] == 0xEF) {
			if(check_magic((char *) &id.sbbuf[0x45c], "ext3_chk"))
				fstype = "ext3";
			else if(check_magic((char *) &id.sbbuf[0x45c], "ext4_chk"))
				fstype = "ext4";
			else
				fstype = "ext2";
		}
	}
	/* detect ntfs */
	else if (!id.error && volume_id_probe_ntfs(&id) == 0)
		fstype = "ntfs";
#ifdef HFS
	/* detect hfs */
	else if (!id.error && volume_id_probe_hfs_hfsplus(&id) == 0) {
		if ((!memcmp(id.sbbuf+1032, "HFSJ", 4)) || (!memcmp(id.sbbuf+1032, "H+", 2)) || (!memcmp(id.sbbuf+1024, "H+", 2)) || (!memcmp(id.sbbuf+1024, "HX", 2))) {
			if (id.sbbuf[1025] == 0x58)
				fstype = "hfsplus";	/* hfs+jx */
			else
				fstype = "hfsplus";	/* hfs+j */
		}
		else
			fstype = "hfs";
	}
#endif
	/* detect exfat */
	else if (!id.error && volume_id_probe_exfat(&id) == 0)
		fstype = "exfat";
	/* -> unknown FS */
	else if (!id.error)
		fstype = "unknown";

	volume_id_free_buffer(&id);

	if (label && (*id.label != 0))
		strcpy(label, id.label);

	if (uuid && (*id.uuid != 0))
		strcpy(uuid, id.uuid);

	close(id.fd);

	return (fstype);
}

void *xmalloc(size_t siz)
{
	return (malloc(siz));
}

void *xrealloc(void *old, size_t size)
{
	return realloc(old, size);
}

ssize_t full_read(int fd, void *buf, size_t len)
{
	return read(fd, buf, len);
}
