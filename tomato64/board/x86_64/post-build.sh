#!/bin/sh

set -e

BOARD_DIR=$(dirname "$0")

# Detect boot strategy, EFI or BIOS
if [ -d "$BINARIES_DIR/efi-part/" ]; then
    cp -f "$BOARD_DIR/grub-efi.cfg" "$BINARIES_DIR/efi-part/EFI/BOOT/grub.cfg"
else
    cp -f "$BOARD_DIR/grub-bios.cfg" "$TARGET_DIR/boot/grub/grub.cfg"

    # Copy grub 1st stage to binaries, required for genimage
    cp -f "$TARGET_DIR/lib/grub/i386-pc/boot.img" "$BINARIES_DIR"
fi

rm -rf \
$TARGET_DIR/etc \
$TARGET_DIR/home \
$TARGET_DIR/mnt \
$TARGET_DIR/root \
$TARGET_DIR/var

mkdir -p \
$TARGET_DIR/tmp/etc \
$TARGET_DIR/tmp/home/root \
$TARGET_DIR/tmp/mnt \
$TARGET_DIR/tmp/var

ln -sf tmp/etc $TARGET_DIR/etc
ln -sf tmp/home $TARGET_DIR/home
ln -sf tmp/home/root $TARGET_DIR/root
ln -sf tmp/mnt $TARGET_DIR/mnt
ln -sf tmp/var $TARGET_DIR/var

ln -sf rc $TARGET_DIR/sbin/init
ln -sf /lib/ld-musl-x86_64.so.1 $TARGET_DIR/usr/bin/ldd
ln -sf /usr/bin/openssl $TARGET_DIR/usr/sbin/openssl

# To make buildroot genimage happy
touch $TARGET_DIR/tmp/etc/group
touch $TARGET_DIR/tmp/etc/passwd
touch $TARGET_DIR/tmp/etc/shadow
