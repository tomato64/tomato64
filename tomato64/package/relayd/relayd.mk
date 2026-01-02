################################################################################
#
# relayd
#
################################################################################

RELAYD_VERSION = 708a76faa7a1f2f62af83303efea0eb760d99c62
RELAYD_SITE = 	https://git.openwrt.org/project/relayd.git
RELAYD_SITE_METHOD=git
RELAYD_DEPENDENCIES = libubox
RELAYD_INSTALL_STAGING = YES

$(eval $(cmake-package))
