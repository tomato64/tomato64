INSTALLDIR=$(TARGET_DIR)
HASH=$(shell git rev-parse HEAD)
HASH_SHORT=$(shell git rev-parse --short HEAD)

all:

clean:

install:
	rm -rf $(INSTALLDIR)/www
	mkdir -p $(INSTALLDIR)/www

	cp *.gif *.png *.ico *.html *.php robots.txt $(INSTALLDIR)/www

# Squish some of the files by trimming whitespace...
	for F in $(wildcard *.js *.jsx *.jsz); do \
		sed '/^\/\*\s*$$/,/\*\//! { s/^\s\+//; s/\s\+$$//; /^\/\/ --\+\s*/d; /^$$/d }' < $$F > $(INSTALLDIR)/www/$$F; \
	done

	for F in $(wildcard *.css); do \
		sed '/^\/\*\s*$$/,/\*\//! { s/\s\+/ /g; s/^\s\+//; s/\s\+$$//; /^$$/d }' < $$F > $(INSTALLDIR)/www/$$F; \
	done

# remove comments in between REMOVE-BEGIN and REMOVE-END, and compress whitespace
	for F in $(wildcard *.asp *.svg); do \
		sed	-e "/REMOVE-BEGIN/,/REMOVE-END/d" \
			-e "s,<!-- / / / -->,," \
			-e "s,\x0d,," \
			-e "s,^\s\+,," \
			-e "/^$$/d" \
			-e "/^<!--$$/,/^-->$$/d" \
			-e "/^<!--$$/,/^-->$$/! { s,^\s\+, , }" $$F > $(INSTALLDIR)/www/$$F; \
	done

# Only include MultiWAN options if MULTIWAN is configured in.
ifneq ($(TCONFIG_MULTIWAN),y)
	cd $(INSTALLDIR)/www && \
	for F in $(wildcard *.asp *.js *.jsx *.jsz); do \
		sed -i $$F -e "/MULTIWAN-BEGIN/,/MULTIWAN-END/d"; \
	done
# Or remove dualwan options
else
	cd $(INSTALLDIR)/www && \
	for F in $(wildcard *.asp *.js *.jsx *.jsz); do \
		sed -i $$F -e "/DUALWAN-BEGIN/,/DUALWAN-END/d"; \
	done
endif

# Only include the CIFS pages if CIFS is configured in.
ifneq ($(TCONFIG_CIFS),y)
	rm -f $(INSTALLDIR)/www/admin-cifs.asp
	sed -i $(INSTALLDIR)/www/tomato.js -e "/CIFS-BEGIN/,/CIFS-END/d"
	sed -i $(INSTALLDIR)/www/admin-bwm.asp -e "/CIFS-BEGIN/,/CIFS-END/d"
	sed -i $(INSTALLDIR)/www/admin-iptraffic.asp -e "/CIFS-BEGIN/,/CIFS-END/d"
	sed -i $(INSTALLDIR)/www/nas-media.asp -e "/CIFS-BEGIN/,/CIFS-END/d"
	sed -i $(INSTALLDIR)/www/nas-bittorrent.asp -e "/CIFS-BEGIN/,/CIFS-END/d"
endif

# Only include the JFFS pages if JFFS is configured in. Distinguish JFFS2 and BRCMNAND.
ifneq ($(TCONFIG_JFFS2),y)
	rm -f $(INSTALLDIR)/www/admin-jffs2.asp
	sed -i $(INSTALLDIR)/www/tomato.js -e "/JFFS2-BEGIN/,/JFFS2-END/d"
	sed -i $(INSTALLDIR)/www/admin-bwm.asp -e "/JFFS2-BEGIN/,/JFFS2-END/d"
	sed -i $(INSTALLDIR)/www/admin-iptraffic.asp -e "/JFFS2-BEGIN/,/JFFS2-END/d"
	sed -i $(INSTALLDIR)/www/admin-upgrade.asp -e "/JFFS2-BEGIN/,/JFFS2-END/d"
	sed -i $(INSTALLDIR)/www/admin-access.asp -e "/JFFS2-BEGIN/,/JFFS2-END/d"
	sed -i $(INSTALLDIR)/www/nas-media.asp -e "/JFFS2-BEGIN/,/JFFS2-END/d"
	sed -i $(INSTALLDIR)/www/nas-bittorrent.asp -e "/JFFS2-BEGIN/,/JFFS2-END/d"
else
ifeq ($(TCONFIG_BRCM_NAND_JFFS2),y)
	sed -i $(INSTALLDIR)/www/admin-jffs2.asp -e "/JFFS2-BEGIN/,/JFFS2-END/d"
	sed -i $(INSTALLDIR)/www/nas-bittorrent.asp -e "/JFFS2-BEGIN/,/JFFS2-END/d"
else
	sed -i $(INSTALLDIR)/www/admin-jffs2.asp -e "/JFFS2NAND-BEGIN/,/JFFS2NAND-END/d"
	sed -i $(INSTALLDIR)/www/nas-bittorrent.asp -e "/JFFS2NAND-BEGIN/,/JFFS2NAND-END/d"
endif
endif

# Only include EMF options if EMF is configured in.
ifneq ($(TCONFIG_EMF),y)
	sed -i $(INSTALLDIR)/www/advanced-firewall.asp -e "/EMF-BEGIN/,/EMF-END/d"
	sed -i $(INSTALLDIR)/www/advanced-wireless.asp -e "/EMF-BEGIN/,/EMF-END/d"
endif

# Only include NTFS settings if NTFS support is configured in.
ifneq ($(TCONFIG_NTFS),y)
	sed -i $(INSTALLDIR)/www/nas-usb.asp -e "/NTFS-BEGIN/,/NTFS-END/d"
endif

# Only include Paragon NTFS settings if Paragon is configured in.
ifneq ($(TCONFIG_UFSDA),y)
ifneq ($(TCONFIG_UFSDN),y)
	sed -i $(INSTALLDIR)/www/nas-usb.asp -e "/PARAGON-BEGIN/,/PARAGON-END/d"
endif
endif

# Only include Tuxera NTFS settings if Tuxera is configured in.
ifneq ($(TCONFIG_TUXERA),y)
	sed -i $(INSTALLDIR)/www/nas-usb.asp -e "/TUXERA-BEGIN/,/TUXERA-END/d"
endif

