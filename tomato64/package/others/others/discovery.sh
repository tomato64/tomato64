#!/bin/sh
export PATH=/bin:/usr/bin:/sbin:/usr/sbin:/home/root:
# Network discovery script v1.7 - ARM and MIPS R2 - 06/2021 - rs232
#------------------------------------------------------
#    FreshTomato Network Discovery - usage
#
#    default (or arping) = arping scanning
#    traceroute = uses a traceroute discovery
#------------------------------------------------------

[ "$(ps w | grep 'traceroute -i\ | arping -q' | grep -v grep | wc -l)" -gt 0 ] && {
	logger "Device List Discovery already running - exiting ..."
	exit
}

for bridge in $(brctl show | grep -Eo ^br[0-9]); do
	NETWORK=$(ifconfig $bridge | grep inet | awk '{print $2}' | grep -Eo "(([0-9]|[1-9][0-9]|1[0-9]{2}|2[0-4][0-9]|25[0-5])\.){3})*")
	ip=$(ifconfig $bridge | grep inet | awk '{print $2}' | cut -d: -f2)
	mask=$(ifconfig $bridge | grep inet | awk '{print $4}' | cut -f2 -d":" | head -1)
	i4=$(echo $ip | awk -F"." '{print $4}')
	m4=$(echo $mask | awk -F"." '{print $4}')
	NETSTART=$(((i4 & m4)+1))
	NETEND=$(((i4 & m4 | 255-m4)-1))
	while [ "$NETSTART" -le "$NETEND" ]; do
		HOST=$NETWORK$NETSTART
		if [ -z $1 ] || [ $1 == "arping" ]; then usleep 1500 && arping -q -c1 -w1 -I $bridge $HOST &>/dev/null &
		elif [ $1 == "traceroute" ]; then traceroute -i $bridge -r -F -m1 -q1 -s $ip $HOST &>/dev/null &
		fi
		NETSTART=$(($NETSTART+1))
	done
done
