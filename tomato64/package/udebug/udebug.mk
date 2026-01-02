################################################################################
#
# udebug
#
################################################################################

UDEBUG_VERSION = 75f39cd4a8067a6f0503c2f1c83c6b1af733a6f2
UDEBUG_SITE = https://git.openwrt.org/project/udebug.git
UDEBUG_SITE_METHOD=git
UDEBUG_DEPENDENCIES = ubus ucode
UDEBUG_INSTALL_STAGING = YES

$(eval $(cmake-package))
