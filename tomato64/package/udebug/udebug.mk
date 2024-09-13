################################################################################
#
# udebug
#
################################################################################

UDEBUG_VERSION = 6d3f51f9fda706f0cf4732c762e4dbe8c21e12cf
UDEBUG_SITE = https://git.openwrt.org/project/udebug.git
UDEBUG_SITE_METHOD=git
UDEBUG_DEPENDENCIES = ubus
UDEBUG_INSTALL_STAGING = YES

$(eval $(cmake-package))
