################################################################################
#
# libnvram
#
################################################################################

LIBNVRAM_VERSION = 153a64ae0b8c35ee6f01c07dfaaf63129086ee67
LIBNVRAM_SITE = $(call github,tomato64,libnvram,$(LIBNVRAM_VERSION))
LIBNVRAM_INSTALL_STAGING = YES
LIBNVRAM_LICENSE = MIT

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
