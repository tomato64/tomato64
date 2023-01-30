ALL_CPPFLAGS = -fPIC
ALL_CPPFLAGS += -I$(BR2_EXTERNAL_TOMATO64_PATH)/package/libshared/shared
ALL_CPPFLAGS += -I$(BR2_EXTERNAL_TOMATO64_PATH)/package/libshared/shared/include
ALL_CPPFLAGS += -I$(BR2_EXTERNAL_TOMATO64_PATH)/package/libshared/shared/common/include
ALL_CPPFLAGS += -I$(BR2_EXTERNAL_TOMATO64_PATH)/package/libshared/shared/bcmwifi/include
ALL_CPPFLAGS += $(CPPFLAGS)

ALL_CFLAGS = -fPIC
ALL_CFLAGS += $(CFLAGS)

ALL_LDFLAGS = -fPIC -lnvram -lshared
ALL_LDFLAGS += $(LDFLAGS)

ifeq ($(TOMATO_EXPERIMENTAL),1)
export CFLAGS += -DDEBUG_LOGMSG
endif

OBJS = rstats.o

all: rstats

rstats: $(OBJS)
	@echo " [rstats] CC -o $@"
	@$(CC) $(ALL_CPPFLAGS) $(ALL_CFLAGS) -o $@ $(OBJS) $(ALL_LDFLAGS)

clean:
	rm -f rstats .*.depend
	rm -f *.o

install: all
	@echo " [rstats] Installing..."
	install -D rstats $(INSTALLDIR)/bin/rstats
	$(STRIP) $(INSTALLDIR)/bin/rstats
	chmod 0500 $(INSTALLDIR)/bin/rstats

%.o: %.c
	@echo " [rstats] CC $@"
	@$(CC) $(ALL_CPPFLAGS) $(ALL_CFLAGS) -o $@ -c $<
