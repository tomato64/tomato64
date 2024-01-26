################################################################################
#
# sd-idle
#
################################################################################

SD_IDLE_VERSION = 1.0
SD_IDLE_SITE = $(BR2_EXTERNAL_TOMATO64_PATH)/package/sd-idle/sd-idle
SD_IDLE_SITE_METHOD = local
SD_IDLE_INSTALL_STAGING = YES
SD_IDLE_LICENSE = GPL-3.0+

define SD_IDLE_BUILD_CMDS
	$(MAKE) $(TARGET_CONFIGURE_OPTS) -C $(@D)
endef

define SD_IDLE_INSTALL_STAGING_CMDS
	$(INSTALL) -D -m 0755 $(@D)/sd-idle $(STAGING_DIR)/bin
endef

define SD_IDLE_INSTALL_TARGET_CMDS
	$(INSTALL) -D -m 0755 $(@D)/sd-idle $(TARGET_DIR)/bin
endef

$(eval $(generic-package))
