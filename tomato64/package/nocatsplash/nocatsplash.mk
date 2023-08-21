################################################################################
#
# nocatsplash
#
################################################################################

NOCATSPLASH_VERSION = 0e9300df2a342cb213b4636b4dc1b45eabb9ac75
NOCATSPLASH_SITE = $(call github,tomato64,nocatsplash,$(NOCATSPLASH_VERSION))
NOCATSPLASH_LICENSE = GPLv2+
NOCATSPLASH_DEPENDENCIES = libglib2

NOCATSPLASH_CONF_ENV += NC_CONF_PATH="/"
NOCATSPLASH_CONF_ENV += CFLAGS="$(TARGET_CFLAGS) -I$(STAGING_DIR)/usr/include/glib-2.0 -I$(STAGING_DIR)/usr/lib/glib-2.0/include -lglib-2.0"
NOCATSPLASH_CONF_ENV += CPPFLAGS="$(TARGET_CPPFLAGS) -I$(STAGING_DIR)/usr/include/glib-2.0 -I$(STAGING_DIR)/usr/lib/glib-2.0/include -lglib-2.0"

NOCATSPLASH_CONF_OPTS = \
	--with-firewall=iptables \
	--with-glib-prefix="$(STAGING_DIR)" \
	--localstatedir=/var \
	--sysconfdir=/etc

NOCATSPLASH_MAKE_OPTS = AR="$(TARGET_AR)"

define NOCATSPLASH_INSTALL_TARGET_CMDS
	mkdir -p $(TARGET_DIR)/usr/libexec/nocat
	$(INSTALL) -D $(@D)/src/splashd $(TARGET_DIR)/usr/sbin
	$(INSTALL) -D $(@D)/libexec/iptables/* $(TARGET_DIR)/usr/libexec/nocat
endef

$(eval $(autotools-package))