# Only include Tuxera HFS settings if Tuxera HFS is configured in.
ifneq ($(TCONFIG_TUX_HFS),y)
	sed -i $(INSTALLDIR)/www/nas-usb.asp -e "/TUXERAHFS-BEGIN/,/TUXERAHFS-END/d"
endif

# Only include sd/mmc card support if MICROSD is configured in.
ifneq ($(TCONFIG_MICROSD),y)
	sed -i $(INSTALLDIR)/www/nas-usb.asp -e "/MICROSD-BEGIN/,/MICROSD-END/d"
endif

# Only include the FTP pages if FTP Server is configured in.
ifneq ($(TCONFIG_FTP),y)
	rm -f $(INSTALLDIR)/www/nas-ftp.asp
	sed -i $(INSTALLDIR)/www/tomato.js -e "/FTP-BEGIN/,/FTP-END/d"
	sed -i $(INSTALLDIR)/www/isup.jsz -e "/FTP-BEGIN/,/FTP-END/d"
endif

# Only include the Samba pages if Samba is configured in.
ifneq ($(TCONFIG_SAMBASRV),y)
	rm -f $(INSTALLDIR)/www/nas-samba.asp
	sed -i $(INSTALLDIR)/www/tomato.js -e "/SAMBA-BEGIN/,/SAMBA-END/d"
	sed -i $(INSTALLDIR)/www/isup.jsz -e "/SAMBA-BEGIN/,/SAMBA-END/d"
endif

# Only include the Media Server pages if Media Server is configured in.
ifneq ($(TCONFIG_MEDIA_SERVER),y)
	rm -f $(INSTALLDIR)/www/nas-media.asp
	sed -i $(INSTALLDIR)/www/tomato.js -e "/MEDIA-SRV-BEGIN/,/MEDIA-SRV-END/d"
	sed -i $(INSTALLDIR)/www/isup.jsz -e "/MEDIA-SRV-BEGIN/,/MEDIA-SRV-END/d"
endif

# Only include Captive Portal menu option and pages when configured.
ifneq ($(TCONFIG_NOCAT),y)
	rm -f $(INSTALLDIR)/www/splashd.asp
	rm -f $(INSTALLDIR)/www/splash.html
	rm -f $(INSTALLDIR)/www/style.css
	sed -i $(INSTALLDIR)/www/tomato.js -e "/NOCAT-BEGIN/,/NOCAT-END/d"
	sed -i $(INSTALLDIR)/www/about.asp -e "/NOCAT-BEGIN/,/NOCAT-END/d"
endif

# only add php-fpm part for ARM routers
ifneq ($(TCONFIG_BCMARM),y)
	sed -i $(INSTALLDIR)/www/web-nginx.asp -e "/BCMARM-BEGIN/,/BCMARM-END/d"
endif

# Clean up NGinX files if not needed
ifneq ($(TCONFIG_NGINX),y)
	rm -f $(INSTALLDIR)/www/web-nginx.asp
	rm -f $(INSTALLDIR)/www/web-mysql.asp
	rm -f $(INSTALLDIR)/www/index.html
	rm -f $(INSTALLDIR)/www/phpinfo.php
	rm -f $(INSTALLDIR)/www/adminer.php
	sed -i $(INSTALLDIR)/www/tomato.js -e "/NGINX-BEGIN/,/NGINX-END/d"
	sed -i $(INSTALLDIR)/www/about.asp -e "/NGINX-BEGIN/,/NGINX-END/d"
	sed -i $(INSTALLDIR)/www/isup.jsz -e "/NGINX-BEGIN/,/NGINX-END/d"
endif

# Only include HFS/HFS+ option and pages when configured.
ifneq ($(TCONFIG_HFS),y)
	sed -i $(INSTALLDIR)/www/nas-usb.asp -e "/HFS-BEGIN/,/HFS-END/d"
	sed -i $(INSTALLDIR)/www/about.asp -e "/HFS-BEGIN/,/HFS-END/d"
endif

# Only include ZFS option if is compiled in
ifneq ($(TCONFIG_ZFS),y)
	sed -i $(INSTALLDIR)/www/nas-usb.asp -e "/ZFS-BEGIN/,/ZFS-END/d"
	sed -i $(INSTALLDIR)/www/about.asp -e "/ZFS-BEGIN/,/ZFS-END/d"
endif

# Only include IPv6 options if IPv6 is configured in.
ifneq ($(TCONFIG_IPV6),y)
	cd $(INSTALLDIR)/www && \
	for F in $(wildcard *.asp *.js *.jsx *.jsz); do \
		[ -f $(INSTALLDIR)/www/$$F ] && sed -i $$F \
		-e "/IPV6-BEGIN/,/IPV6-END/d" \
		|| true; \
	done
	rm -f $(INSTALLDIR)/www/basic-ipv6.asp
endif

# Only include the Transmission binary path select if Transmission binaries is configured in.
# Also only then (when libcurl is included), let to choose curl as a connection checker.
ifneq ($(TCONFIG_BBT),y)
	sed -i $(INSTALLDIR)/www/nas-bittorrent.asp -e "/BBT-BEGIN/,/BBT-END/d"
	sed -i $(INSTALLDIR)/www/about.asp -e "/BBT-BEGIN/,/BBT-END/d"
	sed -i $(INSTALLDIR)/www/basic-network.asp -e "/BBT-BEGIN/,/BBT-END/d"
endif

# Only include the Transmission pages if Transmission is configured in.
ifneq ($(TCONFIG_BT),y)
	rm -f $(INSTALLDIR)/www/nas-bittorrent.asp
	sed -i $(INSTALLDIR)/www/tomato.js -e "/BT-BEGIN/,/BT-END/d"
	sed -i $(INSTALLDIR)/www/about.asp -e "/BT-BEGIN/,/BT-END/d"
	sed -i $(INSTALLDIR)/www/isup.jsz -e "/BT-BEGIN/,/BT-END/d"
endif

# Only include the UPS pages if apcupsd is configured in.
ifneq ($(TCONFIG_UPS),y)
	rm -f $(INSTALLDIR)/www/nas-ups.asp
	sed -i $(INSTALLDIR)/www/tomato.js -e "/UPS-BEGIN/,/UPS-END/d"
	sed -i $(INSTALLDIR)/www/about.asp -e "/UPS-BEGIN/,/UPS-END/d"
	sed -i $(INSTALLDIR)/www/nas-usb.asp -e "/UPS-BEGIN/,/UPS-END/d"
endif

