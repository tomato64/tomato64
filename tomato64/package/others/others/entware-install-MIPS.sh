#!/bin/sh

export PATH=/opt/sbin:/opt/bin:$PATH
unset LD_LIBRARY_PATH
unset LD_PRELOAD
# STUBBYNO-BEGIN
WGET="/usr/bin/wget --no-check-certificate"
# STUBBYNO-END
# STUBBY-BEGIN
WGET="/usr/bin/wget"
# STUBBY-END

URL=http://pkg.entware.net/binaries/mipsel/installer

echo "Info: Creating folders..."
for folder in bin etc/init.d lib/opkg sbin share tmp usr var/log var/lock var/run; do
	[ -d "/opt/$folder" ] && {
		echo "Warning: Folder /opt/$folder exists! If something goes wrong please clean /opt folder and try again."
	} || {
		mkdir -p /opt/$folder
	}
done

dl () {
	# $1 - URL to download
	# $2 - place to store
	# $3 - 'x' if should be executable
	echo -n "Downloading $2... "
	$WGET -q $1 -O $2
	[ $? -eq 0 ] && {
		echo "success!"
	} || {
		echo "failed!"
		exit 1
	}
	[ -z "$3" ] || chmod +x $2
}

echo "Info: Deploying opkg package manager..."
dl $URL/opkg /opt/bin/opkg x
dl $URL/opkg.conf /opt/etc/opkg.conf
dl $URL/profile /opt/etc/profile x
dl $URL/rc.func /opt/etc/init.d/rc.func
dl $URL/rc.unslung /opt/etc/init.d/rc.unslung x

echo "Info: Basic packages installation..."
/opt/bin/opkg update
/opt/bin/opkg install ldconfig findutils
/opt/sbin/ldconfig &>/dev/null
[ -f /etc/TZ ] && ln -sf /etc/TZ /opt/etc/TZ

cat << EOF

Congratulations! If there are no errors above then Entware-ng is successfully initialized.

Found a Bug? Please report at https://github.com/Entware-ng/Entware-ng/issues

Type 'opkg install <pkg_name>' to install necessary package.

EOF
