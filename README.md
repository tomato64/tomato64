Tomato64 is a port of tomato firmware to the x86_64 and arm64 architectures.

Supported devices:

| Platform | Device |
|----------|--------|
| **x86_64** | x86_64_v2 (UEFI) |
| **x86_64** | x86_64_v1 (BIOS) |
| **Mediatek** | GL.iNet GL-MT6000/Flint 2 |
| **Mediatek** | Banana Pi BPI-R3 |
| **Mediatek** | Banana Pi BPI-R3 Mini |
| **Rockchip** | NanoPi R6S |
| **Broadcom** | Raspberry Pi 4 |

---

To build Tomato64 use Debian 12 or 13 and run:
```sh
apt install bc build-essential cpio file git libncurses-dev rsync unzip wget

git clone https://github.com/tomato64/tomato64.git
cd tomato64

# x86_64
make (x86_64_v2 uefi)
make legacy (x86_64_v1 bios)

# Mediatek
make mt6000 (GL.iNet GL-MT6000/Flint 2)
make bpi-r3 (Banana Pi BPI-R3)
make bpi-r3-mini (Banana Pi BPI-R3 Mini)

# Rockchip
make r6s (NanoPi R6S)

# Broadcom
make rpi4 (Raspberry Pi 4)

make distclean (between builds)
```

The software sources are downloaded to `~/buildroot-src/` and will be used in subsequent builds instead of being redownloaded. The build system creates a cache at `~/.buildroot-ccache` which is used to speed up later builds. Plan on allocating 50GB+ disk space to compile all variants.

The resulting images are found in `./src/buildroot/output/images`
