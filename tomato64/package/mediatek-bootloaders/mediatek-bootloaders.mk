################################################################################
#
# mediatek-bootloaders
#
################################################################################

MEDIATEK_BOOTLOADERS_VERSION = 2f118ccddd78f3597b2cd5a7ac9c32980e34fc18
MEDIATEK_BOOTLOADERS_SITE = $(call github,tomato64,mediatek-bootloaders,$(MEDIATEK_BOOTLOADERS_VERSION))

define MEDIATEK_BOOTLOADERS_INSTALL_TARGET_CMDS
	$(INSTALL) -D -m 0644 $(@D)/bpi-r3/mt7986-emmc-ddr4-bl2.img $(BINARIES_DIR)/mt7986-emmc-ddr4-bl2.img
	$(INSTALL) -D -m 0644 $(@D)/bpi-r3/mt7986_bananapi_bpi-r3-emmc-u-boot.fip $(BINARIES_DIR)/mt7986_bananapi_bpi-r3-emmc-u-boot.fip
endef

$(eval $(generic-package))
