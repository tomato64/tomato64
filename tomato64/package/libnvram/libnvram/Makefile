ALL_CFLAGS = -O2 -fPIC -Wall
ALL_LDFLAGS = -shared

ALL_CFLAGS += $(CFLAGS)
ALL_LDFLAGS += $(LDFLAGS)


OBJECTS=$(SOURCES:.c=.o)
SOURCES=nvram.c
TARGET=libnvram.so

all: $(SOURCES) $(TARGET)

$(TARGET): $(OBJECTS)
	$(CC) $(ALL_CFLAGS) $(ALL_LDFLAGS) $(OBJECTS) -o $@

.c.o:
	$(CC) -c $(ALL_CFLAGS) $< -o $@

clean:
	rm -f *.o libnvram.so test

.PHONY: clean
