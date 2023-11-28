################################################################################
#
# wg-quick-posix
#
################################################################################

WG_QUICK_POSIX_VERSION = 1.0
WG_QUICK_POSIX_SITE = $(BR2_EXTERNAL_TOMATO64_PATH)/package/wg-quick-posix
WG_QUICK_POSIX_SITE_METHOD = local
WG_QUICK_POSIX_LICENSE = GPL-2.0
WG_QUICK_POSIX_DEPENDENCIES = wireguard-tools


define WG_QUICK_POSIX_INSTALL_TARGET_CMDS
	$(INSTALL) -D -m 0755 $(@D)/posix.sh $(TARGET_DIR)/usr/sbin/wg-quick
endef

$(eval $(generic-package))
