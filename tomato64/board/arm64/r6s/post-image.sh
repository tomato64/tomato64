#!/bin/sh

set -e
set -x

BOARD_NAME="friendlyarm_nanopi-r6s"
BOOT_DIR="$BINARIES_DIR/boot"

	rm -rf $BOOT_DIR
	mkdir -p $BOOT_DIR
	cp $BINARIES_DIR/friendlyarm_nanopi-r6s-kernel.bin $BOOT_DIR/kernel.img
	cp $BINARIES_DIR/boot.scr $BOOT_DIR/boot.scr

	rm -f $BINARIES_DIR/boot.ext4
	mkfs.ext4 -L kernel -d $BOOT_DIR $BINARIES_DIR/boot.ext4 32M

	cp \
	$BR2_EXTERNAL_TOMATO64_PATH/board/arm64/r6s/genimage-sysupgrade.cfg \
	"$BINARIES_DIR/genimage-sysupgrade.cfg"
	support/scripts/genimage.sh -c "$BINARIES_DIR/genimage-sysupgrade.cfg"

	gzip \
	-f -9n -c \
	$BINARIES_DIR/tomato64-r6s-sysupgrade.img > \
	$BINARIES_DIR/tomato64-r6s-sysupgrade.img.gz

	VERSION="$(cat $BR2_EXTERNAL_TOMATO64_PATH/version)"
	COMMIT="$(git -C $BR2_EXTERNAL_TOMATO64_PATH log -n 1 --pretty=format:"%H" 2>/dev/null || echo "unknown")"

	echo "{ \"metadata_version\": \"1.1\", \"compat_version\": \"1.0\", \"supported_devices\":[\"friendlyarm,nanopi-r6s\"], \"version\": { \"dist\": \"Tomato64\", \"version\": \"$VERSION\", \"revision\": \"$COMMIT\", \"target\": \"rockchip/armv8\", \"board\": \"friendlyarm_nanopi-r6s\" } }" | \
	fwtool -I - $BINARIES_DIR/tomato64-r6s-sysupgrade.img.gz

	rm -rf $BOOT_DIR
	rm -f $BINARIES_DIR/boot.ext4
	rm -f $BINARIES_DIR/genimage-sysupgrade.cfg

	exit 0
