################################################################################
#
# relayd
#
################################################################################

RELAYD_VERSION = f646ba40489371e69f624f2dee2fc4e19ceec00e
RELAYD_SITE = 	https://git.openwrt.org/project/relayd.git
RELAYD_SITE_METHOD=git
RELAYD_DEPENDENCIES = libubox
RELAYD_INSTALL_STAGING = YES

$(eval $(cmake-package))