# Only include the TOR pages if tor project is configured in.
ifneq ($(TCONFIG_TOR),y)
	rm -f $(INSTALLDIR)/www/advanced-tor.asp
	sed -i $(INSTALLDIR)/www/tomato.js -e "/TOR-BEGIN/,/TOR-END/d"
	sed -i $(INSTALLDIR)/www/about.asp -e "/TOR-BEGIN/,/TOR-END/d"
	sed -i $(INSTALLDIR)/www/advanced-dhcpdns.asp -e "/TOR-BEGIN/,/TOR-END/d"
	sed -i $(INSTALLDIR)/www/isup.jsz -e "/TOR-BEGIN/,/TOR-END/d"
endif

# Only include the USB and NAS pages if USB Support is configured in.
ifneq ($(TCONFIG_USB),y)
	rm -f $(INSTALLDIR)/www/nas-*.*
	rm -f $(INSTALLDIR)/www/wwan_parser.js
	rm -f $(INSTALLDIR)/www/wwan-sms.asp
	rm -f $(INSTALLDIR)/www/cell.gif
	sed -i $(INSTALLDIR)/www/tomato.js -e "/USB-BEGIN/,/USB-END/d"
	sed -i $(INSTALLDIR)/www/status-overview.asp -e "/USB-BEGIN/,/USB-END/d"
	sed -i $(INSTALLDIR)/www/admin-buttons.asp -e "/USB-BEGIN/,/USB-END/d"
	sed -i $(INSTALLDIR)/www/admin-access.asp -e "/USB-BEGIN/,/USB-END/d"
	sed -i $(INSTALLDIR)/www/about.asp -e "/USB-BEGIN/,/USB-END/d"
	sed -i $(INSTALLDIR)/www/basic-network.asp -e "/USB-BEGIN/,/USB-END/d"
	sed -i $(INSTALLDIR)/www/status-devices.asp -e "/USB-BEGIN/,/USB-END/d"
endif

# Only include the USB and NAS pages if REMOVE_USBAPP is NOT configured in.
ifeq ($(TCONFIG_REMOVE_USBAPP),y)
	rm -f $(INSTALLDIR)/www/nas-*.*
	rm -f $(INSTALLDIR)/www/cell.gif
	sed -i $(INSTALLDIR)/www/tomato.js -e "/USB-BEGIN/,/USB-END/d"
	sed -i $(INSTALLDIR)/www/admin-buttons.asp -e "/USB-BEGIN/,/USB-END/d"
	sed -i $(INSTALLDIR)/www/admin-access.asp -e "/USB-BEGIN/,/USB-END/d"
	sed -i $(INSTALLDIR)/www/status-devices.asp -e "/USB-BEGIN/,/USB-END/d"
endif

# Only include the CTF configuration if CTF is configured in.
ifneq ($(TCONFIG_CTF),y)
	sed -i $(INSTALLDIR)/www/advanced-misc.asp -e "/CTF-BEGIN/,/CTF-END/d"
	sed -i $(INSTALLDIR)/www/bwlimit.asp -e "/CTF-BEGIN/,/CTF-END/d"
	sed -i $(INSTALLDIR)/www/qos-settings.asp -e "/CTF-BEGIN/,/CTF-END/d"
endif

# Only include the Broadcom FastNAT configuration if BCMNAT is configured in and not in RT-AC branch.
ifneq ($(TCONFIG_BCMNAT),y)
	sed -i $(INSTALLDIR)/www/advanced-misc.asp -e "/BCMNAT-BEGIN/,/BCMNAT-END/d"
	sed -i $(INSTALLDIR)/www/bwlimit.asp -e "/BCMNAT-BEGIN/,/BCMNAT-END/d"
	sed -i $(INSTALLDIR)/www/qos-settings.asp -e "/BCMNAT-BEGIN/,/BCMNAT-END/d"
	sed -i $(INSTALLDIR)/www/restrict.asp -e "/BCMNAT-BEGIN/,/BCMNAT-END/d"
endif
ifneq ($(TCONFIG_BCMARM),y)
ifeq ($(CONFIG_BCMWL6),y)
	sed -i $(INSTALLDIR)/www/advanced-misc.asp -e "/BCMNAT-BEGIN/,/BCMNAT-END/d"
	sed -i $(INSTALLDIR)/www/bwlimit.asp -e "/BCMNAT-BEGIN/,/BCMNAT-END/d"
	sed -i $(INSTALLDIR)/www/qos-settings.asp -e "/BCMNAT-BEGIN/,/BCMNAT-END/d"
	sed -i $(INSTALLDIR)/www/restrict.asp -e "/BCMNAT-BEGIN/,/BCMNAT-END/d"
endif
endif

ifneq ($(TCONFIG_BCMWL6),y)
	sed -i $(INSTALLDIR)/www/basic-network.asp -e "/BCMWL6-BEGIN/,/BCMWL6-END/d"
	sed -i $(INSTALLDIR)/www/advanced-wlanvifs.asp -e "/BCMWL6-BEGIN/,/BCMWL6-END/d"
	sed -i $(INSTALLDIR)/www/status-overview.asp -e "/BCMWL6-BEGIN/,/BCMWL6-END/d"
endif

ifneq ($(TCONFIG_BCM714),y)
	sed -i $(INSTALLDIR)/www/advanced-wireless.asp -e "/BCMWL714-BEGIN/,/BCMWL714-END/d"
	sed -i $(INSTALLDIR)/www/admin-debug.asp -e "/BCMWL714-BEGIN/,/BCMWL714-END/d"
endif

# Only include the wireless band steering option if is compiled in
ifneq ($(TCONFIG_BCMBSD),y)
	sed -i $(INSTALLDIR)/www/basic-network.asp -e "/BSD-BEGIN/,/BSD-END/d"
endif

# Only include the roaming assistant option if is compiled in
ifneq ($(TCONFIG_ROAM),y)
	sed -i $(INSTALLDIR)/www/advanced-wireless.asp -e "/ROAM-BEGIN/,/ROAM-END/d"
endif

# Only include SSH stuff when needed
ifneq ($(TCONFIG_SSH),y)
	sed -i $(INSTALLDIR)/www/isup.jsz -e "/SSH-BEGIN/,/SSH-END/d"
endif

# Add "beta" tag if needed
ifeq ($(TOMATO_EXPERIMENTAL),1)
	cd $(INSTALLDIR)/www && \
	for F in $(wildcard *.asp); do \
		sed -e "s,<div class=\"title\">Tomato64</div>,<div class=\"title\">Tomato64 <small class=\"betaver\"><i>(beta)</i></small></div>," $$F > $$F.tmp; \
		mv $$F.tmp $$F; \
	done
