#!/bin/sh

set -e

# Tomato64 will create and populate these directories on boot
rm -rf \
$TARGET_DIR/etc \
$TARGET_DIR/home \
$TARGET_DIR/mnt \
$TARGET_DIR/root \
$TARGET_DIR/var \
$TARGET_DIR/run

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
ln -sf tmp/var/run $TARGET_DIR/run

# create ldd symlink
if grep -q ^BR2_x86_64=y ${BR2_CONFIG}
then
	ln -sf /lib/ld-musl-x86_64.so.1 $TARGET_DIR/usr/bin/ldd
fi

if grep -q ^BR2_aarch64=y ${BR2_CONFIG}
then
	ln -sf /lib/ld-musl-aarch64.so.1 $TARGET_DIR/usr/bin/ldd
fi

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

# ntfs symlink
ln -sf mount.ntfs-3g $TARGET_DIR/sbin/mount.ntfs3

# symlinks for wireguard
ln -sf /usr/bin/wg $TARGET_DIR/usr/sbin/wg
ln -sf /sbin/ip $TARGET_DIR/usr/sbin/ip

# To make buildroot genimage happy
touch $TARGET_DIR/tmp/etc/group
touch $TARGET_DIR/tmp/etc/passwd
touch $TARGET_DIR/tmp/etc/shadow

# symlinks from others/rootprep.sh
ln -sf /tmp/var/wwwext $TARGET_DIR/www/ext
ln -sf /tmp/var/wwwext $TARGET_DIR/www/user
ln -sf /www/ext/proxy.pac $TARGET_DIR/www/proxy.pac
ln -sf /www/ext/proxy.pac $TARGET_DIR/www/wpad.dat

# usb_modeswitch.conf file
USB_MODESWITCH_DIR=$BUILD_DIR/$(ls $BUILD_DIR --ignore='*data*' | grep usb_modeswitch)
sed -e '/^\s*#.*$/d' -e '/^\s*$/d' < $USB_MODESWITCH_DIR/usb_modeswitch.conf > $TARGET_DIR/rom/etc/usb_modeswitch.conf

#GL-MT6000 specific edits
if grep -q ^BR2_aarch64=y ${BR2_CONFIG}
then
	ln -sf /usr/bin/uci $TARGET_DIR/sbin/uci
fi

# Remove unneeded mariadb stuff
cd $TARGET_DIR/usr/bin && rm -f myisam_ftdump myisamlog myisampack mysql_client_test mariadb-client-test mysql_client_test_embedded mariadb-client-test-embedded mysql_convert_table_format mariadb-convert-table-format mysql_embedded mariadb-embedded mysql_find_rows mariadb-find-rows mysql_fix_extensions mariadb-fix-extensions mysql_plugin mariadb-plugin mysql_secure_installation mariadb-secure-installation mysql_setpermission mariadb-setpermission mysql_tzinfo_to_sql mariadb-tzinfo-to-sql mysql_upgrade mariadb-upgrade mysql_waitpid mariadb-waitpid mysqlaccess mariadb-access mysqlbinlog mariadb-binlog mysqlcheck mariadb-check mysqld_multi mariadbd-multi mysqld_safe mariadbd-safe mysqld_safe_helper mariadbd-safe-helper mysqldumpslow mariadb-dumpslow mysqlhotcopy mariadb-hotcopy mysqlimport mariadb-import mysqlshow mariadb-show mysqlslap mariadb-slap mysqltest mariadb-test mysqltest_embedded mariadb-test-embedded msql2mysql wsrep_sst_backup wsrep_sst_common wsrep_sst_mariabackup wsrep_sst_mysqldump wsrep_sst_rsync wsrep_sst_rsync_wan aria_chk aria_dump_log aria_ftdump aria_pack aria_read_log aria_s3_copy mariabackup mariadb-backup mariadb-config
