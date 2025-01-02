#!/bin/sh

. /usr/sbin/nvram_ops
. /usr/share/libubox/jshn.sh

mkdir -p /etc/config
touch /etc/config/network

rm -f /etc/config/wireless
touch /etc/config/wireless

json_load_file /etc/board.json

phycount=0
enabled_interface=0

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
			enabled_interface=1

			# If interface is an Access Point
			if [ "$(NG "wifi_phy${i}iface${j}_mode")" == "ap" ];
			then
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
			fi
		fi
	done
done
uci commit wireless

if [ "${enabled_interface}" == "1" ];
then
	start-stop-daemon -b -S -n ubusd -x /usr/sbin/ubusd
	start-stop-daemon -b -S -n hostapd -x /usr/sbin/hostapd -- -s -g /var/run/hostapd/global
	start-stop-daemon -b -S -n netifd -x /usr/sbin/netifd
fi
