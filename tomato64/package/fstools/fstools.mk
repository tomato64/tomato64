################################################################################
#
# fstools
#
################################################################################

FSTOOLS_VERSION = 12858e2878efe973049bc9fdbaf48860b9747ce3
FSTOOLS_SITE = 	https://git.openwrt.org/project/fstools.git
FSTOOLS_SITE_METHOD=git
FSTOOLS_DEPENDENCIES = libubox
FSTOOLS_INSTALL_STAGING = YES

$(eval $(cmake-package))
