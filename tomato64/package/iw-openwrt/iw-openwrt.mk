################################################################################
#
# iw-openwrt
#
################################################################################

IW_OPENWRT_VERSION = 6.17
IW_OPENWRT_SOURCE = iw-$(IW_OPENWRT_VERSION).tar.xz
IW_OPENWRT_SITE = $(BR2_KERNEL_MIRROR)/software/network/iw
IW_OPENWRT_LICENSE = ISC
IW_OPENWRT_LICENSE_FILES = COPYING
IW_OPENWRT_CPE_ID_VENDOR = kernel
IW_OPENWRT_DEPENDENCIES = host-pkgconf libnl-tiny
IW_OPENWRT_MAKE_ENV = \
	$(TARGET_MAKE_ENV) \
	PKG_CONFIG="$(HOST_DIR)/bin/pkg-config" \
	NO_PKG_CONFIG=1 \
	NL1FOUND="" \
	NL2FOUND=Y \
	IW_FULL=1 \
	NLLIBNAME="libnl-tiny" \
	LIBS="-lm -lnl-tiny"

IW_OPENWRT_TARGET_CPPFLAGS = \
	$(TARGET_CPPFLAGS) \
	-I$(STAGING_DIR)/usr/include/libnl-tiny \
	-DCONFIG_LIBNL20 \
	-D_GNU_SOURCE \
	-DIW_FULL

define IW_OPENWRT_BUILD_CMDS
	$(IW_OPENWRT_MAKE_ENV) $(TARGET_CONFIGURE_OPTS) $(MAKE) CPPFLAGS="$(IW_OPENWRT_TARGET_CPPFLAGS)" -C $(@D)
endef

define IW_OPENWRT_INSTALL_TARGET_CMDS
	$(IW_OPENWRT_MAKE_ENV) $(MAKE) -C $(@D) DESTDIR=$(TARGET_DIR) install
endef

$(eval $(generic-package))
