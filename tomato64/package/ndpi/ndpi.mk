################################################################################
#
# ndpi
#
################################################################################

NDPI_VERSION = 4c1a23b0539ec15296029b149c53f5946d8b7280
NDPI_SITE = $(call github,vel21ripn,nDPI,$(NDPI_VERSION))
NDPI_LICENSE = GPL-3.0
NDPI_DEPENDENCIES = iptables linux
NDPI_AUTORECONF = YES
NDPI_CONF_OPTS = --with-only-libndpi

define NDPI_BUILD_CMDS
	$(TARGET_MAKE_ENV) $(MAKE) KERNEL_DIR=$(LINUX_DIR) CC="$(TARGET_CC)" -C $(@D)/ndpi-netfilter $(LINUX_MAKE_FLAGS)
endef

define NDPI_INSTALL_TARGET_CMDS
	$(INSTALL) -D -m 0755 $(@D)/ndpi-netfilter/ipt/libxt_ndpi.so $(TARGET_DIR)/usr/lib/xtables

	cd $(@D)/ndpi-netfilter/src && \
	make -C $(LINUX_DIR) M="$$PWD" modules_install \
	INSTALL_MOD_PATH=$(TARGET_DIR) \
	INSTALL_MOD_DIR=extra
endef

$(eval $(autotools-package))
