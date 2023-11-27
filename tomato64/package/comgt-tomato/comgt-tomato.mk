################################################################################
#
# comgt-tomato
#
################################################################################

COMGT_TOMATO_VERSION = dcf6c541506d4aa4e07b5cb3d6cc86e1d828d2f5
COMGT_TOMATO_SITE = $(call github,tomato64,comgt,$(COMGT_TOMATO_VERSION))
COMGT_TOMATO_LICENSE = GPL-2.0+

define COMGT_TOMATO_BUILD_CMDS
	$(MAKE) $(TARGET_CONFIGURE_OPTS) -C $(@D)
endef

define COMGT_TOMATO_INSTALL_TARGET_CMDS
	$(INSTALL) -D -m 0755 $(@D)/comgt $(TARGET_DIR)/usr/sbin
	ln -sf comgt $(TARGET_DIR)/usr/sbin/gcom
endef

$(eval $(generic-package))
