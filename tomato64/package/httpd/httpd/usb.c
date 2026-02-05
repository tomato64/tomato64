/*
 *
 * FreshTomato Firmware
 * USB Support
 *
 * Fixes/updates (C) 2018 - 2026 pedro
 * https://freshtomato.org/
 *
 */


#include "tomato.h"

#include <sys/ioctl.h>
#include <fcntl.h>
#include <dirent.h>
#include <sys/statfs.h>
#include <mntent.h>

#ifndef BLKGETSIZE
 #define BLKGETSIZE _IO(0x12,96)
#endif
#ifndef BLKGETSIZE64
 #define BLKGETSIZE64 _IOR(0x12,114,size_t)
#endif
//#define USE_USBSTORAGE


const char proc_scsi_root[] = "/proc/scsi";
const char usb_storage[] = "usb-storage";

static uint64_t get_psize(char *dev)
{
	uint64_t bytes = 0;
	unsigned long sectors;
	int fd;

	if ((fd = open(dev, O_RDONLY)) >= 0) {
		if (ioctl(fd, BLKGETSIZE64, &bytes) < 0) {
			bytes = 0;
			/* can't get bytes, try 512 byte sectors */
			if (ioctl(fd, BLKGETSIZE, &sectors) >= 0)
				bytes = (uint64_t)sectors << 9;
		}
		close(fd);
	}

	return bytes;
}

static int is_partition_mounted(char *dev_name, int host_num, char *dsc_name, char *pt_name, uint flags)
{
	char the_label[128];
	char *type, *js;
	struct mntent *mnt;
	struct statfs sf;
	uint64_t size, fsize;
	int is_mounted = 0;

	type = find_label_or_uuid(dev_name, the_label, NULL);
	if (*the_label == 0)
		strlcpy(the_label, pt_name, sizeof(the_label));

	if (flags & EFH_PRINT) {
		if (flags & EFH_1ST_DISC) {
			/* [disc_name, [partitions array]],... */
			web_printf("]],['%s',[", dsc_name);
		}
		/* [partition_name, is_mounted, mount_point, type, opts, size, free],... */
		js = js_string(the_label);
		web_printf("%s['%s',", (flags & EFH_1ST_DISC) ? "" : ",", js ? : "");
		free(js);
	}

	if ((mnt = findmntents(dev_name, 0, 0, 0))) {
		is_mounted = 1;
		if (flags & EFH_PRINT) {
			if (statfs(mnt->mnt_dir, &sf) == 0) {
				size = (uint64_t)sf.f_bsize * sf.f_blocks;
				fsize = (uint64_t)sf.f_bsize * sf.f_bfree;
			}
			else {
				size = get_psize(dev_name);
				fsize = 0;
			}
			web_printf("1,'%s','%s','%s',%llu,%llu]", mnt->mnt_dir, mnt->mnt_type, mnt->mnt_opts, size, fsize);
		}
	}
	else if ((mnt = findmntents(dev_name, 1, 0, 0))) {
		is_mounted = 1;
		if (flags & EFH_PRINT)
			web_printf("2,'','swap','',%llu,0]", (uint64_t)atoi(mnt->mnt_type) * 1024);
	}
	else {
		if (flags & EFH_PRINT)
			web_printf("0,'','%s','',%llu,0]", type ? : "", get_psize(dev_name));
	}

	return is_mounted;
}

static int is_host_mounted(int host_no, int print_parts)
{
	if (print_parts)
		web_puts("[-1,[");

	int mounted = exec_for_host(host_no, 0x00, print_parts ? EFH_PRINT : 0, is_partition_mounted);
	
	if (print_parts)
		web_puts("]]");

	return mounted;
}

/* helper function (for asp_usbdevices) to extract value after delimiter and trim whitespace */
static int extract_token_value(const char *line, const char *delimiter, char *dest, size_t dest_size)
{
	char line_copy[128];
	char *token;

	if ((!line) || (!delimiter) || (!dest) || (dest_size == 0))
		return 0;

	/* copy line for tokenization (strtok modifies the string) */
	strlcpy(line_copy, line, sizeof(line_copy));

	/* get first token (the key) */
	token = strtok(line_copy, delimiter);
	if (!token)
		return 0;

	/* get second token (the value) */
	token = strtok(NULL, "\n");
	if (!token)
		return 0;

	/* skip leading whitespace */
	while ((*token == ' ') || (*token == '\t'))
		token++;

	/* copy to destination */
	strlcpy(dest, token, dest_size);

	return 1;
}

