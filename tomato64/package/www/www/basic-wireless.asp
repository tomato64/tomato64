<!DOCTYPE html>
<!--
	Wireless Web GUI
	Copyright (C) 2024 Lance Fredrickson
	lancethepants@gmail.com
-->

<html lang="en-GB">
<head>
<meta http-equiv="content-type" content="text/html;charset=utf-8">
<meta name="robots" content="noindex,nofollow">
<title>[<% ident(); %>] Wifi</title>
<link rel="stylesheet" type="text/css" href="tomato.css?rel=<% version(); %>">
<% css(); %>
<script src="isup.jsz?rel=<% version(); %>"></script>
<script src="tomato.js?rel=<% version(); %>"></script>
<script src="wireless.js?rel=<% version(); %>"></script>

<script>

//	<% nvram("wifi_sta_list,wan_sta,wan2_sta,wan3_sta,wan4_sta,wifi_phy0_band,wifi_phy0_mode,wifi_phy0_channel,wifi_phy0_width,wifi_phy0_brates,wifi_phy0_power,wifi_phy0_country,wifi_phy0_noscan,wifi_phy0_ifaces,wifi_phy1_band,wifi_phy1_mode,wifi_phy1_channel,wifi_phy1_width,wifi_phy1_brates,wifi_phy1_power,wifi_phy1_country,wifi_phy1_noscan,wifi_phy1_ifaces,wifi_phy2_band,wifi_phy2_mode,wifi_phy2_channel,wifi_phy2_width,wifi_phy2_brates,wifi_phy2_power,wifi_phy2_country,wifi_phy2_noscan,wifi_phy2_ifaces,wifi_phy0iface0_enable,wifi_phy0iface0_mode,wifi_phy0iface0_essid,wifi_phy0iface0_bssid,wifi_phy0iface0_network,wifi_phy0iface0_hidden,wifi_phy0iface0_wmm,wifi_phy0iface0_uapsd,wifi_phy0iface0_encryption,wifi_phy0iface0_cipher,wifi_phy0iface0_key,wifi_phy0iface0_isolate,wifi_phy0iface0_br_isolate,wifi_phy0iface0_ifname,wifi_phy0iface0_macfilter,wifi_phy0iface0_maclist,wifi_phy0iface1_enable,wifi_phy0iface1_mode,wifi_phy0iface1_essid,wifi_phy0iface1_bssid,wifi_phy0iface1_network,wifi_phy0iface1_hidden,wifi_phy0iface1_wmm,wifi_phy0iface1_uapsd,wifi_phy0iface1_encryption,wifi_phy0iface1_cipher,wifi_phy0iface1_key,wifi_phy0iface1_isolate,wifi_phy0iface1_br_isolate,wifi_phy0iface1_ifname,wifi_phy0iface1_macfilter,wifi_phy0iface1_maclist,wifi_phy0iface2_enable,wifi_phy0iface2_mode,wifi_phy0iface2_essid,wifi_phy0iface2_bssid,wifi_phy0iface2_network,wifi_phy0iface2_hidden,wifi_phy0iface2_wmm,wifi_phy0iface2_uapsd,wifi_phy0iface2_encryption,wifi_phy0iface2_cipher,wifi_phy0iface2_key,wifi_phy0iface2_isolate,wifi_phy0iface2_br_isolate,wifi_phy0iface2_ifname,wifi_phy0iface2_macfilter,wifi_phy0iface2_maclist,wifi_phy0iface3_enable,wifi_phy0iface3_mode,wifi_phy0iface3_essid,wifi_phy0iface3_bssid,wifi_phy0iface3_network,wifi_phy0iface3_hidden,wifi_phy0iface3_wmm,wifi_phy0iface3_uapsd,wifi_phy0iface3_encryption,wifi_phy0iface3_cipher,wifi_phy0iface3_key,wifi_phy0iface3_isolate,wifi_phy0iface3_br_isolate,wifi_phy0iface3_ifname,wifi_phy0iface3_macfilter,wifi_phy0iface3_maclist,wifi_phy0iface4_enable,wifi_phy0iface4_mode,wifi_phy0iface4_essid,wifi_phy0iface4_bssid,wifi_phy0iface4_network,wifi_phy0iface4_hidden,wifi_phy0iface4_wmm,wifi_phy0iface4_uapsd,wifi_phy0iface4_encryption,wifi_phy0iface4_cipher,wifi_phy0iface4_key,wifi_phy0iface4_isolate,wifi_phy0iface4_br_isolate,wifi_phy0iface4_ifname,wifi_phy0iface4_macfilter,wifi_phy0iface4_maclist,wifi_phy0iface5_enable,wifi_phy0iface5_mode,wifi_phy0iface5_essid,wifi_phy0iface5_bssid,wifi_phy0iface5_network,wifi_phy0iface5_hidden,wifi_phy0iface5_wmm,wifi_phy0iface5_uapsd,wifi_phy0iface5_encryption,wifi_phy0iface5_cipher,wifi_phy0iface5_key,wifi_phy0iface5_isolate,wifi_phy0iface5_br_isolate,wifi_phy0iface5_ifname,wifi_phy0iface5_macfilter,wifi_phy0iface5_maclist,wifi_phy0iface6_enable,wifi_phy0iface6_mode,wifi_phy0iface6_essid,wifi_phy0iface6_bssid,wifi_phy0iface6_network,wifi_phy0iface6_hidden,wifi_phy0iface6_wmm,wifi_phy0iface6_uapsd,wifi_phy0iface6_encryption,wifi_phy0iface6_cipher,wifi_phy0iface6_key,wifi_phy0iface6_isolate,wifi_phy0iface6_br_isolate,wifi_phy0iface6_ifname,wifi_phy0iface6_macfilter,wifi_phy0iface6_maclist,wifi_phy0iface7_enable,wifi_phy0iface7_mode,wifi_phy0iface7_essid,wifi_phy0iface7_bssid,wifi_phy0iface7_network,wifi_phy0iface7_hidden,wifi_phy0iface7_wmm,wifi_phy0iface7_uapsd,wifi_phy0iface7_encryption,wifi_phy0iface7_cipher,wifi_phy0iface7_key,wifi_phy0iface7_isolate,wifi_phy0iface7_br_isolate,wifi_phy0iface7_ifname,wifi_phy0iface7_macfilter,wifi_phy0iface7_maclist,wifi_phy0iface8_enable,wifi_phy0iface8_mode,wifi_phy0iface8_essid,wifi_phy0iface8_bssid,wifi_phy0iface8_network,wifi_phy0iface8_hidden,wifi_phy0iface8_wmm,wifi_phy0iface8_uapsd,wifi_phy0iface8_encryption,wifi_phy0iface8_cipher,wifi_phy0iface8_key,wifi_phy0iface8_isolate,wifi_phy0iface8_br_isolate,wifi_phy0iface8_ifname,wifi_phy0iface8_macfilter,wifi_phy0iface8_maclist,wifi_phy0iface9_enable,wifi_phy0iface9_mode,wifi_phy0iface9_essid,wifi_phy0iface9_bssid,wifi_phy0iface9_network,wifi_phy0iface9_hidden,wifi_phy0iface9_wmm,wifi_phy0iface9_uapsd,wifi_phy0iface9_encryption,wifi_phy0iface9_cipher,wifi_phy0iface9_key,wifi_phy0iface9_isolate,wifi_phy0iface9_br_isolate,wifi_phy0iface9_ifname,wifi_phy0iface9_macfilter,wifi_phy0iface9_maclist,wifi_phy0iface10_enable,wifi_phy0iface10_mode,wifi_phy0iface10_essid,wifi_phy0iface10_bssid,wifi_phy0iface10_network,wifi_phy0iface10_hidden,wifi_phy0iface10_wmm,wifi_phy0iface10_uapsd,wifi_phy0iface10_encryption,wifi_phy0iface10_cipher,wifi_phy0iface10_key,wifi_phy0iface10_isolate,wifi_phy0iface10_br_isolate,wifi_phy0iface10_ifname,wifi_phy0iface10_macfilter,wifi_phy0iface10_maclist,wifi_phy0iface11_enable,wifi_phy0iface11_mode,wifi_phy0iface11_essid,wifi_phy0iface11_bssid,wifi_phy0iface11_network,wifi_phy0iface11_hidden,wifi_phy0iface11_wmm,wifi_phy0iface11_uapsd,wifi_phy0iface11_encryption,wifi_phy0iface11_cipher,wifi_phy0iface11_key,wifi_phy0iface11_isolate,wifi_phy0iface11_br_isolate,wifi_phy0iface11_ifname,wifi_phy0iface11_macfilter,wifi_phy0iface11_maclist,wifi_phy0iface12_enable,wifi_phy0iface12_mode,wifi_phy0iface12_essid,wifi_phy0iface12_bssid,wifi_phy0iface12_network,wifi_phy0iface12_hidden,wifi_phy0iface12_wmm,wifi_phy0iface12_uapsd,wifi_phy0iface12_encryption,wifi_phy0iface12_cipher,wifi_phy0iface12_key,wifi_phy0iface12_isolate,wifi_phy0iface12_br_isolate,wifi_phy0iface12_ifname,wifi_phy0iface12_macfilter,wifi_phy0iface12_maclist,wifi_phy0iface13_enable,wifi_phy0iface13_mode,wifi_phy0iface13_essid,wifi_phy0iface13_bssid,wifi_phy0iface13_network,wifi_phy0iface13_hidden,wifi_phy0iface13_wmm,wifi_phy0iface13_uapsd,wifi_phy0iface13_encryption,wifi_phy0iface13_cipher,wifi_phy0iface13_key,wifi_phy0iface13_isolate,wifi_phy0iface13_br_isolate,wifi_phy0iface13_ifname,wifi_phy0iface13_macfilter,wifi_phy0iface13_maclist,wifi_phy0iface14_enable,wifi_phy0iface14_mode,wifi_phy0iface14_essid,wifi_phy0iface14_bssid,wifi_phy0iface14_network,wifi_phy0iface14_hidden,wifi_phy0iface14_wmm,wifi_phy0iface14_uapsd,wifi_phy0iface14_encryption,wifi_phy0iface14_cipher,wifi_phy0iface14_key,wifi_phy0iface14_isolate,wifi_phy0iface14_br_isolate,wifi_phy0iface14_ifname,wifi_phy0iface14_macfilter,wifi_phy0iface14_maclist,wifi_phy0iface15_enable,wifi_phy0iface15_mode,wifi_phy0iface15_essid,wifi_phy0iface15_bssid,wifi_phy0iface15_network,wifi_phy0iface15_hidden,wifi_phy0iface15_wmm,wifi_phy0iface15_uapsd,wifi_phy0iface15_encryption,wifi_phy0iface15_cipher,wifi_phy0iface15_key,wifi_phy0iface15_isolate,wifi_phy0iface15_br_isolate,wifi_phy0iface15_ifname,wifi_phy0iface15_macfilter,wifi_phy0iface15_maclist,wifi_phy1iface0_enable,wifi_phy1iface0_mode,wifi_phy1iface0_essid,wifi_phy1iface0_bssid,wifi_phy1iface0_network,wifi_phy1iface0_hidden,wifi_phy1iface0_wmm,wifi_phy1iface0_uapsd,wifi_phy1iface0_encryption,wifi_phy1iface0_cipher,wifi_phy1iface0_key,wifi_phy1iface0_isolate,wifi_phy1iface0_br_isolate,wifi_phy1iface0_ifname,wifi_phy1iface0_macfilter,wifi_phy1iface0_maclist,wifi_phy1iface1_enable,wifi_phy1iface1_mode,wifi_phy1iface1_essid,wifi_phy1iface1_bssid,wifi_phy1iface1_network,wifi_phy1iface1_hidden,wifi_phy1iface1_wmm,wifi_phy1iface1_uapsd,wifi_phy1iface1_encryption,wifi_phy1iface1_cipher,wifi_phy1iface1_key,wifi_phy1iface1_isolate,wifi_phy1iface1_br_isolate,wifi_phy1iface1_ifname,wifi_phy1iface1_macfilter,wifi_phy1iface1_maclist,wifi_phy1iface2_enable,wifi_phy1iface2_mode,wifi_phy1iface2_essid,wifi_phy1iface2_bssid,wifi_phy1iface2_network,wifi_phy1iface2_hidden,wifi_phy1iface2_wmm,wifi_phy1iface2_uapsd,wifi_phy1iface2_encryption,wifi_phy1iface2_cipher,wifi_phy1iface2_key,wifi_phy1iface2_isolate,wifi_phy1iface2_br_isolate,wifi_phy1iface2_ifname,wifi_phy1iface2_macfilter,wifi_phy1iface2_maclist,wifi_phy1iface3_enable,wifi_phy1iface3_mode,wifi_phy1iface3_essid,wifi_phy1iface3_bssid,wifi_phy1iface3_network,wifi_phy1iface3_hidden,wifi_phy1iface3_wmm,wifi_phy1iface3_uapsd,wifi_phy1iface3_encryption,wifi_phy1iface3_cipher,wifi_phy1iface3_key,wifi_phy1iface3_isolate,wifi_phy1iface3_br_isolate,wifi_phy1iface3_ifname,wifi_phy1iface3_macfilter,wifi_phy1iface3_maclist,wifi_phy1iface4_enable,wifi_phy1iface4_mode,wifi_phy1iface4_essid,wifi_phy1iface4_bssid,wifi_phy1iface4_network,wifi_phy1iface4_hidden,wifi_phy1iface4_wmm,wifi_phy1iface4_uapsd,wifi_phy1iface4_encryption,wifi_phy1iface4_cipher,wifi_phy1iface4_key,wifi_phy1iface4_isolate,wifi_phy1iface4_br_isolate,wifi_phy1iface4_ifname,wifi_phy1iface4_macfilter,wifi_phy1iface4_maclist,wifi_phy1iface5_enable,wifi_phy1iface5_mode,wifi_phy1iface5_essid,wifi_phy1iface5_bssid,wifi_phy1iface5_network,wifi_phy1iface5_hidden,wifi_phy1iface5_wmm,wifi_phy1iface5_uapsd,wifi_phy1iface5_encryption,wifi_phy1iface5_cipher,wifi_phy1iface5_key,wifi_phy1iface5_isolate,wifi_phy1iface5_br_isolate,wifi_phy1iface5_ifname,wifi_phy1iface5_macfilter,wifi_phy1iface5_maclist,wifi_phy1iface6_enable,wifi_phy1iface6_mode,wifi_phy1iface6_essid,wifi_phy1iface6_bssid,wifi_phy1iface6_network,wifi_phy1iface6_hidden,wifi_phy1iface6_wmm,wifi_phy1iface6_uapsd,wifi_phy1iface6_encryption,wifi_phy1iface6_cipher,wifi_phy1iface6_key,wifi_phy1iface6_isolate,wifi_phy1iface6_br_isolate,wifi_phy1iface6_ifname,wifi_phy1iface6_macfilter,wifi_phy1iface6_maclist,wifi_phy1iface7_enable,wifi_phy1iface7_mode,wifi_phy1iface7_essid,wifi_phy1iface7_bssid,wifi_phy1iface7_network,wifi_phy1iface7_hidden,wifi_phy1iface7_wmm,wifi_phy1iface7_uapsd,wifi_phy1iface7_encryption,wifi_phy1iface7_cipher,wifi_phy1iface7_key,wifi_phy1iface7_isolate,wifi_phy1iface7_br_isolate,wifi_phy1iface7_ifname,wifi_phy1iface7_macfilter,wifi_phy1iface7_maclist,wifi_phy1iface8_enable,wifi_phy1iface8_mode,wifi_phy1iface8_essid,wifi_phy1iface8_bssid,wifi_phy1iface8_network,wifi_phy1iface8_hidden,wifi_phy1iface8_wmm,wifi_phy1iface8_uapsd,wifi_phy1iface8_encryption,wifi_phy1iface8_cipher,wifi_phy1iface8_key,wifi_phy1iface8_isolate,wifi_phy1iface8_br_isolate,wifi_phy1iface8_ifname,wifi_phy1iface8_macfilter,wifi_phy1iface8_maclist,wifi_phy1iface9_enable,wifi_phy1iface9_mode,wifi_phy1iface9_essid,wifi_phy1iface9_bssid,wifi_phy1iface9_network,wifi_phy1iface9_hidden,wifi_phy1iface9_wmm,wifi_phy1iface9_uapsd,wifi_phy1iface9_encryption,wifi_phy1iface9_cipher,wifi_phy1iface9_key,wifi_phy1iface9_isolate,wifi_phy1iface9_br_isolate,wifi_phy1iface9_ifname,wifi_phy1iface9_macfilter,wifi_phy1iface9_maclist,wifi_phy1iface10_enable,wifi_phy1iface10_mode,wifi_phy1iface10_essid,wifi_phy1iface10_bssid,wifi_phy1iface10_network,wifi_phy1iface10_hidden,wifi_phy1iface10_wmm,wifi_phy1iface10_uapsd,wifi_phy1iface10_encryption,wifi_phy1iface10_cipher,wifi_phy1iface10_key,wifi_phy1iface10_isolate,wifi_phy1iface10_br_isolate,wifi_phy1iface10_ifname,wifi_phy1iface10_macfilter,wifi_phy1iface10_maclist,wifi_phy1iface11_enable,wifi_phy1iface11_mode,wifi_phy1iface11_essid,wifi_phy1iface11_bssid,wifi_phy1iface11_network,wifi_phy1iface11_hidden,wifi_phy1iface11_wmm,wifi_phy1iface11_uapsd,wifi_phy1iface11_encryption,wifi_phy1iface11_cipher,wifi_phy1iface11_key,wifi_phy1iface11_isolate,wifi_phy1iface11_br_isolate,wifi_phy1iface11_ifname,wifi_phy1iface11_macfilter,wifi_phy1iface11_maclist,wifi_phy1iface12_enable,wifi_phy1iface12_mode,wifi_phy1iface12_essid,wifi_phy1iface12_bssid,wifi_phy1iface12_network,wifi_phy1iface12_hidden,wifi_phy1iface12_wmm,wifi_phy1iface12_uapsd,wifi_phy1iface12_encryption,wifi_phy1iface12_cipher,wifi_phy1iface12_key,wifi_phy1iface12_isolate,wifi_phy1iface12_br_isolate,wifi_phy1iface12_ifname,wifi_phy1iface12_macfilter,wifi_phy1iface12_maclist,wifi_phy1iface13_enable,wifi_phy1iface13_mode,wifi_phy1iface13_essid,wifi_phy1iface13_bssid,wifi_phy1iface13_network,wifi_phy1iface13_hidden,wifi_phy1iface13_wmm,wifi_phy1iface13_uapsd,wifi_phy1iface13_encryption,wifi_phy1iface13_cipher,wifi_phy1iface13_key,wifi_phy1iface13_isolate,wifi_phy1iface13_br_isolate,wifi_phy1iface13_ifname,wifi_phy1iface13_macfilter,wifi_phy1iface13_maclist,wifi_phy1iface14_enable,wifi_phy1iface14_mode,wifi_phy1iface14_essid,wifi_phy1iface14_bssid,wifi_phy1iface14_network,wifi_phy1iface14_hidden,wifi_phy1iface14_wmm,wifi_phy1iface14_uapsd,wifi_phy1iface14_encryption,wifi_phy1iface14_cipher,wifi_phy1iface14_key,wifi_phy1iface14_isolate,wifi_phy1iface14_br_isolate,wifi_phy1iface14_ifname,wifi_phy1iface14_macfilter,wifi_phy1iface14_maclist,wifi_phy1iface15_enable,wifi_phy1iface15_mode,wifi_phy1iface15_essid,wifi_phy1iface15_bssid,wifi_phy1iface15_network,wifi_phy1iface15_hidden,wifi_phy1iface15_wmm,wifi_phy1iface15_uapsd,wifi_phy1iface15_encryption,wifi_phy1iface15_cipher,wifi_phy1iface15_key,wifi_phy1iface15_isolate,wifi_phy1iface15_br_isolate,wifi_phy1iface15_ifname,wifi_phy1iface15_macfilter,wifi_phy1iface15_maclist,wifi_phy2iface0_enable,wifi_phy2iface0_mode,wifi_phy2iface0_essid,wifi_phy2iface0_bssid,wifi_phy2iface0_network,wifi_phy2iface0_hidden,wifi_phy2iface0_wmm,wifi_phy2iface0_uapsd,wifi_phy2iface0_encryption,wifi_phy2iface0_cipher,wifi_phy2iface0_key,wifi_phy2iface0_isolate,wifi_phy2iface0_br_isolate,wifi_phy2iface0_ifname,wifi_phy2iface0_macfilter,wifi_phy2iface0_maclist,wifi_phy2iface1_enable,wifi_phy2iface1_mode,wifi_phy2iface1_essid,wifi_phy2iface1_bssid,wifi_phy2iface1_network,wifi_phy2iface1_hidden,wifi_phy2iface1_wmm,wifi_phy2iface1_uapsd,wifi_phy2iface1_encryption,wifi_phy2iface1_cipher,wifi_phy2iface1_key,wifi_phy2iface1_isolate,wifi_phy2iface1_br_isolate,wifi_phy2iface1_ifname,wifi_phy2iface1_macfilter,wifi_phy2iface1_maclist,wifi_phy2iface2_enable,wifi_phy2iface2_mode,wifi_phy2iface2_essid,wifi_phy2iface2_bssid,wifi_phy2iface2_network,wifi_phy2iface2_hidden,wifi_phy2iface2_wmm,wifi_phy2iface2_uapsd,wifi_phy2iface2_encryption,wifi_phy2iface2_cipher,wifi_phy2iface2_key,wifi_phy2iface2_isolate,wifi_phy2iface2_br_isolate,wifi_phy2iface2_ifname,wifi_phy2iface2_macfilter,wifi_phy2iface2_maclist,wifi_phy2iface3_enable,wifi_phy2iface3_mode,wifi_phy2iface3_essid,wifi_phy2iface3_bssid,wifi_phy2iface3_network,wifi_phy2iface3_hidden,wifi_phy2iface3_wmm,wifi_phy2iface3_uapsd,wifi_phy2iface3_encryption,wifi_phy2iface3_cipher,wifi_phy2iface3_key,wifi_phy2iface3_isolate,wifi_phy2iface3_br_isolate,wifi_phy2iface3_ifname,wifi_phy2iface3_macfilter,wifi_phy2iface3_maclist,wifi_phy2iface4_enable,wifi_phy2iface4_mode,wifi_phy2iface4_essid,wifi_phy2iface4_bssid,wifi_phy2iface4_network,wifi_phy2iface4_hidden,wifi_phy2iface4_wmm,wifi_phy2iface4_uapsd,wifi_phy2iface4_encryption,wifi_phy2iface4_cipher,wifi_phy2iface4_key,wifi_phy2iface4_isolate,wifi_phy2iface4_br_isolate,wifi_phy2iface4_ifname,wifi_phy2iface4_macfilter,wifi_phy2iface4_maclist,wifi_phy2iface5_enable,wifi_phy2iface5_mode,wifi_phy2iface5_essid,wifi_phy2iface5_bssid,wifi_phy2iface5_network,wifi_phy2iface5_hidden,wifi_phy2iface5_wmm,wifi_phy2iface5_uapsd,wifi_phy2iface5_encryption,wifi_phy2iface5_cipher,wifi_phy2iface5_key,wifi_phy2iface5_isolate,wifi_phy2iface5_br_isolate,wifi_phy2iface5_ifname,wifi_phy2iface5_macfilter,wifi_phy2iface5_maclist,wifi_phy2iface6_enable,wifi_phy2iface6_mode,wifi_phy2iface6_essid,wifi_phy2iface6_bssid,wifi_phy2iface6_network,wifi_phy2iface6_hidden,wifi_phy2iface6_wmm,wifi_phy2iface6_uapsd,wifi_phy2iface6_encryption,wifi_phy2iface6_cipher,wifi_phy2iface6_key,wifi_phy2iface6_isolate,wifi_phy2iface6_br_isolate,wifi_phy2iface6_ifname,wifi_phy2iface6_macfilter,wifi_phy2iface6_maclist,wifi_phy2iface7_enable,wifi_phy2iface7_mode,wifi_phy2iface7_essid,wifi_phy2iface7_bssid,wifi_phy2iface7_network,wifi_phy2iface7_hidden,wifi_phy2iface7_wmm,wifi_phy2iface7_uapsd,wifi_phy2iface7_encryption,wifi_phy2iface7_cipher,wifi_phy2iface7_key,wifi_phy2iface7_isolate,wifi_phy2iface7_br_isolate,wifi_phy2iface7_ifname,wifi_phy2iface7_macfilter,wifi_phy2iface7_maclist,wifi_phy2iface8_enable,wifi_phy2iface8_mode,wifi_phy2iface8_essid,wifi_phy2iface8_bssid,wifi_phy2iface8_network,wifi_phy2iface8_hidden,wifi_phy2iface8_wmm,wifi_phy2iface8_uapsd,wifi_phy2iface8_encryption,wifi_phy2iface8_cipher,wifi_phy2iface8_key,wifi_phy2iface8_isolate,wifi_phy2iface8_br_isolate,wifi_phy2iface8_ifname,wifi_phy2iface8_macfilter,wifi_phy2iface8_maclist,wifi_phy2iface9_enable,wifi_phy2iface9_mode,wifi_phy2iface9_essid,wifi_phy2iface9_bssid,wifi_phy2iface9_network,wifi_phy2iface9_hidden,wifi_phy2iface9_wmm,wifi_phy2iface9_uapsd,wifi_phy2iface9_encryption,wifi_phy2iface9_cipher,wifi_phy2iface9_key,wifi_phy2iface9_isolate,wifi_phy2iface9_br_isolate,wifi_phy2iface9_ifname,wifi_phy2iface9_macfilter,wifi_phy2iface9_maclist,wifi_phy2iface10_enable,wifi_phy2iface10_mode,wifi_phy2iface10_essid,wifi_phy2iface10_bssid,wifi_phy2iface10_network,wifi_phy2iface10_hidden,wifi_phy2iface10_wmm,wifi_phy2iface10_uapsd,wifi_phy2iface10_encryption,wifi_phy2iface10_cipher,wifi_phy2iface10_key,wifi_phy2iface10_isolate,wifi_phy2iface10_br_isolate,wifi_phy2iface10_ifname,wifi_phy2iface10_macfilter,wifi_phy2iface10_maclist,wifi_phy2iface11_enable,wifi_phy2iface11_mode,wifi_phy2iface11_essid,wifi_phy2iface11_bssid,wifi_phy2iface11_network,wifi_phy2iface11_hidden,wifi_phy2iface11_wmm,wifi_phy2iface11_uapsd,wifi_phy2iface11_encryption,wifi_phy2iface11_cipher,wifi_phy2iface11_key,wifi_phy2iface11_isolate,wifi_phy2iface11_br_isolate,wifi_phy2iface11_ifname,wifi_phy2iface11_macfilter,wifi_phy2iface11_maclist,wifi_phy2iface12_enable,wifi_phy2iface12_mode,wifi_phy2iface12_essid,wifi_phy2iface12_bssid,wifi_phy2iface12_network,wifi_phy2iface12_hidden,wifi_phy2iface12_wmm,wifi_phy2iface12_uapsd,wifi_phy2iface12_encryption,wifi_phy2iface12_cipher,wifi_phy2iface12_key,wifi_phy2iface12_isolate,wifi_phy2iface12_br_isolate,wifi_phy2iface12_ifname,wifi_phy2iface12_macfilter,wifi_phy2iface12_maclist,wifi_phy2iface13_enable,wifi_phy2iface13_mode,wifi_phy2iface13_essid,wifi_phy2iface13_bssid,wifi_phy2iface13_network,wifi_phy2iface13_hidden,wifi_phy2iface13_wmm,wifi_phy2iface13_uapsd,wifi_phy2iface13_encryption,wifi_phy2iface13_cipher,wifi_phy2iface13_key,wifi_phy2iface13_isolate,wifi_phy2iface13_br_isolate,wifi_phy2iface13_ifname,wifi_phy2iface13_macfilter,wifi_phy2iface13_maclist,wifi_phy2iface14_enable,wifi_phy2iface14_mode,wifi_phy2iface14_essid,wifi_phy2iface14_bssid,wifi_phy2iface14_network,wifi_phy2iface14_hidden,wifi_phy2iface14_wmm,wifi_phy2iface14_uapsd,wifi_phy2iface14_encryption,wifi_phy2iface14_cipher,wifi_phy2iface14_key,wifi_phy2iface14_isolate,wifi_phy2iface14_br_isolate,wifi_phy2iface14_ifname,wifi_phy2iface14_macfilter,wifi_phy2iface14_maclist,wifi_phy2iface15_enable,wifi_phy2iface15_mode,wifi_phy2iface15_essid,wifi_phy2iface15_bssid,wifi_phy2iface15_network,wifi_phy2iface15_hidden,wifi_phy2iface15_wmm,wifi_phy2iface15_uapsd,wifi_phy2iface15_encryption,wifi_phy2iface15_cipher,wifi_phy2iface15_key,wifi_phy2iface15_isolate,wifi_phy2iface15_br_isolate,wifi_phy2iface15_ifname,wifi_phy2iface15_macfilter,wifi_phy2iface15_maclist,lan_ifname,lan1_ifname,lan2_ifname,lan3_ifname,lan4_ifname,lan5_ifname,lan6_ifname,lan7_ifname"); %>
//	<% wireless(); %>

