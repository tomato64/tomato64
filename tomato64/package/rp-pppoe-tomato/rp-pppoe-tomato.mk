################################################################################
#
# rp-pppoe-tomato
#
################################################################################

RP_PPPOE_TOMATO_VERSION = 3.15
RP_PPPOE_TOMATO_SOURCE = rp-pppoe-$(RP_PPPOE_TOMATO_VERSION).tar.gz
RP_PPPOE_TOMATO_SITE = https://dianne.skoll.ca/projects/rp-pppoe/download
RP_PPPOE_TOMATO_LICENSE = GPL-2.0
RP_PPPOE_TOMATO_LICENSE_FILES = doc/LICENSE
RP_PPPOE_TOMATO_CPE_ID_VENDOR = rp-pppoe_project

RP_PPPOE_TOMATO_DEPENDENCIES = pppd-tomato
RP_PPPOE_TOMATO_SUBDIR = src
RP_PPPOE_TOMATO_CONF_OPTS = --prefix=/usr --enable-plugin=$(PPPD_TOMATO_DIR)/pppd --disable-debugging

# The pppd, echo, setsid and id paths must be the ones on the
# target. Indeed, the result of these checks is used to replace
# variables in scripts that are installed in the target.
RP_PPPOE_TOMATO_CONF_ENV = \
	ac_cv_linux_kernel_pppoe=yes \
	rpppoe_cv_pack_bitfields=rev \
	ac_cv_path_PPPD=$(PPPD_TOMATO_DIR)/pppd

RP_PPPOE_TOMATO_CONF_ENV += CFLAGS="$(TARGET_CFLAGS) -I$(PPPD_TOMATO_DIR)/include"

define RP_PPPOE_TOMATO_INSTALL_TARGET_CMDS
	$(INSTALL) -D $(@D)/src/rp-pppoe.so \
		$(TARGET_DIR)/usr/lib/pppd/rp-pppoe.so
endef

$(eval $(autotools-package))
