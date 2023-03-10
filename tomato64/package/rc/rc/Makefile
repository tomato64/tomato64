ALL_CPPFLAGS = -fPIC
ALL_CPPFLAGS += -I.
ALL_CPPFLAGS += -I$(BR2_EXTERNAL_TOMATO64_PATH)/package/libshared/shared
ALL_CPPFLAGS += -I$(BR2_EXTERNAL_TOMATO64_PATH)/package/libshared/shared/include
ALL_CPPFLAGS += -I$(BR2_EXTERNAL_TOMATO64_PATH)/package/libshared/shared/common/include
ALL_CPPFLAGS += -I$(BR2_EXTERNAL_TOMATO64_PATH)/package/libshared/shared/bcmwifi/include
ALL_CPPFLAGS += $(CPPFLAGS)

TCONFIG_USB=y
TCONFIG_DDNS=y
TCONFIG_CIFS=y
TCONFIG_JFFS2=y
TCONFIG_SAMBASRV=y
TCONFIG_FTP=y
TCONFIG_OPENVPN=y
TCONFIG_TINC=y
TCONFIG_PPTPD=y
TCONFIG_NOCAT=y
TCONFIG_NGINX=y
TCONFIG_BT=y
TCONFIG_NFS=y
TCONFIG_SNMP=y
TCONFIG_UPS=y
TCONFIG_TOR=y
TCONFIG_STUBBY=y
TCONFIG_MULTIWAN=y
TCONFIG_HTTPS=y
TCONFIG_IPV6=y
TCONFIG_DNSSEC=y
TCONFIG_MDNS=y

#-DTCONFIG_USBAP -DTCONFIG_ZEBRA

ALL_CFLAGS = -fPIC
ALL_CFLAGS += $(CFLAGS)

ALL_LDFLAGS = -fPIC -lshared -lnvram
ALL_LDFLAGS += $(LDFLAGS)

#CFLAGS += -I. -I$(TOP)/shared -I$(SRCBASE)/include -idirafter$(LINUXDIR)/include
#CFLAGS += -ffunction-sections -fdata-sections

#ifeq ($(TCONFIG_DPSTA),y)
#CFLAGS += -I$(TOP)/dpsta
#endif

#ifeq ($(TCONFIG_BCMARM),y)
#LDFLAGS = -L$(TOP)/nvram${BCMEX} -lnvram -L$(TOP)/shared -lshared -lcrypt -L$(TOP)/libbcmcrypto -lbcmcrypto
#CFLAGS += -I$(SRCBASE)/shared/bcmwifi/include -DHAVE_TYPE_FLOAT
#CFLAGS += -I$(SRCBASE)/common/include
#LDFLAGS += -L$(TOP)/libbcm -lbcm
#LDFLAGS += -lgcc_s
#else
#LDFLAGS = -L$(TOP)/nvram -lnvram -L$(TOP)/shared -lshared -lcrypt
#endif

#LDFLAGS += -ffunction-sections -fdata-sections -Wl,--gc-sections
#ifeq ($(STATIC),1)
#LDFLAGS += -static
#endif

#ifeq ($(TCONFIG_BCMARM),y)
#LDFLAGS += $(EXTRA_LDFLAGS)
#endif

#ifeq ($(TCONFIG_BCMSMP),y)
#CFLAGS += -DSMP
#endif

#ifeq ($(TOMATO_EXPERIMENTAL),1)
#CFLAGS += -DDEBUG_LOGMSG
#endif

OBJS := rc.o init.o interface.o network.o wan.o services.o dhcp.o \
        firewall.o ppp.o telssh.o wnas.o \
        listen.o redial.o led.o qos.o forward.o misc.o \
        restrict.o gpio.o sched.o \
        bwlimit.o arpbind.o \
        tomatoanon.o \
	samba.o usb.o jffs2.o cifs.o snmp.o pptpd.o pptp_client.o ddns.o ftpd.o tinc.o openvpn.o blink.o

#buttons.o mtd.o

ifeq ($(or $(TCONFIG_BCMARM),$(TCONFIG_BLINK)),y)
OBJS += blink.o
OBJS += blink_br.o
endif

ifeq ($(TCONFIG_ROAM),y)
OBJS += roamast.o
endif

ifeq ($(TCONFIG_BCMARM),y)
OBJS += bcmutils.o
endif

ifeq ($(TCONFIG_FANCTRL),y)
OBJS += phy_tempsense.o
endif

# dualwan / multiwan
OBJS += mwan.o pbr.o

ifeq ($(TCONFIG_USB),y)
OBJS += usb.o
endif

ifeq ($(TCONFIG_DDNS),y)
OBJS += ddns.o
endif

ifeq ($(TCONFIG_CIFS),y)
OBJS += cifs.o
endif

ifeq ($(TCONFIG_JFFS2),y)
OBJS += jffs2.o
endif

ifeq ($(TCONFIG_SAMBASRV),y)
OBJS += samba.o
endif

ifeq ($(TCONFIG_FTP),y)
OBJS += ftpd.o
endif

ifeq ($(TCONFIG_OPENVPN),y)
OBJS += openvpn.o
endif

ifeq ($(TCONFIG_TINC),y)
OBJS += tinc.o
endif

ifeq ($(TCONFIG_PPTPD),y)
OBJS += pptpd.o
OBJS += pptp_client.o
endif

