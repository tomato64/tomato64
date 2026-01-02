################################################################################
#
# openwrt-wifi
#
################################################################################

OPENWRT_WIFI_VERSION = v25.12.0-rc2
OPENWRT_WIFI_SITE = https://git.openwrt.org/openwrt/openwrt.git
OPENWRT_WIFI_SITE_METHOD = git
OPENWRT_WIFI_LICENSE = GPL-2.0

# For WIFI_SCRIPTS_UCODE mode, we need ucode-mod-digest
OPENWRT_WIFI_DEPENDENCIES = ucode

define OPENWRT_WIFI_INSTALL_TARGET_CMDS

	# === Common files (from files/) ===
	$(INSTALL) -d $(TARGET_DIR)/usr/share/hostap/
	$(INSTALL) -m 0644 $(@D)/package/network/config/wifi-scripts/files/usr/share/hostap/common.uc $(TARGET_DIR)/usr/share/hostap/
	$(INSTALL) -m 0644 $(@D)/package/network/config/wifi-scripts/files/usr/share/hostap/wdev.uc $(TARGET_DIR)/usr/share/hostap/
	$(INSTALL) -m 0644 $(@D)/package/network/config/wifi-scripts/files/usr/share/hostap/wifi-detect.uc $(TARGET_DIR)/usr/share/hostap/

	$(INSTALL) -d $(TARGET_DIR)/sbin/
	$(INSTALL) -m 0755 $(@D)/package/network/config/wifi-scripts/files/sbin/wifi $(TARGET_DIR)/sbin/

	$(INSTALL) -d $(TARGET_DIR)/lib/wifi
	$(INSTALL) -m 0644 $(@D)/package/network/config/wifi-scripts/files/lib/wifi/mac80211.uc $(TARGET_DIR)/lib/wifi/

	$(INSTALL) -d $(TARGET_DIR)/lib/netifd/wireless
	$(INSTALL) -m 0644 $(@D)/package/network/config/wifi-scripts/files/lib/netifd/hostapd.sh $(TARGET_DIR)/lib/netifd/
	$(INSTALL) -m 0644 $(@D)/package/network/config/wifi-scripts/files/lib/netifd/netifd-wireless.sh $(TARGET_DIR)/lib/netifd/

	# New in v25: wireless.uc and wireless-device.uc for netifd (from wifi-scripts)
	$(INSTALL) -m 0644 $(@D)/package/network/config/wifi-scripts/files/lib/netifd/wireless.uc $(TARGET_DIR)/lib/netifd/
	$(INSTALL) -m 0644 $(@D)/package/network/config/wifi-scripts/files/lib/netifd/wireless-device.uc $(TARGET_DIR)/lib/netifd/

	# New in v25: netifd ucode scripts main.uc and utils.uc (from netifd package)
	$(INSTALL) -m 0644 $(@D)/package/network/config/netifd/files/lib/netifd/main.uc $(TARGET_DIR)/lib/netifd/
	$(INSTALL) -m 0644 $(@D)/package/network/config/netifd/files/lib/netifd/utils.uc $(TARGET_DIR)/lib/netifd/

	# New in v25: wifi/utils.uc module
	$(INSTALL) -d $(TARGET_DIR)/usr/share/ucode/wifi
	$(INSTALL) -m 0644 $(@D)/package/network/config/wifi-scripts/files/usr/share/ucode/wifi/utils.uc $(TARGET_DIR)/usr/share/ucode/wifi/

	# === UCODE wifi-scripts (from files-ucode/) - replaces legacy shell scripts ===

	# Ucode mac80211.sh replaces the legacy 33K line shell script
	$(INSTALL) -m 0755 $(@D)/package/network/config/wifi-scripts/files-ucode/lib/netifd/wireless/mac80211.sh $(TARGET_DIR)/lib/netifd/wireless/

	# Ucode iwinfo CLI and module
