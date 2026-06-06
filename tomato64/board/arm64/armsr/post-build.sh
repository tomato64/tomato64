#!/bin/sh

set -e

BOARD_DIR=$(dirname "$0")

# EFI boot: install the GRUB config into the ESP staging dir produced by grub2
if [ -d "$BINARIES_DIR/efi-part/" ]; then
    cp -f "$BOARD_DIR/grub-efi.cfg" "$BINARIES_DIR/efi-part/EFI/BOOT/grub.cfg"
fi

$BR2_EXTERNAL_TOMATO64_PATH/board/common/post-build-fs.sh
