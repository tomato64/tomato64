#!/bin/sh

set -e

KERNEL_VERSION=$(ls $BUILD_DIR | grep "linux-headers" | cut -d- -f3)

rm -f $BINARIES_DIR/bananapi_bpi-r3-mini-kernel.bin
rm -f $BINARIES_DIR/tomato64-bpi-r3-mini-sysupgrade.itb

cp $BINARIES_DIR/Image \
$BINARIES_DIR/bananapi_bpi-r3-mini-kernel.bin

gzip -f -9n -c $BINARIES_DIR/bananapi_bpi-r3-mini-kernel.bin  > $BINARIES_DIR/bananapi_bpi-r3-mini-kernel.bin.new
mv $BINARIES_DIR/bananapi_bpi-r3-mini-kernel.bin.new $BINARIES_DIR/bananapi_bpi-r3-mini-kernel.bin

dd if=$BINARIES_DIR/bananapi_bpi-r3-mini-kernel.bin >> $BINARIES_DIR/tomato64-bpi-r3-mini-sysupgrade.itb

$BR2_EXTERNAL_TOMATO64_PATH/board/arm64/common/mkits.sh \
-D bananapi_bpi-r3-mini \
-o $BINARIES_DIR/tomato64-bpi-r3-mini-sysupgrade.itb.its \
-k $BINARIES_DIR/tomato64-bpi-r3-mini-sysupgrade.itb \
-C gzip \
-d $BINARIES_DIR/mt7986a-bananapi-bpi-r3-mini.dtb \
-r $BINARIES_DIR/rootfs.squashfs \
-a 0x44000000 -e 0x44000000 -s 0x43f00000 \
-c config-mt7986a-bananapi-bpi-r3-mini \
-A arm64 \
-v $KERNEL_VERSION

mkimage  -E \
-B 0x1000 -p 0x1000 \
-f $BINARIES_DIR/tomato64-bpi-r3-mini-sysupgrade.itb.its \
$BINARIES_DIR/tomato64-bpi-r3-mini-sysupgrade.itb.new

mv $BINARIES_DIR/tomato64-bpi-r3-mini-sysupgrade.itb.new \
$BINARIES_DIR/tomato64-bpi-r3-mini-sysupgrade.itb

VERSION="$(cat $BR2_EXTERNAL_TOMATO64_PATH/version)"
COMMIT="$(git log -n 1 --pretty=format:"%H")"

echo '{  \"metadata_version\": \"1.1\", \"compat_version\": \"1.0\",   \"supported_devices\":[\"bananapi,bpi-r3-mini\"], "\version\": { \"dist\": \"Tomato64\", \"version\": \"$(echo $VERSION)\", \"revision\": \"$(echo $COMMIT)\", \"target\": \"mediatek/filogic\", \"board\": \"bananapi_bpi-r3-mini\" } }' | fwtool -I - $BINARIES_DIR/tomato64-bpi-r3-mini-sysupgrade.itb
