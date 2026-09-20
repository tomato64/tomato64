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

# The socket module is built only where something needs it. On the BE14000 the
# front-panel UI reads the httpd JSON API over loopback, and ucode has no other
# way to speak HTTP: fs.popen("curl ...") would be a process per poll, and
# ucode-mod-curl would be another package to carry. Everywhere else it stays
# off, so no other target grows the module for nothing.
ifeq ($(BR2_PACKAGE_PLATFORM_BE14000),y)
UCODE_SOCKET_SUPPORT = ON
else
UCODE_SOCKET_SUPPORT = OFF
endif

UCODE_CONF_OPTS = -DDEBUG_SUPPORT=OFF \
		  -DDIGEST_SUPPORT=ON \
		  -DFS_SUPPORT=ON \
		  -DLOG_SUPPORT=OFF \
		  -DMATH_SUPPORT=ON \
		  -DNL80211_SUPPORT=ON \
		  -DRESOLV_SUPPORT=OFF \
		  -DRTNL_SUPPORT=ON \
		  -DSOCKET_SUPPORT=$(UCODE_SOCKET_SUPPORT) \
		  -DSTRUCT_SUPPORT=OFF \
		  -DUBUS_SUPPORT=ON \
		  -DUCI_SUPPORT=ON \
		  -DULOOP_SUPPORT=ON \
		  -Dlibnl_tiny="$(STAGING_DIR)/usr/lib/libnl-tiny.so" \
		  -Dnl_include_dir="$(STAGING_DIR)/usr/include/libnl-tiny"

$(eval $(cmake-package))
