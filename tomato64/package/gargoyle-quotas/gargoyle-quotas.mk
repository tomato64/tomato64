################################################################################
#
# gargoyle-quotas
#
################################################################################

GARGOYLE_QUOTAS_VERSION = 1.0
GARGOYLE_QUOTAS_SITE = $(GARGOYLE_QUOTAS_PKGDIR)/tools
GARGOYLE_QUOTAS_SITE_METHOD = local
GARGOYLE_QUOTAS_LICENSE = GPL-2.0+
GARGOYLE_QUOTAS_INSTALL_STAGING = YES

# libiptbwctl is self-contained (libc only), so there is nothing to depend on
# beyond the toolchain. The xt_bandwidth match it talks to comes from the
# kernel patch selected via BR2_PACKAGE_GARGOYLE_BANDWIDTH.

define GARGOYLE_QUOTAS_BUILD_CMDS
	$(TARGET_CC) $(TARGET_CFLAGS) -c -o $(@D)/ipt_bwctl.o $(@D)/ipt_bwctl.c
	$(TARGET_CC) $(TARGET_CFLAGS) -c -o $(@D)/ipt_bwctl_safe_malloc.o $(@D)/ipt_bwctl_safe_malloc.c
	$(TARGET_AR) rcs $(@D)/libiptbwctl.a $(@D)/ipt_bwctl.o $(@D)/ipt_bwctl_safe_malloc.o
	$(TARGET_CC) $(TARGET_CFLAGS) $(TARGET_LDFLAGS) -I$(@D) \
		-o $(@D)/quota_backup $(@D)/quota_backup.c $(@D)/libiptbwctl.a
endef

# Static library + header go to staging only: httpd links them for the quota
# usage page, and there is no reason to ship a shared object on the image.
define GARGOYLE_QUOTAS_INSTALL_STAGING_CMDS
	$(INSTALL) -D -m 0644 $(@D)/libiptbwctl.a $(STAGING_DIR)/usr/lib/libiptbwctl.a
	$(INSTALL) -D -m 0644 $(@D)/ipt_bwctl.h $(STAGING_DIR)/usr/include/ipt_bwctl.h
endef

define GARGOYLE_QUOTAS_INSTALL_TARGET_CMDS
	$(INSTALL) -D -m 0755 $(@D)/quota_backup $(TARGET_DIR)/usr/sbin/quota_backup
endef

$(eval $(generic-package))
