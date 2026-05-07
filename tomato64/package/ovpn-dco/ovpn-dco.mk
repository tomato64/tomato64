################################################################################
#
# ovpn-dco
#
################################################################################

OVPN_DCO_VERSION = 7.0.0.2026032400
OVPN_DCO_SITE = https://build.openvpn.net/downloads/releases
OVPN_DCO_SOURCE = ovpn-backports-$(OVPN_DCO_VERSION).tar.gz
OVPN_DCO_LICENSE = GPL-2.0-only
OVPN_DCO_DEPENDENCIES = linux

OVPN_DCO_NOSTDINC_FLAGS = \
	-I$(@D)/include \
	-I$(@D)/compat-include \
	-include $(@D)/linux-compat.h

ifeq ($(BR2_PACKAGE_PLATFORM_RPI4),y)
OVPN_DCO_NOSTDINC_FLAGS += -DOVPN_MODULE_VERSION='\"ovpn-backports-$(OVPN_DCO_VERSION)\"'
endif

define OVPN_DCO_BUILD_CMDS
	$(TARGET_MAKE_ENV) $(MAKE) \
		-C $(LINUX_DIR) \
		$(LINUX_MAKE_FLAGS) \
		M="$(@D)/drivers/net/ovpn" \
		NOSTDINC_FLAGS="$(OVPN_DCO_NOSTDINC_FLAGS)" \
		CONFIG_OVPN=m \
		modules
endef

define OVPN_DCO_INSTALL_TARGET_CMDS
	cd $(@D)/drivers/net/ovpn && \
	make -C $(LINUX_DIR) M="$$PWD" modules_install \
	INSTALL_MOD_PATH=$(TARGET_DIR) \
	INSTALL_MOD_DIR=extra
endef

$(eval $(generic-package))
