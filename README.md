# Tomato64

> A port of Tomato firmware to x86_64 and arm64 architectures.

## Supported devices

| Manufacturer | Models |
|---|---|
| **Generic PC** | x86_64_v2 (UEFI), x86_64_v1 (BIOS) |
| **GL.iNet** | Flint 4 (GL-BE14000), Flint 2 (GL-MT6000), Beryl 7 (GL-MT3600BE) |
| **Banana Pi** | BPI-R3, BPI-R3 Mini |
| **FriendlyElec** | NanoPi R5S, NanoPi R6S, NanoPi R76S |
| **Raspberry Pi** | Raspberry Pi 4 Model B |
| **Generic ARM64** | ARM64 UEFI (QEMU/Proxmox VM or UEFI hardware) |

## Building

To build Tomato64 use Debian 12 or 13 and run:
```sh
apt install bc build-essential cpio file git libncurses-dev rsync unzip wget

git clone https://github.com/tomato64/tomato64.git
cd tomato64

# Generic PC
make (x86_64_v2 uefi)
make legacy (x86_64_v1 bios)

# GL.iNet
make be14000 (Flint 4 / GL-BE14000)
make mt6000 (Flint 2 / GL-MT6000)
make mt3600be (Beryl 7 / GL-MT3600BE)

# Banana Pi
make bpi-r3 (BPI-R3)
make bpi-r3-mini (BPI-R3 Mini)

# FriendlyElec
make r5s (NanoPi R5S)
make r6s (NanoPi R6S)
make r76s (NanoPi R76S)

# Raspberry Pi
make rpi4 (Raspberry Pi 4 Model B)

# Generic ARM64
make armsr (ARM64 UEFI - QEMU/Proxmox VM or UEFI hardware)

make distclean (between builds)
```

The software sources are downloaded to `~/buildroot-src/` and will be used in subsequent builds instead of being redownloaded. The build system creates a cache at `~/.buildroot-ccache` which is used to speed up later builds. Plan on allocating 50GB+ disk space to compile all variants.

The resulting images are found in `./src/buildroot/output/images`
