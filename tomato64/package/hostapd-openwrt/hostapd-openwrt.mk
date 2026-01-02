################################################################################
#
# hostapd-openwrt
#
################################################################################

HOSTAPD_OPENWRT_VERSION = ca266cc24d8705eb1a2a0857ad326e48b1408b20
HOSTAPD_OPENWRT_SITE = https://w1.fi/hostap.git
HOSTAPD_OPENWRT_SITE_METHOD=git
HOSTAPD_OPENWRT_SUBDIR = hostapd
HOSTAPD_OPENWRT_CONFIG = $(HOSTAPD_OPENWRT_DIR)/$(HOSTAPD_OPENWRT_SUBDIR)/.config
HOSTAPD_OPENWRT_DEPENDENCIES = host-pkgconf libubox ucode udebug
HOSTAPD_OPENWRT_LICENSE = BSD-3-Clause
HOSTAPD_OPENWRT_LICENSE_FILES = README

HOSTAPD_OPENWRT_CPE_ID_VENDOR = w1.fi
HOSTAPD_OPENWRT_SELINUX_MODULES = hostapd

# Base CFLAGS using Buildroot's TARGET_CFLAGS (contains architecture-appropriate flags)
HOSTAPD_OPENWRT_CFLAGS = $(TARGET_CFLAGS) -Wformat -Werror=format-security -DPIC -fPIC -fstack-protector -D_FORTIFY_SOURCE=1
HOSTAPD_OPENWRT_LDFLAGS = $(TARGET_LDFLAGS) -Wl,-z,now -Wl,-z,relro

# Include paths and defines for hostapd build
HOSTAPD_OPENWRT_INCLUDES = -I$(STAGING_DIR)/usr/include/libnl-tiny -DCONFIG_LIBNL20 -D_GNU_SOURCE -DCONFIG_MSG_MIN_PRIORITY=3

define HOSTAPD_OPENWRT_PATCHES
	cp -fpR $(BR2_EXTERNAL_TOMATO64_PATH)/package/hostapd-openwrt/src/. $(@D)
endef
HOSTAPD_OPENWRT_PRE_PATCH_HOOKS += HOSTAPD_OPENWRT_PATCHES

define HOSTAPD_OPENWRT_COPY_CONFIG
	cp -fpR $(BR2_EXTERNAL_TOMATO64_PATH)/package/hostapd-openwrt/files/hostapd-full.config $(@D)/hostapd/.config
	cp -fpR $(BR2_EXTERNAL_TOMATO64_PATH)/package/hostapd-openwrt/files/wpa_supplicant-full.config $(@D)/wpa_supplicant/.config
endef
HOSTAPD_OPENWRT_POST_PATCH_HOOKS += HOSTAPD_OPENWRT_COPY_CONFIG

HOSTAPD_OPENWRT_DEPENDENCIES += libopenssl
HOSTAPD_OPENWRT_LIBS += `$(PKG_CONFIG_HOST_BINARY) --libs openssl`

HOSTAPD_OPENWRT_DEPENDENCIES += libnl-tiny

# Common make options
HOSTAPD_OPENWRT_MAKE_OPTS = \
	AR="$(TARGET_AR)" \
	AS="$(TARGET_CC) -c $(HOSTAPD_OPENWRT_CFLAGS)" \
	LD="$(TARGET_LD)" \
	NM="$(TARGET_NM)" \
	CC="$(TARGET_CC)" \
	GCC="$(TARGET_CC)" \
	CXX="$(TARGET_CXX)" \
	RANLIB="$(TARGET_RANLIB)" \
	STRIP="$(TARGET_STRIP)" \
	OBJCOPY="$(TARGET_OBJCOPY)" \
	OBJDUMP="$(TARGET_OBJDUMP)" \
	SIZE="$(TARGET_CROSS)size" \
	CONFIG_ACS=y \
	CONFIG_DRIVER_NL80211=y \
	CONFIG_IEEE80211AC=y \
	CONFIG_IEEE80211AX=y \
	CONFIG_IEEE80211BE=y \
	CONFIG_MBO=y \
	CONFIG_UCODE=y \
	CONFIG_APUP=y \
	CONFIG_OCV=y \
	CONFIG_TLS=openssl \
	CONFIG_SAE=y \
	CONFIG_OWE=y \
	CONFIG_SUITEB192=y \
	CONFIG_AP=y \
	CONFIG_MESH=y \
	CONFIG_EAP_PWD=y \
	CONFIG_DPP=y \
	CONFIG_DPP2=y \
	LIBS="-L$(TARGET_DIR)/usr/lib -fuse-ld=bfd -Wl,--gc-sections -flto=auto -fuse-linker-plugin -DPIC -fPIC -znow -zrelro -lcrypto -lssl -lubox -lubus -lblobmsg_json -lucode -lm -lnl-tiny -ludebug" \
	LIBS_c="$(TARGET_LDFLAGS)" \
	BCHECK= \
	V=1 \
	MULTICALL=1

