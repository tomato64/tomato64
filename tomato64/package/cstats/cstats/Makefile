ALL_CPPFLAGS = -fPIC
ALL_CPPFLAGS += -DTOMATO64
ALL_CPPFLAGS += -I$(BR2_EXTERNAL_TOMATO64_PATH)/package/libshared/shared
ALL_CPPFLAGS += -I$(BR2_EXTERNAL_TOMATO64_PATH)/package/libshared/shared/include
ALL_CPPFLAGS += -I$(BR2_EXTERNAL_TOMATO64_PATH)/package/libshared/shared/common/include
ALL_CPPFLAGS += -I$(BR2_EXTERNAL_TOMATO64_PATH)/package/libshared/shared/bcmwifi/include
ALL_CPPFLAGS += $(CPPFLAGS)

ALL_CFLAGS = -fPIC
ALL_CFLAGS += $(CFLAGS)

ALL_LDFLAGS = -fPIC
ALL_LDFLAGS += -lshared -lnvram
ALL_LDFLAGS += $(LDFLAGS)

ifeq ($(TOMATO_EXPERIMENTAL),1)
ALL_CFLAGS += -DDEBUG_LOGMSG
endif

OBJS = cstats.o

all: cstats

cstats: $(OBJS)
	@echo " [cstats] CC -o $@"
	@$(CC) $(ALL_CPPFLAGS) $(ALL_CFLAGS) -o $@ $(OBJS) $(ALL_LDFLAGS)

clean:
	rm -f cstats .*.depend
	rm -f *.o

install: all
	@echo " [cstats] Installing..."
	install -D -m 0700 cstats $(INSTALLDIR)/bin/cstats
	$(STRIP) $(INSTALLDIR)/bin/cstats

%.o: %.c
	@echo " [cstats] CC $@"
	@$(CC) $(ALL_CPPFLAGS) $(ALL_CFLAGS) -o $@ -c $<
