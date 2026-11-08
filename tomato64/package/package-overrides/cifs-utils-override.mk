################################################################################
#
# cifs-utils-override
#
# Only mount.cifs is wanted: util-linux mount hands "-t cifs" to it, and it
# resolves the server hostname (adds ip=) and handles user=/pass= options.
# Everything else (cifs.upcall, cifs.idmap, cifscreds, get/setcifsacl,
# smbinfo, PAM module, idmap plugin) is disabled.
#
# Capability dropping only matters for setuid user mounts, so libcap/libcap-ng
# are disabled rather than picked up depending on what else is in the build.
#
# mount.cifs itself links libtalloc (resolve_host.c's CLDAP site lookup), so
# libtalloc stays on the target.
#
################################################################################

CIFS_UTILS_CONF_OPTS += \
	--disable-cifsupcall \
	--disable-cifscreds \
	--disable-cifsidmap \
	--disable-cifsacl \
	--disable-smbinfo \
	--disable-pythontools \
	--disable-pam \
	--disable-systemd \
	--without-libcap-ng \
	--without-libcap

override define CIFS_UTILS_INSTALL_TARGET_CMDS
	$(INSTALL) -D -m 0755 $(@D)/mount.cifs $(TARGET_DIR)/sbin/mount.cifs
	ln -sf mount.cifs $(TARGET_DIR)/sbin/mount.smb3
endef
