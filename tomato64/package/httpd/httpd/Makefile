ALL_CPPFLAGS = -fPIC
ALL_CPPFLAGS += -std=gnu17
ALL_CPPFLAGS += -DTOMATO64
ALL_CPPFLAGS += -DUSE_OPENSSL
ALL_CPPFLAGS += -I.
ALL_CPPFLAGS += -I$(BR2_EXTERNAL_TOMATO64_PATH)/package/libshared/shared
ALL_CPPFLAGS += -I$(BR2_EXTERNAL_TOMATO64_PATH)/package/libshared/shared/include
ALL_CPPFLAGS += -I$(BR2_EXTERNAL_TOMATO64_PATH)/package/libshared/shared/common/include
ALL_CPPFLAGS += -I$(BR2_EXTERNAL_TOMATO64_PATH)/package/libshared/shared/bcmwifi/include
ALL_CPPFLAGS += $(CPPFLAGS)

ifeq ($(PLATFORM_X86_64),y)
ALL_CPPFLAGS += -DTOMATO64_X86_64
endif

ifeq ($(PLATFORM_ARM64),y)
ALL_CPPFLAGS += -DTOMATO64_ARM64
endif

ifeq ($(PLATFORM_MT6000),y)
ALL_CPPFLAGS += -DTOMATO64_MT6000
endif

ifeq ($(PLATFORM_BPIR3),y)
ALL_CPPFLAGS += -DTOMATO64_BPIR3
endif

ifeq ($(PLATFORM_BPIR3MINI),y)
ALL_CPPFLAGS += -DTOMATO64_BPIR3MINI
endif

ifeq ($(PLATFORM_WIFI),y)
ALL_CPPFLAGS += -DTOMATO64_WIFI
endif

ALL_CFLAGS = -fPIC
ALL_CFLAGS += $(CFLAGS)

ALL_LDFLAGS = -fPIC
ALL_LDFLAGS += -Wl,--start-group -lmssl -lssl -lcrypto -lshared -lnvram -Wl,--end-group
ALL_LDFLAGS += $(LDFLAGS)

ifeq ($(TOMATO_EXPERIMENTAL),1)
ALL_CFLAGS += -DDEBUG_LOGMSG
endif

OBJS  = httpd.o cgi.o tomato.o version.o misc.o dhcp.o upgrade.o \
	traceping.o parser.o upnp.o ctnf.o nvram.o log.o webio.o \
	wl.o devlist.o ddns.o config.o bwm.o utf8.o cpuinfo.o \
	iptraffic.o #blackhole.o

#ifeq ($(TCONFIG_USB),y)
OBJS += usb.o
OBJS += wwan.o
#endif

#ifeq ($(TCONFIG_NOCAT),y)
OBJS += nocat.o
#endif

#ifeq ($(TCONFIG_OPENVPN),y)
OBJS += openvpn.o
#endif

#ifeq ($(TCONFIG_PPTPD),y)
OBJS += pptpd.o
#endif

ifeq ($(TCONFIG_WIREGUARD),y)
OBJS += wireguard.o
endif

#ifeq ($(TCONFIG_IPERF),y)
OBJS += iperf.o
#endif

all: httpd

httpd: $(OBJS)
	@echo " [httpd] CC $@"
	@$(CC) $(ALL_CPPFLAGS) $(ALL_CFLAGS) -o $@ $(OBJS) $(ALL_LDFLAGS)
ifneq ($(TCONFIG_BCMARM),y)
	$(SIZECHECK)
	$(CPTMP)
endif

install:
	@echo " [httpd] Installing to $(INSTALLDIR)"
	@install -D httpd $(INSTALLDIR)/usr/sbin/httpd
	@$(STRIP) $(INSTALLDIR)/usr/sbin/httpd
	@chmod 0500 $(INSTALLDIR)/usr/sbin/httpd

clean:
	rm -f httpd *.o .*.depend

size: httpd
ifneq ($(TCONFIG_BCMARM),y)
	mipsel-uclibc-nm --print-size --size-sort httpd
endif
	@true

%.o: %.c
	@echo " [httpd] CC $@"
	@$(CC) $(ALL_CPPFLAGS) $(ALL_CFLAGS) -o $@ -c $<
