################################################################################
#
# ucode
#
################################################################################

UCODE_VERSION = 85922056ef7abeace3cca3ab28bc1ac2d88e31b1
UCODE_SITE = $(call github,jow-,ucode,$(UCODE_VERSION))
UCODE_LICENSE = GPL-2.0
UCODE_DEPENDENCIES = libubox ubus libuci libnl-tiny libmd
UCODE_INSTALL_STAGING = YES

UCODE_CONF_OPTS = -DDEBUG_SUPPORT=OFF \
		  -DDIGEST_SUPPORT=ON \
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
		  -Dlibnl_tiny="$(STAGING_DIR)/usr/lib/libnl-tiny.so" \
		  -Dnl_include_dir="$(STAGING_DIR)/usr/include/libnl-tiny"

$(eval $(cmake-package))
