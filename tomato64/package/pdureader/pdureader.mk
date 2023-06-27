################################################################################
#
# pdureader
#
################################################################################

PDUREADER_VERSION = 1.0
PDUREADER_SITE = $(BR2_EXTERNAL_TOMATO64_PATH)/package/pdureader/pdureader
PDUREADER_SITE_METHOD = local
PDUREADER_LICENSE = tomato

define PDUREADER_BUILD_CMDS
	$(MAKE) $(TARGET_CONFIGURE_OPTS) -C $(@D)
endef

define PDUREADER_INSTALL_TARGET_CMDS
	$(INSTALL) -D -m 0755 $(@D)/pdureader $(TARGET_DIR)/bin
endef

$(eval $(generic-package))
