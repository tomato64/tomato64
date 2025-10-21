#!/bin/sh

. /usr/sbin/nvram_ops
. /usr/share/libubox/jshn.sh

# Script mode: "reload" or "start" (default)
MODE="${1:-start}"

# Wait for wlconfig only on initial start (not reload)
if [ "$MODE" = "start" ]; then
	timeout=40  # 20 seconds (40 * 0.5s)
	while [ $timeout -gt 0 ] && [ ! -f /tmp/.wlconfig_done ]; do
		sleep 0.5
		timeout=$((timeout - 1))
	done

	if [ ! -f /tmp/.wlconfig_done ]; then
		logger -p user.error "wlconfig did not detect all expected WiFi PHYs within timeout, WiFi may not start correctly"
	fi
fi

mkdir -p /etc/config
touch /etc/config/network

rm -f /etc/config/wireless
touch /etc/config/wireless

json_load_file /etc/board.json

phycount=0
start_hostapd=0
start_wpa_supplicant=0
client_ifaces=""
error=0

count_phy() {
        phycount=$((phycount+1))
}

print_phy_path() {
	json_select wlan
	json_select phy${1}
	json_get_var var path
	uci set "wireless.radio${1}.path=${var}"
	json_select ..
	json_select ..
}

print_htmode() {

	case $(NG "wifi_phy${1}_mode") in
		n)
			case $(NG "wifi_phy${1}_width") in
				20)
					uci set "wireless.radio${1}.htmode=HT20"
				;;
				40)
					uci set "wireless.radio${1}.htmode=HT40"
				;;
			esac
		;;
		ac)
			case $(NG "wifi_phy${1}_width") in
				20)
					uci set "wireless.radio${1}.htmode=VHT20"
				;;
				40)
					uci set "wireless.radio${1}.htmode=VHT40"
				;;
				80)
					uci set "wireless.radio${1}.htmode=VHT80"
				;;
				160)
					uci set "wireless.radio${1}.htmode=VHT160"
				;;
			esac
		;;
		ax)
			case $(NG "wifi_phy${1}_width") in
				20)
					uci set "wireless.radio${1}.htmode=HE20"
				;;
				40)
					uci set "wireless.radio${1}.htmode=HE40"
				;;
				80)
					uci set "wireless.radio${1}.htmode=HE80"
				;;
				160)
					uci set "wireless.radio${1}.htmode=HE160"
				;;
			esac
		;;
		be)
			case $(NG "wifi_phy${1}_width") in
				20)
					uci set "wireless.radio${1}.htmode=EHT20"
				;;
				40)
					uci set "wireless.radio${1}.htmode=EHT40"
				;;
				80)
					uci set "wireless.radio${1}.htmode=EHT80"
				;;
				160)
					uci set "wireless.radio${1}.htmode=EHT160"
				;;
				320)
					uci set "wireless.radio${1}.htmode=EHT320"
				;;
			esac
		;;
	esac
}

print_legacy_rates() {

	if [ "$(NG "wifi_phy${1}_band")" == "2g" ];
	then
		uci set "wireless.radio${1}.legacy_rates=$(NG wifi_phy${i}_brates)"
	fi

}

print_wmm() {

	if [ $(NG wifi_phy${1}iface${2}_wmm) -eq 0 ];
	then
		uci set "wireless.phy${1}iface${2}.wmm=0"
		uci set "wireless.phy${1}iface${2}.uapsd=0"
	elif [ $(NG wifi_phy${1}iface${2}_uapsd) -eq 0 ];
	then
		uci set "wireless.phy${1}iface${2}.uapsd=0"
	fi
}

print_encryption() {

	encryption=$(NG wifi_phy${1}iface${2}_encryption)

	if [ "${encryption}" == "psk2" ] || [ "${encryption}" == "psk-mixed" ] || [ "${encryption}" == "psk" ];
	then
		if [ "$(NG wifi_phy${1}iface${2}_cipher)" != "auto" ];
		then
			encryption="${encryption}+$(NG wifi_phy${1}iface${2}_cipher)"
		fi
	fi

	uci set "wireless.phy${1}iface${2}.encryption=${encryption}"

	if [ "${encryption}" != "owe" ] && [ "${encryption}" != "none" ];
	then
		uci set "wireless.phy${1}iface${2}.key=$(NG wifi_phy${1}iface${2}_key)"
	fi
}

