################################################################################
#
# tinc-1.1
#
################################################################################

TINC11_VERSION = 940d15c46be812821f134fd7d6333088445b914c
TINC11_SITE = $(call github,gsliepen,tinc,$(TINC11_VERSION))
TINC11_SOURCE = tinc-$(TINC11_VERSION).tar.gz
TINC11_DEPENDENCIES = lzo openssl zlib lz4
TINC11_LICENSE = GPL-2.0+ with OpenSSL exception
TINC11_LICENSE_FILES = COPYING COPYING.README
TINC11_CPE_ID_VENDOR = tinc-vpn
TINC11_INSTALL_STAGING = YES

$(eval $(meson-package))
