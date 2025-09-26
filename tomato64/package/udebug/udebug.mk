################################################################################
#
# udebug
#
################################################################################

UDEBUG_VERSION = edeb4d6dc690acb476a47e6b11633b5632b08437
UDEBUG_SITE = https://git.openwrt.org/project/udebug.git
UDEBUG_SITE_METHOD=git
UDEBUG_DEPENDENCIES = ubus ucode
UDEBUG_INSTALL_STAGING = YES

$(eval $(cmake-package))
