################################################################################
#
# rom
#
################################################################################

ROM_VERSION = 1.0
ROM_SITE = $(BR2_EXTERNAL_TOMATO64_PATH)/package/rom/rom
ROM_SITE_METHOD = local
ROM_INSTALL_STAGING = YES
ROM_LICENSE = tomato

define ROM_BUILD_CMDS
	$(MAKE) $(TARGET_CONFIGURE_OPTS) -C $(@D) install
	$(MAKE) $(TARGET_CONFIGURE_OPTS) -C $(@D) all
endef

$(eval $(generic-package))
