################################################################################
#
# fstools
#
################################################################################

FSTOOLS_VERSION = 16718b6e3c0fc7db7be6ae5848db0eae88ac8a8b
FSTOOLS_SITE = 	https://git.openwrt.org/project/fstools.git
FSTOOLS_SITE_METHOD=git
FSTOOLS_DEPENDENCIES = libubox util-linux
FSTOOLS_INSTALL_STAGING = YES

$(eval $(cmake-package))
