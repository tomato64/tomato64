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
ROM_DEPENDENCIES = libshared

define ROM_BUILD_CMDS
	patch -d $(@D) -p1 < $(BR2_EXTERNAL_TOMATO64_PATH)/package/rom/001-remove-ldd-env.patch 
	$(MAKE) $(TARGET_CONFIGURE_OPTS) -C $(@D) all
	$(MAKE) $(TARGET_CONFIGURE_OPTS) -C $(@D) install
endef

$(eval $(generic-package))
