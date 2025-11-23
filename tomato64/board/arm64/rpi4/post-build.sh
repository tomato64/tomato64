#!/bin/bash

set -u
set -e

mkdir -p $TARGET_DIR/boot/overlays
for i in "${BINARIES_DIR}"/*.dtb "${BINARIES_DIR}"/rpi-firmware/*; do
    echo $i
    cp -ar $i $TARGET_DIR/boot
done

KERNEL=$(sed -n 's/^kernel=//p' "${BINARIES_DIR}/rpi-firmware/config.txt")
cp -a "${BINARIES_DIR}"/$KERNEL $TARGET_DIR/boot

$BR2_EXTERNAL_TOMATO64_PATH/board/common/post-build-fs.sh