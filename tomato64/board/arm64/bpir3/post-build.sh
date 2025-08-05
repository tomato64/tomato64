#!/bin/sh

set -e
set -x

	KERNEL_VERSION=$(ls $BUILD_DIR | grep "linux-headers" | cut -d- -f3)

	rm -f $BINARIES_DIR/mt7986-sdmmc-ddr4-bl2.img
	rm -f $BINARIES_DIR/mt7986-sdmmc-ddr4-bl31.bin
	rm -f $BINARIES_DIR/u-boot.fip
	rm -f $BINARIES_DIR/mt7986_bananapi_bpi-r3-sdmmc-u-boot.fip
	rm -f $BINARIES_DIR/bananapi_bpi-r3-kernel.bin
	rm -f $BINARIES_DIR/bananapi_bpi-r3-kernel.bin.new
	rm -f $BINARIES_DIR/tomato64-bananapi_bpi-r3-sysupgrade.itb
	rm -f $BINARIES_DIR/tomato64-bananapi_bpi-r3-sysupgrade.itb.its
	rm -f $BINARIES_DIR/tomato64-bananapi_bpi-r3-sysupgrade.itb.new
	rm -f $BINARIES_DIR/tomato64-bpi-r3-emmc.img
	rm -f $BINARIES_DIR/tomato64-bpi-r3-emmc.img.gz
	rm -f $BINARIES_DIR/tomato64-bpi-r3-sdcard.img
	rm -f $BINARIES_DIR/tomato64-bpi-r3-sdcard.img.gz

	FIPTOOL_DIR=$BUILD_DIR/$(ls $BUILD_DIR --ignore='host*' | grep arm-trusted-firmware)/tools/fiptool
	(cd $FIPTOOL_DIR && make HOSTCC="/usr/bin/gcc -Wl,-rpath,$HOST_DIR/lib" OPENSSL_DIR=$HOST_DIR)

	install -m0644 $BINARIES_DIR/bl2.img $BINARIES_DIR/mt7986-sdmmc-ddr4-bl2.img
	install -m0644 $BINARIES_DIR/bl31.bin $BINARIES_DIR/mt7986-sdmmc-ddr4-bl31.bin

	$FIPTOOL_DIR/fiptool create \
	--soc-fw $BINARIES_DIR/mt7986-sdmmc-ddr4-bl31.bin \
	--nt-fw $BINARIES_DIR/u-boot.bin \
	$BINARIES_DIR/u-boot.fip

	install -m0644 $BINARIES_DIR/u-boot.fip $BINARIES_DIR/mt7986_bananapi_bpi-r3-sdmmc-u-boot.fip

	cp \
	$BINARIES_DIR/Image \
	$BINARIES_DIR/bananapi_bpi-r3-kernel.bin

	gzip -f -9n -c \
	$BINARIES_DIR/bananapi_bpi-r3-kernel.bin  > $BINARIES_DIR/bananapi_bpi-r3-kernel.bin.new
	mv $BINARIES_DIR/bananapi_bpi-r3-kernel.bin.new $BINARIES_DIR/bananapi_bpi-r3-kernel.bin

	dd if=$BINARIES_DIR/bananapi_bpi-r3-kernel.bin >> $BINARIES_DIR/tomato64-bananapi_bpi-r3-sysupgrade.itb

	$BR2_EXTERNAL_TOMATO64_PATH/board/arm64/common/mkits.sh \
	-D bananapi_bpi-r3 \
	-o $BINARIES_DIR/tomato64-bananapi_bpi-r3-sysupgrade.itb.its \
	-k $BINARIES_DIR/tomato64-bananapi_bpi-r3-sysupgrade.itb \
	-C gzip \
	-d $BINARIES_DIR/mt7986a-bananapi-bpi-r3.dtb \
	-a 0x44000000 -e 0x44000000 -s 0x43f00000 \
	-O mt7986a-bananapi-bpi-r3-emmc:$BINARIES_DIR/mt7986a-bananapi-bpi-r3-emmc.dtbo \
	-O mt7986a-bananapi-bpi-r3-nand:$BINARIES_DIR/mt7986a-bananapi-bpi-r3-nand.dtbo \
	-O mt7986a-bananapi-bpi-r3-nor:$BINARIES_DIR/mt7986a-bananapi-bpi-r3-nor.dtbo \
	-O mt7986a-bananapi-bpi-r3-sd:$BINARIES_DIR/mt7986a-bananapi-bpi-r3-sd.dtbo \
	-O mt7986a-bananapi-bpi-r3-respeaker-2mics:$BINARIES_DIR/mt7986a-bananapi-bpi-r3-respeaker-2mics.dtbo \
	-c config-mt7986a-bananapi-bpi-r3 \
	-A arm64 \
	-v $KERNEL_VERSION

	mkimage -E \
	-B 0x1000 -p 0x1000 \
	-f $BINARIES_DIR/tomato64-bananapi_bpi-r3-sysupgrade.itb.its \
	$BINARIES_DIR/tomato64-bananapi_bpi-r3-sysupgrade.itb.new

	mv \
	$BINARIES_DIR/tomato64-bananapi_bpi-r3-sysupgrade.itb.new \
	$BINARIES_DIR/tomato64-bananapi_bpi-r3-sysupgrade.itb

	VERSION="$(cat $BR2_EXTERNAL_TOMATO64_PATH/version)"
	COMMIT="$(git log -n 1 --pretty=format:"%H")"

	echo '{  \"metadata_version\": \"1.1\", \"compat_version\": \"1.2\", \"supported_devices\": [\"bananapi,bpi-r3\"],  \"version\": { \"dist\": \"Tomato64\n", \"version\": \"$(echo $VERSION)\", \"revision\": \"$(echo $COMMIT)\", \"target\": \"mediatek/filogic\", \"board\": \"bananapi_bpi-r3\" } }' | fwtool -I - $BINARIES_DIR/tomato64-bananapi_bpi-r3-sysupgrade.itb

	mkdir -p $TARGET_DIR/boot
	cp $BINARIES_DIR/tomato64-bananapi_bpi-r3-sysupgrade.itb $TARGET_DIR/boot/

	$BR2_EXTERNAL_TOMATO64_PATH/board/common/post-build-fs.sh
