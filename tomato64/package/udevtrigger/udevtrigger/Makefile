ALL_CPPFLAGS = -fPIC
ALL_CPPFLAGS += $(CPPFLAGS)

ALL_CFLAGS = -fPIC
ALL_CFLAGS += $(CFLAGS)

ALL_LDFLAGS = -fPIC
ALL_LDFLAGS += $(LDFLAGS)

OBJS = udevtrigger.o

all: udevtrigger

udevtrigger: $(OBJS)
	@echo " [udevtrigger] CC -o $@"
	@$(CC) $(ALL_CPPFLAGS) $(ALL_CFLAGS) -o $@ $(OBJS) $(ALL_LDFLAGS)

install: all
	install -D udevtrigger $(INSTALLDIR)/sbin/udevtrigger
	$(STRIP) $(INSTALLDIR)/sbin/udevtrigger
	chmod 0500 $(INSTALLDIR)/sbin/udevtrigger

clean:
	rm -f udevtrigger .*.depend *.o

%.o: %.c
	@echo " [udevtrigger] CC $@"
	@$(CC) $(ALL_CPPFLAGS) $(ALL_CFLAGS) -o $@ -c $<
