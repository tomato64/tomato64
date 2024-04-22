################################################################################
#
# tinc-1.1
#
################################################################################

TINC11_VERSION = d9e42faa6a7f4da98502be44566605a01a16a637
TINC11_SITE = $(call github,gsliepen,tinc,$(TINC11_VERSION))
TINC11_SOURCE = tinc-$(TINC11_VERSION).tar.gz
TINC11_DEPENDENCIES = lzo openssl zlib lz4
TINC11_LICENSE = GPL-2.0+ with OpenSSL exception
TINC11_LICENSE_FILES = COPYING COPYING.README
TINC11_CPE_ID_VENDOR = tinc-vpn
TINC11_INSTALL_STAGING = YES

$(eval $(meson-package))
