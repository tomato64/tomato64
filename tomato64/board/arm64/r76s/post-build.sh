#!/bin/sh

set -e
set -x

	KERNEL_VERSION=$(ls $BUILD_DIR | grep "linux-headers" | cut -d- -f3)

	rm -f $BINARIES_DIR/friendlyarm_nanopi-r76s-kernel.bin
	rm -f $BINARIES_DIR/friendlyarm_nanopi-r76s-kernel.bin.its
	rm -f $BINARIES_DIR/boot.scr
	rm -f $BINARIES_DIR/boot.script

	cp $BINARIES_DIR/Image $BINARIES_DIR/friendlyarm_nanopi-r76s-kernel.bin

	lzma_alone e \
	$BINARIES_DIR/friendlyarm_nanopi-r76s-kernel.bin \
	-lc1 -lp2 -pb2 \
	$BINARIES_DIR/friendlyarm_nanopi-r76s-kernel.bin.new

	mv $BINARIES_DIR/friendlyarm_nanopi-r76s-kernel.bin.new \
	$BINARIES_DIR/friendlyarm_nanopi-r76s-kernel.bin

	$BR2_EXTERNAL_TOMATO64_PATH/board/arm64/common/mkits.sh \
	-D friendlyarm_nanopi-r76s \
	-o $BINARIES_DIR/friendlyarm_nanopi-r76s-kernel.bin.its \
	-k $BINARIES_DIR/friendlyarm_nanopi-r76s-kernel.bin \
	-C lzma \
	-d $BINARIES_DIR/rk3576-nanopi-r76s.dtb \
	-a 0x43000000 -e 0x43000000 \
	-c "config-1" \
	-A arm64 \
	-v $KERNEL_VERSION

	mkimage \
	-f \
	$BINARIES_DIR/friendlyarm_nanopi-r76s-kernel.bin.its \
	$BINARIES_DIR/friendlyarm_nanopi-r76s-kernel.bin.new

	mv $BINARIES_DIR/friendlyarm_nanopi-r76s-kernel.bin.new \
	$BINARIES_DIR/friendlyarm_nanopi-r76s-kernel.bin

cat > $BINARIES_DIR/boot.script <<'BOOTSCRIPT'
part uuid ${devtype} ${devnum}:2 uuid

setenv bootargs "console=ttyS0,1500000 console=tty0 earlycon=uart8250,mmio32,0x2ad40000 root=PARTUUID=${uuid} rw rootwait pcie_aspm=off pcie_port_pm=off";

load ${devtype} ${devnum}:1 ${kernel_addr_r} kernel.img

bootm ${kernel_addr_r}
BOOTSCRIPT

	mkimage -A arm -O linux -T script -C none -a 0 -e 0 \
	-d $BINARIES_DIR/boot.script \
	$BINARIES_DIR/boot.scr

	rm -f $BINARIES_DIR/boot.script

	mkdir -p $TARGET_DIR/boot
	cp $BINARIES_DIR/friendlyarm_nanopi-r76s-kernel.bin $TARGET_DIR/boot/

	$BR2_EXTERNAL_TOMATO64_PATH/board/common/post-build-fs.sh
