#!/bin/sh

#
# VPN Client selective routing up down script
#
# Copyright by pedro 2019 - 2022
#


PID=$$
IFACE=$dev
SERVICE=$(echo $dev | sed 's/\(tun\|tap\)1/client/;s/\(tun\|tap\)2/server/')
FIREWALL_ROUTING="/etc/openvpn/fw/$SERVICE-fw-routing.sh"
DNSMASQ_IPSET="/etc/dnsmasq.ipset"
RESTART_DNSMASQ=0
RESTART_FW=0
ID="0"
LOGS="logger -t openvpn-vpnrouting.sh[$PID][$IFACE]"
[ -d /etc/openvpn/fw ] || mkdir -m 0700 "/etc/openvpn/fw"


NV() {
	nvram get "$1"
}

find_iface() {
	# These IDs were intentionally picked to avoid overwriting
	# marks set by QoS. See qos.c
	if [ "$SERVICE" == "client1" ]; then
		ID="2304" # 0x900
	elif [ "$SERVICE" == "client2" ]; then
		ID="2560" # 0xA00
# BCMARM-BEGIN
	elif [ "$SERVICE" == "client3" ]; then
		ID="2816" # 0xB00
# BCMARM-END
	else
		$LOGS "Interface not found!"
		exit 0
	fi

	PIDFILE="/var/run/vpnrouting$ID.pid"
}

initTable() {
	local ROUTE
	$LOGS "Creating VPN routing table (mode $VPN_REDIR)"

	[ "$VPN_REDIR" -eq 3 ] && {
		ip route show table main dev $IFACE | while read ROUTE; do
			ip route add table $ID $ROUTE dev $IFACE
		done
	}
	# copy routes from main routing table (exclude vpns and default gateway)
	[ "$VPN_REDIR" -eq 2 ] && {
		ip route show table main | grep -Ev 'tun11|tun12|tun13|^default ' | while read ROUTE; do
			ip route add table $ID $ROUTE
		done
	}
}

stopRouting() {
	$LOGS "Clean-up routing"

	ip route flush table $ID
	ip route flush cache

	[ "$(ip rule | grep "lookup $ID" | wc -l)" -gt 0 ] && {
		ip rule del fwmark $ID/0xf00 table $ID
	}

# BCMARM-BEGIN
	ipset destroy vpnrouting$ID
# BCMARM-END
# BCMARMNO-BEGIN
	ipset --destroy vpnrouting$ID
# BCMARMNO-END

	sed -i "s/-A/-D/g" $FIREWALL_ROUTING
	$FIREWALL_ROUTING
	rm -f $FIREWALL_ROUTING > /dev/null 2>&1

	sed -i $DNSMASQ_IPSET -e "/vpnrouting$ID/d"
}

