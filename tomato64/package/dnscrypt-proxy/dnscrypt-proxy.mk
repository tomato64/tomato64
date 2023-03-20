################################################################################
#
# dnscrypt-proxy
#
################################################################################

DNSCRYPT_PROXY_VERSION = 1.9.5
DNSCRYPT_PROXY_SOURCE = dnscrypt-proxy-$(DNSCRYPT_PROXY_VERSION).tar.gz
DNSCRYPT_PROXY_SITE = https://ftp.nluug.nl/os/Linux/distr/fatdog/source/800
DNSCRYPT_PROXY_LICENSE = MIT
DNSCRYPT_PROXY_LICENSE_FILES = COPYING
DNSCRYPT_PROXY_INSTALL_STAGING = YES
DNSCRYPT_PROXY_DEPENDENCIES = libsodium
DNSCRYPT_PROXY_AUTORECONF = YES

ifeq ($(BR2_TOOLCHAIN_SUPPORTS_PIE),)
DNSCRYPT_PROXY_CONF_OPTS += --disable-pie
endif

DNSCRYPT_PROXY_CONF_OPTS += --enable-static
$(eval $(autotools-package))
