################################################################################
#
# pppd-tomato
#
################################################################################

# The tarball provided at https://download.samba.org/pub/ppp/ does not
# include the license files yet so we use the github tarball.
PPPD_TOMATO_VERSION = 2.5.2
PPPD_TOMATO_SOURCE = pppd-$(PPPD_TOMATO_VERSION).tar.gz
PPPD_TOMATO_SITE = $(call github,ppp-project,ppp,v$(PPPD_TOMATO_VERSION))
PPPD_TOMATO_LICENSE = LGPL-2.0+, LGPL, BSD-4-Clause, BSD-3-Clause, GPL-2.0+
PPPD_TOMATO_LICENSE_FILES = LICENSE.BSD LICENSE.GPL-2
PPPD_TOMATO_CPE_ID_VENDOR = point-to-point_protocol_project
PPPD_TOMATO_CPE_ID_PRODUCT = point-to-point_protocol
PPPD_TOMATO_SELINUX_MODULES = ppp
PPPD_TOMATO_AUTORECONF = YES
PPPD_TOMATO_INSTALL_STAGING = YES

# hack
PPPD_TOMATO_CONF_ENV += CFLAGS="$(TARGET_CFLAGS) -std=gnu17 -Wno-return-mismatch -Wno-implicit-function-declaration -Wno-incompatible-pointer-types"

PPPD_TOMATO_CONF_OPTS = \
		--prefix=/usr \
		--sysconfdir=/tmp \
		--with-plugin-dir=/usr/lib/pppd \
		--with-runtime-dir=/var/run \
		--with-logfile-dir=/var/log \
		--with-system-ca-path=/etc/ssl/certs \
		--enable-microsoft-extensions \
		--enable-plugins \
		--disable-openssl-engine \
		--without-openssl \
		--disable-eaptls \
		--disable-peap \
		--disable-systemd \
		--without-pam \
		--without-pcap \
		--without-srp \
		--without-atm \
		--disable-cbcp \
		--disable-mslanman \
		--enable-multilink \
		--enable-ipv6cp

$(eval $(autotools-package))
