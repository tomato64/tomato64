# Admin > Storage only runs smartctl. Its drive database is compiled in, so
# smartd, its config/warning scripts, update-smart-drivedb and the external
# drivedb.h are all left out.
override define SMARTMONTOOLS_INSTALL_TARGET_CMDS
	$(INSTALL) -D -m 0755 $(@D)/smartctl			$(TARGET_DIR)/usr/sbin/smartctl
endef
