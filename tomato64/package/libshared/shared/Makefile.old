include ../common.mak

CFLAGS = -Os -Wall $(EXTRACFLAGS) -ffunction-sections -fdata-sections -fPIC -Wsign-compare
CFLAGS += -I$(TOP)/shared -I$(SRCBASE)/include
CFLAGS += -I$(LINUXDIR)/include
CFLAGS += -DTRAFFIC_MGMT

ifeq (2_6_36,$(LINUX_VERSION))
CFLAGS += -DAUTOFW_PORT_DEPRECATED
endif

ifeq ($(TCONFIG_BCMWL6),y)
CFLAGS += -I$(SRCBASE)/shared/bcmwifi/include
CFLAGS += -I$(SRCBASE)/common/include
else
CFLAGS += -I$(SRCBASE)/shared
endif

ifeq ($(TCONFIG_BCMSMP),y)
CFLAGS += -DSMP
endif

ifeq ($(TCONFIG_DHDAP),y)
export CFLAGS += -D__CONFIG_DHDAP__
endif

ifeq ($(TCONFIG_BCMBSD),y)
export CFLAGS += -DBCM_BSD
endif

ifeq ($(TOMATO_EXPERIMENTAL),1)
export CFLAGS += -DDEBUG_LOGMSG
endif

LDFLAGS = --gc-sections -fPIC

OBJS = shutils.o wl.o wl_linux.o linux_timer.o defaults.o id.o misc.o \
       led.o version.o base64.o files.o strings.o process.o nvparse.o

ifeq ($(TCONFIG_USB),y)
OBJS += usb.o \
        $(TOP)/busybox/util-linux/volume_id/ext.o \
        $(TOP)/busybox/util-linux/volume_id/util2.o \
        $(TOP)/busybox/util-linux/volume_id/fat.o \
        $(TOP)/busybox/util-linux/volume_id/ntfs.o \
        $(TOP)/busybox/util-linux/volume_id/exfat.o \
        $(TOP)/busybox/util-linux/volume_id/linux_swap.o
ifeq ($(TCONFIG_HFS),y)
CFLAGS += -DHFS
OBJS += $(TOP)/busybox/libbb/hash_md5_sha.o \
        $(TOP)/busybox/libbb/bb_bswap_64.o \
        $(TOP)/busybox/util-linux/volume_id/hfs.o
endif
endif

ifeq ($(CONFIG_BCMWL5),y)
OBJS += wlif_utils.o UdpLib.o bcmutils.o
OBJS += bcmwifi_channels.o
endif

ifeq ($(TCONFIG_BCM7),y)
OBJS += et_linux.o bcmwifi_channels.o
endif

vpath %.c $(SRCBASE)/shared/ $(SRCBASE)/shared/bcmwifi/src $(WLAN_ComponentSrcDirs)

all: libshared.so libshared.a

libshared.so: $(OBJS)
	@echo " [shared] LD $@"
	@$(LD) $(LDFLAGS) -shared -o $@ $^

	$(SIZECHECK)
	$(CPTMP)

libshared.a: $(OBJS)
	@echo " [shared] AR $@"
	@rm -f $@
	@$(AR) cruv $@ $^

install: all
	@echo " [shared] Installing as $(INSTALLDIR)/usr/lib/libshared.so"
	@install -d $(INSTALLDIR)/usr/lib
	@install -m 755 libshared.so $(INSTALLDIR)/usr/lib
	@$(STRIP) $(INSTALLDIR)/usr/lib/libshared.so

clean:
	rm -f *.o *.so *.a .*.depend

%.o: %.c .%.depend
	@echo " [shared] CC $@"
	@$(CC) $(CFLAGS) -o $@ -c $<

.depend: $(OBJS:%.o=%.c)
	@$(CC) $(CFLAGS) -M $^ > .depend

.%.depend: %.c
	@$(CC) $(CFLAGS) -M $< > $@

-include $(OBJS:%.o=.%.depend)
