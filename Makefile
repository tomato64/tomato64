BUILDROOT_VERSION = 2025.11
BUILDROOT_TARBALL = ${HOME}/buildroot-src/buildroot/buildroot-$(BUILDROOT_VERSION).tar.xz
BUILDROOT_URL = https://github.com/tomato64/buildroot-release/releases/download/$(BUILDROOT_VERSION)
MEDIATEK_KERNEL_VERSION=$(shell grep "BR2_LINUX_KERNEL_CUSTOM_VERSION_VALUE" tomato64/configs/mt6000_defconfig | cut -d '"' -f2)
MEDIATEK_KERNEL_PATCH=${HOME}/buildroot-src/mediatek-kernel/00001-openwrt-mediatek-kernel-${MEDIATEK_KERNEL_VERSION}.patch
ROCKCHIP_KERNEL_VERSION=$(shell grep "BR2_LINUX_KERNEL_CUSTOM_VERSION_VALUE" tomato64/configs/r6s_defconfig | cut -d '"' -f2)
ROCKCHIP_KERNEL_PATCH=${HOME}/buildroot-src/rockchip-kernel/00001-openwrt-rockchip-kernel-${ROCKCHIP_KERNEL_VERSION}.patch

default: .configure
	make -C src/buildroot

legacy: .configure-legacy
	make -C src/buildroot

mt6000: .configure-mt6000
	make -C src/buildroot

rpi4: .configure-rpi4
	make -C src/buildroot

bpi-r3: .configure-bpi-r3
	make -C src/buildroot

bpi-r3-mini: .configure-bpi-r3-mini
	make -C src/buildroot

r6s: .configure-r6s
	make -C src/buildroot

legacy-menuconfig: .configure-legacy
	make -C src/buildroot menuconfig

mt6000-menuconfig: .configure-mt6000
	make -C src/buildroot menuconfig

rpi4-menuconfig: .configure-rpi4
	make -C src/buildroot menuconfig

bpi-r3-menuconfig: .configure-bpi-r3

	make -C src/buildroot menuconfig
bpi-r3-mini-menuconfig: .configure-bpi-r3-mini
	make -C src/buildroot menuconfig

r6s-menuconfig: .configure-r6s
	make -C src/buildroot menuconfig

distclean:
	git clean -fdxq && git reset --hard

.configure: .prepatch .patch
	make -C src/buildroot BR2_EXTERNAL=../../tomato64 tomato64_defconfig
	@touch $@

.configure-legacy: .prepatch .patch
	make -C src/buildroot BR2_EXTERNAL=../../tomato64 tomato64_legacy_defconfig
	@touch $@
	@touch .configure

.configure-mt6000: .download-mediatek-kernel .patch
	make -C src/buildroot BR2_EXTERNAL=../../tomato64 mt6000_defconfig
	@touch $@
	@touch .configure

.configure-rpi4: .patch
	make -C src/buildroot BR2_EXTERNAL=../../tomato64 rpi4_defconfig
	@touch $@
	@touch .configure

.configure-bpi-r3: .download-mediatek-kernel .patch
	make -C src/buildroot BR2_EXTERNAL=../../tomato64 bpi-r3_defconfig
	@touch $@
	@touch .configure

.configure-bpi-r3-mini: .download-mediatek-kernel .patch
	make -C src/buildroot BR2_EXTERNAL=../../tomato64 bpi-r3-mini_defconfig
	@touch $@
	@touch .configure

.configure-r6s: .download-rockchip-kernel .patch
	make -C src/buildroot BR2_EXTERNAL=../../tomato64 r6s_defconfig
	@touch $@
	@touch .configure

.patch: .extract-buildroot
	@touch .prepatch
	for patch in $(sort $(wildcard src/patches/*.patch)); do \
		patch -p1 -d src/buildroot < $$patch; \
	done
	@touch $@

.prepatch:
#	cp tomato64/board/x86_64/*.patch src/patches/
	@touch $@

.extract-buildroot: .download-buildroot
	tar xJf ${HOME}/buildroot-src/buildroot/buildroot-$(BUILDROOT_VERSION).tar.xz -C src/
	mv src/buildroot-$(BUILDROOT_VERSION) src/buildroot
	@touch $@

.download-buildroot:
	mkdir -p ${HOME}/buildroot-src/buildroot
ifeq (,$(wildcard $(BUILDROOT_TARBALL)))
	wget -O $(BUILDROOT_TARBALL) $(BUILDROOT_URL)/buildroot-$(BUILDROOT_VERSION).tar.xz
endif
	@touch $@

.download-mediatek-kernel:
	mkdir -p ${HOME}/buildroot-src/mediatek-kernel
ifeq (,$(wildcard ${MEDIATEK_KERNEL_PATCH}))
	wget -O ${MEDIATEK_KERNEL_PATCH} https://github.com/tomato64/openwrt-mediatek-kernel/releases/download/${MEDIATEK_KERNEL_VERSION}/00001-openwrt-mediatek-kernel-${MEDIATEK_KERNEL_VERSION}.patch
endif
	cp ${MEDIATEK_KERNEL_PATCH} tomato64/board/arm64/common/linux-patches-mt/
	@touch $@

.download-rockchip-kernel:
	mkdir -p ${HOME}/buildroot-src/rockchip-kernel
ifeq (,$(wildcard ${ROCKCHIP_KERNEL_PATCH}))
	wget -O ${ROCKCHIP_KERNEL_PATCH} https://github.com/tomato64/openwrt-rockchip-kernel/releases/download/${ROCKCHIP_KERNEL_VERSION}/00001-openwrt-rockchip-kernel-${ROCKCHIP_KERNEL_VERSION}.patch
endif
	mkdir -p tomato64/board/arm64/r6s/linux-patches
	cp ${ROCKCHIP_KERNEL_PATCH} tomato64/board/arm64/r6s/linux-patches/
	@touch $@

.DEFAULT:
ifneq (,$(wildcard ./.configure-legacy))
	make .configure-legacy
else
	make .configure
endif
	make -C src/buildroot $@
