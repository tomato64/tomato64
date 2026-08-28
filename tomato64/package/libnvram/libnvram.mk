################################################################################
#
# libnvram
#
# Virtual package. The implementation is chosen in Config.in; providers are
# libnvram-c (the original C library) and libnvram-rs (Rust, shared memory).
#
# Everything that needs NVRAM keeps depending on plain "libnvram", so nothing
# else in the tree has to know which one is selected.
#
################################################################################

$(eval $(virtual-package))
