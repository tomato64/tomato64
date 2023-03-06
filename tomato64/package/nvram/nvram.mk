################################################################################
#
# nvram
#
################################################################################

NVRAM_VERSION = 1.0
NVRAM_SITE = $(BR2_EXTERNAL_TOMATO64_PATH)/package/nvram/nvram
NVRAM_SITE_METHOD = local
NVRAM_INSTALL_STAGING = YES
NVRAM_LICENSE = tomato
NVRAM_DEPENDENCIES = libnvram

define NVRAM_BUILD_CMDS
	$(MAKE) $(TARGET_CONFIGURE_OPTS) -C $(@D)
endef

define NVRAM_INSTALL_STAGING_CMDS
	$(INSTALL) -D -m 0755 $(@D)/nvram $(STAGING_DIR)/bin
endef

define NVRAM_INSTALL_TARGET_CMDS
	$(INSTALL) -D -m 0755 $(@D)/nvram $(TARGET_DIR)/bin
endef

$(eval $(generic-package))