#	$(INSTALL) -m 0755 $(@D)/package/network/config/wifi-scripts/files-ucode/usr/bin/iwinfo $(TARGET_DIR)/usr/bin/
	$(INSTALL) -m 0644 $(@D)/package/network/config/wifi-scripts/files-ucode/usr/share/ucode/iwinfo.uc $(TARGET_DIR)/usr/share/ucode/

	# Ucode wifi management modules
	$(INSTALL) -m 0644 $(@D)/package/network/config/wifi-scripts/files-ucode/usr/share/ucode/wifi/ap.uc $(TARGET_DIR)/usr/share/ucode/wifi/
	$(INSTALL) -m 0644 $(@D)/package/network/config/wifi-scripts/files-ucode/usr/share/ucode/wifi/common.uc $(TARGET_DIR)/usr/share/ucode/wifi/
	$(INSTALL) -m 0644 $(@D)/package/network/config/wifi-scripts/files-ucode/usr/share/ucode/wifi/hostapd.uc $(TARGET_DIR)/usr/share/ucode/wifi/
	$(INSTALL) -m 0644 $(@D)/package/network/config/wifi-scripts/files-ucode/usr/share/ucode/wifi/iface.uc $(TARGET_DIR)/usr/share/ucode/wifi/
	$(INSTALL) -m 0644 $(@D)/package/network/config/wifi-scripts/files-ucode/usr/share/ucode/wifi/netifd.uc $(TARGET_DIR)/usr/share/ucode/wifi/
	$(INSTALL) -m 0644 $(@D)/package/network/config/wifi-scripts/files-ucode/usr/share/ucode/wifi/supplicant.uc $(TARGET_DIR)/usr/share/ucode/wifi/
	$(INSTALL) -m 0644 $(@D)/package/network/config/wifi-scripts/files-ucode/usr/share/ucode/wifi/validate.uc $(TARGET_DIR)/usr/share/ucode/wifi/

	# JSON schemas for validation
	$(INSTALL) -d $(TARGET_DIR)/usr/share/schema
	$(INSTALL) -m 0644 $(@D)/package/network/config/wifi-scripts/files-ucode/usr/share/schema/wireless.wifi-device.json $(TARGET_DIR)/usr/share/schema/
	$(INSTALL) -m 0644 $(@D)/package/network/config/wifi-scripts/files-ucode/usr/share/schema/wireless.wifi-iface.json $(TARGET_DIR)/usr/share/schema/
	$(INSTALL) -m 0644 $(@D)/package/network/config/wifi-scripts/files-ucode/usr/share/schema/wireless.wifi-station.json $(TARGET_DIR)/usr/share/schema/
	$(INSTALL) -m 0644 $(@D)/package/network/config/wifi-scripts/files-ucode/usr/share/schema/wireless.wifi-vlan.json $(TARGET_DIR)/usr/share/schema/

	# Data files
	$(INSTALL) -m 0644 $(@D)/package/network/config/wifi-scripts/files-ucode/usr/share/iso3166.json $(TARGET_DIR)/usr/share/
	$(INSTALL) -m 0644 $(@D)/package/network/config/wifi-scripts/files-ucode/usr/share/wifi_devices.json $(TARGET_DIR)/usr/share/

	# === Other OpenWrt base files ===
	$(INSTALL) -d $(TARGET_DIR)/usr/libexec/network
	$(INSTALL) -m 0755 $(@D)/package/network/config/netifd/files/usr/libexec/network/packet-steering.uc $(TARGET_DIR)/usr/libexec/network/

	$(INSTALL) -d $(TARGET_DIR)/lib/functions
	$(INSTALL) -m 0644 $(@D)/package/base-files/files/lib/functions.sh $(TARGET_DIR)/lib/
	$(INSTALL) -m 0644 $(@D)/package/base-files/files/lib/functions/caldata.sh $(TARGET_DIR)/lib/functions/
	$(INSTALL) -m 0644 $(@D)/package/base-files/files/lib/functions/ipv4.sh $(TARGET_DIR)/lib/functions/
	$(INSTALL) -m 0644 $(@D)/package/base-files/files/lib/functions/leds.sh $(TARGET_DIR)/lib/functions/
	$(INSTALL) -m 0644 $(@D)/package/base-files/files/lib/functions/migrations.sh $(TARGET_DIR)/lib/functions/
	$(INSTALL) -m 0644 $(@D)/package/base-files/files/lib/functions/network.sh $(TARGET_DIR)/lib/functions/
	$(INSTALL) -m 0644 $(@D)/package/base-files/files/lib/functions/preinit.sh $(TARGET_DIR)/lib/functions/
	$(INSTALL) -m 0644 $(@D)/package/base-files/files/lib/functions/service.sh $(TARGET_DIR)/lib/functions/
	$(INSTALL) -m 0644 $(@D)/package/base-files/files/lib/functions/system.sh $(TARGET_DIR)/lib/functions/
	$(INSTALL) -m 0644 $(@D)/package/base-files/files/lib/functions/uci-defaults.sh $(TARGET_DIR)/lib/functions/

	$(INSTALL) -d $(TARGET_DIR)/lib/config
	$(INSTALL) -m 0644 $(@D)/package/system/uci/files/lib/config/uci.sh $(TARGET_DIR)/lib/config/

	# === Tomato64 custom scripts ===
	$(INSTALL) -d $(TARGET_DIR)/usr/bin/
	$(INSTALL) -m 0755 $(BR2_EXTERNAL_TOMATO64_PATH)/package/openwrt-wifi/start_wifi.sh $(TARGET_DIR)/usr/bin/
	$(INSTALL) -m 0755 $(BR2_EXTERNAL_TOMATO64_PATH)/package/openwrt-wifi/enumerate-phy.sh $(TARGET_DIR)/usr/bin/
	$(INSTALL) -m 0755 $(BR2_EXTERNAL_TOMATO64_PATH)/package/openwrt-wifi/packet-steering $(TARGET_DIR)/usr/bin/
	$(INSTALL) -m 0755 $(BR2_EXTERNAL_TOMATO64_PATH)/package/openwrt-wifi/hostapd_event $(TARGET_DIR)/usr/bin/
endef

$(eval $(generic-package))
