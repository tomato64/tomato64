#!/bin/sh

set -e
set -x

cp $BR2_EXTERNAL_TOMATO64_PATH/board/arm64/bpir3mini/genimage.cfg "$BINARIES_DIR/genimage.cfg"
support/scripts/genimage.sh -c "$BINARIES_DIR/genimage.cfg"

gzip -k $BINARIES_DIR/tomato64-bpi-r3-mini.img
