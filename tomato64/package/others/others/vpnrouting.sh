#!/bin/sh
export PATH=/bin:/usr/bin:/sbin:/usr/sbin:/home/root
#
# VPN Client selective routing up down script
#
# Copyright by pedro 2019 - 2025
#


. nvram_ops

PID=$$
IFACE=$dev
SERVICE=$(echo $dev | sed 's/\(tun\|tap\)1/client/;s/\(tun\|tap\)2/server/')
FIREWALL_ROUTING="/etc/openvpn/fw/$SERVICE-fw-routing.sh"
DNSMASQ_IPSET="/etc/dnsmasq.ipset"
RESTART_DNSMASQ=0
RESTART_FW=0
FWMARK="0"
CID="${dev:4:1}"
ENV_VARS="/tmp/env_vars_${CID}"
LOGS="logger -t openvpn-vpnrouting.sh[$PID][$IFACE]"
[ -d /etc/openvpn/fw ] || mkdir -m 0700 "/etc/openvpn/fw"


# utility function for retrieving environment variable values
env_get() {
	echo $(grep -Em1 "^$1=" $ENV_VARS | cut -d = -f2)
}

find_iface() {
	# These FWMARKs were intentionally picked to avoid overwriting
	# marks set by QoS. See qos.c
	if [ "$SERVICE" == "client1" ]; then
		FWMARK="2304" # 0x900
	elif [ "$SERVICE" == "client2" ]; then
		FWMARK="2560" # 0xA00
# BCMARM-BEGIN
	elif [ "$SERVICE" == "client3" ]; then
		FWMARK="2816" # 0xB00
# BCMARM-END
	else
		$LOGS "Interface not found!"
		exit 0
	fi

	PIDFILE="/var/run/vpnrouting$FWMARK.pid"
}

stopRouting() {
	$LOGS "Clean-up routing"

	ip route flush table $FWMARK
	ip route flush cache
	ip rule del fwmark $FWMARK/0xf00 table $FWMARK

	[ -f "$FIREWALL_ROUTING" ] && {
		sed -i -e "s/-I/-D/g; s/-A/-D/g" "$FIREWALL_ROUTING" &>/dev/null
		$FIREWALL_ROUTING &>/dev/null
		rm -f $FIREWALL_ROUTING &>/dev/null
	}
# BCMARM-BEGIN
	ipset destroy vpnrouting$FWMARK &>/dev/null
# BCMARM-END
# BCMARMNO-BEGIN
	ipset --destroy vpnrouting$FWMARK &>/dev/null
# BCMARMNO-END
	[ -f "$DNSMASQ_IPSET" ] && {
		if grep -q "/vpnrouting$FWMARK" "$DNSMASQ_IPSET"; then
			sed -i "$DNSMASQ_IPSET" -e "/vpnrouting$FWMARK/d" &>/dev/null
			# ipset was used on this client so dnsmasq restart is needed
			RESTART_DNSMASQ=1
		fi
	}
}

