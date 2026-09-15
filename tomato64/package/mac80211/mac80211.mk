################################################################################
#
# mac80211
#
################################################################################

ifeq ($(BR2_PACKAGE_PLATFORM_BE14000),y)
MAC80211_VERSION = 6.18.39
else
MAC80211_VERSION = 6.18.26
endif
MAC80211_SOURCE = backports-$(MAC80211_VERSION).tar.zst
MAC80211_SITE = https://github.com/openwrt/backports/releases/download/backports-v$(MAC80211_VERSION)
MAC80211_LICENSE = GPL-3.0
MAC80211_DEPENDENCIES = linux host-flex
MAC80211_INSTALL_STAGING = YES

# Exported so packages installed outside this recipe can report the wireless
# stack version (e.g. TomatoAnon). Only set when backports is actually built.
ifeq ($(BR2_PACKAGE_MAC80211),y)
export MAC80211_VERSION
endif

define MAC80211_BUILD_CMDS

	cp $(BR2_EXTERNAL_TOMATO64_PATH)/package/mac80211/config_base $(@D)/.config

	# x86_64
	$(if $(BR2_PACKAGE_PLATFORM_X86_64), \
		cat $(BR2_EXTERNAL_TOMATO64_PATH)/package/mac80211/config_x86_64 >> $(@D)/.config; \
	)

	# R76S (RTW88 SDIO for optional onboard RTL8822CS WiFi module)
	$(if $(BR2_PACKAGE_PLATFORM_R76S), \
		cat $(BR2_EXTERNAL_TOMATO64_PATH)/package/mac80211/config_rockchip >> $(@D)/.config; \
	)

	$(TARGET_MAKE_ENV) \
	$(MAKE) \
	-C $(@D) \
	KCFLAGS="-fno-caller-saves " \
	HOSTCFLAGS="-O2 -I$(HOST_DIR)/include/  -Wall -Wmissing-prototypes -Wstrict-prototypes" \
	CROSS_COMPILE="$(TARGET_CROSS)" \
	ARCH="$(KERNEL_ARCH)" \
	KBUILD_HAVE_NLS=no \
	KBUILD_BUILD_USER="" \
	KBUILD_BUILD_HOST="" \
	KBUILD_BUILD_TIMESTAMP="$(shell perl -e 'print scalar gmtime($(SOURCE_DATE_EPOCH))')" \
	KBUILD_BUILD_VERSION="0" \
	KBUILD_HOSTLDFLAGS="-L$(HOST_DIR)/lib" \
	CONFIG_SHELL="bash" \
	V=''  \
	cmd_syscalls= \
	KBUILD_EXTRA_SYMBOLS="$(BR2_EXTERNAL_TOMATO64_PATH)/package/mac80211/gpio-button-hotplug.symvers $(call tomato64-extra-symvers,mac80211)" \
	LEX="flex" \
	KERNELRELEASE=$(LINUX_VERSION) \
	EXTRA_CFLAGS="-I$(@D)/include -fmacro-prefix-map=$(@D)=mac80211-$(MAC80211_VERSION) " \
	KLIB_BUILD=$(LINUX_DIR) \
	MODPROBE=true \
	KLIB=/lib/modules/$(LINUX_VERSION) \
	KERNEL_SUBLEVEL=6 \
	KBUILD_LDFLAGS_MODULE_PREREQ= \
	allnoconfig

	$(TARGET_MAKE_ENV) \
	$(MAKE) \
	--jobserver-auth=3,4  \
	-C $(@D) \
	KCFLAGS="-fno-caller-saves " \
	HOSTCFLAGS="-O2 -I$(HOST_DIR)/include/  -Wall -Wmissing-prototypes -Wstrict-prototypes" \
	CROSS_COMPILE="$(TARGET_CROSS)" \
	ARCH="$(KERNEL_ARCH)" \
	KBUILD_HAVE_NLS=no \
	KBUILD_BUILD_USER="" \
	KBUILD_BUILD_HOST="" \
	KBUILD_BUILD_TIMESTAMP="$(shell perl -e 'print scalar gmtime($(SOURCE_DATE_EPOCH))')" \
	KBUILD_BUILD_VERSION="0" \
	KBUILD_HOSTLDFLAGS="-L$(HOST_DIR)/lib" \
	CONFIG_SHELL="bash" \
	V=''  \
	cmd_syscalls= \
	KBUILD_EXTRA_SYMBOLS="$(BR2_EXTERNAL_TOMATO64_PATH)/package/mac80211/gpio-button-hotplug.symvers $(call tomato64-extra-symvers,mac80211)" \
	CC=$(TARGET_CC) \
	LEX="flex" \
	KERNELRELEASE=$(LINUX_VERSION) \
	EXTRA_CFLAGS="-I$(@D)/include -fmacro-prefix-map=$(@D)=mac80211-$(MAC80211_VERSION) " \
	KLIB_BUILD=$(LINUX_DIR) \
	MODPROBE=true \
	KLIB=/lib/modules/$(LINUX_VERSION) \
	KERNEL_SUBLEVEL=6 \
	KBUILD_LDFLAGS_MODULE_PREREQ= \
	modules

