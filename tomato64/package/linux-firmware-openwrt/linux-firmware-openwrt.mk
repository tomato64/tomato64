################################################################################
#
# linux-firmware-openwrt
#
################################################################################

LINUX_FIRMWARE_OPENWRT_VERSION = 20251125
LINUX_FIRMWARE_OPENWRT_SOURCE = linux-firmware-$(LINUX_FIRMWARE_OPENWRT_VERSION).tar.xz
LINUX_FIRMWARE_OPENWRT_SITE = $(BR2_KERNEL_MIRROR)/linux/kernel/firmware

# Airoha EN7581 and Inside-Secure EIP-197 crypto firmware for BPI-R3 Mini
ifeq ($(BR2_PACKAGE_PLATFORM_BPIR3MINI),y)
define LINUX_FIRMWARE_OPENWRT_INSTALL_TARGET_CMDS
	mkdir -p $(TARGET_DIR)/lib/firmware
	cp -r $(@D)/airoha $(TARGET_DIR)/lib/firmware
	cp -r $(@D)/inside-secure $(TARGET_DIR)/lib/firmware
	cp $(@D)/airoha/* $(BINARIES_DIR)
endef
endif

# RTL8169/RTL8125 firmware for Realtek 1G/2.5G NICs (NanoPi R6S/R5S)
ifneq ($(BR2_PACKAGE_PLATFORM_R6S)$(BR2_PACKAGE_PLATFORM_R5S),)
define LINUX_FIRMWARE_OPENWRT_INSTALL_TARGET_CMDS
	mkdir -p $(TARGET_DIR)/lib/firmware/rtl_nic
	cp $(@D)/rtl_nic/rtl810* $(TARGET_DIR)/lib/firmware/rtl_nic
	cp $(@D)/rtl_nic/rtl812* $(TARGET_DIR)/lib/firmware/rtl_nic
	cp $(@D)/rtl_nic/rtl8168* $(TARGET_DIR)/lib/firmware/rtl_nic
	cp $(@D)/rtl_nic/rtl84* $(TARGET_DIR)/lib/firmware/rtl_nic
endef
endif

# MediaTek MT7921/MT7922/MT7925 Bluetooth firmware for x86_64
ifeq ($(BR2_PACKAGE_PLATFORM_X86_64),y)
define LINUX_FIRMWARE_OPENWRT_INSTALL_TARGET_CMDS
	mkdir -p $(TARGET_DIR)/lib/firmware/mediatek/mt7925
	# MT7921 Bluetooth firmware
	cp $(@D)/mediatek/BT_RAM_CODE_MT7961_1_2_hdr.bin $(TARGET_DIR)/lib/firmware/mediatek
	# MT7922 Bluetooth firmware
	cp $(@D)/mediatek/BT_RAM_CODE_MT7922_1_1_hdr.bin $(TARGET_DIR)/lib/firmware/mediatek
	# MT7925 Bluetooth firmware
	cp $(@D)/mediatek/mt7925/BT_RAM_CODE_MT7925_1_1_hdr.bin $(TARGET_DIR)/lib/firmware/mediatek/mt7925
endef
endif

$(eval $(generic-package))
