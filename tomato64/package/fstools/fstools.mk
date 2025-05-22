################################################################################
#
# fstools
#
################################################################################

FSTOOLS_VERSION = 408c2cc48e6694446c89da7f8121b399063e1067
FSTOOLS_SITE = 	https://git.openwrt.org/project/fstools.git
FSTOOLS_SITE_METHOD=git
FSTOOLS_DEPENDENCIES = libubox
FSTOOLS_INSTALL_STAGING = YES

$(eval $(cmake-package))
