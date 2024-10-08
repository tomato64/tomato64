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
HOSTAPD_OPENWRT_CFLAGS += -I$(STAGING_DIR)/usr/include/libnl-tiny

define HOSTAPD_OPENWRT_CONFIGURE_CMDS

	echo ` \
	CFLAGS="-I$(STAGING_DIR)/usr/include/libnl-tiny -I$(@D)/src/crypto -DCONFIG_LIBNL20 -D_GNU_SOURCE -DCONFIG_MSG_MIN_PRIORITY=3 -Os -pipe -mcpu=cortex-a53 -fno-caller-saves -fno-plt -ffunction-sections -fdata-sections -flto=auto -fno-fat-lto-objects -Wformat -Werror=format-security -DPIC -fPIC -fstack-protector -D_FORTIFY_SOURCE=1 -Wl,-z,now -Wl,-z,relro" \
	$(MAKE) --no-print-directory \
	-C $(@D)/hostapd \
	AR="$(TARGET_CROSS)gcc-ar" \
	AS="$(TARGET_CROSS)gcc -c -Os -pipe -mcpu=cortex-a53 -fno-caller-saves -fno-plt -ffunction-sections -fdata-sections -flto=auto -fno-fat-lto-objects -Wformat -Werror=format-security -DPIC -fPIC -fstack-protector -Wl,-z,now -Wl,-z,relro" \
	LD="$(TARGET_CROSS)ld.bfd" \
	NM="$(TARGET_CROSS)gcc-nm" \
	CC="$(TARGET_CROSS)gcc" \
	GCC="$(TARGET_CROSS)gcc" \
	CXX="$(TARGET_CROSS)g++" \
	RANLIB="$(TARGET_CROSS)gcc-ranlib" \
	STRIP="$(TARGET_CROSS)strip" \
	OBJCOPY="$(TARGET_CROSS)objcopy" \
	OBJDUMP="$(TARGET_CROSS)objdump" \
	SIZE="$(TARGET_CROSS)size" \
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
	LIBS="-L$(TARGET_DIR)/usr/lib -fuse-ld=bfd -Wl,--gc-sections -flto=auto -fuse-linker-plugin -DPIC -fPIC -znow -zrelro -lcrypto -lssl -lubox -lubus -lblobmsg_json -lucode -lm -lnl-tiny -ludebug" \
	LIBS_c="" \
	AR="$(TARGET_CROSS)gcc-ar" \
	BCHECK= \
	V=1 \
	-s \
	MULTICALL=1 \
	dump_cflags; \
	CFLAGS="-I$(STAGING_DIR)/usr/include/libnl-tiny -I$(@D)/src/crypto -DCONFIG_LIBNL20 -D_GNU_SOURCE -DCONFIG_MSG_MIN_PRIORITY=3 -Os -pipe -mcpu=cortex-a53 -fno-caller-saves -fno-plt -ffunction-sections -fdata-sections -flto=auto -fno-fat-lto-objects -Wformat -Werror=format-security -DPIC -fPIC -fstack-protector -D_FORTIFY_SOURCE=1 -Wl,-z,now -Wl,-z,relro" \
	$(MAKE) --no-print-directory \
	-C $(@D)/wpa_supplicant \
	AR="$(TARGET_CROSS)gcc-ar" \
	AS="$(TARGET_CROSS)gcc -c -Os -pipe -mcpu=cortex-a53 -fno-caller-saves -fno-plt -ffunction-sections -fdata-sections -flto=auto -fno-fat-lto-objects -Wformat -Werror=format-security -DPIC -fPIC -fstack-protector -Wl,-z,now -Wl,-z,relro" \
	LD="$(TARGET_CROSS)ld.bfd" \
	NM="$(TARGET_CROSS)gcc-nm" \
	CC="$(TARGET_CROSS)gcc" \
	GCC="$(TARGET_CROSS)gcc" \
	CXX="$(TARGET_CROSS)g++" \
	RANLIB="$(TARGET_CROSS)gcc-ranlib" \
	STRIP="$(TARGET_CROSS)strip" \
	OBJCOPY="$(TARGET_CROSS)objcopy" \
	OBJDUMP="$(TARGET_CROSS)objdump" \
	SIZE="$(TARGET_CROSS)size" \
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
	LIBS="-L$(TARGET_DIR)/usr/lib -fuse-ld=bfd -Wl,--gc-sections -flto=auto -fuse-linker-plugin -DPIC -fPIC -znow -zrelro -lcrypto -lssl -lubox -lubus -lblobmsg_json -lucode -lm -lnl-tiny -ludebug" \
	LIBS_c="" \
	AR="$(TARGET_CROSS)gcc-ar" \
	BCHECK= \
	V=1 \
	-s \
	MULTICALL=1 \
	dump_cflags | sed -e 's,-n ,,g' -e 's^-Os -pipe -mcpu=cortex-a53 -fno-caller-saves -fno-plt -ffunction-sections -fdata-sections -flto=auto -fno-fat-lto-objects -Wformat -Werror=format-security -DPIC -fPIC -fstack-protector -D_FORTIFY_SOURCE=1 -Wl,-z,now -Wl,-z,relro^^' ` > $(@D)/.cflags

sed -i 's/"/\\"/g' $(@D)/.cflags

endef

