#!/bin/sh
export PATH=/bin:/usr/bin:/sbin:/usr/sbin:/home/root
# Network discovery script v2.64 - ARM and MIPSR2 - 02/2025 - rs232
#------------------------------------------------------
#	FreshTomato Network Discovery - usage
#
#	arping = * scanning via arping (preferred)
#	traceroute = scanning via traceroute
#	nc = scanning via netcat
#	all = all the above in round-robin
#
#	lan = * only LANs interfaces
#	wan = only WANs interfaces
#	both = scan LANs and WANs
#
#	clear = removes dirty records from the arp table post scan
#
#	<5-200> = default 60, maximum number of concurrent scans
#------------------------------------------------------
script_name=$(basename "$0")
pid_file="/tmp/var/run/${script_name}.pid"

cleanup() {
	rm -f "$pid_file"
	exit 0
}

trap cleanup INT TERM EXIT

if [ -e "$pid_file" ]; then
	pid_age=$(($(date +%s) - $(stat -c %Y "$pid_file")))
	if [ "$pid_age" -gt 120 ]; then
		echo "PID file is older than 2 minutes. Removing stale PID file."
		rm -f "$pid_file"
	fi
fi

debug=$(nvram get discovery_debug 2>/dev/null)
debug="${debug:-0}"
[ "$debug" -eq 1 ] && { echo "Debugging..."; date >> /tmp/discovery.debug; echo "$@" >> /tmp/discovery.debug; }
lim=$(echo "$@" | grep -oE '[0-9]+' | head -n 1 | awk '{if($1 < 5) print 5; else if($1 > 200) print 200; else print $1}')
[ -z "$lim" ] && lim=60
alias slp='usleep 250000'
alias low='nice -n 19'
tmp="/tmp/discovery.tmp"
scan=0
cl=0
runlan=0
runwan=0
fkill=0

iplist() {
	input_cidr="$1"
	cidr=$(echo "$input_cidr" | cut -d '/' -f 2)
	ip=$(echo "$input_cidr" | cut -d '/' -f 1)
	awk -v cidr="$cidr" -v ip="$ip" '
	BEGIN {
		split(ip, a, ".");
		ip_int = a[1] * 256^3 + a[2] * 256^2 + a[3] * 256 + a[4];
		num_addrs = 2^(32 - cidr);
		base_ip = int(ip_int / num_addrs) * num_addrs;
		for (i = 1; i < num_addrs - 1; i++) {
		current_ip = base_ip + i;
		printf "%d.%d.%d.%d\n",
		int(current_ip / 256^3) % 256,
		int(current_ip / 256^2) % 256,
		int(current_ip / 256) % 256,
		current_ip % 256;
		}
	}'
}

check_procs() {
	case "$1" in
		arping) pidof arping ;;
		traceroute) pidof traceroute ;;
		nc) pidof nc ;;
	esac | wc -w
}

md5_param=$(echo "$@" | md5sum | cut -d' ' -f1)
if [ -f "$tmp" ]; then
	if ! grep -q "$md5_param" "$tmp"; then
		# Kill previous instances with different parameters
		for PID in $(pidof "$script_name"); do
			[ "$PID" != "$$" ] && kill -9 "$PID" &>/dev/null
		done
		# Kill scanning tools
		for tool in arping traceroute nc; do
			[ "$(check_procs "$tool")" -gt 0 ] && killall -q "$tool"
		done
		sleep 1
		fkill=1
	fi
else
	echo 0 > "$tmp"
	echo "$md5_param" >> "$tmp"
fi

for param in "$@"; do
	case "$param" in
		clear)		cl=1 ;;
		lan)		runlan=1 ;;
		wan)		runlan=0; runwan=1 ;;
		both)		runlan=1; runwan=1 ;;
		traceroute)	scan=1 ;;
		nc)			scan=2 ;;
		all)		scan=$(head -1 "$tmp") ;;
	esac
done
[ "$runlan" -eq 0 ] && [ "$runwan" -eq 0 ] && runlan=1

if [ -e "$pid_file" -a "$fkill" -ne 1 ]; then
	echo "Status-devices - Background discovery processes already running; skipping this run." | tee /dev/tty | logger
	cleanup
fi

scanthis() {
	for iface in $this; do
		cidr=$(ip a l dev "$iface" | grep inet | awk '{print $2}')
		[ "$(echo "$cidr" | cut -d/ -f2)" -lt 22 ] && {
			echo "$iface - subnet mask shorter than /22. Too many IPs to scan - skipping..." | tee /dev/tty | logger
			continue
		}
		[ "$(echo "$cidr" | cut -d/ -f2)" -lt 32 ] && {
			ip=$(echo "$cidr" | cut -d/ -f1)
			ipn=$(ip neigh show dev "$iface" | grep 'REACHABLE\|PERMANENT' | grep -Eo '[0-9]{1,3}\.[0-9]{1,3}\.[0-9]{1,3}\.[0-9]{1,3}')
			iplist "$cidr" | grep -vwE "$ip|$(echo "$ipn" | tr ' ' '\n')" | while read -r ip_addr; do
				if [ "$scan" -eq 0 ]; then
					while [ "$(check_procs arping)" -gt "$lim" ]; do slp; done
					low arping -I "$iface" -q -c1 -w1 "$ip_addr" &>/dev/null &
				elif [ "$scan" -eq 1 ]; then
					while [ "$(check_procs traceroute)" -gt "$lim" ]; do slp; done
					low traceroute -i "$iface" -r -F -m1 -q1 -s "$ip" "$ip_addr" &>/dev/null &
				elif [ "$scan" -eq 2 ]; then
					while [ "$(check_procs nc)" -gt "$lim" ]; do slp; done
					low nc -w 1 "$ip_addr" &>/dev/null &
				fi
			done
			wait
		}
	done
}

prune() {
	ip n | grep -w "$1" | while read -r badip; do
		ip=$(echo "$badip" | awk '{print $1}')
		dev=$(echo "$badip" | awk '{print $3}')
		ip neigh change "$ip" nud none dev "$dev"
	done
}

echo $$ > "$pid_file"

[ "$runlan" -eq 1 ] && {
	this=$(brctl show | grep -Eo ^br[0-9])
	scanthis
}

unset wans
[ "$runwan" -eq 1 ] && {
	[ "$(ip rule | grep -Eo 'WAN[1-4]' | sort -u | wc -l)" -gt 0 ] && {
		for WAN in $(ip rule | grep -Eo 'WAN[1-4]' | sort -u); do
		wans="$wans $(ip r l t "$WAN" | grep default | grep -Eo '[vlan|eth]+[0-9]{1,4}')"
	done
	} || {
		wans=$(ip route | grep ^default | awk '{print $5}')
	}
	this="$wans"
	scanthis
}

[ "$cl" -eq 1 ] && {
	prune "FAILED\|INCOMPLETE"
	usleep $((lim * 30000))
	prune "STALE\|DELAY\|PROBE"
}

scan=$(((${scan:-0} + 1) % 3))
{
	echo "$scan"
	echo "$md5_param"
} >> "$tmp"

cleanup