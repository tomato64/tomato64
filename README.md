Tomato64 is a port of tomato firmware to the x86_64 architecture.

To build Tomato64 run:
```sh
git clone https://github.com/tomato64/tomato64.git
cd tomato64
make
```
To build the live iso, first run `make tomato64_iso_defconfig` then `make`

The software sources are downloaded to `~/buildroot-src/` and will be used in subsequent builds instead of being redownloaded. The build system creates a cache at `~/.buildroot-ccache` which is used to speed up later builds.

The resulting image is found in `./src/buildroot/output/images`
