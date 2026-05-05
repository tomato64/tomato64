################################################################################
#
# mtd-openwrt
#
################################################################################

MTD_OPENWRT_VERSION = v25.12.3
MTD_OPENWRT_SITE = https://github.com/openwrt/openwrt.git
MTD_OPENWRT_SITE_METHOD = git
MTD_OPENWRT_LICENSE = GPL-2.0+
MTD_OPENWRT_DEPENDENCIES = libubox
MTD_OPENWRT_SUBDIR = package/system/mtd/src

define MTD_OPENWRT_BUILD_CMDS
	$(TARGET_MAKE_ENV) $(MAKE) -C $(@D)/package/system/mtd/src \
		CC="$(TARGET_CC)" \
		CFLAGS="$(TARGET_CFLAGS) -Dtarget_bcm53xx=1 -Wall" \
		LDFLAGS="$(TARGET_LDFLAGS) -lubox" \
		TARGET="bcm53xx"
endef

define MTD_OPENWRT_INSTALL_TARGET_CMDS
	$(INSTALL) -D -m 0755 $(@D)/package/system/mtd/src/mtd $(TARGET_DIR)/sbin/mtd
endef

$(eval $(generic-package))
