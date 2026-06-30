#!/bin/sh

set -e
set -x

ZIMAGE="$BINARIES_DIR/zImage"
SQUASHFS="$BINARIES_DIR/rootfs.squashfs"

# NAND parameters (BCM53XX: 128KiB erase block, 2048 byte page)
BLOCKSIZE="128KiB"
PAGESIZE="2048"

if [ ! -f "$ZIMAGE" ]; then
	echo "ERROR: zImage not found at $ZIMAGE"
	exit 1
fi

if [ ! -f "$SQUASHFS" ]; then
	echo "ERROR: rootfs.squashfs not found at $SQUASHFS"
	exit 1
fi

# Step 1: Create UBI image containing squashfs rootfs + overlay volume
# Matches OpenWrt's append-ubi pipeline
ROOTFS_SIZE=$(stat -c%s "$SQUASHFS")
ROOTFS_SIZE_KIB=$(( (ROOTFS_SIZE + 1023) / 1024 ))

UBINIZE_CFG="$BINARIES_DIR/ubinize.cfg"
cat > "$UBINIZE_CFG" <<EOF
[rootfs]
mode=ubi
vol_id=0
vol_type=dynamic
vol_name=rootfs
vol_size=${ROOTFS_SIZE_KIB}KiB
image=$SQUASHFS

[rootfs_data]
mode=ubi
vol_id=1
vol_type=dynamic
vol_name=rootfs_data
vol_size=1MiB
vol_flags=autoresize
EOF

UBI_IMG="$BINARIES_DIR/rootfs.ubi"
ubinize -p "$BLOCKSIZE" -m "$PAGESIZE" -o "$UBI_IMG" "$UBINIZE_CFG"

# Step 2: Create ubi_mark (EOF marker for UBI attach)
# Matches OpenWrt's ubi_mark: 0xdeadc0de. The kernel's UBI attach (patch
# 500-UBI-Detect-EOF-mark-and-erase-all-remaining-blocks) reads this as a
# PEB EC-header magic and erases every block behind it, wiping stale UBI data
# left by a prior install (otherwise UBI attach fails with "bad image sequence
# number" when a raw write / CFE / FreshTomato flash leaves old PEBs).
# Use OCTAL escapes: this script runs under /bin/sh (dash on Debian), whose
# printf does NOT support \xNN - '\xde...' there emits the literal ASCII text
# "\xde\xad\xc0\xde", producing a broken marker. \ddd octal is POSIX-portable.
UBI_MARK="$BINARIES_DIR/ubi_mark"
printf '\336\255\300\336' > "$UBI_MARK"

# Clean up any previous TRX files
rm -f $BINARIES_DIR/*.trx

# Step 3: Process each DTB to create a device-specific TRX
# BCM53XX TRX-NAND layout (matches OpenWrt trx-nand):
#   Partition 1: LZMA-compressed kernel+DTB (padded to 4MB)
#   Partition 2: UBI image (squashfs + overlay volumes)
#   Appended: ubi_mark EOF marker
for DTB in $BINARIES_DIR/*.dtb; do
	[ -f "$DTB" ] || continue

	DTB_NAME=$(basename "$DTB" .dtb)

	echo "=== Creating TRX for $DTB_NAME ==="

	# Append DTB to zImage
	cat "$ZIMAGE" "$DTB" > "$BINARIES_DIR/${DTB_NAME}-kernel.bin"

	# LZMA compress with -d16
	lzma_alone e \
		"$BINARIES_DIR/${DTB_NAME}-kernel.bin" \
		-d16 \
		"$BINARIES_DIR/${DTB_NAME}-kernel.lzma"

	# Create TRX: kernel (4MB padded) + UBI + ubi_mark
	# -a 0x20000: align to 128KB NAND erase block
	# -b 0x400000: pad kernel partition to 4MB
	# -A ubi_mark: append EOF marker after UBI
	otrx create "$BINARIES_DIR/${DTB_NAME}.trx" \
		-f "$BINARIES_DIR/${DTB_NAME}-kernel.lzma" -a 0x20000 -b 0x400000 \
		-f "$UBI_IMG" \
		-A "$UBI_MARK" -a 0x20000

	# Cleanup intermediate files
	rm -f "$BINARIES_DIR/${DTB_NAME}-kernel.bin"
	rm -f "$BINARIES_DIR/${DTB_NAME}-kernel.lzma"

	echo "=== Created $BINARIES_DIR/${DTB_NAME}.trx ==="
done

# Cleanup UBI build artifacts
rm -f "$UBINIZE_CFG" "$UBI_IMG" "$UBI_MARK"

# Publish a per-device update image. The artifact is the bare Broadcom TRX -
# the native sysupgrade container that CFE/u-boot recovery accepts AND that the
# in-system upgrade consumes (via otrx/mtd, which are length-aware). We do NOT
# wrap it in a .tzst: httpd streams the raw multipart POST body (trailing
# boundary included) into the upgrade FIFO, and modern zstd refuses to
# decompress a stream with trailing data - whereas every TRX-aware tool just
# reads the header length and ignores the trailer. One artifact, no zstd.
# Only the DTB-named build outputs (bcm4708-..., bcm47094-..., etc.) - never a
# published <device>.trx from a previous run left in a non-clean images dir.
for TRX in $BINARIES_DIR/bcm*.trx; do
	[ -f "$TRX" ] || continue

	DTB_NAME=$(basename "$TRX" .trx)
	# Extract device name (remove broadcom-bcmXXXX- prefix)
	DEVICE_NAME=$(echo "$DTB_NAME" | sed 's/^bcm[0-9]*-//')

	# Clean per-device name, e.g. netgear-r7000.trx. One generic artifact per
	# device, used for BOTH first-time install and in-system upgrade (no
	# confusing bcmXXXX- DTB prefix, no "-update" - it is not upgrade-only).
	DEVICE_IMAGE="$BINARIES_DIR/${DEVICE_NAME}.trx"
	rm -f "$DEVICE_IMAGE"
	mv "$TRX" "$DEVICE_IMAGE"

	echo "=== Created $DEVICE_IMAGE ==="
done

exit 0