var cprefix = 'basic_wireless';

var devices = [];
var interfaces = [];
var mode_loaded = [];
var width_loaded = [];
var channel_loaded = [];
var power_loaded = [];
var country_loaded = [];
var macTables = [];

for (let i = 0; i < wireless.phy_count; i++) {
	devices.push(['phy'+i, 'phy'+i]);
	mode_loaded.push(0);
	width_loaded.push(0);
	channel_loaded.push(0);
	power_loaded.push(0);
	country_loaded.push(0);
	macTables.push([]);
}

var interfaceCount = [];

for (let i = 0; i < devices.length; i++) {
	interfaceCount.push([eval('nvram.wifi_phy'+i+'_ifaces')]);
}

var deviceSections = [
	['general', 'General Setup'],
	['advanced', 'Advanced Settings']
];

var interfaceSections = [
	['general', 'General Setup'],
	['security', 'Wireless Security'],
	['filter', 'MAC-Filter'],
	['advanced', 'Advanced Settings']
];

var security_modes = [
	['psk2', 'WPA2-PSK (strong security)'],
	['sae', 'WPA3-SAE (strong security)'],
	['sae-mixed', 'WPA2/WPA3-SAE Mixed Mode (strong security)'],
	['psk-mixed', 'WPA-PSK/WPA2-PSK Mixed Mode (medium security)'],
	['psk', 'WPA-PSK (weak security)'],
	['owe', 'OWE (open network)'],
	['none', 'No Encryption (open network)']
];

