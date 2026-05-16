################################################################################
#
# ubi-utils-openwrt
#
################################################################################

UBI_UTILS_OPENWRT_VERSION = 2.3.0
UBI_UTILS_OPENWRT_SOURCE = mtd-utils-$(UBI_UTILS_OPENWRT_VERSION).tar.bz2
UBI_UTILS_OPENWRT_SITE = https://infraroot.at/pub/mtd
UBI_UTILS_OPENWRT_LICENSE = GPL-2.0
UBI_UTILS_OPENWRT_LICENSE_FILES = COPYING
UBI_UTILS_OPENWRT_DEPENDENCIES = util-linux host-pkgconf

# OpenWrt sets PKG_FIXUP:=autoreconf; the patch set touches autotools input
UBI_UTILS_OPENWRT_AUTORECONF = YES

# Configure args verbatim from OpenWrt's package/utils/mtd-utils Makefile
UBI_UTILS_OPENWRT_CONF_OPTS = \
	--enable-tests \
	--disable-unit-tests \
	--without-crypto \
	--without-xattr \
	--without-zstd \
	--without-lzo \
	--without-zlib

# Install only the ubi-utils we need on the GL-MT3600BE
define UBI_UTILS_OPENWRT_INSTALL_TARGET_CMDS
	$(INSTALL) -D -m 0755 $(@D)/ubiattach	$(TARGET_DIR)/usr/sbin/ubiattach
	$(INSTALL) -D -m 0755 $(@D)/ubidetach	$(TARGET_DIR)/usr/sbin/ubidetach
	$(INSTALL) -D -m 0755 $(@D)/ubimkvol	$(TARGET_DIR)/usr/sbin/ubimkvol
	$(INSTALL) -D -m 0755 $(@D)/ubirmvol	$(TARGET_DIR)/usr/sbin/ubirmvol
	$(INSTALL) -D -m 0755 $(@D)/ubiupdatevol	$(TARGET_DIR)/usr/sbin/ubiupdatevol
	$(INSTALL) -D -m 0755 $(@D)/ubinfo	$(TARGET_DIR)/usr/sbin/ubinfo
	$(INSTALL) -D -m 0755 $(@D)/ubiformat	$(TARGET_DIR)/usr/sbin/ubiformat
	$(INSTALL) -D -m 0755 $(@D)/ubiblock	$(TARGET_DIR)/usr/sbin/ubiblock
endef

$(eval $(autotools-package))
