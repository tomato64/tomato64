################################################################################
#
# libnl-tiny
#
################################################################################

LIBNL_TINY_VERSION = c0df580adbd4d555ecc1962dbe88e91d75b67a4e
LIBNL_TINY_SITE = https://git.openwrt.org/project/libnl-tiny.git
LIBNL_TINY_SITE_METHOD=git
LIBNL_TINY_INSTALL_STAGING = YES

$(eval $(cmake-package))
