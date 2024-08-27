ALL_CPPFLAGS = -fPIC
ALL_CPPFLAGS += -DTOMATO64
ALL_CPPFLAGS += -I.
ALL_CPPFLAGS += -Iinclude
ALL_CPPFLAGS += -Icommon/include
ALL_CPPFLAGS += -Ibcmwifi/include
ALL_CPPFLAGS += $(CPPFLAGS)

ifeq ($(PLATFORM_X86_64),y)
ALL_CPPFLAGS += -DTOMATO64_X86_64
endif

ifeq ($(PLATFORM_MT6000),y)
ALL_CPPFLAGS += -DTOMATO64_MT6000
endif

ALL_CFLAGS = -fPIC
ALL_CFLAGS += $(CFLAGS)

ALL_LDFLAGS = -fPIC
ALL_LDFLAGS += $(LDFLAGS)

ifeq ($(TOMATO_EXPERIMENTAL),1)
ALL_CFLAGS += -DDEBUG_LOGMSG
endif

OBJS = shutils.o wl.o wl_linux.o linux_timer.o defaults.o id.o misc.o \
       led.o version.o base64.o files.o strings.o process.o nvparse.o usb.o

OBJS += \
$(BUSYBOX_DIR)/util-linux/volume_id/ext.o \
$(BUSYBOX_DIR)/util-linux/volume_id/util2.o \
$(BUSYBOX_DIR)/util-linux/volume_id/fat.o \
$(BUSYBOX_DIR)/util-linux/volume_id/ntfs.o \
$(BUSYBOX_DIR)/util-linux/volume_id/exfat.o \
$(BUSYBOX_DIR)/util-linux/volume_id/linux_swap.o

all: libshared.so libshared.a

libshared.so: $(OBJS)
	@echo " [shared] LD $@"
	@$(CC) $(ALL_CPPFLAGS) $(ALL_CFLAGS) $(ALL_LDFLAGS) -shared -o $@ $^

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

%.o: %.c
	@echo " [shared] CC $@"
	@$(CC) $(ALL_CPPFLAGS) $(ALL_CFLAGS) -o $@ -c $<

genversion:
	./gen_tomato_version.h.sh

genconfig:
	./gen_tomato_config.h.sh

.PHONY: genconfig
