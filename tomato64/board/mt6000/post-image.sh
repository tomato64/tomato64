#!/bin/sh

set -e

KERNEL_VERSION=$(ls $BUILD_DIR | grep "linux-headers" | cut -d- -f3)

lzma_alone e $BINARIES_DIR/Image -lc1 -lp2 -pb2 $BINARIES_DIR/glinet_gl-mt6000-kernel.bin

$BR2_EXTERNAL_TOMATO64_PATH/board/mt6000/mkits.sh \
-D glinet_gl-mt6000 \
-o $BINARIES_DIR/glinet_gl-mt6000-kernel.bin.its \
-k $BINARIES_DIR/glinet_gl-mt6000-kernel.bin \
-C lzma \
-d $BINARIES_DIR/mt7986a-glinet-gl-mt6000.dtb \
-a 0x48000000 -e 0x48000000 \
-c "config-1" \
-A arm64 \
-v $KERNEL_VERSION

mkimage \
-f $BINARIES_DIR/glinet_gl-mt6000-kernel.bin.its \
$BINARIES_DIR/glinet_gl-mt6000-kernel.bin

sh \
$BR2_EXTERNAL_TOMATO64_PATH/board/mt6000/sysupgrade-tar.sh \
--board glinet_gl-mt6000 \
--kernel $BINARIES_DIR/glinet_gl-mt6000-kernel.bin \
--rootfs $BINARIES_DIR/rootfs.ext4 \
$BINARIES_DIR/tomato64-gl-mt6000-sysupgrade.bin

VERSION="$(cat $BR2_EXTERNAL_TOMATO64_PATH/version)"
COMMIT="$(git log -n 1 --pretty=format:"%H")"

echo "{  \"metadata_version\": \"1.1\", \"compat_version\": \"1.0\",   \"supported_devices\":[\"glinet,gl-mt6000\"], \"version\": { \"release\": \"$(echo $VERSION)\", \"date\": \"$(date '+%Y%m%d%H%M%S')\", \"dist\": \"Tomato64\", \"version\": \"$(echo $VERSION)\", \"revision\": \"$(echo $COMMIT)\", \"target\": \"mediatek/filogic\", \"board\": \"glinet_gl-mt6000\" } }" \
| fwtool -I - $BINARIES_DIR/tomato64-gl-mt6000-sysupgrade.bin
