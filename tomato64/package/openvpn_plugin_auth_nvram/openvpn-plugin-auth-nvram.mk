################################################################################
#
# openvpn-plugin-auth-nvram
#
################################################################################

OPENVPN_PLUGIN_AUTH_NVRAM_VERSION = 1.0
OPENVPN_PLUGIN_AUTH_NVRAM_SITE = $(BR2_EXTERNAL_TOMATO64_PATH)/package/openvpn_plugin_auth_nvram/openvpn_plugin_auth_nvram
OPENVPN_PLUGIN_AUTH_NVRAM_SITE_METHOD = local
OPENVPN_PLUGIN_AUTH_NVRAM_INSTALL_STAGING = YES
OPENVPN_PLUGIN_AUTH_NVRAM_LICENSE = tomato
OPENVPN_PLUGIN_AUTH_NVRAM_DEPENDENCIES = openvpn libnvram

define OPENVPN_PLUGIN_AUTH_NVRAM_BUILD_CMDS
	$(MAKE) $(TARGET_CONFIGURE_OPTS) -C $(@D)
endef

define OPENVPN_PLUGIN_AUTH_NVRAM_INSTALL_TARGET_CMDS
	$(INSTALL) -D -m 0755 $(@D)/openvpn_plugin_auth_nvram.so $(TARGET_DIR)/lib
endef

$(eval $(generic-package))
