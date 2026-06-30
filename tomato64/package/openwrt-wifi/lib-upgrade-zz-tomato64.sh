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

# BCM53XX-only override (filogic/MT3600BE never calls this). Stock OpenWrt
# platform_do_upgrade_nand_trx does `otrx extract -2 root`, which mallocs the
# ENTIRE rootfs partition into RAM. Tomato64's rootfs is ~85 MB, so on these
# 128 MB-flash Broadcom routers that alloc fails after stage2's RAM pivot; the
# stock code then `return`s and platform_other_do_upgrade falls through to the
# bad-block-UNAWARE "Writing whole image to NAND flash" path (default_do_upgrade
# raw mtd write), which corrupts UBI on any flash with a bad block -> brick
# (ubi_attach: bad image sequence number ...).
#
# We instead: extract ONLY the (small) kernel, then STREAM the UBI region from
# the TRX straight into ubiformat via nand.sh's cmd mechanism (no big malloc,
# no second copy). And we never `return` into the whole-image fallback - every
# error path `exit 1`s so a failed upgrade leaves flash untouched / boots the
# old image rather than half-writing a raw UBI.
#
# $1 = TRX file (already validated by otrx check). $2 = TRX offset within $1
#      (non-zero only for chk/seama wrappers, which Tomato64 does not ship).
platform_do_upgrade_nand_trx() {
	local trx="$1"
	local offset="${2:-0}"

	# TRX header is little-endian: u32 total length @4, u32 partition offsets
	# @16 (kernel) / @20 (ubi) / @24. Read a LE u32 at byte $1 of the file.
	_t64_le32() {
		set -- $(dd if="$trx" bs=1 skip="$1" count=4 2>/dev/null | hexdump -v -e '4/1 "%d "')
		echo $(( ($1 & 255) | (($2 & 255) << 8) | (($3 & 255) << 16) | (($4 & 255) << 24) ))
	}
	local trx_length=$(_t64_le32 $((offset + 4)))
	local part1=$(_t64_le32 $((offset + 20)))
	if [ -z "$trx_length" ] || [ -z "$part1" ] || [ "$part1" -eq 0 ]; then
		echo "Tomato64: cannot parse TRX partition table - aborting"; exit 1
	fi
	local ubi_abs=$((offset + part1))
	local ubi_length=$((trx_length - part1))

	# We stream the UBI in 128 KiB erase blocks, so both must be EB-aligned.
	# Tomato64 ships only bare TRX (offset 0; otrx -a 0x20000 aligns the UBI
	# partition), so this always holds; bail loudly if a future image breaks it.
	if [ $((ubi_abs % 131072)) -ne 0 ] || [ $((ubi_length % 131072)) -ne 0 ] || [ "$ubi_length" -le 0 ]; then
		echo "Tomato64: UBI region not 128K-aligned (abs=$ubi_abs len=$ubi_length) - aborting"; exit 1
	fi
	if [ "$(dd if="$trx" bs=1 skip=$ubi_abs count=4 2>/dev/null)" != "UBI#" ]; then
		echo "Tomato64: no UBI magic at offset $ubi_abs - aborting"; exit 1
	fi

	# Validate the kernel fits the "linux" mtd BEFORE writing anything.
	local linux_length=$(grep "\"linux\"" /proc/mtd | sed 's/mtd[0-9]*:[ \t]*\([^ \t]*\).*/\1/')
	[ -z "$linux_length" ] && { echo "Tomato64: no \"linux\" partition - aborting"; exit 1; }
	linux_length=$((0x$linux_length))

	rm -f /tmp/kernel.bin /tmp/null.bin /tmp/kernel.trx
	if ! otrx extract "$trx" ${offset:+-o $offset} -1 /tmp/kernel.bin || [ ! -s /tmp/kernel.bin ]; then
		echo "Tomato64: failed to extract kernel - aborting"; exit 1
	fi
	local kernel_length=$(wc -c < /tmp/kernel.bin)
	if [ "$kernel_length" -gt "$linux_length" ]; then
		echo "Tomato64: new kernel ($kernel_length) doesn't fit \"linux\" ($linux_length) - aborting"; exit 1
	fi
	touch /tmp/null.bin
	if ! otrx create /tmp/kernel.trx -f /tmp/kernel.bin -b $((linux_length + 28)) -f /tmp/null.bin; then
		echo "Tomato64: failed to build kernel-only TRX - aborting"; exit 1
	fi
	rm -f /tmp/kernel.bin /tmp/null.bin

	# Point of no return: write the kernel TRX, then hand the UBI region to
	# nand_do_upgrade as a streaming cmd (dd reads only the UBI bytes; the
	# multipart-boundary trailer past trx_length is excluded by count).
	if ! mtd write /tmp/kernel.trx firmware; then
		echo "Tomato64: kernel write failed - aborting"; exit 1
	fi
	rm -f /tmp/kernel.trx

	local skip_eb=$((ubi_abs / 131072))
	local full_eb=$((ubi_length / 131072))

	# Exclude the trailing 0xdeadc0de EOF-marker block (and any pad): ubiformat
	# wants a clean UBI image and erases the whole mtd itself, so the marker
	# isn't needed here (it's only for raw-write installs). This mirrors
	# OpenWrt's "truncate to consecutive UBI# blocks". Scan back to the last
	# UBI# block - the marker is at the end, so this stops almost immediately.
	local count_eb=$full_eb
	while [ "$count_eb" -gt 0 ]; do
		[ "$(dd if="$trx" bs=1 skip=$((ubi_abs + (count_eb - 1) * 131072)) count=4 2>/dev/null)" = "UBI#" ] && break
		count_eb=$((count_eb - 1))
	done
	if [ "$count_eb" -le 0 ]; then
		echo "Tomato64: no UBI# blocks in UBI region - aborting"; exit 1
	fi

	# Hand the UBI region to nand_do_upgrade as a streaming cmd. nand.sh runs
	# it as `$cmd < file` (via variable expansion), so the cmd must be OPERANDS
	# ONLY - a shell redirection like `2>/dev/null` is NOT reparsed after
	# expansion and would become a literal dd argument ("Usage: dd..." ->
	# identify() fails -> wrong tar path). Use dd's own `status=none` to mute
	# the records-count noise instead.
	# nand_do_upgrade -> nand_do_upgrade_success (our override) reboots on
	# success; nand_do_upgrade_failed also reboots. Either way we do NOT return
	# into platform_other_do_upgrade's whole-image fallback.
	nand_do_upgrade "$trx" "dd bs=131072 skip=$skip_eb count=$count_eb status=none"
	exit 1
}
