################################################################################
#
# libnl-tiny
#
################################################################################

LIBNL_TINY_VERSION = 965c4bf49658342ced0bd6e7cb069571b4a1ddff
LIBNL_TINY_SITE = https://git.openwrt.org/project/libnl-tiny.git
LIBNL_TINY_SITE_METHOD=git
LIBNL_TINY_INSTALL_STAGING = YES

$(eval $(cmake-package))