get_ifname() {

	if [ ! -z "$(NG wifi_phy${1}iface${2}_ifname)" ];
	then
		echo "$(NG wifi_phy${1}iface${2}_ifname)"
	else
		echo "phy${1}-sta${2}"
	fi
}

print_ifname() {

	if [ ! -z "$(NG wifi_phy${1}iface${2}_ifname)" ];
	then
		uci set "wireless.phy${1}iface${2}.ifname=$(NG wifi_phy${1}iface${2}_ifname)"
	else
		uci set "wireless.phy${1}iface${2}.ifname=phy${1}-ap${2}"
	fi
}

print_ifname_client() {

	if [ ! -z "$(NG wifi_phy${1}iface${2}_ifname)" ];
	then
		uci set "wireless.phy${1}iface${2}.ifname=$(NG wifi_phy${1}iface${2}_ifname)"
		client_ifaces="$client_ifaces $(NG wifi_phy${1}iface${2}_ifname)"
	else
		uci set "wireless.phy${1}iface${2}.ifname=phy${1}-sta${2}"
		client_ifaces="$client_ifaces phy${1}-sta${2}"
	fi
	client_ifaces=$(echo "$client_ifaces" | sed 's/^[[:space:]]*//;s/[[:space:]]*$//')
}

print_mac_filter() {

	if [ ! -z "$(NG wifi_phy${1}iface${2}_macfilter)" ] && [ ! -z "$(NG wifi_phy${1}iface${2}_maclist)" ];
	then
		uci set "wireless.phy${1}iface${2}.macfilter=$(NG wifi_phy${1}iface${2}_macfilter)"

		mac_addresses=$(echo "$(NG wifi_phy${1}iface${2}_maclist)" | grep -oE '([0-9A-Fa-f]{2}:){5}[0-9A-Fa-f]{2}' | tr '\n' ' ')
		mac_addresses=${mac_addresses% }
		uci set "wireless.phy${1}iface${2}.maclist=${mac_addresses}"
	fi
}

manage_relayd() {
	# Kill any existing relayd instances
	# relayd runs with interface name as process name, so we need to kill all instances
	killall -q relayd 2>/dev/null

	# Start relayd for each wireless bridge interface
	for i in $(seq 0 1 $(($phycount - 1)));
	do
		for j in $(seq 0 1 $(($(NG "wifi_phy${i}_ifaces") - 1)));
		do
			# If interface is enabled and in bridge mode
			if [ $(NG "wifi_phy${i}iface${j}_enable") -eq 1 ] && [ "$(NG "wifi_phy${i}iface${j}_mode")" == "bridge" ];
			then
				start-stop-daemon -b -S -n $(get_ifname ${i} ${j}) -x \
				/usr/sbin/relayd -- \
				-B -D \
				-I $(get_ifname ${i} ${j}) \
				-I "$(NG wifi_phy${i}iface${j}_network)" \
				$([ -n "$(NG wan_ipaddr)" ] && [ "$(NG wan_ipaddr)" != "0.0.0.0" ] && echo "-L $(NG wan_ipaddr)")
			fi
		done
	done
}

json_for_each_item "count_phy" "wlan"

# Validate PHY count
if [ $phycount -eq 0 ]; then
	logger -p user.error "No WiFi PHYs found in board.json, aborting WiFi start"
	exit 1
fi

# Cross-check with nvram
nvram_phycount=$(NG wifi_phy_count)
if [ -n "$nvram_phycount" ] && [ $phycount -ne $nvram_phycount ]; then
	logger -p user.warning "PHY count mismatch: board.json=$phycount nvram=$nvram_phycount, using board.json count"
fi

