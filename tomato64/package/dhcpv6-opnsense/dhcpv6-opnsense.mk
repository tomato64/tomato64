################################################################################
#
# dhcpv6-opnsense
#
# WIDE-DHCPv6 client (dhcp6c) from the actively maintained OPNsense fork,
# with Tomato64 nvram/GUI integration applied via local patches.
#
################################################################################

DHCPV6_OPNSENSE_VERSION = 60c87d02c41dbfbef8552a10bb2020db29da8d04
DHCPV6_OPNSENSE_SITE = $(call github,opnsense,dhcp6c,$(DHCPV6_OPNSENSE_VERSION))
DHCPV6_OPNSENSE_LICENSE = BSD-3-Clause
DHCPV6_OPNSENSE_DEPENDENCIES = libshared libnvram
DHCPV6_OPNSENSE_MAKE = $(MAKE1)

DHCPV6_OPNSENSE_CONF_ENV += CFLAGS="$(TARGET_CFLAGS) -std=gnu17 -D_GNU_SOURCE -DTOMATO -DTOMATO64 \
-I$(BR2_EXTERNAL_TOMATO64_PATH)/package/libshared/shared \
-I$(BR2_EXTERNAL_TOMATO64_PATH)/package/libshared/shared/include \
-I$(BR2_EXTERNAL_TOMATO64_PATH)/package/libshared/shared/common/include \
-I$(BR2_EXTERNAL_TOMATO64_PATH)/package/libshared/shared/bcmwifi/include"

DHCPV6_OPNSENSE_CONF_ENV += LDFLAGS="$(TARGET_LDFLAGS)"

# libshared/libnvram are referenced from common.o/prefixconf.o, so they must be
# appended after the objects on the link line: the Makefile places $(LIBS) last.
DHCPV6_OPNSENSE_CONF_ENV += LIBS="-lshared -lnvram"

DHCPV6_OPNSENSE_CONF_ENV += \
	ac_cv_func_setpgrp_void=yes

DHCPV6_OPNSENSE_CONF_OPTS = \
	--prefix= \
	--with-localdbdir=/var

define DHCPV6_OPNSENSE_INSTALL_TARGET_CMDS
	$(INSTALL) -D $(@D)/dhcp6c $(TARGET_DIR)/usr/sbin/dhcp6c
endef

$(eval $(autotools-package))
