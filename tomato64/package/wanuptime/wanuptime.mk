################################################################################
#
# wanuptime
#
################################################################################

WANUPTIME_VERSION = 1.0
WANUPTIME_SITE = $(BR2_EXTERNAL_TOMATO64_PATH)/package/wanuptime/wanuptime
WANUPTIME_SITE_METHOD = local
WANUPTIME_INSTALL_STAGING = YES
WANUPTIME_LICENSE = tomato
WANUPTIME_DEPENDENCIES = libshared

define WANUPTIME_BUILD_CMDS
	$(MAKE) $(TARGET_CONFIGURE_OPTS) -C $(@D)
endef

define WANUPTIME_INSTALL_STAGING_CMDS
	$(INSTALL) -D -m 0755 $(@D)/wanuptime $(STAGING_DIR)/usr/sbin
endef

define WANUPTIME_INSTALL_TARGET_CMDS
	$(INSTALL) -D -m 0755 $(@D)/wanuptime $(TARGET_DIR)/usr/sbin
endef

$(eval $(generic-package))
