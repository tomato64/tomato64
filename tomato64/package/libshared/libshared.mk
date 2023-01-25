################################################################################
#
# libshared
#
################################################################################

LIBSHARED_VERSION = 1.0
LIBSHARED_SITE = $(BR2_EXTERNAL_TOMATO64_PATH)/package/libshared/shared
LIBSHARED_SITE_METHOD = local
LIBSHARED_INSTALL_STAGING = YES
LIBSHARED_LICENSE = tomato
LIBSHARED_DEPENDENCIES = libnvram busybox

define LIBSHARED_BUILD_CMDS
	$(MAKE) $(TARGET_CONFIGURE_OPTS) -C $(@D) libshared.so
endef

define LIBSHARED_INSTALL_STAGING_CMDS
	$(INSTALL) -D -m 0755 $(@D)/libshared.so $(STAGING_DIR)/usr/lib
endef

define LIBSHARED_INSTALL_TARGET_CMDS
	$(INSTALL) -D -m 0755 $(@D)/libshared.so $(TARGET_DIR)/usr/lib
endef

$(eval $(generic-package))
