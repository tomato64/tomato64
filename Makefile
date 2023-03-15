BUILDROOT_VERSION = 2023.02

default: .configure
	make -C src/buildroot

distclean:
	git clean -fdxq && git reset --hard

.configure: .extract
	make -C src/buildroot BR2_EXTERNAL=../../tomato64 tomato64_defconfig
	@touch $@

.extract:
	tar xvJf src/buildroot-$(BUILDROOT_VERSION).tar.xz -C src/
	mv src/buildroot-$(BUILDROOT_VERSION) src/buildroot
	@touch $@

.DEFAULT:
	make .configure
	make -C src/buildroot $@
