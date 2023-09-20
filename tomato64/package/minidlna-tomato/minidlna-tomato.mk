################################################################################
#
# minidlna-tomato
#
################################################################################

MINIDLNA_TOMATO_VERSION = 1.3.3
MINIDLNA_TOMATO_SOURCE = minidlna-$(MINIDLNA_TOMATO_VERSION).tar.gz
MINIDLNA_TOMATO_SITE = https://downloads.sourceforge.net/project/minidlna/minidlna/$(MINIDLNA_TOMATO_VERSION)
MINIDLNA_TOMATO_LICENSE = GPL-2.0, BSD-3-Clause
MINIDLNA_TOMATO_LICENSE_FILES = COPYING LICENCE.miniupnpd
MINIDLNA_TOMATO_CPE_ID_VENDOR = readymedia_project
MINIDLNA_TOMATO_CPE_ID_PRODUCT = readymedia
MINIDLNA_TOMATO_SELINUX_MODULES = minidlna
MINIDLNA_TOMATO_AUTORECONF = YES

MINIDLNA_TOMATO_DEPENDENCIES = \
	$(TARGET_NLS_DEPENDENCIES) \
	ffmpeg flac libvorbis libogg libid3tag libexif jpeg sqlite \
	host-xutil_makedepend

MINIDLNA_TOMATO_CONF_OPTS = \
	--disable-static \
	--with-db-path=/tmp/minidlna \
	--with-log-path=/var/log \
	--with-os-name="Tomato64" \
	--with-os-url="https://tomato64.org/" \
	--enable-tivo

define MINIDLNA_INSTALL_SYMLINK
	ln -sf minidlnad $(TARGET_DIR)/usr/sbin/minidlna
endef

MINIDLNA_TOMATO_POST_INSTALL_TARGET_HOOKS += MINIDLNA_INSTALL_SYMLINK

$(eval $(autotools-package))