startRouting() {
	local DNSMASQ=0 i VAL1 VAL2 VAL3

	stopRouting
	nvram set vpn_client"${ID#??}"_rdnsmasq=0

	$LOGS "Starting routing policy for openvpn-$SERVICE - Interface $IFACE - Table $ID"

	[ -n "$route_vpn_gateway" ] && {
		ip route add table $ID default via $route_vpn_gateway dev $IFACE
	} || {
		ip route add table $ID default dev $IFACE
	}
	ip rule add fwmark $ID/0xf00 table $ID priority 90

	initTable
# BCMARM-BEGIN
	ipset create vpnrouting$ID hash:ip
# BCMARM-END
# BCMARMNO-BEGIN
	ipset --create vpnrouting$ID iphash
# BCMARMNO-END

	echo "#!/bin/sh" > $FIREWALL_ROUTING # new routing file
# BCMARM-BEGIN
	echo "iptables -t mangle -A PREROUTING -m set --match-set vpnrouting$ID dst,src -j MARK --set-mark $ID/0xf00" >> $FIREWALL_ROUTING
# BCMARM-END
# BCMARMNO-BEGIN
	echo "iptables -t mangle -A PREROUTING -m set --set vpnrouting$ID dst,src -j MARK --set-mark $ID/0xf00" >> $FIREWALL_ROUTING
# BCMARMNO-END

	# example of routing_val: 1<2<8.8.8.8<1>1<1<1.2.3.4<0>1<3<domain.com<0> (enabled<type<domain_or_IP<kill_switch>)
	for i in $(echo "$(NV vpn_"$SERVICE"_routing_val)" | tr ">" "\n"); do
		VAL1=$(echo $i | cut -d "<" -f1)
		VAL2=$(echo $i | cut -d "<" -f2)
		VAL3=$(echo $i | cut -d "<" -f3)

		# only if rule is enabled
		[ "$VAL1" -eq 1 ] && {
			case "$VAL2" in
				1)	# from source
					$LOGS "Type: $VAL2 - add $VAL3"
					[ "$(echo $VAL3 | grep -)" ] && { # range
						echo "iptables -t mangle -A PREROUTING -m iprange --src-range $VAL3 -j MARK --set-mark $ID/0xf00" >> $FIREWALL_ROUTING
					} || {
						echo "iptables -t mangle -A PREROUTING -s $VAL3 -j MARK --set-mark $ID/0xf00" >> $FIREWALL_ROUTING
					}
				;;
				2)	# to destination
					$LOGS "Type: $VAL2 - add $VAL3"
					echo "iptables -t mangle -A PREROUTING -d $VAL3 -j MARK --set-mark $ID/0xf00" >> $FIREWALL_ROUTING
				;;
				3)	# to domain
					$LOGS "Type: $VAL2 - add $VAL3"
					echo "ipset=/$VAL3/vpnrouting$ID" >> $DNSMASQ_IPSET
					# try to add ipset rule using forced query to DNS server
					nslookup $VAL3 2>/dev/null

					DNSMASQ=1
				;;
				*) continue ;;
			esac
		}
	done

	chmod 700 $FIREWALL_ROUTING
	RESTART_FW=1

	[ "$DNSMASQ" -eq 1 ] && {
		nvram set vpn_client"${ID#??}"_rdnsmasq=1
		RESTART_DNSMASQ=1
	}

	$LOGS "Completed routing policy configuration for openvpn-$SERVICE"
}

checkRestart() {
	[ "$RESTART_DNSMASQ" -eq 1 -o "$(NV "vpn_client"${ID#??}"_rdnsmasq")" -eq 1 ] && service dnsmasq restart
	[ "$RESTART_FW" -eq 1 ] && service firewall restart
}

checkPid() {
	local PIDNO

	[ -f $PIDFILE ] && {
		PIDNO=$(cat $PIDFILE)
		cat "/proc/$PIDNO/cmdline" > /dev/null 2>&1

		[ $? -eq 0 ] && {
			# priority has the last process
			$LOGS "Killing previous process ..."
			kill -9 $PIDNO
			echo $PID > $PIDFILE

			[ $? -ne 0 ] && {
				$LOGS "Could not create PID file"
				exit 0
			}
		} || {
			# process not found assume not running
			echo $PID > $PIDFILE
			[ $? -ne 0 ] && {
				$LOGS "Could not create PID file"
				exit 0
			}
		}
	} || {
		echo $PID > $PIDFILE
		[ $? -ne 0 ] && {
			$LOGS "Could not create PID file"
			exit 0
		}
	}
}


###################################################


find_iface
checkPid
VPN_REDIR=$(NV vpn_"$SERVICE"_rgw)

[ "$script_type" == "route-up" -a "$VPN_REDIR" -lt 2 ] && {
	$LOGS "Skipping, $SERVICE not in routing policy mode"
	checkRestart
	exit 0
}

[ "$script_type" == "route-pre-down" ] && {
	stopRouting
}

[ "$script_type" == "route-up" ] && {
	startRouting
}

checkRestart

ip route flush cache

rm -f $PIDFILE > /dev/null 2>&1

exit 0
