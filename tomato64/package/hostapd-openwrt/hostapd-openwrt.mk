################################################################################
#
# hostapd-openwrt
#
################################################################################

HOSTAPD_OPENWRT_VERSION = 5ace39b0a4cdbe18ddbc4e18f80ee3876233c20b
HOSTAPD_OPENWRT_SITE = https://w1.fi/hostap.git
HOSTAPD_OPENWRT_SITE_METHOD=git
HOSTAPD_OPENWRT_SUBDIR = hostapd
HOSTAPD_OPENWRT_CONFIG = $(HOSTAPD_OPENWRT_DIR)/$(HOSTAPD_OPENWRT_SUBDIR)/.config
HOSTAPD_OPENWRT_DEPENDENCIES = host-pkgconf libubox ucode udebug
HOSTAPD_OPENWRT_CFLAGS = $(TARGET_CFLAGS)
HOSTAPD_OPENWRT_LICENSE = BSD-3-Clause
HOSTAPD_OPENWRT_LICENSE_FILES = README

HOSTAPD_OPENWRT_CPE_ID_VENDOR = w1.fi
HOSTAPD_OPENWRT_SELINUX_MODULES = hostapd

HOSTAPD_OPENWRT_CONFIG_ENABLE = \
	CONFIG_INTERNAL_LIBTOMMATH \
	CONFIG_DEBUG_FILE \
	CONFIG_DEBUG_SYSLOG

HOSTAPD_OPENWRT_CONFIG_DISABLE =

define HOSTAPD_OPENWRT_PATCHES
	cp -fpR $(BR2_EXTERNAL_TOMATO64_PATH)/package/hostapd-openwrt/src/. $(@D)
endef

HOSTAPD_OPENWRT_PRE_PATCH_HOOKS += HOSTAPD_OPENWRT_PATCHES

define HOSTAPD_OPENWRT_COPY_CONFIG
	cp -fpR $(BR2_EXTERNAL_TOMATO64_PATH)/package/hostapd-openwrt/files/hostapd-full.config $(@D)/hostapd/.config
	cp -fpR $(BR2_EXTERNAL_TOMATO64_PATH)/package/hostapd-openwrt/files/wpa_supplicant-full.config $(@D)/wpa_supplicant/.config

#	sed -i 's,CONFIG_UBUS=y,#CONFIG_UBUS=y,g' \
#	$(@D)/hostapd/.config \
#	$(@D)/wpa_supplicant/.config

endef

HOSTAPD_OPENWRT_POST_PATCH_HOOKS += HOSTAPD_OPENWRT_COPY_CONFIG

HOSTAPD_OPENWRT_DEPENDENCIES += libopenssl
HOSTAPD_OPENWRT_LIBS += `$(PKG_CONFIG_HOST_BINARY) --libs openssl`
HOSTAPD_OPENWRT_CONFIG_EDITS += 's/\#\(CONFIG_TLS=openssl\)/\1/'

HOSTAPD_OPENWRT_DEPENDENCIES += libnl
HOSTAPD_OPENWRT_CFLAGS += -I$(STAGING_DIR)/usr/include/libnl3/
HOSTAPD_OPENWRT_CONFIG_ENABLE += CONFIG_LIBNL32

define HOSTAPD_OPENWRT_CONFIGURE_CMDS

	echo ` \
	CFLAGS="$(TARGET_CFLAGS) -I$(@D)/src -I$(@D)/src/utils -I$(@D)/src/crypto -DCONFIG_LIBNL20 -D_GNU_SOURCE -DCONFIG_MSG_MIN_PRIORITY=3 -Os -pipe -mcpu=cortex-a53 -fno-caller-saves -fno-plt -fhonour-copts -ffunction-sections -fdata-sections -flto=auto -fno-fat-lto-objects -Wformat -Werror=format-security -DPIC -fPIC -fstack-protector -D_FORTIFY_SOURCE=1 -Wl,-z,now -Wl,-z,relro" \
	$(TARGET_CONFIGURE_OPTS) \
	$(MAKE) \
	-C $(@D)/hostapd \
	CONFIG_ACS=y \
	CONFIG_DRIVER_NL80211=y \
	CONFIG_IEEE80211AC=y \
	CONFIG_IEEE80211AX=y \
	CONFIG_MBO=y \
	CONFIG_UCODE=y \
	CONFIG_APUP=y \
	CONFIG_TLS=openssl \
	CONFIG_SAE=y \
	CONFIG_OWE=y \
	CONFIG_SUITEB192=y \
	CONFIG_AP=y \
	CONFIG_MESH=y \
	LIBS="-fuse-ld=bfd -Wl,--gc-sections -flto=auto -fuse-linker-plugin -DPIC -fPIC -znow -zrelro -lcrypto -lssl -lubox -lubus -lblobmsg_json -lucode -lm -lnl-tiny -ludebug" \
	LIBS_c="" \
	BCHECK= \
	V=1 \
	-s \
	MULTICALL=1 \
	dump_cflags; \
	$(MAKE) \
	-C $(@D)/wpa_supplicant \
	CONFIG_ACS=y \
	CONFIG_DRIVER_NL80211=y \
	CONFIG_IEEE80211AC=y \
	CONFIG_IEEE80211AX=y \
	CONFIG_MBO=y \
	CONFIG_UCODE=y \
	CONFIG_APUP=y \
	CONFIG_TLS=openssl \
	CONFIG_SAE=y \
	CONFIG_OWE=y \
	CONFIG_SUITEB192=y \
	CONFIG_AP=y \
	CONFIG_MESH=y \
	LIBS="-fuse-ld=bfd -Wl,--gc-sections -flto=auto -fuse-linker-plugin -DPIC -fPIC -znow -zrelro -lcrypto -lssl -lubox -lubus -lblobmsg_json -lucode -lm -lnl-tiny -ludebug" \
	LIBS_c="" \
	BCHECK= \
	V=1 \
	-s \
	MULTICALL=1 \
	dump_cflags \
	| sed -e 's,-n ,,g' -e 's^-Os -pipe -mcpu=cortex-a53 -fno-caller-saves -fno-plt -fhonour-copts -ffunction-sections -fdata-sections -flto=auto -fno-fat-lto-objects -Wformat -Werror=format-security -DPIC -fPIC -fstack-protector -D_FORTIFY_SOURCE=1 -Wl,-z,now -Wl,-z,relro^^' \
	` > $(@D)/.cflags

