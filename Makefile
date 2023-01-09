default: .configure
	make -C src/buildroot

clean: .configure
	make -C src/buildroot clean

distclean:
	git clean -fdxq && git reset --hard

menuconfig: .configure
	make -C src/buildroot menuconfig

savedefconfig: .configure
	make -C src/buildroot savedefconfig

linux-menuconfig: .configure
	make -C src/buildroot linux-menuconfig

linux-update-defconfig: .configure
	make -C src/buildroot linux-update-defconfig

.configure: .extract
	make -C src/buildroot BR2_EXTERNAL=../../tomato64 tomato64_defconfig
	@touch $@

.extract:
	tar xvJf src/buildroot-git.tar.xz -C src/
	mv src/buildroot-git src/buildroot
	@touch $@
