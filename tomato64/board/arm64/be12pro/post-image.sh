#!/bin/sh

set -e

# Tenda BE12 Pro image build. The only artifact is the OpenWrt-style sysupgrade
# tar, laid out exactly like OpenWrt's tenda_be12-pro image (same flash map,
# same board name), so it installs from OpenWrt's sysupgrade as well as from
# the Tomato64 web UI (/sbin/upgrade -> tomato64-sysupgrade -> stage2 ->
# nand_do_upgrade, which writes the kernel to the raw "kernel" mtd and the
# rootfs to UBI).

KERNEL="$BINARIES_DIR/tenda_be12-pro-kernel.bin"
SQUASHFS="$BINARIES_DIR/rootfs.squashfs"

if [ ! -f "$KERNEL" ]; then
	echo "ERROR: kernel FIT not found at $KERNEL"
	exit 1
fi
if [ ! -f "$SQUASHFS" ]; then
	echo "ERROR: rootfs.squashfs not found at $SQUASHFS"
	exit 1
fi

# Flash budget (mt7987a-tenda-be12-pro.dts): "kernel" is 0x600000 of raw
# NAND, "ubi" is 0x5a00000 = 720 PEBs of 128 KiB. UBI keeps 4 PEBs for
# itself plus 20 for bad blocks (MTD_UBI_BEB_LIMIT per 1024 PEBs of the
# chip), and each LEB is 126976 bytes. Whatever the rootfs volume doesn't
# use becomes rootfs_data, the overlay that holds /nvram and everything else
# written at runtime.
KERNEL_MAX=$((0x600000))
LEB_SIZE=126976
UBI_LEBS=$((720 - 4 - 20))
ROOTFS_DATA_MIN=$((8 * 1024 * 1024))

kernel_size=$(stat -c %s "$KERNEL")
rootfs_size=$(stat -c %s "$SQUASHFS")
rootfs_lebs=$(( (rootfs_size + LEB_SIZE - 1) / LEB_SIZE ))
data_size=$(( (UBI_LEBS - rootfs_lebs) * LEB_SIZE ))

echo "kernel: $kernel_size of $KERNEL_MAX bytes ($(( (KERNEL_MAX - kernel_size) / 1024 )) KiB free)"
echo "rootfs: $rootfs_size bytes, $rootfs_lebs of $UBI_LEBS LEBs; rootfs_data $data_size bytes"

if [ "$kernel_size" -gt "$KERNEL_MAX" ]; then
	echo "ERROR: kernel is $((kernel_size - KERNEL_MAX)) bytes too big for the kernel partition"
	echo "trim board/arm64/be12pro/linux-fragment.config"
	exit 1
fi
if [ "$data_size" -lt "$ROOTFS_DATA_MIN" ]; then
	echo "ERROR: rootfs leaves only $data_size bytes for rootfs_data (minimum $ROOTFS_DATA_MIN)"
	echo "trim packages in configs/be12pro_defconfig"
	exit 1
fi

SYSUPGRADE="$BINARIES_DIR/tomato64-tenda-be12-pro-sysupgrade.bin"

sh \
$BR2_EXTERNAL_TOMATO64_PATH/board/arm64/common/sysupgrade-tar.sh \
--board tenda_be12-pro \
--kernel $KERNEL \
--rootfs $SQUASHFS \
$SYSUPGRADE

# fwtool metadata trailer. OpenWrt's sysupgrade checks supported_devices
# against its board name, so this is what lets an OpenWrt install take the
# Tomato64 image directly.
VERSION="$(cat $BR2_EXTERNAL_TOMATO64_PATH/version)"
COMMIT="$(git log -n 1 --pretty=format:"%H")"

echo "{  \"metadata_version\": \"1.1\", \"compat_version\": \"1.0\",   \"supported_devices\":[\"tenda,be12-pro\"], \"version\": { \"release\": \"$(echo $VERSION)\", \"date\": \"$(date '+%Y%m%d%H%M%S')\", \"dist\": \"Tomato64\", \"version\": \"$(echo $VERSION)\", \"revision\": \"$(echo $COMMIT)\", \"target\": \"mediatek/filogic\", \"board\": \"tenda_be12-pro\" } }" \
| fwtool -I - $SYSUPGRADE
