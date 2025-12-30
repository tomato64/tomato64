################################################################################
#
# wireless-regdb-openwrt
#
################################################################################

WIRELESS_REGDB_OPENWRT_VERSION = 2025.07.10
WIRELESS_REGDB_OPENWRT_SOURCE = wireless-regdb-$(WIRELESS_REGDB_OPENWRT_VERSION).tar.xz
WIRELESS_REGDB_OPENWRT_SITE = $(BR2_KERNEL_MIRROR)/software/network/wireless-regdb
WIRELESS_REGDB_OPENWRT_LICENSE = ISC
WIRELESS_REGDB_OPENWRT_LICENSE_FILES = LICENSE
WIRELESS_REGDB_OPENWRT_DEPENDENCIES = host-python3

define WIRELESS_REGDB_OPENWRT_BUILD_CMDS
	$(HOST_DIR)/bin/python3 $(@D)/db2fw.py $(@D)/regulatory.db $(@D)/db.txt
endef

define WIRELESS_REGDB_OPENWRT_INSTALL_TARGET_CMDS
	$(INSTALL) -d $(TARGET_DIR)/lib/firmware
	$(INSTALL) -m 0644 $(@D)/regulatory.db $(TARGET_DIR)/lib/firmware/
endef

$(eval $(generic-package))
