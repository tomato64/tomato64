#!/bin/sh
. /usr/share/libubox/jshn.sh

if [ ! -f /etc/board.json ]; then
	echo "'phy_count': '0'"
	exit 0
fi

json_load_file /etc/board.json  ## Load JSON from file

device=""
band=""
count=0

modes(){
	echo "'${device}_${band}_${1}': '1',"
}

bands(){

	band=${2}
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

dump_item() {

	count=$((count+1))
	device=${2}
	json_select ${device}
	json_get_var var path
	echo "'${device}_path': '${var}',"

	json_select info
	json_get_var var antenna_rx
	echo "'${device}_rx': '${var}',"

	json_get_var var antenna_tx
	echo "'${device}_tx': '${var}',"

	json_for_each_item "bands" "bands"

	json_select ..
	json_select ..
}

json_for_each_item "dump_item" "wlan"
echo "'phy_count': '${count}'"
