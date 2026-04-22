################################################################################
#
# platform
#
################################################################################

ifeq ($(BR2_PACKAGE_PLATFORM_X86_64),y)
export PLATFORM_X86_64=y
export PLATFORM_WIFI=y
endif

ifeq ($(BR2_PACKAGE_PLATFORM_ARM64),y)
export PLATFORM_ARM64=y
export PLATFORM_WIFI=y
endif

ifeq ($(BR2_PACKAGE_PLATFORM_HAS_POWEROFF),y)
export PLATFORM_HAS_POWEROFF=y
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

ifeq ($(BR2_PACKAGE_PLATFORM_RPI4),y)
export PLATFORM_RPI4=y
endif

ifeq ($(BR2_PACKAGE_PLATFORM_R6S),y)
export PLATFORM_R6S=y
endif

ifeq ($(BR2_PACKAGE_PLATFORM_R76S),y)
export PLATFORM_R76S=y
endif

ifeq ($(BR2_PACKAGE_PLATFORM_R5S),y)
export PLATFORM_R5S=y
endif

ifeq ($(BR2_PACKAGE_PLATFORM_ARM),y)
export PLATFORM_ARM=y
export PLATFORM_WIFI=y
endif

ifeq ($(BR2_PACKAGE_PLATFORM_BCM53XX),y)
export PLATFORM_BCM53XX=y
endif

$(eval $(generic-package))
