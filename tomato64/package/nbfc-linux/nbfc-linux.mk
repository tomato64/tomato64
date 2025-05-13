################################################################################
#
# nbfc-linux
#
################################################################################

NBFC_LINUX_VERSION = 0.3.16
NBFC_LINUX_SITE = $(call github,nbfc-linux,nbfc-linux,$(NBFC_LINUX_VERSION))
NBFC_LINUX_LICENSE = GPL-3.0
NBFC_LINUX_DEPENDENCIES = lm-sensors
NBFC_LINUX_AUTORECONF = YES

define NBFC_LINUX_BUILD_CMDS
        $(TARGET_MAKE_ENV) $(MAKE) CC="$(TARGET_CC)" CFLAGS="$(TARGET_CFLAGS)" \
                LDFLAGS="$(TARGET_LDFLAGS)" LIBS="$(VSFTPD_TOMATO_LIBS)" -C $(@D)
endef

$(eval $(autotools-package))
