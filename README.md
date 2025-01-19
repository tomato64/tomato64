Tomato64 is a port of tomato firmware to the x86_64 and arm64 (GL-MT6000) architectures.

To build Tomato64 run:
```sh
git clone https://github.com/tomato64/tomato64.git
cd tomato64

make (x86_64_v2 uefi) or
make legacy (x86_64_v1 bios) or
make mt6000 (GL-MT6000/Flint 2)

make distclean (between builds)
```

The software sources are downloaded to `~/buildroot-src/` and will be used in subsequent builds instead of being redownloaded. The build system creates a cache at `~/.buildroot-ccache` which is used to speed up later builds. Tomato64 is known to build on Debian 12.

The resulting images are found in `./src/buildroot/output/images`
