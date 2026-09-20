################################################################################
#
# glinet-panel-ui
#
################################################################################

GLINET_PANEL_UI_VERSION = e751db45dbc7494344c7b472fa82dc0bdd243959
GLINET_PANEL_UI_SITE = https://github.com/tomato64/glinet-panel-ui.git
GLINET_PANEL_UI_SITE_METHOD = git
GLINET_PANEL_UI_LICENSE = GPL-2.0, OFL-1.1 (fonts)
GLINET_PANEL_UI_LICENSE_FILES = LICENSE.txt assets/fonts/OFL.txt
GLINET_PANEL_UI_DEPENDENCIES = ucode-mod-lvgl libuci

define GLINET_PANEL_UI_INSTALL_TARGET_CMDS
	$(INSTALL) -d $(TARGET_DIR)/usr/share/ucode/glinet-panel-ui/templates
	$(INSTALL) -d $(TARGET_DIR)/usr/share/glinet-panel-ui
	cp -a $(@D)/ucode/. $(TARGET_DIR)/usr/share/ucode/glinet-panel-ui/
	cp -a $(@D)/assets/. $(TARGET_DIR)/usr/share/glinet-panel-ui/

	# The splash images sit beside the other assets, where the applet looks
	# for them; the directory they arrive in is the fork's own and carries a
	# host script with them.
	mv $(TARGET_DIR)/usr/share/glinet-panel-ui/tomato64/tux.png \
		$(TARGET_DIR)/usr/share/glinet-panel-ui/tux.png
	mv $(TARGET_DIR)/usr/share/glinet-panel-ui/tomato64/tomato.png \
		$(TARGET_DIR)/usr/share/glinet-panel-ui/tomato.png
	rm -rf $(TARGET_DIR)/usr/share/glinet-panel-ui/tomato64

	$(INSTALL) -D -m 0755 $(@D)/tomato64/panel_ui \
		$(TARGET_DIR)/usr/bin/panel_ui
	$(INSTALL) -D -m 0755 $(@D)/tomato64/panel_ctl \
		$(TARGET_DIR)/usr/bin/panel_ctl
	$(INSTALL) -D -m 0755 $(@D)/tomato64/panel_boot \
		$(TARGET_DIR)/usr/bin/panel_boot
endef

$(eval $(generic-package))
