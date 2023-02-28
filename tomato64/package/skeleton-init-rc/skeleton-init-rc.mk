################################################################################
#
# skeleton-init-rc
#
################################################################################

# The skeleton can't depend on the toolchain, since all packages depends on the
# skeleton and the toolchain is a target package, as is skeleton.
# Hence, skeleton would depends on the toolchain and the toolchain would depend
# on skeleton.
SKELETON_INIT_RC_ADD_TOOLCHAIN_DEPENDENCY = NO
SKELETON_INIT_RC_ADD_SKELETON_DEPENDENCY = NO
SKELETON_INIT_RC_PROVIDES = skeleton
SKELETON_INIT_RC_INSTALL_STAGING = YES

SKELETON_INIT_COMMON_PATH = system/skeleton

define SKELETON_INIT_RC_INSTALL_TARGET_CMDS
	$(call SYSTEM_RSYNC,$(SKELETON_INIT_COMMON_PATH),$(TARGET_DIR))
	$(call SYSTEM_RSYNC,$(SKELETON_INIT_RC_PKGDIR)/skeleton,$(TARGET_DIR))
	$(call SYSTEM_USR_SYMLINKS_OR_DIRS,$(TARGET_DIR))
	$(call SYSTEM_LIB_SYMLINK,$(TARGET_DIR))
	$(INSTALL) -m 0644 support/misc/target-dir-warning.txt \
		$(TARGET_DIR_WARNING_FILE)
endef

# We don't care much about what goes in staging, as long as it is
# correctly setup for merged/non-merged /usr. The simplest is to
# fill it in with the content of the skeleton.
define SKELETON_INIT_RC_INSTALL_STAGING_CMDS
	$(call SYSTEM_RSYNC,$(SKELETON_INIT_COMMON_PATH),$(STAGING_DIR))
	$(call SYSTEM_RSYNC,$(SKELETON_INIT_RC_PKGDIR)/skeleton,$(TARGET_DIR))
	$(call SYSTEM_USR_SYMLINKS_OR_DIRS,$(STAGING_DIR))
	$(call SYSTEM_LIB_SYMLINK,$(STAGING_DIR))
	$(INSTALL) -d -m 0755 $(STAGING_DIR)/usr/include
endef

$(eval $(generic-package))
