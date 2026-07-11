################################################################################
#
# transmission-override
#
# Match FreshTomato: prune transmission-create/edit/show, keep
# transmission-remote only when TCONFIG_TR_EXTRAS=y (rc/features). cli is
# dropped via unselected BR2_PACKAGE_TRANSMISSION_CLI in rc/Config.in.
#
# ENABLE_UTILS (default ON) builds all four together, so prune post-install.
# TCONFIG_TR_EXTRAS is read from the recipe env, not a parse-time ifeq:
# package-overrides is parsed before rc.mk sets it.
#
################################################################################

define TRANSMISSION_TRIM_UTILS
	rm -f $(TARGET_DIR)/usr/bin/transmission-create
	rm -f $(TARGET_DIR)/usr/bin/transmission-edit
	rm -f $(TARGET_DIR)/usr/bin/transmission-show
	if [ "$${TCONFIG_TR_EXTRAS}" != "y" ]; then \
		rm -f $(TARGET_DIR)/usr/bin/transmission-remote; \
	fi
endef
TRANSMISSION_POST_INSTALL_TARGET_HOOKS += TRANSMISSION_TRIM_UTILS