var ciphers = [
	['auto', 'auto'],
	['ccmp', 'Force CCMP (AES)'],
	['ccmp256', 'Force CCMP-256 (AES)'],
	['gcmp', 'Force GCMP (AES)'],
	['gcmp256', 'Force GCMP-256 (AES)'],
	['tkip', 'Force TKIP'],
	['tkip+ccmp', 'Force TKIP and CCMP (AES)']
];

var networks = [
	['br0', 'LAN0 (br0)'],
	['br1', 'LAN1 (br1)'],
	['br2', 'LAN2 (br2)'],
	['br3', 'LAN3 (br3)'],
	['br4', 'LAN4 (br4)'],
	['br5', 'LAN5 (br5)'],
	['br6', 'LAN6 (br6)'],
	['br7', 'LAN7 (br7)'],
	['', 'none']
];

const regionNames = new Intl.DisplayNames(['en'], {
	type: 'region'
});

var cmdresult = '';
var cmd = null;
var cmdresult = '';
var changed = 0;

function show() {}

function displayCountry(device) {
	var country = '';
	var countrylist = [];
	var result = cmdresult.split('\n');

	countrylist.push(['', 'driver default']);

	for (var i = 0; i < result.length; i++) {
		if (result[i] !== "") {
			var result2 = result[i].split(/\s+/);
			if ((result2[0] == '*') || (result2[0] == ''))
				result2.shift();

			if (result2[0] !== '00')
				countrylist.push([result2[0], result2[0] + ' - ' + regionNames.of(result2[0])]);
		}
	}

	t = devices[device][0];
	e = E('_wifi_'+t+'_country');
	buf = '';
	val = (!country_loaded[device]) ? eval('nvram["wifi_'+t+'_country"]') : e.value;

	for (i = 0; i < countrylist.length; ++i)
		buf += '<option value="' + countrylist[i][0] + '"' + ((countrylist[i][0] == val) ? ' selected="selected"' : '') + '>' + countrylist[i][1] + '<\/option>';

	e = E('__wifi_'+t+'_country');
	buf = '<select name="wifi_'+t+'_country" onchange="verifyFields(this, 1)" id = "_wifi_'+t+'_country">' + buf + '<\/select>';
	elem.setInnerHTML(e, buf);
	country_loaded[device] = 1;
	cmdresult = '';
}

