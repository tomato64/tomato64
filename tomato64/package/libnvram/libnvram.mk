################################################################################
#
# libnvram
#
################################################################################

LIBNVRAM_VERSION = 1.0
LIBNVRAM_SITE = $(BR2_EXTERNAL_TOMATO64_PATH)/package/libnvram/libnvram
LIBNVRAM_SITE_METHOD = local
LIBNVRAM_INSTALL_STAGING = YES
LIBNVRAM_LICENSE = tomato

define LIBNVRAM_BUILD_CMDS
	$(MAKE) $(TARGET_CONFIGURE_OPTS) -C $(@D) libnvram.so
endef

define LIBNVRAM_INSTALL_STAGING_CMDS
	$(INSTALL) -D -m 0755 $(@D)/libnvram.so $(STAGING_DIR)/usr/lib
endef

define LIBNVRAM_INSTALL_TARGET_CMDS
	$(INSTALL) -D -m 0755 $(@D)/libnvram.so $(TARGET_DIR)/usr/lib
endef

$(eval $(generic-package))
