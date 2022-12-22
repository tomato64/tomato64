/*
 *
 * USB Support
 *
 */


#include "rc.h"

#include <sys/types.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <arpa/inet.h>
#include <time.h>
#include <sys/time.h>
#include <errno.h>

#include <sys/mount.h>
#include <mntent.h>
#include <dirent.h>
#include <sys/file.h>
#include <sys/swap.h>

#include <wlutils.h>

/* needed by logmsg() */
#define LOGMSG_DISABLE	0
#define LOGMSG_NVDEBUG	"usb_debug"

#define MOUNT_VAL_FAIL	0
#define MOUNT_VAL_RONLY	1
#define MOUNT_VAL_RW	2
#define MOUNT_VAL_EXIST	3

#define USBCORE_MOD	"usbcore"
#ifdef TCONFIG_BCMARM
#define USB30_MOD	"xhci-hcd"
#endif
#define USB20_MOD	"ehci-hcd"
#define USBSTORAGE_MOD	"usb-storage"
#define SCSI_MOD	"scsi_mod"
#define SD_MOD		"sd_mod"
#define USBOHCI_MOD	"ohci-hcd"
#define USBUHCI_MOD	"uhci-hcd"
#define USBPRINTER_MOD	"usblp"
#define SCSI_WAIT_MOD	"scsi_wait_scan"
#define USBFS		"usbfs"

#ifndef MNT_DETACH
#define MNT_DETACH	0x00000002 /* from linux/fs.h - just detach from the tree */
#endif


int umount_mountpoint(struct mntent *mnt, uint flags);
int uswap_mountpoint(struct mntent *mnt, uint flags);

/* Adjust bdflush parameters.
 * Do this here, because Tomato doesn't have the sysctl command.
 * With these values, a disk block should be written to disk within 2 seconds.
 */
void tune_bdflush(void)
{
	f_write_string("/proc/sys/vm/dirty_expire_centisecs", "200", 0, 0);
	f_write_string("/proc/sys/vm/dirty_writeback_centisecs", "200", 0, 0);
}

static int p9100d_sig(int sig)
{
	const char p910pid[] = "/var/run/p9100d.pid";
	char s[32];
	int pid;

	if (f_read_string(p910pid, s, sizeof(s)) > 0) {
		if ((pid = atoi(s)) > 1) {
			if (kill(pid, sig) == 0) {
				if (sig == SIGTERM) {
					sleep(1);
					unlink(p910pid);
				}
				return 0;
			}
		}
	}
	return -1;
}