function refreshCountry(device) {
	var cmd = new XmlHttp();
	cmd.onCompleted = function(text, xml) {
		eval(text);
		displayCountry(device);
	}
	cmd.onError = function(x) {
		cmdresult = 'ERROR: ' + x;
		displayCountry(device);
	}
	var c = 'iwinfo phy'+device+' countrylist';
	cmd.post('shell.cgi', 'action=execute&command=' + escapeCGI(c.replace(/\r/g, '')));
}

function displayPower(device) {
	var powerDisplay = '';
	var powerlist = [];
	var result = cmdresult.split('\n');

	powerlist.push(['', 'driver default']);

	for (var i = 0; i < result.length; i++) {
		if (result[i] !== "") {
			var result2 = result[i].split(/\s+/);
			if ((result2[0] == '*') || (result2[0] == ''))
				result2.shift();

			if (result2.length >= 5) {
				powerDisplay = result2[0] + ' ' + result2[1] + ' ' + result2[2] + result2[3] + ' ' + result2[4];
			} else {
				powerDisplay = result2[0] + ' ' + result2[1] + ' ' + result2[2] + ' ' + result2[3];
			}
			powerlist.push([result2[0], powerDisplay]);
		}
	}

	t = devices[device][0];
	e = E('_wifi_'+t+'_power');
	buf = '';
	val = (!power_loaded[device]) ? eval('nvram["wifi_'+t+'_power"]') : e.value;

	for (i = 0; i < powerlist.length; ++i)
		buf += '<option value="' + powerlist[i][0] + '"' + ((powerlist[i][0] == val) ? ' selected="selected"' : '') + '>' + powerlist[i][1] + '<\/option>';

	e = E('__wifi_'+t+'_power');
	buf = '<select name="wifi_'+t+'_power" onchange="verifyFields(this, 1)" id = "_wifi_'+t+'_power">' + buf + '<\/select>';
	elem.setInnerHTML(e, buf);
	power_loaded[device] = 1;
	cmdresult = '';
}

function refreshPower(device) {
	var cmd = new XmlHttp();
	cmd.onCompleted = function(text, xml) {
		eval(text);
		displayPower(device);
	}
	cmd.onError = function(x) {
		cmdresult = 'ERROR: ' + x;
		displayPower(device);
	}
	var c = 'iwinfo phy'+device+' txpowerlist';
	cmd.post('shell.cgi', 'action=execute&command=' + escapeCGI(c.replace(/\r/g, '')));
}

function displayChannels(device) {
	var channel = '';
	var channels = [];
	var result = cmdresult.split('\n');

	channels.push(['auto', 'auto']);
	for (var i = 0; i < result.length; i++) {
		if (result[i] !== "") {
			var result2 = result[i].split(/\s+/);
			if ((result2[0] == '*') || (result2[0] == ''))
				result2.shift();

			channel = result2[6].substring(0, result2[6].length - 1);
			channels.push([channel, channel + ' (' + result2[0] + ' ' + result2[4].substring(0, result2[4].length - 1) + ')']);
		}
	}

	t = devices[device][0];
	e = E('_wifi_'+t+'_channel');
	buf = '';
	val = (!channel_loaded[device] || (e.value + '' == '')) ? eval('nvram["wifi_'+t+'_channel"]') : e.value;

	for (i = 0; i < channels.length; ++i)
		buf += '<option value="' + channels[i][0] + '"' + ((channels[i][0] == val) ? ' selected="selected"' : '') + '>' + channels[i][1] + '<\/option>';

	e = E('__wifi_'+t+'_channel');
	buf = '<select name="wifi_'+t+'_channel" onchange="verifyFields(this, 1)" id = "_wifi_'+t+'_channel">' + buf + '<\/select>';
	elem.setInnerHTML(e, buf);
	channel_loaded[device] = 1;
	cmdresult = '';
}

