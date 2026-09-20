################################################################################
#
# be14000-panel-firmware
#
################################################################################

BE14000_PANEL_FIRMWARE_VERSION = 1.0
BE14000_PANEL_FIRMWARE_SITE = $(BE14000_PANEL_FIRMWARE_PKGDIR)/files
BE14000_PANEL_FIRMWARE_SITE_METHOD = local
BE14000_PANEL_FIRMWARE_LICENSE = GPL-2.0

define BE14000_PANEL_FIRMWARE_INSTALL_TARGET_CMDS
	$(INSTALL) -D -m 0644 $(@D)/glinet,gl-be14000-panel.bin \
		$(TARGET_DIR)/lib/firmware/glinet,gl-be14000-panel.bin
endef

$(eval $(generic-package))
