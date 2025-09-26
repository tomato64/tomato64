################################################################################
#
# jsonpath
#
################################################################################

JSONPATH_VERSION = 8a86fb78235b5d7925b762b7b0934517890cc034
JSONPATH_SITE = https://git.openwrt.org/project/jsonpath.git
JSONPATH_SITE_METHOD=git
JSONPATH_DEPENDENCIES = libubox json-c

define JSONPATH_INSTALL_TARGET_CMDS
	$(INSTALL) -m 0755 $(@D)/jsonpath $(TARGET_DIR)/usr/bin/jsonfilter
endef

$(eval $(cmake-package))
