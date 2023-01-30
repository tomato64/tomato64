################################################################################
#
# cstats
#
################################################################################

CSTATS_VERSION = 1.0
CSTATS_SITE = $(BR2_EXTERNAL_TOMATO64_PATH)/package/cstats/cstats
CSTATS_SITE_METHOD = local
CSTATS_INSTALL_STAGING = YES
CSTATS_LICENSE = tomato
CSTATS_DEPENDENCIES = libshared

define CSTATS_BUILD_CMDS
	$(MAKE) $(TARGET_CONFIGURE_OPTS) -C $(@D)
endef

define CSTATS_INSTALL_STAGING_CMDS
	$(INSTALL) -D -m 0755 $(@D)/cstats $(STAGING_DIR)/bin
endef

define CSTATS_INSTALL_TARGET_CMDS
	$(INSTALL) -D -m 0755 $(@D)/cstats $(TARGET_DIR)/bin
endef

$(eval $(generic-package))
