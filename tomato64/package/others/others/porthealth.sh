#!/bin/sh
# porthealth v3.0 - rs232
MODE=${1:-help}
BASE_RATE=30
HOLD_TIME=180
CACHE_TIMEOUT=1800
IF_TYPE=l
for arg in "$@"; do
	case "$arg" in
		max=*) BASE_RATE=${arg#max=};;
		hold=*) HOLD_TIME=${arg#hold=};;
		cache=*) CACHE_TIMEOUT=${arg#cache=};;
		if=*) IF_TYPE=${arg#if=};;
		[0-9]*) BASE_RATE=$arg;;
	esac
done

case "$MODE" in
	help)
		cat <<'EOF'
Usage: porthealth.sh [mode] [options]

Modes:
  monitor  : logs a warning when threshold is exceeded.

  recover  : a) resets the interface to its maximum capabilities e.g. 1000FD
             b) between each change it re-evaluates (enforcing a hold time);
             c) step down link speed/duplex via robocfg;
             d) disable if still failing at lowest.

  disable  : just log and disable the guilty port.

  help     : (default) show this help message.

Options:
  max=N    : max acceptable errors per port per minute (default 30)
  hold=N   : recovery hold time before scale-down in seconds (default 180)
  cache=N  : port list cache timeout in seconds (default 1800)
  if=TYPE  : monitor interface type - l='LANs (default)', w='WANs', a='LANs & WANs'

Examples:
  porthealth.sh monitor
  porthealth.sh recover max=80 hold=300 if=l
  porthealth.sh disable max=100 if=a

EOF
		exit 0
		;;
esac

STATE_DIR=/tmp/porthealth
COUNTERS=$STATE_DIR/counters
ERROR_SUM=$STATE_DIR/errors.total
RECOVER_STATE=$STATE_DIR/recover.state   # recover state: "ethX:idx:baseline:action_time"
N=$(date +%s)

if [ ! -d "$STATE_DIR" ]; then
	mkdir -p "$STATE_DIR"
fi

log_warn() { logger -p user.warn "porthealth: $*"; }
log_err()  { logger -p user.err  "porthealth: $*"; }

get_mtime() {
	[ ! -f "$1" ] && return 1
	res=$(stat -c %Y "$1" 2>/dev/null)
	if [ -n "$res" ]; then
		echo "$res"
		return 0
	fi
	date -r "$1" +%s 2>/dev/null
}

# robocfg speed steps (high -> low)
# NOTE: tokens must match your robocfg build (common: 1000FD 1000HD 100FD 100HD 10FD 10HD)
STEPS="1000FD 1000HD 100FD 100HD 10FD 10HD"

# Threshold calc (errors per elapsed minutes)
if [ -f "$ERROR_SUM" ]; then
	last_time=$(get_mtime "$ERROR_SUM")
else
	last_time=""
	logger "porthealth: init"
fi
[ -z "$last_time" ] && last_time=$N
elapsed=$((N - last_time))
[ "$elapsed" -lt 1 ] && elapsed=60
R=$((BASE_RATE * elapsed / 60))
[ "$R" -lt 1 ] && R=1

# Helpers to read/write recover state (kept small; only used in recover mode)
state_get() {
	# prints: idx baseline action_time
	awk -F: -v k="$1" 'BEGIN{idx=""} $1==k {print $2" "$3" "$4; exit}' "$RECOVER_STATE" 2>/dev/null
}
state_set() {
	# $1=if $2=idx $3=baseline $4=action_time
	# rewrite file (small N)
	{ awk -F: -v k="$1" '$1!=k {print}' "$RECOVER_STATE" 2>/dev/null; echo "$1:$2:$3:$4"; } >"$RECOVER_STATE.n" && mv "$RECOVER_STATE.n" "$RECOVER_STATE"
}
state_del() {
	{ awk -F: -v k="$1" '$1!=k {print}' "$RECOVER_STATE" 2>/dev/null; } >"$RECOVER_STATE.n" && mv "$RECOVER_STATE.n" "$RECOVER_STATE"
}

# Parse robocfg show to build port list dynamically
build_ports() {
	local vlan_num="$1"
	local robocfg_out

	robocfg_out=$(robocfg show 2>/dev/null)

	# Use single awk pass to:
	# 1. Extract UP ports 0-4
	# 2. Extract VLAN port list
	# 3. Map port to interface
	echo "$robocfg_out" | awk -v vlan="$vlan_num" '
	/^Port [0-4]:/ && !/DOWN/ {
		port = substr($2, 1, length($2)-1)
		up_ports[port] = 1
	}
	$0 ~ "^   " vlan ":" {
		# Extract port list from VLAN line
		line = $0
		sub(/^[^:]*: *vlan[0-9]*: */, "", line)
		sub(/ [0-9]*t.*/, "", line)
		gsub(/[ut]/, "", line)
		ports = line
	}
	END {
		# Map UP ports to interfaces
		n = split(ports, p)
		for (i = 1; i <= n; i++) {
			port = p[i]
			if (port ~ /^[0-4]$/ && port in up_ports) {
				if (port == 0) iface = "eth0"
				else if (port ~ /[1-3]/) iface = "eth1"
				else if (port == 4) iface = "eth2"
				printf "%s:%s ", port, iface
			}
		}
		print ""
	}'
}

