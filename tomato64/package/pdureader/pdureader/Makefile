ALL_CPPFLAGS = -fPIC -D_GNU_SOURCE
ALL_CPPFLAGS += -I.
ALL_CPPFLAGS += -I$(BR2_EXTERNAL_TOMATO64_PATH)/package/libshared/shared
ALL_CPPFLAGS += -I$(BR2_EXTERNAL_TOMATO64_PATH)/package/libshared/shared/include
ALL_CPPFLAGS += -I$(BR2_EXTERNAL_TOMATO64_PATH)/package/libshared/shared/common/include
ALL_CPPFLAGS += -I$(BR2_EXTERNAL_TOMATO64_PATH)/package/libshared/shared/bcmwifi/include
ALL_CPPFLAGS += $(CPPFLAGS)

ALL_CFLAGS = -fPIC
ALL_CFLAGS += $(CFLAGS)

ALL_LDFLAGS = -fPIC
ALL_LDFLAGS += $(LDFLAGS)

OBJS = pdureader.o

all: pdureader

pdureader: $(OBJS)
	@$(CC) $(ALL_CPPFLAGS) $(ALL_CFLAGS) $(ALL_LDFLAGS) -o $@ $^

%.o: %.c
	@echo " [pdureader] CC $@"
	@$(CC) $(ALL_CPPFLAGS) $(ALL_CFLAGS) -o $@ -c $<
