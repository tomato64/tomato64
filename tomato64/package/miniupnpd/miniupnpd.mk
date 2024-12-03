################################################################################
#
# miniupnpd
#
################################################################################

MINIUPNPD_VERSION = 2.3.7
MINIUPNPD_SITE = https://miniupnp.tuxfamily.org/files
MINIUPNPD_LICENSE = BSD-3-Clause
MINIUPNPD_DEPENDENCIES = iptables

define MINIUPNPD_CONFIGURE_CMDS
	cd $(MINIUPNPD_DIR) && \
	$(TARGET_CONFIGURE_OPTS)  \
	./configure \
	--leasefile \
	--vendorcfg \
	--portinuse \
	--firewall=iptables \
	$(if $(TCONFIG_IPV6),--ipv6 --igd2,) \
	--iptablespath=$(IPTABLES_DIR)
endef

define MINIUPNPD_BUILD_CMDS
	$(MAKE) $(TARGET_CONFIGURE_OPTS) -C $(@D)
endef

define MINIUPNPD_INSTALL_TARGET_CMDS
	$(INSTALL) -D $(@D)/miniupnpd $(TARGET_DIR)/usr/sbin
endef

$(eval $(generic-package))
