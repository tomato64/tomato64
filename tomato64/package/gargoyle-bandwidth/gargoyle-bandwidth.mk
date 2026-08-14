################################################################################
#
# gargoyle-bandwidth
#
################################################################################

GARGOYLE_BANDWIDTH_VERSION = 1.0
GARGOYLE_BANDWIDTH_SITE = $(GARGOYLE_BANDWIDTH_PKGDIR)/extension
GARGOYLE_BANDWIDTH_SITE_METHOD = local
GARGOYLE_BANDWIDTH_LICENSE = GPL-2.0+
GARGOYLE_BANDWIDTH_DEPENDENCIES = iptables

# The xt_bandwidth kernel module itself is NOT built here: it is patched
# directly into the kernel tree by
# board/common/linux-patches/0007-xt-bandwidth.patch (same approach Gargoyle
# uses, and the same approach as xt_webmon in 0006). This package only builds
# the userspace iptables/ip6tables match extension, and flips the kernel
# option on so enabling the package gets you the module too.
define GARGOYLE_BANDWIDTH_LINUX_CONFIG_FIXUPS
	$(call KCONFIG_ENABLE_OPT,CONFIG_NETFILTER_XT_BANDWIDTH)
endef

# extension/include/linux/netfilter/xt_bandwidth.h is a copy of the header installed
# by 0007-xt-bandwidth.patch; keep the two in sync if the struct ever changes.
#
# -DXTABLES_INTERNAL: xtables.h only exposes ARRAY_SIZE() (which every shipped
# match extension uses) under this define. Gargoyle gets it implicitly by
# building its extensions inside the iptables source tree, whose extensions/
# Makefile sets it; we build standalone, so we set it explicitly.
define GARGOYLE_BANDWIDTH_BUILD_CMDS
	$(TARGET_CC) $(TARGET_CFLAGS) $(TARGET_LDFLAGS) -DXTABLES_INTERNAL -I$(@D)/include \
		-fPIC -shared \
		-o $(@D)/libxt_bandwidth.so $(@D)/libxt_bandwidth.c -lxtables
endef

define GARGOYLE_BANDWIDTH_INSTALL_TARGET_CMDS
	$(INSTALL) -D -m 0755 $(@D)/libxt_bandwidth.so \
		$(TARGET_DIR)/usr/lib/xtables/libxt_bandwidth.so
endef

$(eval $(generic-package))
