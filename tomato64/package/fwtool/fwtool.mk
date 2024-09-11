################################################################################
#
# fwtool
#
################################################################################

FWTOOL_VERSION = 8f7fe925ca205c8e8e2d0d1b16218c1e148d5173
FWTOOL_SITE = $(call github,openwrt,fwtool,$(FWTOOL_VERSION))
FWTOOL_LICENSE = GPL-2.0

$(eval $(host-cmake-package))