endif

# Only include the vpn pages if OpenVPN is compiled in
# Remove AES ciphers from the GUI if openssl doesn't have an AES directory
ifeq ($(TCONFIG_OPENVPN),y)
	test -d ../$(OPENSSLDIR)/crypto/aes || sed -i $(INSTALLDIR)/www/vpn.js -e "/AES-BEGIN/,/AES-END/d"
	sed -i $(INSTALLDIR)/www/tomato.js -e "/VPN-BEGIN/d" -e "/VPN-END/d"
	sed -i $(INSTALLDIR)/www/admin-access.asp -e "/VPN-BEGIN/d" -e "/VPN-END/d"
	sed -i $(INSTALLDIR)/www/about.asp -e "/VPN-BEGIN/d" -e "/VPN-END/d"
else
	rm -f $(INSTALLDIR)/www/vpn-server.asp
	rm -f $(INSTALLDIR)/www/vpn-client.asp
	rm -f $(INSTALLDIR)/www/vpn.js
	sed -i $(INSTALLDIR)/www/tomato.js -e "/OPENVPN-BEGIN/,/OPENVPN-END/d"
	sed -i $(INSTALLDIR)/www/about.asp -e "/OPENVPN-BEGIN/,/OPENVPN-END/d"
	sed -i $(INSTALLDIR)/www/advanced-dhcpdns.asp -e "/OPENVPN-BEGIN/,/OPENVPN-END/d"
	sed -i $(INSTALLDIR)/www/isup.jsz -e "/OPENVPN-BEGIN/,/OPENVPN-END/d"
endif

# Only include the PPTPD pages if PPTPD is compiled in
ifeq ($(TCONFIG_PPTPD),y)
	sed -i $(INSTALLDIR)/www/tomato.js -e "/VPN-BEGIN/d" -e "/VPN-END/d"
	sed -i $(INSTALLDIR)/www/admin-access.asp -e "/VPN-BEGIN/d" -e "/VPN-END/d"
	sed -i $(INSTALLDIR)/www/about.asp -e "/VPN-BEGIN/d" -e "/VPN-END/d"
else
	rm -f $(INSTALLDIR)/www/vpn-pptp-server.asp
	rm -f $(INSTALLDIR)/www/vpn-pptp-online.asp
	rm -f $(INSTALLDIR)/www/vpn-pptp.asp
	sed -i $(INSTALLDIR)/www/tomato.js -e "/PPTPD-BEGIN/,/PPTPD-END/d"
	sed -i $(INSTALLDIR)/www/about.asp -e "/PPTPD-BEGIN/,/PPTPD-END/d"
	sed -i $(INSTALLDIR)/www/advanced-dhcpdns.asp -e "/PPTPD-BEGIN/,/PPTPD-END/d"
	sed -i $(INSTALLDIR)/www/isup.jsz -e "/PPTPD-BEGIN/,/PPTPD-END/d"
endif

# Only include the nfs pages if NFS is compiled in
ifneq ($(TCONFIG_NFS),y)
	rm -f $(INSTALLDIR)/www/admin-nfs.asp
	sed -i $(INSTALLDIR)/www/tomato.js -e "/NFS-BEGIN/,/NFS-END/d"
	sed -i $(INSTALLDIR)/www/about.asp -e "/NFS-BEGIN/,/NFS-END/d"
endif

# Only include the snmp pages if SNMP is compiled in
ifneq ($(TCONFIG_SNMP),y)
	rm -f $(INSTALLDIR)/www/admin-snmp.asp
	sed -i $(INSTALLDIR)/www/tomato.js -e "/SNMP-BEGIN/,/SNMP-END/d"
	sed -i $(INSTALLDIR)/www/about.asp -e "/SNMP-BEGIN/,/SNMP-END/d"
endif

# Only include the dnssec option if is compiled in
ifneq ($(TCONFIG_DNSSEC),y)
	sed -i $(INSTALLDIR)/www/advanced-dhcpdns.asp -e "/DNSSEC-BEGIN/,/DNSSEC-END/d"
	sed -i $(INSTALLDIR)/www/about.asp -e "/DNSSEC-BEGIN/,/DNSSEC-END/d"
endif

# Only include the Tinc page if it is compiled in
ifneq ($(TCONFIG_TINC),y)
	rm -f $(INSTALLDIR)/www/vpn-tinc.asp
	sed -i $(INSTALLDIR)/www/tomato.js -e "/TINC-BEGIN/,/TINC-END/d"
	sed -i $(INSTALLDIR)/www/about.asp -e "/TINC-BEGIN/,/TINC-END/d"
	sed -i $(INSTALLDIR)/www/isup.jsz -e "/TINC-BEGIN/,/TINC-END/d"
endif

# Only include the dnscrypt option if is compiled in
ifneq ($(TCONFIG_DNSCRYPT),y)
	sed -i $(INSTALLDIR)/www/advanced-dhcpdns.asp -e "/DNSCRYPT-BEGIN/,/DNSCRYPT-END/d"
	sed -i $(INSTALLDIR)/www/about.asp -e "/DNSCRYPT-BEGIN/,/DNSCRYPT-END/d"
endif

# Only include the stubby option if is compiled in
ifneq ($(TCONFIG_STUBBY),y)
	sed -i $(INSTALLDIR)/www/advanced-dhcpdns.asp -e "/STUBBY-BEGIN/,/STUBBY-END/d"
endif

# Only include the dnssec/stubby (common part) option if one of them is compiled in
ifeq ($(TCONFIG_STUBBY)$(TCONFIG_DNSSEC),)
	sed -i $(INSTALLDIR)/www/advanced-dhcpdns.asp -e "/STORSEC-BEGIN/,/STORSEC-END/d"
endif

# Only include the Iperf page if it is compiled in
ifneq ($(TCONFIG_IPERF),y)
	rm -f $(INSTALLDIR)/www/tools-iperf.asp
	sed -i $(INSTALLDIR)/www/tomato.js -e "/IPERF-BEGIN/,/IPERF-END/d"
	sed -i $(INSTALLDIR)/www/about.asp -e "/IPERF-BEGIN/,/IPERF-END/d"
endif

# Only include Adblock v2 if it is required (not for MIPS R1)
ifeq ($(or $(TCONFIG_BCMARM),$(TCONFIG_MIPSR2)),y)
	rm -f $(INSTALLDIR)/www/advanced-adblock-v1.asp
	mv $(INSTALLDIR)/www/advanced-adblock-v2.asp $(INSTALLDIR)/www/advanced-adblock.asp || true
else
	rm -f $(INSTALLDIR)/www/advanced-adblock-v2.asp
	mv $(INSTALLDIR)/www/advanced-adblock-v1.asp $(INSTALLDIR)/www/advanced-adblock.asp || true
endif

# Only include the https option if it is compiled in
ifneq ($(TCONFIG_HTTPS),y)
	rm -f $(INSTALLDIR)/www/advanced-adblock.asp
	sed -i $(INSTALLDIR)/www/tomato.js -e "/HTTPS-BEGIN/,/HTTPS-END/d"
	sed -i $(INSTALLDIR)/www/admin-access.asp -e "/HTTPS-BEGIN/,/HTTPS-END/d"
# depends on USB and NAS pages - only if USB Support is configured in
ifeq ($(TCONFIG_USB),y)
	sed -i $(INSTALLDIR)/www/nas-ftp.asp -e "/HTTPS-BEGIN/,/HTTPS-END/d"
endif
endif

# Only include Advanced Themes stuff if it is required
ifneq ($(TCONFIG_ADVTHEMES),y)
	rm -f $(INSTALLDIR)/www/resize-charts.js
	rm -f $(INSTALLDIR)/www/at.css
	rm -f $(INSTALLDIR)/www/at-blue.css
	rm -f $(INSTALLDIR)/www/at-dark.css
	rm -f $(INSTALLDIR)/www/at-green.css
	rm -f $(INSTALLDIR)/www/at-red.css
	sed -i $(INSTALLDIR)/www/admin-access.asp -e "/ADVTHEMES-BEGIN/,/ADVTHEMES-END/d"
	sed -i $(INSTALLDIR)/www/bwm-24.asp -e "/ADVTHEMES-BEGIN/,/ADVTHEMES-END/d"
	sed -i $(INSTALLDIR)/www/bwm-realtime.asp -e "/ADVTHEMES-BEGIN/,/ADVTHEMES-END/d"
	sed -i $(INSTALLDIR)/www/ipt-24.asp -e "/ADVTHEMES-BEGIN/,/ADVTHEMES-END/d"
	sed -i $(INSTALLDIR)/www/ipt-realtime.asp -e "/ADVTHEMES-BEGIN/,/ADVTHEMES-END/d"
	sed -i $(INSTALLDIR)/www/tools-survey.asp -e "/ADVTHEMES-BEGIN/,/ADVTHEMES-END/d"
	sed -i $(INSTALLDIR)/www/bwm-graph.svg -e "/ADVTHEMES-BEGIN/,/ADVTHEMES-END/d"
	sed -i $(INSTALLDIR)/www/ipt-graph.svg -e "/ADVTHEMES-BEGIN/,/ADVTHEMES-END/d"
	sed -i $(INSTALLDIR)/www/qos-graph.svg -e "/ADVTHEMES-BEGIN/,/ADVTHEMES-END/d"
endif

# Only include Discovery helper if it is required
ifneq ($(TCONFIG_BCMARM),y)
ifneq ($(TCONFIG_MIPSR2),y)
	sed -i $(INSTALLDIR)/www/status-devices.asp -e "/DISCOVERY-BEGIN/,/DISCOVERY-END/d"
endif
endif

# Only include the mdns option if it is compiled in
ifneq ($(TCONFIG_MDNS),y)
	sed -i $(INSTALLDIR)/www/advanced-dhcpdns.asp -e "/MDNS-BEGIN/,/MDNS-END/d"
endif

# Only include the QR Code page if it is compiled in
ifneq ($(TCONFIG_QRCODE),y)
	rm -f $(INSTALLDIR)/www/tools-qr.asp
	rm -f $(INSTALLDIR)/www/html5-qrcode.js
	rm -f $(INSTALLDIR)/www/qrcode.js
	sed -i $(INSTALLDIR)/www/tomato.js -e "/QRCODE-BEGIN/,/QRCODE-END/d"
	sed -i $(INSTALLDIR)/www/status-overview.asp -e "/QRCODE-BEGIN/,/QRCODE-END/d"
endif

# distinguish version if ARM or MIPS
ifeq ($(TCONFIG_OPENVPN),y)
ifneq ($(TCONFIG_BCMARM),y)
	sed -i $(INSTALLDIR)/www/isup.jsz -e "/BCMARM-BEGIN/,/BCMARM-END/d"
else
	sed -i $(INSTALLDIR)/www/isup.jsz -e "/BCMARMNO-BEGIN/,/BCMARMNO-END/d"
endif
endif

# add a note about WL Survey issue only on ARM routers
ifneq ($(TCONFIG_BCMARM),y)
	sed -i $(INSTALLDIR)/www/tools-survey.asp -e "/BCMARM-BEGIN/,/BCMARM-END/d"
endif

# only include isup.js helper if it is needed
ifeq ($(TCONFIG_FTP)$(TCONFIG_SAMBASRV)$(TCONFIG_MEDIA_SERVER)$(TCONFIG_PPTPD)$(TCONFIG_NGINX)$(TCONFIG_BT),)
	rm -f $(INSTALLDIR)/www/isup.js
endif

# only add for RT-N+ routers
ifeq ($(filter y,$(TCONFIG_BLINK) $(TCONFIG_BCMARM)),)
	sed -i $(INSTALLDIR)/www/basic-ipv6.asp -e "/RTNPLUS-BEGIN/,/RTNPLUS-END/d"
	sed -i $(INSTALLDIR)/www/basic-network.asp -e "/RTNPLUS-BEGIN/,/RTNPLUS-END/d"
	sed -i $(INSTALLDIR)/www/status-overview.asp -e "/RTNPLUS-BEGIN/,/RTNPLUS-END/d"
endif

# Only include udpxy/igmpproxy part if needed
ifneq ($(TCONFIG_PROXY),y)
	sed -i $(INSTALLDIR)/www/advanced-firewall.asp -e "/PROXY-BEGIN/,/PROXY-END/d"
endif

