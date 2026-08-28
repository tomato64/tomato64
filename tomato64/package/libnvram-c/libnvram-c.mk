################################################################################
#
# libnvram-c
#
# The original firmadyne-derived C implementation. One of the providers of the
# libnvram virtual package; see package/libnvram/Config.in for the choice.
#
################################################################################

LIBNVRAM_C_VERSION = e33692277d475d61a03e0772efeef5c829872f34
LIBNVRAM_C_SITE = $(call github,tomato64,libnvram,$(LIBNVRAM_C_VERSION))

LIBNVRAM_C_SOURCE = libnvram-$(LIBNVRAM_C_VERSION).tar.gz

LIBNVRAM_C_INSTALL_STAGING = YES
LIBNVRAM_C_LICENSE = MIT
LIBNVRAM_C_PROVIDES = libnvram

define LIBNVRAM_C_BUILD_CMDS
	$(MAKE) $(TARGET_CONFIGURE_OPTS) -C $(@D) libnvram.so
endef

define LIBNVRAM_C_INSTALL_STAGING_CMDS
	$(INSTALL) -D -m 0755 $(@D)/libnvram.so $(STAGING_DIR)/usr/lib
endef

define LIBNVRAM_C_INSTALL_TARGET_CMDS
	$(INSTALL) -D -m 0755 $(@D)/libnvram.so $(TARGET_DIR)/usr/lib
endef

$(eval $(generic-package))
