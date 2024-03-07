################################################################################
#
# dhcpv6-tomato
#
################################################################################

DHCPV6_TOMATO_VERSION = a183c2a9a7eba5b2e77525af7738b2af854ebfc7
DHCPV6_TOMATO_SITE = $(call github,tomato64,dhcpv6,$(DHCPV6_TOMATO_VERSION))
DHCPV6_TOMATO_LICENSE = MIT
DHCPV6_TOMATO_DEPENDENCIES = libshared libnvram
DHCPV6_TOMATO_MAKE=$(MAKE1)

DHCPV6_TOMATO_CONF_ENV += CFLAGS="$(TARGET_CFLAGS) -D_GNU_SOURCE -DTOMATO -DTOMATO64 -DHAVE_CLOEXEC -DNEED_DEBUG \
-I$(BR2_EXTERNAL_TOMATO64_PATH)/package/libshared/shared \
-I$(BR2_EXTERNAL_TOMATO64_PATH)/package/libshared/shared/include \
-I$(BR2_EXTERNAL_TOMATO64_PATH)/package/libshared/shared/common/include \
-I$(BR2_EXTERNAL_TOMATO64_PATH)/package/libshared/shared/bcmwifi/include"

DHCPV6_TOMATO_CONF_ENV += LDFLAGS="$(TARGET_LDFLAGS) -lshared -lnvram"

DHCPV6_TOMATO_CONF_ENV += \
        ac_cv_func_setpgrp_void=yes

DHCPV6_TOMATO_CONF_OPTS = \
	--prefix= \
	--with-localdbdir=/var

define DHCPV6_TOMATO_INSTALL_TARGET_CMDS
	$(INSTALL) -D $(@D)/dhcp6c $(TARGET_DIR)/usr/sbin
endef

$(eval $(autotools-package))
