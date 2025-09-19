#!/bin/sh

. /usr/sbin/nvram_ops
. /usr/share/libubox/jshn.sh

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

json_for_each_item "count_phy" "wlan"

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

# Start relayd when using Wireless Ethernet Bridge
# For each wireless device
for i in $(seq 0 1 $(($phycount - 1)));
do
	# For each device interface
	for j in $(seq 0 1 $(($(NG "wifi_phy${i}_ifaces") - 1)));
	do
		# If interface is enabled
		if [ $(NG "wifi_phy${i}iface${j}_enable") -eq 1 ];
		then
			if [ "$(NG "wifi_phy${i}iface${j}_mode")" == "bridge" ];
			then
				start-stop-daemon -b -S -n $(get_ifname ${i} ${j}) -x \
				/usr/sbin/relayd -- \
				-B -D \
				-I $(get_ifname ${i} ${j}) \
				-I "$(NG wifi_phy${i}iface${j}_network)" \
				$([ -n "$(NG wan_ipaddr)" ] && [ "$(NG wan_ipaddr)" != "0.0.0.0" ] && echo "-L $(NG wan_ipaddr)")
			fi
		fi
	done
done

if [ "$error" == 1 ];
then
	logger -p user.error "Wireless client(s) did not start or connect, check logs and settings"
else
	logger -p user.info "Wireless client(s) started successfully"
fi
