ALL_CPPFLAGS = -fPIC
ALL_CPPFLAGS += -std=gnu17
ALL_CPPFLAGS += -DUSE_ZLIB
ALL_CPPFLAGS += -DTOMATO64
ALL_CPPFLAGS += -I$(BR2_EXTERNAL_TOMATO64_PATH)/package/libshared/shared
ALL_CPPFLAGS += -I$(BR2_EXTERNAL_TOMATO64_PATH)/package/libshared/shared/include
ALL_CPPFLAGS += -I$(BR2_EXTERNAL_TOMATO64_PATH)/package/libshared/shared/common/include
ALL_CPPFLAGS += -I$(BR2_EXTERNAL_TOMATO64_PATH)/package/libshared/shared/bcmwifi/include
ALL_CPPFLAGS += $(CPPFLAGS)

ALL_CFLAGS = -fPIC
ALL_CFLAGS += $(CFLAGS)

ALL_LDFLAGS = -fPIC
ALL_LDFLAGS += -lshared -lnvram -lz
ALL_LDFLAGS += $(LDFLAGS)

MIGRATE_LDFLAGS = -lz

ifeq ($(TOMATO_EXPERIMENTAL),1)
ALL_CFLAGS += -DDEBUG_LOGMSG
endif

OBJS = rstats.o
MIGRATE_OBJS = rstats_migrate.o

all: rstats rstats_migrate

rstats: $(OBJS)
	@echo " [rstats] CC -o $@"
	@$(CC) $(ALL_CPPFLAGS) $(ALL_CFLAGS) -o $@ $(OBJS) $(ALL_LDFLAGS)

rstats_migrate: $(MIGRATE_OBJS)
	@echo " [rstats] CC -o $@"
	@$(CC) $(ALL_CPPFLAGS) $(ALL_CFLAGS) -o $@ $(MIGRATE_OBJS) $(MIGRATE_LDFLAGS)

clean:
	rm -f rstats rstats_migrate .*.depend
	rm -f *.o

install: all
	@echo " [rstats] Installing..."
	install -D rstats $(INSTALLDIR)/bin/rstats
	$(STRIP) $(INSTALLDIR)/bin/rstats
	chmod 0500 $(INSTALLDIR)/bin/rstats

%.o: %.c
	@echo " [rstats] CC $@"
	@$(CC) $(ALL_CPPFLAGS) $(ALL_CFLAGS) -o $@ -c $<
