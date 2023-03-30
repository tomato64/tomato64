################################################################################
#
# accel-pptp
#
################################################################################

ACCEL_PPTP_VERSION = 0.8.5
ACCEL_PPTP_SOURCE = accel-pptp-$(ACCEL_PPTP_VERSION).tar.bz2
ACCEL_PPTP_SITE = https://github.com/lancethepants/accel-pptp/releases/download/$(ACCEL_PPTP_VERSION)
ACCEL_PPTP_SUBDIR = pppd_plugin
ACCEL_PPTP_LICENSE = GPL-2.0
ACCEL_PPTP_DEPENDENCIES = linux pppd-tomato
ACCEL_PPTP_INSTALL_STAGING = YES
ACCEL_PPTP_CONF_ENV += CFLAGS="$(TARGET_CFLAGS) -I$(PPPD_TOMATO_DIR)/include"
ACCEL_PPTP_CONF_OPTS = --libdir=/usr/lib/pppd KDIR=$(LINUX_DIR) PPPDIR=$(PPPD_TOMATO_DIR)

define ACCEL_PPTP_COPY_PPPD_SOURCE
	rm -rf $(ACCEL_PPTP_DIR)/kernel
	rm -rf $(ACCEL_PPTP_DIR)/pppd_plugin/src/pppd
	ln -sf $(PPPD_TOMATO_DIR)/pppd $(ACCEL_PPTP_DIR)/pppd_plugin/src/pppd
endef
ACCEL_PPTP_POST_EXTRACT_HOOKS += ACCEL_PPTP_COPY_PPPD_SOURCE

$(eval $(autotools-package))
