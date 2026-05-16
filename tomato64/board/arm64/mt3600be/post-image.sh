#!/bin/sh

set -e

# GL-MT3600BE image build. The only artifact is the OpenWrt-style sysupgrade
# tar - used both for the initial install (GL u-boot's firmware upgrade page
# accepts it directly) and for in-system updates via the Tomato64 launcher
# (/sbin/upgrade -> tomato64-sysupgrade -> stage2 -> nand_do_upgrade).

KERNEL="$BINARIES_DIR/glinet_gl-mt3600be-kernel.bin"
SQUASHFS="$BINARIES_DIR/rootfs.squashfs"

if [ ! -f "$KERNEL" ]; then
	echo "ERROR: kernel FIT not found at $KERNEL"
	exit 1
fi
if [ ! -f "$SQUASHFS" ]; then
	echo "ERROR: rootfs.squashfs not found at $SQUASHFS"
	exit 1
fi

SYSUPGRADE="$BINARIES_DIR/tomato64-gl-mt3600be-sysupgrade.bin"

sh \
$BR2_EXTERNAL_TOMATO64_PATH/board/arm64/common/sysupgrade-tar.sh \
--board glinet_gl-mt3600be \
--kernel $KERNEL \
--rootfs $SQUASHFS \
$SYSUPGRADE

# fwtool metadata trailer (informational; lets the artifact be consumed by
# OpenWrt's stock sysupgrade tool too, even though our launcher uses tar
# validation instead of fwtool).
VERSION="$(cat $BR2_EXTERNAL_TOMATO64_PATH/version)"
COMMIT="$(git log -n 1 --pretty=format:"%H")"

echo "{  \"metadata_version\": \"1.1\", \"compat_version\": \"1.0\",   \"supported_devices\":[\"glinet,gl-mt3600be\"], \"version\": { \"release\": \"$(echo $VERSION)\", \"date\": \"$(date '+%Y%m%d%H%M%S')\", \"dist\": \"Tomato64\", \"version\": \"$(echo $VERSION)\", \"revision\": \"$(echo $COMMIT)\", \"target\": \"mediatek/filogic\", \"board\": \"glinet_gl-mt3600be\" } }" \
| fwtool -I - $SYSUPGRADE