function refreshChannels(device) {
	var cmd = new XmlHttp();
	cmd.onCompleted = function(text, xml) {
		eval(text);
		displayChannels(device);
	}
	cmd.onError = function(x) {
		cmdresult = 'ERROR: ' + x;
		displayChannels(device);
	}
	var c = 'iwinfo phy'+device+' freqlist';
	cmd.post('shell.cgi', 'action=execute&command=' + escapeCGI(c.replace(/\r/g, '')));
}

function refreshModes(device) {
	var m = [];
	t = devices[device][0];

	if (device >= devices.length)
		return;

	var bandselect = document.getElementById('_wifi_'+t+'_band');
	var selected = bandselect.value;

	//htmodes=[['legacy','Legacy'],['n','N'],['ac','AC'],['ax','AX']];
	if (selected == '2g') {
		m.push(['legacy', 'Legacy']);
		if (eval('wireless.'+t+'_2G_ht') == 1) m.push(['n', 'N']);
		if (eval('wireless.'+t+'_2G_he') == 1) m.push(['ax', 'AX']);
	}
	if (selected == '5g') {
		if (eval('wireless.'+t+'_5G_ht') == 1) m.push(['n', 'N']);
		if (eval('wireless.'+t+'_5G_vht') == 1) m.push(['ac', 'AC']);
		if (eval('wireless.'+t+'_5G_he') == 1) m.push(['ax', 'AX']);
		if (eval('wireless.'+t+'_5G_eht') == 1) m.push(['be', 'BE']);
	}
	if (selected == '6g') {
		if (eval('wireless.'+t+'_6G_ht') == 1) m.push(['n', 'N']);
		if (eval('wireless.'+t+'_6G_vht') == 1) m.push(['ac', 'AC']);
		if (eval('wireless.'+t+'_6G_he') == 1) m.push(['ax', 'AX']);
		if (eval('wireless.'+t+'_6G_eht') == 1) m.push(['be', 'BE']);
	}

	e = E('_wifi_'+t+'_mode');
	buf = '';
	val = (!mode_loaded[device] || (e.value + '' == '')) ? eval('nvram["wifi_'+t+'_mode"]') : e.value;

	for (i = 0; i < m.length; ++i)
		buf += '<option value="' + m[i][0] + '"' + ((m[i][0] == val) ? ' selected="selected"' : '') + '>' + m[i][1] + '<\/option>';

	e = E('__wifi_'+t+'_mode');
	buf = '<select name="wifi_'+t+'_mode" onchange="verifyFields(this, 1)" id = "_wifi_'+t+'_mode">' + buf + '<\/select>';
	elem.setInnerHTML(e, buf);
	mode_loaded[device] = 1;
}

function refreshWidths(device) {
	var w = [];
	var bandselect = document.getElementById('_wifi_'+t+'_band');
	var band = bandselect.value;
	var widthselect = document.getElementById('_wifi_'+t+'_mode');
	var mode = widthselect.value;
	t = devices[device][0]+'_'+band.toUpperCase();

	if (mode == 'legacy') {}
	if (mode == 'n') {
		if (eval('wireless.'+t+'_HT20') == 1) w.push(['20', '20 MHz']);
		if (eval('wireless.'+t+'_HT40') == 1) w.push(['40', '40 MHz']);
	}
	if (mode == 'ac') {
		if (eval('wireless.'+t+'_VHT20') == 1) w.push(['20', '20 MHz']);
		if (eval('wireless.'+t+'_VHT40') == 1) w.push(['40', '40 MHz']);
		if (eval('wireless.'+t+'_VHT80') == 1) w.push(['80', '80 MHz']);
		if (eval('wireless.'+t+'_VHT160') == 1) w.push(['160', '160 MHz']);
	}
	if (mode == 'ax') {
		if (eval('wireless.'+t+'_HE20') == 1) w.push(['20', '20 MHz']);
		if (eval('wireless.'+t+'_HE40') == 1) w.push(['40', '40 MHz']);
		if (eval('wireless.'+t+'_HE80') == 1) w.push(['80', '80 MHz']);
		if (eval('wireless.'+t+'_HE160') == 1) w.push(['160', '160 MHz']);
	}
	if (mode == 'be') {
		if (eval('wireless.'+t+'_EHT20') == 1) w.push(['20', '20 MHz']);
		if (eval('wireless.'+t+'_EHT40') == 1) w.push(['40', '40 MHz']);
		if (eval('wireless.'+t+'_EHT80') == 1) w.push(['80', '80 MHz']);
		if (eval('wireless.'+t+'_EHT160') == 1) w.push(['160', '160 MHz']);
		if (eval('wireless.'+t+'_EHT320') == 1) w.push(['320', '320 MHz']);
	}

	u = devices[device][0];
	e = E('_wifi_'+u+'_width');
	buf = '';
	val = (!width_loaded[device] || (e.value + '' == '')) ? eval('nvram["wifi_'+u+'_width"]') : e.value;

	for (i = 0; i < w.length; ++i)
		buf += '<option value="' + w[i][0] + '"' + ((w[i][0] == val) ? ' selected="selected"' : '') + '>' + w[i][1] + '<\/option>';

	e = E('__wifi_'+u+'_width');
	buf = '<select name="wifi_'+u+'_width" onchange="verifyFields(this, 1)" id = "_wifi_'+u+'_width">' + buf + '<\/select>';
	elem.setInnerHTML(e, buf);
	width_loaded[device] = 1;
}

function verifyFields(focused, quiet) {
	var b;

	/* --- visibility --- */

	for (var i = 0; i < devices.length; ++i) {
		refreshModes(i);
		refreshWidths(i);
		refreshChannels(i);
		refreshPower(i);
		refreshCountry(i);
		t = devices[i][0];

		// hide legacy b rates when not a 2G device
		b = E('_f_wifi_'+t+'_brates');
		if ((typeof eval('wireless.phy'+i+'_2G') === 'undefined') || E('_wifi_'+t+'_band').value != '2g') {
			b.disabled = 1;
			PR(b).style.display = 'none';
		} else {
			b.disabled = 0;
			PR(b).style.display = '';
		}

		// hide width when enabling legacy mode for 2G devices
		b = E('_wifi_'+t+'_width');
		if ((E('_wifi_'+t+'_band').value == '2g') && (E('_wifi_'+t+'_mode').value == 'legacy')) {
			b.disabled = 1;
			PR(b).style.display = 'none';
		} else {
			b.disabled = 0;
			PR(b).style.display = '';
		}

		for (var j = 0; j < interfaceCount[i]; ++j) {
			t = 'phy'+i+'iface'+j;

			// Show and Hide fields for ap and sta modes
			b = E('_wifi_'+t+'_bssid');
			c = E('_wifi_'+t+'_network');
			d = E('_f_wifi_'+t+'_hidden');
			e = E('_f_wifi_'+t+'_wmm');
			f = E('_f_wifi_'+t+'_uapsd');
			if (E('_wifi_'+t+'_mode').value == 'ap') {
				b.disabled = 1;
				PR(b).style.display = 'none';

				c.disabled = 0;
				PR(c).style.display = '';
				d.disabled = 0;
				PR(d).style.display = '';
				e.disabled = 0;
				PR(e).style.display = '';

				// UAPSD is visible only if WMM is checked
				if (e.checked) {
					f.disabled = 0;
					PR(f).style.display = '';
				} else {
					f.disabled = 1;
					PR(f).style.display = 'none';
				}

			} else if (E('_wifi_'+t+'_mode').value == 'sta') {
				b.disabled = 0;
				PR(b).style.display = '';

				c.disabled = 1;
				PR(c).style.display = 'none';
				d.disabled = 1;
				PR(d).style.display = 'none';
				e.disabled = 1;
				PR(e).style.display = 'none';
				f.disabled = 1;
				PR(f).style.display = 'none';
			} else if (E('_wifi_'+t+'_mode').value == 'bridge') {
				b.disabled = 0;
				PR(b).style.display = '';

				c.disabled = 0;
				PR(c).style.display = '';
				d.disabled = 1;
				PR(d).style.display = 'none';
				e.disabled = 1;
				PR(e).style.display = 'none';
				f.disabled = 1;
				PR(f).style.display = 'none';
			}


			// show the cipher option only for relevant modes
			b = E('_wifi_'+t+'_cipher');
			if ((E('_wifi_'+t+'_encryption').value == 'psk2') || (E('_wifi_'+t+'_encryption').value == 'psk-mixed') || (E('_wifi_'+t+'_encryption').value == 'psk')) {
				b.disabled = 0;
				PR(b).style.display = '';
			} else {
				b.disabled = 1;
				PR(b).style.display = 'none';
			}

			// show the key (password) field option only for relevant modes
			b = E('_wifi_'+t+'_key');
			c = E('_f_wifi_'+t+'_psk_random1');

			if ((E('_wifi_'+t+'_encryption').value == 'psk2') || (E('_wifi_'+t+'_encryption').value == 'sae') || (E('_wifi_'+t+'_encryption').value == 'sae-mixed') ||
			    (E('_wifi_'+t+'_encryption').value == 'psk-mixed') || (E('_wifi_'+t+'_encryption').value == 'psk')) {
				b.disabled = 0;
				PR(b).style.display = '';
				c.disabled = 0;
				PR(c).style.display = '';
			} else {
				b.disabled = 1;
				PR(b).style.display = 'none';
				c.disabled = 1;
				PR(c).style.display = 'none';
			}

			// show mac filter table for relevant modes
			b = E('table_'+t+'_maclist');

			if ((E('_wifi_'+t+'_macfilter').value == 'allow') || (E('_wifi_'+t+'_macfilter').value == 'deny')) {
				b.disabled = 0;
				PR(b).style.display = '';
			} else {
				b.disabled = 1;
				PR(b).style.display = 'none';
			}

			for (var k = 0; k <= MAX_BRIDGE_ID; ++k) {
				n = (k == 0 ? '' : k);
				if (eval('nvram.lan'+n+'_ifname.length') < 1) {
					E('_wifi_'+t+'_network').options[k].disabled = 1;
				}
			}
		}
	}

	/* --- verify --- */

	var ok = 1
	for (var i = 0; i < devices.length; ++i) {
		for (var j = 0; j < interfaceCount[i]; ++j) {
			t = 'phy'+i+'iface'+j;

			if (E('_f_wifi_'+t+'_enable').checked) {

				if (E('_wifi_'+t+'_essid').value.length == 0 ) {
					ferror.set(E('_wifi_'+t+'_essid'), 'ESSID cannot be empty', quiet || !ok);
					ok = 0;
				} else {
					ferror.clear(E('_wifi_'+t+'_essid'));
				}

				if ((E('_wifi_'+t+'_bssid').value.length > 0 ) && (E('_wifi_'+t+'_mode').value == "sta" ) && (!v_mac(E('_wifi_'+t+'_bssid'),quiet))) {
					ok = 0;
				} else {
					ferror.clear(E('_wifi_'+t+'_bssid'));
				}

				if (((E('_wifi_'+t+'_encryption').value == 'psk2') || (E('_wifi_'+t+'_encryption').value == 'sae') || (E('_wifi_'+t+'_encryption').value == 'sae-mixed') ||
				     (E('_wifi_'+t+'_encryption').value == 'psk-mixed') || (E('_wifi_'+t+'_encryption').value == 'psk')) && (E('_wifi_'+t+'_key').value.length < 8)) {
					ferror.set(E('_wifi_'+t+'_key'), 'Key must be at least 8 characters', quiet || !ok);
					ok = 0;
				} else {
					ferror.clear(E('_wifi_'+t+'_key'));
				}
			} else {
				ferror.clear(E('_wifi_'+t+'_essid'));
				ferror.clear(E('_wifi_'+t+'_bssid'));
				ferror.clear(E('_wifi_'+t+'_key'));
			}
		}
	}
	changed = 1;
	return ok;
}

