#!/bin/sh

set -e
set -x


	# Generate kernel fit file

	KERNEL_VERSION=$(ls $BUILD_DIR | grep "linux-headers" | cut -d- -f3)

	lzma_alone e $BINARIES_DIR/Image -lc1 -lp2 -pb2 $BINARIES_DIR/glinet_gl-mt3600be-kernel.bin

	$BR2_EXTERNAL_TOMATO64_PATH/board/arm64/common/mkits.sh \
	-D glinet_gl-mt3600be \
	-o $BINARIES_DIR/glinet_gl-mt3600be-kernel.bin.its \
	-k $BINARIES_DIR/glinet_gl-mt3600be-kernel.bin \
	-C lzma \
	-d $BINARIES_DIR/mt7987a-glinet-gl-mt3600be.dtb \
	-a 0x40000000 -e 0x40000000 \
	-c "config-1" \
	-A arm64 \
	-v $KERNEL_VERSION

	mkimage \
	-f $BINARIES_DIR/glinet_gl-mt3600be-kernel.bin.its \
	$BINARIES_DIR/glinet_gl-mt3600be-kernel.bin

	# NAND/squashfs device: the kernel FIT is written to the "kernel" UBI
	# volume by post-image.sh / the upgrade script - it is NOT carried inside
	# the (read-only squashfs) rootfs, so do not copy it into $TARGET_DIR/boot.

	$BR2_EXTERNAL_TOMATO64_PATH/board/common/post-build-fs.sh
