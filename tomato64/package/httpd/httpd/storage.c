/*
 * storage.c
 *
 * Storage health for Admin > Storage. Everything is discovered at run time
 * from sysfs, so one build covers every target: a board's kernel config
 * already decides which of eMMC, NAND, NVMe and SATA can show up, and only
 * sysfs knows which actually did - an empty M.2 slot, an x86 box with or
 * without eMMC, a BPI-R3 strapped for NAND or for NOR.
 *
 *   eMMC  /sys/block/mmcblkN/device/{life_time,pre_eol_info}: the JEDEC 5.0
 *         health registers, which the mmc core already reads out of EXT_CSD.
 *   NAND  /sys/class/mtd/mtdN/{bad_blocks,corrected_bits,ecc_failures,...}
 *         and /sys/class/ubi/ubiN/{bad_peb_count,reserved_for_bad,max_ec}.
 *         There is no SMART for raw NAND; these counters are all there is.
 *   NVMe, SATA, USB disks
 *         smartctl -j, passed through to the page as an escaped string for
 *         it to JSON.parse. Absent on targets that do not ship smartmontools,
 *         and then those disks are listed without health data.
 *
 * Every value goes out as a JS string. The page parses numbers itself, which
 * keeps a malformed sysfs read from breaking the script it is embedded in.
 *
 * Hooks into the rest of httpd: one row in aspapi[] and the include that
 * declares it.
 */

#include "tomato.h"
#include "storage.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <poll.h>
#include <signal.h>
#include <time.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>

#define SMARTCTL		"/usr/sbin/smartctl"
#define SMARTCTL_TIMEOUT_MS	10000		/* a wedged USB bridge must not hang httpd */
#define SMARTCTL_MAX		(256 * 1024)

/* ------------------------------------------------------------- output -- */

static int comma;

static void s_key(const char *name)
{
	web_printf("%s%s:", comma ? "," : "", name);
	comma = 1;
}

static void s_str(const char *name, const char *value)
{
	s_key(name);
	web_puts("'");
	web_putj(value ? value : "");
	web_puts("'");
}

static void s_open(const char *name, char brace)
{
	if (name)
		s_key(name);
	else if (comma)
		web_puts(",");

	web_printf("%c", brace);
	comma = 0;
}

static void s_close(char brace)
{
	web_printf("%c", brace);
	comma = 1;
}

/* One sysfs attribute, trailing newline dropped. Missing reads as "". */
static void s_attr(const char *name, const char *dir, const char *attr)
{
	char path[512], buf[256];
	int n;

	snprintf(path, sizeof(path), "%s/%s", dir, attr);
	if ((n = f_read_string(path, buf, sizeof(buf))) < 0)
		n = 0;

	buf[n] = 0;
	while (n > 0 && isspace((unsigned char)buf[n - 1]))
		buf[--n] = 0;

	s_str(name, buf);
}

/* -------------------------------------------------------------- helpers -- */

static int match_num(const char *s, const char *prefix)
{
	size_t n = strlen(prefix);

	if (strncmp(s, prefix, n) != 0 || !s[n])
		return 0;

	for (s += n; *s; s++)
		if (!isdigit((unsigned char)*s))
			return 0;

	return 1;
}

/* Kernel block names are [a-z0-9] in practice; anything else is refused
 * before it gets near an exec. */
static int safe_name(const char *s)
{
	if (!*s)
		return 0;

	for (; *s; s++)
		if (!islower((unsigned char)*s) && !isdigit((unsigned char)*s))
			return 0;

	return 1;
}

/* Run argv, capture stdout into a malloc'd string. Killed at the deadline.
 * No shell, so nothing in argv is ever interpreted. */