function tabSelect(name) {
	tabHigh(name);
	for (var i = 0; i < devices.length; ++i) {
		elem.display(devices[i][0]+'-tab', (name == devices[i][0]));
		elem.display('phy'+i+'-interface-tab', (name == devices[i][0]));
	}
	cookie.set('wireless_device', name);
}

function deviceSectSelect(tab, deviceSection) {
	for (var i = 0; i < deviceSections.length; ++i) {
		if (deviceSection == deviceSections[i][0]) {
			elem.addClass(devices[tab][0]+'-'+deviceSections[i][0]+'-tab', 'active');
			elem.display(devices[tab][0]+'-'+deviceSections[i][0], true);
		} else {
			elem.removeClass(devices[tab][0]+'-'+deviceSections[i][0]+'-tab', 'active');
			elem.display(devices[tab][0]+'-'+deviceSections[i][0], false);
		}
	}
	cookie.set('wireless'+tab+'_deviceSection', deviceSection);
}

function interfaceSelect(tab, my_interface) {
	for (var i = 0; i < interfaces.length; ++i) {
		if (my_interface == interfaces[i][0]) {
			elem.addClass(my_interface+'-tab', 'active');
			elem.display(my_interface, true);
		} else {
			if (interfaces[i][0].charAt(3) == tab) {
				elem.removeClass(interfaces[i][0]+'-tab', 'active');
				elem.display(interfaces[i][0], false);
			}
		}
	}
	cookie.set('wireless'+tab+'_interface', my_interface);
}

function interfaceSectSelect(device, interface_num, category) {
	for (var i = 0; i < devices.length; ++i) {
		for (var j = 0; j < interfaceCount[i]; ++j) {
			for (var k = 0; k < interfaceSections.length; ++k) {
				if ((i == device) && (j == interface_num)) {
					if (interfaceSections[k][0] == interfaceSections[category][0]) {
						elem.addClass('phy'+device+'iface'+interface_num+'-'+interfaceSections[category][0]+'-tab', 'active');
						elem.display('phy'+device+'iface'+interface_num+'-'+interfaceSections[category][0], true);
					} else {
						elem.removeClass('phy'+device+'iface'+interface_num+'-'+interfaceSections[k][0]+'-tab', 'active');
						elem.display('phy'+device+'iface'+interface_num+'-'+interfaceSections[k][0], false);
					}
				}
			}
		}
	}
	cookie.set('wireless'+device+'_interface'+interface_num, category);
}

function addInterface(device) {
	if (changed == 0 || (changed == 1 && confirm("Any unsaved changes will be lost, continue?"))) {
		var fom = E('t_iface');
		for (var i = 0; i < devices.length; i++) {
			if (i == device) {
				fom['wifi_phy'+i+'_ifaces'].value = +eval('nvram["wifi_phy"+i+"_ifaces"]') + +1;
				cookie.set('wireless'+device+'_interface', 'phy'+device+'iface'+(Number(nvram["wifi_phy"+i+"_ifaces"])));
			} else {
				fom['wifi_phy'+i+'_ifaces'].value = eval('nvram["wifi_phy"+i+"_ifaces"]');
			}
		}
		form.submit(fom, 0);
	}
}

function removeInterface(device) {
	if (changed == 0 || (changed == 1 && confirm("Any unsaved changes will be lost, continue?"))) {
		var fom = E('t_iface');
		for (var i = 0; i < devices.length; i++) {
			if (i == device) {
				fom['wifi_phy'+i+'_ifaces'].value = +eval('nvram["wifi_phy"+i+"_ifaces"]') - +1;
				cookie.set('wireless'+device+'_interface', 'phy'+device+'iface'+(Number(nvram["wifi_phy"+i+"_ifaces"]) - 2));
			} else {
				fom['wifi_phy'+i+'_ifaces'].value = eval('nvram["wifi_phy"+i+"_ifaces"]');
			}
		}
		form.submit(fom, 0);
	}
}

function save_pre() {
	if (!verifyFields(null, 0))
		return 0;
	return 1;
}

function save(nomsg) {
	if (!save_pre()) return;
	if (!nomsg) show(); /* update '_service' field first */
	var fom = E('t_fom');
	var sta_list = "";
	for (var i = 0; i < devices.length; i++) {
		t = devices[i][0];
		E('wifi_'+t+'_brates').value = E('_f_wifi_'+t+'_brates').checked ? 1 : "";
		E('wifi_'+t+'_noscan').value = E('_f_wifi_'+t+'_noscan').checked ? 1 : "";
		for (var j = 0; j < interfaceCount[i]; j++) {
			t = 'phy'+i+'iface'+j;
			E('wifi_'+t+'_enable').value = E('_f_wifi_'+t+'_enable').checked ? 1 : 0;
			E('wifi_'+t+'_hidden').value = E('_f_wifi_'+t+'_hidden').checked ? 1 : "";
			E('wifi_'+t+'_wmm').value = E('_f_wifi_'+t+'_wmm').checked ? 1 : 0;
			E('wifi_'+t+'_uapsd').value = E('_f_wifi_'+t+'_uapsd').checked ? 1 : 0;
			E('wifi_'+t+'_isolate').value = E('_f_wifi_'+t+'_isolate').checked ? 1 : "";
			E('wifi_'+t+'_br_isolate').value = E('_f_wifi_'+t+'_br_isolate').checked ? 1 : "";

			var macdata = macTables[i][j].getAllData();
			var macs = '';
			for (k = 0; k < macdata.length; k++) {
				macs += macdata[k].join('<')+'>';
			}
			E('wifi_'+t+'_maclist').value = macs;

			if (((E('_wifi_'+t+'_mode').value == "sta") || (E('_wifi_'+t+'_mode').value == "bridge")) && (E('_f_wifi_'+t+'_enable').checked)) {
				if (E('_wifi_'+t+'_ifname').value != "") {
					sta_list += ' ' + E('_wifi_'+t+'_ifname').value;
				} else {
					sta_list += ' phy'+i+'-sta'+j;
				}
				E('wifi_sta_list').value = sta_list.trim();
			}
		}
	}

	// We don't have a gui element for these so set the hidden element to it's nvram value
	// If we renamed/destroyed a station that was used by a wan we will modify its value below
	E('wan_sta').value  = nvram['wan_sta'];
	E('wan2_sta').value = nvram['wan2_sta'];
	E('wan3_sta').value = nvram['wan3_sta'];
	E('wan4_sta').value = nvram['wan4_sta'];

	if (E('wifi_sta_list').value.length > 0) {

		// check if we've renamed/destroyed a station that a wan was using
		// if so, set wanX_sta = ""
		const wan_list = ["wan_sta", "wan2_sta", "wan3_sta", "wan4_sta"];
		const station_list = E('wifi_sta_list').value.split(" ");

		for (var i = 0; i < wan_list.length; i++) {
			if (nvram[wan_list[i]].length > 0) {
				for (var j = 0; j < station_list.length; j++) {
					if (nvram[wan_list[i]] == station_list[j]) {
						break;
					}
					if (j == (station_list.length -1)) {
						E(wan_list[i]).value = "";
					}
				}
			}
		}

		fom._service.value = '*';
	} else {
		fom._service.value = 'wifi-restart';
	}
	form.submit(fom, 1);
	changed = 0;
}

