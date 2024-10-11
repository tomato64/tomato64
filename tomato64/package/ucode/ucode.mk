################################################################################
#
# ucode
#
################################################################################

UCODE_VERSION = 1a8a0bcf725520820802ad433db22d8f64fbed6c
UCODE_SITE = $(call github,jow-,ucode,$(UCODE_VERSION))
UCODE_LICENSE = GPL-2.0
UCODE_DEPENDENCIES = libubox ubus libuci libnl-tiny
UCODE_INSTALL_STAGING = YES

UCODE_CONF_OPTS = -DNL80211_SUPPORT=ON
UCODE_CONF_OPTS += -DRTNL_SUPPORT=ON

$(eval $(cmake-package))