# For each wireless device
for i in $(seq 0 1 $(($phycount - 1)));
do
	uci set "wireless.radio${i}=wifi-device"
	uci set "wireless.radio${i}.type=mac80211"
	print_phy_path ${i}
	uci set "wireless.radio${i}.band=$(NG wifi_phy${i}_band)"
	uci set "wireless.radio${i}.channel=$(NG wifi_phy${i}_channel)"
	print_htmode ${i}
	print_legacy_rates ${i}
	uci set "wireless.radio${i}.txpower=$(NG wifi_phy${i}_power)"
	uci set "wireless.radio${i}.country=$(NG wifi_phy${i}_country)"
	uci set "wireless.radio${i}.cell_density=0"
	uci set "wireless.radio${i}.noscan=$(NG wifi_phy${i}_noscan)"

	# For each device interface
	for j in $(seq 0 1 $(($(NG "wifi_phy${i}_ifaces") - 1)));
	do
		# If interface is enabled
		if [ $(NG "wifi_phy${i}iface${j}_enable") -eq 1 ];
		then
			# If interface is an Access Point
			if [ "$(NG "wifi_phy${i}iface${j}_mode")" == "ap" ];
			then
				start_hostapd=1

				uci set "wireless.phy${i}iface${j}=wifi-iface"
				uci set "wireless.phy${i}iface${j}.device=radio${i}"
				uci set "wireless.phy${i}iface${j}.mode=$(NG wifi_phy${i}iface${j}_mode)"
				uci set "wireless.phy${i}iface${j}.ssid=$(NG wifi_phy${i}iface${j}_essid)"
				uci set "wireless.phy${i}iface${j}.ocv=0"
				print_wmm ${i} ${j}
				print_encryption ${i} ${j}
				uci set "wireless.phy${i}iface${j}.hidden=$(NG wifi_phy${i}iface${j}_hidden)"
				uci set "wireless.phy${i}iface${j}.isolate=$(NG wifi_phy${i}iface${j}_isolate)"
				uci set "wireless.phy${i}iface${j}.bridge=$(NG wifi_phy${i}iface${j}_network)"
				print_ifname ${i} ${j}
				print_mac_filter ${i} ${j}
			fi

			if [ "$(NG "wifi_phy${i}iface${j}_mode")" == "sta" ] || [ "$(NG "wifi_phy${i}iface${j}_mode")" == "bridge" ];
			then
				start_wpa_supplicant=1

				uci set "wireless.phy${i}iface${j}=wifi-iface"
				uci set "wireless.phy${i}iface${j}.device=radio${i}"
				uci set "wireless.phy${i}iface${j}.mode=sta"
				uci set "wireless.phy${i}iface${j}.ssid=$(NG wifi_phy${i}iface${j}_essid)"
				uci set "wireless.phy${i}iface${j}.bssid=$(NG wifi_phy${i}iface${j}_bssid)"
				print_encryption ${i} ${j}
				uci set "wireless.phy${i}iface${j}.hidden=$(NG wifi_phy${i}iface${j}_hidden)"
				uci set "wireless.phy${i}iface${j}.isolate=$(NG wifi_phy${i}iface${j}_isolate)"
				print_ifname_client ${i} ${j}
				print_mac_filter ${i} ${j}
			fi
		fi
	done
done
uci commit wireless