ifeq ($(TCONFIG_NOCAT),y)
OBJS += nocat.o
endif

ifeq ($(TCONFIG_NGINX),y)
OBJS += nginx.o
OBJS += mysql.o
endif

ifeq ($(TCONFIG_BT),y)
OBJS += transmission.o
endif

ifeq ($(TCONFIG_NFS),y)
OBJS += nfs.o
endif

ifeq ($(TCONFIG_SNMP),y)
OBJS += snmp.o
endif

ifeq ($(TCONFIG_SDHC),y)
OBJS += mmc.o
endif

ifeq ($(TCONFIG_UPS),y)
OBJS += ups.o
endif

ifeq ($(TCONFIG_TOR),y)
OBJS += tor.o
endif

ifeq ($(TCONFIG_BCMARM),y)
vpath %.c $(SRCBASE)/shared
endif

all: rc

rc: $(OBJS)
	@echo " [rc] CC $@"
	@$(CC) -zmuldefs $(ALL_CPPFLAGS) $(ALL_CFLAGS) $(ALL_LDFLAGS) -o $@ $^

#	$(SIZECHECK)
#	$(CPTMP)

install: all
	@echo "[rc] Installing..."
	@install -D rc $(INSTALLDIR)/sbin/rc
	@$(STRIP) $(INSTALLDIR)/sbin/rc
	@chmod 0500 $(INSTALLDIR)/sbin/rc

ifeq ($(TCONFIG_BCMARM),y)
	@cd $(INSTALLDIR)/sbin && ln -sf rc preinit
else
	@cd $(INSTALLDIR)/sbin && ln -sf rc init
endif
	@cd $(INSTALLDIR)/sbin && ln -sf rc console
	@cd $(INSTALLDIR)/sbin && ln -sf rc hotplug
	@cd $(INSTALLDIR)/sbin && ln -sf rc service
	@cd $(INSTALLDIR)/sbin && ln -sf rc buttons
ifeq ($(or $(TCONFIG_BCMARM),$(TCONFIG_BLINK)),y)
	@cd $(INSTALLDIR)/sbin && ln -sf rc blink
	@cd $(INSTALLDIR)/sbin && ln -sf rc blink_br
endif
ifeq ($(TCONFIG_FANCTRL),y)
	@cd $(INSTALLDIR)/sbin && ln -sf rc phy_tempsense
endif
	@cd $(INSTALLDIR)/sbin && ln -sf rc rcheck
	@cd $(INSTALLDIR)/sbin && ln -sf rc radio
	@cd $(INSTALLDIR)/sbin && ln -sf rc led
	@cd $(INSTALLDIR)/sbin && ln -sf rc reboot
	@cd $(INSTALLDIR)/sbin && ln -sf rc halt
	@cd $(INSTALLDIR)/sbin && ln -sf rc redial

	@cd $(INSTALLDIR)/sbin && ln -sf rc mwanroute

	@cd $(INSTALLDIR)/sbin && ln -sf rc gpio
	@cd $(INSTALLDIR)/sbin && ln -sf rc sched

	@cd $(INSTALLDIR)/sbin && ln -sf rc disconnected_pppoe
	@cd $(INSTALLDIR)/sbin && ln -sf rc listen
	@cd $(INSTALLDIR)/sbin && ln -sf rc ppp_event

	@cd $(INSTALLDIR)/sbin && ln -sf rc ntpd_synced

ifeq ($(TCONFIG_BCMARM),y)
	@cd $(INSTALLDIR)/sbin && ln -sf rc mtd-write2
	@cd $(INSTALLDIR)/sbin && ln -sf rc mtd-erase2
	@cd $(INSTALLDIR)/sbin && ln -sf rc nvram_erase
else
	@cd $(INSTALLDIR)/sbin && ln -sf rc mtd-write
	@cd $(INSTALLDIR)/sbin && ln -sf rc mtd-erase
endif
	@cd $(INSTALLDIR)/sbin && ln -sf rc mtd-unlock

	@cd $(INSTALLDIR)/sbin && ln -sf rc dhcpc-event
	@cd $(INSTALLDIR)/sbin && ln -sf rc dhcpc-release
	@cd $(INSTALLDIR)/sbin && ln -sf rc dhcpc-renew
	@cd $(INSTALLDIR)/sbin && ln -sf rc dhcpc-event-lan

	@cd $(INSTALLDIR)/sbin && ln -sf rc wldist

ifeq ($(TCONFIG_IPV6),y)
	@cd $(INSTALLDIR)/sbin && ln -sf rc dhcp6c-state
endif

ifeq ($(TCONFIG_DDNS),y)
	@cd $(INSTALLDIR)/sbin && ln -sf rc ddns-update
endif

ifeq ($(TCONFIG_CIFS),y)
	@cd $(INSTALLDIR)/sbin && ln -sf rc mount-cifs
endif

ifeq ($(TCONFIG_ROAM),y)
	@cd $(INSTALLDIR)/sbin && ln -sf rc roamast
endif

clean:
	rm -f rc *.o .*.depend

size: rc
	mipsel-uclibc-nm --print-size --size-sort rc

%.o: %.c
	@echo " [rc] CC $@"
	@$(CC) $(ALL_CPPFLAGS) $(ALL_CFLAGS) -o $@ -c $<

pswatch: pswatch.c
	$(CC) -o $@ $^
	$(STRIP) $@
	$(CPTMP)
