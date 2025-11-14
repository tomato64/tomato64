#!/bin/sh

set -e
set -x

MODEL="$2"

FILENAME="${MODEL}-update.tzst"

rm -rf $BINARIES_DIR/$FILENAME
mv $BINARIES_DIR/rootfs.tar.zst $BINARIES_DIR/$FILENAME
