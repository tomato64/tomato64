################################################################################
#
# linux-firmware-openwrt
#
################################################################################

LINUX_FIRMWARE_OPENWRT_VERSION = 20241110
LINUX_FIRMWARE_OPENWRT_SOURCE = linux-firmware-$(LINUX_FIRMWARE_OPENWRT_VERSION).tar.xz
LINUX_FIRMWARE_OPENWRT_SITE = $(BR2_KERNEL_MIRROR)/linux/kernel/firmware

# Airoha EN7581 and Inside-Secure EIP-197 crypto firmware for BPI-R3 Mini
ifeq ($(BR2_PACKAGE_PLATFORM_BPIR3MINI),y)
define LINUX_FIRMWARE_OPENWRT_INSTALL_TARGET_CMDS
	cp -r $(@D)/airoha $(TARGET_DIR)/lib/firmware
	cp -r $(@D)/inside-secure $(TARGET_DIR)/lib/firmware
	cp $(@D)/airoha/* $(BINARIES_DIR)
endef
endif

# RTL8169/RTL8125 firmware for Realtek 1G/2.5G NICs (NanoPi R6S)
ifeq ($(BR2_PACKAGE_PLATFORM_R6S),y)
define LINUX_FIRMWARE_OPENWRT_INSTALL_TARGET_CMDS
	mkdir -p $(TARGET_DIR)/lib/firmware/rtl_nic
	cp $(@D)/rtl_nic/rtl810* $(TARGET_DIR)/lib/firmware/rtl_nic
	cp $(@D)/rtl_nic/rtl812* $(TARGET_DIR)/lib/firmware/rtl_nic
	cp $(@D)/rtl_nic/rtl8168* $(TARGET_DIR)/lib/firmware/rtl_nic
	cp $(@D)/rtl_nic/rtl84* $(TARGET_DIR)/lib/firmware/rtl_nic
endef
endif

$(eval $(generic-package))
