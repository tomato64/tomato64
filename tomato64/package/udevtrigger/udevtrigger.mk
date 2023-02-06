################################################################################
#
# udevtrigger
#
################################################################################

UDEVTRIGGER_VERSION = 1.0
UDEVTRIGGER_SITE = $(BR2_EXTERNAL_TOMATO64_PATH)/package/udevtrigger/udevtrigger
UDEVTRIGGER_SITE_METHOD = local
UDEVTRIGGER_INSTALL_STAGING = YES
UDEVTRIGGER_LICENSE = tomato

define UDEVTRIGGER_BUILD_CMDS
	$(MAKE) $(TARGET_CONFIGURE_OPTS) -C $(@D)
endef

define UDEVTRIGGER_INSTALL_STAGING_CMDS
	$(INSTALL) -D -m 0500 $(@D)/udevtrigger $(STAGING_DIR)/sbin
endef

define UDEVTRIGGER_INSTALL_TARGET_CMDS
	$(INSTALL) -D -m 0500 $(@D)/udevtrigger $(TARGET_DIR)/sbin
endef

$(eval $(generic-package))
