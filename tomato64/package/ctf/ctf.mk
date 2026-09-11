################################################################################
#
# ctf
#
################################################################################

CTF_VERSION = e5bf1dac7100d1923bd8b414e8c6ee156491c332
CTF_SITE = https://github.com/tomato64/ctf.git
CTF_SITE_METHOD = git
CTF_LICENSE = GPL-2.0
CTF_LICENSE_FILES = COPYING

# ctf.ko is built by the kernel-module infra from the repo's Kbuild, against
# a kernel carrying board/arm/bcm53xx/linux-patches/00003-net-cutthrough-offload.patch
# (include/linux/netdev_cutthrough.h). The infra passes $(LINUX_MAKE_FLAGS), so
# the module is stripped at modules_install like in-tree ones.

$(eval $(kernel-module))
$(eval $(generic-package))
