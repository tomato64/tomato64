BUILDROOT_VERSION = 2024.02.2
TARBALL = ${HOME}/buildroot-src/buildroot/buildroot-$(BUILDROOT_VERSION).tar.xz
URL = https://github.com/tomato64/buildroot-release/releases/download
PATCHES := $(wildcard src/patches/*.patch)

default: .configure
	make -C src/buildroot

distclean:
	git clean -fdxq && git reset --hard

.configure: .patch
	make -C src/buildroot BR2_EXTERNAL=../../tomato64 tomato64_defconfig
	@touch $@

.patch: .extract
	for patch in $(sort $(PATCHES)); do \
		patch -p1 -d src/buildroot < $$patch; \
	done
	@touch $@

.extract: .download
	tar xJf ${HOME}/buildroot-src/buildroot/buildroot-$(BUILDROOT_VERSION).tar.xz -C src/
	mv src/buildroot-$(BUILDROOT_VERSION) src/buildroot
	@touch $@

.download:
	mkdir -p ${HOME}/buildroot-src/buildroot
ifeq (,$(wildcard $(TARBALL)))
	wget -O $(TARBALL) $(URL)/$(BUILDROOT_VERSION)/buildroot-$(BUILDROOT_VERSION).tar.xz
endif
	@touch $@

.DEFAULT:
	make .configure
	make -C src/buildroot $@
