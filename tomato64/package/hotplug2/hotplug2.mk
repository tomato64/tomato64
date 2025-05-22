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
	patch -d $(@D) -p1 < $(BR2_EXTERNAL_TOMATO64_PATH)/package/hotplug2/0001-hotplug2-run-wifi-config-on-ieee80211-events.patch
	$(if $(BR2_PACKAGE_PLATFORM_ARM64),patch -d $(@D) -p1 < $(BR2_EXTERNAL_TOMATO64_PATH)/package/hotplug2/0002-hotplug2-run-packet-steering-on-net-events.patch)
	$(MAKE) $(TARGET_CONFIGURE_OPTS) -C $(@D)
endef

define HOTPLUG2_INSTALL_STAGING_CMDS
	$(INSTALL) -D -m 0755 $(@D)/hotplug2 $(STAGING_DIR)/sbin
endef

define HOTPLUG2_INSTALL_TARGET_CMDS
	$(INSTALL) -D -m 0755 $(@D)/hotplug2 $(TARGET_DIR)/sbin
	$(INSTALL) -D -m 0644 $(@D)/examples/hotplug2.rules-2.6kernel $(BR2_EXTERNAL_TOMATO64_PATH)/package/rom/rom/rom/etc/hotplug2.rules
endef

$(eval $(generic-package))
