################################################################################
#
# netifd
#
################################################################################

NETIFD_VERSION = cbb83a1857407a28a63dc09412a1f209195914ef
NETIFD_SITE = https://git.openwrt.org/project/netifd.git
NETIFD_SITE_METHOD=git
NETIFD_DEPENDENCIES = libubox ubus udebug libuci libnl-tiny ucode
NETIFD_INSTALL_STAGING = YES

NETIFD_CFLAGS = \
        -I$(STAGING_DIR)/usr/include/libnl-tiny \
        -I$(STAGING_DIR)/usr/include

NETIFD_CONF_OPTS += \
	-DCMAKE_C_FLAGS="$(TARGET_CFLAGS) $(NETIFD_CFLAGS)" \
	-DCMAKE_EXE_LINKER_FLAGS="-lnl-tiny" \
	-DLIBNL_LIBS=nl-tiny

define NETIFD_POST_INSTALL_SCRIPTS
	$(INSTALL) -d $(TARGET_DIR)/lib/netifd
	$(INSTALL) -D -m 0644 $(@D)/scripts/netifd-proto.sh $(TARGET_DIR)/lib/netifd
	$(INSTALL) -D -m 0644 $(@D)/scripts/utils.sh $(TARGET_DIR)/lib/netifd
endef

NETIFD_POST_INSTALL_TARGET_HOOKS += NETIFD_POST_INSTALL_SCRIPTS

$(eval $(cmake-package))
