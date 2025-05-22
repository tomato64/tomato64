################################################################################
#
# linux-firmware-openwrt
#
################################################################################

LINUX_FIRMWARE_OPENWRT_VERSION = 20241110
LINUX_FIRMWARE_OPENWRT_SOURCE = linux-firmware-$(LINUX_FIRMWARE_OPENWRT_VERSION).tar.xz
LINUX_FIRMWARE_OPENWRT_SITE = $(BR2_KERNEL_MIRROR)/linux/kernel/firmware

define LINUX_FIRMWARE_OPENWRT_INSTALL_TARGET_CMDS
	cp -r $(@D)/airoha $(TARGET_DIR)/lib/firmware
	cp -r $(@D)/inside-secure $(TARGET_DIR)/lib/firmware
endef

$(eval $(generic-package))
