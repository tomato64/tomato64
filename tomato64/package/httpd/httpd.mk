################################################################################
#
# httpd
#
################################################################################

HTTPD_VERSION = 1.0
HTTPD_SITE = $(BR2_EXTERNAL_TOMATO64_PATH)/package/httpd/httpd
HTTPD_SITE_METHOD = local
HTTPD_INSTALL_STAGING = YES
HTTPD_LICENSE = tomato
HTTPD_DEPENDENCIES = libnvram libshared libmssl openssl

define HTTPD_BUILD_CMDS
	$(MAKE) $(TARGET_CONFIGURE_OPTS) -C $(@D)
endef

define HTTPD_INSTALL_STAGING_CMDS
	$(INSTALL) -D -m 0755 $(@D)/httpd $(STAGING_DIR)/sbin
endef

define HTTPD_INSTALL_TARGET_CMDS
	$(INSTALL) -D -m 0755 $(@D)/httpd $(TARGET_DIR)/sbin
endef

$(eval $(generic-package))
