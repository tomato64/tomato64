################################################################################
#
# platform
#
################################################################################

ifeq ($(BR2_PACKAGE_PLATFORM_X86_64),y)
export PLATFORM_X86_64=y
endif

ifeq ($(BR2_PACKAGE_PLATFORM_ARM64),y)
export PLATFORM_ARM64=y
export PLATFORM_WIFI=y
endif

ifeq ($(BR2_PACKAGE_PLATFORM_MT6000),y)
export PLATFORM_MT6000=y
endif

ifeq ($(BR2_PACKAGE_PLATFORM_BPIR3),y)
export PLATFORM_BPIR3=y
endif

ifeq ($(BR2_PACKAGE_PLATFORM_BPIR3MINI),y)
export PLATFORM_BPIR3MINI=y
endif

$(eval $(generic-package))
