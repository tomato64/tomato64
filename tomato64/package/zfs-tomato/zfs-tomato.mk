################################################################################
#
# zfs-tomato
#
################################################################################

ZFS_TOMATO_VERSION = 2.2.0
ZFS_TOMATO_SOURCE = zfs-$(ZFS_TOMATO_VERSION).tar.gz
ZFS_TOMATO_SITE = https://github.com/openzfs/zfs/releases/download/zfs-$(ZFS_TOMATO_VERSION)
ZFS_TOMATO_LICENSE = CDDL
ZFS_TOMATO_LICENSE_FILES = LICENSE COPYRIGHT
ZFS_TOMATO_CPE_ID_VENDOR = openzfs
ZFS_TOMATO_CPE_ID_PRODUCT = openzfs

ZFS_TOMATO_CONF_ENV += KERNEL_CC="$(TARGET_CC)"
ZFS_TOMATO_CONF_ENV += LDFLAGS="$(TARGET_LDFLAGS) -zmuldefs"

# 0001-removal-of-LegacyVersion-broke-ax_python_dev.m4.patch
ZFS_TOMATO_AUTORECONF = YES

ZFS_TOMATO_DEPENDENCIES = libaio openssl udev util-linux zlib libcurl linux

# sysvinit installs only a commented-out modules-load.d/ config file
ZFS_TOMATO_CONF_OPTS = \
	--with-linux=$(LINUX_DIR) \
	--with-linux-obj=$(LINUX_DIR) \
	--disable-rpath \
	--disable-sysvinit \
	--enable-static \
	--disable-shared

ifeq ($(BR2_PACKAGE_LIBTIRPC),y)
ZFS_TOMATO_DEPENDENCIES += libtirpc
ZFS_TOMATO_CONF_OPTS += --with-tirpc
else
ZFS_TOMATO_CONF_OPTS += --without-tirpc
endif

ifeq ($(BR2_INIT_SYSTEMD),y)
# Installs the optional systemd generators, units, and presets files.
ZFS_TOMATO_CONF_OPTS += --enable-systemd
else
ZFS_TOMATO_CONF_OPTS += --disable-systemd
endif

ifeq ($(BR2_PACKAGE_PYTHON3),y)
ZFS_TOMATO_DEPENDENCIES += python3 python-setuptools host-python-cffi host-python-packaging
ZFS_TOMATO_CONF_ENV += \
	PYTHON=$(HOST_DIR)/bin/python3 \
	PYTHON_CPPFLAGS="`$(STAGING_DIR)/usr/bin/python3-config --includes`" \
	PYTHON_LIBS="`$(STAGING_DIR)/usr/bin/python3-config --ldflags`" \
	PYTHON_EXTRA_LIBS="`$(STAGING_DIR)/usr/bin/python3-config --libs --embed`" \
	PYTHON_SITE_PKG="/usr/lib/python$(PYTHON3_VERSION_MAJOR)/site-packages"
ZFS_TOMATO_CONF_OPTS += --enable-pyzfs
else
ZFS_TOMATO_CONF_OPTS += --disable-pyzfs --without-python
endif

ifeq ($(BR2_PACKAGE_LINUX_PAM),y)
ZFS_TOMATO_DEPENDENCIES += linux-pam
ZFS_TOMATO_CONF_OPTS += --enable-pam
else
ZFS_TOMATO_CONF_OPTS += --disable-pam
endif

# Sets the environment for the `make` that will be run ZFS autotools checks.
ZFS_TOMATO_CONF_ENV += \
       ARCH=$(KERNEL_ARCH) \
       CROSS_COMPILE="$(TARGET_CROSS)"
ZFS_TOMATO_MAKE_ENV += \
       ARCH=$(KERNEL_ARCH) \
       CROSS_COMPILE="$(TARGET_CROSS)"

# ZFS userland tools are unfunctional without the Linux kernel modules.
ZFS_TOMATO_MODULE_SUBDIRS = \
	module/avl \
	module/icp \
	module/lua \
	module/nvpair \
	module/spl \
	module/unicode \
	module/zcommon \
	module/zstd \
	module/zfs

# These requirements will be validated by zfs/config/kernel-config-defined.m4
define ZFS_TOMATO_LINUX_CONFIG_FIXUPS
	$(call KCONFIG_DISABLE_OPT,CONFIG_DEBUG_LOCK_ALLOC)
	$(call KCONFIG_DISABLE_OPT,CONFIG_TRIM_UNUSED_KSYMS)
	$(call KCONFIG_ENABLE_OPT,CONFIG_CRYPTO_DEFLATE)
	$(call KCONFIG_ENABLE_OPT,CONFIG_ZLIB_DEFLATE)
	$(call KCONFIG_ENABLE_OPT,CONFIG_ZLIB_INFLATE)
endef

define ZFS_TOMATO_INSTALL_TARGET_CMDS
	cd $(ZFS_TOMATO_DIR)/module && \
	make -C $(LINUX_DIR) M="$$PWD" modules_install \
	INSTALL_MOD_PATH=$(TARGET_DIR) \
	INSTALL_MOD_DIR=extra \
	KERNELRELEASE=$(LINUX_VERSION)

	$(INSTALL) -D $(@D)/zfs $(TARGET_DIR)/usr/sbin
	$(INSTALL) -D $(@D)/zpool $(TARGET_DIR)/usr/sbin
endef

# ZFS autotools will compile the kernel modules, so there is not needed to execute
# $(eval $(kernel-module))
$(eval $(autotools-package))
