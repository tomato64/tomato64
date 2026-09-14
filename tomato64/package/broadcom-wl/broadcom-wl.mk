################################################################################
#
# broadcom-wl
#
################################################################################

BROADCOM_WL_VERSION = f5b412c245658ce6ec5b6cbadc991081dc34b88f
BROADCOM_WL_SITE = https://github.com/tomato64/broadcom-wl.git
BROADCOM_WL_SITE_METHOD = git
BROADCOM_WL_DEPENDENCIES = linux libnvram libshared

define BROADCOM_WL_LINUX_CONFIG_FIXUPS
	$(call KCONFIG_DISABLE_OPT,CONFIG_BCMA_HOST_PCI)
endef

define BROADCOM_WL_BUILD_CMDS
	$(TARGET_MAKE_ENV) $(MAKE) $(TARGET_CONFIGURE_OPTS) \
		BR2_EXTERNAL_TOMATO64_PATH=$(BR2_EXTERNAL_TOMATO64_PATH) \
		OBJCOPY="$(TARGET_CROSS)objcopy" \
		-C $(@D) all
endef

define BROADCOM_WL_INSTALL_TARGET_CMDS
	$(INSTALL) -D -m 0755 $(@D)/userspace/wlconf/wlconf $(TARGET_DIR)/usr/sbin/wlconf
	$(INSTALL) -D -m 0755 $(@D)/userspace/eapd/eapd     $(TARGET_DIR)/usr/sbin/eapd
	$(INSTALL) -D -m 0755 $(@D)/userspace/nas/nas       $(TARGET_DIR)/usr/sbin/nas
	$(INSTALL) -D -m 0755 $(@D)/userspace/wldiag/wldiag $(TARGET_DIR)/usr/sbin/wldiag
	$(BROADCOM_WL_INSTALL_WL_TOOL)
endef

define BROADCOM_WL_INSTALL_WL_TOOL
	$(INSTALL) -D -m 0755 $(@D)/userspace/uclibc-compat/wl \
		$(TARGET_DIR)/usr/sbin/wl
	$(INSTALL) -D -m 0755 $(@D)/userspace/uclibc-compat/uclibc.so \
		$(TARGET_DIR)/lib/uclibc.so
	@# test first: a dangling interpreter symlink would only show up on the
	@# device, as "wl: not found" from exec, which points nowhere useful.
	test -e $(TARGET_DIR)/lib/ld-musl-arm.so.1
	ln -sf ld-musl-arm.so.1 $(TARGET_DIR)/lib/ld-uClibc.so.0
endef

$(eval $(kernel-module))
$(eval $(generic-package))
