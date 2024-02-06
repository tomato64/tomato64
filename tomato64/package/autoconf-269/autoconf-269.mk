################################################################################
#
# autoconf-269
#
################################################################################

AUTOCONF_269_VERSION = 2.69
AUTOCONF_269_SOURCE = autoconf-$(AUTOCONF_269_VERSION).tar.xz
AUTOCONF_269_SITE = $(BR2_GNU_MIRROR)/autoconf

AUTOCONF_269_LICENSE = GPL-3.0+ with exceptions
AUTOCONF_269_LICENSE_FILES = COPYINGv3 COPYING.EXCEPTION

HOST_AUTOCONF_269_CONF_ENV = \
	EMACS="no" \
	ac_cv_path_M4=$(HOST_DIR)/bin/m4 \
	ac_cv_prog_gnu_m4_gnu=no

HOST_AUTOCONF_269_DEPENDENCIES = host-m4 host-libtool

HOST_AUTOCONF_269_CONF_OPTS += --prefix=$(HOST_AUTOCONF_269_DIR)/install

$(eval $(host-autotools-package))
