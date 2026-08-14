################################################################################
#
# gargoyle-timerange
#
################################################################################

GARGOYLE_TIMERANGE_VERSION = 1.0
GARGOYLE_TIMERANGE_SITE = $(GARGOYLE_TIMERANGE_PKGDIR)/extension
GARGOYLE_TIMERANGE_SITE_METHOD = local
GARGOYLE_TIMERANGE_LICENSE = GPL-2.0+
GARGOYLE_TIMERANGE_DEPENDENCIES = iptables

# The xt_timerange kernel module itself is NOT built here: it is patched
# directly into the kernel tree by
# board/common/linux-patches/0008-xt-timerange.patch (same approach Gargoyle
# uses, and the same approach as xt_webmon in 0006). This package only builds
# the userspace iptables/ip6tables match extension, and flips the kernel
# option on so enabling the package gets you the module too.
define GARGOYLE_TIMERANGE_LINUX_CONFIG_FIXUPS
	$(call KCONFIG_ENABLE_OPT,CONFIG_NETFILTER_XT_TIMERANGE)
endef

# extension/include/linux/netfilter/xt_timerange.h is a copy of the header installed
# by 0008-xt-timerange.patch; keep the two in sync if the struct ever changes.
#
# -DXTABLES_INTERNAL: xtables.h only exposes ARRAY_SIZE() (which every shipped
# match extension uses) under this define. Gargoyle gets it implicitly by
# building its extensions inside the iptables source tree, whose extensions/
# Makefile sets it; we build standalone, so we set it explicitly.
define GARGOYLE_TIMERANGE_BUILD_CMDS
	$(TARGET_CC) $(TARGET_CFLAGS) $(TARGET_LDFLAGS) -DXTABLES_INTERNAL -I$(@D)/include \
		-fPIC -shared \
		-o $(@D)/libxt_timerange.so $(@D)/libxt_timerange.c -lxtables
endef

define GARGOYLE_TIMERANGE_INSTALL_TARGET_CMDS
	$(INSTALL) -D -m 0755 $(@D)/libxt_timerange.so \
		$(TARGET_DIR)/usr/lib/xtables/libxt_timerange.so
endef

$(eval $(generic-package))
