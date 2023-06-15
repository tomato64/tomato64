################################################################################
#
# comgt-tomato
#
################################################################################

COMGT_TOMATO_VERSION = 2dc25f00dd1af7e790f587e2e123ac61d64ebf71
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
