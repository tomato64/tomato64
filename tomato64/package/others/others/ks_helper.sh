#!/bin/sh
export PATH=/bin:/usr/bin:/sbin:/usr/sbin:/home/root
#
# Kill Switch FQDNs (type 3) helper script
#
# Copyright by pedro 2025 - 2026
# https://freshtomato.org/
#


. nvram_ops
. simple_lock_unlock.sh

PID=$$
MODE="$1"
FILE="/tmp/kill-switch/fqdns"
PIDFILE="/var/run/ks_helper.pid"
LOGI="logger -t KS-helper[$PID]"
LOGE="logger -p ERROR -t KS-helper[$PID]"
LOGW="logger -p WARN -t KS-helper[$PID]"
LOGD="logger -p DEBUG -t KS-helper[$PID]"
PRIORITY=0
ALLOK=1
IP="$(NG wan_checker)"


[ -z "$1" ] && { echo "usage: $0 update | add"; exit 0; }


ipv6Enabled() {
	local names="native native-pd 6to4 sit other 6rd 6rd-pd"
	local p=$(NG ipv6_service 2>/dev/null) i=1

	for name in $names; do
		[ "$p" = "$name" ] && { echo "$i"; return; }
		i=$((i + 1))
	done

	echo 0
}

pingOK() {
	local i=1 REPEAT

	[ "$PRIORITY" -eq 1 ] && REPEAT=300 || REPEAT=30

	while :; do
		ping -c 1 -A -W 1 -q $IP &>/dev/null && { echo 1; return; }
		[ $((i++)) -ge $REPEAT ] && break || sleep 5
	done

	echo 0
}

addToFW() {
	local DOMAIN VPN_IF WAN_IF FLAG RESULT IP ipt ADD1 ADD2 ADDED SUCCESS RETRIES

	while IFS= read -r line; do
		# omit comments and blank lines
		[ -z "$line" ] || [ "${line#\#}" != "$line" ] && continue

		DOMAIN=$(echo "$line" | awk '{print $1}')
		VPN_IF=$(echo "$line" | awk '{print $2}')
		WAN_IF=$(echo "$line" | awk '{print $3}')
		FLAG=$(echo "$line" | awk '{print $4}')

		# for add mode skip the domain if FLAG=0
		[ "$PRIORITY" -eq 1 ] && [ "$FLAG" -eq 0 ] && continue

		# resolve
		RETRIES=3
		SUCCESS=0
		for i in $(seq 1 $RETRIES); do
			RESULT=$(nslookup "$DOMAIN" 2>&1)
			if [ $? -eq 0 ] && ! echo "$RESULT" | grep -q "can't resolve\|connection timed out\|server can't find\|Can't find"; then
				SUCCESS=1
				break
			fi
			usleep 200000
		done

		# add info about problem
		if [ "$SUCCESS" -ne 1 ]; then
			$LOGE "type: 3 - can't resolve '$DOMAIN' after $RETRIES attempts: $RESULT (are all WANs down, or did you enter the wrong domain?)"
			ALLOK=0
			continue
		fi

		# update FLAG to 0 for successfully resolved domain (add mode only)
		if [ "$PRIORITY" -eq 1 ]; then
			sed -i "s/^\($DOMAIN[[:space:]]\+$VPN_IF[[:space:]]\+$WAN_IF[[:space:]]\+\)[[:digit:]]\+$/\10/" "$FILE"
		fi

		# add IP(s) for given domain
		ADDGLOB=0
		echo "$RESULT" | awk -v domain="$DOMAIN" '/^Name:/ { if ($2 == domain) {flag=1; next} } flag && /Address [0-9]+:/ { print $3 }' | \
		while IFS= read -r IP; do
			if echo "$IP" | grep -qv ':'; then
				ipt="iptables"
			else
				[ "$(ipv6Enabled)" -eq 1 ] && ipt=ip6tables || continue
			fi
			ADD1=0
			ADD2=0

			! $ipt -C FORWARD -d "$IP" ! -o "$VPN_IF" -j REJECT 2>/dev/null && ADD1=1
			! $ipt -C FORWARD -d "$IP" -o "$WAN_IF" -j REJECT 2>/dev/null && ADD2=1

			[ "$ADD1$ADD2" != "00" ] && {
				ADDED=1

				# keep the lock as short as possible (in add mode)
				[ "$PRIORITY" -eq 1 ] && simple_lock

				[ "$ADD1" -eq 1 ] && $ipt -I FORWARD -d "$IP" ! -o "$VPN_IF" -j REJECT
				[ "$ADD2" -eq 1 ] && $ipt -I FORWARD -d "$IP" -o "$WAN_IF" -j REJECT

				[ "$PRIORITY" -eq 1 ] && simple_unlock
			}
		done
		[ "$ADDED" -eq 1 ] && $LOGI "type: 3 - add '$DOMAIN'"

	done < "$FILE"
}

checkPid() {
	local PIDNO

	if [ -f "$PIDFILE" ]; then
		PIDNO=$(cat "$PIDFILE")
		if cat "/proc/$PIDNO/cmdline" &>/dev/null; then
			if [ "$(PRIORITY)" -eq 1 ]; then
				# our process has priority (add mode)
				$LOGW "killing previous process ..."
				kill -9 "$PIDNO"
			else
				# priority has the previous process (std mode)
				$LOGW "another process in action - Exiting ..."
				exit 0
			fi
		fi
	fi

	if ! echo "$PID" >"$PIDFILE"; then
		$LOGE "could not create PID file"
		exit 0
	fi
}


###################################################


# set PRIORITY for checkPid
if [ "$MODE" = "add" ]; then
	PRIORITY=1
elif [ "$MODE" = "update" ]; then
	PRIORITY=0
else
	exit 1
fi

checkPid

if [ "$MODE" = "add" ]; then
	# wait for FW to finish (in add mode)
	sleep 5
	$LOGI "trying to resolve FQDNs for FW ..."
elif [ "$MODE" = "update" ]; then
	$LOGD "perform scheduled FQDN checks ..."
fi

# in add mode, retry until all domains are resolved
while :; do
	# check connection
	[ "$(pingOK)" -eq 1 ] && {
		ALLOK=1
		addToFW
		if [ "$MODE" = "add" ] && [ "$ALLOK" != "1" ]; then
			$LOGE "not all domain IPs have been resolved and added to FW! Will retry ASAP ..."
			continue
		fi
	} || {
		$LOGE "could not $MODE IPs of FQDNs for FW ($MODE mode) - connection is down! Will retry later ..."
		[ "$MODE" = "add" ] && { sleep 10; continue; }
	}
	break
done

rm -f $PIDFILE &>/dev/null

exit 0
