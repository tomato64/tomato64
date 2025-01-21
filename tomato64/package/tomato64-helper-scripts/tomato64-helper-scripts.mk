################################################################################
#
# tomato64-helper-scripts
#
################################################################################

TOMATO64_HELPER_SCRIPTS_VERSION = 1.0
TOMATO64_HELPER_SCRIPTS_SITE = $(BR2_EXTERNAL_TOMATO64_PATH)/package/tomato64-helper-scripts
TOMATO64_HELPER_SCRIPTS_SITE_METHOD = local
TOMATO64_HELPER_SCRIPTS_LICENSE = MIT

# Make this smarter at some point
define TOMATO64_HELPER_SCRIPTS_INSTALL_TARGET_CMDS
	$(INSTALL) -D -m 0755 $(@D)/sbin/poweroff			$(TARGET_DIR)/sbin
	$(INSTALL) -D -m 0755 $(@D)/sbin/shutdown			$(TARGET_DIR)/sbin
	$(INSTALL) -D -m 0755 $(@D)/sbin/upgrade			$(TARGET_DIR)/sbin

	$(INSTALL) -D -m 0755 $(@D)/usr/bin/expand_root_partition	$(TARGET_DIR)/usr/bin
	$(INSTALL) -D -m 0755 $(@D)/usr/bin/fudge_time			$(TARGET_DIR)/usr/bin
	$(INSTALL) -D -m 0755 $(@D)/usr/bin/growpart			$(TARGET_DIR)/usr/bin
	$(INSTALL) -D -m 0755 $(@D)/usr/bin/mount_nvram			$(TARGET_DIR)/usr/bin
	$(INSTALL) -D -m 0755 $(@D)/usr/bin/nic_count			$(TARGET_DIR)/usr/bin
	$(INSTALL) -D -m 0755 $(@D)/usr/bin/set_devs			$(TARGET_DIR)/usr/bin
	$(INSTALL) -D -m 0755 $(@D)/usr/bin/set_devs_mt6000		$(TARGET_DIR)/usr/bin
	$(INSTALL) -D -m 0755 $(@D)/usr/bin/set_macs			$(TARGET_DIR)/usr/bin
	$(INSTALL) -D -m 0755 $(@D)/usr/bin/setup-tomato64		$(TARGET_DIR)/usr/bin
	$(INSTALL) -D -m 0755 $(@D)/usr/bin/start_qemu_guest		$(TARGET_DIR)/usr/bin
	$(INSTALL) -D -m 0755 $(@D)/usr/bin/start_ttyd			$(TARGET_DIR)/usr/bin
	$(INSTALL) -D -m 0755 $(@D)/usr/bin/set_jumbo_frame		$(TARGET_DIR)/usr/bin
	$(INSTALL) -D -m 0755 $(@D)/usr/bin/wlbands			$(TARGET_DIR)/usr/bin
	$(INSTALL) -D -m 0755 $(@D)/usr/bin/wldev			$(TARGET_DIR)/usr/bin
	$(INSTALL) -D -m 0755 $(@D)/usr/bin/wlnoise			$(TARGET_DIR)/usr/bin
	$(INSTALL) -D -m 0755 $(@D)/usr/bin/wlifaces			$(TARGET_DIR)/usr/bin
	$(INSTALL) -D -m 0755 $(@D)/usr/bin/wlstats			$(TARGET_DIR)/usr/bin
endef

$(eval $(generic-package))
