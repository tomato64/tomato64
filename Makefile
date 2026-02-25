BUILDROOT_VERSION = 2026.02-rc2
BUILDROOT_TARBALL = ${HOME}/buildroot-src/buildroot/buildroot-$(BUILDROOT_VERSION).tar.xz
BUILDROOT_URL = https://github.com/tomato64/buildroot-release/releases/download/$(BUILDROOT_VERSION)
X86_64_KERNEL_VERSION=$(shell grep "BR2_LINUX_KERNEL_CUSTOM_VERSION_VALUE" tomato64/configs/tomato64_defconfig | cut -d '"' -f2)
X86_64_KERNEL_PATCH=${HOME}/buildroot-src/x86_64-kernel/00001-openwrt-x86_64-kernel-${X86_64_KERNEL_VERSION}.patch
MEDIATEK_KERNEL_VERSION=$(shell grep "BR2_LINUX_KERNEL_CUSTOM_VERSION_VALUE" tomato64/configs/mt6000_defconfig | cut -d '"' -f2)
MEDIATEK_KERNEL_PATCH=${HOME}/buildroot-src/mediatek-kernel/00001-openwrt-mediatek-kernel-${MEDIATEK_KERNEL_VERSION}.patch
ROCKCHIP_KERNEL_VERSION=$(shell grep "BR2_LINUX_KERNEL_CUSTOM_VERSION_VALUE" tomato64/configs/r6s_defconfig | cut -d '"' -f2)
ROCKCHIP_KERNEL_PATCH=${HOME}/buildroot-src/rockchip-kernel/00001-openwrt-rockchip-kernel-${ROCKCHIP_KERNEL_VERSION}.patch

# Default target: continue building the last configured device, or x86_64 if none
default:
	@if [ -f .target ]; then \
		$(MAKE) $$(cat .target); \
	else \
		$(MAKE) x86_64; \
	fi

# x86_64 UEFI (explicit target)
x86_64: .configure
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

menuconfig:
	@if [ -f .target ]; then \
		$(MAKE) $$(cat .target)-menuconfig; \
	else \
		$(MAKE) x86_64-menuconfig; \
	fi

x86_64-menuconfig: .configure
	make -C src/buildroot menuconfig

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

.configure: .download-x86_64-kernel .patch
	make -C src/buildroot BR2_EXTERNAL=../../tomato64 tomato64_defconfig
	@echo x86_64 > .target
	@touch $@

.configure-legacy: .download-x86_64-kernel .patch
	make -C src/buildroot BR2_EXTERNAL=../../tomato64 tomato64_legacy_defconfig
	@echo legacy > .target
	@touch $@

.configure-mt6000: .download-mediatek-kernel .patch
	make -C src/buildroot BR2_EXTERNAL=../../tomato64 mt6000_defconfig
	@echo mt6000 > .target
	@touch $@

.configure-rpi4: .patch
	make -C src/buildroot BR2_EXTERNAL=../../tomato64 rpi4_defconfig
	@echo rpi4 > .target
	@touch $@

.configure-bpi-r3: .download-mediatek-kernel .patch
	make -C src/buildroot BR2_EXTERNAL=../../tomato64 bpi-r3_defconfig
	@echo bpi-r3 > .target
	@touch $@

.configure-bpi-r3-mini: .download-mediatek-kernel .patch
	make -C src/buildroot BR2_EXTERNAL=../../tomato64 bpi-r3-mini_defconfig
	@echo bpi-r3-mini > .target
	@touch $@

.configure-r6s: .download-rockchip-kernel .patch
	make -C src/buildroot BR2_EXTERNAL=../../tomato64 r6s_defconfig
	@echo r6s > .target
	@touch $@

.patch: .extract-buildroot
	for patch in $(sort $(wildcard src/patches/*.patch)); do \
		patch -p1 -d src/buildroot < $$patch; \
	done
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

.download-x86_64-kernel:
	mkdir -p ${HOME}/buildroot-src/x86_64-kernel
ifeq (,$(wildcard ${X86_64_KERNEL_PATCH}))
	wget -O ${X86_64_KERNEL_PATCH} https://github.com/tomato64/openwrt-x86_64-kernel/releases/download/${X86_64_KERNEL_VERSION}/00001-openwrt-x86_64-kernel-${X86_64_KERNEL_VERSION}.patch
endif
	mkdir -p tomato64/board/x86_64/linux-patches
	cp ${X86_64_KERNEL_PATCH} tomato64/board/x86_64/linux-patches/
	@touch $@

.DEFAULT:
	make -C src/buildroot $@
