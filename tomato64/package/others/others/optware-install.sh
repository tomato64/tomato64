#!/bin/sh
# Optware pre-installation script, Leon Kos 2006-2008
# Broadcom ARM support - Shibby 2014
export PATH=/bin:/usr/bin:/sbin:/usr/sbin:/home/root:
# BCMARM-BEGIN
REPOSITORY=http://ipkg.nslu2-linux.org/feeds/optware/mbwe-bluering/cross/stable
# BCMARM-END
# BCMARMNO-BEGIN
REPOSITORY=http://ipkg.nslu2-linux.org/feeds/optware/oleg/cross/stable
# BCMARMNO-END

TMP=/tmp
# STUBBYNO-BEGIN
WGET="/usr/bin/wget --no-check-certificate"
# STUBBYNO-END
# STUBBY-BEGIN
WGET="/usr/bin/wget"
# STUBBY-END

PATH=/bin:/sbin:/usr/bin:/usr/sbin:/opt/bin:/opt/sbin
unset LD_PRELOAD
unset LD_LIBRARY_PATH

_check_config() {
	echo "Checking system config ..."
	GATEWAY=$(netstat -rn | sed -n 's/^0.0.0.0[ \t]\{1,\}\([0-9.]\{8,\}\).*/\1/p')
	[ -n "${GATEWAY}" ] && {
		echo "Using ${GATEWAY} as default gateway."
	} || {
		echo "Error: No default gateway set!"
		exit 2
	}
	[ -s /etc/resolv.conf ] && {
		echo "Using the following nameserver(s):"
		if grep nameserver /etc/resolv.conf ; then
			GATEWAY_SUBNET=$(echo "${GATEWAY}" |
			sed 's/\.[0-9]\{1,3\}\.[0-9]\{1,3\}$//')
			[ "${GATEWAY_SUBNET}" = "192.168" ] && {
				if grep -q ${GATEWAY} /etc/resolv.conf ; then
					echo "Gateway ${GATEWAY} is also nameserver."
				else
					echo "Warning: local nameserver is different than gateway!"
					echo "Check config or enter:"
					if test -L /etc/resolv.conf ; then 
						echo "  sed -i s/192.168.*/${GATEWAY}/ /tmp/resolv.conf"
					else
						echo "  sed -i s/192.168.*/${GATEWAY}/ /etc/resolv.conf"
					fi
					echo "to correct this."
				fi
			}
		else
			echo "Error: No nameserver specified in /etc/resolv.conf"
			exit 5
		fi
	} || {
		echo "Error: Empty or nonexistent /etc/resolv.conf"
		exit 3
	}

	if mount | grep -q /opt ; then
		[ -d /opt/etc ] && echo "Warning: /opt partition not empty!"
	else
		echo "Error: /opt partition not mounted."
		echo "Enter"
		echo "    mkdir /jffs/opt"
		echo "    mount -o bind /jffs/opt /opt"
		echo "to correct this."
		exit 4
	fi
}

_install_package() {
	PACKAGE=$1
	echo "Installing package ${PACKAGE} ..."
	$WGET -O ${TMP}/${PACKAGE} ${REPOSITORY}/${PACKAGE}
	cd  ${TMP} 
	tar xzf ${TMP}/${PACKAGE} 
	tar xzf ${TMP}/control.tar.gz
	cd /
	[ -f ${TMP}/preinst ] && {
		sh ${TMP}/preinst
		rm -f ${TMP}/preints
	}
	tar xzf ${TMP}/data.tar.gz
	[ -f ${TMP}/postinst ] && {
		sh ${TMP}/postinst
		rm -f ${TMP}/postinst
	}
	rm -f ${TMP}/data.tar.gz
	rm -f ${TMP}/control.tar.gz
	rm -f ${TMP}/control
	rm -f ${TMP}/${PACKAGE}
}

_check_config
# BCMARM-BEGIN
_install_package uclibc-opt_0.9.28-1_arm.ipk
_install_package ipkg-opt_0.99.163-10_arm.ipk
# BCMARM-END
# BCMARMNO-BEGIN
_install_package uclibc-opt_0.9.28-13_mipsel.ipk
_install_package ipkg-opt_0.99.163-10_mipsel.ipk
/opt/sbin/ldconfig
# BCMARMNO-END
/opt/bin/ipkg update
/opt/bin/ipkg install -force-reinstall uclibc-opt
/opt/bin/ipkg install -force-reinstall ipkg-opt

# BCMARM-BEGIN
## ipkg.conf
echo "src/gz nslu2 http://ipkg.nslu2-linux.org/feeds/optware/mbwe-bluering/cross/stable" > /opt/etc/ipkg.conf
echo "src shibby http://tomato.groov.pl/repo-arm" >> /opt/etc/ipkg.conf
echo "dest /opt/ /" >> /opt/etc/ipkg.conf
# BCMARM-END
# BCMARMNO-BEGIN
## shibby`s repository ##
echo "src shibby http://tomato.groov.pl/repo" >> /opt/etc/ipkg.conf
# BCMARMNO-END
/opt/bin/ipkg update
