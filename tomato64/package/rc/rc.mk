################################################################################
#
# rc
#
################################################################################

RC_VERSION = 1.0
RC_SITE = $(BR2_EXTERNAL_TOMATO64_PATH)/package/rc/rc
RC_SITE_METHOD = local
RC_INSTALL_STAGING = YES
RC_LICENSE = tomato
RC_DEPENDENCIES = libnvram libshared

ifeq ($(BR2_PACKAGE_PLATFORM_BPIR3MINI),y)
RC_DEPENDENCIES += host-gzip
endif

include $(BR2_EXTERNAL_TOMATO64_PATH)/package/rc/features

define RC_BUILD_CMDS
	$(MAKE) $(TARGET_CONFIGURE_OPTS) -C $(@D)
endef

define RC_INSTALL_STAGING_CMDS
	$(INSTALL) -D -m 0755 $(@D)/rc $(STAGING_DIR)/sbin
endef

define RC_INSTALL_TARGET_CMDS
	$(INSTALL) -D -m 0755 $(@D)/rc $(TARGET_DIR)/sbin
	ln -sf rc $(TARGET_DIR)/sbin/init
	ln -sf rc $(TARGET_DIR)/sbin/console
	ln -sf rc $(TARGET_DIR)/sbin/hotplug
	ln -sf rc $(TARGET_DIR)/sbin/service
	ln -sf rc $(TARGET_DIR)/sbin/buttons
	ln -sf rc $(TARGET_DIR)/sbin/blink
	ln -sf rc $(TARGET_DIR)/sbin/blink_br
	ln -sf rc $(TARGET_DIR)/sbin/phy_tempsense
	ln -sf rc $(TARGET_DIR)/sbin/rcheck
	ln -sf rc $(TARGET_DIR)/sbin/radio
	ln -sf rc $(TARGET_DIR)/sbin/led
	ln -sf rc $(TARGET_DIR)/sbin/reboot
	ln -sf rc $(TARGET_DIR)/sbin/fast-reboot
	ln -sf rc $(TARGET_DIR)/sbin/halt
	ln -sf rc $(TARGET_DIR)/sbin/redial
	ln -sf rc $(TARGET_DIR)/sbin/mwanroute
	ln -sf rc $(TARGET_DIR)/sbin/gpio
	ln -sf rc $(TARGET_DIR)/sbin/sched
	ln -sf rc $(TARGET_DIR)/sbin/disconnected_pppoe
	ln -sf rc $(TARGET_DIR)/sbin/listen
	ln -sf rc $(TARGET_DIR)/sbin/ppp_event
	ln -sf rc $(TARGET_DIR)/sbin/ntpd_synced
	ln -sf rc $(TARGET_DIR)/sbin/dhcpc-event
	ln -sf rc $(TARGET_DIR)/sbin/dhcpc-release
	ln -sf rc $(TARGET_DIR)/sbin/dhcpc-renew
	ln -sf rc $(TARGET_DIR)/sbin/dhcpc-event-lan
	ln -sf rc $(TARGET_DIR)/sbin/wldist
	ln -sf rc $(TARGET_DIR)/sbin/dhcp6c-state
	ln -sf rc $(TARGET_DIR)/sbin/ddns-update
	ln -sf rc $(TARGET_DIR)/sbin/mount-cifs
	ln -sf rc $(TARGET_DIR)/sbin/roamast
endef

$(eval $(generic-package))
