/*
 * bridger.c
 *
 * Bridge forwarding accelerator (https://github.com/nbd168/bridger).
 *
 * bridger attaches an eBPF tc classifier to every bridge member and moves
 * established bridged flows out of the kernel bridge path. Where the ethernet
 * driver can take them (MediaTek PPE, and WED for WiFi) it also installs them
 * in hardware. It handles bridged (same-LAN) traffic only; routed/NAT traffic
 * is the business of flow_offloading and is unaffected by this setting.
 */

#include "rc.h"

#include <sys/mount.h>
#include <sys/vfs.h>

#ifdef TOMATO64_HAS_BRIDGER

/* needed by logmsg() */
#define LOGMSG_DISABLE	DISABLE_SYSLOG_OS
#define LOGMSG_NVDEBUG	"bridger_debug"

#define BRIDGER_BIN	"/usr/bin/bridger"
#define BPFFS_DIR	"/sys/fs/bpf"
#define BPFFS_MAGIC	0xcafe4a11	/* BPF_FS_MAGIC */

/* Mounting bpffs a second time does not fail, it stacks another instance on
 * top - so look before mounting.
 */
static int bpffs_mounted(void)
{
	struct statfs sf;

	return (statfs(BPFFS_DIR, &sf) == 0) && ((unsigned long)sf.f_type == BPFFS_MAGIC);
}

/* Bring bridger in line with nvram. Safe to call repeatedly: a running daemon
 * is left alone, so this can sit on the firewall restart path.
 */
void start_bridger(void)
{
	if (!nvram_get_int("bridger_enable")) {
		stop_bridger();
		return;
	}

	if (pidof("bridger") > 0)
		return;

	/* bridger pins its program and maps under /sys/fs/bpf, and nothing
	 * else mounts bpffs.
	 */
	if (!bpffs_mounted() && mount("bpf", BPFFS_DIR, "bpf", 0, NULL)) {
		logmsg(LOG_ERR, "bridger: cannot mount %s (%s)", BPFFS_DIR, strerror(errno));
		return;
	}

	xstart(BRIDGER_BIN);
}

void stop_bridger(void)
{
	if (pidof("bridger") > 0)
		killall_tk_period_wait("bridger", 50);
}

#endif /* TOMATO64_HAS_BRIDGER */
