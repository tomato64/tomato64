################################################################################
#
# pppd-tomato
#
################################################################################

PPPD_TOMATO_VERSION = 2.4.9
PPPD_TOMATO_SOURCE = pppd-$(PPPD_TOMATO_VERSION).tar.gz
PPPD_TOMATO_SITE = $(call github,paulusmack,ppp,ppp-$(PPPD_TOMATO_VERSION))
PPPD_TOMATO_LICENSE = LGPL-2.0+, LGPL, BSD-4-Clause, BSD-3-Clause, GPL-2.0+
PPPD_TOMATO_LICENSE_FILES = \
	pppd/tdb.c pppd/plugins/pppoatm/COPYING \
	pppdump/bsd-comp.c pppd/ccp.c pppd/plugins/passprompt.c
PPPD_TOMATO_CPE_ID_VENDOR = point-to-point_protocol_project
PPPD_TOMATO_CPE_ID_PRODUCT = point-to-point_protocol
PPPD_TOMATO_SELINUX_MODULES = ppp

PPPD_TOMATO_MAKE_OPTS = HAVE_INET6=y

ifeq ($(BR2_PACKAGE_OPENSSL),y)
PPPD_TOMATO_DEPENDENCIES += openssl
PPPD_TOMATO_MAKE_OPTS += USE_EAPTLS=y
else
PPPD_TOMATO_MAKE_OPTS += \
	USE_CRYPT=y \
	USE_EAPTLS=
endif

PPPD_TOMATO_INSTALL_STAGING = YES
PPPD_TOMATO_TARGET_BINS = chat pppd
PPPD_TOMATO_RADIUS_CONF = \
	dictionary dictionary.ascend dictionary.compat \
	dictionary.merit dictionary.microsoft \
	issue port-id-map realms servers radiusclient.conf

ifeq ($(BR2_TOOLCHAIN_HEADERS_AT_LEAST_5_15),y)
define PPPD_TOMATO_DROP_IPX
	$(SED) 's/-DIPX_CHANGE//' $(PPPD_TOMATO_DIR)/pppd/Makefile.linux
endef
#PPPD_TOMATO_POST_EXTRACT_HOOKS += PPPD_TOMATO_DROP_IPX
endif

define PPPD_TOMATO_CONFIGURE_CMDS
	( cd $(@D); $(TARGET_MAKE_ENV) ./configure --prefix=/usr --sysconfdir=/tmp )
endef

define PPPD_TOMATO_BUILD_CMDS
	$(TARGET_MAKE_ENV) $(MAKE) CC="$(TARGET_CC)" COPTS="$(TARGET_CFLAGS)" \
		-C $(@D) $(PPPD_TOMATO_MAKE_OPTS)
endef

define PPPD_TOMATO_INSTALL_TARGET_CMDS
	for sbin in $(PPPD_TOMATO_TARGET_BINS); do \
		$(INSTALL) -D $(PPPD_TOMATO_DIR)/$$sbin/$$sbin \
			$(TARGET_DIR)/usr/sbin/$$sbin; \
	done
	$(INSTALL) -D $(PPPD_TOMATO_DIR)/pppd/plugins/pppol2tp/pppol2tp.so \
		$(TARGET_DIR)/usr/lib/pppd/pppol2tp.so
endef

define PPPD_TOMATO_INSTALL_STAGING_CMDS
	$(TARGET_MAKE_ENV) $(MAKE) INSTROOT=$(STAGING_DIR)/ -C $(@D) $(PPPD_TOMATO_MAKE_OPTS) install-devel
endef

$(eval $(generic-package))
