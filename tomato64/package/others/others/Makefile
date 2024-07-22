INSTALLDIR=$(TARGET_DIR)

all:

install:
	install -d $(INSTALLDIR)/sbin
	install -d $(INSTALLDIR)/usr/sbin
	install -d $(INSTALLDIR)/usr/bin
	install -d $(INSTALLDIR)/lib

ifeq ($(TCONFIG_USB),y)
# Optware perl symlink
	install -d $(INSTALLDIR)/usr/bin
	ln -sf /opt/bin/perl $(INSTALLDIR)/usr/bin/perl

# 3G modem switch script
	install -m 0700 switch3g $(INSTALLDIR)/usr/sbin
# 4G modem switch script
	install -m 0700 switch4g $(INSTALLDIR)/usr/sbin
ifneq ($(TCONFIG_BCMARM),y)
	sed -i $(INSTALLDIR)/usr/sbin/switch4g -e "/BCMARM-BEGIN/,/BCMARM-END/d"
else
	sed -i $(INSTALLDIR)/usr/sbin/switch4g -e "/BCMARMNO-BEGIN/,/BCMARMNO-END/d"
endif
# modem status script
	install -m 0700 wwansignal $(INSTALLDIR)/usr/sbin
# Only include curl part if libcurl is installed.
ifneq ($(TCONFIG_BBT),y)
	sed -i $(INSTALLDIR)/usr/sbin/wwansignal -e "/BBT-BEGIN/,/BBT-END/d"
endif
endif # TCONFIG_USB

# MultiWAN watchdog script
	install -m 0700 mwwatchdog $(INSTALLDIR)/usr/sbin
# Only include curl part if libcurl is installed.
ifneq ($(TCONFIG_BBT),y)
	sed -i $(INSTALLDIR)/usr/sbin/mwwatchdog -e "/BBT-BEGIN/,/BBT-END/d"
endif

# ntp2ip
ifeq ($(or $(TCONFIG_DNSCRYPT),$(TCONFIG_STUBBY)),y)
	install -m 0700 ntp2ip $(INSTALLDIR)/usr/sbin
endif

# TomatoAnon project
	install -m 0700 tomatoanon $(INSTALLDIR)/usr/sbin
ifneq ($(TCONFIG_STUBBY),y)
	sed -i $(INSTALLDIR)/usr/sbin/tomatoanon -e "/STUBBY-BEGIN/,/STUBBY-END/d"
else
	sed -i $(INSTALLDIR)/usr/sbin/tomatoanon -e "/STUBBYNO-BEGIN/,/STUBBYNO-END/d"
endif

# Adblock
ifeq ($(TCONFIG_HTTPS),y)
# Only include Adblock v2 if it is required (not for MIPS R1)
ifeq ($(or $(TCONFIG_BCMARM),$(TCONFIG_MIPSR2)),y)
	install -m 0700 adblock-v2 $(INSTALLDIR)/usr/sbin/adblock
else
	install -m 0700 adblock-v1 $(INSTALLDIR)/usr/sbin/adblock
endif
ifneq ($(TCONFIG_STUBBY),y)
	sed -i $(INSTALLDIR)/usr/sbin/adblock -e "/STUBBY-BEGIN/,/STUBBY-END/d"
else
	sed -i $(INSTALLDIR)/usr/sbin/adblock -e "/STUBBYNO-BEGIN/,/STUBBYNO-END/d"
endif
ifeq ($(TCONFIG_TOMATO64),y)
	sed -i $(INSTALLDIR)/usr/sbin/adblock -e "/TOMATO64-REMOVE-BEGIN/,/TOMATO64-REMOVE-END/d"
else
	sed -i $(INSTALLDIR)/usr/sbin/adblock -e "/TOMATO64-BEGIN/,/TOMATO64-END/d"
endif #TOMATO64
endif

# TTB project
ifeq ($(TCONFIG_USB),y)
	install -m 0700 ttb $(INSTALLDIR)/usr/sbin/ttb
else
	install -m 0700 ttb-simple $(INSTALLDIR)/usr/sbin/ttb
