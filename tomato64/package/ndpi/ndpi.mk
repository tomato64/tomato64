################################################################################
#
# ndpi
#
################################################################################

NDPI_VERSION = 90514cb272b17ab600e68589f0436a25483e1fdb
NDPI_SITE = $(call github,vel21ripn,nDPI,$(NDPI_VERSION))
NDPI_LICENSE = GPL-3.0
NDPI_DEPENDENCIES = iptables linux
NDPI_AUTORECONF = YES
NDPI_CONF_OPTS = --with-only-libndpi

define NDPI_BUILD_CMDS
	$(TARGET_CONFIGURE_OPTS) $(MAKE) KERNEL_DIR=$(LINUX_DIR) CC="$(TARGET_CC)" -C $(@D)/ndpi-netfilter
endef

define NDPI_INSTALL_TARGET_CMDS
	$(INSTALL) -D -m 0755 $(@D)/ndpi-netfilter/ipt/libxt_ndpi.so $(TARGET_DIR)/usr/lib/xtables

	cd $(@D)/ndpi-netfilter/src && \
	make -C $(LINUX_DIR) M="$$PWD" modules_install \
	INSTALL_MOD_PATH=$(TARGET_DIR) \
	INSTALL_MOD_DIR=extra \
	KERNELRELEASE=$(LINUX_VERSION)
endef

$(eval $(autotools-package))
