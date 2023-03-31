################################################################################
#
# vsftpd-tomato
#
################################################################################

VSFTPD_TOMATO_VERSION = 3.0.5
VSFTPD_TOMATO_SITE = https://security.appspot.com/downloads
VSFTPD_TOMATO_SOURCE = vsftpd-$(VSFTPD_TOMATO_VERSION).tar.gz
VSFTPD_TOMATO_LIBS = -lcrypt
VSFTPD_TOMATO_LICENSE = GPL-2.0
VSFTPD_TOMATO_LICENSE_FILES = COPYING
VSFTPD_TOMATO_CPE_ID_VENDOR = vsftpd_project
VSFTPD_TOMATO_SELINUX_MODULES = ftp
VSFTPD_TOMATO_DEPENDENCIES = libshared

define VSFTPD_TOMATO_DISABLE_UTMPX
	$(SED) 's/.*VSF_BUILD_UTMPX/#undef VSF_BUILD_UTMPX/' $(@D)/builddefs.h
endef

ifeq ($(BR2_PACKAGE_VSFTPD_TOMATO_UTMPX),)
VSFTPD_TOMATO_POST_CONFIGURE_HOOKS += VSFTPD_TOMATO_DISABLE_UTMPX
endif

ifeq ($(BR2_PACKAGE_OPENSSL),y)
VSFTPD_TOMATO_DEPENDENCIES += openssl host-pkgconf
VSFTPD_TOMATO_LIBS += `$(PKG_CONFIG_HOST_BINARY) --libs libssl libcrypto`
endif

ifeq ($(BR2_PACKAGE_LIBCAP),y)
VSFTPD_TOMATO_DEPENDENCIES += libcap
VSFTPD_TOMATO_LIBS += -lcap
endif

ifeq ($(BR2_PACKAGE_LINUX_PAM),y)
VSFTPD_TOMATO_DEPENDENCIES += linux-pam
VSFTPD_TOMATO_LIBS += -lpam
endif

define VSFTPD_TOMATO_BUILD_CMDS
	$(TARGET_MAKE_ENV) $(MAKE) CC="$(TARGET_CC)" CFLAGS="$(TARGET_CFLAGS)" \
		LDFLAGS="$(TARGET_LDFLAGS)" LIBS="$(VSFTPD_TOMATO_LIBS)" -C $(@D)
endef

# vsftpd won't work if the jail directory is writable, it has to be
# readable only otherwise you get the following error:
# 500 OOPS: vsftpd: refusing to run with writable root inside chroot()
# That's why we have to adjust the permissions of /home/ftp
define VSFTPD_TOMATO_INSTALL_TARGET_CMDS
	$(INSTALL) -D -m 755 $(@D)/vsftpd $(TARGET_DIR)/usr/sbin/vsftpd
	test -f $(TARGET_DIR)/etc/vsftpd.conf || \
		$(INSTALL) -D -m 644 $(@D)/vsftpd.conf \
			$(TARGET_DIR)/etc/vsftpd.conf
	$(INSTALL) -d -m 700 $(TARGET_DIR)/usr/share/empty
	$(INSTALL) -d -m 555 $(TARGET_DIR)/home/ftp
endef

$(eval $(generic-package))
