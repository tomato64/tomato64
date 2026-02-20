################################################################################
#
# fstools
#
################################################################################

FSTOOLS_VERSION = 8d377aa627be1ae538f8cdeb3295c0c39b9b1d90
FSTOOLS_SITE = 	https://git.openwrt.org/project/fstools.git
FSTOOLS_SITE_METHOD=git
FSTOOLS_DEPENDENCIES = libubox util-linux
FSTOOLS_INSTALL_STAGING = YES

$(eval $(cmake-package))
