################################################################################
#
# libnl-tiny
#
################################################################################

LIBNL_TINY_VERSION = 40493a655d8caa2ccf5206dde1e733abe2920432
LIBNL_TINY_SITE = https://git.openwrt.org/project/libnl-tiny.git
LIBNL_TINY_SITE_METHOD=git
LIBNL_TINY_INSTALL_STAGING = YES

$(eval $(cmake-package))
