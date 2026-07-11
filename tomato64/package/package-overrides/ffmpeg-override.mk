################################################################################
#
# ffmpeg-override
#
# Trim ffmpeg to what minidlna actually uses: decoding and demuxing for
# metadata and album art. minidlnad links only libavformat, libavcodec
# and libavutil, so drop encoders, muxers, filters, devices and the
# ffmpeg CLI binary. Appended flags win over the earlier --enable-* in
# buildroot's ffmpeg.mk because ffmpeg's configure processes them in
# order.
#
################################################################################

FFMPEG_CONF_OPTS += \
	--disable-encoders \
	--disable-muxers \
	--disable-filters \
	--disable-avfilter \
	--disable-avdevice \
	--disable-indevs \
	--disable-outdevs \
	--disable-ffmpeg
