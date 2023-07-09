#!/bin/sh

set -e

# Tomato64 will create and populate these directories on boot
rm -rf \
$TARGET_DIR/etc \
$TARGET_DIR/home \
$TARGET_DIR/mnt \
$TARGET_DIR/root \
$TARGET_DIR/var

# Needed for some hook scripts
mkdir -p \
$TARGET_DIR/tmp/etc \
$TARGET_DIR/tmp/home/root \
$TARGET_DIR/tmp/mnt \
$TARGET_DIR/tmp/var

# Create symbolic links to the tmp tmpfs.
ln -sf tmp/etc $TARGET_DIR/etc
ln -sf tmp/home $TARGET_DIR/home
ln -sf tmp/home/root $TARGET_DIR/root
ln -sf tmp/mnt $TARGET_DIR/mnt
ln -sf tmp/var $TARGET_DIR/var

# create ldd symlink
ln -sf /lib/ld-musl-x86_64.so.1 $TARGET_DIR/usr/bin/ldd

# symlink openssl since Tomato expects it in a non-standard place
ln -sf /usr/bin/openssl $TARGET_DIR/usr/sbin/openssl

# symlink mysqld since Tomato expects it in a non-standard place
ln -sf /usr/sbin/mysqld $TARGET_DIR/usr/bin/mysqld

# Symlinks for nfs-utils binaries
ln -sf rpc.statd $TARGET_DIR/usr/sbin/statd
ln -sf rpc.nfsd $TARGET_DIR/usr/sbin/nfsd
ln -sf rpc.mountd $TARGET_DIR/usr/sbin/mountd

# PHP symlinks
ln -sf /usr/bin/php $TARGET_DIR/usr/sbin/php-cli
ln -sf /usr/bin/php-cgi $TARGET_DIR/usr/sbin/php-cgi
ln -sf /usr/bin/php-cgi $TARGET_DIR/usr/sbin/php-fcgi

# iperf symlink
ln -sf iperf3 $TARGET_DIR/usr/bin/iperf

# To make buildroot genimage happy
touch $TARGET_DIR/tmp/etc/group
touch $TARGET_DIR/tmp/etc/passwd
touch $TARGET_DIR/tmp/etc/shadow