# For reload mode, verify all required daemons are running
if [ "$MODE" = "reload" ]; then
	# If no interfaces are enabled, stop all WiFi services
	if [ "${start_hostapd}" == "0" ] && [ "${start_wpa_supplicant}" == "0" ]; then
		logger -p user.info "All WiFi interfaces disabled, stopping WiFi services"
		killall -q hostapd_cli 2>/dev/null
		killall -q hostapd 2>/dev/null
		killall -q wpa_supplicant 2>/dev/null
		killall -q netifd 2>/dev/null
		killall -q ubusd 2>/dev/null
		killall -q relayd 2>/dev/null
		rm -f /etc/config/wireless
		exit 0
	fi

	# Check essential daemons
	if ! pidof ubusd > /dev/null || ! pidof netifd > /dev/null; then
		logger -p user.warning "Essential WiFi daemons (ubusd/netifd) not running, performing full restart instead of reload"
		MODE="start"
	fi

	# Check hostapd if needed
	if [ "${start_hostapd}" == "1" ] && [ "$MODE" = "reload" ]; then
		if ! pidof hostapd > /dev/null || ! pidof hostapd_cli > /dev/null; then
			logger -p user.warning "hostapd/hostapd_cli required but not running, performing full restart instead of reload"
			MODE="start"
		fi
	fi

	# Check wpa_supplicant if needed
	if [ "${start_wpa_supplicant}" == "1" ] && [ "$MODE" = "reload" ]; then
		if ! pidof wpa_supplicant > /dev/null; then
			logger -p user.warning "wpa_supplicant required but not running, performing full restart instead of reload"
			MODE="start"
		fi
	fi

	# If still in reload mode, all required daemons are running
	if [ "$MODE" = "reload" ]; then
		# Proceed with reload
		wifi reload

		# Restart relayd for wireless bridge interfaces (interface names may have changed)
		manage_relayd

		logger -p user.info "WiFi configuration reloaded"
		exit 0
	fi

	# Mode was changed to "start" due to unhealthy daemons - clean up before restart
	logger -p user.info "Stopping all WiFi services before full restart"

	# Send SIGTERM to all daemons first (same order as stop_wifi() in C)
	for daemon in relayd netifd hostapd_cli hostapd wpa_supplicant ubusd; do
		if pidof $daemon > /dev/null; then
			killall -q $daemon 2>/dev/null
		fi
	done

	# Wait for all daemons to die (up to 5 seconds total, 50 deciseconds like C code)
	for i in 1 2 3 4 5 6 7 8 9 10; do
		all_dead=1
		for daemon in relayd netifd hostapd_cli hostapd wpa_supplicant ubusd; do
			if pidof $daemon > /dev/null; then
				all_dead=0
				break
			fi
		done
		[ $all_dead -eq 1 ] && break
		sleep 0.5
	done

	# Force kill any survivors with SIGKILL (like C code does after timeout)
	for daemon in relayd netifd hostapd_cli hostapd wpa_supplicant ubusd; do
		if pidof $daemon > /dev/null; then
			killall -q -9 $daemon 2>/dev/null
		fi
	done

	# Brief wait after SIGKILL to ensure processes are fully terminated
	sleep 0.5

	# Note: Don't delete /etc/config here - we already generated wireless config above
	# and will use it in start mode below
	# Fall through to start mode below
fi

# Only start services and wait for interfaces on initial start (not reload)
if [ "$MODE" = "start" ]; then
	if [ "${start_hostapd}" == "1" ] || [ "${start_wpa_supplicant}" == "1" ];
	then
		start-stop-daemon -b -S -n ubusd -x /usr/sbin/ubusd
		start-stop-daemon -b -S -n netifd -x /usr/sbin/netifd

		if [ "${start_wpa_supplicant}" == "1" ];
		then
			mkdir -p /var/run/wpa_supplicant
			start-stop-daemon -b -S -n wpa_supplicant -x /usr/sbin/wpa_supplicant -- -n -s -g /var/run/wpa_supplicant/global
		fi

		if [ "${start_hostapd}" == "1" ];
		then
			start-stop-daemon -b -S -n hostapd -x /usr/sbin/hostapd -- -s -g /var/run/hostapd/global
		fi


		timeout_duration=20
		start_time=$(date +%s)
		error=0
		all_up=0

		while [ $(($(date +%s) - start_time)) -lt $timeout_duration ] && [ $all_up -eq 0 ]; do
			all_up=1
			for iface in $client_ifaces; do
				if ! ip link show "$iface" 2>/dev/null | grep -q "state UP"; then
					all_up=0
				fi
			done
			sleep 0.1
		done

		for iface in $client_ifaces; do
			if ! ip link show "$iface" 2>/dev/null | grep -q "state UP"; then
				logger -p user.error "Interface $iface did not come up within $timeout_duration seconds"
				error=1
			fi
		done

		while ! hostapd_cli -s /var/run/hostapd/ -i global ping > /dev/null 2>&1; do
			sleep .1
		done
		/usr/sbin/hostapd_cli -B -s /var/run/hostapd/ -i global -a /usr/bin/hostapd_event
	fi

	# Start relayd for wireless bridge interfaces
	manage_relayd

	if [ "$error" == 1 ];
	then
		logger -p user.error "Wireless client(s) did not start or connect, check logs and settings"
	else
		logger -p user.info "Wireless client(s) started successfully"
	fi
fi
