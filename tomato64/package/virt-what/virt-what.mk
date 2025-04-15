################################################################################
#
# virt-what
#
################################################################################

VIRT_WHAT_VERSION = 1.27
VIRT_WHAT_SOURCE = virt-what-$(VIRT_WHAT_VERSION).tar.gz
VIRT_WHAT_SITE = https://people.redhat.com/~rjones/virt-what/files
VIRT_WHAT_LICENSE = GPL-2.0

$(eval $(autotools-package))
