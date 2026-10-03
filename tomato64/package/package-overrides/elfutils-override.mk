################################################################################
#
# elfutils-override
#
# elfutils is only in the image because libbpf (for bridger) links libelf.
# Its "make install" also ships libdw (~765KB) and libasm (~30KB), which
# nothing on the router uses, so remove them from the target. They stay in
# staging, so anything that links them at build time still builds - but a
# package that needs libdw or libasm at run time must drop this hook.
#
################################################################################

define ELFUTILS_REMOVE_UNUSED_LIBS
	rm -f $(TARGET_DIR)/usr/lib/libdw.so* $(TARGET_DIR)/usr/lib/libdw-*.so
	rm -f $(TARGET_DIR)/usr/lib/libasm.so* $(TARGET_DIR)/usr/lib/libasm-*.so
endef
ELFUTILS_POST_INSTALL_TARGET_HOOKS += ELFUTILS_REMOVE_UNUSED_LIBS
