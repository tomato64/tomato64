ALL_CPPFLAGS = -fPIC
ALL_CPPFLAGS += -DTOMATO64
ALL_CPPFLAGS += -Wsign-compare -DUSE_LIBCURL
ALL_CPPFLAGS += -I.
ALL_CPPFLAGS += -I$(BR2_EXTERNAL_TOMATO64_PATH)/package/libshared/shared
ALL_CPPFLAGS += -I$(BR2_EXTERNAL_TOMATO64_PATH)/package/libshared/shared/include
ALL_CPPFLAGS += -I$(BR2_EXTERNAL_TOMATO64_PATH)/package/libshared/shared/common/include
ALL_CPPFLAGS += -I$(BR2_EXTERNAL_TOMATO64_PATH)/package/libshared/shared/bcmwifi/include
ALL_CPPFLAGS += $(CPPFLAGS)

ALL_CFLAGS = -fPIC
ALL_CFLAGS += $(CFLAGS)

ALL_LDFLAGS = -fPIC
ALL_LDFLAGS += -Wl,--start-group -lcurl -lssl -lcrypto -lshared -lnvram -Wl,--end-group
ALL_LDFLAGS += $(LDFLAGS)

OBJS = mdu.o

all: mdu

mdu: $(OBJS)
	@$(CC) $(ALL_CPPFLAGS) $(ALL_CFLAGS) $(ALL_LDFLAGS) -o $@ $^

%.o: %.c
	@echo " [mdu] CC $@"
	@$(CC) $(ALL_CPPFLAGS) $(ALL_CFLAGS) -o $@ -c $<
