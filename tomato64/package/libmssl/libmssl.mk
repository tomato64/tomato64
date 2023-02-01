################################################################################
#
# libmssl
#
################################################################################

LIBMSSL_VERSION = 1.0
LIBMSSL_SITE = $(BR2_EXTERNAL_TOMATO64_PATH)/package/libmssl/mssl
LIBMSSL_SITE_METHOD = local
LIBMSSL_INSTALL_STAGING = YES
LIBMSSL_LICENSE = tomato
LIBMSSL_DEPENDENCIES = libshared openssl

define LIBMSSL_BUILD_CMDS
	$(MAKE) $(TARGET_CONFIGURE_OPTS) -C $(@D)
endef

define LIBMSSL_INSTALL_STAGING_CMDS
	$(INSTALL) -D -m 0755 $(@D)/libmssl.so $(STAGING_DIR)/bin
	$(INSTALL) -D -m 0644 $(@D)/mssl.h  $(STAGING_DIR)/usr/include
endef

define LIBMSSL_INSTALL_TARGET_CMDS
	$(INSTALL) -D -m 0755 $(@D)/libmssl.so $(TARGET_DIR)/bin
endef

$(eval $(generic-package))