endif
ifneq ($(TCONFIG_STUBBY),y)
	sed -i $(INSTALLDIR)/usr/sbin/ttb -e "/STUBBY-BEGIN/,/STUBBY-END/d"
else
	sed -i $(INSTALLDIR)/usr/sbin/ttb -e "/STUBBYNO-BEGIN/,/STUBBYNO-END/d"
endif

# Ethernet state report
	install -m 0700 ethstate $(INSTALLDIR)/usr/sbin
ifneq ($(TCONFIG_BCMARM),y)
	sed -i $(INSTALLDIR)/usr/sbin/ethstate -e "/BCMARM-BEGIN/,/BCMARM-END/d"
else
	sed -i $(INSTALLDIR)/usr/sbin/ethstate -e "/BCMARMNO-BEGIN/,/BCMARMNO-END/d"
endif

# stealthMode
ifneq ($(TCONFIG_BCMARM),y)
ifneq ($(TCONFIG_OPTIMIZE_SIZE_MORE),y)
	install -m 0700 stealthMode $(INSTALLDIR)/usr/sbin
endif
endif

# MOTD
	install -m 0700 mymotd $(INSTALLDIR)/usr/sbin
ifneq ($(TCONFIG_BCMARM),y)
	sed -i $(INSTALLDIR)/usr/sbin/mymotd -e "/BCMARM-BEGIN/,/BCMARM-END/d"
	sed -i $(INSTALLDIR)/usr/sbin/mymotd -e "/TOMATO64-BEGIN/,/TOMATO64-END/d"
else
	sed -i $(INSTALLDIR)/usr/sbin/mymotd -e "/BCMARMNO-BEGIN/,/BCMARMNO-END/d"
	sed -i $(INSTALLDIR)/usr/sbin/mymotd -e "/TOMATO64-REMOVE-BEGIN/,/TOMATO64-REMOVE-END/d"
endif

# sysinfo-helper
ifeq ($(TCONFIG_BCMARM),y)
	install -m 0700 sysinfo-helper $(INSTALLDIR)/usr/sbin
endif

# Link Aggregation
ifeq ($(TCONFIG_BONDING),y)
	install -m 0700 linkagg $(INSTALLDIR)/usr/sbin
endif

# Web Monitor Backup Script
	install -m 0700 webmon_bkp $(INSTALLDIR)/usr/sbin

# optware/entware install script
ifneq ($(TCONFIG_TOMATO64),y)
ifeq ($(TCONFIG_USB),y)
ifneq ($(TCONFIG_BCMARM),y)
	install -m 0700 optware-install.sh $(INSTALLDIR)/usr/sbin
ifneq ($(TCONFIG_BCMARM),y)
	sed -i $(INSTALLDIR)/usr/sbin/optware-install.sh -e "/BCMARM-BEGIN/,/BCMARM-END/d"
else
	sed -i $(INSTALLDIR)/usr/sbin/optware-install.sh -e "/BCMARMNO-BEGIN/,/BCMARMNO-END/d"
endif
ifneq ($(TCONFIG_STUBBY),y)
	sed -i $(INSTALLDIR)/usr/sbin/optware-install.sh -e "/STUBBY-BEGIN/,/STUBBY-END/d"
else
	sed -i $(INSTALLDIR)/usr/sbin/optware-install.sh -e "/STUBBYNO-BEGIN/,/STUBBYNO-END/d"
endif
endif
ifeq ($(TCONFIG_BCMARM),y)
	install -m 0700 entware-install-ARM.sh $(INSTALLDIR)/usr/sbin/entware-install.sh
else
	install -m 0700 entware-install-MIPS.sh $(INSTALLDIR)/usr/sbin/entware-install.sh
endif
ifneq ($(TCONFIG_STUBBY),y)
	sed -i $(INSTALLDIR)/usr/sbin/entware-install.sh -e "/STUBBY-BEGIN/,/STUBBY-END/d"
else
	sed -i $(INSTALLDIR)/usr/sbin/entware-install.sh -e "/STUBBYNO-BEGIN/,/STUBBYNO-END/d"
