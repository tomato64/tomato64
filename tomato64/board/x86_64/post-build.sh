#!/bin/sh

set -e

BOARD_DIR=$(dirname "$0")
GRUB2_DIR=$BUILD_DIR/$(ls $BUILD_DIR --ignore='host*' | grep grub2)

# Detect boot strategy, EFI or BIOS
if [ -d "$BINARIES_DIR/efi-part/" ]; then
    cp -f "$BOARD_DIR/grub-efi.cfg" "$BINARIES_DIR/efi-part/EFI/BOOT/grub.cfg"
fi

if [ -d "$GRUB2_DIR/build-i386-pc/" ]; then
    cp -f "$BOARD_DIR/grub-bios.cfg" "$TARGET_DIR/boot/grub/grub.cfg"

    # Copy grub 1st stage to binaries, required for genimage
    cp -f "$GRUB2_DIR/build-i386-pc/grub-core/boot.img" "$BINARIES_DIR"
fi

rm  -f $BINARIES_DIR/iwlwifi*
rm -rf $BINARIES_DIR/rtl*
rm -rf $BINARIES_DIR/rtw*
rm -rf $BINARIES_DIR/mediatek/
rm -rf $BINARIES_DIR/ath10k/

$BR2_EXTERNAL_TOMATO64_PATH/board/common/post-build-fs.sh