/*
 * The disc # doesn't correspond to the host#, since there may be more than
 * one partition on a disk.
 * Nor does either correspond to the scsi host number.
 * And if the user plugs or unplugs a usb storage device after bringing up the
 * NAS:USB support page, the numbers won't match anymore, since "part#"s
 * may be added or deleted to the /dev/discs* or /dev/scsi**.
 *
 * But since we only need to support the devices list and mount/unmount 
 * functionality on the host level, the host# should work ok. Just make sure
 * to always pass and use the _host#_, and not the disc#.
 */
void asp_usbdevices(int argc, char **argv)
{
	DIR *usb_dir = NULL;
	FILE *fp = NULL;
	struct dirent *dp;
	char g_usb_vendor[30], g_usb_product[30], line[128], path[256];
	int host_no, mounted, i = 0;
	char *js_vend, *js_prod;
#ifndef USE_USBSTORAGE
	int last_hn = -1;
	char *p, *p1;
	size_t len;
#else
	DIR *scsi_dir = NULL;
	struct dirent *scsi_dirent;
	char g_usb_serial[64];
	char *js_serial;
#endif /* !USE_USBSTORAGE */

	web_puts("\nusbdev = [");

	if (!nvram_match("usb_enable", "1")) {
		web_puts("];\n");
		return;
	}

#ifndef USE_USBSTORAGE /* get the info from the SCSI subsystem */
	snprintf(line, sizeof(line), "%s/scsi", proc_scsi_root);
	fp = fopen(line, "r");
	if (!fp) {
		web_puts("];\n");
		return;
	}

	while (fgets(line, sizeof(line), fp) != NULL) {
		/* look for "Host: scsi" line */
		p = strstr(line, "Host: scsi");
		if (!p)
			continue;

		host_no = atoi(p + 10);
		if (host_no == last_hn)
			continue;

		last_hn = host_no;

		/* read the next line with Vendor/Model info */
		if (fgets(line, sizeof(line), fp) == NULL)
			break;

		/* find Vendor field */
		p = strstr(line, "  Vendor: ");
		if (!p)
			continue;

		/* find Model field */
		p1 = strstr(line, " Model: ");
		if (!p1)
			continue;

		memset(g_usb_vendor, 0, sizeof(g_usb_vendor));
		memset(g_usb_product, 0, sizeof(g_usb_product));

		/*
		 * extract Vendor: max 8 characters after "  Vendor: "
		 * strlcpy needs size to be +1 for null terminator
		 * We want to copy UP TO 8 chars, so size = 9
		 */
		strlcpy(g_usb_vendor, p + 10, 9);
		/* trim at 8 chars in case source is longer */
		g_usb_vendor[8] = '\0';
		/* remove trailing spaces */
		len = strlen(g_usb_vendor);
		while (len > 0 && g_usb_vendor[len - 1] == ' ')
			g_usb_vendor[--len] = '\0';

		/*
		 * extract Model: max 16 characters after " Model: "
		 * We want to copy UP TO 16 chars, so size = 17
		 */
		strlcpy(g_usb_product, p1 + 8, 17);
		/* trim at 16 chars in case source is longer */
		g_usb_product[16] = '\0';
		/* remove trailing spaces */
		len = strlen(g_usb_product);
		while (len > 0 && g_usb_product[len - 1] == ' ')
			g_usb_product[--len] = '\0';

		/* escape strings for JavaScript */
		js_vend = js_string(g_usb_vendor);
		js_prod = js_string(g_usb_product);

		web_printf("%s['Storage','%d','%s','%s','', [", i ? "," : "", host_no, js_vend ? js_vend : "", js_prod ? js_prod : "");

		free(js_vend);
		free(js_prod);

		mounted = is_host_mounted(host_no, 1);
		web_printf("], %d]", mounted);
		++i;
	}

	fclose(fp);
	fp = NULL;
#else /* get the info from the usb/storage subsystem */
	scsi_dir = opendir(proc_scsi_root);
	if (!scsi_dir) {
		web_puts("];\n");
		return;
	}

	/* find all attached USB storage devices */
	while ((scsi_dirent = readdir(scsi_dir)) != NULL) {
		/* look for usb-storage directory */
		if (strncmp(usb_storage, scsi_dirent->d_name, strlen(usb_storage)) != 0)
			continue;

		snprintf(path, sizeof(path), "%s/%s", proc_scsi_root, scsi_dirent->d_name);
		usb_dir = opendir(path);
		if (!usb_dir)
			continue;

		while ((dp = readdir(usb_dir)) != NULL) {
			/* skip . and .. */
			if ((strcmp(dp->d_name, ".") == 0) || (strcmp(dp->d_name, "..") == 0))
				continue;

			/* open device info file */
			snprintf(path, sizeof(path), "%s/%s/%s", proc_scsi_root, scsi_dirent->d_name, dp->d_name);
			fp = fopen(path, "r");
			if (!fp)
				continue;

			/* initialize buffers */
			memset(g_usb_vendor, 0, sizeof(g_usb_vendor));
			memset(g_usb_product, 0, sizeof(g_usb_product));
			memset(g_usb_serial, 0, sizeof(g_usb_serial));

			/* parse device info - file presence means device is attached */
			while (fgets(line, sizeof(line), fp) != NULL) {
				if (strstr(line, "Vendor:")) {
					extract_token_value(line, ":", g_usb_vendor, sizeof(g_usb_vendor));
				}
				else if (strstr(line, "Product:")) {
					extract_token_value(line, ":", g_usb_product, sizeof(g_usb_product));
				}
				else if (strstr(line, "Serial Number:")) {
					if (extract_token_value(line, ":", g_usb_serial, sizeof(g_usb_serial))) {
						/* replace "None" with empty string */
						if (strcmp(g_usb_serial, "None") == 0)
							g_usb_serial[0] = '\0';
					}
				}
			}

			fclose(fp);
			fp = NULL;

			/* check if device has valid info */
			if ((g_usb_vendor[0]) || (g_usb_product[0])) {
				/* host no. assigned by scsi driver for this device */
				host_no = atoi(dp->d_name);

				/* escape strings for JavaScript */
				js_vend = js_string(g_usb_vendor);
				js_prod = js_string(g_usb_product);
				js_serial = js_string(g_usb_serial);

				web_printf("%s['Storage','%d','%s','%s','%s', [", i ? "," : "", host_no, js_vend ? js_vend : "", js_prod ? js_prod : "", js_serial ? js_serial : "");

				free(js_vend);
				free(js_prod);
				free(js_serial);

				mounted = is_host_mounted(host_no, 1);
				web_printf("], %d]", mounted);
				++i;
			}
		}

		closedir(usb_dir);
		usb_dir = NULL;
	}

	closedir(scsi_dir);
	scsi_dir = NULL;
#endif /* !USE_USBSTORAGE */

	/* look for printers */
	usb_dir = opendir("/proc/usblp");
	if (!usb_dir) {
		web_puts("];\n");
		return;
	}

	i = 0;
	while ((dp = readdir(usb_dir)) != NULL) {
		/* skip . and .. */
		if ((strcmp(dp->d_name, ".") == 0) || (strcmp(dp->d_name, "..") == 0))
			continue;

		snprintf(path, sizeof(path), "/proc/usblp/%s", dp->d_name);
		fp = fopen(path, "r");
		if (!fp)
			continue;

		/* initialize buffers */
		memset(g_usb_vendor, 0, sizeof(g_usb_vendor));
		memset(g_usb_product, 0, sizeof(g_usb_product));

		/* parse printer info */
		while (fgets(line, sizeof(line), fp) != NULL) {
			if (strstr(line, "Manufacturer="))
				extract_token_value(line, "=", g_usb_vendor, sizeof(g_usb_vendor));
			else if (strstr(line, "Model="))
				extract_token_value(line, "=", g_usb_product, sizeof(g_usb_product));
		}

		fclose(fp);
		fp = NULL;

		/* output printer info if we have any data */
		if ((g_usb_vendor[0]) || (g_usb_product[0])) {
			js_vend = js_string(g_usb_vendor);
			js_prod = js_string(g_usb_product);

			web_printf("%s['Printer','%s','%s','%s','']", i ? "," : "", dp->d_name, js_vend ? js_vend : "", js_prod ? js_prod : "");

			free(js_vend);
			free(js_prod);
			++i;
		}
	}

	closedir(usb_dir);
	usb_dir = NULL;

	web_puts("];\n");
}

void wo_usbcommand(char *url)
{
	char *p;
	unsigned int add = 0;

	web_puts("\nusb = [\n");
	if ((p = webcgi_get("remove")) != NULL)
		add = 0;
	else if ((p = webcgi_get("mount")) != NULL)
		add = 1;

	if (p) {
		add_remove_usbhost(p, add);
		web_printf("%d", is_host_mounted(atoi(p), 0));
	}
	web_puts("];\n");
}
