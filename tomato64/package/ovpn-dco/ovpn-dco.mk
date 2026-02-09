################################################################################
#
# ovpn-dco
#
################################################################################

OVPN_DCO_VERSION = 0.2.20250801
OVPN_DCO_SITE = $(call github,OpenVPN,ovpn-dco,v$(OVPN_DCO_VERSION))
OVPN_DCO_LICENSE = GPL-2.0-only
OVPN_DCO_DEPENDENCIES = linux

OVPN_DCO_NOSTDINC_FLAGS = \
	-I$(@D)/include \
	-I$(@D)/compat-include \
	-include $(@D)/linux-compat.h

define OVPN_DCO_BUILD_CMDS
	$(TARGET_MAKE_ENV) $(MAKE) \
		-C $(LINUX_DIR) \
		$(LINUX_MAKE_FLAGS) \
		M="$(@D)/drivers/net/ovpn-dco" \
		NOSTDINC_FLAGS="$(OVPN_DCO_NOSTDINC_FLAGS)" \
		CONFIG_OVPN_DCO_V2=m \
		modules
endef

define OVPN_DCO_INSTALL_TARGET_CMDS
	cd $(@D)/drivers/net/ovpn-dco && \
	make -C $(LINUX_DIR) M="$$PWD" modules_install \
	INSTALL_MOD_PATH=$(TARGET_DIR) \
	INSTALL_MOD_DIR=extra
endef

$(eval $(generic-package))