endif
endif # TCONFIG_USB
else #TOMATO64
	install -m 0700 entware-install-x86_64.sh $(INSTALLDIR)/usr/sbin/entware-install.sh
endif # TOMATO64

# ovpn up/down/etc scripts
ifeq ($(TCONFIG_OPENVPN),y)
	install -m 0700 updown-client.sh $(INSTALLDIR)/usr/sbin
	install -m 0700 vpnrouting.sh $(INSTALLDIR)/usr/sbin
endif

# cron helper
	install -m 0700 cru $(INSTALLDIR)/usr/sbin

# system info
	install -m 0700 sysinfo $(INSTALLDIR)/usr/sbin

# nvram aliases used in other scripts
	install -m 0700 nvram_ops $(INSTALLDIR)/usr/sbin

# network discovery script for status-devices page
ifeq ($(or $(TCONFIG_BCMARM),$(TCONFIG_MIPSR2)),y)
	install -m 0755 discovery.sh $(INSTALLDIR)/usr/sbin
endif

# certificate generator
ifneq ($(TCONFIG_HTTPS),)
	install -m 0500 gencert.sh $(INSTALLDIR)/usr/sbin
endif

# distinguish version if ARM or MIPS
ifeq ($(TCONFIG_OPENVPN),y)
ifneq ($(TCONFIG_BCMARM),y)
	sed -i $(INSTALLDIR)/usr/sbin/vpnrouting.sh -e "/BCMARM-BEGIN/,/BCMARM-END/d"
else
	sed -i $(INSTALLDIR)/usr/sbin/vpnrouting.sh -e "/BCMARMNO-BEGIN/,/BCMARMNO-END/d"
endif
endif

# clean up
	cd $(INSTALLDIR)/usr/sbin && \
		for F in tomatoanon adblock ttb optware-install.sh entware-install.sh vpnrouting.sh ethstate mymotd switch4g; do \
			[ -f $(INSTALLDIR)/usr/sbin/$$F ] && sed -i $$F \
			-e "/STUBBY-BEGIN/d"	-e "/STUBBY-END/d" \
			-e "/STUBBYNO-BEGIN/d"	-e "/STUBBYNO-END/d" \
			-e "/BCMARM-BEGIN/d"	-e "/BCMARM-END/d" \
			-e "/BCMARMNO-BEGIN/d"	-e "/BCMARMNO-END/d" \
			-e "/BBT-BEGIN/d"	-e "/BBT-END/d" \
			-e "/TOMATO64-BEGIN/d"	-e "/TOMATO64-END/d" \
			-e "/TOMATO64-REMOVE-BEGIN/d"	-e "/TOMATO64-REMOVE-END/d" \
			|| true; \
		done

ifneq ($(TCONFIG_BCMARM),y)
	cd $(INSTALLDIR)/usr/sbin && \
		for F in switch3g switch4g wwansignal mwwatchdog ntp2ip tomatoanon adblock ttb ethstate mymotd sysinfo-helper linkagg webmon_bkp optware-install.sh entware-install.sh updown-client.sh vpnrouting.sh cru sysinfo stealthMode gencert.sh; do \
			[ -f $(INSTALLDIR)/usr/sbin/$$F ] && { \
				sed -i $$F -e "/^#!/p" -e "/^\s*#/d"; \
				sed -i $$F -e '/^\/\*\s*$$/,/\*\//! { s/^\s\+//; s/\s\+$$//; /^$$/d }'; \
			} || true; \
	done
ifeq ($(or $(TCONFIG_BT),$(TCONFIG_NGINX)),y)
	cd $(INSTALLDIR)/usr/bin && \
		for F in mycheck btcheck; do \
			[ -f $(INSTALLDIR)/usr/bin/$$F ] && { \
				sed -i $$F -e "/^#!/p" -e "/^\s*#/d"; \
				sed -i $$F -e '/^\/\*\s*$$/,/\*\//! { s/^\s\+//; s/\s\+$$//; /^$$/d }'; \
			} || true; \
	done
endif
endif # TCONFIG_BCMARM

clean:
