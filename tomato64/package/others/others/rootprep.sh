#!/bin/bash

ROOTDIR=$PWD

mkdir -p -m 0755 dev
mkdir -p -m 0755 proc
mkdir -p -m 0755 sys
mkdir -p -m 0755 tmp
mkdir -p -m 0755 jffs
mkdir -p -m 0755 cifs1
mkdir -p -m 0755 cifs2
mkdir -p -m 0755 opt

mkdir -p -m 0755 mmc
mkdir -p -m 0755 usr/local
case "$1" in
	ln_usr_share)
		ln -sf /tmp/share usr/share
	;;
esac
ln -sf /tmp/share usr/local/share

ln -sf tmp/mnt mnt
ln -sf tmp/var var
ln -sf tmp/etc etc
ln -sf tmp/home home
ln -sf tmp/home/root root
(cd usr && ln -sf ../tmp)

ln -sf /tmp/var/wwwext www/ext
ln -sf /tmp/var/wwwext www/user
ln -sf /www/ext/proxy.pac www/proxy.pac
ln -sf /www/ext/proxy.pac www/wpad.dat

mkdir -p -m 0755 nas
mkdir -p -m 0755 bkp
mkdir -p -m 0777 tftpboot

# Tor GeoIP DB in /tmp
case "$1" in
	ln_tor_geoip)
		ln -sf /tmp/tor usr/share/tor
	;;
esac
