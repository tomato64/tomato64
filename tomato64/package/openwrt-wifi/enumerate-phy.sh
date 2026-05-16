#!/bin/sh
. /usr/share/libubox/jshn.sh
. /usr/bin/phy-map.sh

if [ ! -f /etc/board.json ]; then
	echo "'phy_count': '0'"
	exit 0
fi

json_load_file /etc/board.json  ## Load JSON from file

device=""
band=""
var=""
count=0

modes(){
	echo "'${device}_${band}_${1}': '1',"
}

bands(){

	band=${1}
	json_select ${band}
	echo "'${device}_${band}': '1',"

	json_get_var var ht
	if [ "${var}" == "1" ]; then
		echo "'${device}_${band}_ht': '1',"
	fi

	json_get_var var vht
	if [ "${var}" == "1" ]; then
		echo "'${device}_${band}_vht': '1',"
	fi

	json_get_var var he
	if [ "${var}" == "1" ]; then
		echo "'${device}_${band}_he': '1',"
	fi

	json_get_var var eht
	if [ "${var}" == "1" ]; then
		echo "'${device}_${band}_eht': '1',"
	fi

	json_get_var var max_width
	echo "'${device}_${band}_max_width': '$var',"

	json_for_each_item "modes" "modes"

	json_get_var var default_channel
	echo "'${device}_${band}_default_channel': '${var}',"
	json_select ..
}

# Walk logical phys (flattens single-wiphy multi-radio parts; see phy-map.sh).
# Each logical phy maps the UI's phyN onto an underlying board.json phy plus the
# band(s) it owns. Capability flags always come from info.bands.<BAND>; the
# radio entry only selects which band(s) a logical phy exposes.
emit_logical_phys > /tmp/.enum_phy_map

while IFS='|' read -r lphy phy_key ridx phy_bands path; do
	[ -z "$lphy" ] && continue

	count=$((count+1))
	device="phy${lphy}"
	echo "'${device}_path': '${path}',"
	# Real kernel wiphy name (board.json key == /sys/class/ieee80211/<name>). On
	# single-wiphy multi-radio parts several logical phys share one wiphy, so the
	# web UI must query iwinfo against this name, not phy<logical-index>.
	echo "'${device}_phyname': '${phy_key}',"

	json_select wlan
	json_select "${phy_key}"
	json_select info

	json_get_var var antenna_rx
	echo "'${device}_rx': '${var}',"

	json_get_var var antenna_tx
	echo "'${device}_tx': '${var}',"

	json_select bands
	for b in $phy_bands; do
		bands "$b"
	done
	json_select ..   ## back to info

	json_select ..   ## back to phy
	json_select ..   ## back to wlan
	json_select ..   ## back to root
done < /tmp/.enum_phy_map

rm -f /tmp/.enum_phy_map
echo "'phy_count': '${count}'"
