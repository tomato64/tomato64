ALL_CPPFLAGS = -fPIC
ALL_CPPFLAGS += -DTOMATO64
ALL_CPPFLAGS += -I$(BR2_EXTERNAL_TOMATO64_PATH)/package/libshared/shared
ALL_CPPFLAGS += -I$(BR2_EXTERNAL_TOMATO64_PATH)/package/libshared/shared/include
ALL_CPPFLAGS += -I$(BR2_EXTERNAL_TOMATO64_PATH)/package/libshared/shared/common/include
ALL_CPPFLAGS += -I$(BR2_EXTERNAL_TOMATO64_PATH)/package/libshared/shared/bcmwifi/include
ALL_CPPFLAGS += -I$(TARGET_DIR)/usr/include
ALL_CPPFLAGS += $(CPPFLAGS)

ALL_CFLAGS = -fPIC
ALL_CFLAGS += $(CFLAGS)

ALL_LDFLAGS = -fPIC -lnvram
ALL_LDFLAGS += $(LDFLAGS)

OBJS = openvpn_plugin_auth_nvram.o

all: openvpn_plugin_auth_nvram.so

openvpn_plugin_auth_nvram.so: $(OBJS)
	@echo " [openvpn_plugin_auth_nvram.so] CC -o $@"
	@$(CC) -shared $(ALL_CPPFLAGS) $(ALL_CFLAGS) -o $@ $(OBJS) $(ALL_LDFLAGS)

install: all
	install -D openvpn_plugin_auth_nvram.so $(INSTALLDIR)/lib/openvpn_plugin_auth_nvram.so
	$(STRIP) $(INSTALLDIR)/lib/openvpn_plugin_auth_nvram.so
	chmod 0500 $(INSTALLDIR)/lib/openvpn_plugin_auth_nvram.so

clean:
	rm -f openvpn_plugin_auth_nvram.so .*.depend *.o

%.o: %.c
	@echo " [openvpn_plugin_auth_nvram.o] CC $@"
	@$(CC) $(ALL_CPPFLAGS) $(ALL_CFLAGS) -o $@ -c $<
