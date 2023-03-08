#!/bin/sh

#
# VPN Client up down script
#
# 2019 - 2020 by pedro
#


PID=$$
IFACE=$dev
SERVICE=$(echo $dev | sed 's/\(tun\|tap\)1/client/;s/\(tun\|tap\)2/server/')
DNSDIR="/etc/openvpn/dns"
DNSCONFFILE="$DNSDIR/$SERVICE.conf"
DNSRESOLVFILE="$DNSDIR/$SERVICE.resolv"
FOREIGN_OPTIONS=$(set | grep "^foreign_option_" | sed "s/^\(.*\)=.*$/\1/g")
LOGS="logger -t openvpn-updown-client.sh[$PID][$IFACE]"
FILEEXISTS=""


startAdns() {
	local optionname option

	[ -n "$FOREIGN_OPTIONS" ] & {
		$LOGS "FOREIGN_OPTIONS: $FOREIGN_OPTIONS"

		for optionname in $FOREIGN_OPTIONS; do
			option=$(eval "echo \$$optionname")
			$LOGS "Optionname: $optionname, Option: $option"
			if echo $option | grep "dhcp-option WINS ";	then echo $option | sed "s/ WINS /=44,/" >> $DNSCONFFILE; fi
			if echo $option | grep "dhcp-option DNS";	then echo $option | sed "s/dhcp-option DNS/nameserver/" >> $DNSRESOLVFILE; fi
			if echo $option | grep "dhcp-option DOMAIN";	then echo $option | sed "s/dhcp-option DOMAIN/search/" >> $DNSRESOLVFILE; fi
		done
	}

	[ ! -f $DNSCONFFILE -a ! -f $DNSRESOLVFILE ] && {
		$LOGS "Warning: 'Accept DNS configuration' is enabled but no foreign options (push dhcp-option) have been received from the OpenVPN server!"
	}
}

stopAdns() {
	# nothing here, for now...
	return
}


###################################################


[ "$SERVICE" == "" ] || [ "$(nvram get "vpn_"$SERVICE"_adns")" -eq 0 ] && exit 0

[ ! -d $DNSDIR ] && mkdir $DNSDIR
[ -f $DNSCONFFILE ] && {
	rm $DNSCONFFILE
	FILEEXISTS=1
}
[ -f $DNSRESOLVFILE ] && {
	rm $DNSRESOLVFILE
	FILEEXISTS=1
}

[ "$script_type" == "up" ] && {
	startAdns
}

[ "$script_type" == "down" ] && {
	stopAdns
}

[ -f $DNSCONFFILE -o -f $DNSRESOLVFILE -o -n "$FILEEXISTS" ] && service dnsmasq restart

rmdir $DNSDIR

exit 0
