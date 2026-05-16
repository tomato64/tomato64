# Tomato64 overrides for the OpenWrt sysupgrade framework.
# Sourced last by `include /lib/upgrade` (alphabetical: common.sh, nand.sh,
# platform.sh, zz-tomato64.sh), so these defs win over the stock ones.
# nand.sh / common.sh / platform.sh / stage2 / do_stage2 stay byte-for-byte
# OpenWrt.

# No-op: OpenWrt's indicate_upgrade pokes procd LED diag that we don't ship.
indicate_upgrade() { :; }

# Reliable + quiet reboot. OpenWrt's stock flow falls through from
# nand_do_upgrade_success to nand_do_upgrade_failed (echoing "sysupgrade
# failed" after success), and `reboot -f` on this board often stalls in
# the kernel restart-handler loop. We exit after rebooting (suppressing
# the cascade) and trigger sysrq-b as a fallback that calls
# emergency_restart() directly, with /dev/watchdog as the last resort.
_t64_reboot() {
	sync
	# Enable + arm sysrq-b BEFORE umount -a (which would unmount /proc and
	# leave us with nowhere to write sysrq-trigger). reboot -f tries the
	# normal restart path; if its registered handlers stall, sysrq-b's
	# emergency_restart() takes over.
	[ -w /proc/sys/kernel/sysrq ] && echo 1 > /proc/sys/kernel/sysrq
	reboot -f 2>/dev/null
	sleep 3
	[ -w /proc/sysrq-trigger ] && echo b > /proc/sysrq-trigger
	sleep 5
	umount -a 2>/dev/null
	[ -e /dev/watchdog ] && exec 9> /dev/watchdog
	while :; do sleep 60; done
}

nand_do_upgrade_success() {
	if nand_do_restore_config && sync; then
		echo "sysupgrade successful"
		_t64_reboot
	fi
	nand_do_upgrade_failed
}

nand_do_upgrade_failed() {
	echo "sysupgrade failed"
	_t64_reboot
}

# stage2's supivot pivot_root's but never chdir()s, so the shell's cwd
# stays in the lazy-detached old overlay root, keeping the squashfs
# claimed and ubiblock_remove returning EBUSY. cd / drops that ref.
# We also try every plausible mount name (Tomato64 uses /dev/root +
# /romfs, not /dev/ubiblockX_Y), then retry to absorb the async lazy
# cleanup.
nand_remove_ubiblock() {
	local ubivol="$1"
	local ubiblk="ubiblock${ubivol:3}"
	[ -e "/dev/$ubiblk" ] || return 0

	cd / 2>/dev/null
	for tgt in "/dev/$ubiblk" /dev/root /mnt/romfs /romfs; do
		umount "$tgt" 2>/dev/null
	done
	sync
	sleep 1

	local i
	for i in 1 2 3 4 5; do
		ubiblock -r "/dev/$ubivol" 2>/dev/null && return 0
		sleep 1
	done
	echo "ERROR: cannot remove $ubiblk after 5 attempts (still busy)"
	return 1
}

# OpenWrt-native restore: place the tarball at the ubifs root. fstools'
# fopivot renames <vol>/sysupgrade.tgz -> <vol>/upper/sysupgrade.tgz on
# next boot, then the Tomato64 init.c hook (in sysinit, right after
# eval("mount_root")) extracts /sysupgrade.tgz into / before any nvram
# reads. This mirrors OpenWrt's /lib/preinit/80_mount_root.
nand_restore_config() {
	local conf_tar="${1:-/tmp/sysupgrade.tgz}"
	[ -f "$conf_tar" ] || { echo "no config backup to restore"; return 0; }

	local ubidev ubivol
	ubidev="$( nand_find_ubi "${CI_ROOT_UBIPART:-$CI_UBIPART}" )"
	ubivol="$( nand_find_volume "$ubidev" rootfs_data )"
	[ "$ubivol" ] || ubivol="$( nand_find_volume "$ubidev" "$CI_ROOTPART" )"
	if [ ! "$ubivol" ]; then
		echo "cannot find rootfs_data ubifs volume - config NOT restored"
		return 1
	fi

	mkdir -p /tmp/new_root
	if ! mount -t ubifs "/dev/$ubivol" /tmp/new_root; then
		echo "cannot mount ubifs volume $ubivol - config NOT restored"
		rmdir /tmp/new_root
		return 1
	fi

	if mv "$conf_tar" /tmp/new_root/sysupgrade.tgz; then
		echo "config backup staged at rootfs_data root ($(wc -c < /tmp/new_root/sysupgrade.tgz) bytes)"
	else
		echo "WARNING: failed to stage config backup - settings will reset"
	fi

	sync
	umount /tmp/new_root
	rmdir /tmp/new_root 2>/dev/null
	return 0
}
