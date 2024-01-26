ALL_CPPFLAGS = -fPIC
ALL_CPPFLAGS += $(CPPFLAGS)

ALL_CFLAGS = -fPIC
ALL_CFLAGS += $(CFLAGS)

ALL_LDFLAGS = -fPIC
ALL_LDFLAGS += $(LDFLAGS)

OBJS = sd-idle.o

all: sd-idle

sd-idle: $(OBJS)
	@echo " [sd-idle] CC -o $@"
	@$(CC) $(ALL_CPPFLAGS) $(ALL_CFLAGS) -o $@ $(OBJS) $(ALL_LDFLAGS)

clean:
	rm -f sd-idle
	rm -f *.o

%.o: %.c
	@echo " [sd-idle] CC $@"
	@$(CC) $(ALL_CPPFLAGS) $(ALL_CFLAGS) -o $@ -c $<
