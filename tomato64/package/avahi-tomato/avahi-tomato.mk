################################################################################
#
# avahi-tomato
#
################################################################################

AVAHI_TOMATO_VERSION = 0.8
AVAHI_TOMATO_SOURCE = avahi-$(AVAHI_TOMATO_VERSION).tar.gz
AVAHI_TOMATO_SITE = https://github.com/lathiat/avahi/releases/download/v$(AVAHI_TOMATO_VERSION)
AVAHI_TOMATO_LICENSE = LGPL-2.1+
AVAHI_TOMATO_LICENSE_FILES = LICENSE
AVAHI_TOMATO_CPE_ID_VENDOR = avahi
AVAHI_TOMATO_SELINUX_MODULES = avahi
AVAHI_TOMATO_INSTALL_STAGING = YES
AVAHI_TOMATO_AUTORECONF = YES

# CVE-2021-26720 is an issue in avahi-daemon-check-dns.sh, which is
# part of the Debian packaging and not part of upstream avahi
AVAHI_TOMATO_IGNORE_CVES += CVE-2021-26720

# 0001-Fix-NULL-pointer-crashes-from-175.patch
AVAHI_TOMATO_IGNORE_CVES += CVE-2021-36217

AVAHI_TOMATO_CONF_ENV = \
	avahi_cv_sys_cxx_works=yes \
	DATADIRNAME=share

AVAHI_TOMATO_CONF_OPTS = \
	--disable-introspection \
	--disable-glib --disable-libevent --disable-gobject \
	--disable-qt3 --disable-qt4 --disable-qt5 --disable-gtk --disable-gtk3 \
	--disable-dbus --disable-gdbm --disable-python --disable-python-dbus \
	--disable-mono --disable-monodoc --disable-autoipd \
	--disable-doxygen-doc --disable-manpages --disable-xmltoman \
	--with-xml=expat \
	--with-avahi-user=nobody --with-avahi-group=nobody \
	--disable-stack-protector \
	--with-distro=none --disable-pygobject \
	avahi_runtime_dir=/var/run servicedir=/etc/avahi/services \

AVAHI_TOMATO_DEPENDENCIES = host-pkgconf $(TARGET_NLS_DEPENDENCIES)
AVAHI_TOMATO_DEPENDENCIES += libdaemon
AVAHI_TOMATO_DEPENDENCIES += expat

ifeq ($(BR2_PACKAGE_LIBCAP),y)
AVAHI_TOMATO_DEPENDENCIES += libcap
endif

AVAHI_TOMATO_CFLAGS = $(TARGET_CFLAGS)

AVAHI_TOMATO_CONF_ENV += CFLAGS="$(AVAHI_TOMATO_CFLAGS)"

AVAHI_TOMATO_MAKE_OPTS += LIBS=$(TARGET_NLS_LIBS)

$(eval $(autotools-package))
