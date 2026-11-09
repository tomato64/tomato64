################################################################################
#
# mt7996app
#
################################################################################

MT7996APP_VERSION = 8effeb78c56b5a1face798ac00ceeef3d70113fb
MT7996APP_SITE = $(call github,tomato64,mt7996app,$(MT7996APP_VERSION))
MT7996APP_LICENSE = GPL-2.0-only

ifeq ($(BR2_ENABLE_DEBUG),y)
MT7996APP_CARGO_OUTPUT_DIR = debug
else
MT7996APP_CARGO_OUTPUT_DIR = release
endif

define MT7996APP_INSTALL_TARGET_CMDS
	$(INSTALL) -D -m 0755 \
		$(@D)/target/$(RUSTC_TARGET_NAME)/$(MT7996APP_CARGO_OUTPUT_DIR)/mt7996app \
		$(TARGET_DIR)/usr/sbin/mt7996app
endef

$(eval $(cargo-package))
