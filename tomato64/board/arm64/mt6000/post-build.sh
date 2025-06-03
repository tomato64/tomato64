#!/bin/sh

set -e
set -x


	# Generate kernel fit file

	KERNEL_VERSION=$(ls $BUILD_DIR | grep "linux-headers" | cut -d- -f3)

	lzma_alone e $BINARIES_DIR/Image -lc1 -lp2 -pb2 $BINARIES_DIR/glinet_gl-mt6000-kernel.bin

	$BR2_EXTERNAL_TOMATO64_PATH/board/arm64/common/mkits.sh \
	-D glinet_gl-mt6000 \
	-o $BINARIES_DIR/glinet_gl-mt6000-kernel.bin.its \
	-k $BINARIES_DIR/glinet_gl-mt6000-kernel.bin \
	-C lzma \
	-d $BINARIES_DIR/mt7986a-glinet-gl-mt6000.dtb \
	-a 0x48000000 -e 0x48000000 \
	-c "config-1" \
	-A arm64 \
	-v $KERNEL_VERSION

	mkimage \
	-f $BINARIES_DIR/glinet_gl-mt6000-kernel.bin.its \
	$BINARIES_DIR/glinet_gl-mt6000-kernel.bin

	mkdir -p $TARGET_DIR/boot
	cp $BINARIES_DIR/glinet_gl-mt6000-kernel.bin $TARGET_DIR/boot/

	$BR2_EXTERNAL_TOMATO64_PATH/board/common/post-build-fs.sh