# Exclude features not supported Tomato64
ifeq ($(TCONFIG_TOMATO64),y)
	sed -i $(INSTALLDIR)/www/tomato.js -e "/TOMATO64-REMOVE-BEGIN/,/TOMATO64-REMOVE-END/d"
	sed -i $(INSTALLDIR)/www/bwm-common.js -e "/TOMATO64-REMOVE-BEGIN/,/TOMATO64-REMOVE-END/d"
	sed -i $(INSTALLDIR)/www/bwlimit.asp -e "/TOMATO64-REMOVE-BEGIN/,/TOMATO64-REMOVE-END/d"
	sed -i $(INSTALLDIR)/www/bwm-24.asp -e "/TOMATO64-REMOVE-BEGIN/,/TOMATO64-REMOVE-END/d"
	sed -i $(INSTALLDIR)/www/tomato.css -e "/TOMATO64-REMOVE-BEGIN/,/TOMATO64-REMOVE-END/d"
	sed -i $(INSTALLDIR)/www/nas-media.asp -e "/TOMATO64-REMOVE-BEGIN/,/TOMATO64-REMOVE-END/d" || true
	sed -i $(INSTALLDIR)/www/nas-samba.asp -e "/TOMATO64-REMOVE-BEGIN/,/TOMATO64-REMOVE-END/d" || true
	sed -i $(INSTALLDIR)/www/nas-usb.asp -e "/TOMATO64-REMOVE-BEGIN/,/TOMATO64-REMOVE-END/d"
	sed -i $(INSTALLDIR)/www/status-data.jsx -e "/TOMATO64-REMOVE-BEGIN/,/TOMATO64-REMOVE-END/d"
	sed -i $(INSTALLDIR)/www/qos-classify.asp -e "/TOMATO64-REMOVE-BEGIN/,/TOMATO64-REMOVE-END/d"
	sed -i $(INSTALLDIR)/www/qos-ctrate.asp -e "/TOMATO64-REMOVE-BEGIN/,/TOMATO64-REMOVE-END/d"
	sed -i $(INSTALLDIR)/www/admin-access.asp -e "/TOMATO64-REMOVE-BEGIN/,/TOMATO64-REMOVE-END/d"
	sed -i $(INSTALLDIR)/www/admin-config.asp -e "/TOMATO64-REMOVE-BEGIN/,/TOMATO64-REMOVE-END/d"
	sed -i $(INSTALLDIR)/www/admin-debug.asp -e "/TOMATO64-REMOVE-BEGIN/,/TOMATO64-REMOVE-END/d"
	sed -i $(INSTALLDIR)/www/admin-iptraffic.asp -e "/TOMATO64-REMOVE-BEGIN/,/TOMATO64-REMOVE-END/d"
	sed -i $(INSTALLDIR)/www/admin-log.asp -e "/TOMATO64-REMOVE-BEGIN/,/TOMATO64-REMOVE-END/d"
	sed -i $(INSTALLDIR)/www/admin-upgrade.asp -e "/TOMATO64-REMOVE-BEGIN/,/TOMATO64-REMOVE-END/d"
	sed -i $(INSTALLDIR)/www/advanced-access.asp -e "/TOMATO64-REMOVE-BEGIN/,/TOMATO64-REMOVE-END/d"
	sed -i $(INSTALLDIR)/www/advanced-ctnf.asp -e "/TOMATO64-REMOVE-BEGIN/,/TOMATO64-REMOVE-END/d"
	sed -i $(INSTALLDIR)/www/advanced-dhcpdns.asp -e "/TOMATO64-REMOVE-BEGIN/,/TOMATO64-REMOVE-END/d"
	sed -i $(INSTALLDIR)/www/advanced-firewall.asp -e "/TOMATO64-REMOVE-BEGIN/,/TOMATO64-REMOVE-END/d"
	sed -i $(INSTALLDIR)/www/advanced-routing.asp -e "/TOMATO64-REMOVE-BEGIN/,/TOMATO64-REMOVE-END/d"
	sed -i $(INSTALLDIR)/www/advanced-tor.asp -e "/TOMATO64-REMOVE-BEGIN/,/TOMATO64-REMOVE-END/d" || true
	sed -i $(INSTALLDIR)/www/advanced-vlan.asp -e "/TOMATO64-REMOVE-BEGIN/,/TOMATO64-REMOVE-END/d"
	sed -i $(INSTALLDIR)/www/basic-network.asp -e "/TOMATO64-REMOVE-BEGIN/,/TOMATO64-REMOVE-END/d"
	sed -i $(INSTALLDIR)/www/forward-upnp.asp -e "/TOMATO64-REMOVE-BEGIN/,/TOMATO64-REMOVE-END/d"
	sed -i $(INSTALLDIR)/www/reboot.asp -e "/TOMATO64-REMOVE-BEGIN/,/TOMATO64-REMOVE-END/d"
	sed -i $(INSTALLDIR)/www/reboot-default.asp -e "/TOMATO64-REMOVE-BEGIN/,/TOMATO64-REMOVE-END/d"
	sed -i $(INSTALLDIR)/www/restrict-edit.asp -e "/TOMATO64-REMOVE-BEGIN/,/TOMATO64-REMOVE-END/d"
	sed -i $(INSTALLDIR)/www/shutdown.asp -e "/TOMATO64-REMOVE-BEGIN/,/TOMATO64-REMOVE-END/d"
	sed -i $(INSTALLDIR)/www/splashd.asp -e "/TOMATO64-REMOVE-BEGIN/,/TOMATO64-REMOVE-END/d" || true
	sed -i $(INSTALLDIR)/www/status-devices.asp -e "/TOMATO64-REMOVE-BEGIN/,/TOMATO64-REMOVE-END/d"
	sed -i $(INSTALLDIR)/www/status-overview.asp -e "/TOMATO64-REMOVE-BEGIN/,/TOMATO64-REMOVE-END/d"
	sed -i $(INSTALLDIR)/www/vpn-server.asp -e "/TOMATO64-REMOVE-BEGIN/,/TOMATO64-REMOVE-END/d" || true
	sed -i $(INSTALLDIR)/www/vpn-wireguard.asp -e "/TOMATO64-REMOVE-BEGIN/,/TOMATO64-REMOVE-END/d"

	sed -i 's,___HASH___,${HASH},g' $(INSTALLDIR)/www/about.asp
	sed -i 's,___HASH_SHORT___,${HASH_SHORT},g' $(INSTALLDIR)/www/about.asp
