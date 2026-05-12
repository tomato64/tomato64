################################################################################
#
# linux-firmware-openwrt
#
################################################################################

LINUX_FIRMWARE_OPENWRT_VERSION = 20260221
LINUX_FIRMWARE_OPENWRT_SOURCE = linux-firmware-$(LINUX_FIRMWARE_OPENWRT_VERSION).tar.xz
LINUX_FIRMWARE_OPENWRT_SITE = $(BR2_KERNEL_MIRROR)/linux/kernel/firmware

# Airoha EN8811H 2.5G PHY + Inside-Secure EIP-197 crypto firmware for BPI-R3 Mini
# EN8811H filenames match OpenWrt package/firmware/linux-firmware/airoha.mk
# (airoha-en8811h-firmware); other airoha/* blobs in this tarball are for
# unrelated SoCs (EN7581, AN7583, AN8811HB) and aren't needed on BPI-R3 Mini.
ifeq ($(BR2_PACKAGE_PLATFORM_BPIR3MINI),y)
define LINUX_FIRMWARE_OPENWRT_INSTALL_TARGET_CMDS
	mkdir -p $(TARGET_DIR)/lib/firmware/airoha
	cp $(@D)/airoha/EthMD32.dm.bin  $(TARGET_DIR)/lib/firmware/airoha
	cp $(@D)/airoha/EthMD32.DSP.bin $(TARGET_DIR)/lib/firmware/airoha
	cp $(@D)/airoha/EthMD32.dm.bin  $(BINARIES_DIR)
	cp $(@D)/airoha/EthMD32.DSP.bin $(BINARIES_DIR)
	mkdir -p $(TARGET_DIR)/lib/firmware/inside-secure/eip197_minifw
	cp $(@D)/inside-secure/eip197_minifw/ifpp.bin $(TARGET_DIR)/lib/firmware/inside-secure/eip197_minifw
	cp $(@D)/inside-secure/eip197_minifw/ipue.bin $(TARGET_DIR)/lib/firmware/inside-secure/eip197_minifw
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

# Realtek 1G/2.5G NIC + RTL8822CS SDIO WiFi firmware for NanoPi R76S
# (WiFi module is optional but firmware is always installed so it works if present)
# rtw8822c_{fw,wow_fw}.bin match OpenWrt package/firmware/linux-firmware/realtek.mk
# (rtl8822ce-firmware package; same firmware is used for 8822CE and 8822CS).
ifeq ($(BR2_PACKAGE_PLATFORM_R76S),y)
define LINUX_FIRMWARE_OPENWRT_INSTALL_TARGET_CMDS
	mkdir -p $(TARGET_DIR)/lib/firmware/rtl_nic
	cp $(@D)/rtl_nic/rtl810*  $(TARGET_DIR)/lib/firmware/rtl_nic
	cp $(@D)/rtl_nic/rtl812*  $(TARGET_DIR)/lib/firmware/rtl_nic
	cp $(@D)/rtl_nic/rtl8168* $(TARGET_DIR)/lib/firmware/rtl_nic
	cp $(@D)/rtl_nic/rtl84*   $(TARGET_DIR)/lib/firmware/rtl_nic
	mkdir -p $(TARGET_DIR)/lib/firmware/rtw88
	cp $(@D)/rtw88/rtw8822c_fw.bin     $(TARGET_DIR)/lib/firmware/rtw88
	cp $(@D)/rtw88/rtw8822c_wow_fw.bin $(TARGET_DIR)/lib/firmware/rtw88
endef
endif

