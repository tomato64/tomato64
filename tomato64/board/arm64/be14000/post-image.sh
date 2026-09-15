#!/bin/sh

set -e

# GL-BE14000 image build. Two artifacts, for two different jobs:
#
#   tomato64-gl-be14000-sysupgrade.bin   initial flash, via GL's u-boot recovery
#   be14000-update.tzst                  in-UI updates afterwards
#                                        (board/common/post-image-tar.sh)

KERNEL="$BINARIES_DIR/glinet_gl-be14000-kernel.bin"
ROOTFS="$BINARIES_DIR/rootfs.ext4"
SYSUPGRADE="$BINARIES_DIR/tomato64-gl-be14000-sysupgrade.bin"

if [ ! -f "$KERNEL" ]; then
	echo "ERROR: kernel FIT not found at $KERNEL"
	exit 1
fi
if [ ! -f "$ROOTFS" ]; then
	echo "ERROR: rootfs.ext4 not found at $ROOTFS"
	exit 1
fi

sh \
$BR2_EXTERNAL_TOMATO64_PATH/board/arm64/common/sysupgrade-tar.sh \
--board glinet,gl-be14000 \
--kernel $KERNEL \
--rootfs $ROOTFS \
$SYSUPGRADE

VERSION="$(cat $BR2_EXTERNAL_TOMATO64_PATH/version)"
COMMIT="$(git -C $BR2_EXTERNAL_TOMATO64_PATH log -n 1 --pretty=format:"%H" 2>/dev/null || echo unknown)"

echo "{  \"metadata_version\": \"1.1\", \"compat_version\": \"1.0\",   \"supported_devices\":[\"glinet,gl-be14000\"], \"version\": { \"release\": \"$(echo $VERSION)\", \"date\": \"$(date '+%Y%m%d%H%M%S')\", \"dist\": \"Tomato64\", \"version\": \"$(echo $VERSION)\", \"revision\": \"$(echo $COMMIT)\", \"target\": \"mediatek/filogic\", \"board\": \"glinet_gl-be14000\" } }" \
| fwtool -I - $SYSUPGRADE

exit 0
