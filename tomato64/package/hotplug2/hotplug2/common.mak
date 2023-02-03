# vim:set sw=8 nosta:

ALL_CFLAGS = -fpic -Os -DHAVE_RULES -Wall -g -Wextra $(EXTRACFLAGS) -DQCAMUSL -DHND_ROUTER
ALL_CFLAGS += $(CFLAGS)

ALL_LDFLAGS = -g -fpic
ALL_LDFLAGS += $(LDFLAGS)

INSTALL=install -c -m 644
INSTALL_BIN=install -c -m 755
INSTALL_DIR=install -d


.PHONY: all clean dep install install-recursive clean-recursive \
	dep-recursive all-recursive

MAKEDEP=-$(CC) $(ALL_CFLAGS) -MM $(wildcard *.c *.cc) > .depend
dep: dep-recursive
	$(MAKEDEP)
.depend:
	$(MAKEDEP)
dep-recursive:
	@for i in $(SUBDIRS); do $(MAKE) -C $$i dep; done
	
-include .depend

clean: clean-recursive
	$(RM) $(wildcard *.o *.so *.a $(BINS)) .depend
clean-recursive:
	@for i in $(SUBDIRS); do $(MAKE) -C $$i clean; done

all: all-recursive
all-recursive:
	@for i in $(SUBDIRS); do $(MAKE) -C $$i all; done

install: all install-recursive
install-recursive:
	@for i in $(SUBDIRS); do $(MAKE) -C $$i install; done

%.o: %.c
	@echo " [hotplug] CC $@"
	@$(CC) $(ALL_CPPFLAGS) $(ALL_CFLAGS) -o $@ -c $<
