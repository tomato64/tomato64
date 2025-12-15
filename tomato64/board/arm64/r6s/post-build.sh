#!/bin/sh

set -e
set -x

	KERNEL_VERSION=$(ls $BUILD_DIR | grep "linux-headers" | cut -d- -f3)

	rm -f $BINARIES_DIR/friendlyarm_nanopi-r6s-kernel.bin
	rm -f $BINARIES_DIR/friendlyarm_nanopi-r6s-kernel.bin.its
	rm -f $BINARIES_DIR/boot.scr
	rm -f $BINARIES_DIR/boot.script

	cp $BINARIES_DIR/Image $BINARIES_DIR/friendlyarm_nanopi-r6s-kernel.bin

	lzma_alone e \
	$BINARIES_DIR/friendlyarm_nanopi-r6s-kernel.bin \
	-lc1 -lp2 -pb2 \
	$BINARIES_DIR/friendlyarm_nanopi-r6s-kernel.bin.new

	mv $BINARIES_DIR/friendlyarm_nanopi-r6s-kernel.bin.new \
	$BINARIES_DIR/friendlyarm_nanopi-r6s-kernel.bin

	$BR2_EXTERNAL_TOMATO64_PATH/board/arm64/common/mkits.sh \
	-D friendlyarm_nanopi-r6s \
	-o $BINARIES_DIR/friendlyarm_nanopi-r6s-kernel.bin.its \
	-k $BINARIES_DIR/friendlyarm_nanopi-r6s-kernel.bin \
	-C lzma \
	-d $BINARIES_DIR/rk3588s-nanopi-r6s.dtb \
	-a 0x03200000 -e 0x03200000 \
	-c "config-1" \
	-A arm64 \
	-v $KERNEL_VERSION

	mkimage \
	-f \
	$BINARIES_DIR/friendlyarm_nanopi-r6s-kernel.bin.its \
	$BINARIES_DIR/friendlyarm_nanopi-r6s-kernel.bin.new

	mv $BINARIES_DIR/friendlyarm_nanopi-r6s-kernel.bin.new \
	$BINARIES_DIR/friendlyarm_nanopi-r6s-kernel.bin

cat > $BINARIES_DIR/boot.script <<'BOOTSCRIPT'
part uuid ${devtype} ${devnum}:2 uuid

if test $stdout = 'serial@fe660000' ;
then serial_addr=',0xfe660000';
elif test $stdout = 'serial@feb50000' ;
then serial_addr=',0xfeb50000';
elif test $stdout = 'serial@ff130000' ;
then serial_addr=',0xff130000';
elif test $stdout = 'serial@ff1a0000' ;
then serial_addr=',0xff1a0000';
fi;

setenv bootargs "console=ttyS2,1500000 earlycon=uart8250,mmio32${serial_addr} root=PARTUUID=${uuid} rw rootwait";

load ${devtype} ${devnum}:1 ${kernel_addr_r} kernel.img

bootm ${kernel_addr_r}
BOOTSCRIPT

	mkimage -A arm -O linux -T script -C none -a 0 -e 0 \
	-d $BINARIES_DIR/boot.script \
	$BINARIES_DIR/boot.scr

	rm -f $BINARIES_DIR/boot.script

	mkdir -p $TARGET_DIR/boot
	cp $BINARIES_DIR/friendlyarm_nanopi-r6s-kernel.bin $TARGET_DIR/boot/

	$BR2_EXTERNAL_TOMATO64_PATH/board/common/post-build-fs.sh
