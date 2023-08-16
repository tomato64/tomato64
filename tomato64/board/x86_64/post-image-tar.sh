#!/bin/sh

set -e
set -x

rm -rf $BINARIES_DIR/update.tzst
mv $BINARIES_DIR/rootfs.tar.zst $BINARIES_DIR/update.tzst
