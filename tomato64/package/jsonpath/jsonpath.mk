################################################################################
#
# jsonpath
#
################################################################################

JSONPATH_VERSION = b9034210bd331749673416c6bf389cccd4e23610
JSONPATH_SITE = https://git.openwrt.org/project/jsonpath.git
JSONPATH_SITE_METHOD=git
JSONPATH_DEPENDENCIES = libubox json-c

define JSONPATH_INSTALL_TARGET_CMDS
	$(INSTALL) -m 0755 $(@D)/jsonpath $(TARGET_DIR)/usr/bin/jsonfilter
endef

$(eval $(cmake-package))
