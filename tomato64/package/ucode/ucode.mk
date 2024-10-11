################################################################################
#
# ucode
#
################################################################################

UCODE_VERSION = b610860dd4a0591ff586dd71a50f556a0ddafced
UCODE_SITE = $(call github,jow-,ucode,$(UCODE_VERSION))
UCODE_LICENSE = GPL-2.0
UCODE_DEPENDENCIES = libubox ubus libuci libnl-tiny
UCODE_INSTALL_STAGING = YES

UCODE_CONF_OPTS = -DNL80211_SUPPORT=ON
UCODE_CONF_OPTS += -DRTNL_SUPPORT=ON

$(eval $(cmake-package))
