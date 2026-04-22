################################################################################
#
# r8125 - Realtek RTL8125 2.5GbE vendor driver
#
################################################################################

R8125_VERSION = 9.016.01
R8125_SOURCE = r8125-$(R8125_VERSION).tar.bz2
R8125_SITE = https://github.com/openwrt/rtl8125/releases/download/$(R8125_VERSION)
R8125_LICENSE = GPL-2.0-only
R8125_LICENSE_FILES = COPYING
R8125_DEPENDENCIES = linux

R8125_MAKE_FLAGS = CONFIG_ASPM=n

define R8125_BUILD_CMDS
	$(TARGET_MAKE_ENV) $(MAKE) \
		-C $(LINUX_DIR) \
		$(LINUX_MAKE_FLAGS) \
		M="$(@D)/src" \
		$(R8125_MAKE_FLAGS) \
		modules
endef

define R8125_INSTALL_TARGET_CMDS
	cd $(@D)/src && \
	$(MAKE) -C $(LINUX_DIR) M="$$PWD" modules_install \
		INSTALL_MOD_PATH=$(TARGET_DIR) \
		INSTALL_MOD_DIR=extra
endef

$(eval $(generic-package))
