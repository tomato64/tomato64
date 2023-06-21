BUILDROOT_VERSION = 2023.05
PATCHES := $(wildcard src/patches/*.patch)

default: .configure
	make -C src/buildroot

distclean:
	git clean -fdxq && git reset --hard

.configure: .patch
	make -C src/buildroot BR2_EXTERNAL=../../tomato64 tomato64_defconfig
	@touch $@

.patch: .extract
	for patch in $(PATCHES); do \
		patch -p1 -d src/buildroot < $$patch; \
	done
	@touch $@

.extract:
	tar xJf src/buildroot-$(BUILDROOT_VERSION).tar.xz -C src/
	mv src/buildroot-$(BUILDROOT_VERSION) src/buildroot
	@touch $@

.DEFAULT:
	make .configure
	make -C src/buildroot $@
