################################################################################
#
# openwrt-wifi
#
################################################################################

OPENWRT_WIFI_VERSION = v24.10.1
OPENWRT_WIFI_SITE = https://git.openwrt.org/openwrt/openwrt.git
OPENWRT_WIFI_SITE_METHOD = git
OPENWRT_WIFI_LICENSE = GPL-2.0

define OPENWRT_WIFI_INSTALL_TARGET_CMDS

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
	$(INSTALL) -m 0755 $(@D)/package/network/config/wifi-scripts/files/lib/netifd/wireless/mac80211.sh $(TARGET_DIR)/lib/netifd/wireless/

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

	$(INSTALL) -d $(TARGET_DIR)/usr/bin/
	$(INSTALL) -m 0755 $(BR2_EXTERNAL_TOMATO64_PATH)/package/openwrt-wifi/start_wifi.sh $(TARGET_DIR)/usr/bin/
	$(INSTALL) -m 0755 $(BR2_EXTERNAL_TOMATO64_PATH)/package/openwrt-wifi/enumerate-phy.sh $(TARGET_DIR)/usr/bin/
	$(INSTALL) -m 0755 $(BR2_EXTERNAL_TOMATO64_PATH)/package/openwrt-wifi/packet-steering $(TARGET_DIR)/usr/bin/
	$(INSTALL) -m 0755 $(BR2_EXTERNAL_TOMATO64_PATH)/package/openwrt-wifi/hostapd_event $(TARGET_DIR)/usr/bin/
endef

$(eval $(generic-package))
