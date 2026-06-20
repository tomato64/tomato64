################################################################################
#
# fullconenat
#
################################################################################

FULLCONENAT_VERSION = 74c5e6f3c7faaf33ece451697537c81781781c20
FULLCONENAT_SITE = $(call github,llccd,netfilter-full-cone-nat,$(FULLCONENAT_VERSION))
FULLCONENAT_LICENSE = GPL-2.0
FULLCONENAT_LICENSE_FILES = LICENSE

# linux: xt_FULLCONENAT.ko is built via the kernel-module infra.
# iptables: the userspace libipt_/libip6t_FULLCONENAT.so extensions link
# against libxtables and are loaded by iptables from $(xtlibdir).
FULLCONENAT_DEPENDENCIES = linux iptables

# RFC3489-compatible full cone SNAT netfilter target (xt_FULLCONENAT.ko).
# This is the ImmortalWrt-pinned llccd commit, which already carries the
# mainline (LINUX_VERSION_CODE >= 5.15 && !CONFIG_NF_CONNTRACK_CHAIN_EVENTS)
# conntrack-notifier path required to build AND register on kernel 6.x.
# We deliberately do NOT enable CONFIG_NF_CONNTRACK_CHAIN_EVENTS: that symbol
# is provided only by an out-of-tree OpenWrt hack patch that no longer exists
# for 6.x; the module uses the single mainline notifier slot instead.
define FULLCONENAT_LINUX_CONFIG_FIXUPS
	$(call KCONFIG_ENABLE_OPT,CONFIG_NF_CONNTRACK)
	$(call KCONFIG_ENABLE_OPT,CONFIG_NF_CONNTRACK_EVENTS)
	$(call KCONFIG_ENABLE_OPT,CONFIG_NF_NAT)
endef

# Build the userspace iptables/ip6tables target extensions (shared objects).
# Mirrors the upstream Makefile's "%.so" rule. The kernel module itself is
# built separately by the kernel-module infra via FULLCONENAT_POST_BUILD_HOOKS.
define FULLCONENAT_BUILD_CMDS
	$(TARGET_CC) $(TARGET_CFLAGS) $(TARGET_LDFLAGS) -fPIC -shared \
		-o $(@D)/libipt_FULLCONENAT.so $(@D)/libipt_FULLCONENAT.c -lxtables
	$(TARGET_CC) $(TARGET_CFLAGS) $(TARGET_LDFLAGS) -fPIC -shared \
		-o $(@D)/libip6t_FULLCONENAT.so $(@D)/libip6t_FULLCONENAT.c -lxtables
endef

define FULLCONENAT_INSTALL_TARGET_CMDS
	$(INSTALL) -D -m 0755 $(@D)/libipt_FULLCONENAT.so \
		$(TARGET_DIR)/usr/lib/xtables/libipt_FULLCONENAT.so
	$(INSTALL) -D -m 0755 $(@D)/libip6t_FULLCONENAT.so \
		$(TARGET_DIR)/usr/lib/xtables/libip6t_FULLCONENAT.so
endef

$(eval $(kernel-module))
$(eval $(generic-package))
