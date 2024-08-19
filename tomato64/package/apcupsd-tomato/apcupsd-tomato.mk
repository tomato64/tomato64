################################################################################
#
# apcupsd-tomato
#
################################################################################

APCUPSD_TOMATO_VERSION = cabab8ff548f96ff4b17c4e4f019af9622094c6b
APCUPSD_TOMATO_SITE = $(call github,tomato64,apcupsd,$(APCUPSD_TOMATO_VERSION))
APCUPSD_TOMATO_LICENSE = GPL-2.0
APCUPSD_TOMATO_LICENSE_FILES = COPYING
APCUPSD_TOMATO_CPE_ID_VENDOR = apcupsd
APCUPSD_TOMATO_SELINUX_MODULES = apache apcupsd
APCUPSD_TOMATO_CONF_OPTS = --disable-test

APCUPSD_TOMATO_DEPENDENCIES += host-autoconf-269 libusb libusb-compat www

APCUPSD_TOMATO_CONF_ENV += ac_cv_path_usbcfg=$(STAGING_DIR)/usr/bin/libusb-config

define APCUPSD_TOMATO_CONFIGURE_FIXUP
	touch $(@D)/autoconf/variables.mak && \
	PATH=$(HOST_AUTOCONF_269_DIR)/install/bin:$(PATH) $(MAKE1) -C $(@D) configure
endef

APCUPSD_TOMATO_PRE_CONFIGURE_HOOKS += APCUPSD_TOMATO_CONFIGURE_FIXUP

APCUPSD_TOMATO_CONF_OPTS += --sysconfdir=/etc/apcupsd --with-cgi-bin=/www/apcupsd \
			    --enable-usb --enable-cgi --disable-lgd --enable-net \
			    --disable-dumb --without-x --with-serial-dev= \
			    --enable-pcnet --enable-snmp

define APCUPSD_TOMATO_BUILD_CMDS
	$(TARGET_MAKE_ENV) $(MAKE) LD="$(TARGET_CXX)" -C $(@D)
endef

define APCUPSD_TOMATO_INSTALL_TARGET_CMDS
	$(TARGET_MAKE_ENV) $(MAKE) -C $(@D) DESTDIR=$(TARGET_DIR) install

#	rm -rf $(TARGET_DIR)/www/apcupsd/ups*.cgi
	rm -rf $(TARGET_DIR)/rom/etc/apcupsd
	mkdir -p $(TARGET_DIR)/rom/etc/apcupsd
	cp -f $(TARGET_DIR)/etc/apcupsd/* $(TARGET_DIR)/rom/etc/apcupsd/
	mkdir -p $(TARGET_DIR)/usr/bin
	cd $(TARGET_DIR)/usr/bin && ln -sf ../../bin/hostname hostname
endef

$(eval $(autotools-package))
