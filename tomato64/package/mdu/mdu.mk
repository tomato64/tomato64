################################################################################
#
# mdu
#
################################################################################

MDU_VERSION = 1.0
MDU_SITE = $(BR2_EXTERNAL_TOMATO64_PATH)/package/mdu/mdu
MDU_SITE_METHOD = local
MDU_LICENSE = tomato
MDU_DEPENDENCIES = libcurl libopenssl libshared libnvram

define MDU_BUILD_CMDS
	$(MAKE) $(TARGET_CONFIGURE_OPTS) -C $(@D)
endef

define MDU_INSTALL_TARGET_CMDS
	$(INSTALL) -D -m 0755 $(@D)/mdu $(TARGET_DIR)/bin
endef

$(eval $(generic-package))
