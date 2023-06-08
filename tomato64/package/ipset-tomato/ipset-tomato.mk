################################################################################
#
# ipset-tomato
#
################################################################################

IPSET_TOMATO_VERSION = 6.38
IPSET_TOMATO_SOURCE = ipset-$(IPSET_TOMATO_VERSION).tar.bz2
IPSET_TOMATO_SITE = http://ipset.netfilter.org
IPSET_TOMATO_DEPENDENCIES = libmnl host-pkgconf
IPSET_TOMATO_CONF_OPTS = --with-kmod=no
IPSET_TOMATO_LICENSE = GPL-2.0
IPSET_TOMATO_LICENSE_FILES = COPYING
IPSET_TOMATO_CPE_ID_VENDOR = netfilter
IPSET_TOMATO_INSTALL_STAGING = YES
IPSET_TOMATO_SELINUX_MODULES = iptables

$(eval $(autotools-package))
