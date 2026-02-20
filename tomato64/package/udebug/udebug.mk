################################################################################
#
# udebug
#
################################################################################

UDEBUG_VERSION = 875e1a7af6ca9d86524d18169c3a79f4a1920053
UDEBUG_SITE = https://git.openwrt.org/project/udebug.git
UDEBUG_SITE_METHOD=git
UDEBUG_DEPENDENCIES = ubus ucode
UDEBUG_INSTALL_STAGING = YES

$(eval $(cmake-package))
