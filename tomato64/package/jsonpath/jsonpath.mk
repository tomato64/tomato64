################################################################################
#
# jsonpath
#
################################################################################

JSONPATH_VERSION = f4fe702d0e8d9f8704b42f5d5c10950470ada231
JSONPATH_SITE = https://git.openwrt.org/project/jsonpath.git
JSONPATH_SITE_METHOD=git
JSONPATH_DEPENDENCIES = libubox json-c

define JSONPATH_INSTALL_TARGET_CMDS
	$(INSTALL) -m 0755 $(@D)/jsonpath $(TARGET_DIR)/usr/bin/jsonfilter
endef

$(eval $(cmake-package))
