################################################################################
#
# rstats
#
################################################################################

RSTATS_VERSION = 1.0
RSTATS_SITE = $(BR2_EXTERNAL_TOMATO64_PATH)/package/rstats/rstats
RSTATS_SITE_METHOD = local
RSTATS_INSTALL_STAGING = YES
RSTATS_LICENSE = tomato
RSTATS_DEPENDENCIES = libshared zlib

define RSTATS_BUILD_CMDS
	$(MAKE) $(TARGET_CONFIGURE_OPTS) -C $(@D)
endef

define RSTATS_INSTALL_STAGING_CMDS
	$(INSTALL) -D -m 0755 $(@D)/rstats $(STAGING_DIR)/bin
	$(INSTALL) -D -m 0755 $(@D)/rstats_migrate $(STAGING_DIR)/usr/sbin
endef

define RSTATS_INSTALL_TARGET_CMDS
	$(INSTALL) -D -m 0755 $(@D)/rstats $(TARGET_DIR)/bin
	$(INSTALL) -D -m 0755 $(@D)/rstats_migrate $(TARGET_DIR)/usr/sbin
endef

$(eval $(generic-package))
