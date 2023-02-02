################################################################################
#
# hotplug2
#
################################################################################

HOTPLUG2_VERSION = 1.0
HOTPLUG2_SITE = $(BR2_EXTERNAL_TOMATO64_PATH)/package/hotplug2/hotplug2
HOTPLUG2_SITE_METHOD = local
HOTPLUG2_INSTALL_STAGING = YES
HOTPLUG2_LICENSE = tomato

define HOTPLUG2_BUILD_CMDS
	$(MAKE) $(TARGET_CONFIGURE_OPTS) -C $(@D)
endef

define HOTPLUG2_INSTALL_STAGING_CMDS
	$(INSTALL) -D -m 0755 $(@D)/hotplug2 $(STAGING_DIR)/sbin
endef

define HOTPLUG2_INSTALL_TARGET_CMDS
	$(INSTALL) -D -m 0755 $(@D)/hotplug2 $(TARGET_DIR)/sbin
endef

$(eval $(generic-package))
