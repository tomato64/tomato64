#!/bin/sh
#
# phy-map.sh - shared logical-PHY enumeration for /etc/board.json
#
# Sourced library (not executable on its own). The caller must have already
# sourced jshn.sh and run "json_load_file /etc/board.json".
#
# Background: a "logical phy" is what the Tomato64 UI and NVRAM call phyN. Older
# WiFi devices expose one top-level board.json "phyN" key per radio and leave
# "info.radios" empty. Newer single-wiphy parts (e.g. GL-MT3600BE) expose a
# single board phy whose individual radios live in an "info.radios" array. This
# helper flattens both layouts into one ordered, contiguous numbering, mirroring
# how OpenWrt's mac80211.uc assigns wifi-device indices:
#
#   - board phy with a non-empty "radios" array -> one logical phy per radio
#   - board phy with an empty/absent "radios" array -> a single logical phy
#
# emit_logical_phys() prints, in order, one line per logical phy:
#
#   <lphy_index>|<phy_key>|<radio_index|->|<bands>|<path>
#
# where <radio_index> is the radio's "index" field for multi-radio parts or "-"
# for whole-phy parts, <bands> is a space-separated band list (e.g. "2G" or
# "2G 5G"), and <phy_key>/<path> identify the underlying board.json phy.

_LPHY_IDX=0
_LPHY_PHY=""
_LPHY_PATH=""
_LPHY_HADRADIO=0

_lphy_radio() {
	# Called per element of the radios[] array; $2 is the 1-based array position.
	json_select "$2"
	json_get_var _r_index index
	json_get_keys _r_bands bands
	json_select ..

	_LPHY_HADRADIO=1
	echo "${_LPHY_IDX}|${_LPHY_PHY}|${_r_index}|${_r_bands}|${_LPHY_PATH}"
	_LPHY_IDX=$((_LPHY_IDX + 1))
}

_lphy_phy() {
	# Called per top-level wlan entry; $2 is the board phy key (phy0, phy1, ...).
	_LPHY_PHY="$2"
	json_select "$2"
	json_get_var _LPHY_PATH path

	json_select info
	_LPHY_HADRADIO=0
	json_get_type _r_type radios
	[ "$_r_type" = "array" ] && json_for_each_item _lphy_radio radios

	if [ "$_LPHY_HADRADIO" = "0" ]; then
		json_get_keys _p_bands bands
		echo "${_LPHY_IDX}|${_LPHY_PHY}|-|${_p_bands}|${_LPHY_PATH}"
		_LPHY_IDX=$((_LPHY_IDX + 1))
	fi

	json_select ..
	json_select ..
}

emit_logical_phys() {
	_LPHY_IDX=0
	# Bail out when board.json has no "wlan" container (e.g. an empty "{}" on
	# WiFi-less devices). jshn's json_for_each_item would otherwise fall into its
	# default branch and invoke the callback once with an empty key, fabricating a
	# phantom logical phy.
	json_get_type _w_type wlan
	[ "$_w_type" = "object" ] || [ "$_w_type" = "array" ] || return 0
	json_for_each_item _lphy_phy wlan
}
