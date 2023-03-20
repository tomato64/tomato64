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

ifeq ($(BR2_PACKAGE_RC_BT),y)
export TCONFIG_BT=y
endif

ifeq ($(BR2_PACKAGE_RC_CIFS),y)
export TCONFIG_CIFS=y
endif

ifeq ($(BR2_PACKAGE_RC_DDNS),y)
export TCONFIG_DDNS=y
endif

ifeq ($(BR2_PACKAGE_RC_DNSCRYPT),y)
export TCONFIG_DNSCRYPT=y
endif

ifeq ($(BR2_PACKAGE_RC_DNSSEC),y)
export TCONFIG_DNSSEC=y
endif

ifeq ($(BR2_PACKAGE_RC_FTP),y)
export TCONFIG_FTP=y
endif

ifeq ($(BR2_PACKAGE_RC_HTTPS),y)
export TCONFIG_HTTPS=y
endif

ifeq ($(BR2_PACKAGE_RC_IPV6),y)
export TCONFIG_IPV6=y
endif

ifeq ($(BR2_PACKAGE_RC_MDNS),y)
export TCONFIG_MDNS=y
endif

ifeq ($(BR2_PACKAGE_RC_MULTIWAN),y)
export TCONFIG_MULTIWAN=y
endif

ifeq ($(BR2_PACKAGE_RC_NFS),y)
export TCONFIG_NFS=y
endif

ifeq ($(BR2_PACKAGE_RC_NGINX),y)
export TCONFIG_NGINX=y
endif

ifeq ($(BR2_PACKAGE_RC_NOCAT),y)
export TCONFIG_NOCAT=y
endif

ifeq ($(BR2_PACKAGE_RC_OPENVPN),y)
export TCONFIG_OPENVPN=y
endif

ifeq ($(BR2_PACKAGE_RC_PPTPD),y)
export TCONFIG_PPTPD=y
endif

ifeq ($(BR2_PACKAGE_RC_SAMBA),y)
export TCONFIG_SAMBASRV=y
endif

ifeq ($(BR2_PACKAGE_RC_SNMP),y)
export TCONFIG_SNMP=y
endif

ifeq ($(BR2_PACKAGE_RC_STUBBY),y)
export TCONFIG_STUBBY=y
endif

ifeq ($(BR2_PACKAGE_RC_TINC),y)
export TCONFIG_TINC=y
endif

ifeq ($(BR2_PACKAGE_RC_TOR),y)
export TCONFIG_TOR=y
endif

ifeq ($(BR2_PACKAGE_RC_UPS),y)
export TCONFIG_UPS=y
endif

ifeq ($(BR2_PACKAGE_RC_USB),y)
export TCONFIG_USB=y
endif

define RC_BUILD_CMDS
	$(MAKE) $(TARGET_CONFIGURE_OPTS) -C $(@D)
endef

define RC_INSTALL_STAGING_CMDS
	$(INSTALL) -D -m 0755 $(@D)/rc $(STAGING_DIR)/sbin
endef

define RC_INSTALL_TARGET_CMDS
	$(INSTALL) -D -m 0755 $(@D)/rc $(TARGET_DIR)/sbin
#	ln -sf rc $(TARGET_DIR)/sbin/init
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
