/*
 * tomato64-init-stub - no-op pid 1 used only during sysupgrade.
 *
 * On the GL-MT3600BE, pid 1 (rc) is loaded from /romfs/sbin/rc with its libs
 * from /romfs/{usr/lib,lib}. Those mmaps keep the squashfs superblock alive,
 * which keeps /dev/ubiblock0_1 claimed, which makes ubiblock_remove return
 * EBUSY and breaks nand_upgrade_prepare_ubi.
 *
 * OpenWrt's procd works around this by re-execing itself into /sbin/upgraded
 * in the ramfs. Tomato64's rc has no such mechanism, so /sbin/upgrade copies
 * THIS binary into /tmp and sends pid 1 a signal that triggers execve into
 * it. Because this binary is fully statically linked, it has zero runtime
 * references to /romfs once exec'd, the squashfs is released, and the flash
 * proceeds.
 *
 * Must be built STATIC: a dynamic link drags /romfs/lib/libc.so + ld.so via
 * PT_INTERP back in, defeating the purpose.
 */

#include <signal.h>
#include <unistd.h>

int main(void)
{
	sigset_t empty;
	sigemptyset(&empty);
	for (;;)
		sigsuspend(&empty);
	return 0;
}
