ALL_CPPFLAGS = -fPIC
ALL_CPPFLAGS += -I.
ALL_CPPFLAGS += -I$(BR2_EXTERNAL_TOMATO64_PATH)/package/libshared/shared
ALL_CPPFLAGS += -I$(BR2_EXTERNAL_TOMATO64_PATH)/package/libshared/shared/include
ALL_CPPFLAGS += -I$(BR2_EXTERNAL_TOMATO64_PATH)/package/libshared/shared/common/include
ALL_CPPFLAGS += -I$(BR2_EXTERNAL_TOMATO64_PATH)/package/libshared/shared/bcmwifi/include
ALL_CPPFLAGS += -DTCONFIG_HTTPS -DUSE_OPENSSL -DUSE_OPENSSL11
ALL_CPPFLAGS += $(CPPFLAGS)

ALL_CFLAGS = -fPIC -g
ALL_CFLAGS += $(CFLAGS)

ALL_LDFLAGS = -fPIC -lssl -lcrypto -lshared
ALL_LDFLAGS += $(LDFLAGS)


#ifneq ($(TCONFIG_HTTPS),)
#ifeq ($(TCONFIG_OPENSSL11),y)
#CFLAGS	+= -I$(TOP)/openssl-1.1/include -DUSE_OPENSSL -DUSE_OPENSSL11
#LIBS += -L$(TOP)/openssl-1.1 -lssl -lcrypto
#else
#CFLAGS	+= -I$(TOP)/openssl/include -DUSE_OPENSSL
#LIBS += -L$(TOP)/openssl -lssl -lcrypto
#endif
#else
#CFLAGS	+= -I$(TOP)/cyassl/include -I$(TOP)/cyassl/ctaocrypt/include
#LIBS += $(TOP)/cyassl/src/.libs/libcyassl.a
#endif

#ifneq ($(TCONFIG_BCMARM),y)
#LIBS += -lpthread
#endif
#OBJ = mssl.o

ifeq ($(TOMATO_EXPERIMENTAL),1)
export CFLAGS += -DDEBUG_LOGMSG
endif

all: libmssl.so libmssl.a

libmssl.so: $(OBJ)
	$(CC) $(ALL_CPPFLAGS) $(ALL_CFLAGS) -shared -o $@ $^ $(ALL_LDFLAGS)

libmssl.a: $(OBJ)
	rm -f $@
	$(AR) cruv $@ $^

clean:
	rm -f .*.depend *.o *.so *.a

install: all
	@echo " [mssl] Installing..."
	install -D libmssl.so $(INSTALLDIR)/usr/lib/libmssl.so
ifneq ($(TCONFIG_BCMARM),y)
	$(STRIP) $(INSTALLDIR)/usr/lib/libmssl.so
endif

%.o: %.c
	@echo " [mssl] CC $@"
	@$(CC) $(ALL_CPPFLAGS) $(ALL_CFLAGS) -o $@ -c $<
