################################################################################
#
# ucode-mod-lvgl
#
################################################################################

UCODE_MOD_LVGL_VERSION = b3ea3d46d5fa0eebeeb80f27b896dcdb3d3bdc77
UCODE_MOD_LVGL_SITE = $(call github,blogic,ucode-mod-lvgl,$(UCODE_MOD_LVGL_VERSION))
UCODE_MOD_LVGL_LICENSE = GPL-2.0
UCODE_MOD_LVGL_LICENSE_FILES = LICENSE.txt
UCODE_MOD_LVGL_DEPENDENCIES = lvgl ucode libubox libdrm

$(eval $(cmake-package))
