#!/bin/sh

set -e

sh \
$BR2_EXTERNAL_TOMATO64_PATH/board/arm64/common/sysupgrade-tar.sh \
--board glinet_gl-mt6000 \
--kernel $BINARIES_DIR/glinet_gl-mt6000-kernel.bin \
--rootfs $BINARIES_DIR/rootfs.ext4 \
$BINARIES_DIR/tomato64-gl-mt6000-sysupgrade.bin

VERSION="$(cat $BR2_EXTERNAL_TOMATO64_PATH/version)"
COMMIT="$(git log -n 1 --pretty=format:"%H")"

echo "{  \"metadata_version\": \"1.1\", \"compat_version\": \"1.0\",   \"supported_devices\":[\"glinet,gl-mt6000\"], \"version\": { \"release\": \"$(echo $VERSION)\", \"date\": \"$(date '+%Y%m%d%H%M%S')\", \"dist\": \"Tomato64\", \"version\": \"$(echo $VERSION)\", \"revision\": \"$(echo $COMMIT)\", \"target\": \"mediatek/filogic\", \"board\": \"glinet_gl-mt6000\" } }" \
| fwtool -I - $BINARIES_DIR/tomato64-gl-mt6000-sysupgrade.bin