static char *run_capture(char *const argv[], int timeout_ms)
{
	struct timespec start, now;
	struct pollfd pfd;
	char *buf;
	size_t len = 0;
	ssize_t r;
	int fds[2], devnull, elapsed, status, i, killed = 0;
	pid_t pid;

	if ((buf = malloc(SMARTCTL_MAX + 1)) == NULL)
		return NULL;

	if (pipe(fds) < 0) {
		free(buf);
		return NULL;
	}

	if ((pid = fork()) < 0) {
		close(fds[0]);
		close(fds[1]);
		free(buf);
		return NULL;
	}

	if (pid == 0) {
		devnull = open("/dev/null", O_RDWR);
		if (devnull >= 0) {
			dup2(devnull, STDIN_FILENO);
			dup2(devnull, STDERR_FILENO);
		}
		dup2(fds[1], STDOUT_FILENO);
		close(fds[0]);
		close(fds[1]);
		execv(argv[0], argv);
		_exit(127);
	}

	close(fds[1]);
	clock_gettime(CLOCK_MONOTONIC, &start);

	pfd.fd = fds[0];
	pfd.events = POLLIN;

	for (;;) {
		clock_gettime(CLOCK_MONOTONIC, &now);
		elapsed = (now.tv_sec - start.tv_sec) * 1000 + (now.tv_nsec - start.tv_nsec) / 1000000;
		if (elapsed >= timeout_ms) {
			kill(pid, SIGKILL);
			killed = 1;
			len = 0;	/* a truncated JSON document is worse than none */
			break;
		}

		r = poll(&pfd, 1, timeout_ms - elapsed);
		if (r < 0 && errno == EINTR)
			continue;
		if (r <= 0)
			continue;	/* deadline is checked at the top */

		r = read(fds[0], buf + len, SMARTCTL_MAX - len);
		if (r < 0 && errno == EINTR)
			continue;
		if (r <= 0)
			break;

		len += r;
		if (len >= SMARTCTL_MAX) {
			kill(pid, SIGKILL);
			killed = 1;
			len = 0;
			break;
		}
	}

	close(fds[0]);

	if (killed) {
		/* A child stuck in uninterruptible I/O (a wedged USB bridge) does
		 * not die until that I/O returns, so a blocking wait would hang
		 * httpd after all. Give it a moment, then leave it. */
		for (i = 0; i < 10; i++) {
			if (waitpid(pid, &status, WNOHANG) != 0)
				break;
			usleep(10000);
		}
	}
	else {
		while (waitpid(pid, &status, 0) < 0 && errno == EINTR)
			;
	}

	buf[len] = 0;

	return buf;
}

/* ---------------------------------------------------------- block disks -- */

static const char *disk_bus(const char *name, const char *real)
{
	if (strncmp(name, "mmcblk", 6) == 0)
		return "mmc";
	if (strncmp(name, "nvme", 4) == 0)
		return "nvme";
	if (strstr(real, "/usb"))
		return "usb";
	if (strstr(real, "/virtio"))
		return "virtio";
	if (strstr(real, "/ata"))
		return "sata";

	return "other";
}

/* Whole disks only: partitions live under their disk, not in /sys/block,
 * but a lot of other things do. */
static int disk_wanted(const char *name)
{
	static const char *const skip[] = {
		"loop", "ram", "zram", "mtdblock", "ubiblock", "dm-", "md", "nbd", "sr", "fd", NULL
	};
	int i;

	for (i = 0; skip[i]; i++)
		if (strncmp(name, skip[i], strlen(skip[i])) == 0)
			return 0;

	/* eMMC hardware partitions: same chip as mmcblkN, same health registers */
	if (strncmp(name, "mmcblk", 6) == 0 && (strstr(name, "boot") || strstr(name, "rpmb")))
		return 0;

	return safe_name(name);
}

static void put_smart(const char *name, const char *bus, int have_smartctl)
{
	char dev[300];
	char *argv[8];
	char *out;
	char *p;
	int i = 0;

	if (!have_smartctl)
		return;

	if (strcmp(bus, "nvme") == 0) {
		/* the controller, not the namespace: nvme0n1 -> nvme0 */
		snprintf(dev, sizeof(dev), "/dev/%s", name);
		if ((p = strchr(dev + 9, 'n')) != NULL)
			*p = 0;
	}
	else if (strcmp(bus, "sata") == 0 || strcmp(bus, "usb") == 0 || strcmp(bus, "other") == 0)
		snprintf(dev, sizeof(dev), "/dev/%s", name);
	else
		return;	/* mmc and virtio have no SMART */

	argv[i++] = SMARTCTL;
	argv[i++] = "-j";
	argv[i++] = "-a";
	argv[i++] = "-n";
	argv[i++] = "standby";	/* never spin up a sleeping drive to read it */
	argv[i++] = dev;
	argv[i] = NULL;

	if ((out = run_capture(argv, SMARTCTL_TIMEOUT_MS)) != NULL) {
		s_str("smart", out);
		free(out);
	}
}