void start_usb(void)
{
	char param[32];
	int i = 255;

#ifdef TCONFIG_BCMARM
#ifdef TCONFIG_BCMSMP
	int fd;
#endif
#if defined(TCONFIG_BCM714) || defined(TCONFIG_BCM7)
	/* get router model */
	int model = get_model();
	static int usb_reset_once = 0;

	if (!usb_reset_once) {
		switch(model) {
#ifdef TCONFIG_BCM7
#ifdef TCONFIG_AC3200
			case MODEL_RTAC3200:
#endif /* TCONFIG_AC3200 */
#endif
#ifdef TCONFIG_BCM714
			case MODEL_RTAC3100:
			case MODEL_RTAC88U:
#ifdef TCONFIG_AC5300
			case MODEL_RTAC5300:
#endif
#endif /* TCONFIG_BCM714 */
				set_gpio(GPIO_09, T_LOW); /* disable USB power */
				usleep(25 * 1000); /* wait 25 ms */
				set_gpio(GPIO_09, T_HIGH); /* enable USB power */
				usleep(25 * 1000); /* wait 25 ms (again) */
				usb_reset_once = 1;
				logmsg(LOG_INFO, "%s: FreshTomato - reset USB Power Supply (done)", nvram_safe_get("t_model_name"));
				break;
#ifdef TCONFIG_BCM7
#ifdef TCONFIG_AC3200
			case MODEL_R8000:
				set_gpio(GPIO_00, T_LOW); /* disable USB power */
				usleep(25 * 1000); /* wait 25 ms */
				set_gpio(GPIO_00, T_HIGH); /* enable USB power */
				usleep(25 * 1000); /* wait 25 ms (again) */
				usb_reset_once = 1;
				logmsg(LOG_INFO, "%s: FreshTomato - reset USB Power Supply (done)", nvram_safe_get("t_model_name"));
				break;
#endif /* TCONFIG_AC3200 */
#endif
			default:
				/* nothing to do right now! */
				break;
		}
	}
#endif /* TCONFIG_BCM714 OR TCONFIG_BCM7 */
	/* nothing to do right now for ARM! */
	/* enable USB2/3 power by default - see file bcm5301x_pcie.c */

#elif CONFIG_BCMWL6 /* TCONFIG_BCMARM */
	if (nvram_match("boardtype", "0x0617") &&  nvram_match("boardrev", "0x1102")) /* DIR-865L enable USB */
		xstart("gpio", "enable", "7");
#elif TCONFIG_BLINK /* TCONFIG_BCMARM */
	if (nvram_match("boardtype", "0x052b")) /* Netgear WNR3500L v2 - initialize USB port */
		xstart("gpio", "enable", "20");
#endif /* TCONFIG_BCMARM */

	logmsg(LOG_DEBUG, "*** %s", __FUNCTION__);
	tune_bdflush();

	/* load modules if USB is enabled */
	if (nvram_get_int("usb_enable")) {
#ifdef TCONFIG_USBAP
		eval("insmod", "usbcore");
#else
		modprobe(USBCORE_MOD);
#endif /* TCONFIG_USBAP */

		/* mount usb device filesystem */
		mount(USBFS, "/proc/bus/usb", USBFS, MS_MGC_VAL, NULL);

		/* check USB LED */
		i = do_led(LED_USB, LED_PROBE);
		if (i != 255) {

#ifdef TCONFIG_BCMARM
			do_led(LED_USB, LED_OFF); /* turn off USB LED */
		}

		i = 255; /* reset to 255 */
		/* check USB3 LED */
		i = do_led(LED_USB3, LED_PROBE);
		if (i != 255) {
			do_led(LED_USB3, LED_OFF); /* turn off USB3 LED */

#elif defined(CONFIG_BCMWL6) || defined(TCONFIG_BLINK) /* TCONFIG_BCMARM */
			/* Remove legacy approach in the code here - rather, use do_led() function, which is designed to do this
			 * The reason for changing this... some HW (like Netgear WNDR4000) don't work with direct GPIO write -> use do_led()!
			 */
			do_led(LED_USB, LED_OFF); /* turn off USB LED */
#else
			modprobe("ledtrig-usbdev");
			modprobe("leds-usb");
			sprintf(param, "%d", i);
			f_write_string("/proc/leds-usb/gpio_pin", param, 0, 0);
#endif /* TCONFIG_BCMARM */
		}

#ifdef TCONFIG_USBAP
		char insmod_arg[128];
		int j = 0, maxwl_eth = 0, maxunit = -1;
		char ifname[16] = {0};
		int unit = -1;
		const int wl_wait = 3; /* max wait time for wl_high to up */

		/* From Asus QTD cache params */
		char arg1[20] = {0};
		char arg2[20] = {0};
		char arg3[20] = {0};
		char arg4[20] = {0};
		char arg5[20] = {0};
		char arg6[20] = {0};
		char arg7[20] = {0};

		/* Save QTD cache params in nvram */
		sprintf(arg1, "log2_irq_thresh=%d", nvram_get_int("ehciirqt"));
		sprintf(arg2, "qtdc_pid=%d", nvram_get_int("qtdc_pid"));
		sprintf(arg3, "qtdc_vid=%d", nvram_get_int("qtdc_vid"));
		sprintf(arg4, "qtdc0_ep=%d", nvram_get_int("qtdc0_ep"));
		sprintf(arg5, "qtdc0_sz=%d", nvram_get_int("qtdc0_sz"));
		sprintf(arg6, "qtdc1_ep=%d", nvram_get_int("qtdc1_ep"));
		sprintf(arg7, "qtdc1_sz=%d", nvram_get_int("qtdc1_sz"));

		eval("insmod", "ehci-hcd", arg1, arg2, arg3, arg4, arg5, arg6, arg7);

		/* Search for existing PCI wl devices and the max unit number used.
		 * Note that PCI driver has to be loaded before USB hotplug event.
		 * see load_wl() at init.c for USBAP
		 */
		#define DEV_NUMIFS 8
		for (j = 1; j <= DEV_NUMIFS; j++) {
			sprintf(ifname, "eth%d", j);
			if (!wl_probe(ifname)) {
				if (!wl_ioctl(ifname, WLC_GET_INSTANCE, &unit, sizeof(unit))) {
					maxwl_eth = j;
					maxunit = (unit > maxunit) ? unit : maxunit;
				}
			}
		}

		/* Set instance base (starting unit number) for USB device */
		sprintf(insmod_arg, "instance_base=%d", maxunit + 1);
		eval("insmod", "wl_high", insmod_arg);

		/* Hold until the USB/HSIC interface is up (up to wl_wait sec) */
		sprintf(ifname, "eth%d", maxwl_eth + 1);
		j = wl_wait;
		while (wl_probe(ifname) && j--) {
			sleep(1);
		}
		if (!wl_ioctl(ifname, WLC_GET_INSTANCE, &unit, sizeof(unit)))
			logmsg(LOG_INFO, "wl%d is up in %d sec", unit, wl_wait - j);
		else
			logmsg(LOG_WARNING, "wl%d not up in %d sec", unit, wl_wait);

		modprobe(USBCORE_MOD);
#endif /* TCONFIG_USBAP */

		if (nvram_get_int("usb_storage")) {
			/* insert scsi and storage modules before usb drivers */
			modprobe(SCSI_MOD);
			modprobe(SCSI_WAIT_MOD);
			modprobe(SD_MOD);
			modprobe(USBSTORAGE_MOD);

#ifdef TCONFIG_BCMARM
			if (nvram_get_int("usb_fs_ext4")) {
#else
			if (nvram_get_int("usb_fs_ext3")) {
#endif /* TCONFIG_BCMARM */
				modprobe("mbcache"); /* used by ext2/3/(4) */
#ifdef TCONFIG_BCMARM
				modprobe("jbd2");
				modprobe("crc16");
				modprobe("ext4");
#else
				modprobe("jbd");
				modprobe("ext3");
				modprobe("ext2");
#endif /* TCONFIG_BCMARM */
			}

			if (nvram_get_int("usb_fs_fat")) {
				modprobe("fat");
				modprobe("vfat");
			}

#ifdef TCONFIG_BCMARM
			if (nvram_get_int("usb_fs_exfat"))
				modprobe("exfat");
#endif

#if defined(TCONFIG_UFSDA) || defined(TCONFIG_UFSDN)
			if (nvram_get_int("usb_fs_ntfs") && nvram_match("usb_ntfs_driver", "paragon"))
				modprobe("ufsd");
#elif TCONFIG_UFSD
			if (nvram_get_int("usb_fs_ntfs"))
				modprobe("ufsd");
#endif

#ifdef TCONFIG_TUXERA
			if (nvram_get_int("usb_fs_ntfs") && nvram_match("usb_ntfs_driver", "tuxera"))
				modprobe("tntfs");
#endif

#ifdef TCONFIG_HFS
			if (nvram_get_int("usb_fs_hfs")
#ifdef TCONFIG_BCMARM
			    && nvram_match("usb_hfs_driver", "kernel")
#endif
			) {
				modprobe("hfs");
				modprobe("hfsplus");
			}
#endif /* TCONFIG_HFS */

#ifdef TCONFIG_TUXERA_HFS
			if (nvram_get_int("usb_fs_hfs") && nvram_match("usb_hfs_driver", "tuxera"))
				modprobe("thfsplus");
#endif

#ifdef TCONFIG_ZFS
			if (nvram_get_int("usb_fs_zfs"))
				modprobe("zfs");
#endif

#ifdef TCONFIG_MICROSD
			if (nvram_get_int("usb_mmc") == 1) {
				/* insert SD/MMC modules if present */
				modprobe("mmc_core");
				modprobe("mmc_block");
				modprobe("sdhci");
			}
#endif

		} /* if (nvram_get_int("usb_storage")) */

#ifdef TCONFIG_BCMARM
		if (nvram_get_int("usb_usb3") == 1) {
			modprobe(USB30_MOD);
#ifdef TCONFIG_BCMSMP
			sleep(1);
			if ((fd = open("/proc/irq/163/smp_affinity", O_RDWR)) >= 0) {
				close(fd);
				f_write_string("/proc/irq/112/smp_affinity", TOMATO_CPU1, 0, 0); /* xhci_hcd --> CPU 1 */
			}
#endif
		}
#endif /* TCONFIG_BCMARM */

		/* if enabled, force USB2 before USB1.1 */
		if (nvram_get_int("usb_usb2") == 1) {
			i = nvram_get_int("usb_irq_thresh");
			if ((i < 0) || (i > 6))
				i = 0;
			sprintf(param, "log2_irq_thresh=%d", i);
			modprobe(USB20_MOD, param);
#if defined(TCONFIG_BCMARM) && defined(TCONFIG_BCMSMP)
			sleep(1);
			if ((fd = open("/proc/irq/163/smp_affinity", O_RDWR)) >= 0) {
				close(fd);
				f_write_string("/proc/irq/111/smp_affinity", TOMATO_CPU1, 0, 0); /* ehci_hcd --> CPU 1 */
			}
#endif
		}

		if (nvram_get_int("usb_uhci") == 1)
			modprobe(USBUHCI_MOD);

		if (nvram_get_int("usb_ohci") == 1)
			modprobe(USBOHCI_MOD);

		if (nvram_get_int("usb_printer")) {
			symlink("/dev/usb", "/dev/printers");
			modprobe(USBPRINTER_MOD);

			/* start printer server only if not already running */
			if (p9100d_sig(0) != 0) {
				eval("p910nd",
				nvram_get_int("usb_printer_bidirect") ? "-b" : "", /* bidirectional */
				              "-f", "/dev/usb/lp0", /* device */
				              "0" /* listen port */
				);
			}
		}
		if (nvram_get_int("idle_enable") == 1)
			xstart( "sd-idle" );

#ifdef TCONFIG_UPS
		if (nvram_get_int("usb_apcupsd") == 1) {
			modprobe("input-core");
			modprobe("hid");
			modprobe("usbhid");
			start_ups();
		}
#endif

#ifdef TCONFIG_USBAP
		/* enable eth2 after detect new iface by wl_high module */
		sleep(5);
		xstart("service", "wireless", "restart");
#endif

/* If we want restore backup of webmon from USB device,
 * we have to wait for mount USB devices by hotplug
 * and then reboot firewall service (webmon iptables rules) one more time.
 */
		if (nvram_match("log_wm", "1") && nvram_match("webmon_bkp", "1"))
			xstart("service", "firewall", "restart");

	}
}

void remove_usb_modem_modules(void)
{
	/* 3G */
	modprobe_r("sierra");
	modprobe_r("option");
	modprobe_r("cdc-acm");

	/* 4G */
	modprobe_r("cdc_mbim");
	modprobe_r("qmi_wwan");
	modprobe_r("cdc_wdm");
#ifdef TCONFIG_BCMARM
	modprobe_r("huawei_cdc_ncm");
#else
	modprobe_r("huawei_ether");
#endif
	modprobe_r("cdc_ncm");
	modprobe_r("rndis_host");
	modprobe_r("cdc_ether");

	modprobe_r("usbnet");
	//modprobe_r("usbserial"); /* may crash */
}

void remove_usb_ups_module(void)
{
#ifdef TCONFIG_UPS
	modprobe_r("usbhid");
	modprobe_r("hid");
	modprobe_r("input-core");
#endif
}

void remove_usb_prn_module(void)
{
	/* only find and kill the printer server we started (port 0) */
	p9100d_sig(SIGTERM);
	modprobe_r(USBPRINTER_MOD);
}

void remove_usb_storage_module(void)
{
#ifdef TCONFIG_BCMARM
	modprobe_r("ext4");
	modprobe_r("crc16");
	modprobe_r("jbd2");
#else
	modprobe_r("ext2");
	modprobe_r("ext3");
	modprobe_r("jbd");
#endif
	modprobe_r("mbcache");
	modprobe_r("vfat");
	modprobe_r("fat");
	modprobe_r("exfat");
#if defined(TCONFIG_UFSDA) || defined(TCONFIG_UFSDN)
	modprobe_r("ufsd");
#endif
#ifdef TCONFIG_TUXERA
	modprobe_r("tntfs");
#endif
#ifdef TCONFIG_HFS
	modprobe_r("hfs");
	modprobe_r("hfsplus");
#endif
#ifdef TCONFIG_TUXERA_HFS
	modprobe_r("thfsplus");
#endif
#ifdef TCONFIG_ZFS
	modprobe_r("zfs");
#endif

	modprobe_r("fuse");
	sleep(1);

#ifdef TCONFIG_SAMBASRV
	modprobe_r("nls_cp437");
	modprobe_r("nls_cp850");
	modprobe_r("nls_cp852");
	modprobe_r("nls_cp866");
	modprobe_r("nls_cp932");
	modprobe_r("nls_cp936");
	modprobe_r("nls_cp949");
	modprobe_r("nls_cp950");
#endif
	modprobe_r(SD_MOD);
	modprobe_r(USBSTORAGE_MOD);
	modprobe_r(SCSI_WAIT_MOD);
	modprobe_r(SCSI_MOD);
#ifdef TCONFIG_MICROSD
	if (nvram_get_int("usb_mmc") != 1) {
		modprobe_r("sdhci");
		modprobe_r("mmc_block");
		modprobe_r("mmc_core");
	}
#endif
}

void remove_usb_host_module(void)
{
	modprobe_r(USBOHCI_MOD);
	modprobe_r(USBUHCI_MOD);
	modprobe_r(USB20_MOD);
#ifdef TCONFIG_BCMARM
	modprobe_r(USB30_MOD);
#endif
	modprobe_r(USBCORE_MOD);
}

void remove_usb_module(void)
{
	remove_usb_modem_modules();
	remove_usb_ups_module();
	remove_usb_prn_module();
	remove_usb_storage_module();
	remove_usb_host_module();
}

void stop_usb(void)
{
	int disabled = !nvram_get_int("usb_enable");
#ifdef TCONFIG_BCMARM
	int i = 255;
#endif

#ifdef TCONFIG_UPS
	stop_ups();
#endif
	remove_usb_ups_module();
	remove_usb_modem_modules();
	remove_usb_prn_module();

	if (nvram_get_int("idle_enable") == 0)
		killall("sd-idle", SIGTERM);

	/* only stop storage services if disabled */
	if ((disabled) || (!nvram_get_int("usb_storage"))) {
		/* Unmount all partitions */
		remove_storage_main(0);

		/* Stop storage services */
		remove_usb_storage_module();
	}

	if (disabled || nvram_get_int("usb_ohci") != 1)
		modprobe_r(USBOHCI_MOD);
	if (disabled || nvram_get_int("usb_uhci") != 1)
		modprobe_r(USBUHCI_MOD);
	if (disabled || nvram_get_int("usb_usb2") != 1)
		modprobe_r(USB20_MOD);

#ifdef TCONFIG_BCMARM
	if (disabled || nvram_get_int("usb_xhci") != 1)
		modprobe_r(USB30_MOD);

	/* check USB LED */
	i = do_led(LED_USB, LED_PROBE);
	if (i != 255)
		do_led(LED_USB, LED_OFF); /* turn off USB LED */

	i = 255; /* reset to 255 */
	/* check USB3 LED */
	i = do_led(LED_USB3, LED_PROBE);
	if (i != 255)
		do_led(LED_USB3, LED_OFF); /* turn off USB3 LED */

#elif !defined(CONFIG_BCMWL6) && !defined (TCONFIG_BLINK) /* TCONFIG_BCMARM */
	modprobe_r("leds-usb");
	modprobe_r("ledtrig-usbdev");
#endif /* TCONFIG_BCMARM */

#ifndef TCONFIG_BCMARM
	led(LED_USB, LED_OFF);
#endif

	/* only unload core modules if usb is disabled */
	if (disabled) {
		umount("/proc/bus/usb"); /* unmount usb device filesystem */
		modprobe_r(USBOHCI_MOD);
		modprobe_r(USBUHCI_MOD);
		modprobe_r(USB20_MOD);
#ifdef TCONFIG_BCMARM
		modprobe_r(USB30_MOD);
#endif
		modprobe_r(USBCORE_MOD);
	}

#ifdef TCONFIG_BCMARM
	/* nothing to do right now for ARM! */
	/* enable USB2/3 power by default - see file bcm5301x_pcie.c */
	/* do not disable USB2/3 power with stop_usb() */
#elif CONFIG_BCMWL6 /* TCONFIG_BCMARM */
	if (nvram_match("boardtype", "0x0617") &&  nvram_match("boardrev", "0x1102")) /* DIR-865L disable USB */
		xstart("gpio", "disable", "7");
#elif TCONFIG_BLINK /* TCONFIG_BCMARM */
	if (nvram_match("boardtype", "0x052b")) /* Netgear WNR3500L v2 - disable USB port */
		xstart("gpio", "disable", "20");
#endif /* TCONFIG_BCMARM */
}

int mount_r(char *mnt_dev, char *mnt_dir, char *type)
{
	struct mntent *mnt;
	int ret;
	char options[140];
	char flagfn[128];
	int dir_made;

	if ((mnt = findmntents(mnt_dev, 0, NULL, 0))) {
		logmsg(LOG_INFO, "USB partition at %s already mounted on %s", mnt_dev, mnt->mnt_dir);
		return MOUNT_VAL_EXIST;
	}

	options[0] = 0;

	if (type) {
		unsigned long flags = MS_NOATIME | MS_NODEV;

		if ((strcmp(type, "swap") == 0) || (strcmp(type, "mbr") == 0)) {
			/* not a mountable partition */
			flags = 0;
		}
		else if ((strcmp(type, "ext2") == 0) || (strcmp(type, "ext3") == 0)
#ifdef TCONFIG_BCMARM
		         || (strcmp(type, "ext4") == 0)
#endif
		) {
			if (nvram_invmatch("usb_ext_opt", ""))
				sprintf(options, nvram_safe_get("usb_ext_opt"));
		}
		else if (strcmp(type, "vfat") == 0) {
			if (nvram_invmatch("smbd_cset", ""))
				sprintf(options, "iocharset=%s%s", isdigit(nvram_get("smbd_cset")[0]) ? "cp" : "", nvram_get("smbd_cset"));

			if (nvram_invmatch("smbd_cpage", "")) {
				char *cp = nvram_safe_get("smbd_cpage");
				sprintf(options + strlen(options), ",codepage=%s" + (options[0] ? 0 : 1), cp);
				sprintf(flagfn, "nls_cp%s", cp);

				cp = nvram_get("smbd_nlsmod");
				if ((cp) && (*cp != 0) && (strcmp(cp, flagfn) != 0))
					modprobe_r(cp);

				modprobe(flagfn);
				nvram_set("smbd_nlsmod", flagfn);
			}
			sprintf(options + strlen(options), ",shortname=winnt" + (options[0] ? 0 : 1));
			sprintf(options + strlen(options), ",flush" + (options[0] ? 0 : 1));

			if (nvram_invmatch("usb_fat_opt", ""))
				sprintf(options + strlen(options), "%s%s", options[0] ? "," : "", nvram_safe_get("usb_fat_opt"));
		}
		else if (strncmp(type, "ntfs", 4) == 0) {
			if (nvram_invmatch("smbd_cset", ""))
				sprintf(options, "iocharset=%s%s", isdigit(nvram_get("smbd_cset")[0]) ? "cp" : "", nvram_get("smbd_cset"));

			if (nvram_invmatch("usb_ntfs_opt", ""))
				sprintf(options + strlen(options), "%s%s", options[0] ? "," : "", nvram_safe_get("usb_ntfs_opt"));
		}

#if defined(TCONFIG_BCMARM) && defined(TCONFIG_HFS)
		else if (strncmp(type, "hfs", 3) == 0) {
			if (nvram_get_int("usb_fs_hfs")) {
				if (nvram_match("usb_hfs_driver", "kernel")) {
					sprintf(options, "rw,noatime,nodev");

					if (strncmp(type, "hfsplus", 7) == 0)
						sprintf(options + strlen(options), ",force" + (options[0] ? 0 : 1));
				}
#ifdef TCONFIG_TUXERA_HFS
				else if (nvram_match("usb_hfs_driver", "tuxera")) {
					/* override fs fype */
					type = "thfsplus";
				}
#endif
				if (nvram_invmatch("usb_hfs_opt", ""))
					sprintf(options + strlen(options), "%s%s", options[0] ? "," : "", nvram_safe_get("usb_hfs_opt"));
			}
			else /* HFS support disabled by user, don't try to mount */
				flags = 0;
		}
#endif /* TCONFIG_BCMARM && TCONFIG_HFS */

		if (flags) {
			if ((dir_made = mkdir_if_none(mnt_dir))) {
				/* Create the flag file for remove the directory on dismount. */
				sprintf(flagfn, "%s/.autocreated-dir", mnt_dir);
				f_write(flagfn, NULL, 0, 0, 0);
			}

			/* mount at last */
			ret = mount(mnt_dev, mnt_dir, type, flags, options[0] ? options : "");
			logmsg(LOG_DEBUG, "*** %s: # mount # type: %s, options: %s, mnt_dev: %s, mnt_dir: %s; return code: %d", __FUNCTION__, type, strlen(options) ? options : "none", mnt_dev, mnt_dir, ret);

#ifdef TCONFIG_NTFS
			if (ret != 0 && strncmp(type, "ntfs", 4) == 0) {
				sprintf(options + strlen(options), ",noatime,nodev" + (options[0] ? 0 : 1));
				if (nvram_get_int("usb_fs_ntfs")) {
#ifdef TCONFIG_BCMARM
					if (nvram_match("usb_ntfs_driver", "ntfs3g"))
						ret = eval("ntfs-3g", "-o", options, mnt_dev, mnt_dir);
#if defined(TCONFIG_UFSDA) || defined(TCONFIG_UFSDN)
					else if (nvram_match("usb_ntfs_driver", "paragon"))
						ret = eval("mount", "-t", "ufsd", "-o", options, "-o", "force", mnt_dev, mnt_dir);
#endif
#ifdef TCONFIG_TUXERA
					else if (nvram_match("usb_ntfs_driver", "tuxera"))
						ret = eval("mount", "-t", "tntfs", "-o", options, mnt_dev, mnt_dir);
#endif
#else /* TCONFIG_BCMARM */
#ifdef TCONFIG_UFSD
					ret = eval("mount", "-t", "ufsd", "-o", options, "-o", "force", mnt_dev, mnt_dir);
#else
					ret = eval("ntfs-3g", "-o", options, mnt_dev, mnt_dir);
#endif
#endif /* TCONFIG_BCMARM */
				}
			}
#endif /* TCONFIG_NTFS */

#ifdef TCONFIG_HFS
			/* try rw mount for kernel HFS/HFS+ driver (guess fs) */
			if (ret != 0 && (strncmp(type, "hfs", 3) == 0)) {
#ifdef TCONFIG_BCMARM
				eval("fsck.hfsplus", "-f", mnt_dev);

				ret = eval("mount", "-o", options, mnt_dev, mnt_dir);
				if (ret == 0)
					logmsg(LOG_INFO, "USB: %s: attempt to mount rw after unclean unmounting succeeded!", type);

				logmsg(LOG_DEBUG, "*** %s: mount cmd: mount -o %s %s %s, return: %d", __FUNCTION__, options, mnt_dev, mnt_dir, ret);
#else /* TCONFIG_BCMARM */
				ret = eval("mount", "-o", "noatime,nodev", mnt_dev, mnt_dir);
#endif /* TCONFIG_BCMARM */
			}
#endif /* TCONFIG_HFS */

			if (ret != 0) /* give it another try - guess fs */
				ret = eval("mount", "-o", "noatime,nodev", mnt_dev, mnt_dir);

			if (ret == 0) {
				logmsg(LOG_INFO, "USB %s%s fs at %s mounted on %s", type, (flags & MS_RDONLY) ? " (ro)" : "", mnt_dev, mnt_dir);
				return (flags & MS_RDONLY) ? MOUNT_VAL_RONLY : MOUNT_VAL_RW;
			}
			else
				logmsg(LOG_INFO, "USB %s%s fs at %s failed to mount on %s", type, (flags & MS_RDONLY) ? " (ro)" : "", mnt_dev, mnt_dir);

			if (dir_made) {
				unlink(flagfn);
				rmdir(mnt_dir);
			}
		}
	}

	return MOUNT_VAL_FAIL;
}

struct mntent *mount_fstab(char *dev_name, char *type, char *label, char *uuid)
{
	struct mntent *mnt = NULL;
	char spec[PATH_MAX+1];

	if (label && *label) {
		sprintf(spec, "LABEL=%s", label);
		if (eval("mount", spec) == 0)
			mnt = findmntents(dev_name, 0, NULL, 0);
	}

	if (!mnt && uuid && *uuid) {
		sprintf(spec, "UUID=%s", uuid);
		if (eval("mount", spec) == 0)
			mnt = findmntents(dev_name, 0, NULL, 0);
	}

	if (!mnt) {
		if (eval("mount", dev_name) == 0)
			mnt = findmntents(dev_name, 0, NULL, 0);
	}

	if (!mnt) {
		/* Still did not find what we are looking for, try absolute path */
		if (realpath(dev_name, spec)) {
			if (eval("mount", spec) == 0)
				mnt = findmntents(dev_name, 0, NULL, 0);
		}
	}

	if (mnt)
		logmsg(LOG_INFO, "USB %s fs at %s mounted on %s", type, dev_name, mnt->mnt_dir);

	return (mnt);
}

/* Check if the UFD is still connected because the links created in /dev/discs
 * are not removed when the UFD is  unplugged.
 * The file to read is: /proc/scsi/usb-storage-#/#, where # is the host no.
 * We are looking for "Attached: Yes".
 */
static int usb_ufd_connected(int host_no)
{
	char proc_file[128];
	FILE *fp;

	sprintf(proc_file, "%s/%s-%d/%d", PROC_SCSI_ROOT, USB_STORAGE, host_no, host_no);
	fp = fopen(proc_file, "r");

	if (!fp) {
		/* try the way it's implemented in newer kernels: /proc/scsi/usb-storage/[host] */
		sprintf(proc_file, "%s/%s/%d", PROC_SCSI_ROOT, USB_STORAGE, host_no);
		fp = fopen(proc_file, "r");
	}

	if (fp) {
		fclose(fp);
		return 1;
	}

	return 0;
}

/* Unmount this partition from all its mountpoints.  Note that it may
 * actually be mounted several times, either with different names or
 * with "-o bind" flag.
 * If the special flagfile is now revealed, delete it and [attempt to] delete
 * the directory.
 */
int umount_partition(char *dev_name, int host_num, char *dsc_name, char *pt_name, uint flags)
{
	sync(); /* This won't matter if the device is unplugged, though. */

	if (flags & EFH_HUNKNOWN) {
		/* EFH_HUNKNOWN flag is passed if the host was unknown.
		 * Only unmount disconnected drives in this case.
		 */
		if (usb_ufd_connected(host_num))
			return 0;
	}

	/* Find all the active swaps that are on this device and stop them. */
	findmntents(dev_name, 1, uswap_mountpoint, flags);

	/* Find all the mountpoints that are for this device and unmount them. */
	findmntents(dev_name, 0, umount_mountpoint, flags);

	return 0;
}

int uswap_mountpoint(struct mntent *mnt, uint flags)
{
	swapoff(mnt->mnt_fsname);

	return 0;
}

int umount_mountpoint(struct mntent *mnt, uint flags)
{
	int ret = 1, count;
	char flagfn[128];

	sprintf(flagfn, "%s/.autocreated-dir", mnt->mnt_dir);

	/* Run user pre-unmount scripts if any. It might be too late if
	 * the drive has been disconnected, but we'll try it anyway.
	 */
	if (nvram_get_int("usb_automount"))
		run_nvscript("script_usbumount", mnt->mnt_dir, 3);

	/* Run *.autostop scripts located in the root of the partition being unmounted if any. */
	run_userfile(mnt->mnt_dir, ".autostop", mnt->mnt_dir, 5);
	run_nvscript("script_autostop", mnt->mnt_dir, 5);

	count = 0;
	while ((ret = umount(mnt->mnt_dir)) && (count < 2)) {
		count++;
		/* If we could not unmount the drive on the 1st try,
		 * kill all NAS applications so they are not keeping the device busy -
		 * unless it's an unmount request from the Web GUI.
		 */
		if ((count == 1) && ((flags & EFH_USER) == 0))
			restart_nas_services(1, 0);

		sleep(1);
	}

	if (ret == 0)
		logmsg(LOG_INFO, "USB partition unmounted from %s", mnt->mnt_dir);

	if (ret && ((flags & EFH_SHUTDN) != 0)) {
		/* If system is stopping (not restarting), and we couldn't unmount the
		 * partition, try to remount it as read-only. Ignore the return code -
		 * we can still try to do a lazy unmount.
		 */
		eval("mount", "-o", "remount,ro", mnt->mnt_dir);
	}

	if (ret && ((flags & EFH_USER) == 0)) {
		/* Make one more try to do a lazy unmount unless it's an unmount
		 * request from the Web GUI.
		 * MNT_DETACH will expose the underlying mountpoint directory to all
		 * except whatever has cd'ed to the mountpoint (thereby making it busy).
		 * So the unmount can't actually fail. It disappears from the ken of
		 * everyone else immediately, and from the ken of whomever is keeping it
		 * busy when they move away from it. And then it disappears for real.
		 */
		ret = umount2(mnt->mnt_dir, MNT_DETACH);
		logmsg(LOG_INFO, "USB partition busy - will unmount ASAP from %s", mnt->mnt_dir);
	}

	if (ret == 0) {
		if ((unlink(flagfn) == 0)) /* Only delete the directory if it was auto-created */
			rmdir(mnt->mnt_dir);
	}

	return (ret == 0);
}

/* Mount this partition on this disc.
 * If the device is already mounted on any mountpoint, don't mount it again.
 * If this is a swap partition, try swapon -a.
 * If this is a regular partition, try mount -a.
 *
 * Before we mount any partitions:
 * If the type is swap and /etc/fstab exists, do "swapon -a"
 * If /etc/fstab exists, try mounting using fstab.
 * We delay invoking mount because mount will probe all the partitions
 * to read the labels, and we don't want it to do that early on.
 * We don't invoke swapon until we actually find a swap partition.
 *
 * If the mount succeeds, execute the *.autorun scripts in the top
 * directory of the newly mounted partition.
 * Returns NZ for success, 0 if we did not mount anything.
 */
int mount_partition(char *dev_name, int host_num, char *dsc_name, char *pt_name, uint flags)
{
	char the_label[128], mountpoint[128], uuid[40];
	int ret;
	char *type, *p;
	static char *swp_argv[] = { "swapon", "-a", NULL };
	struct mntent *mnt;

	if ((type = find_label_or_uuid(dev_name, the_label, uuid)) == NULL)
		return 0;

	if (f_exists("/etc/fstab")) {
		if (strcmp(type, "swap") == 0) {
			_eval(swp_argv, NULL, 0, NULL);
			return 0;
		}

		if (mount_r(dev_name, NULL, NULL) == MOUNT_VAL_EXIST)
			return 0;

		if ((mnt = mount_fstab(dev_name, type, the_label, uuid))) {
			strcpy(mountpoint, mnt->mnt_dir);
			ret = MOUNT_VAL_RW;
			goto done;
		}
	}

	if (*the_label != 0) {
		for (p = the_label; *p; p++) {
			if (!isalnum(*p) && !strchr("+-&.@", *p))
				*p = '_';
		}
		sprintf(mountpoint, "%s/%s", MOUNT_ROOT, the_label);
		if ((ret = mount_r(dev_name, mountpoint, type)))
			goto done;
	}

	/* Can't mount to /mnt/LABEL, so try mounting to /mnt/discDN_PN */
	sprintf(mountpoint, "%s/%s", MOUNT_ROOT, pt_name);
	ret = mount_r(dev_name, mountpoint, type);

done:
	if (ret == MOUNT_VAL_RONLY || ret == MOUNT_VAL_RW) {
		/* Run user *.autorun and post-mount scripts if any. */
		run_userfile(mountpoint, ".autorun", mountpoint, 3);
		if (nvram_get_int("usb_automount"))
			run_nvscript("script_usbmount", mountpoint, 3);
	}

	return (ret == MOUNT_VAL_RONLY || ret == MOUNT_VAL_RW);
}

int dir_is_mountpoint(const char *root, const char *dir)
{
	char path[256];
	struct stat sb;
	unsigned int thisdev;

	snprintf(path, sizeof(path), "%s%s%s", root ? : "", root ? "/" : "", dir);

	/* Check if this is a directory */
	sb.st_mode = S_IFDIR; /* failsafe */
	stat(path, &sb);

	if (S_ISDIR(sb.st_mode)) {
		/* If this dir & its parent dir are on the same device, it is not a mountpoint */
		strcat(path, "/.");
		stat(path, &sb);
		thisdev = sb.st_dev;
		strcat(path, ".");
		++sb.st_dev; /* failsafe */
		stat(path, &sb);

		return (thisdev != sb.st_dev);
	}

	return 0;
}

/* Mount or unmount all partitions on this controller.
 * Parameter: action_add:
 * 0  = unmount
 * >0 = mount only if automount config option is enabled.
 * <0 = mount regardless of config option.
 */
void hotplug_usb_storage_device(int host_no, int action_add, uint flags)
{
	if (!nvram_get_int("usb_enable"))
		return;

	logmsg(LOG_DEBUG, "*** %s: host %d action: %d\n", __FUNCTION__, host_no, action_add);

	if (action_add) {
		if (nvram_get_int("usb_storage") && (nvram_get_int("usb_automount") || action_add < 0)) {
			/* Do not probe the device here. It's either initiated by user,
			 * or hotplug_usb() already did.
			 */
			if (exec_for_host(host_no, 0x00, flags, mount_partition))
				restart_nas_services(1, 1); /* restart all NAS applications */
		}
	}
	else {
		if (nvram_get_int("usb_storage") || ((flags & EFH_USER) == 0)) {
			/* When unplugged, unmount the device even if
			 * usb storage is disabled in the GUI.
			 */
			exec_for_host(host_no, (flags & EFH_USER) ? 0x00 : 0x02, flags, umount_partition);
			/* Restart NAS applications (they could be killed by umount_mountpoint),
			 * or just re-read the configuration.
			 */
			restart_nas_services(1, 1);
		}
	}
}

/* This gets called at reboot or upgrade. The system is stopping. */
void remove_storage_main(int shutdn)
{
	if (shutdn)
		restart_nas_services(1, 0);

	/* Unmount all partitions */
	exec_for_host(-1, 0x02, shutdn ? EFH_SHUTDN : 0, umount_partition);
}

/*******
 * All the complex locking & checking code was removed when the kernel USB-storage
 * bugs were fixed.
 * The crash bug was with overlapped I/O to different USB drives, not specifically
 * with mount processing.
 *
 * And for USB devices that are slow to come up.  The kernel now waits until the
 * USB drive has settled, and it correctly reads the partition table before calling
 * the hotplug agent.
 *
 * The kernel patch was cleaning up data structures on an unplug.  It
 * needs to wait until the disk is unmounted.  We have 20 seconds to do
 * the unmounts.
 *******/
static inline void usbled_proc(char *device, int add)
{
	char *p;
	char param[32];
#if defined(CONFIG_BCMWL6) || defined (TCONFIG_BLINK)
	DIR *usb1 = NULL;
	DIR *usb2 = NULL;
	DIR *usb3 = NULL;
	DIR *usb4 = NULL;
#endif

#ifdef TCONFIG_BCMARM
	/* check if there are two LEDs for USB and USB3, see LED table at shared/led.c */
	if (do_led(LED_USB, LED_PROBE) != 255 && do_led(LED_USB3, LED_PROBE) != 255) {
		if (device != NULL) {
			strncpy(param, device, sizeof(param));
			if ((p = strchr(param, ':')) != NULL)
				*p = 0;

			/* verify if we need to ignore this device (i.e. an internal SD/MMC slot ) */
			p = nvram_safe_get("usb_noled");
			if (strcmp(p, param) == 0)
				return;
		}

		/* get router model */
		int model = get_model();

		switch(model) {
		case MODEL_RTN18U:
		case MODEL_RTAC56U:
		case MODEL_DSLAC68U:
		case MODEL_RTAC68U:
		case MODEL_RTAC68UV3:
		case MODEL_RTAC1900P:
#ifdef TCONFIG_BCM714
		case MODEL_RTAC3100:
		case MODEL_RTAC88U:
#ifdef TCONFIG_AC5300
		case MODEL_RTAC5300:
#endif
#endif /* TCONFIG_BCM714 */
		case MODEL_R6400:
		case MODEL_R6400v2:
		case MODEL_R6700v1:
		case MODEL_R6700v3:
		case MODEL_R6900:
		case MODEL_R7000:
		case MODEL_XR300:
#ifdef TCONFIG_AC3200
		case MODEL_R8000:
#endif
		case MODEL_F9K1113v2_20X0:
		case MODEL_F9K1113v2:
			/* switch usb2 --> usb1 and usb4 --> usb3 */
			usb2 = opendir ("/sys/bus/usb/devices/2-1:1.0"); /* Example RT-N18U: port 1 gpio 14 for USB3 */
			usb1 = opendir ("/sys/bus/usb/devices/2-2:1.0"); /* Example RT-N18U: port 2 gpio 3 */
			usb4 = opendir ("/sys/bus/usb/devices/1-1:1.0");
			usb3 = opendir ("/sys/bus/usb/devices/1-2:1.0");
			break;
		default:
			/* default - keep it in place (for the future), if there is a router with a different setup/config!
			   Right now all router have USB3 connected to port 1 */
			usb1 = opendir ("/sys/bus/usb/devices/2-1:1.0");
			usb2 = opendir ("/sys/bus/usb/devices/2-2:1.0");
			usb3 = opendir ("/sys/bus/usb/devices/1-1:1.0");
			usb4 = opendir ("/sys/bus/usb/devices/1-2:1.0");
			break;
		}

		if (add) {
			if (usb1 != NULL) {
				do_led(LED_USB, LED_ON); /* USB LED On! */
				(void) closedir (usb1);
				usb1 = NULL;
			}
			if (usb3 != NULL) {
				do_led(LED_USB, LED_ON); /* USB LED On! */
				(void) closedir (usb3);
				usb3 = NULL;
			}
			if (usb2 != NULL) {
				do_led(LED_USB3, LED_ON); /* USB3 LED On! */
				(void) closedir (usb2);
				usb2 = NULL;
			}
			if (usb4 != NULL) {
				do_led(LED_USB3, LED_ON); /* USB3 LED On! */
				(void) closedir (usb4);
				usb4 = NULL;
			}
		}
		else {
			if (usb1 == NULL && usb3 == NULL)
				do_led(LED_USB, LED_OFF); /* USB LED Off! */

			if (usb2 == NULL && usb4 == NULL)
				do_led(LED_USB3, LED_OFF); /* USB3 LED Off! */
		}
	}
	else
#endif /* TCONFIG_BCMARM */
	/* only one LED for USB */
	     if (do_led(LED_USB, LED_PROBE) != 255) {
#if defined(CONFIG_BCMWL6) || defined (TCONFIG_BLINK)
		if (device != NULL) {
#endif
			strncpy(param, device, sizeof(param));
			if ((p = strchr(param, ':')) != NULL)
				*p = 0;

			/* verify if we need to ignore this device (i.e. an internal SD/MMC slot ) */
			p = nvram_safe_get("usb_noled");
			if (strcmp(p, param) == 0)
				return;

#if defined(CONFIG_BCMWL6) || defined (TCONFIG_BLINK)
		}

		usb1 = opendir ("/sys/bus/usb/devices/2-1:1.0");
		usb2 = opendir ("/sys/bus/usb/devices/2-2:1.0");
		usb3 = opendir ("/sys/bus/usb/devices/1-1:1.0");
		usb4 = opendir ("/sys/bus/usb/devices/1-2:1.0");

		if (add) {
			if (usb1 != NULL) {
				do_led(LED_USB, LED_ON); /* USB LED On! */
				(void) closedir (usb1);
				usb1 = NULL;
			}
			if (usb3 != NULL) {
				do_led(LED_USB, LED_ON); /* USB LED On! */
				(void) closedir (usb3);
				usb3 = NULL;
			}
			if (usb2 != NULL) {
				do_led(LED_USB, LED_ON); /* USB LED On! */
				(void) closedir (usb2);
				usb2 = NULL;
			}
			if (usb4 != NULL) {
				do_led(LED_USB, LED_ON); /* USB LED On! */
				(void) closedir (usb4);
				usb4 = NULL;
			}
		}
		else {
			if (usb1 == NULL && usb3 == NULL && usb2 == NULL && usb4 == NULL)
				do_led(LED_USB, LED_OFF); /* USB LED Off! */
		}
#else
		f_write_string(add ? "/proc/leds-usb/add" : "/proc/leds-usb/remove", param, 0, 0);
#endif
	} /* else if (only one LED for USB) */
}

/* Plugging or removing usb device
 *
 * On an occurrance, multiple hotplug events may be fired off.
 * For example, if a hub is plugged or unplugged, an event
 * will be generated for everything downstream of it, plus one for
 * the hub itself.  These are fired off simultaneously, not serially.
 * This means that many many hotplug processes will be running at
 * the same time.
 *
 * The hotplug event generated by the kernel gives us several pieces
 * of information:
 * PRODUCT is vendorid/productid/rev#.
 * DEVICE is /proc/bus/usb/bus#/dev#
 * ACTION is add or remove
 * SCSI_HOST is the host (controller) number (this relies on the custom kernel patch)
 *
 * Note that when we get a hotplug add event, the USB susbsystem may
 * or may not have yet tried to read the partition table of the
 * device.  For a new controller that has never been seen before,
 * generally yes.  For a re-plug of a controller that has been seen
 * before, generally no.
 *
 * On a remove, the partition info has not yet been expunged.  The
 * partitions show up as /dev/discs/disc#/part#, and /proc/partitions.
 * It appears that doing a "stat" for a non-existant partition will
 * causes the kernel to re-validate the device and update the
 * partition table info.  However, it won't re-validate if the disc is
 * mounted--you'll get a "Device busy for revalidation (usage=%d)" in
 * syslog.
 *
 * The $INTERFACE is "class/subclass/protocol"
 * Some interesting classes:
 *	8 = mass storage
 *	7 = printer
 *	3 = HID.   3/1/2 = mouse.
 *	6 = still image (6/1/1 = Digital camera Camera)
 *	9 = Hub
 *	255 = scanner (255/255/255)
 *
 * Observed:
 *	Hub seems to have no INTERFACE (null), and TYPE of "9/0/0"
 *	Flash disk seems to have INTERFACE of "8/6/80", and TYPE of "0/0/0"
 *
 * When a hub is unplugged, a hotplug event is generated for it and everything
 * downstream from it.  You cannot depend on getting these events in any
 * particular order, since there will be many hotplug programs all fired off
 * at almost the same time.
 * On a remove, don't try to access the downstream devices right away, give the
 * kernel time to finish cleaning up all the data structures, which will be
 * in the process of being torn down.
 *
 * On the initial plugin, the first time the kernel usb-storage subsystem sees
 * the host (identified by GUID), it automatically reads the partition table.
 * On subsequent plugins, it does not.
 *
 * Special values for Web Administration to unmount or remount
 * all partitions of the host:
 *	INTERFACE=TOMATO/...
 *	ACTION=add/remove
 *	SCSI_HOST=<host_no>
 * If host_no is negative, we unmount all partions of *all* hosts.
 */
void hotplug_usb(void)
{
	int add;
	int host = -1;
	char *interface = getenv("INTERFACE");
	char *action = getenv("ACTION");
	char *product = getenv("PRODUCT");
	char *device = getenv("DEVICENAME");
	int is_block = strcmp(getenv("SUBSYSTEM") ? : "", "block") == 0;
	char *scsi_host = getenv("SCSI_HOST");

	logmsg(LOG_DEBUG, "*** %s: %s hotplug INTERFACE=%s ACTION=%s PRODUCT=%s HOST=%s DEVICE=%s\n", __FUNCTION__, getenv("SUBSYSTEM") ? : "USB", interface, action, product, scsi_host, device);

	if (!nvram_get_int("usb_enable"))
		return;
	if (!action || ((!interface || !product) && !is_block))
		return;

	if (scsi_host)
		host = atoi(scsi_host);

	if (!wait_action_idle(10)) return;

	add = (strcmp(action, "add") == 0);
	if (add && (strncmp(interface ? : "", "TOMATO/", 7) != 0)) {
		if (!is_block && device)
			logmsg(LOG_DEBUG, "*** %s: attached USB device %s [INTERFACE=%s PRODUCT=%s]", __FUNCTION__, device, interface, product);
	}

	if (strncmp(interface ? : "", "TOMATO/", 7) == 0) { /* web admin */
		if (scsi_host == NULL)
			host = atoi(product); /* for backward compatibility */
		/* If host is negative, unmount all partitions of *all* hosts.
		 * If host == -1, execute "soft" unmount (do not kill NAS apps, no "lazy" umount).
		 * If host == -2, run "hard" unmount, as if the drive is unplugged.
		 * This feature can be used in custom scripts as following:
		 *
		 * # INTERFACE=TOMATO/1 ACTION=remove PRODUCT=-1 SCSI_HOST=-1 hotplug usb
		 *
		 * PRODUCT is required to pass the env variables verification.
		 */

		/* Unmount or remount all partitions of the host. */
		hotplug_usb_storage_device(host < 0 ? -1 : host, add ? -1 : 0, host == -2 ? 0 : EFH_USER);

#if defined(CONFIG_BCMWL6) || defined (TCONFIG_BLINK)
		if (device == NULL)
			usbled_proc(device, add);
#endif
	}
	else if (is_block && strcmp(getenv("MAJOR") ? : "", "8") == 0 && strcmp(getenv("PHYSDEVBUS") ? : "", "scsi") == 0) {
		/* scsi partition */
		char devname[64];
		int lock;

		sprintf(devname, "/dev/%s", device);
		lock = file_lock("usb");
		if (add) {
			if (nvram_get_int("usb_storage") && nvram_get_int("usb_automount")) {
				int minor = atoi(getenv("MINOR") ? : "0");
				if ((minor % 16) == 0 && !is_no_partition(device)) {
					/* This is a disc, and not a "no-partition" device,
					 * like APPLE iPOD shuffle. We can't mount it.
					 */
					return;
				}
				if (mount_partition(devname, host, NULL, device, EFH_HP_ADD))
					restart_nas_services(1, 1); /* restart all NAS applications */
			}
		}
		else {
			/* When unplugged, unmount the device even if usb storage is disabled in the GUI */
			umount_partition(devname, host, NULL, device, EFH_HP_REMOVE);
			/* Restart NAS applications (they could be killed by umount_mountpoint),
			 * or just re-read the configuration.
			 */
			restart_nas_services(1, 1);
		}
		file_unlock(lock);
	}
	else if (strncmp(interface ? : "", "8/", 2) == 0) { /* usb storage */
		usbled_proc(device, add);
		run_nvscript("script_usbhotplug", NULL, 2);
	}
	else { /* It's some other type of USB device, not storage. */
		if (is_block)
			return;
		if (strncmp(interface ? : "", "7/", 2) == 0) /* printer */
			usbled_proc(device, add);

		/* Do nothing. The user's hotplug script must do it all. */
		run_nvscript("script_usbhotplug", NULL, 2);
	}
}