define HOSTAPD_OPENWRT_CONFIGURE_CMDS
	echo ` \
	CFLAGS="$(HOSTAPD_OPENWRT_INCLUDES) -I$(@D)/src/crypto $(HOSTAPD_OPENWRT_CFLAGS)" \
	$(MAKE) --no-print-directory \
	-C $(@D)/hostapd \
	$(HOSTAPD_OPENWRT_MAKE_OPTS) \
	-s \
	dump_cflags; \
	CFLAGS="$(HOSTAPD_OPENWRT_INCLUDES) -I$(@D)/src/crypto $(HOSTAPD_OPENWRT_CFLAGS)" \
	$(MAKE) --no-print-directory \
	-C $(@D)/wpa_supplicant \
	$(HOSTAPD_OPENWRT_MAKE_OPTS) \
	-s \
	dump_cflags | sed -e 's,-n ,,g' -e 's^$(TARGET_CFLAGS)^^' ` > $(@D)/.cflags

	sed -i 's/"/\\"/g' $(@D)/.cflags
endef

define HOSTAPD_OPENWRT_BUILD_CMDS
	CFLAGS="$(HOSTAPD_OPENWRT_INCLUDES) -I$(@D)/src/crypto $(HOSTAPD_OPENWRT_CFLAGS)" \
	$(MAKE) \
	-C $(@D)/hostapd \
	$(HOSTAPD_OPENWRT_MAKE_OPTS) \
	CFLAGS='$(shell cat $(@D)/.cflags)' \
	hostapd_cli \
	hostapd_multi.a

	CFLAGS="$(HOSTAPD_OPENWRT_INCLUDES) -I$(@D)/src/crypto $(HOSTAPD_OPENWRT_CFLAGS)" \
	$(MAKE) \
	-C $(@D)/wpa_supplicant \
	$(HOSTAPD_OPENWRT_MAKE_OPTS) \
	CFLAGS='$(shell cat $(@D)/.cflags)' \
	wpa_cli \
	wpa_supplicant_multi.a

	$(TARGET_CC) \
	-o $(@D)/wpad \
	$(HOSTAPD_OPENWRT_CFLAGS) \
	$(BR2_EXTERNAL_TOMATO64_PATH)/package/hostapd-openwrt/files/multicall.c \
	$(@D)/hostapd/hostapd_multi.a \
	$(@D)/wpa_supplicant/wpa_supplicant_multi.a \
	-L$(TARGET_DIR)/usr/lib \
	-fuse-ld=bfd -Wl,--gc-sections -flto=auto -fuse-linker-plugin -DPIC -fPIC -znow -zrelro \
	-lcrypto -lssl -lubox -lubus -lblobmsg_json -lucode -lm -lnl-tiny -ludebug
endef

define HOSTAPD_OPENWRT_INSTALL_TARGET_CMDS

	$(INSTALL) -m 0755 $(@D)/wpad $(TARGET_DIR)/usr/sbin/
	ln -sf wpad $(TARGET_DIR)/usr/sbin/hostapd
	ln -sf wpad $(TARGET_DIR)/usr/sbin/wpa_supplicant
	$(INSTALL) -m 0755 $(@D)/hostapd/hostapd_cli $(TARGET_DIR)/usr/sbin/

	$(INSTALL) -d $(TARGET_DIR)/usr/share/hostap/
	$(INSTALL) -m 0644 $(BR2_EXTERNAL_TOMATO64_PATH)/package/hostapd-openwrt/files/hostapd.uc $(TARGET_DIR)/usr/share/hostap/
	$(INSTALL) -m 0644 $(BR2_EXTERNAL_TOMATO64_PATH)/package/hostapd-openwrt/files/wpa_supplicant.uc $(TARGET_DIR)/usr/share/hostap/

endef

$(eval $(generic-package))