endef

define HOSTAPD_OPENWRT_BUILD_CMDS
	CFLAGS="$(TARGET_CFLAGS) $(cat $(@D)/.cflags) -I$(@D)/src -I$(@D)/src/utils -I$(@D)/src/crypto -DCONFIG_LIBNL20 -D_GNU_SOURCE -DCONFIG_MSG_MIN_PRIORITY=3 -Os -pipe -mcpu=cortex-a53 -fno-caller-saves -fno-plt -fhonour-copts -ffunction-sections -fdata-sections -flto=auto -fno-fat-lto-objects -Wformat -Werror=format-security -DPIC -fPIC -fstack-protector -D_FORTIFY_SOURCE=1 -Wl,-z,now -Wl,-z,relro" \
	$(TARGET_CONFIGURE_OPTS) \
	$(MAKE) \
	-C $(@D)/hostapd \
	CONFIG_ACS=y \
	CONFIG_DRIVER_NL80211=y \
	CONFIG_IEEE80211AC=y \
	CONFIG_IEEE80211AX=y \
	CONFIG_MBO=y \
	CONFIG_UCODE=y \
	CONFIG_APUP=y \
	CONFIG_TLS=openssl \
	CONFIG_SAE=y \
	CONFIG_OWE=y \
	CONFIG_SUITEB192=y \
	CONFIG_AP=y \
	CONFIG_MESH=y \
	LIBS="-fuse-ld=bfd -Wl,--gc-sections -flto=auto -fuse-linker-plugin -DPIC -fPIC -znow -zrelro -lcrypto -lssl -lubox -lubus -lblobmsg_json -lucode -lm -lnl-tiny -ludebug" \
	LIBS_c="" \
	BCHECK= \
	V=1  \
	MULTICALL=1 \
	hostapd_cli \
	hostapd_multi.a

	CFLAGS="$(TARGET_CFLAGS) $(cat $(@D)/.cflags) -I$(@D)/src -I$(@D)/src/utils -I$(@D)/src/crypto -DCONFIG_LIBNL20 -D_GNU_SOURCE -DCONFIG_MSG_MIN_PRIORITY=3 -Os -pipe -mcpu=cortex-a53 -fno-caller-saves -fno-plt -fhonour-copts -ffunction-sections -fdata-sections -flto=auto -fno-fat-lto-objects -Wformat -Werror=format-security -DPIC -fPIC -fstack-protector -D_FORTIFY_SOURCE=1 -Wl,-z,now -Wl,-z,relro" \
	$(TARGET_CONFIGURE_OPTS) \
	$(MAKE) \
	-C $(@D)/wpa_supplicant \
	CONFIG_ACS=y \
	CONFIG_DRIVER_NL80211=y \
	CONFIG_IEEE80211AC=y \
	CONFIG_IEEE80211AX=y \
	CONFIG_MBO=y \
	CONFIG_UCODE=y \
	CONFIG_APUP=y \
	CONFIG_TLS=openssl \
	CONFIG_SAE=y \
	CONFIG_OWE=y \
	CONFIG_SUITEB192=y \
	CONFIG_AP=y \
	CONFIG_MESH=y \
	LIBS="-fuse-ld=bfd -Wl,--gc-sections -flto=auto -fuse-linker-plugin -DPIC -fPIC -znow -zrelro -lcrypto -lssl -lubox -lubus -lblobmsg_json -lucode -lm -lnl-tiny -ludebug" \
	LIBS_c="" \
	BCHECK= \
	V=1  \
	MULTICALL=1 \
	wpa_cli \
	wpa_supplicant_multi.a

	$(TARGET_CC) \
	-o $(@D)/wpad \
	-Os -pipe -mcpu=cortex-a53 -fno-caller-saves -fno-plt -ffunction-sections -fdata-sections -flto=auto -fno-fat-lto-objects -Wformat -Werror=format-security -DPIC -fPIC -fstack-protector -D_FORTIFY_SOURCE=1 -Wl,-z,now -Wl,-z,relro $(BR2_EXTERNAL_TOMATO64_PATH)/package/hostapd-openwrt/files/multicall.c $(@D)/hostapd/hostapd_multi.a $(@D)/wpa_supplicant/wpa_supplicant_multi.a -fuse-ld=bfd -Wl,--gc-sections -flto=auto -fuse-linker-plugin -DPIC -fPIC -znow -zrelro -lcrypto -lssl -lubox -lubus -lblobmsg_json -lucode -lm -lnl-3 -lnl-genl-3 -ludebug

endef

define HOSTAPD_OPENWRT_INSTALL_TARGET_CMDS

	install -m0755 $(@D)/wpad $(TARGET_DIR)/usr/sbin/
	ln -sf wpad $(TARGET_DIR)/usr/sbin/hostapd
	ln -sf wpad $(TARGET_DIR)/usr/sbin/wpa_supplicant

endef

$(eval $(generic-package))