else
	sed -i $(INSTALLDIR)/www/tomato.js -e "/TOMATO64-BEGIN/,/TOMATO64-END/d"
	sed -i $(INSTALLDIR)/www/bwm-common.js -e "/TOMATO64-BEGIN/,/TOMATO64-END/d"
	sed -i $(INSTALLDIR)/www/bwlimit.asp -e "/TOMATO64-BEGIN/,/TOMATO64-END/d"
	sed -i $(INSTALLDIR)/www/bwm-24.asp -e "/TOMATO64-BEGIN/,/TOMATO64-END/d"
	sed -i $(INSTALLDIR)/www/nas-media.asp -e "/TOMATO64-BEGIN/,/TOMATO64-END/d" || true
	sed -i $(INSTALLDIR)/www/nas-samba.asp -e "/TOMATO64-BEGIN/,/TOMATO64-END/d" || true
	sed -i $(INSTALLDIR)/www/nas-usb.asp -e "/TOMATO64-BEGIN/,/TOMATO64-END/d"
	sed -i $(INSTALLDIR)/www/status-data.jsx -e "/TOMATO64-BEGIN/,/TOMATO64-END/d"
	sed -i $(INSTALLDIR)/www/advanced-access.asp -e "/TOMATO64-BEGIN/,/TOMATO64-END/d"
	sed -i $(INSTALLDIR)/www/advanced-ctnf.asp -e "/TOMATO64-BEGIN/,/TOMATO64-END/d"
	sed -i $(INSTALLDIR)/www/advanced-dhcpdns.asp -e "/TOMATO64-BEGIN/,/TOMATO64-END/d"
	sed -i $(INSTALLDIR)/www/advanced-firewall.asp -e "/TOMATO64-BEGIN/,/TOMATO64-END/d"
	sed -i $(INSTALLDIR)/www/advanced-routing.asp -e "/TOMATO64-BEGIN/,/TOMATO64-END/d"
	sed -i $(INSTALLDIR)/www/advanced-tor.asp -e "/TOMATO64-BEGIN/,/TOMATO64-END/d" || true
	sed -i $(INSTALLDIR)/www/advanced-vlan.asp -e "/TOMATO64-BEGIN/,/TOMATO64-END/d"
	sed -i $(INSTALLDIR)/www/admin-access.asp -e "/TOMATO64-BEGIN/,/TOMATO64-END/d"
	sed -i $(INSTALLDIR)/www/admin-iptraffic.asp -e "/TOMATO64-BEGIN/,/TOMATO64-END/d"
	sed -i $(INSTALLDIR)/www/admin-log.asp -e "/TOMATO64-BEGIN/,/TOMATO64-END/d"
	sed -i $(INSTALLDIR)/www/admin-upgrade.asp -e "/TOMATO64-BEGIN/,/TOMATO64-END/d"
	sed -i $(INSTALLDIR)/www/basic-network.asp -e "/TOMATO64-BEGIN/,/TOMATO64-END/d"
	sed -i $(INSTALLDIR)/www/forward-upnp.asp -e "/TOMATO64-BEGIN/,/TOMATO64-END/d"
	sed -i $(INSTALLDIR)/www/reboot-default.asp -e "/TOMATO64-BEGIN/,/TOMATO64-END/d"
	sed -i $(INSTALLDIR)/www/reboot.asp -e "/TOMATO64-BEGIN/,/TOMATO64-END/d"
	sed -i $(INSTALLDIR)/www/restrict-edit.asp -e "/TOMATO64-BEGIN/,/TOMATO64-END/d"
	sed -i $(INSTALLDIR)/www/shutdown.asp -e "/TOMATO64-BEGIN/,/TOMATO64-END/d"
	sed -i $(INSTALLDIR)/www/splashd.asp -e "/TOMATO64-BEGIN/,/TOMATO64-END/d" || true
	sed -i $(INSTALLDIR)/www/status-devices.asp -e "/TOMATO64-BEGIN/,/TOMATO64-END/d"
	sed -i $(INSTALLDIR)/www/status-overview.asp -e "/TOMATO64-BEGIN/,/TOMATO64-END/d"
	sed -i $(INSTALLDIR)/www/vpn-server.asp -e "/TOMATO64-BEGIN/,/TOMATO64-END/d" || true
	sed -i $(INSTALLDIR)/www/vpn-wireguard.asp -e "/TOMATO64-BEGIN/,/TOMATO64-END/d"
	sed -i $(INSTALLDIR)/www/qos-classify.asp -e "/TOMATO64-BEGIN/,/TOMATO64-END/d"
	sed -i $(INSTALLDIR)/www/qos-ctrate.asp -e "/TOMATO64-BEGIN/,/TOMATO64-END/d"
endif

# Only include TFTP if it is compiled in
ifneq ($(TCONFIG_USB_EXTRAS),y)
	sed -i $(INSTALLDIR)/www/advanced-dhcpdns.asp -e "/TFTP-BEGIN/,/TFTP-END/d"
endif

# Only include wireguard page if it is compiled in
ifneq ($(TCONFIG_WIREGUARD),y)
	rm -f $(INSTALLDIR)/www/vpn-wireguard.asp
	rm -f $(INSTALLDIR)/www/cfg-icon.svg
	rm -f $(INSTALLDIR)/www/html5-qrcode.js
#	rm -f $(INSTALLDIR)/www/interfaces-v6.js
	rm -f $(INSTALLDIR)/www/qr-icon.svg
	rm -f $(INSTALLDIR)/www/wireguard.js
	sed -i $(INSTALLDIR)/www/tomato.js -e "/WIREGUARD-BEGIN/,/WIREGUARD-END/d"
endif

# Only include 4 DDNS clients if nvram >= 64KB and optimize size is disabled
ifeq ($(or $(TCONFIG_NVRAM_32K),$(TCONFIG_OPTIMIZE_SIZE)),y)
	sed -i $(INSTALLDIR)/www/basic-ddns.asp -e "/DDNS4CLS-BEGIN/,/DDNS4CLS-END/d"
else
	sed -i $(INSTALLDIR)/www/basic-ddns.asp -e "/DDNS2CLS-BEGIN/,/DDNS2CLS-END/d"
endif

