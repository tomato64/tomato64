################################################################################
#
# ucode
#
################################################################################

UCODE_VERSION = 1a8a0bcf725520820802ad433db22d8f64fbed6c
UCODE_SITE = $(call github,jow-,ucode,$(UCODE_VERSION))
UCODE_LICENSE = GPL-2.0
UCODE_INSTALL_STAGING = YES

$(eval $(cmake-package))
