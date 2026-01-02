################################################################################
#
# iwinfo
#
################################################################################

IWINFO_VERSION = 2ebef3da84e8e35e54657eb8d7e493a9a144da4d
IWINFO_SITE = https://git.openwrt.org/project/iwinfo.git
IWINFO_SITE_METHOD = git
IWINFO_DEPENDENCIES = libnl-tiny libubox ubus libuci
IWINFO_INSTALL_STAGING = YES
IWINFO_CFLAGS = $(TARGET_CFLAGS)
IWINFO_LICENSE = GPL-2

IWINFO_CFLAGS += \
        -I$(STAGING_DIR)/usr/include/libnl-tiny \
        -I$(STAGING_DIR)/usr/include \
        -D_GNU_SOURCE

define IWINFO_BUILD_CMDS
	$(MAKE) $(TARGET_CONFIGURE_OPTS) FPIC="-fPIC" CFLAGS="$(IWINFO_CFLAGS)" BACKENDS="nl80211" libiwinfo.so -C $(@D)
	$(MAKE) $(TARGET_CONFIGURE_OPTS) FPIC="-fPIC" CFLAGS="$(IWINFO_CFLAGS)" BACKENDS="nl80211" iwinfo -C $(@D)
endef

define IWINFO_INSTALL_STAGING_CMDS
	$(INSTALL) -D -m 0755 $(@D)/libiwinfo.so.0 $(STAGING_DIR)/usr/lib
	$(INSTALL) -D -m 0755 $(@D)/iwinfo $(STAGING_DIR)/usr/bin
endef

define IWINFO_INSTALL_TARGET_CMDS
	$(INSTALL) -D -m 0755 $(@D)/libiwinfo.so.0 $(TARGET_DIR)/usr/lib
	$(INSTALL) -D -m 0755 $(@D)/iwinfo $(TARGET_DIR)/usr/bin
endef

$(eval $(generic-package))