static void put_disks(void)
{
	DIR *d;
	struct dirent *de;
	char dir[300], path[320], real[512], size[32];
	int have_smartctl = f_exists(SMARTCTL);
	const char *bus;

	s_open("disks", '[');

	if ((d = opendir("/sys/block")) == NULL) {
		s_close(']');
		return;
	}

	while ((de = readdir(d)) != NULL) {
		if (de->d_name[0] == '.' || !disk_wanted(de->d_name))
			continue;

		snprintf(dir, sizeof(dir), "/sys/block/%s", de->d_name);
		if (!realpath(dir, real))
			real[0] = 0;

		/* empty card reader slots report a zero size */
		snprintf(path, sizeof(path), "%s/size", dir);
		if (f_read_string(path, size, sizeof(size)) <= 0 || atoll(size) == 0)
			continue;

		bus = disk_bus(de->d_name, real);

		s_open(NULL, '{');
		s_str("name", de->d_name);
		s_str("bus", bus);
		s_attr("size", dir, "size");			/* 512-byte sectors */
		s_attr("rotational", dir, "queue/rotational");
		s_attr("removable", dir, "removable");

		if (strcmp(bus, "mmc") != 0) {
			s_attr("vendor", dir, "device/vendor");
			s_attr("model", dir, "device/model");
		}

		if (strcmp(bus, "mmc") == 0) {
			s_attr("type", dir, "device/type");	/* MMC, SD, SDIO */
			s_attr("model", dir, "device/name");
			s_attr("manfid", dir, "device/manfid");
			s_attr("date", dir, "device/date");
			s_attr("fwrev", dir, "device/fwrev");
			s_attr("life", dir, "device/life_time");	/* "0xAA 0xBB" */
			s_attr("eol", dir, "device/pre_eol_info");
		}
		else if (strcmp(bus, "nvme") == 0) {
			s_attr("fwrev", dir, "device/firmware_rev");
		}

		put_smart(de->d_name, bus, have_smartctl);
		s_close('}');
	}

	closedir(d);
	s_close(']');
}

/* ------------------------------------------------------------------ MTD -- */

/* A partition's device sits under the chip it was carved from, and a
 * sub-partition (bcm53xx's TRX parser splits "firmware" into "linux" and
 * "ubi") directly inside its parent partition's directory:
 *
 *   /sys/devices/.../spi0.0/mtd/mtd3
 *   /sys/devices/.../18028000.nand-controller/mtd/mtd2/mtd3
 *
 * The kernel sets a sub-partition's dev.parent to the parent partition
 * (mtdpart.c allocate_partition), and because that parent is itself an mtd
 * class device the driver core adds no "mtd/" glue directory between them.
 * With MTD_PARTITIONED_MASTER the master shows up the same way, as the
 * parent of every partition.
 *
 * The chip is whatever comes before the first "/mtd/"; the parent partition,
 * if any, is the second-to-last path component. */
static void put_mtd_topology(const char *real)
{
	char chip[512], parent[64];
	const char *first, *last, *prev, *p;
	size_t n;

	chip[0] = parent[0] = 0;

	if ((first = strstr(real, "/mtd/")) != NULL) {
		n = first - real;
		if (n >= sizeof(chip))
			n = sizeof(chip) - 1;
		memcpy(chip, real, n);
		chip[n] = 0;

		/* last = slash before this mtdN, prev = slash before the one above it */
		last = strrchr(real, '/');
		for (prev = last - 1; prev > first && *prev != '/'; prev--)
			;

		if (prev > first) {	/* stopped short of the "/mtd" glue directory */
			n = last - prev - 1;
			if (n < sizeof(parent)) {
				memcpy(parent, prev + 1, n);
				parent[n] = 0;
			}
		}
	}

	p = strrchr(chip, '/');
	s_str("chip", p ? p + 1 : chip);
	s_str("chippath", chip);
	s_str("parent", match_num(parent, "mtd") ? parent + 3 : "");
}

