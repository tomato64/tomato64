################################################################################
#
# www
#
################################################################################

WWW_VERSION = 1.0
WWW_SITE = $(BR2_EXTERNAL_TOMATO64_PATH)/package/www/www
WWW_SITE_METHOD = local
WWW_INSTALL_STAGING = YES
WWW_LICENSE = tomato
WWW_DEPENDENCIES = libshared

define WWW_BUILD_CMDS
	$(MAKE) $(TARGET_CONFIGURE_OPTS) -C $(@D) install
endef

$(eval $(generic-package))
