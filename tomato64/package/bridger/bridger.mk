################################################################################
#
# bridger
#
################################################################################

BRIDGER_VERSION = 2a2d1261d12e3054b9e3f9e134f51048bb1ba1ad
BRIDGER_SITE = https://github.com/nbd168/bridger.git
BRIDGER_SITE_METHOD = git
BRIDGER_LICENSE = GPL-2.0
BRIDGER_DEPENDENCIES = libbpf libubox libnl-tiny host-clang

# The eBPF object is an ELF file for the BPF machine, not for the target CPU,
# so buildroot's check-bin-arch would reject it.
BRIDGER_BIN_ARCH_EXCLUDE = /lib/bpf

BRIDGER_CFLAGS = \
	-I$(STAGING_DIR)/usr/include/libnl-tiny \
	-I$(STAGING_DIR)/usr/include

# ubus only carries bridger's optional configuration (a device blacklist and
# the local rx/tx switches). ubusd is started by the WiFi scripts, so leaving
# it out keeps the daemon independent of WiFi being up.
BRIDGER_CONF_OPTS += \
	-DCMAKE_C_FLAGS="$(TARGET_CFLAGS) $(BRIDGER_CFLAGS)" \
	-DLIBNL_LIBS=nl-tiny \
	-DUBUS_SUPPORT=OFF

# The eBPF classifier is not part of the CMake build; OpenWrt compiles it
# separately too (include/bpf.mk). Buildroot's clang is installed behind a
# wrapper that adds the cross sysroot and CPU flags, none of which apply to the
# bpf target, so call the real binary.
BRIDGER_CLANG = $(HOST_DIR)/bin/clang.br_real
BRIDGER_BPF_TARGET = bpf$(if $(filter BIG,$(call qstrip,$(BR2_ENDIAN))),eb,el)

# bridger-bpf.c includes the kernel UAPI headers as <uapi/linux/...>, the way
# they are laid out in a kernel source tree. Point that prefix at the sanitized
# headers in staging, which is also where libbpf put <bpf/bpf_helpers.h>.
#
# -g is required, not optional: the maps are BTF-defined, and libbpf reads
# their layout from the .BTF section. llvm-strip --strip-debug then drops the
# DWARF and keeps .BTF.
define BRIDGER_BUILD_BPF
	mkdir -p $(@D)/bpf-include
	ln -sfn $(STAGING_DIR)/usr/include $(@D)/bpf-include/uapi
	$(BRIDGER_CLANG) -g -O2 -target $(BRIDGER_BPF_TARGET) -mcpu=v3 \
		-nostdinc -ffreestanding \
		-isystem $$($(BRIDGER_CLANG) -print-resource-dir)/include \
		-include $(BRIDGER_PKGDIR)/bpf-compat.h \
		-I$(@D)/bpf-include \
		-isystem $(STAGING_DIR)/usr/include \
		-I$(@D) \
		-fno-stack-protector -Wall \
		-Wno-unused-value -Wno-pointer-sign \
		-Wno-compare-distinct-pointer-types \
		-Wno-unused-variable -Wno-unused-label \
		-c $(@D)/bridger-bpf.c -o $(@D)/bridger-bpf.o
	$(HOST_DIR)/bin/llvm-strip --strip-debug $(@D)/bridger-bpf.o
endef
BRIDGER_POST_BUILD_HOOKS += BRIDGER_BUILD_BPF

# /lib/bpf/bridger-bpf.o is the path compiled into the daemon (bridger.h).
define BRIDGER_INSTALL_BPF
	$(INSTALL) -D -m 0644 $(@D)/bridger-bpf.o $(TARGET_DIR)/lib/bpf/bridger-bpf.o
endef
BRIDGER_POST_INSTALL_TARGET_HOOKS += BRIDGER_INSTALL_BPF

$(eval $(cmake-package))
