#!/bin/sh
export PATH=/bin:/usr/bin:/sbin:/usr/sbin:/home/root:
# Network discovery script v2.0 - ARM and MIPSR2 - 05/2024 - rs232
#------------------------------------------------------
#    FreshTomato Network Discovery - usage
#
#    default (or arping) = arping scanning
#    traceroute = uses a traceroute discovery
#------------------------------------------------------

iplist() {
input_cidr="$1"
subnet_mask=$(echo "$input_cidr" | cut -d '/' -f 2)
num_addresses=$((2 ** (32 - subnet_mask)))
ip_address=$(echo "$input_cidr" | cut -d '/' -f 1)
ip_int=0
for octet in $(echo "$ip_address" | tr '.' ' '); do
	ip_int=$((ip_int * 256 + octet))
done
first_ip_int=$((ip_int & ~(num_addresses - 1)))
last_ip_int=$((first_ip_int + num_addresses - 1))
for i in $(seq 1 $((num_addresses - 2))); do
	current_ip_int=$((first_ip_int + i))
	printf "%d.%d.%d.%d\n" $((current_ip_int >> 24 & 255)) $((current_ip_int >> 16 & 255)) $((current_ip_int >> 8 & 255)) $((current_ip_int & 255))
done
}

[ "$(ps w | grep 'traceroute -i\ | arping -q' | grep -v grep | wc -l)" -gt 0 ] && {
	logger "Device List Discovery already running - exiting ..."
	exit
}

for bridge in $(brctl show | grep -Eo ^br[0-9]); do
	cidr=$(ip addr list dev $bridge | grep inet | awk '{print $2}')
	ip=$(echo $cidr | cut -d/ -f1)
	iplist $cidr | while read -r ip_address; do
		if [ -z $1 ] || [ $1 == "arping" ]; then usleep 1500 && arping -q -c1 -w1 -I $bridge $ip_address &>/dev/null &
		elif [ $1 == "traceroute" ]; then traceroute -i $bridge -r -F -m1 -q1 -s $ip $ip_address &>/dev/null &
		fi
	done
done
