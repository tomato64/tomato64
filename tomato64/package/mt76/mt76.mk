################################################################################
#
# mt76
#
################################################################################

MT76_VERSION = 8e4f72b682e9070108536507c5e2720b18c3816d
MT76_SITE = $(call github,openwrt,mt76,$(MT76_VERSION))
MT76_LICENSE = GPL-2.0
MT76_DEPENDENCIES = linux mac80211

define MT76_BUILD_CMDS

	cd $(@D)/tools && \
	$(TARGET_MAKE_ENV) \
	cmake \
	--no-warn-unused-cli \
	-DCMAKE_SYSTEM_NAME=Linux \
	-DCMAKE_SYSTEM_VERSION=1 \
	-DCMAKE_SYSTEM_PROCESSOR=aarch64 \
	-DCMAKE_BUILD_TYPE=Release \
	-DCMAKE_C_FLAGS_RELEASE="-DNDEBUG" \
	-DCMAKE_CXX_FLAGS_RELEASE="-DNDEBUG" \
	-DCMAKE_C_COMPILER_LAUNCHER="" \
	-DCMAKE_CXX_COMPILER_LAUNCHER="" \
	-DCMAKE_ASM_COMPILER_LAUNCHER="" \
	-DCMAKE_EXE_LINKER_FLAGS:STRING="-L$(TARGET_DIR)/usr/lib -L$(TARGET_DIR)/lib -znow -zrelro" \
	-DCMAKE_MODULE_LINKER_FLAGS:STRING="-L$(TARGET_DIR)/usr/lib -L$(TARGET_DIR)/lib -znow -zrelro -Wl,-Bsymbolic-functions" \
	-DCMAKE_SHARED_LINKER_FLAGS:STRING="-L$(TARGET_DIR)/usr/lib -L$(TARGET_DIR)/lib -znow -zrelro -Wl,-Bsymbolic-functions" \
	-DCMAKE_FIND_ROOT_PATH_MODE_PROGRAM=BOTH \
	-DCMAKE_FIND_ROOT_PATH_MODE_LIBRARY=ONLY \
	-DCMAKE_FIND_ROOT_PATH_MODE_INCLUDE=ONLY \
	-DCMAKE_STRIP=: \
	-DCMAKE_INSTALL_PREFIX=/usr \
	-DDL_LIBRARY=$(TARGET_DIR) \
	-DCMAKE_PREFIX_PATH=$(TARGET_DIR) \
	-DCMAKE_SKIP_RPATH=TRUE \
	-DCMAKE_EXPORT_PACKAGE_REGISTRY=FALSE \
	-DCMAKE_EXPORT_NO_PACKAGE_REGISTRY=TRUE \
	-DCMAKE_FIND_USE_PACKAGE_REGISTRY=FALSE \
	-DCMAKE_FIND_PACKAGE_NO_PACKAGE_REGISTRY=TRUE \
	-DCMAKE_FIND_USE_SYSTEM_PACKAGE_REGISTRY=FALSE \
	-DCMAKE_FIND_PACKAGE_NO_SYSTEM_PACKAGE_REGISTRY=TRUE \
	-DCMAKE_GENERATOR="Unix Makefiles" \
	$(@D)/tools


	cd $(@D)/tools && \
	$(TARGET_MAKE_ENV) \
	$(MAKE) \
	-C $(LINUX_DIR) \
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
	CONFIG_MT76_CONNAC_LIB=m \
	CONFIG_MT7915E=m \
	CONFIG_MT798X_WMAC=y \
	M="$(@D)" \
	NOSTDINC_FLAGS="-nostdinc -isystem $(TARGET_DIR)/include -I$(@D) -I$(STAGING_DIR)/usr/include/mac80211-backport/uapi -I$(STAGING_DIR)/usr/include/mac80211-backport -I$(STAGING_DIR)/usr/include/mac80211/uapi -I$(STAGING_DIR)/usr/include/mac80211 -include backport/autoconf.h -include backport/backport.h -DCONFIG_MAC80211_MESH -DCONFIG_MT798X_WMAC" \
	modules

endef

define MT76_INSTALL_TARGET_CMDS

	mkdir -p $(TARGET_DIR)/lib/modules/$(LINUX_VERSION)/wifi

        $(INSTALL) $(@D)/mt76-connac-lib.ko	$(TARGET_DIR)/lib/modules/$(LINUX_VERSION)/wifi
        $(INSTALL) $(@D)/mt7915/mt7915e.ko	$(TARGET_DIR)/lib/modules/$(LINUX_VERSION)/wifi
        $(INSTALL) $(@D)/mt76.ko		$(TARGET_DIR)/lib/modules/$(LINUX_VERSION)/wifi


	mkdir -p $(TARGET_DIR)/lib/firmware/mediatek

	$(INSTALL) $(@D)/firmware/mt7986_wa.bin			$(TARGET_DIR)/lib/firmware/mediatek
	$(INSTALL) $(@D)/firmware/mt7986_wm_mt7975.bin		$(TARGET_DIR)/lib/firmware/mediatek
	$(INSTALL) $(@D)/firmware/mt7986_wm.bin			$(TARGET_DIR)/lib/firmware/mediatek
	$(INSTALL) $(@D)/firmware/mt7986_rom_patch_mt7975.bin	$(TARGET_DIR)/lib/firmware/mediatek
	$(INSTALL) $(@D)/firmware/mt7986_rom_patch.bin		$(TARGET_DIR)/lib/firmware/mediatek
	$(INSTALL) $(@D)/firmware/mt7986_wo_0.bin		$(TARGET_DIR)/lib/firmware/mediatek
	$(INSTALL) $(@D)/firmware/mt7986_wo_1.bin		$(TARGET_DIR)/lib/firmware/mediatek

endef

$(eval $(generic-package))
