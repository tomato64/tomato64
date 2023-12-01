################################################################################
#
# libnvram
#
################################################################################

LIBNVRAM_VERSION = e33692277d475d61a03e0772efeef5c829872f34
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
