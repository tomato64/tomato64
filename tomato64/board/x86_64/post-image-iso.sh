#!/bin/bash

set -e

rm -rf $BINARIES_DIR/iso
mkdir -p $BINARIES_DIR/iso
mkdir -p "${BINARIES_DIR}/iso"/{staging/{EFI/BOOT,boot/grub/x86_64-efi,isolinux,live},tmp}

cp $BINARIES_DIR/bzImage $BINARIES_DIR/iso/staging/live/
cp $BINARIES_DIR/rootfs.cpio.zst $BINARIES_DIR/iso/staging/live/initrd

rm -f $BINARIES_DIR/tomato64.img.zst
zstd --compress -19 $BINARIES_DIR/tomato64.img
cp $BINARIES_DIR/tomato64.img.zst $BINARIES_DIR/iso/staging/

cat <<'EOF' > "${BINARIES_DIR}/iso/staging/isolinux/isolinux.cfg"
UI vesamenu.c32

MENU TITLE Boot Menu
DEFAULT linux
TIMEOUT 30
MENU RESOLUTION 640 480
MENU COLOR border       30;44   #40ffffff #a0000000 std
MENU COLOR title        1;36;44 #9033ccff #a0000000 std
MENU COLOR sel          7;37;40 #e0ffffff #20ffffff all
MENU COLOR unsel        37;44   #50ffffff #a0000000 std
MENU COLOR help         37;40   #c0ffffff #a0000000 std
MENU COLOR timeout_msg  37;40   #80ffffff #00000000 std
MENU COLOR timeout      1;37;40 #c0ffffff #00000000 std
MENU COLOR msg07        37;40   #90ffffff #a0000000 std
MENU COLOR tabmsg       31;40   #30ffffff #00000000 std

LABEL linux
  MENU LABEL Tomato64 Live [BIOS/ISOLINUX]
  MENU DEFAULT
  KERNEL /live/bzImage
  APPEND initrd=/live/initrd boot=live net.ifnames=0

LABEL linux
  MENU LABEL Tomato64 Live [BIOS/ISOLINUX] ((Disable Microcode Loading))
  MENU DEFAULT
  KERNEL /live/bzImage
  APPEND initrd=/live/initrd boot=livenet.ifnames=0 dis_ucode_ldr

LABEL linux
  MENU LABEL Tomato64 Live [BIOS/ISOLINUX] (nomodeset)
  MENU DEFAULT
  KERNEL /live/bzImage
  APPEND initrd=/live/initrd boot=live net.ifnames=0 nomodeset
EOF

cat <<'EOF' > "${BINARIES_DIR}/iso/staging/boot/grub/grub.cfg"
insmod part_gpt
insmod part_msdos
insmod fat
insmod iso9660

insmod all_video
insmod font

set default="0"
set timeout=3

# If X has issues finding screens, experiment with/without nomodeset.

menuentry "Tomato64 Live [EFI/GRUB]" {
    search --no-floppy --set=root --label TOMATO64
    linux ($root)/live/bzImage boot=live net.ifnames=0
    initrd ($root)/live/initrd
}

menuentry "Tomato64 Live [EFI/GRUB] (Disable Microcode Loading)" {
    search --no-floppy --set=root --label TOMATO64
    linux ($root)/live/bzImage boot=live net.ifnames=0 dis_ucode_ldr
    initrd ($root)/live/initrd
}

menuentry "Tomato64 Live [EFI/GRUB] (nomodeset)" {
    search --no-floppy --set=root --label TOMATO64
    linux ($root)/live/bzImage boot=live net.ifnames=0 nomodeset
    initrd ($root)/live/initrd
}

set menu_color_normal=light-cyan/dark-gray
set menu_color_highlight=dark-gray/light-cyan
set color_normal=white/black
set color_highlight=black/white
EOF

cp $BINARIES_DIR/iso/staging/boot/grub/grub.cfg $BINARIES_DIR/iso/staging/EFI/BOOT/


cat <<'EOF' > "${BINARIES_DIR}/iso/tmp/grub-embed.cfg"
if ! [ -d "$cmdpath" ]; then
    # On some firmware, GRUB has a wrong cmdpath when booted from an optical disc.
    # https://gitlab.archlinux.org/archlinux/archiso/-/issues/183
    if regexp --set=1:isodevice '^(\([^)]+\))\/?[Ee][Ff][Ii]\/[Bb][Oo][Oo][Tt]\/?$' "$cmdpath"; then
        cmdpath="${isodevice}/EFI/BOOT"
    fi
fi
configfile "${cmdpath}/grub.cfg"
EOF

cp $BINARIES_DIR/syslinux/isolinux.bin $BINARIES_DIR/iso/staging/isolinux/
cp $HOST_DIR/share/syslinux/*.c32 $BINARIES_DIR/iso/staging/isolinux/

GRUB2_DIR=$BUILD_DIR/$(ls $BUILD_DIR --ignore='host*' | grep grub2)

grub-mkstandalone -O i386-efi \
    --directory "$GRUB2_DIR/build-i386-efi/grub-core" \
    --modules="part_gpt part_msdos fat iso9660 all_video font" \
    --locales="" \
    --themes="" \
    --fonts="" \
    --output="$BINARIES_DIR/iso/staging/EFI/BOOT/BOOTIA32.EFI" \
    "boot/grub/grub.cfg=$BINARIES_DIR/iso/tmp/grub-embed.cfg"

grub-mkstandalone -O x86_64-efi \
    --directory "$GRUB2_DIR/build-x86_64-efi/grub-core" \
    --modules="part_gpt part_msdos fat iso9660 all_video font" \
    --locales="" \
    --themes="" \
    --fonts="" \
    --output="$BINARIES_DIR/iso/staging/EFI/BOOT/BOOTx64.EFI" \
    "boot/grub/grub.cfg=$BINARIES_DIR/iso/tmp/grub-embed.cfg"

(cd "$BINARIES_DIR/iso/staging" && \
    dd if=/dev/zero of=efiboot.img bs=1M count=20 && \
    mkfs.vfat efiboot.img && \
    mmd -i efiboot.img ::/EFI ::/EFI/BOOT && \
    mcopy -vi efiboot.img \
        "$BINARIES_DIR/iso/staging/EFI/BOOT/BOOTIA32.EFI" \
        "$BINARIES_DIR/iso/staging/EFI/BOOT/BOOTx64.EFI" \
        "$BINARIES_DIR/iso/staging/boot/grub/grub.cfg" \
        ::/EFI/BOOT/
)

xorriso \
    -as mkisofs \
    -iso-level 3 \
    -o "$BINARIES_DIR/tomato64.iso" \
    -full-iso9660-filenames \
    -volid "TOMATO64" \
    --mbr-force-bootable -partition_offset 16 \
    -joliet -joliet-long -rational-rock \
    -isohybrid-mbr $HOST_DIR/share/syslinux/isohdpfx.bin \
    -eltorito-boot \
        isolinux/isolinux.bin \
        -no-emul-boot \
        -boot-load-size 4 \
        -boot-info-table \
        --eltorito-catalog isolinux/isolinux.cat \
    -eltorito-alt-boot \
        -e --interval:appended_partition_2:all:: \
        -no-emul-boot \
        -isohybrid-gpt-basdat \
    -append_partition 2 C12A7328-F81F-11D2-BA4B-00A0C93EC93B $BINARIES_DIR/iso/staging/efiboot.img \
    "$BINARIES_DIR/iso/staging"
