#!/bin/sh

set -e

support/scripts/genimage.sh -c "$BR2_EXTERNAL_TOMATO64_PATH/board/x86_64/genimage-bios.cfg"
sfdisk --disk-id "$BINARIES_DIR/tomato64.img" 0x12345678
