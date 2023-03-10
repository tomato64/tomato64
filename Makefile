default: .configure
	make -C src/buildroot

distclean:
	git clean -fdxq && git reset --hard

.configure: .extract
	make -C src/buildroot BR2_EXTERNAL=../../tomato64 tomato64_defconfig
	@touch $@

.extract:
	tar xvJf src/buildroot-git.tar.xz -C src/
	mv src/buildroot-git src/buildroot
	@touch $@

.DEFAULT:
	make .configure
	make -C src/buildroot $@
