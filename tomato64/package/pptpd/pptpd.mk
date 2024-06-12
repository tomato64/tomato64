################################################################################
#
# pptpd
#
################################################################################

PPTPD_VERSION = 1.5.0
PPTPD_SITE = https://sourceforge.net/projects/poptop/files/pptpd/pptpd-$(PPTPD_VERSION)
PPTPD_LICENSE = MIT
PPTPD_LICENSE_FILES = COPYING
PPTPD_DEPENDENCIES = pppd-tomato
PPTPD_AUTORECONF = YES
PPTPD_MAKE_OPTS += CPPFLAGS="$(TARGET_CPPFLAGS) -I$(PPPD_TOMATO_DIR)/include"
PPTPD_CONF_OPTS += --enable-bcrelay

define PPTPD_LINK_PPPD_SOURCE
        ln -sf $(PPPD_TOMATO_DIR)/pppd $(PPTPD_DIR)/plugins/pppd
endef
PPTPD_POST_EXTRACT_HOOKS += PPTPD_LINK_PPPD_SOURCE

define PPTPD_INSTALL_TARGET_CMDS
	$(INSTALL) -D $(@D)/pptpd $(TARGET_DIR)/usr/sbin
	$(INSTALL) -D $(@D)/bcrelay $(TARGET_DIR)/usr/sbin
	$(INSTALL) -D $(@D)/pptpctrl $(TARGET_DIR)/usr/sbin
endef

$(eval $(autotools-package))
