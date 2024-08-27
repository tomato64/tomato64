################################################################################
#
# platform
#
################################################################################

ifeq ($(BR2_PACKAGE_PLATFORM_X86_64),y)
export PLATFORM_X86_64=y
endif

ifeq ($(BR2_PACKAGE_PLATFORM_MT6000),y)
export PLATFORM_MT6000=y
endif

$(eval $(generic-package))
