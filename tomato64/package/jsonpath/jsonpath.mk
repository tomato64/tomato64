################################################################################
#
# jsonpath
#
################################################################################

JSONPATH_VERSION = 594cfa86469c005972ba750614f5b3f1af84d0f6
JSONPATH_SITE = https://git.openwrt.org/project/jsonpath.git
JSONPATH_SITE_METHOD=git
JSONPATH_DEPENDENCIES = libubox json-c

define JSONPATH_INSTALL_TARGET_CMDS
	$(INSTALL) -m 0755 $(@D)/jsonpath $(TARGET_DIR)/usr/bin/jsonfilter
endef

$(eval $(cmake-package))
