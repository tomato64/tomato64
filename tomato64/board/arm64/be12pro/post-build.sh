#!/bin/sh

set -e
set -x


	# Generate kernel fit file

	KERNEL_VERSION=$(ls $BUILD_DIR | grep "linux-headers" | cut -d- -f3)

	lzma_alone e $BINARIES_DIR/Image -lc1 -lp2 -pb2 $BINARIES_DIR/tenda_be12-pro-kernel.bin

	$BR2_EXTERNAL_TOMATO64_PATH/board/arm64/common/mkits.sh \
	-D tenda_be12-pro \
	-o $BINARIES_DIR/tenda_be12-pro-kernel.bin.its \
	-k $BINARIES_DIR/tenda_be12-pro-kernel.bin \
	-C lzma \
	-d $BINARIES_DIR/mt7987a-tenda-be12-pro.dtb \
	-a 0x40000000 -e 0x40000000 \
	-c "config-1" \
	-A arm64 \
	-v $KERNEL_VERSION

	mkimage \
	-f $BINARIES_DIR/tenda_be12-pro-kernel.bin.its \
	$BINARIES_DIR/tenda_be12-pro-kernel.bin

	# The stock Tenda bootloader only boots a kernel that starts with its
	# 16-byte header: "God1", 4 zero bytes, then the gzip trailer of the
	# image (CRC32 + length, little endian). Same as OpenWrt's
	# Build/tenda-mkdualimageheader.
	KERNEL=$BINARIES_DIR/tenda_be12-pro-kernel.bin
	printf 'God1\000\000\000\000' > $KERNEL.new
	gzip -c $KERNEL | tail -c8 >> $KERNEL.new
	cat $KERNEL >> $KERNEL.new
	mv $KERNEL.new $KERNEL

	# NAND/squashfs device: the kernel is written to the raw "kernel" mtd
	# partition by the upgrade flow - it is NOT carried inside the
	# (read-only squashfs) rootfs, so do not copy it into $TARGET_DIR/boot.

	# Board identity for /sbin/upgrade and /sbin/tomato64-sysupgrade
	mkdir -p $TARGET_DIR/rom/etc
	cat > $TARGET_DIR/rom/etc/tomato64-sysupgrade.conf <<-EOF
	BOARD="tenda,be12-pro"
	BOARD_DIR="sysupgrade-tenda_be12-pro"
	MODEL="Tenda BE12 Pro"
	IMAGE_NAME="tomato64-tenda-be12-pro-sysupgrade.bin"
	EOF

	$BR2_EXTERNAL_TOMATO64_PATH/board/common/post-build-fs.sh