function earlyInit() {
	tabSelect(cookie.get('wireless_device') || devices[0][0]);
	for (var i = 0; i < devices.length; ++i) {
		deviceSectSelect(i, cookie.get('wireless'+i+'_deviceSection') || deviceSections[0][0]);
		interfaceSelect(i, cookie.get('wireless'+i+'_interface') || 'phy'+i+'iface0');
		for (var j = 0; j < interfaceCount[i]; ++j) {

			macTables[i].push(new MacGrid());

			macTables[i][j].init('table_phy'+i+'iface'+j+'_maclist','sort',0,[
				{ type: 'text', maxlen: 17 },
				{ type: 'text', maxlen: 48 }]);

			macTables[i][j].headerSet(['MAC Address','Description']);

			var nv = nvram['wifi_phy'+i+'iface'+j+'_maclist'].split('>');
			for (var m = 0; m < nv.length; m++) {
				var t = nv[m].split('<');
					if (t.length == 2) {
					macTables[i][j].insertData(-1, t);
				}
			}

			macTables[i][j].showNewEditor();
			macTables[i][j].resetNewEditor();

			for (var k = 0; k < interfaceSections.length; ++k) {
				interfaceSectSelect(i, j, cookie.get('wireless'+i+'_interface'+j, k) || 0);
			}
		}
	}
	//	show();
	verifyFields(null, 1);
	changed = 0;
	cookie.set('addmac', '', 0);
}

function init() {
        if (((c = cookie.get(cprefix + '_notes_vis')) != null) && (c == '1'))
                toggleVisibility(cprefix, "notes");

	up.initPage(250, 5);
}

function MacGrid() {return this;}
MacGrid.prototype = new TomatoGrid;

MacGrid.prototype.fieldValuesToData = function(row) {
	var f = fields.getAll(row);
	var r = [f[0].value, f[1].value];
	return r;
}

MacGrid.prototype.verifyFields = function(row, quiet) {
	var f;
	f = fields.getAll(row);

	return v_mac(f[0], quiet) && v_nodelim(f[1], quiet, 'Description', 1);
}

MacGrid.prototype.resetNewEditor = function() {
	var f, c, n;

	f = fields.getAll(this.newEditor);
	ferror.clearAll(f);

	if ((c = cookie.get('addmac')) != null) {
		c = c.split(',');
		if (c.length == 2) {
			f[0].value = c[0];
			f[1].value = c[1];
			return;
		}
	}

	f[0].value = mac_null;
	f[1].value = '';
}

</script>
</head>

<body onload="init()">
<form id="t_iface" method="post" action="tomato.cgi">
<input type="hidden" name="_nextpage" value="basic-wireless.asp">
<input type="hidden" name="_nextwait" value="1">
<script>
for (i = 0; i < devices.length; ++i) {
	W('<input type="hidden" id="wifi_phy'+i+'_ifaces" name="wifi_phy'+i+'_ifaces">');
}
</script>
</form>

<form id="t_fom" method="post" action="tomato.cgi">
<table id="container">
<tr><td colspan="2" id="header">
	<div class="title"><a href="/">Tomato64</a></div>
	<div class="version">Version <% version(); %> on <% nv("t_model_name"); %></div>
</td></tr>
<tr id="body"><td id="navi"><script>navi()</script></td>
<td id="content">
<div id="ident"><% ident(); %> | <script>wikiLink();</script></div>

<!-- / / / -->

<input type="hidden" name="_nextpage" value="basic-wireless.asp">
<input type="hidden" name="_service" value="">
<input type="hidden" name="_nofootermsg">
<input type="hidden" name="wifi_sta_list" id="wifi_sta_list">
<input type="hidden" name="wan_sta" id="wan_sta">
<input type="hidden" name="wan2_sta" id="wan2_sta">
<input type="hidden" name="wan3_sta" id="wan3_sta">
<input type="hidden" name="wan4_sta" id="wan4_sta">

<!-- / / / -->

<div class="section-title">Device Configuration</div>
<div class="section">
<script>

tabCreate.apply(this, devices);

for (i = 0; i < devices.length; ++i) {
	t = devices[i][0];
	W('<div id="'+t+'-tab">');
	W('<input type="hidden" id="wifi_'+t+'_brates" name="wifi_'+t+'_brates">');
	W('<input type="hidden" id="wifi_'+t+'_noscan" name="wifi_'+t+'_noscan">');

	W('<ul class="tabs">');
	for (j = 0; j < deviceSections.length; j++) {
		W('<li><a href="javascript:deviceSectSelect('+i+',\''+deviceSections[j][0]+'\')" id="'+t+'-'+deviceSections[j][0]+'-tab">'+deviceSections[j][1]+'<\/a><\/li>');
	}
	W('<\/ul><div class="tabs-bottom"><\/div>');

	W('<div id="'+t+'-general">');

	var bands = [];

	if (eval('wireless.phy'+i+'_2G') == 1) {
		bands.push(['2g', '2.4 GHz']);
	}
	if (eval('wireless.phy'+i+'_5G') == 1) {
		bands.push(['5g', '5 GHz']);
	}
	if (eval('wireless.phy'+i+'_6G') == 1) {
		bands.push(['6g', '6 GHz']);
	}

	createFieldTable('', [
		{ title: 'Band', name: 'wifi_'+t+'_band', type: 'select', options: bands, value: nvram['wifi_'+t+'_band'] },
		{ title: 'Mode', name: 'wifi_'+t+'_mode', type: 'select',
			value: nvram['wifi_'+t+'_mode'], options: [], prefix: '<span id="__wifi_'+t+'_mode">', suffix: '<\/span>' },
		{ title: 'Channel', name: 'wifi_'+t+'_channel', type: 'select',
			value: nvram['wifi_'+t+'_channel'], options: [], prefix: '<span id="__wifi_'+t+'_channel">', suffix: '<\/span>' },
		{ title: 'Width', name: 'wifi_'+t+'_width', type: 'select',
			value: nvram['wifi_'+t+'_width'], options: [], prefix: '<span id="__wifi_'+t+'_width">', suffix: '<\/span>'},
		{ title: 'Allow legacy 802.11b rates', name: 'f_wifi_'+t+'_brates', type: 'checkbox', value: nvram['wifi_'+t+'_brates'] == 1 },
		{ title: 'Maximum transmit power', name: 'wifi_'+t+'_power', type: 'select',
			value: nvram['wifi_'+t+'_power'], options: [], prefix: '<span id="__wifi_'+t+'_power">', suffix: '<\/span>' },
		{ title: 'Country code', name: 'wifi_'+t+'_country', type: 'select',
			value: nvram['wifi_'+t+'_country'], options: [], prefix: '<span id="__wifi_'+t+'_country">', suffix: '<\/span>' }
	]);
	W('<\/div>');

	W('<div id="'+t+'-advanced">');
	createFieldTable('', [
		{ title: 'Force 40MHz mode', name: 'f_wifi_'+t+'_noscan', type: 'checkbox', value: nvram['wifi_'+t+'_noscan'] == 1 }
	]);
	W('<\/div>');

	W('<\/div>');
}
</script>
</div>

<div class="section-title">Interface Configuration</div>
<div class="section">
<script>

