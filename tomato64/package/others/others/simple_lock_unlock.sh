#!/bin/sh


PID=$$
FN="/var/lock/firewall.lock"
LOGW="logger -p WARN -t simple_lock_unlock.sh[$PID]"
TIMEOUT=8


simple_lock() {
	local n=$(($TIMEOUT + ($PID % 10)))

	while ! rm -- "$FN" 2>/dev/null; do
		n=$((n - 1))
		if [ "$n" -eq 0 ]; then
			$LOGW "breaking $FN"
			break
		fi
		sleep 1
	done
}

simple_unlock() {
	( umask 066; : > "$FN" )
}