endef

define MAC80211_INSTALL_TARGET_CMDS

	mkdir -p $(TARGET_DIR)/lib/modules/$(LINUX_VERSION)/wifi

        $(INSTALL) $(@D)/net/mac80211/mac80211.ko	$(TARGET_DIR)/lib/modules/$(LINUX_VERSION)/wifi
        $(INSTALL) $(@D)/net/wireless/cfg80211.ko	$(TARGET_DIR)/lib/modules/$(LINUX_VERSION)/wifi
        $(INSTALL) $(@D)/compat/compat.ko		$(TARGET_DIR)/lib/modules/$(LINUX_VERSION)/wifi
endef

define MAC80211_INSTALL_STAGING_CMDS
	mkdir -p $(STAGING_DIR)/usr/include/mac80211
	mkdir -p $(STAGING_DIR)/usr/include/mac80211-backport
	mkdir -p $(STAGING_DIR)/usr/include/mac80211/ath
	mkdir -p $(STAGING_DIR)/usr/include/net/mac80211

        cp -r $(@D)/net/mac80211/*.h $(@D)/include/* $(STAGING_DIR)/usr/include/mac80211/
        cp -r $(@D)/backport-include/* $(STAGING_DIR)/usr/include/mac80211-backport/
        cp -r $(@D)/net/mac80211/rate.h $(STAGING_DIR)/usr/include/net/mac80211/
        cp -r $(@D)/drivers/net/wireless/ath/*.h $(STAGING_DIR)/usr/include/mac80211/ath/
        rm -f $(STAGING_DIR)/usr/include/mac80211-backport/linux/module.h
endef

# Install iwlwifi modules for x86_64 only
ifeq ($(BR2_PACKAGE_PLATFORM_X86_64),y)
define MAC80211_INSTALL_IWLWIFI
	$(INSTALL) $(@D)/drivers/net/wireless/intel/iwlwifi/iwlwifi.ko	$(TARGET_DIR)/lib/modules/$(LINUX_VERSION)/wifi
	$(INSTALL) $(@D)/drivers/net/wireless/intel/iwlwifi/dvm/iwldvm.ko	$(TARGET_DIR)/lib/modules/$(LINUX_VERSION)/wifi
	$(INSTALL) $(@D)/drivers/net/wireless/intel/iwlwifi/mvm/iwlmvm.ko	$(TARGET_DIR)/lib/modules/$(LINUX_VERSION)/wifi
	$(INSTALL) $(@D)/drivers/net/wireless/intel/iwlwifi/mld/iwlmld.ko	$(TARGET_DIR)/lib/modules/$(LINUX_VERSION)/wifi
endef
MAC80211_POST_INSTALL_TARGET_HOOKS += MAC80211_INSTALL_IWLWIFI
endif

# Install RTW88 SDIO (RTL8822CS) modules for R76S (optional onboard WiFi module)
ifeq ($(BR2_PACKAGE_PLATFORM_R76S),y)
define MAC80211_INSTALL_RTW88
	$(INSTALL) $(@D)/drivers/net/wireless/realtek/rtw88/rtw88_core.ko	$(TARGET_DIR)/lib/modules/$(LINUX_VERSION)/wifi
	$(INSTALL) $(@D)/drivers/net/wireless/realtek/rtw88/rtw88_sdio.ko	$(TARGET_DIR)/lib/modules/$(LINUX_VERSION)/wifi
	$(INSTALL) $(@D)/drivers/net/wireless/realtek/rtw88/rtw88_8822c.ko	$(TARGET_DIR)/lib/modules/$(LINUX_VERSION)/wifi
	$(INSTALL) $(@D)/drivers/net/wireless/realtek/rtw88/rtw88_8822cs.ko	$(TARGET_DIR)/lib/modules/$(LINUX_VERSION)/wifi
endef
MAC80211_POST_INSTALL_TARGET_HOOKS += MAC80211_INSTALL_RTW88
endif

# Install RTW89 USB (Wi-Fi 6) modules for all targets. Drives Realtek RTL8832BU /
# RTL8852BU USB adapters (RTL8832BU = USB ID 0bda:b832, handled by rtw89_8852bu).
# Enabled in config_base, so built for every platform; autoloads via hotplug2
# coldplug + modalias, same path as the mt76 USB (mt7921u/mt7925u) drivers.
define MAC80211_INSTALL_RTW89
	$(INSTALL) $(@D)/drivers/net/wireless/realtek/rtw89/rtw89_core.ko		$(TARGET_DIR)/lib/modules/$(LINUX_VERSION)/wifi
	$(INSTALL) $(@D)/drivers/net/wireless/realtek/rtw89/rtw89_usb.ko			$(TARGET_DIR)/lib/modules/$(LINUX_VERSION)/wifi
	$(INSTALL) $(@D)/drivers/net/wireless/realtek/rtw89/rtw89_8852b.ko		$(TARGET_DIR)/lib/modules/$(LINUX_VERSION)/wifi
	$(INSTALL) $(@D)/drivers/net/wireless/realtek/rtw89/rtw89_8852b_common.ko	$(TARGET_DIR)/lib/modules/$(LINUX_VERSION)/wifi
	$(INSTALL) $(@D)/drivers/net/wireless/realtek/rtw89/rtw89_8852bu.ko		$(TARGET_DIR)/lib/modules/$(LINUX_VERSION)/wifi
endef
MAC80211_POST_INSTALL_TARGET_HOOKS += MAC80211_INSTALL_RTW89

# .ko are hand-copied (not modules_install), so strip debug info here.
# Runs last to cover the platform-conditional module hooks above.
define MAC80211_STRIP_MODULES
	find $(TARGET_DIR)/lib/modules/$(LINUX_VERSION)/wifi -name '*.ko' \
		-exec $(TARGET_CROSS)strip --strip-debug {} +
endef
MAC80211_POST_INSTALL_TARGET_HOOKS += MAC80211_STRIP_MODULES

define MAC80211_APPLY_VERSIONED_PATCHES
	$(APPLY_PATCHES) $(@D) $(MAC80211_PKGDIR)/patches-$(MAC80211_VERSION) \*.patch
endef
MAC80211_POST_PATCH_HOOKS += MAC80211_APPLY_VERSIONED_PATCHES

# Publish our symbols for anything built afterwards (mt76) - see external.mk.
define MAC80211_COLLECT_SYMVERS
	$(call tomato64-collect-symvers,mac80211,$(@D))
endef
MAC80211_POST_BUILD_HOOKS += MAC80211_COLLECT_SYMVERS

$(eval $(generic-package))
