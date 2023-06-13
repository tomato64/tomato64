#!/bin/sh

set -e

BOARD_DIR=$(dirname "$0")

cp -f "$BOARD_DIR/grub-bios.cfg" "$TARGET_DIR/boot/grub/grub.cfg"

$BR2_EXTERNAL_TOMATO64_PATH/board/x86_64/post-build-fs.sh
