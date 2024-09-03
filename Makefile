BUILDROOT_VERSION = 2024.08-rc3
BUILDROOT_TARBALL = ${HOME}/buildroot-src/buildroot/buildroot-$(BUILDROOT_VERSION).tar.xz
BUILDROOT_URL = https://github.com/tomato64/buildroot-release/releases/download/$(BUILDROOT_VERSION)
MEDIATEK_KERNEL_VERSION=$(shell grep "BR2_LINUX_KERNEL_CUSTOM_VERSION_VALUE" tomato64/configs/mt6000_defconfig | cut -d '"' -f2)
MEDIATEK_KERNEL_PATCH=${HOME}/buildroot-src/mediatek-kernel/00001-openwrt-mediatek-kernel-${MEDIATEK_KERNEL_VERSION}.patch
PATCHES := $(wildcard src/patches/*.patch)

default: .configure
	make -C src/buildroot

legacy: .configure-legacy
	make -C src/buildroot

mt6000: .configure-mt6000
	make -C src/buildroot

legacy-menuconfig: .configure-legacy
	make -C src/buildroot menuconfig

mt6000-menuconfig: .configure-mt6000
	make -C src/buildroot menuconfig

distclean:
	git clean -fdxq && git reset --hard

.configure: .patch
	make -C src/buildroot BR2_EXTERNAL=../../tomato64 tomato64_defconfig
	@touch $@

.configure-legacy: .patch
	make -C src/buildroot BR2_EXTERNAL=../../tomato64 tomato64_legacy_defconfig
	@touch $@
	@touch .configure

.configure-mt6000: .download-mediatek-kernel .patch
	make -C src/buildroot BR2_EXTERNAL=../../tomato64 mt6000_defconfig
	@touch $@
	@touch .configure

.patch: .extract-buildroot
	for patch in $(sort $(PATCHES)); do \
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
	cp ${MEDIATEK_KERNEL_PATCH} tomato64/board/mt6000/linux-patches/
	@touch $@

.DEFAULT:
ifneq (,$(wildcard ./.configure-legacy))
	make .configure-legacy
else
	make .configure
endif
	make -C src/buildroot $@
