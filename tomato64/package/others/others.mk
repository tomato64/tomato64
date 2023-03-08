################################################################################
#
# others
#
################################################################################

OTHERS_VERSION = 1.0
OTHERS_SITE = $(BR2_EXTERNAL_TOMATO64_PATH)/package/others/others
OTHERS_SITE_METHOD = local
OTHERS_INSTALL_STAGING = YES
OTHERS_LICENSE = tomato

define OTHERS_BUILD_CMDS
	$(MAKE) $(TARGET_CONFIGURE_OPTS) -C $(@D) install
	$(MAKE) $(TARGET_CONFIGURE_OPTS) -C $(@D) all
endef

$(eval $(generic-package))
