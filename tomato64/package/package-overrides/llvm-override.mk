################################################################################
#
# llvm-override
#
# bridger's eBPF classifier is compiled with host-clang, which needs the BPF
# backend in host-llvm. Buildroot only adds that backend through
# BR2_PACKAGE_LLVM_BPF, which sits under the *target* LLVM package - and
# building LLVM for the router just to get a host compiler option is not on.
#
# HOST_LLVM_CONF_OPTS expands LLVM_TARGETS_TO_BUILD lazily, so appending here
# (after llvm.mk has been read) is enough.
#
################################################################################

ifeq ($(BR2_PACKAGE_BRIDGER),y)
ifneq ($(BR2_PACKAGE_LLVM_BPF),y)
LLVM_TARGETS_TO_BUILD += BPF
endif
endif
