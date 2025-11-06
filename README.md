Tomato64 is a port of tomato firmware to the x86_64 and arm64 (GL-MT6000 | BPI-R3 | BPI-R3 Mini) architectures.

To build Tomato64 use Debian 12 or 13 and run:
```sh
apt install bc build-essential cpio file git libncurses-dev rsync unzip wget

git clone https://github.com/tomato64/tomato64.git
cd tomato64

make (x86_64_v2 uefi) or
make legacy (x86_64_v1 bios) or
make mt6000 (GL.iNet GL-MT6000/Flint 2) or
make bpi-r3 (Banana Pi BPI-R3) or
make bpi-r3-mini (Banana Pi BPI-R3 Mini)

make distclean (between builds)
```

The software sources are downloaded to `~/buildroot-src/` and will be used in subsequent builds instead of being redownloaded. The build system creates a cache at `~/.buildroot-ccache` which is used to speed up later builds. Plan on allocating 50GB+ disk space to compile all variants.

The resulting images are found in `./src/buildroot/output/images`
