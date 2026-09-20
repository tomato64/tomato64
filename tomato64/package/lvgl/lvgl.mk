################################################################################
#
# lvgl
#
################################################################################

LVGL_VERSION = 9.3.0
LVGL_SITE = $(call github,lvgl,lvgl,v$(LVGL_VERSION))
LVGL_LICENSE = MIT
LVGL_LICENSE_FILES = LICENCE.txt
LVGL_INSTALL_STAGING = YES
LVGL_INSTALL_TARGET = NO
LVGL_DEPENDENCIES = libdrm

LVGL_CFLAGS = $(TARGET_CFLAGS) -I$(STAGING_DIR)/usr/include/libdrm

LVGL_CONF_OPTS = \
	-DCMAKE_C_FLAGS="$(LVGL_CFLAGS)" \
	-DBUILD_SHARED_LIBS=OFF \
	-DCMAKE_POSITION_INDEPENDENT_CODE=ON \
	-DLV_BUILD_CONF_DIR=$(LVGL_PKGDIR)/files \
	-DCONFIG_LV_BUILD_DEMOS=OFF \
	-DCONFIG_LV_BUILD_EXAMPLES=OFF \
	-DCONFIG_LV_USE_THORVG_INTERNAL=OFF \
	-DCONFIG_LV_USE_PRIVATE_API=ON

$(eval $(cmake-package))
