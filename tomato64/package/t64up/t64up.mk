################################################################################
#
# t64up
#
################################################################################

T64UP_VERSION = 249e5d9cc7b3a0fc578e53850e40e1246d3cba19
T64UP_SITE = $(call github,HommeOursPorc,t64up,$(T64UP_VERSION))

define T64UP_INSTALL_TARGET_CMDS
	$(INSTALL) -D -m 0755 $(@D)/t64up $(TARGET_DIR)/usr/sbin
endef

$(eval $(generic-package))
