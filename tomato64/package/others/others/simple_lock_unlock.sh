#!/bin/sh


PID=$$
FN="/var/lock/firewall.lock"
LOGS="logger -t simple_lock_unlock.sh[$PID]"


simple_lock() {
	local n=$((5 + ($PID % 10)))

	while ! rm -- "$FN" 2>/dev/null; do
		n=$((n - 1))
		if [ "$n" -eq 0 ]; then
			$LOGS "breaking $FN"
			break
		fi
		sleep 1
	done
}

simple_unlock() {
	( umask 066; : > "$FN" )
}
