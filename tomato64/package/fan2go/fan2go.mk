################################################################################
#
# fan2go
#
################################################################################

FAN2GO_VERSION = 0.9.0
FAN2GO_SITE = $(call github,markusressel,fan2go,$(FAN2GO_VERSION))
FAN2GO_LICENSE = BSD-3-Clause
FAN2GO_LICENSE_FILES = LICENSE
FAN2GO_DEPENDENCIES = lm-sensors

$(eval $(golang-package))