for (var i = 0; i < devices.length; i++) {
	W('<div id="phy'+i+'-interface-tab">');
	W('<ul id="tabs">');
	for (var j = 0; j < interfaceCount[i]; j++) {
		interfaces.push([devices[i][0]+'iface'+j, devices[i][0]+'iface'+j]);
		W('<li><a href="javascript:interfaceSelect('+i+',\''+devices[i][0]+'iface'+j+'\')" id="'+devices[i][0]+'iface'+j+'-tab">phy'+i+' interface '+j+'<\/a><\/li>');
	}

	if (eval('nvram["wifi_phy"+i+"_ifaces"]') < 16) {
		W('<li><a href="javascript:addInterface('+i+')">+<\/a><\/li>');
	}

	W('<\/ul><div id="tabs-bottom"></div>');
	for (var j = 0; j < interfaceCount[i]; j++) {
		W('<div id="phy'+i+'iface'+j+'">');

		t = 'phy'+i+'iface'+j;
		W('<input type="hidden" id="wifi_'+t+'_enable" name="wifi_'+t+'_enable">');
		W('<input type="hidden" id="wifi_'+t+'_hidden" name="wifi_'+t+'_hidden">');
		W('<input type="hidden" id="wifi_'+t+'_wmm" name="wifi_'+t+'_wmm">');
		W('<input type="hidden" id="wifi_'+t+'_uapsd" name="wifi_'+t+'_uapsd">');
		W('<input type="hidden" id="wifi_'+t+'_isolate" name="wifi_'+t+'_isolate">');
		W('<input type="hidden" id="wifi_'+t+'_br_isolate" name="wifi_'+t+'_br_isolate">');
		W('<input type="hidden" id="wifi_'+t+'_maclist" name="wifi_'+t+'_maclist">');

		W('<ul class="tabs">');
		for (var k = 0; k < interfaceSections.length; k++) {
			var u = devices[i][0]+'iface'+j+'-'+interfaceSections[k][0];
			W('<li><a href="javascript:interfaceSectSelect('+i+','+j+','+k+')" id="'+u+'-tab">'+interfaceSections[k][1]+'<\/a><\/li>');
		}
		W('<\/ul><div class="tabs-bottom"><\/div>');
		W('<div id="'+t+'-general">');

		createFieldTable('', [
			{ title: 'Enable', name: 'f_wifi_'+t+'_enable', type: 'checkbox', value: nvram['wifi_'+t+'_enable'] == 1 },
			{ title: 'Mode', name: 'wifi_'+t+'_mode', type: 'select', options: [['ap', 'Access Point'],['sta','Client'],['bridge','Wireless Ethernet Bridge']], value: nvram['wifi_'+t+'_mode'] },
			{ title: 'ESSID', name: 'wifi_'+t+'_essid', type: 'text', maxlen: 32, size: 34, value: nvram['wifi_'+t+'_essid'] },
			{ title: 'BSSID', name: 'wifi_'+t+'_bssid', type: 'text', maxlen: 17, size: 34, value: nvram['wifi_'+t+'_bssid'] },
			{ title: 'Network', name: 'wifi_'+t+'_network', type: 'select', options: networks, value: nvram['wifi_'+t+'_network'] },
			{ title: 'Hide ESSID', name: 'f_wifi_'+t+'_hidden', type: 'checkbox', value: nvram['wifi_'+t+'_hidden'] == 1 },
			{ title: 'WMM Mode', name: 'f_wifi_'+t+'_wmm', type: 'checkbox', value: nvram['wifi_'+t+'_wmm'] == 1 },
			{ title: 'U-APSD', indent: 2, name: 'f_wifi_'+t+'_uapsd', type: 'checkbox', value: nvram['wifi_'+t+'_uapsd'] == 1, suffix: '&nbsp; <small>Unscheduled Automatic Power Save Delivery<\/small>' }
		]);
		W('<\/div>');

		W('<div id="'+t+'-security">');
		createFieldTable('', [
			{ title: 'Encryption', name: 'wifi_'+t+'_encryption', type: 'select', options: security_modes, value: nvram['wifi_'+t+'_encryption'] },
			{ title: 'Cipher', name: 'wifi_'+t+'_cipher', type: 'select', options: ciphers, value: nvram['wifi_'+t+'_cipher'] },
			{ title: 'Key', name: 'wifi_'+t+'_key', type: 'password', maxlen: 64, size: 66, peekaboo: 1,
				suffix: ' <input type="button" id="_f_wifi_'+t+'_psk_random1" value="Random" onclick="random_psk(\'_wifi_'+t+'_key\')">', value: nvram['wifi_'+t+'_key'] }
		]);
		W('<\/div>');

		W('<div id="'+t+'-filter">');
		createFieldTable('', [
			{ title: 'MAC Address Filter', name: 'wifi_'+t+'_macfilter', type: 'select', options: [['', 'Disable'],['allow', 'Allow listed only'],['deny', 'Allow all except listed']], value: nvram['wifi_'+t+'_macfilter'] },
			{ title: 'MAC-List', suffix: '<div class="tomato-grid" id="table_'+t+'_maclist"><\/div>' },
		]);
		W('<\/div>');

		W('<div id="'+t+'-advanced">');
		createFieldTable('', [
			{ title: 'Isolate Clients', name: 'f_wifi_'+t+'_isolate', type: 'checkbox', value: nvram['wifi_'+t+'_isolate'] == 1 },
			{ title: 'Isolate Bridge Port', name: 'f_wifi_'+t+'_br_isolate', type: 'checkbox', value: nvram['wifi_'+t+'_br_isolate'] == 1 },
			{ title: 'Interface name', name: 'wifi_'+t+'_ifname', type: 'text', maxlen: 15, size: 17, value: nvram['wifi_'+t+'_ifname'] }
		]);
		W('<\/div>');

		if (j == interfaceCount[i] - 1) {
			W('<br>');
			W('<input type="button" style="float: right;margin-right: 4px" value="Remove Interface" onclick="removeInterface('+i+')">');
		}
		W('<\/div>');
	}
	W('<\/div>');
}
</script>
</div>

<!-- / / / -->

<div class="section-title">Notes <small><i><a href="javascript:toggleVisibility(cprefix,'notes');" id="toggleLink-notes"><span id="sesdiv_notes_showhide">(Show)</span></a></i></small></div>
<div class="section" id="sesdiv_notes" style="display:none">
	<i>Device Configuration:</i><br>
	<ul>
		<li><b>General Setup</b></li>
		<ul>
			<li><b>Band</b> - Frequency this radio will operate on.</li>
			<li><b>Mode</b> - Wifi standard to use.</li>
			<ul>
				<li><b>AX</b> - Wifi 6 (802.11ax)</li>
				<li><b>AC</b> - Wifi 5 (802.11ac)</li>
				<li><b>N</b> - Wifi 4 (802.11n)</li>
				<li><b>Legacy</b> - 802.11b/g</li>
			</ul>
			<li><b>Channel</b> - Specifies the wireless channel. “auto” defaults to the lowest available channel.</li>
			<li><b>Width</b> - The width is the range of frequencies i.e. how broad the signal is for transferring data.</li>
			<li><b>Allow legacy 802.11b rates</b> - Enable compatibility for very old devices running up to 11 Mbps. Can reduce network efficiency and lower throughput for all devices.</li>
			<li><b>Maximum transmit power</b> - The highest radio signal strength the Wi-Fi interface can use to transmit.</li>
			<li><b>Country code</b> - Specifies the Country Code/Regulatory Domain (e.g. US, EU) for the Wi-Fi radio. Affects available channels and transmit powers to comply with local regulations.</li>
		</ul>
		<li><b>Advanced Settings</b></li>
		<ul>
			<li><b>Force 40MHz mode </b> - Disables scanning and forces use of the specified width. Applicable to 80 & 160 widths as well.</li>
		</ul>
	</ul>
	<br>
	<i>Interface Configuration:</i><br>
	<ul>
		<li><b>General Setup</b></li>
		<ul>
			<li><b>Enable</b> - Enables this interface.</li>
			<li><b>Mode</b> - Currently 'Access Point' and 'Client' are supported.</li>
			<ul>
				<li><b>Access Point</b> - Creates a wireless network for devices to join.</li>
				<li><b>Client</b> - Connects to another wireless network. This connections can then be shared wired or wirelessly.</li>
			</ul>
			<li><b>ESSID</b> - The broadcasted SSID of the wireless network.</li>
				<ul>
					<li><b>Client</b> - In client mode this is the name of the SSID you want to connect to.</li>
				</ul>
			<li><b>BSSID</b> - Available in Client mode and is optional. This specifies the MAC address of the Wireless SSID network you want to connect to. This is useful for situations where there may be multiple APs with the same SSID and you want to connect to a specific one.</li>
			<li><b>Network</b> - Which network/bridge this interface will join.</li>
			<li><b>Hide ESSID</b> - Disables the broadcasting of beacon frames if checked and hides the ESSID.</li>
			<li><b>WMM Mode</b> - Enables WMM. Where Wi-Fi Multimedia (WMM) Mode QoS is disabled, clients may be limited to 802.11a/802.11g rates. Required for 802.11n/802.11ac/802.11ax.</li>
			<li><b>U-APSD</b> - Allows wireless devices to save battery by sleeping longer and waking only when data is ready to transmit.</li>
		</ul>
		<li><b>Wireless Security</b></li>
		<ul>
			<li><b>Encryption</b> - Wireless encryption method.</li>
			<li><b>Cipher</b> - </li>
			<li><b>Key</b> - The Wifi Password. Must be 8-63 characters long.</li>
		</ul>
		<li><b>MAC-Filter</b></li>
		<ul>
			<li><b>Allow listed only</b> - Create a whitelist of allowed devices.</li>
			<li><b>Allow all except listed</b> - Create a blacklist of disallowed devices</li>
		</ul>
		<li><b>Advanced Settings</b></li>
		<ul>
			<li><b>Isolate Clients</b> - Isolates wireless clients from each other, </li>
			<li><b>Isolate Bridge Port</b> - Isolates wireless clients from each other on the AP's bridge. (i.e. 2.4ghz and 5ghz radios on the same AP.)</li>
			<li><b>Interface name</b> - Specifies a custom name for the Wi-Fi interface, which is otherwise automatically named.</li>
		</ul>
	</ul>
	<br>
	<i>Other relevant notes/hints:</i><br>
	<ul>
		<li>The Channel list and TX Power are dependent on your Country code. They are only updated accordingly once you</li>
		<ul>
			<li>make your country selection</li>
			<li>enable at least one interface</li>
			<li>click Save</li>
			Afterwards you may see a new selection of option to choose from.
		</ul>
		<li><b>5GHz Radio:</b> Before attempting to setup the 5Ghz radio, please follow the steps in the previous point. It can sometimes be difficult to find the right settings to bring the 5Ghz radio up, particularly if you want to use 160Mhz width. To see your country's regulations, run the following over ssh or in Tools -> System Commands</li>
			<ul>
				<li><b>iwinfo phy1-ap0 freqlist</b></li>
			</ul>
			Avoid channels with the "NO_IR" code, and channels with "NO_160MHZ" if wanting to use 160MHz widths. Also keep an eye in the logs for other issues/errors when starting the 5GHz radio.
	</ul>
</div>

<!-- / / / -->

<div id="footer">
	<span id="footer-msg"></span>
	<input type="button" value="Save" id="save-button" onclick="save()">
	<input type="button" value="Cancel" id="cancel-button" onclick="reloadPage();">
</div>

</td></tr>
</table>
</form>
<script>earlyInit();</script>
</body>
</html>