static void put_mtd(void)
{
	DIR *d;
	struct dirent *de;
	char dir[300], real[512];

	s_open("mtd", '[');

	if ((d = opendir("/sys/class/mtd")) == NULL) {
		s_close(']');
		return;
	}

	while ((de = readdir(d)) != NULL) {
		if (!match_num(de->d_name, "mtd"))	/* skips the mtdNro twins */
			continue;

		snprintf(dir, sizeof(dir), "/sys/class/mtd/%s", de->d_name);
		if (!realpath(dir, real))
			real[0] = 0;

		s_open(NULL, '{');
		s_str("n", de->d_name + 3);
		s_attr("name", dir, "name");
		s_attr("type", dir, "type");
		s_attr("size", dir, "size");
		s_attr("erasesize", dir, "erasesize");
		s_attr("writesize", dir, "writesize");
		s_attr("bad", dir, "bad_blocks");
		s_attr("bbt", dir, "bbt_blocks");
		s_attr("corrected", dir, "corrected_bits");
		s_attr("failures", dir, "ecc_failures");
		s_attr("strength", dir, "ecc_strength");
		put_mtd_topology(real);
		s_close('}');
	}

	closedir(d);
	s_close(']');
}

/* ------------------------------------------------------------------ UBI -- */

static void put_ubi_volumes(const char *ubi)
{
	DIR *d;
	struct dirent *de;
	char prefix[300], dir[300];

	snprintf(prefix, sizeof(prefix), "%s_", ubi);

	s_open("vols", '[');

	if ((d = opendir("/sys/class/ubi")) == NULL) {
		s_close(']');
		return;
	}

	while ((de = readdir(d)) != NULL) {
		if (!match_num(de->d_name, prefix))
			continue;

		snprintf(dir, sizeof(dir), "/sys/class/ubi/%s", de->d_name);

		s_open(NULL, '{');
		s_attr("name", dir, "name");
		s_attr("type", dir, "type");
		s_attr("bytes", dir, "data_bytes");
		s_attr("lebs", dir, "reserved_ebs");
		s_close('}');
	}

	closedir(d);
	s_close(']');
}

static void put_ubi(void)
{
	DIR *d;
	struct dirent *de;
	char dir[300];

	s_open("ubi", '[');

	if ((d = opendir("/sys/class/ubi")) == NULL) {
		s_close(']');
		return;
	}

	while ((de = readdir(d)) != NULL) {
		if (!match_num(de->d_name, "ubi"))	/* skips ubiN_M volumes and "version" */
			continue;

		snprintf(dir, sizeof(dir), "/sys/class/ubi/%s", de->d_name);

		s_open(NULL, '{');
		s_str("n", de->d_name + 3);
		s_attr("mtd", dir, "mtd_num");
		s_attr("pebs", dir, "total_eraseblocks");
		s_attr("avail", dir, "avail_eraseblocks");
		s_attr("ebsize", dir, "eraseblock_size");
		s_attr("max_ec", dir, "max_ec");
		s_attr("bad", dir, "bad_peb_count");
		s_attr("reserved", dir, "reserved_for_bad");
		s_attr("ro", dir, "ro_mode");
		put_ubi_volumes(de->d_name);
		s_close('}');
	}

	closedir(d);
	s_close(']');
}

/* ----------------------------------------------------------------- entry -- */

void asp_storage(int argc, char **argv)
{
	comma = 0;

	web_puts("\nstorage = {");
	s_str("smartctl", f_exists(SMARTCTL) ? "1" : "0");
	put_disks();
	put_mtd();
	put_ubi();
	web_puts("};\n");
}
