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
	--igd2 \
	--leasefile \
	--vendorcfg \
	--portinuse \
	--disable-pppconn \
	--firewall=iptables \
	--iptablespath=$(IPTABLES_DIR) \
	--ipv6
endef

define MINIUPNPD_BUILD_CMDS
	$(MAKE) $(TARGET_CONFIGURE_OPTS) -C $(@D)
endef

define MINIUPNPD_INSTALL_TARGET_CMDS
	$(INSTALL) -D $(@D)/miniupnpd $(TARGET_DIR)/usr/sbin
endef

$(eval $(generic-package))