# clean up
	cd $(INSTALLDIR)/www && \
	for F in $(wildcard *.asp *.js *.jsx *.jsz *.html); do \
		[ -f $(INSTALLDIR)/www/$$F ] && sed -i $$F \
		-e "/USB-BEGIN/d"	-e "/USB-END/d" \
		-e "/UPS-BEGIN/d"	-e "/UPS-END/d" \
		-e "/EXTRAS-BEGIN/d"	-e "/EXTRAS-END/d" \
		-e "/NTFS-BEGIN/d"	-e "/NTFS-END/d" \
		-e "/SAMBA-BEGIN/d"	-e "/SAMBA-END/d" \
		-e "/FTP-BEGIN/d"	-e "/FTP-END/d" \
		-e "/MEDIA-SRV-BEGIN/d"	-e "/MEDIA-SRV-END/d" \
		-e "/JFFS2-BEGIN/d"	-e "/JFFS2-END/d" \
		-e "/JFFS2NAND-BEGIN/d"	-e "/JFFS2NAND-END/d" \
		-e "/CIFS-BEGIN/d"	-e "/CIFS-END/d" \
		-e "/EMF-BEGIN/d"	-e "/EMF-END/d" \
		-e "/OPENVPN-BEGIN/d"	-e "/OPENVPN-END/d" \
		-e "/AES-BEGIN/d"	-e "/AES-END/d" \
		-e "/PPTPD-BEGIN/d"	-e "/PPTPD-END/d"\
		-e "/VPN-BEGIN/,/VPN-END/d" \
		-e "/IPV6-BEGIN/d"	-e "/IPV6-END/d" \
		-e "/CTF-BEGIN/d"	-e "/CTF-END/d" \
		-e "/BCMNAT-BEGIN/d"	-e "/BCMNAT-END/d" \
		-e "/BCMWL6-BEGIN/d"	-e "/BCMWL6-END/d" \
		-e "/BCMWL714-BEGIN/d"	-e "/BCMWL714-END/d" \
		-e "/BSD-BEGIN/d"	-e "/BSD-END/d" \
		-e "/ROAM-BEGIN/d"	-e "/ROAM-END/d" \
		-e "/BBT-BEGIN/d"	-e "/BBT-END/d" \
		-e "/BT-BEGIN/d"	-e "/BT-END/d" \
		-e "/NFS-BEGIN/d"	-e "/NFS-END/d" \
		-e "/NOCAT-BEGIN/d"	-e "/NOCAT-END/d" \
		-e "/NGINX-BEGIN/d"	-e "/NGINX-END/d" \
		-e "/SNMP-BEGIN/d"	-e "/SNMP-END/d" \
		-e "/HFS-BEGIN/d"	-e "/HFS-END/d" \
		-e "/DNSCRYPT-BEGIN/d"	-e "/DNSCRYPT-END/d" \
		-e "/STUBBY-BEGIN/d"	-e "/STUBBY-END/d" \
		-e "/DNSSEC-BEGIN/d"	-e "/DNSSEC-END/d" \
		-e "/STORSEC-BEGIN/d"	-e "/STORSEC-END/d" \
		-e "/TOR-BEGIN/d"	-e "/TOR-END/d" \
		-e "/MICROSD-BEGIN/d"	-e "/MICROSD-END/d" \
		-e "/TINC-BEGIN/d"	-e "/TINC-END/d" \
		-e "/PARAGON-BEGIN/d"	-e "/PARAGON-END/d" \
		-e "/TUXERA-BEGIN/d"	-e "/TUXERA-END/d" \
		-e "/TUXERAHFS-BEGIN/d"	-e "/TUXERAHFS-END/d" \
		-e "/MULTIWAN-BEGIN/d"	-e "/MULTIWAN-END/d" \
		-e "/DUALWAN-BEGIN/d"	-e "/DUALWAN-END/d" \
		-e "/HTTPS-BEGIN/d"	-e "/HTTPS-END/d" \
		-e "/IPERF-BEGIN/d"	-e "/IPERF-END/d" \
		-e "/ADVTHEMES-BEGIN/d"	-e "/ADVTHEMES-END/d" \
		-e "/SSH-BEGIN/d"	-e "/SSH-END/d" \
		-e "/DISCOVERY-BEGIN/d"	-e "/DISCOVERY-END/d" \
		-e "/MDNS-BEGIN/d"	-e "/MDNS-END/d" \
		-e "/ZFS-BEGIN/d"	-e "/ZFS-END/d" \
		-e "/QRCODE-BEGIN/d"	-e "/QRCODE-END/d" \
		-e "/BCMARM-BEGIN/d"	-e "/BCMARM-END/d" \
		-e "/BCMARMNO-BEGIN/d"	-e "/BCMARMNO-END/d" \
		-e "/RTNPLUS-BEGIN/d"	-e "/RTNPLUS-END/d" \
		-e "/PROXY-BEGIN/d"	-e "/PROXY-END/d" \
		-e "/TFTP-BEGIN/d"	-e "/TFTP-END/d" \
		-e "/WIREGUARD-BEGIN/d"	-e "/WIREGUARD-END/d" \
		-e "/DDNS4CLS-BEGIN/d"	-e "/DDNS4CLS-END/d" \
		-e "/DDNS2CLS-BEGIN/d"	-e "/DDNS2CLS-END/d" \
		-e "/TOMATO64-BEGIN/d"	-e "/TOMATO64-END/d" \
		-e "/TOMATO64-REMOVE-BEGIN/d"	-e "/TOMATO64-REMOVE-END/d" \
		|| true; \
	done

# remove C-style comments from java, asp and css files. All "control" comments have been processed by now.
	for F in $(wildcard *.js *.jsx *.jsz *.asp *.css); do \
		[ -f $(INSTALLDIR)/www/$$F ] && $(BR2_EXTERNAL_TOMATO64_PATH)/package/www/www/remcoms2.sh $(INSTALLDIR)/www/$$F c; \
	done

ifneq ($(TCONFIG_BCMARM),y)
# use HTML compressor to compress HTML as much as possible
#	cd $(INSTALLDIR)/www && \
#	for F in $(wildcard *.asp); do \
#		[ -s $(INSTALLDIR)/www/$$F ] && { echo $$F; $(TOP)/www/tools/bin/html-minifier $$F --minify-css -o $$F; } || true; \
#	done

# compress JAVASCRIPT files
	cd $(INSTALLDIR)/www && \
	for F in $(wildcard *.js); do \
		[ -s $(INSTALLDIR)/www/$$F ] && { echo $$F; $(TOP)/www/tools/bin/uglifyjs $$F -c -o $$F; } || true; \
	done

# compress CSS files
	cd $(INSTALLDIR)/www && \
	for F in $(wildcard *.css); do \
		[ -s $(INSTALLDIR)/www/$$F ] && { echo $$F; $(TOP)/www/tools/bin/uglifycss $$F --output $$F; } || true; \
	done
endif

	chmod 0644 $(INSTALLDIR)/www/*
