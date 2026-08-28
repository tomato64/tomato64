################################################################################
#
# libnvram-rs
#
# Rust implementation of libnvram. ABI-compatible drop-in for the C package;
# see package/libnvram/Config.in for the choice between them.
#
################################################################################

LIBNVRAM_RS_VERSION = 237bd43725d147dca646071827a31d8d2ad374ad
LIBNVRAM_RS_SITE = $(call github,tomato64,libnvram-rs,$(LIBNVRAM_RS_VERSION))
LIBNVRAM_RS_LICENSE = MIT
LIBNVRAM_RS_LICENSE_FILES = LICENSE
LIBNVRAM_RS_INSTALL_STAGING = YES
LIBNVRAM_RS_PROVIDES = libnvram

LIBNVRAM_RS_CARGO_ENV = \
	CARGO_PROFILE_RELEASE_OPT_LEVEL="z" \
	CARGO_PROFILE_RELEASE_LTO="true" \
	CARGO_PROFILE_RELEASE_CODEGEN_UNITS="1"

LIBNVRAM_RS_RUSTFLAGS = -C target-feature=-crt-static

# pkg-cargo.mk sets this same variable on arm to work around
# https://github.com/rust-lang/compiler-builtins/issues/420. Our assignment
# replaces theirs wholesale, so carry their flag too rather than lose it.
ifeq ($(NORMALIZED_ARCH),arm)
LIBNVRAM_RS_RUSTFLAGS += -Clink-arg=-Wl,--allow-multiple-definition
endif

LIBNVRAM_RS_CARGO_ENV += \
	CARGO_TARGET_$(call UPPERCASE,$(RUSTC_TARGET_NAME))_RUSTFLAGS="$(LIBNVRAM_RS_RUSTFLAGS)"

ifeq ($(BR2_ENABLE_DEBUG),y)
LIBNVRAM_RS_CARGO_OUTPUT_DIR = debug
else
LIBNVRAM_RS_CARGO_OUTPUT_DIR = release
endif

LIBNVRAM_RS_SO = \
	$(@D)/target/$(RUSTC_TARGET_NAME)/$(LIBNVRAM_RS_CARGO_OUTPUT_DIR)/libnvram.so

define LIBNVRAM_RS_INSTALL_STAGING_CMDS
	$(INSTALL) -D -m 0755 $(LIBNVRAM_RS_SO) $(STAGING_DIR)/usr/lib/libnvram.so
endef

define LIBNVRAM_RS_INSTALL_TARGET_CMDS
	$(INSTALL) -D -m 0755 $(LIBNVRAM_RS_SO) $(TARGET_DIR)/usr/lib/libnvram.so
endef

$(eval $(cargo-package))
