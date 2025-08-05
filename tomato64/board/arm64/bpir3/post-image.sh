#!/bin/sh

set -e
set -x

cp $BR2_EXTERNAL_TOMATO64_PATH/board/arm64/bpir3/genimage-sdmmc.cfg "$BINARIES_DIR/genimage-sdmmc.cfg"
cp $BR2_EXTERNAL_TOMATO64_PATH/board/arm64/bpir3/genimage-emmc.cfg "$BINARIES_DIR/genimage-emmc.cfg"
support/scripts/genimage.sh -c "$BINARIES_DIR/genimage-sdmmc.cfg"
support/scripts/genimage.sh -c "$BINARIES_DIR/genimage-emmc.cfg"

gzip -k $BINARIES_DIR/tomato64-bpi-r3-sdcard.img
gzip -k $BINARIES_DIR/tomato64-bpi-r3-emmc.img
