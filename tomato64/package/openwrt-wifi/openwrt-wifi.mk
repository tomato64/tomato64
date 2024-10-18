################################################################################
#
# openwrt-wifi
#
################################################################################

OPENWRT_WIFI_VERSION = dbc2923cbe21ad9ea3fbdddc4b880a6e81678e11
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
endef

$(eval $(generic-package))
