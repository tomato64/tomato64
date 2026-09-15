# Shared symbol directory for out-of-tree kernel modules, mirroring OpenWrt's
# PKG_SYMVERS_DIR (include/kernel.mk): every package that builds kernel modules
# drops its Module.symvers here after building, and every such build is handed
# whatever has been collected so far.
TOMATO64_SYMVERS_DIR = $(BUILD_DIR)/symvers

tomato64-extra-symvers = $$(ls $(TOMATO64_SYMVERS_DIR)/*.symvers 2>/dev/null | grep -v '/$(1)\.symvers$$' | tr '\n' ' ')

# Publish a package's Module.symvers. $(1) = package name, $(2) = build dir.
define tomato64-collect-symvers
	mkdir -p $(TOMATO64_SYMVERS_DIR)
	cp $(2)/Module.symvers $(TOMATO64_SYMVERS_DIR)/$(1).symvers
endef

include $(sort $(wildcard $(BR2_EXTERNAL_TOMATO64_PATH)/package/*/*.mk))