# Get cached port list or rebuild if stale (cache TTL configurable via CACHE_TIMEOUT)
get_ports_cached() {
	local if_type="$1"
	local cache_file="$STATE_DIR/ports.$if_type.cache"
	local now=$N cache_time

	if [ -f "$cache_file" ]; then
		cache_time=$(get_mtime "$cache_file")
		if [ $((now - cache_time)) -lt "$CACHE_TIMEOUT" ]; then
			cat "$cache_file"
			return 0
		fi
	fi

	# Cache expired or doesn't exist; rebuild
	local ports
	case "$if_type" in
		w) ports=$(build_ports 2) ;;
		a)
			local lan=$(build_ports 1)
			local wan=$(build_ports 2)
			ports="$lan $wan"
			;;
		l|*) ports=$(build_ports 1) ;;
	esac
	echo "$ports" > "$cache_file"
	echo "$ports"
}

# Determine which ports to monitor (use cache for speed in monitor mode)
PORTS=$(get_ports_cached "$IF_TYPE")

# Build new cache while checking
SUM=0
: >"$COUNTERS.n"
FAULTY=""
MAX_DELTA=0

# Read old counters into memory (single pass instead of awk per port)
OLD_COUNTERS=$(cat "$COUNTERS" 2>/dev/null)

for PORTMAP in $PORTS; do
	PORT=${PORTMAP%:*}
	IFACE=${PORTMAP#*:}
	[ -d "/sys/class/net/$IFACE" ] || continue

	# Read error counters
	read rx_err <"/sys/class/net/$IFACE/statistics/rx_errors" 2>/dev/null || rx_err=0
	read tx_err <"/sys/class/net/$IFACE/statistics/tx_errors" 2>/dev/null || tx_err=0
	read rx_drop <"/sys/class/net/$IFACE/statistics/rx_dropped" 2>/dev/null || rx_drop=0
	read tx_drop <"/sys/class/net/$IFACE/statistics/tx_dropped" 2>/dev/null || tx_drop=0
	read rx_over <"/sys/class/net/$IFACE/statistics/rx_over_errors" 2>/dev/null || rx_over=0
	read frame <"/sys/class/net/$IFACE/statistics/rx_frame_errors" 2>/dev/null || frame=0
	read carrier <"/sys/class/net/$IFACE/statistics/tx_carrier_errors" 2>/dev/null || carrier=0
	read coll <"/sys/class/net/$IFACE/statistics/collisions" 2>/dev/null || coll=0
	X=$((rx_err + tx_err + rx_drop + tx_drop + rx_over + frame + carrier + coll))

	SUM=$((SUM + X))
	echo "port_$PORT:$X" >>"$COUNTERS.n"

	# Get old value from memory instead of awk from file
	O=$(printf '%s\n' "$OLD_COUNTERS" | awk -F: -v p="port_$PORT" '$1==p {print $2; exit}')
	[ -z "$O" ] && O=0

	D=$((X - O))
	# Track the port with highest error delta
	[ "$D" -gt "$MAX_DELTA" ] && { MAX_DELTA=$D; FAULTY=$PORT; }
done

# Check if any port exceeded threshold
if [ -z "$FAULTY" ] || [ "$MAX_DELTA" -le "$R" ]; then
	mv "$COUNTERS.n" "$COUNTERS"
	printf '%s\n' "$SUM" > "$ERROR_SUM"
	exit 0
fi

# Flood detected on port
case "$MODE" in
	monitor)
		log_warn "FLOOD port $FAULTY (delta=$MAX_DELTA thr=$R)";
		;;
	disable)
		log_err "FLOOD port $FAULTY (disable)";
		robocfg port "$FAULTY" state disable >/dev/null 2>&1
		;;
	recover)
		set -- $(state_get "port_$FAULTY")
		idx="$1"
		if [ -z "$idx" ]; then
			idx=1
			step=$(echo "$STEPS" | awk -v n="$idx" '{print $n}')
			robocfg port "$FAULTY" speed "$step" >/dev/null 2>&1
			state_set "port_$FAULTY" "$idx" "$MAX_DELTA" "$N"
			log_warn "RECOVER START port $FAULTY -> $step"
		else
			action_time=$(state_get "port_$FAULTY" | awk '{print $3}')
			elapsed=$((N - action_time))
			if [ "$elapsed" -ge "$HOLD_TIME" ]; then
				R_eval=$((BASE_RATE * elapsed / 60))
				[ "$R_eval" -lt 1 ] && R_eval=1
				if [ "$MAX_DELTA" -le "$R_eval" ]; then
					log_warn "RECOVER OK port $FAULTY"
					state_del "port_$FAULTY"
				else
					idx=$((idx+1))
					step=$(echo "$STEPS" | awk -v n="$idx" '{print $n}')
					if [ -z "$step" ]; then
						log_err "RECOVER FAIL port $FAULTY (disable)"
						robocfg port "$FAULTY" state disable >/dev/null 2>&1
						state_del "port_$FAULTY"
					else
						robocfg port "$FAULTY" speed "$step" >/dev/null 2>&1
						state_set "port_$FAULTY" "$idx" "$MAX_DELTA" "$N"
						log_warn "RECOVER STEP port $FAULTY -> $step"
					fi
				fi
			fi
		fi
		;;
	*)
		log_warn "Unknown mode '$MODE' (using monitor)";
		log_warn "FLOOD port $FAULTY (delta=$MAX_DELTA thr=$R)";
		;;
esac

mv "$COUNTERS.n" "$COUNTERS"
printf '%s\n' "$SUM" > "$ERROR_SUM"