startRouting() {
	local i VAL1 VAL2 VAL3 ROUTE

	stopRouting

	$LOGS "Starting routing policy for openvpn-$SERVICE - Interface $IFACE - Table $FWMARK - Mode $VPN_REDIR"

	# strict - copy routes from main routing table only for this interface
	[ "$VPN_REDIR" -eq 3 ] && {
		ip route show table main dev $IFACE | while read ROUTE; do
			ip route add table $FWMARK $ROUTE dev $IFACE
		done
	}
	# standard - copy routes from main routing table (exclude vpns and all default gateways)
	[ "$VPN_REDIR" -eq 2 ] && {
		ip route show table main | grep -Ev 'wg0|wg1|wg2|tun11|tun12|tun13|^default |^0.0.0.0/1 |^128.0.0.0/1 ' | while read ROUTE; do
			ip route add table $FWMARK $ROUTE
		done
	}

	# test for presence of vpn gateway override in main routing table
	if ip route | grep -q "^0\.0\.0\.0/1 .*$(env_get dev)"; then
		# add WAN as default gateway to alternate routing table
		ip route add default via $(env_get route_net_gateway) table $FWMARK dev $IFACE
	else
		# add VPN as default gateway to alternate routing table
		ip route add default via $(env_get route_vpn_gateway) table $FWMARK dev $IFACE
	fi

	ip rule add fwmark $FWMARK/0xf00 table $FWMARK priority 90

# BCMARM-BEGIN
	ipset create vpnrouting$FWMARK hash:ip
# BCMARM-END
# BCMARMNO-BEGIN
	ipset --create vpnrouting$FWMARK iphash
# BCMARMNO-END

	echo "#!/bin/sh" > $FIREWALL_ROUTING # new routing file
# BCMARM-BEGIN
	echo "iptables -t mangle -A PREROUTING -m set --match-set vpnrouting$FWMARK dst,src -j MARK --set-mark $FWMARK/0xf00" >> $FIREWALL_ROUTING
# BCMARM-END
# BCMARMNO-BEGIN
	echo "iptables -t mangle -A PREROUTING -m set --set vpnrouting$FWMARK dst,src -j MARK --set-mark $FWMARK/0xf00" >> $FIREWALL_ROUTING
# BCMARMNO-END

	# example of routing_val: 1<2<8.8.8.8<1>1<1<1.2.3.4<0>1<3<domain.com<0> (enabled<type<domain_or_IP<kill_switch>)
	for i in $(echo "$(NG vpn_"$SERVICE"_routing_val)" | tr ">" "\n"); do
		VAL1=$(echo $i | cut -d "<" -f1)
		VAL2=$(echo $i | cut -d "<" -f2)
		VAL3=$(echo $i | cut -d "<" -f3)

		# only if rule is enabled
		[ "$VAL1" -eq 1 ] && {
			case "$VAL2" in
				1)	# from source
					$LOGS "Type: $VAL2 - add $VAL3"
					[ "$(echo $VAL3 | grep -)" ] && { # range
						echo "iptables -t mangle -A PREROUTING -m iprange --src-range $VAL3 -j MARK --set-mark $FWMARK/0xf00" >> $FIREWALL_ROUTING
					} || {
						echo "iptables -t mangle -A PREROUTING -s $VAL3 -j MARK --set-mark $FWMARK/0xf00" >> $FIREWALL_ROUTING
					}
				;;
				2)	# to destination
					$LOGS "Type: $VAL2 - add $VAL3"
					echo "iptables -t mangle -A PREROUTING -d $VAL3 -j MARK --set-mark $FWMARK/0xf00" >> $FIREWALL_ROUTING
				;;
				3)	# to domain
					$LOGS "Type: $VAL2 - add $VAL3"
					echo "ipset=/$VAL3/vpnrouting$FWMARK" >> $DNSMASQ_IPSET # add

					RESTART_DNSMASQ=1
				;;
				*) continue ;;
			esac
		}
	done

	chmod 700 $FIREWALL_ROUTING
	RESTART_FW=1

	$LOGS "Completed routing policy configuration for openvpn-$SERVICE"
}

checkRestart() {
	[ "$RESTART_DNSMASQ" -eq 1 ] && service dnsmasq restart
	[ "$RESTART_FW" -eq 1 ] && service firewall restart
}

checkPid() {
	local PIDNO

	[ -f $PIDFILE ] && {
		PIDNO=$(cat $PIDFILE)
		cat "/proc/$PIDNO/cmdline" &>/dev/null

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
VPN_REDIR=$(NG vpn_"$SERVICE"_rgw)

[ "$script_type" == "route-up" -a "$VPN_REDIR" -lt 2 ] && {
	$LOGS "Skipping, $SERVICE not in routing policy mode"
	exit 0
}

[ "$script_type" == "route-pre-down" ] && {
	stopRouting
	checkRestart
	rm -f $ENV_VARS
}

[ "$script_type" == "route-up" ] && {
	# make environment variables persistent across openvpn events
	env > $ENV_VARS
	startRouting
	checkRestart
}

ip route flush cache

rm -f $PIDFILE &>/dev/null

exit 0
