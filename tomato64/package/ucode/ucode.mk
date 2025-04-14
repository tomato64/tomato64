################################################################################
#
# ucode
#
################################################################################

UCODE_VERSION = a8a11aea0c093d669bb3c45f604dab3c291c8d25
UCODE_SITE = $(call github,jow-,ucode,$(UCODE_VERSION))
UCODE_LICENSE = GPL-2.0
UCODE_DEPENDENCIES = libubox ubus libuci libnl-tiny
UCODE_INSTALL_STAGING = YES

UCODE_CONF_OPTS = -DDEBUG_SUPPORT=OFF \
		  -DFS_SUPPORT=ON \
		  -DLOG_SUPPORT=OFF \
		  -DMATH_SUPPORT=ON \
		  -DNL80211_SUPPORT=ON \
		  -DRESOLV_SUPPORT=OFF \
		  -DRTNL_SUPPORT=ON \
		  -DSOCKET_SUPPORT=OFF \
		  -DSTRUCT_SUPPORT=OFF \
		  -DUBUS_SUPPORT=ON \
		  -DUCI_SUPPORT=ON \
		  -DULOOP_SUPPORT=ON \
		  -Dlibnl_tiny="$(STAGING_DIR)/usr/lib/libnl-tiny.so"

$(eval $(cmake-package))
