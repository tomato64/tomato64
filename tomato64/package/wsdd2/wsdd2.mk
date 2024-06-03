################################################################################
#
# wsdd2
#
################################################################################

WSDD2_VERSION = 1.8.7
WSDD2_SITE = https://github.com/Netgear/wsdd2/archive/refs/tags
WSDD2_SOURCE = $(WSDD2_VERSION).tar.gz
WSDD2_INSTALL_STAGING = YES
WSDD2_LICENSE = GPL-3.0+

WSDD2_CFLAGS = "$(TARGET_CFLAGS) -DTOMATO -DTOMATO64 -Wno-int-conversion"
# gcc-14 fix needed

define WSDD2_BUILD_CMDS
	$(MAKE) $(TARGET_CONFIGURE_OPTS) CFLAGS=$(WSDD2_CFLAGS) -C $(@D)
endef

define WSDD2_INSTALL_STAGING_CMDS
	$(INSTALL) -D $(@D)/wsdd2 $(STAGING_DIR)/usr/sbin
endef

define WSDD2_INSTALL_TARGET_CMDS
	$(INSTALL) -D $(@D)/wsdd2 $(TARGET_DIR)/usr/sbin
endef

$(eval $(generic-package))
