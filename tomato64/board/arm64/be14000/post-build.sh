#!/bin/sh

set -e
set -x

	# Generate kernel fit file

	KERNEL_VERSION=$(ls $BUILD_DIR | grep "linux-headers" | cut -d- -f3)

	lzma_alone e $BINARIES_DIR/Image -lc1 -lp2 -pb2 $BINARIES_DIR/glinet_gl-be14000-kernel.bin

	# 0x48000000 is the filogic default load address (loadaddr-y in OpenWrt's
	# mediatek image Makefile), which GL's GL-BE14000 image entry inherits.
	# NOTE: this is NOT the 0x40000000 used by GL-MT3600BE - that board sets an
	# explicit per-device KERNEL_LOADADDR override.
	$BR2_EXTERNAL_TOMATO64_PATH/board/arm64/common/mkits.sh \
	-D glinet_gl-be14000 \
	-o $BINARIES_DIR/glinet_gl-be14000-kernel.bin.its \
	-k $BINARIES_DIR/glinet_gl-be14000-kernel.bin \
	-C lzma \
	-d $BINARIES_DIR/mt7988a-glinet-gl-be14000.dtb \
	-a 0x48000000 -e 0x48000000 \
	-c "config-1" \
	-A arm64 \
	-v $KERNEL_VERSION

	mkimage \
	-f $BINARIES_DIR/glinet_gl-be14000-kernel.bin.its \
	$BINARIES_DIR/glinet_gl-be14000-kernel.bin.new

	mv $BINARIES_DIR/glinet_gl-be14000-kernel.bin.new \
	$BINARIES_DIR/glinet_gl-be14000-kernel.bin

	# ext4 rootfs, so the FIT travels inside it: /sbin/upgrade dd's this file
	# to the "kernel" GPT partition after unpacking the .tzst, the same way
	# the other eMMC boards do.
	mkdir -p $TARGET_DIR/boot
	cp $BINARIES_DIR/glinet_gl-be14000-kernel.bin $TARGET_DIR/boot/

	$BR2_EXTERNAL_TOMATO64_PATH/board/common/post-build-fs.sh
