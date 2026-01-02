################################################################################
#
# fwtool
#
################################################################################

FWTOOL_VERSION = 04cd252e4e9394ffacd51f56f1f124abc534f715
FWTOOL_SITE = $(call github,openwrt,fwtool,$(FWTOOL_VERSION))
FWTOOL_LICENSE = GPL-2.0

$(eval $(host-cmake-package))
