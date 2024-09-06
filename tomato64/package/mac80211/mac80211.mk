################################################################################
#
# mac80211
#
################################################################################

MAC80211_VERSION = 6.9.9
MAC80211_SOURCE = backports-$(MAC80211_VERSION).tar.xz
MAC80211_SITE = http://mirror2.openwrt.org/sources
MAC80211_LICENSE = GPL-3.0
MAC80211_DEPENDENCIES = linux

define MAC80211_BUILD_CMDS

cp $(BR2_EXTERNAL_TOMATO64_PATH)/package/mac80211/.config $(@D)

$(TARGET_MAKE_ENV) \
$(MAKE) \
-C $(@D) \
KCFLAGS="-fno-caller-saves " \
HOSTCFLAGS="-O2 -I$(HOST_DIR)/include/  -Wall -Wmissing-prototypes -Wstrict-prototypes" \
CROSS_COMPILE="aarch64-tomato64-linux-musl-" \
ARCH="arm64" \
KBUILD_HAVE_NLS=no \
KBUILD_BUILD_USER="" \
KBUILD_BUILD_HOST="" \
KBUILD_BUILD_TIMESTAMP="$(shell perl -e 'print scalar gmtime($(SOURCE_DATE_EPOCH))')" \
KBUILD_BUILD_VERSION="0" \
KBUILD_HOSTLDFLAGS="-L$(HOST_DIR)/lib" \
CONFIG_SHELL="bash" \
V=''  \
cmd_syscalls= \
KBUILD_EXTRA_SYMBOLS="$(BR2_EXTERNAL_TOMATO64_PATH)/package/mac80211/gpio-button-hotplug.symvers $(BR2_EXTERNAL_TOMATO64_PATH)/package/mac80211/mac80211.symvers $(BR2_EXTERNAL_TOMATO64_PATH)/package/mac80211/mt76.symvers" \
CC="aarch64-tomato64-linux-musl-" \
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
CROSS_COMPILE="aarch64-tomato64-linux-musl-" \
ARCH="arm64" \
KBUILD_HAVE_NLS=no \
KBUILD_BUILD_USER="" \
KBUILD_BUILD_HOST="" \
KBUILD_BUILD_TIMESTAMP="$(shell perl -e 'print scalar gmtime($(SOURCE_DATE_EPOCH))')" \
KBUILD_BUILD_VERSION="0" \
KBUILD_HOSTLDFLAGS="-L$(HOST_DIR)/lib" \
CONFIG_SHELL="bash" \
V=''  \
cmd_syscalls= \
KBUILD_EXTRA_SYMBOLS="$(BR2_EXTERNAL_TOMATO64_PATH)/package/mac80211/gpio-button-hotplug.symvers $(BR2_EXTERNAL_TOMATO64_PATH)/package/mac80211/mac80211.symvers $(BR2_EXTERNAL_TOMATO64_PATH)/package/mac80211/mt76.symvers" \
CC=$(TARGET_CC) \
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
	mkdir -p $(TARGET_DIR)/lib/modules/$(LINUX_VERSION)/kernel/net/mac80211
        $(INSTALL) $(@D)/net/mac80211/mac80211.ko $(TARGET_DIR)/lib/modules/$(LINUX_VERSION)/kernel/net/mac80211
	mkdir -p $(TARGET_DIR)/lib/modules/$(LINUX_VERSION)/kernel/net/wireless
        $(INSTALL) $(@D)/net/wireless/cfg80211.ko $(TARGET_DIR)/lib/modules/$(LINUX_VERSION)/kernel/net/wireless
	mkdir -p $(TARGET_DIR)/lib/modules/$(LINUX_VERSION)/kernel/compat
        $(INSTALL) $(@D)/compat/compat.ko $(TARGET_DIR)/lib/modules/$(LINUX_VERSION)/kernel/compat
endef

$(eval $(generic-package))

