################################################################################
#
# cst353x
#
################################################################################

CST353X_VERSION = 1.0
CST353X_SITE = $(CST353X_PKGDIR)/src
CST353X_SITE_METHOD = local
CST353X_LICENSE = GPL-2.0+

# Driver for the Hynitron CST353X touch controller behind the GL-BE14000's
# front panel (hynitron,cst3530 on i2c-1 @ 0x58). Mainline's hynitron_cstxxx
# cannot drive it: the part uses 32-bit register addressing.
#
# src/ is Felix Fietkau's out-of-tree driver, vendored verbatim from
# gl-inet/gl-openwrt e97a566307 (package/kernel/cst353x/src) - there is no
# standalone upstream to fetch it from.
#
# GL's package sets CONFIG_INPUT_TOUCHSCREEN, but an out-of-tree module does
# not need it: the input_mt_*/touchscreen_* helpers it calls are built into
# input-core whenever CONFIG_INPUT is set, and INPUT_TOUCHSCREEN only gates the
# in-tree drivers/input/touchscreen/ directory. evdev IS needed, for the
# /dev/input/eventN node userspace reads touches from.
define CST353X_LINUX_CONFIG_FIXUPS
	$(call KCONFIG_ENABLE_OPT,CONFIG_INPUT_EVDEV)
endef

$(eval $(kernel-module))
$(eval $(generic-package))