# MediaTek Bluetooth + Intel WiFi firmware for x86_64
ifeq ($(BR2_PACKAGE_PLATFORM_X86_64),y)
define LINUX_FIRMWARE_OPENWRT_INSTALL_TARGET_CMDS

	mkdir -p $(TARGET_DIR)/lib/firmware/mediatek/mt7925
	# MT7921 Bluetooth firmware
	cp $(@D)/mediatek/BT_RAM_CODE_MT7961_1_2_hdr.bin $(TARGET_DIR)/lib/firmware/mediatek
	# MT7922 Bluetooth firmware
	cp $(@D)/mediatek/BT_RAM_CODE_MT7922_1_1_hdr.bin $(TARGET_DIR)/lib/firmware/mediatek
	# MT7925 Bluetooth firmware
	cp $(@D)/mediatek/mt7925/BT_RAM_CODE_MT7925_1_1_hdr.bin $(TARGET_DIR)/lib/firmware/mediatek/mt7925

	# Intel iwlwifi firmware
	# Filenames and versions sourced from OpenWrt: package/firmware/linux-firmware/intel.mk
	# When bumping mac80211 backports or linux-firmware version, check OpenWrt's
	# intel.mk at the matching tag for updated firmware filenames.

	# iwl3945/4965 (legacy a/b/g/n)
	cp $(@D)/intel/iwlwifi/iwlwifi-3945-2.ucode $(TARGET_DIR)/lib/firmware
	cp $(@D)/intel/iwlwifi/iwlwifi-4965-2.ucode $(TARGET_DIR)/lib/firmware

	# Centrino Wireless-N 100/1000/105/135/2200/2230
	cp $(@D)/intel/iwlwifi/iwlwifi-100-5.ucode $(TARGET_DIR)/lib/firmware
	cp $(@D)/intel/iwlwifi/iwlwifi-1000-5.ucode $(TARGET_DIR)/lib/firmware
	cp $(@D)/intel/iwlwifi/iwlwifi-105-6.ucode $(TARGET_DIR)/lib/firmware
	cp $(@D)/intel/iwlwifi/iwlwifi-135-6.ucode $(TARGET_DIR)/lib/firmware
	cp $(@D)/intel/iwlwifi/iwlwifi-2000-6.ucode $(TARGET_DIR)/lib/firmware
	cp $(@D)/intel/iwlwifi/iwlwifi-2030-6.ucode $(TARGET_DIR)/lib/firmware

	# 5100/5300/5350AGN, 5150AGN
	cp $(@D)/intel/iwlwifi/iwlwifi-5000-5.ucode $(TARGET_DIR)/lib/firmware
	cp $(@D)/intel/iwlwifi/iwlwifi-5150-2.ucode $(TARGET_DIR)/lib/firmware

	# Centrino 6300/6200/6205/6230/6150/6250
	cp $(@D)/intel/iwlwifi/iwlwifi-6000-4.ucode $(TARGET_DIR)/lib/firmware
	cp $(@D)/intel/iwlwifi/iwlwifi-6000g2a-6.ucode $(TARGET_DIR)/lib/firmware
	cp $(@D)/intel/iwlwifi/iwlwifi-6000g2b-6.ucode $(TARGET_DIR)/lib/firmware
	cp $(@D)/intel/iwlwifi/iwlwifi-6050-5.ucode $(TARGET_DIR)/lib/firmware

	# 7260/7265/7265D/3160/3168
	cp $(@D)/intel/iwlwifi/iwlwifi-7260-17.ucode $(TARGET_DIR)/lib/firmware
	cp $(@D)/intel/iwlwifi/iwlwifi-7265-17.ucode $(TARGET_DIR)/lib/firmware
	cp $(@D)/intel/iwlwifi/iwlwifi-7265D-29.ucode $(TARGET_DIR)/lib/firmware
	cp $(@D)/intel/iwlwifi/iwlwifi-3160-17.ucode $(TARGET_DIR)/lib/firmware
	cp $(@D)/intel/iwlwifi/iwlwifi-3168-29.ucode $(TARGET_DIR)/lib/firmware

	# 8260/8265
	cp $(@D)/intel/iwlwifi/iwlwifi-8000C-36.ucode $(TARGET_DIR)/lib/firmware
	cp $(@D)/intel/iwlwifi/iwlwifi-8265-36.ucode $(TARGET_DIR)/lib/firmware

	# 9000/9260 series (standalone 9560)
	cp $(@D)/intel/iwlwifi/iwlwifi-9000-pu-b0-jf-b0-46.ucode $(TARGET_DIR)/lib/firmware
	cp $(@D)/intel/iwlwifi/iwlwifi-9260-th-b0-jf-b0-46.ucode $(TARGET_DIR)/lib/firmware

	# AX200 (Cyclone Creek, standalone)
	cp $(@D)/intel/iwlwifi/iwlwifi-cc-a0-77.ucode $(TARGET_DIR)/lib/firmware

	# AX201 (Qu-Z platform + Harrison Peak RF CNVi)
	cp $(@D)/intel/iwlwifi/iwlwifi-QuZ-a0-hr-b0-77.ucode $(TARGET_DIR)/lib/firmware

	# AX101 (Solar platform + Harrison Peak RF CNVi)
	cp $(@D)/intel/iwlwifi/iwlwifi-so-a0-hr-b0-89.ucode $(TARGET_DIR)/lib/firmware

	# Solar platform + 9560 RF CNVi (e.g. Alder Lake PCH with 9560)
	cp $(@D)/intel/iwlwifi/iwlwifi-so-a0-jf-b0-77.ucode $(TARGET_DIR)/lib/firmware

	# AX210 (Typhoon platform + Garfield Peak RF)
	cp $(@D)/intel/iwlwifi/iwlwifi-ty-a0-gf-a0-89.ucode $(TARGET_DIR)/lib/firmware
	cp $(@D)/intel/iwlwifi/iwlwifi-ty-a0-gf-a0.pnvm $(TARGET_DIR)/lib/firmware

	# AX210 (Solar platform + Garfield Peak RF)
	cp $(@D)/intel/iwlwifi/iwlwifi-so-a0-gf-a0-89.ucode $(TARGET_DIR)/lib/firmware
	cp $(@D)/intel/iwlwifi/iwlwifi-so-a0-gf-a0.pnvm $(TARGET_DIR)/lib/firmware

	# AX411 (Solar platform + Garfield Peak 4 RF)
	cp $(@D)/intel/iwlwifi/iwlwifi-so-a0-gf4-a0-89.ucode $(TARGET_DIR)/lib/firmware
	cp $(@D)/intel/iwlwifi/iwlwifi-so-a0-gf4-a0.pnvm $(TARGET_DIR)/lib/firmware

	# BE200 (Glacier platform + FM RF)
	cp $(@D)/intel/iwlwifi/iwlwifi-gl-c0-fm-c0-101.ucode $(TARGET_DIR)/lib/firmware
	cp $(@D)/intel/iwlwifi/iwlwifi-gl-c0-fm-c0.pnvm $(TARGET_DIR)/lib/firmware
endef
endif

$(eval $(generic-package))