define HOSTAPD_OPENWRT_BUILD_CMDS

	CFLAGS="-I$(STAGING_DIR)/usr/include/libnl-tiny -I$(@D)/src/crypto -DCONFIG_LIBNL20 -D_GNU_SOURCE -DCONFIG_MSG_MIN_PRIORITY=3 -Os -pipe -mcpu=cortex-a53 -fno-caller-saves -fno-plt -ffunction-sections -fdata-sections -flto=auto -fno-fat-lto-objects -Wformat -Werror=format-security -DPIC -fPIC -fstack-protector -Wl,-z,now -Wl,-z,relro" \
	$(MAKE) \
	-C $(@D)/hostapd \
	AR="$(TARGET_CROSS)gcc-ar" \
	AS="$(TARGET_CROSS)gcc -c -Os -pipe -mcpu=cortex-a53 -fno-caller-saves -fno-plt -ffunction-sections -fdata-sections -flto=auto -fno-fat-lto-objects -Wformat -Werror=format-security -DPIC -fPIC -fstack-protector -D_FORTIFY_SOURCE=1 -Wl,-z,now -Wl,-z,relro" \
	LD="$(TARGET_CROSS)ld.bfd" \
	NM="$(TARGET_CROSS)gcc-nm" \
	CC="$(TARGET_CROSS)gcc" \
	GCC="$(TARGET_CROSS)gcc" \
	CXX="$(TARGET_CROSS)g++" \
	RANLIB="$(TARGET_CROSS)gcc-ranlib" \
	STRIP="$(TARGET_CROSS)strip" \
	OBJCOPY="$(TARGET_CROSS)objcopy" \
	OBJDUMP="$(TARGET_CROSS)objdump" \
	SIZE="$(TARGET_CROSS)size" \
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
	LIBS="-L$(TARGET_DIR)/usr/lib -fuse-ld=bfd -Wl,--gc-sections -flto=auto -fuse-linker-plugin -DPIC -fPIC -znow -zrelro -lcrypto -lssl -lubox -lubus -lblobmsg_json -lucode -lm -lnl-tiny -ludebug" \
	LIBS_c="" \
	AR="$(TARGET_CROSS)gcc-ar" \
	BCHECK= \
	V=1  \
	CFLAGS='$(shell cat $(@D)/.cflags)' \
	MULTICALL=1 \
	hostapd_cli \
	hostapd_multi.a

	CFLAGS="-I$(STAGING_DIR)/usr/include/libnl-tiny -I$(@D)/src/crypto -DCONFIG_LIBNL20 -D_GNU_SOURCE -DCONFIG_MSG_MIN_PRIORITY=3 -Os -pipe -mcpu=cortex-a53 -fno-caller-saves -fno-plt -ffunction-sections -fdata-sections -flto=auto -fno-fat-lto-objects -Wformat -Werror=format-security -DPIC -fPIC -fstack-protector -Wl,-z,now -Wl,-z,relro" \
	$(MAKE) \
	-C $(@D)/wpa_supplicant \
	AR="$(TARGET_CROSS)gcc-ar" \
	AS="$(TARGET_CROSS)gcc -c -Os -pipe -mcpu=cortex-a53 -fno-caller-saves -fno-plt -ffunction-sections -fdata-sections -flto=auto -fno-fat-lto-objects -Wformat -Werror=format-security -DPIC -fPIC -fstack-protector -D_FORTIFY_SOURCE=1 -Wl,-z,now -Wl,-z,relro" \
	LD="$(TARGET_CROSS)ld.bfd" \
	NM="$(TARGET_CROSS)gcc-nm" \
	CC="$(TARGET_CROSS)gcc" \
	GCC="$(TARGET_CROSS)gcc" \
	CXX="$(TARGET_CROSS)g++" \
	RANLIB="$(TARGET_CROSS)gcc-ranlib" \
	STRIP="$(TARGET_CROSS)strip" \
	OBJCOPY="$(TARGET_CROSS)objcopy" \
	OBJDUMP="$(TARGET_CROSS)objdump" \
	SIZE="$(TARGET_CROSS)size" \
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
	LIBS="-L$(TARGET_DIR)/usr/lib -fuse-ld=bfd -Wl,--gc-sections -flto=auto -fuse-linker-plugin -DPIC -fPIC -znow -zrelro -lcrypto -lssl -lubox -lubus -lblobmsg_json -lucode -lm -lnl-tiny -ludebug" \
	LIBS_c="" \
	AR="$(TARGET_CROSS)gcc-ar" \
	BCHECK= \
	V=1  \
	CFLAGS='$(shell cat $(@D)/.cflags)' \
	MULTICALL=1 \
	wpa_cli \
	wpa_supplicant_multi.a

	$(TARGET_CC) \
	-o $(@D)/wpad \
	-Os -pipe -mcpu=cortex-a53 -fno-caller-saves -fno-plt -ffunction-sections -fdata-sections -flto=auto -fno-fat-lto-objects \
	-Wformat -Werror=format-security -DPIC -fPIC -fstack-protector -D_FORTIFY_SOURCE=1 -Wl,-z,now -Wl,-z,relro \
	$(BR2_EXTERNAL_TOMATO64_PATH)/package/hostapd-openwrt/files/multicall.c \
	$(@D)/hostapd/hostapd_multi.a \
	$(@D)/wpa_supplicant/wpa_supplicant_multi.a \
	-L$(TARGET_DIR)/usr/lib \
	-fuse-ld=bfd -Wl,--gc-sections -flto=auto -fuse-linker-plugin -DPIC -fPIC -znow -zrelro \
	-lcrypto -lssl -lubox -lubus -lblobmsg_json -lucode -lm -lnl-tiny -ludebug

endef

define HOSTAPD_OPENWRT_INSTALL_TARGET_CMDS

	install -m0755 $(@D)/wpad $(TARGET_DIR)/usr/sbin/
	ln -sf wpad $(TARGET_DIR)/usr/sbin/hostapd
	ln -sf wpad $(TARGET_DIR)/usr/sbin/wpa_supplicant

endef

$(eval $(generic-package))
