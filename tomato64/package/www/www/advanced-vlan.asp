<!DOCTYPE html>
<!--
	FreshTomato GUI
	Copyright (C) 2018 - 2026 pedro
	https://freshtomato.org/

	For use with FreshTomato Firmware only.
	No part of this file may be used without permission.
-->
<html lang="en-GB">
<head>
<meta http-equiv="content-type" content="text/html;charset=utf-8">
<meta name="robots" content="noindex,nofollow">
<title>[<% ident(); %>] Advanced: VLAN</title>
<link rel="stylesheet" type="text/css" href="tomato.css?rel=<% version(); %>">
<% css(); %>
<script src="isup.jsx?_http_id=<% nv(http_id); %>"></script>
<script src="tomato.js?rel=<% version(); %>"></script>
<script src="wireless.jsx?_http_id=<% nv(http_id); %>"></script>
<script src="interfaces.js?rel=<% version(); %>"></script>
/* TOMATO64-SKIP-BEGIN */
<script src="ethernet-icon.js?rel=<% version(); %>"></script>
<script src="vpn.js?rel=<% version(); %>"></script>
/* TOMATO64-SKIP-END */

<script>
/* TOMATO64-BEGIN */
/* Tomato64 doesn't handle wifi on this page */
wl_ifaces=[];
/* TOMATO64-END */

/* TOMATO64-SKIP-BEGIN */
//	<% nvram ("t_model_name,vlan0ports,vlan1ports,vlan2ports,vlan3ports,vlan4ports,vlan5ports,vlan6ports,vlan7ports,vlan8ports,vlan9ports,vlan10ports,vlan11ports,vlan12ports,vlan13ports,vlan14ports,vlan15ports,vlan0hwname,vlan1hwname,vlan2hwname,vlan3hwname,vlan4hwname,vlan5hwname,vlan6hwname,vlan7hwname,vlan8hwname,vlan9hwname,vlan10hwname,vlan11hwname,vlan12hwname,vlan13hwname,vlan14hwname,vlan15hwname,wan_ifnameX,manual_boot_nv,boardtype,boardflags,lan_ifname,lan_ifnames,vlan0tag,vlan0vid,vlan1vid,vlan2vid,vlan3vid,vlan4vid,vlan5vid,vlan6vid,vlan7vid,vlan8vid,vlan9vid,vlan10vid,vlan11vid,vlan12vid,vlan13vid,vlan14vid,vlan15vid,model,wl_ssid,wl_radio,wl_net_mode,wl_nband,boardnum,boardrev,trunk_vlan_so,mwan_num,eth_desc");%>
/* TOMATO64-SKIP-END */
/* TOMATO64-BEGIN */

//	<% nvram ("t_model_name,nics,vlan0ports,vlan1ports,vlan2ports,vlan3ports,vlan4ports,vlan5ports,vlan6ports,vlan7ports,vlan8ports,vlan9ports,vlan10ports,vlan11ports,vlan12ports,vlan13ports,vlan14ports,vlan15ports,vlan0hwname,vlan1hwname,vlan2hwname,vlan3hwname,vlan4hwname,vlan5hwname,vlan6hwname,vlan7hwname,vlan8hwname,vlan9hwname,vlan10hwname,vlan11hwname,vlan12hwname,vlan13hwname,vlan14hwname,vlan15hwname,wan_ifnameX,wan_ifnameX_vlan,manual_boot_nv,boardtype,boardflags,lan_ifname,lan_ifnames,lan_ifnames_vlan,vlan0vid,vlan1vid,vlan2vid,vlan3vid,vlan4vid,vlan5vid,vlan6vid,vlan7vid,vlan8vid,vlan9vid,vlan10vid,vlan11vid,vlan12vid,vlan13vid,vlan14vid,vlan15vid,model,wl_ssid,wl_radio,wl_net_mode,wl_nband");%>
/* TOMATO64-END */

var cprefix = 'advanced_vlan';
var port_vlan_supported = 0;
/* MIPSR2P-NO-BEGIN */
var trunk_vlan_supported = 0; /* Disable on all routers */
/* MIPSR2P-NO-END */
/* MIPSR2P-BEGIN */
var trunk_vlan_supported = 1; /* Enable on all routers */
/* MIPSR2P-END */

var unknown_router = 0;

var VLAN_COUNT = MAX_VLAN_ID + 1;
var VLAN_OFFSET_MAX = 4095 - MAX_VLAN_ID;

var VLAN_BRIDGE_NONE = 1;
var VLAN_BRIDGE_WAN0 = 2;
var VLAN_BRIDGE_LAN0 = 3;
var VLAN_BRIDGE_WAN1 = VLAN_BRIDGE_LAN0 + MAX_BRIDGE_ID + 1;
var WLAN_BRIDGE_NONE = MAX_BRIDGE_ID + 1;

function lanPrefix(i) {
	return 'lan'+(i ? i : '');
}

function wanPrefix(i) {
	var n = i + 1;
	return 'wan'+((n > 1) ? n : '');
}

function vlanBridgeLan(i) {
	return VLAN_BRIDGE_LAN0 + i;
}

function vlanBridgeWan(i) {
	return i ? (VLAN_BRIDGE_WAN1 + i - 1) : VLAN_BRIDGE_WAN0;
}

var vlanBridgeOptions = [[VLAN_BRIDGE_NONE, 'none'], [VLAN_BRIDGE_WAN0, 'WAN0']];
var vlanBridgeNames = {};
vlanBridgeNames[VLAN_BRIDGE_NONE] = 'none';
vlanBridgeNames[VLAN_BRIDGE_WAN0] = 'WAN0';

for (var i = 0; i <= MAX_BRIDGE_ID; ++i) {
	var label = 'LAN'+i+' (br'+i+')';
	var value = vlanBridgeLan(i);
	vlanBridgeOptions.push([value, label]);
	vlanBridgeNames[value] = label;
}

for (var i = 1; i < MAXWAN_NUM; ++i) {
	var label = 'WAN'+i;
	var value = vlanBridgeWan(i);
	vlanBridgeOptions.push([value, label]);
	vlanBridgeNames[value] = label;
}

var wlanBridgeOptions = [];
for (var i = 0; i <= MAX_BRIDGE_ID; ++i)
	wlanBridgeOptions.push([i, 'LAN'+i+' (br'+i+')']);
wlanBridgeOptions.push([WLAN_BRIDGE_NONE, 'none']);

/* TOMATO64-SKIP-BEGIN */
/* caption/rendering is handled by ethernet-icon.js (renderEthIcon)
 * keep the VLAN page markup minimal and call into renderEthIcon() from show()
 */
function show() {
	var state = [];
	var port = etherstates.port0;
	if (port == 'disabled')
		return 0;

	for (var i = 0 ; i <= MAX_PORT_ID ; i++) {
		var displayIndex = displayPortIndex(i);
		port = etherstates['port'+displayIndex];
		if (port === undefined) continue;
		state = _ethstates(port);
		var p = (state[0] || '').split('_');
		var sp = p[1] || '';
		var du = p[2] || '';
		var spn = parseInt(sp, 10);
		if (!isFinite(spn) || (spn <= 0)) spn = 0;
		sp = ''+spn;
		du = (du || '').toUpperCase();
		if ((du !== 'FD') && (du !== 'HD')) du = 'HD';
		var cap = portCaption(displayIndex, 1); /* shorten */
		var o = E('ethsvg_'+i);
		renderEthIcon(o, sp, du, cap);
		if (o)
			o.title = state[1] ? state[1] : cap;
	}
}
/* TOMATO64-SKIP-END */
/* TOMATO64-BEGIN */

function show() {
	var state = [];
	var port = etherstates.port0;
	if (port == 'disabled')
		return 0;

	for (var i = 0 ; i <= MAX_PORT_ID ; i++) {
		port = etherstates['port'+i];
		state = _ethstates(port);
		elem.setInnerHTML('vport_'+i, '<img src="'+state[0]+'.gif" id="'+state[0]+'_'+i+'" title="'+state[1]+'" alt="">');
	}
}
/* TOMATO64-END */

/* does not seem to be strictly necessary for boardflags as it's supposed to be a bitmap */
nvram['boardflags'] = ((nvram['boardflags'].toLowerCase().indexOf('0x') != -1) ? '0x' : '')+String('0000'+((nvram['boardflags'].toLowerCase()).replace('0x',''))).slice(-4);
/* but the contents of router/shared/id.c seem to indicate string formatting/padding might be required for some models as we check if strings match */
nvram['boardtype'] = ((nvram['boardtype'].toLowerCase().indexOf('0x') != -1) ? '0x' : '')+String('0000'+((nvram['boardtype'].toLowerCase()).replace('0x',''))).slice(-4);

/* see https://www.dd-wrt.com/wiki/index.php/Hardware#Boardflags and router/shared/id.c */
if (nvram['boardflags'] & 0x0100) /* BFL_ENETVLAN = this board has vlan capability */
	port_vlan_supported = 1;

/* MIPSR2P-BEGIN */
switch (nvram['t_model_name']) {
/* BCMARM-BEGIN */
	case 'vlan-testid0':
	case 'Asus RT-AC56U':
	case 'Asus RT-AC56S':
	case 'Buffalo WZR-1750DHP':
	case 'D-Link DIR868L':
	case 'Linksys EA6200':
	case 'Linksys EA6350v1':
	case 'Linksys EA6350v2':
	case 'Cisco Linksys EA6400':
	case 'Cisco Linksys EA6500v2':
	case 'Cisco Linksys EA6700':
	case 'Netgear R7900':
	case 'Netgear R8000':
		COL_P0N = '4';
		COL_P1N = '0';
		COL_P2N = '1';
		COL_P3N = '2';
		COL_P4N = '3';
		break;
	case 'vlan-testid1':
	case 'Tenda AC18':
	case 'Belkin F9K1113v2':
	case 'Asus RT-N18U':
	case 'Asus RT-N66U C1':
	case 'Asus RT-AC1750 B1':
	case 'Asus RT-AC66U B1':
	case 'Asus RT-AC67U':
	case 'Asus RT-AC68R/U':
	case 'Asus RT-AC68P/U B1':
	case 'Asus DSL-AC68U':
	case 'Asus RT-AC68U C1':
	case 'Asus RT-AC68U B2':
	case 'Asus RT-AC68U V3':
	case 'Asus RT-AC1900U':
	case 'Asus RT-AC1900P':
	case 'Huawei WS880':
	case 'Linksys EA6900':
	case 'Netgear R6400':
	case 'Netgear R6400v2':
	case 'Netgear R6700v1':
	case 'Netgear R6700v3':
	case 'Netgear R6900':
	case 'Netgear R7000':
	case 'Netgear EX6200':
	case 'Netgear EX7000':
	case 'Netgear XR300':
	case 'Asus RT-AC5300':
/* TOMATO64-BEGIN */
	case 'x86_64':
	case 'ARM64 SystemReady':
	case 'GL.iNet GL-MT6000':
	case 'GL.iNet GL-MT3600BE':
	case 'Banana Pi BPI-R3':
	case 'Banana Pi BPI-R3 Mini':
	case 'Raspberry Pi 4 Model B':
	case 'FriendlyElec NanoPi R6S':
	case 'FriendlyElec NanoPi R5S':
	case 'FriendlyElec NanoPi R76S':
	case 'Asus RT-AC56U (BCM4708)':
	case 'Asus RT-AC68U (BCM4708)':
	case 'Buffalo WZR-1750DHP (BCM4708)':
	case 'Linksys EA6300 V1':
	case 'Linksys EA6500 V2':
	case 'Netgear R6250 V1 (BCM4708)':
	case 'Netgear R6300 V2 (BCM4708)':
	case 'Asus RT-N18U (BCM47081)':
	case 'Buffalo WZR-600DHP2 (BCM47081)':
	case 'Buffalo WZR-900DHP (BCM47081)':
	case 'ASUS RT-AC3200':
	case 'Asus RT-AC87U':
	case 'Buffalo WXR-1900DHP':
	case 'Linksys EA9200':
	case 'Netgear R7000':
	case 'Netgear R7900':
	case 'Netgear R8000 (BCM4709)':
	case 'TP-LINK Archer C9 V1':
	case 'ASUS RT-AC3100':
	case 'ASUS RT-AC5300':
	case 'ASUS RT-AC88U':
	case 'D-Link DIR-885L':
	case 'D-Link DIR-890L':
	case 'Linksys EA9500':
	case 'Netgear R8500':
	case 'Phicomm K3':
	case 'D-Link DWL-8610AP':
/* TOMATO64-END */
		COL_P0N = '0';
		COL_P1N = '1';
		COL_P2N = '2';
		COL_P3N = '3';
		COL_P4N = '4';
/* TOMATO64-BEGIN */
		COL_P5N = '5';
		COL_P6N = '6';
		COL_P7N = '7';
		COL_P8N = '8';
/* TOMATO64-END */
		break;
	case 'vlan-testid2':
	case 'Netgear AC1450':
	case 'Netgear R6200v2':
	case 'Netgear R6250':
	case 'Netgear R6300v2':
	case 'Asus RT-AC3100':
	case 'Asus RT-AC88U':
		COL_P0N = '4';
		COL_P1N = '3';
		COL_P2N = '2';
		COL_P3N = '1';
		COL_P4N = '0';
		COL_P5N = '5';
		break;
	case 'vlan-testid3':
	case 'Asus RT-AC3200':
		COL_P0N = '0';
		COL_P1N = '4';
		COL_P2N = '3';
		COL_P3N = '2';
		COL_P4N = '1';
		break;
	/* only has 2 Lan Ports */
	case 'Xiaomi MiWiFi':
		COL_P0N = '4';
		COL_P1N = '0';
		COL_P2N = '2';
		COL_P3N = '1';
		COL_P4N = '3';
		break;
	/* only has 3 Lan ports */
	case 'Tenda AC15':
		COL_P0N = '0';
		COL_P1N = '2';
		COL_P2N = '3';
		COL_P3N = '4';
		COL_P4N = '1';
		break;
	default:
		COL_P0N = '0';
		COL_P1N = '1';
		COL_P2N = '2';
		COL_P3N = '3';
		COL_P4N = '4';
		unknown_router = 1;
		break;
/* BCMARM-END */
/* BCMARM-NO-BEGIN */
	case 'vlan-testid0':
	case 'Belkin Share N300 (F7D3302/F7D7302) v1':
	case 'Belkin Play N600 (F7D4302/F7D8302) v1':
	case 'Belkin N600 DB Wireless N+':
	case 'D-Link Dir-620 C1':
/*	case 'FiberHome HG320': */
	case 'Linksys E800 v1.0':
	case 'Linksys E900 v1.0':
	case 'Linksys E1200 v1.0':
	case 'Linksys E1200 v2.0':
	case 'Linksys E1500 v1.0':
	case 'Linksys E1550 v1.0':
	case 'Linksys E2500 v1.0':
	case 'Linksys E2500 v1/v2/v3':
	case 'Linksys E3200 v1.0':
	case 'Linksys E4200 v1':
	case 'Netgear WNDR3700v3':
	case 'Netgear WNDR4000':
	case 'Netgear WNDR4500 V1':
	case 'Netgear WNDR4500 V2':
		COL_P0N = '4';
		COL_P1N = '0';
		COL_P2N = '1';
		COL_P3N = '2';
		COL_P4N = '3';
		break;
	case 'vlan-testid1':
	case 'Asus RT-AC66U':
	case 'Asus RT-N66U':
	case 'Belkin N F5D8235-4 v3':
/*	case 'Buffalo WZR-D1100H': */
/*	case 'Buffalo WZR-D1800H': */
	case 'Cisco M10 v1.0':
	case 'Cisco M10 v2.0':
	case 'D-Link DIR-865L':
	case 'Linksys M20':
	case 'Linksys E1000 v2.0':
	case 'Linksys E1000 v2.1':
	case 'Linksys WRT310N v2':
	case 'Linksys WRT320N':
	case 'Linksys WRT610N v2':
	case 'Tenda N6':
/*	case 'Tenda N80': */
	case 'Tenda W1800R':
		COL_P0N = '0';
		COL_P1N = '1';
		COL_P2N = '2';
		COL_P3N = '3';
		COL_P4N = '4';
		break;
	case 'vlan-testid2':
	case 'Asus RT-N10P':
	case 'Asus RT-N12':
	case 'Asus RT-N12 A1':
	case 'Asus RT-N12 B1':
	case 'Asus RT-N12 C1':
	case 'Asus RT-N12 D1': /* also used for RT-N12 VP/K */
	case 'Asus RT-N12 HP':
	case 'Asus RT-N15U':
	case 'Asus RT-N53':
	case 'Asus RT-N53 A1':
	case 'Belkin Share Max N300 (F7D3301/F7D7301) v1':
	case 'Belkin Play Max / N600 HD (F7D4301/F7D8301) v1':
	case 'Netcore NR235W': /* NOT in Shibby Firmware - https://github.com/Jackysi/advancedtomato/pull/142 */
	case 'Netgear WNDR3400':
	case 'Netgear WNDR3400v2':
	case 'Netgear WNDR3400v3':
	case 'Netgear R6300 V1':
		COL_P0N = '4';
		COL_P1N = '3';
		COL_P2N = '2';
		COL_P3N = '1';
		COL_P4N = '0';
		break;
	case 'vlan-testid3':
	case 'Asus RT-N10U':
	case 'Asus RT-N16': /* invert port order=checked */
	case 'Catchtech CW-5358U':
/*	case 'ChinaNet RG200E-CA': */
	case 'Netgear WNR2000 v2':
	case 'Netgear WNR3500L/U/v2':
	case 'Netgear WNR3500L v2':
	case 'Tenda N60':
	case 'Linksys WRT160N': /* WRT160Nv3 */
	case 'Linksys E1000 v1':
	case 'Linksys E1000 v1.0':
	case 'Linksys E2000':
	case 'Linksys E3000':
		COL_P0N = '0';
		COL_P1N = '4';
		COL_P2N = '3';
		COL_P3N = '2';
		COL_P4N = '1';
		break;
	default:
		COL_P0N = '0';
		COL_P1N = '1';
		COL_P2N = '2';
		COL_P3N = '3';
		COL_P4N = '4';
		unknown_router = 1;
		break;
/* BCMARM-NO-END */
}
/* MIPSR2P-END */
/* MIPSR2P-NO-BEGIN */
switch (nvram['boardtype']) {
	case '0x0467':  /* WRT54GL 1.x, WRT54GS 3.x/4.x */
	case '0x048e':  /* WL-520GU, WL-500G Premium v2 */
	case '0x04ef':  /* WRT320N/E2000 */
	case '0x04cf':  /* WRT610Nv2/E3000, RT-N16 */
	case '0xf52c':  /* E4200v1 */
		trunk_vlan_supported = 1;
		break;
	default:
		break;
}

switch (nvram['boardtype']) {
	case '0x0467': /* WRT54GL 1.x, WRT54GS 3.x/4.x */
		if (nvram['boardrev'] == '0x13') {  /* WHR-G54S */
			COL_P0N = '0';
			COL_P1N = '1';
			COL_P2N = '2';
			COL_P3N = '3';
			COL_P4N = '4';
		break;
		}
	case '0xa4cf': /* Belkin F7D3301 */
		if (nvram['boardrev'] == '0x1100') { /* Belkin F5D8235-4 v3 */
			COL_P0N = '0';
			COL_P1N = '1';
			COL_P2N = '2';
			COL_P3N = '3';
			COL_P4N = '4';
		break;
		}
	case '0xd4cf': /* Belkin F7D4301 */
	case '0x048e': /* WL-520GU, WL-500G Premium v2 */
		COL_P0N = '4';
		COL_P1N = '3';
		COL_P2N = '2';
		COL_P3N = '1';
		COL_P4N = '0';
		break;
	case '0x04ef': /* WRT320N/E2000 */
	case '0x04cf': /* WRT610Nv2/E3000, RT-N16, WNR3500L */
		COL_P0N = '0';
		COL_P1N = '4';
		COL_P2N = '3';
		COL_P3N = '2';
		COL_P4N = '1';
	break;
	case '0xf52c': /* E4200v1 */
		COL_P0N = '4';
		COL_P1N = '0';
		COL_P2N = '1';
		COL_P3N = '2';
		COL_P4N = '3';
		break;
	/* should work on WRT54G v2/v3, WRT54GS v1/v2 and others */
	default:
		COL_P0N = '0';
		COL_P1N = '1';
		COL_P2N = '2';
		COL_P3N = '3';
		COL_P4N = '4';
		break;
}
/* MIPSR2P-NO-END */

var COL_VID = 0;
var COL_MAP = 1;
var COL_P0  = 2;
var COL_VID_DEF = COL_P0 + MAX_PORT_ID + 1;
var COL_BRI = COL_VID_DEF + 1;
var COL_PN = [];

for (var i = 0; i <= MAX_PORT_ID; ++i) {
	var colName = window['COL_P'+i+'N'];
	COL_PN.push((typeof colName == 'undefined') ? i.toString() : colName);
}

function portCol(i) {
	return COL_P0 + i;
}

function portColName(i) {
	return (typeof COL_PN[i] == 'undefined') ? i.toString() : COL_PN[i];
}

/* RTNPLUS-NO-BEGIN */
var vlt = nvram.vlan0tag | '0';
/* RTNPLUS-NO-END */

/* set to either 5 or 8 when nvram settings are read (FastE or GigE routers) */
var SWITCH_INTERNAL_PORT = 0;

/* option made available for experimental purposes on routers known to support port-based VLANs, but not confirmed to support 801.11q trunks */
/* MIPSR2P-NO-BEGIN */
var PORT_VLAN_SUPPORT_OVERRIDE = ((nvram['trunk_vlan_so'] == '1') ? 1 : 0);
/* MIPSR2P-NO-END */
/* MIPSR2P-BEGIN */
var PORT_VLAN_SUPPORT_OVERRIDE = 0;
/* MIPSR2P-END */

/* aka if (supported_hardware) block */
if (port_vlan_supported) {
	var vlg = new TomatoGrid();
	vlg.setup = function() {
		var portOptions = [[0,''],[1,'🌕 On'],[2,'🌓 Tag']];
		var vidOptions = [];
		for (var i = 0; i <= MAX_VLAN_ID; ++i)
			vidOptions.push([i, i.toString()]);

		var cols = [
			{ type: 'select', options: vidOptions, prefix: '<div class="centered">', suffix: '<\/div>' },
			{ type: 'text', maxlen: 4, prefix: '<div class="centered">', suffix: '<\/div>' }
		];

		for (var i = 0; i <= MAX_PORT_ID; ++i)
			cols.push({ type: 'select', options: portOptions, prefix: '<div class="centered">', suffix: '<\/div>' });

		/* Default VLAN */
		cols.push({ type: 'checkbox', prefix: '<div class="centered">', suffix: '<\/div>' });

		cols.push({ type: 'select', options: vlanBridgeOptions, prefix: '<div class="centered">', suffix: '<\/div>' });

		this.init('vlan-grid', 'sort', VLAN_COUNT, cols);

/* TOMATO64-SKIP-BEGIN */
		var ethIconScale = 100; /* percentage */
		var ethIconW = Math.round(46 * ethIconScale / 100);
		var ethIconH = Math.round(35 * ethIconScale / 100);
		var ethDesc = (nvram.eth_desc || '').split('%');

		function portHeader(i) {
			var displayIndex = displayPortIndex(i);
			var desc = escapeHTML(ethDescClean(ethDesc[displayIndex])) || '&nbsp;';
			return '<div id="vport_'+i+'"><div class="small" style="padding-bottom:5px">'+desc+'<\/div><span class="eth-icon" id="ethsvg_'+i+'" data-w="'+ethIconW+'" data-h="'+ethIconH+'"><\/span><\/div>';
		}

		var headers = ['VLAN', 'VID'];
		for (var i = 0; i <= MAX_PORT_ID; ++i)
			headers.push(portHeader(i));

		headers.push('Native<br>VLAN', 'Bridge');
		this.headerSet(headers);
/* TOMATO64-SKIP-END */
/* TOMATO64-BEGIN */

		var headers = ['VLAN', 'VID'];
		for (var i = 0; i <= MAX_PORT_ID; i++) {
			var label = (typeof PortNames !== 'undefined' && PortNames.getVlanLabel) ? PortNames.getVlanLabel(i) : i.toString();
			headers.push('<div id="vport_'+i+'"><img src="eth_off.gif" id="eth_off_'+(i+1)+'" alt=""><\/div>'+label);
		}
		headers.push('Native<br>VLAN', 'Bridge');
		this.headerSet(headers);
/* TOMATO64-END */

		vlg.populate();
		vlg.canDelete = false;
		vlg.sort(0);
		vlg.showNewEditor();
		vlg.resetNewEditor();
/* TOMATO64-BEGIN */

		var nicCount = nvram.nics ? parseInt(nvram.nics) : (MAX_PORT_ID + 1);
		if (nicCount <= MAX_PORT_ID) {
			var data = this.getAllData();
			for (var row = 0; row < data.length; row++) {
				for (var i = nicCount; i <= MAX_PORT_ID; i++) {
					data[row][2 + i] = 0;
				}
			}
			this.removeAllData();
			for (var row = 0; row < data.length; row++) {
				this.insertData(-1, data[row]);
			}

			var style = document.createElement('style');
			style.type = 'text/css';
			var rules = '';
			rules += 'div#vlan-grid tr.header div[id^="vport_"] img[id^="eth_"] { max-width: 35px !important; height: auto !important; padding: 0 !important; padding-bottom: 0 !important; }\n';
			for (var i = nicCount; i <= MAX_PORT_ID; i++) {
				var colIdx = 2 + i;
				rules += '#vlan-grid th:nth-child(' + (colIdx + 1) + '), ';
				rules += '#vlan-grid td:nth-child(' + (colIdx + 1) + ') { display: none; }\n';
			}
			style.innerHTML = rules;
			document.head.appendChild(style);
		}

		var nativeStyle = document.createElement('style');
		nativeStyle.type = 'text/css';
		nativeStyle.innerHTML = '#vlan-grid th:nth-child(' + (COL_VID_DEF + 1) + '), #vlan-grid td:nth-child(' + (COL_VID_DEF + 1) + ') { display: none; }';
		document.head.appendChild(nativeStyle);
/* TOMATO64-END */
	}

	vlg.populate = function() {
		vlg.removeAllData();

		/* find out which vlans are supposed to be bridged to each LAN */
		var bridged = [];

		for (var i = 0; i <= MAX_BRIDGE_ID; ++i) {
			var p = lanPrefix(i);
/* TOMATO64-SKIP-BEGIN */
			var l = (nvram[p+'_ifnames'] || '').split(' ');
/* TOMATO64-SKIP-END */
/* TOMATO64-BEGIN */
			var l = (nvram[p+'_ifnames_vlan'] || '').split(' ');
/* TOMATO64-END */
/* REMOVE-BEGIN
			alert(p+'_ifnames='+l);
REMOVE-END */
			for (var k = 0 ; k < l.length; k++) {
/* REMOVE-BEGIN
				alert("bridge br"+i+"=vlan"+parseInt(l[k].replace('vlan','')));
REMOVE-END */
				if (l[k].indexOf('vlan') != -1) {
/* REMOVE-BEGIN
					alert(p+'_ifname='+nvram[p+'_ifname']);
REMOVE-END */
					if ((nvram[p+'_ifname'] || '') != '')
						bridged[parseInt(l[k].replace('vlan',''), 10)] = vlanBridgeLan(i).toString();
					else
						bridged[parseInt(l[k].replace('vlan',''), 10)] = VLAN_BRIDGE_NONE.toString();
				}
/* WLAN */
				for (var uidx = 0; uidx < wl_ifaces.length; ++uidx) {
					if (l[k].indexOf(wl_ifaces[uidx][0]) != -1)
						E('_f_bridge_wlan'+uidx+'_to').value = i;
				}
			}
		}

/* WAN ports */
		for (var i = 0; i < MAXWAN_NUM; ++i) {
			var p = wanPrefix(i);
/* TOMATO64-SKIP-BEGIN */
			var vlan = parseInt((nvram[p+'_ifnameX'] || '').replace('vlan',''), 10);
/* TOMATO64-SKIP-END */
/* TOMATO64-BEGIN */
			var vlan = parseInt((nvram[p+'_ifnameX_vlan'] || '').replace('vlan',''), 10);
/* TOMATO64-END */
			if (isFinite(vlan))
				bridged[vlan] = vlanBridgeWan(i).toString();
		}

		/* go thru all possible VLANs */
		for (var i = 0 ; i <= MAX_VLAN_ID ; i++) {
			var port = [];
			var tagged = [];
			if ((nvram['vlan'+i+'hwname'].length > 0) || (nvram['vlan'+i+'ports'].length > 0)) {
				/* (re)initialize our bitmap for this particular iteration */
				for (var j = 0; j <= MAX_PORT_ID ; j++) {
					port[j] = '0';
					tagged[j] = '0';
				}
				/* which ports are members of this VLAN? */
				var m = nvram['vlan'+i+'ports'].split(' ');
				for (var j = 0; j < m.length; ++j) {
					var portId = parseInt(m[j], 10);
					if (!isFinite(portId))
						continue;
					port[portId] = '1';
					tagged[portId] = (((trunk_vlan_supported) || (PORT_VLAN_SUPPORT_OVERRIDE)) && (m[j].indexOf('t') != -1)) ? '1' : '0';
				}

				if (port_vlan_supported) {
					var internalPort = (nvram['vlan'+i+'ports'] || '').match(/(?:^|\s)(\d+)\*/);
					if (internalPort)
						SWITCH_INTERNAL_PORT = internalPort[1];

					var pt = function(n) {
						return (port[n] == '1') ? ((((trunk_vlan_supported) || (PORT_VLAN_SUPPORT_OVERRIDE)) && (tagged[n] == '1')) ? '2' : '1') : '0';
					}

					var row = [
						i.toString(),
						((nvram['vlan'+i+'vid'] != '') && (nvram['vlan'+i+'vid'] > 0)) ? (nvram['vlan'+i+'vid']).toString() : '0'
					];
					for (var j = 0; j <= MAX_PORT_ID; ++j)
						row.push(pt(portColName(j)));

					row.push((((nvram['vlan'+i+'ports']).indexOf('*') != -1) ? '1' : '0'));
					row.push((bridged[i] != null) ? bridged[i] : '1');
					vlg.insertData(-1, row);
				}
			}
		}
	}

	vlg.countElem = function(f, v) {
		var data = this.getAllData();
		var total = 0;
		for (var i = 0; i < data.length; ++i)
			total += ((data[i][f] == v) ? 1 : 0);

		return total;
	}

	vlg.countDefaultVID = function() {
		return this.countElem(COL_VID_DEF, 1);
	}

	vlg.countVID = function (v) {
		return this.countElem(COL_VID, v);
	}

	vlg.countWan = function(wan) {
		return this.countElem(COL_BRI, vlanBridgeWan(wan));
	}

	vlg.countLan = function(l) {
		return this.countElem(COL_BRI, vlanBridgeLan(l));
	}

	vlg.verifyFields = function(row, quiet) {
		var i, j, old, me, checkNative, valid = 1;
		var f = fields.getAll(row);

		for (i = 0; i<= MAX_VLAN_ID ; i++)
			f[COL_VID].options[i].disabled = (this.countVID(i) > 0);

		for (i = 0; i <= MAX_BRIDGE_ID; ++i) {
			j = lanPrefix(i);
			f[COL_BRI].options[i + 2].disabled = ((nvram[j+'_ifname'] || '').length < 1);
		}

		if (!v_range(f[COL_MAP], quiet, 0, 4094))
			valid = 0;

		/* enforce trunk VLAN rules */
		function enforcePortState(col) {
			var val = parseInt(f[col].value, 10);
			var trunkAllowed = (trunk_vlan_supported || PORT_VLAN_SUPPORT_OVERRIDE);

			if (f[col].options.length > 2)
				f[col].options[2].disabled = !trunkAllowed;

			if (!trunkAllowed && val === 2)
					f[col].value = '1';

			if (val !== 0 && val !== 1 && val !== 2)
				f[col].value = '0';
		}
		for (i = 0; i <= MAX_PORT_ID; ++i)
			enforcePortState(portCol(i));

		/* Modifications to enable Native VLAN support (allow one untagged vlan per port) by default */
		var err_vlan = 'Only one untagged VLAN per port is allowed (Native VLAN)';
		old = ((row == this.editor) && this.source) ? this.source.getRowData() : null;
		var oldPort = [];
		for (i = 0; i <= MAX_PORT_ID; ++i)
			oldPort[i] = (old && (old.length > portCol(i))) ? old[portCol(i)] : '0';
		me = this;
		checkNative = function(col, oldVal) {
			if (f[col].value == '1') {
				if ((me.countElem(col, 1) - ((oldVal == '1') ? 1 : 0)) > 0) {
					ferror.set(f[col], err_vlan, quiet);
					valid = 0;
				}
				else
					ferror.clear(f[col]);
			}
			else
				ferror.clear(f[col]);
		}
		for (i = 0; i <= MAX_PORT_ID; ++i)
			checkNative(portCol(i), oldPort[i]);

		if (this.countDefaultVID() > 0) {
			f[COL_VID_DEF].disabled = 1;
			f[COL_VID_DEF].checked = 0;
		}

		if ((this.countDefaultVID() > 0) && (f[COL_VID_DEF].checked == 1)) {
			ferror.set(f[COL_VID_DEF], 'Only one VID can be selected as the default VID', quiet);
			valid = 0;
		}
		else
			ferror.clear(f[COL_VID_DEF]);

		if (this.countVID(f[COL_VID].selectedIndex) > 0) {
			ferror.set(f[COL_VID], 'Cannot add more than one entry with VID '+f[0].selectedIndex, quiet);
			valid = 0;
		}
		else
			ferror.clear(f[COL_VID]);

		var bridge = parseInt(f[COL_BRI].value, 10);
		var bridgeError = '';

		for (i = 0; i < MAXWAN_NUM; ++i) {
			if ((this.countWan(i) > 0) && (bridge == vlanBridgeWan(i))) {
				bridgeError = 'Only one VID can be used as WAN'+i+' at any time';
				break;
			}
		}

		if (!bridgeError) {
			for (i = 0; i <= MAX_BRIDGE_ID; ++i) {
				if ((this.countLan(i) > 0) && (bridge == vlanBridgeLan(i))) {
					bridgeError = 'One and only one VID can be used for LAN'+i+' (br'+i+') at any time';
					break;
				}
			}
		}

		if (bridgeError) {
			ferror.set(f[COL_BRI], bridgeError, quiet);
			valid = 0;
		}
		else
			ferror.clear(f[COL_BRI]);

		return valid;
	}

	vlg.dataToView = function(data) {
		var pv = function(v) {
			v = v.toString();
			return (v == '1') ? '🌕' : ((v == '2') ? '🌓' : '');
		}
		var view = [
			data[COL_VID],
/* RTNPLUS-NO-BEGIN */
			((data[COL_MAP].toString() == '') || (data[COL_MAP].toString() == '0')) ? (parseInt(E('_vlan0tag').value) * 1 + data[COL_VID] * 1).toString() : data[COL_MAP].toString()
/* RTNPLUS-NO-END */
/* RTNPLUS-BEGIN */
			((data[COL_MAP].toString() == '') || (data[COL_MAP].toString() == '0')) ? (data[COL_VID] * 1).toString() : data[COL_MAP].toString()
/* RTNPLUS-END */
		];
		for (var i = 0; i <= MAX_PORT_ID; ++i)
			view.push(pv(data[portCol(i)]));

		view.push(
			(data[COL_VID_DEF].toString() != '0') ? '🌑' : '',
			vlanBridgeNames[data[COL_BRI]] || ''
		);
		return view;
	}

	vlg.dataToFieldValues = function (data) {
		var values = [data[COL_VID], data[COL_MAP]];
		for (var i = 0; i <= MAX_PORT_ID; ++i)
			values.push(data[portCol(i)]);

		values.push((data[COL_VID_DEF] != 0) ? 1 : 0, data[COL_BRI]);
		return values;
	}

	vlg.fieldValuesToData = function(row) {
		var f = fields.getAll(row);
		var data = [f[COL_VID].value, f[COL_MAP].value];
		for (var i = 0; i <= MAX_PORT_ID; ++i)
			data.push(parseInt(f[portCol(i)].value, 10) || 0);

		data.push(f[COL_VID_DEF].checked ? 1 : 0, f[COL_BRI].value);
		return data;
	}

	vlg.onCancel = function() {
		this.removeEditor();
		this.showSource();
		this.disableNewEditor(false);

		this.resetNewEditor();
	}

	vlg.onAdd = function() {
		var data;

		this.moving = null;
		this.rpHide();

		if (!this.verifyFields(this.newEditor, false))
			return;

		data = this.fieldValuesToData(this.newEditor);
		this.insertData(-1, data);

		this.disableNewEditor(false);
		this.resetNewEditor();

		this.resort();
	}

	vlg.onOK = function() {
		var i, data, view;

		if (!this.verifyFields(this.editor, false))
			return;

		data = this.fieldValuesToData(this.editor);
		view = this.dataToView(data);

		this.source.setRowData(data);
		for (i = 0; i < this.source.cells.length; ++i)
			this.source.cells[i].innerHTML = view[i];

		this.removeEditor();
		this.showSource();
		this.disableNewEditor(false);

		this.resetNewEditor();
		this.resort();
	}

	vlg.onDelete = function() {
		this.removeEditor();
		elem.remove(this.source);
		this.source = null;
		this.disableNewEditor(false);

		this.resetNewEditor();
	}

	vlg.sortCompare = function(a, b) {
		var col = this.sortColumn;
		var ra = a.getRowData();
		var rb = b.getRowData();
		var r;

		switch (col) {
		case COL_VID:
			/* VLAN (loc) */
			r = cmpInt(parseInt(ra[COL_VID], 10), parseInt(rb[COL_VID], 10));
		break;
		case COL_MAP:
			/* VID (net) */
			r = cmpInt(parseInt(ra[COL_MAP], 10), parseInt(rb[COL_MAP], 10));
		break;
		case COL_BRI:
			/* Bridge */
			r = cmpInt(parseInt(ra[COL_BRI], 10), parseInt(rb[COL_BRI], 10));
		break;
		default:
			r = cmpText(a.cells[col].innerHTML, b.cells[col].innerHTML);
		}

		if (r == 0)
			r = cmpInt(parseInt(ra[COL_VID], 10), parseInt(rb[COL_VID], 10));

		return this.sortAscending ? r : -r;
	}

	vlg.resetNewEditor = function() {
		var f = fields.getAll(this.newEditor);

		for (var i = 0; i <= MAX_BRIDGE_ID; ++i) {
			var p = lanPrefix(i);
			f[COL_BRI].options[i + 2].disabled = ((nvram[p+'_ifname'] || '').length < 1);
		}

		f[COL_MAP].value = '0';

		f[COL_VID].selectedIndex = 0;
		var t = MAX_VLAN_ID;
		while ((this.countVID(f[COL_VID].selectedIndex) > 0) && (t > 0)) {
			f[COL_VID].selectedIndex = (f[COL_VID].selectedIndex + 1) % VLAN_COUNT;
			t--;
		}

		for (var i = 0; i <= MAX_VLAN_ID ; i++)
			f[COL_VID].options[i].disabled = (this.countVID(i) > 0);

		for (var i = 0; i <= MAX_PORT_ID; ++i)
			f[portCol(i)].value = '0';
		f[COL_VID_DEF].checked = 0;
		if (this.countDefaultVID() > 0)
			f[COL_VID_DEF].disabled = 1;

		f[COL_BRI].selectedIndex = 0;
		ferror.clearAll(fields.getAll(this.newEditor));
	}
}
/* end of the so-called if (supported_device) block */

function trailingSpace(s) {
	return ((s.length > 0) && (s.charAt(s.length - 1) != ' ')) ? ' ' : '';
}

function verifyFields(focused, quiet) {
/* MIPSR2P-NO-BEGIN */
	/* sync active editor before validation */
	if (typeof vlg != 'undefined' && vlg.isEditing()) {
		/* try to save edited row */
		if (!vlg.onOK()) {
			return 0;
		}
	}

	PORT_VLAN_SUPPORT_OVERRIDE = (E('_f_trunk_vlan_so').checked ? 1 : 0);

	/* enforce no-tag when trunk unsupported and override disabled */
	if ((!trunk_vlan_supported) && (!PORT_VLAN_SUPPORT_OVERRIDE) && (typeof vlg != 'undefined')) {
		var data = vlg.getAllData();
		var changed = false;

		for (var i = 0; i < data.length; ++i) {
			/* port columns */
			for (var col = COL_P0; col < COL_VID_DEF; ++col) {
				if (data[i][col] == 2) { /* only Tag */
					data[i][col] = 1; /* downgrade Tag -> On */
					changed = true;
				}
			}
		}

		if (changed) {
			vlg.removeAllData();

			for (var i = 0; i < data.length; ++i) {
				vlg.insertData(-1, data[i]);
			}
			vlg.recolor();
		}
	}
/* MIPSR2P-NO-END */
	for (var uidx = 0; uidx < wl_ifaces.length; ++uidx) {
		var wlan = E('_f_bridge_wlan'+uidx+'_to');
		for (var i = 0; i <= MAX_BRIDGE_ID; ++i) {
			var p = lanPrefix(i);
			wlan.options[i].disabled = ((nvram[p+'_ifname'] || '').length < 1);
		}
	}

/* RTNPLUS-NO-BEGIN */
	if (!v_range('_vlan0tag', quiet, 0, VLAN_OFFSET_MAX))
		return 0;

	var e = E('_vlan0tag');
	var v = parseInt(e.value);
	e.value = v - (v % VLAN_COUNT);
	if ((e.value != vlt) && (typeof(vlg) != 'undefined')) {
		vlg.populate();
		vlt = e.value;
	}
/* RTNPLUS-NO-END */

	return 1;
}

function save() {
	if (vlg.isEditing())
		return;

	var i, j, k, p, d, e, v = '';
	var fom = E('t_fom');

/* MIPSR2P-NO-BEGIN */
	fom.trunk_vlan_so.value = (E('_f_trunk_vlan_so').checked ? 1 : 0);
/* MIPSR2P-NO-END */

	/* wipe out relevant fields just in case this is not the first time we try to submit */
	for (i = 0 ; i <= MAX_VLAN_ID ; i++) {
		fom['vlan'+i+'ports'].value = '';
		fom['vlan'+i+'hwname'].value = '';
		fom['vlan'+i+'vid'].value = '';
	}

	for (i = 0; i <= MAX_BRIDGE_ID; ++i)
		fom[lanPrefix(i)+'_ifnames'].value = '';
/* TOMATO64-BEGIN */
	for (i = 0; i <= MAX_BRIDGE_ID; ++i)
		fom[lanPrefix(i)+'_ifnames_vlan'].value = '';
/* TOMATO64-END */

	for (i = 0; i < MAXWAN_NUM; ++i) {
		j = wanPrefix(i);
		fom[j+'_ifnameX'].value = '';
/* TOMATO64-BEGIN */
		fom[j+'_ifnameX_vlan'].value = '';
/* TOMATO64-END */
		fom[j+'_iface'].value = '';
		fom[j+'_iface'].disabled = 1;
		fom[j+'_ifname'].value = '';
		fom[j+'_ifname'].disabled = 1;
		fom[j+'_hwaddr'].value = '';
		fom[j+'_hwaddr'].disabled = 1;
		fom[j+'_proto'].value = '';
		fom[j+'_proto'].disabled = 1;
	}

	d = vlg.getAllData();

	for (i = 0; i < d.length; ++i) {
		p = '';
		for (j = 0; j <= MAX_PORT_ID; ++j) {
			var col = portCol(j);
			p += (d[i][col].toString() != '0') ? portColName(j) : '';
			p += (((trunk_vlan_supported || PORT_VLAN_SUPPORT_OVERRIDE)) && (d[i][col].toString() == '2')) ? 't' : '';
			p += trailingSpace(p);
		}

		p += (d[i][COL_VID_DEF].toString() != '0') ? (SWITCH_INTERNAL_PORT+'*') : SWITCH_INTERNAL_PORT;

		/* arrange port numbers in ascending order just to be safe (not sure if this is really needed... mostly, cosmetics?) */
		p = p.split(" ");
		p = p.sort(cmpInt);
		p = p.join(" ");

		v += (d[i][COL_VID_DEF].toString() != '0') ? d[i][0] : '';

		fom['vlan'+d[i][COL_VID]+'ports'].value = p;
/* TOMATO64-SKIP-BEGIN */
/* BCMARM-BEGIN */
		if ((nvram['t_model_name'] == 'Netgear R7900') || (nvram['t_model_name'] == 'Netgear R8000'))
			fom['vlan'+d[i][COL_VID]+'hwname'].value = 'et2';
		else if ((nvram['t_model_name'] == 'Asus RT-AC5300') || (nvram['t_model_name'] == 'Asus RT-AC88U'))
			fom['vlan'+d[i][COL_VID]+'hwname'].value = 'et1';
		else
/* BCMARM-END */
		fom['vlan'+d[i][COL_VID]+'hwname'].value = 'et0';
/* TOMATO64-SKIP-END */

		fom['vlan'+d[i][COL_VID]+'vid'].value = ((d[i][COL_MAP].toString() != '') && (d[i][COL_MAP].toString() != '0')) ? d[i][COL_MAP] : '';

		var bridge = parseInt(d[i][COL_BRI], 10);
/* TOMATO64-SKIP-BEGIN */
		for (j = 0; j <= MAX_BRIDGE_ID; ++j) {
			if (bridge == vlanBridgeLan(j))
				fom[lanPrefix(j)+'_ifnames'].value += 'vlan'+d[i][0];
		}
		for (j = 0; j < MAXWAN_NUM; ++j) {
			if (bridge == vlanBridgeWan(j))
				fom[wanPrefix(j)+'_ifnameX'].value += 'vlan'+d[i][0];
		}
/* REMOVE-BEGIN
		fom['lan_ifnames'].value += trailingSpace(fom['lan_ifnames'].value);
		alert('vlan'+d[i][0]+'ports='+fom['vlan'+d[i][0]+'ports'].value+'\nvlan'+d[i][0]+'hwname='+fom['vlan'+d[i][0]+'hwname'].value);
REMOVE-END */
/* TOMATO64-SKIP-END */
/* TOMATO64-BEGIN */

		/* the abstract vlanX assignment is kept in the shadow *_vlan vars; the real
		 * lan/wan vars get the actual ethX[.VID] interface names instead
		 */
		var vlanTag = 'vlan'+d[i][0];

		for (j = 0; j <= MAX_BRIDGE_ID; ++j) {
			if (bridge == vlanBridgeLan(j))
				fom[lanPrefix(j)+'_ifnames_vlan'].value += vlanTag;
		}
		for (j = 0; j < MAXWAN_NUM; ++j) {
			if (bridge == vlanBridgeWan(j))
				fom[wanPrefix(j)+'_ifnameX_vlan'].value += vlanTag;
		}

		for (var port = 0; port <= MAX_PORT_ID; ++port) {
			var ifname = '';

			if (d[i][2 + port] == 1)
				ifname = 'eth' + port;
			else if (d[i][2 + port] == 2) {
				var vid = ((d[i][COL_MAP].toString() != '') && (d[i][COL_MAP].toString() != '0')) ? d[i][COL_MAP].toString() : d[i][COL_VID].toString();
				fom['vlan'+d[i][COL_VID]+'hwname'].value += 'eth' + port + ' ';
				ifname = 'eth' + port + '.' + vid;
			}
			else
				continue;

			for (j = 0; j <= MAX_BRIDGE_ID; ++j) {
				if (bridge == vlanBridgeLan(j))
					fom[lanPrefix(j)+'_ifnames'].value += ifname + ' ';
			}
			for (j = 0; j < MAXWAN_NUM; ++j) {
				if (bridge == vlanBridgeWan(j))
					fom[wanPrefix(j)+'_ifnameX'].value += ifname;
			}
		}

		fom['vlan'+d[i][COL_VID]+'hwname'].value = fom['vlan'+d[i][COL_VID]+'hwname'].value.trim();
		for (j = 0; j <= MAX_BRIDGE_ID; ++j)
			fom[lanPrefix(j)+'_ifnames'].value = fom[lanPrefix(j)+'_ifnames'].value.trim();
/* TOMATO64-END */
	}

	/* count active WANs / wipe out relevant fields for inactive or just disabled WAN - needed in various places for the proper operation of FW */
	k = 0;
	for (i = 0; i < MAXWAN_NUM; ++i) {
		j = wanPrefix(i);
/* TOMATO64-SKIP-BEGIN */
		if (fom[j+'_ifnameX'].value.length > 1)
/* TOMATO64-SKIP-END */
/* TOMATO64-BEGIN */
		if (fom[j+'_ifnameX'].value.length > 1 || fom[j+'_ifnameX_vlan'].value.length > 1)
/* TOMATO64-END */
			k++;
		else {
			fom[j+'_iface'].disabled = 0;
			fom[j+'_iface'].value = '';
			fom[j+'_ifname'].disabled = 0;
			fom[j+'_ifname'].value = '';
			fom[j+'_hwaddr'].disabled = 0;
			fom[j+'_hwaddr'].value = '';
			fom[j+'_proto'].disabled = 0;
			fom[j+'_proto'].value = 'disabled';
		}
	}
	//fom.mwan_num.value = (k < 1 ? 1 : k);
	fom.mwan_num.value = 1; /* just reset mwan_num to 1 to avoid problems */

	for (i = 0; i < wl_ifaces.length; ++i) {
		var wlan = E('_f_bridge_wlan'+i+'_to');
		var bridge = parseInt(wlan.value, 10);
/* REMOVE-BEGIN
		alert(bridge);
REMOVE-END */
		if ((bridge >= 0) && (bridge <= MAX_BRIDGE_ID))
			fom[lanPrefix(bridge)+'_ifnames'].value += ' '+wl_ifaces[i][0];
	}
/* REMOVE-BEGIN
	var lif = (nvram['lan_ifnames'] || '').split(' ');
	for (var j = 0; j < lif.length; ++j) {
		fom['lan_ifnames'].value += (lif[j].indexOf('vlan') != -1) ? '' : lif[j];
		fom['lan_ifnames'].value += trailingSpace(fom['lan_ifnames'].value);
	}
	var debugLan = [];
	for (var j = 0; j <= MAX_BRIDGE_ID; ++j) {
		var p = lanPrefix(j)+'_ifnames';
		debugLan.push(p+'='+fom[p].value);
	}
	alert(debugLan.join('\n'));
REMOVE-END */

	/* Prevent vlan reset to default at init */
	fom['manual_boot_nv'].value = 1;

	e = E('footer-msg');

	if (vlg.countWan(0) != 1) {
		e.innerHTML = 'Cannot proceed: one VID must be assigned to WAN.';
		e.style.display = 'inline-block';
		setTimeout(
			function() {
				e.innerHTML = '';
				e.style.display = 'none';
			}, 5000);
		return;
	}

	if (vlg.countLan(0) != 1) {
		e.innerHTML = 'Cannot proceed: one and only one VID must be assigned to the primary LAN0 (br0).';
		e.style.display = 'inline-block';
		setTimeout(
			function() {
				e.innerHTML = '';
				e.style.display = 'none';
			}, 5000);
		return;
	}

	if (v.length < 1) {
		e.innerHTML = 'Cannot proceed without setting a default VID';
		e.style.display = 'inline-block';
		setTimeout(
			function() {
				e.innerHTML = '';
				e.style.display = 'none';
			}, 5000);
		return;
	}

	if (confirm("Router must be rebooted to proceed. Commit changes to NVRAM and reboot now?"))
		form.submit(fom, 0);
}

function earlyInit() {
	if (!port_vlan_supported) {
		E('save-button').disabled = 1;
		return;
	}

/* MIPSR2P-NO-BEGIN */
	PORT_VLAN_SUPPORT_OVERRIDE = ((nvram['trunk_vlan_so'] == '1') ? 1 : 0);
/* MIPSR2P-NO-END */
/* MIPSR2P-BEGIN */
	PORT_VLAN_SUPPORT_OVERRIDE = 0;

	if (unknown_router == 1)
		E('unknown_router').style.display = 'block';
/* MIPSR2P-END */

	verifyFields(null, 1);
	insOvl();
}

function init() {
	if (port_vlan_supported) {
		E('sesdiv').style.display = 'block';
		vlg.recolor();
		vlg.resetNewEditor();
		restoreVisibility(cprefix, 'notes');
	}
	else
		E('notice-msg').innerHTML = '<div id="notice">The feature is not supported on this router.<\/div>';

	up.initPage(250, 5);

	eventHandler();
	show();
}
</script>
</head>

<body onload="init()">
<form id="t_fom" method="post" action="tomato.cgi">
<table id="container">
<tr><td colspan="2" id="header">
	<div class="title"><a href="/">Tomato64</a></div>
	<div class="version">Version <% version(); %> on <% nv("t_model_name"); %><span class="blinking bl2"><script><% anonupdate(); %> anon_update()</script>&nbsp;</span></div>
</td></tr>
<tr id="body"><td id="navi"><script>navi()</script></td>
<td id="content">
<div id="ident"><% ident(); %> | <script>wikiLink();</script></div>

<!-- / / / -->

<input type="hidden" name="_nextpage" value="advanced-vlan.asp">
<input type="hidden" name="_nextwait" value="30">
<input type="hidden" name="_reboot" value="1">
<input type="hidden" name="_nvset" value="1">
<input type="hidden" name="_commit" value="1">
<script>
for (var i = 0; i <= MAX_VLAN_ID; ++i) {
	W('<input type="hidden" name="vlan'+i+'ports">');
	W('<input type="hidden" name="vlan'+i+'hwname">');
	W('<input type="hidden" name="vlan'+i+'vid">');
}

for (var i = 0; i < MAXWAN_NUM; ++i) {
	var p = wanPrefix(i);
	W('<input type="hidden" name="'+p+'_ifnameX">');
	W('<input type="hidden" name="'+p+'_iface" value="" disabled="disabled">');
	W('<input type="hidden" name="'+p+'_ifname" value="" disabled="disabled">');
	W('<input type="hidden" name="'+p+'_hwaddr" value="" disabled="disabled">');
	W('<input type="hidden" name="'+p+'_proto" value="" disabled="disabled">');
/* TOMATO64-BEGIN */
	W('<input type="hidden" name="'+p+'_ifnameX_vlan">');
/* TOMATO64-END */
}

for (var i = 0; i <= MAX_BRIDGE_ID; ++i)
	W('<input type="hidden" name="'+lanPrefix(i)+'_ifnames">');
/* TOMATO64-BEGIN */
for (var i = 0; i <= MAX_BRIDGE_ID; ++i)
	W('<input type="hidden" name="'+lanPrefix(i)+'_ifnames_vlan">');
/* TOMATO64-END */
</script>
<input type="hidden" name="mwan_num">
<input type="hidden" name="manual_boot_nv">
<!-- MIPSR2P-NO-BEGIN -->
<input type="hidden" name="trunk_vlan_so">
<!-- MIPSR2P-NO-END -->

<!-- MIPSR2P-BEGIN -->
<div id="unknown_router" style="display:none">
	<div class="section-title">!! Unknown Port Mapping, using default !!</div>
	<div class="section-centered">
		<a href="https://www.linksysinfo.org/index.php?threads/can-vlan-gui-port-order-be-corrected.70160/#post-247634" class="new_window"><b>Please follow this link for instructions to get it corrected.</b></a>
		<br><br>Include router Brand/Model (<% nv('t_model_name'); %>), results from "nvram show | grep vlan1ports" &amp;
		<br>port numbers on BACK of router case (left -> right viewed from front).
	</div>
</div>
<!-- MIPSR2P-END -->

<div id="notice-msg"></div>

<!-- / / / -->

<div id="sesdiv" style="display:none">
	<div class="section-title">VLAN Ethernet</div>
	<div class="section">
		<div class="tomato-grid" id="vlan-grid"></div>
	</div>

<!-- RTNPLUS-NO-BEGIN -->
	<div class="section-title">VID Offset</div>
	<div class="section">
		<script>
			createFieldTable('', [
				{ title: 'First 802.1Q VLAN tag', name: 'vlan0tag', type: 'text', maxlen: 4, size: 6, value: fixInt(nvram.vlan0tag, 0, VLAN_OFFSET_MAX, 0), suffix: '&nbsp; <small><i>(range: 0 - '+VLAN_OFFSET_MAX+'; must be a multiple of '+VLAN_COUNT+'; set to 0 to disable)<\/i><\/small>' }
			]);
		</script>
	</div>
<!-- RTNPLUS-NO-END -->

/* TOMATO64-SKIP-BEGIN */
	<div class="section-title">Wireless bridging</div>
	<div class="section">
		<script>
			var f = [];
			for (var uidx = 0; uidx < wl_ifaces.length; ++uidx) {
				var u = wl_fface(uidx).toString();
				if (u) {
					var ssid = wl_ifaces[uidx][4] || '';
					if (nvram['wl'+u+'_radio'] != '1' || nvram['wl'+u+'_net_mode'] == 'disabled')
						ssid = '<s title="Disabled!" style="cursor:help">'+ssid+'<\/s>';

					f.push( { title: wl_display_ifname(uidx), name: 'f_bridge_wlan'+uidx+'_to', type: 'select',
								options: wlanBridgeOptions, suffix: '&nbsp;&nbsp;⇔&nbsp; SSID: '+ssid, value: WLAN_BRIDGE_NONE, prefix: '⇔ &nbsp;&nbsp;&nbsp;' } );
				}
			}
			createFieldTable('', f);
			if (port_vlan_supported)
				vlg.setup();
		</script>
	</div>
/* TOMATO64-SKIP-END */
/* TOMATO64-BEGIN */
	<div class="section">
		<script>
			if (port_vlan_supported)
				vlg.setup();
		</script>
	</div>
/* TOMATO64-END */

<!-- MIPSR2P-NO-BEGIN -->
	<div class="section-title">Trunk VLAN support override (experimental)</div>
	<div class="section">
		<script>
			createFieldTable('', [
				{ title: 'Enable', name: 'f_trunk_vlan_so', type: 'checkbox', value: nvram.trunk_vlan_so == '1' },
			]);
		</script>
	</div>
<!-- MIPSR2P-NO-END -->

	<script>writeToggleSectionTitle('Notes', 'notes');</script>
	<div class="section" id="sesdiv_notes" style="display:none">
		<div>*** If you notice that the order of the LAN Ports are incorrect, try the <a href="basic-network.asp">Invert Ports Order</a> first, if not read <a href="https://www.linksysinfo.org/index.php?threads/can-vlan-gui-port-order-be-corrected.70160/#post-247634" target="_blank" rel="noopener noreferrer"> <b>this</b></a> ***</div>
		<br>
		VLAN Ethernet:
		<ul>
			<li><b>VLAN</b>: Locally unique identifier</li>
			<li><b>VID</b>: Override default VLAN/VID mapping with custom VID (0 = default)</li>
			<li><b>Per-port setting</b>: Off = empty, Untagged = 🌕, Tagged = 🌓
<!-- MIPSR2P-NO-BEGIN -->
				<script>
					if (!trunk_vlan_supported)
						W(' <i>(it is not known whether tagging is supported by this model)<\/i>');
				</script>
<!-- MIPSR2P-NO-END -->
			</li>
/* TOMATO64-SKIP-BEGIN */
			<li><b>Native VLAN</b>: Default VLAN for untagged ingress frames = 🌑</li>
/* TOMATO64-SKIP-END */
			<li><b>Bridge</b>: One VLAN per bridge. WAN bridge (logical) ≠ WAN port (physical)</li>
		</ul>
		<br>
<!-- RTNPLUS-NO-BEGIN -->
		<div><i>VID Offset:</i> First 802.1Q VLAN tag to be used as <i>base/initial tag/VID</i> for VLAN and VID assignments. This allows using VIDs larger than <script>W(MAX_VLAN_ID);</script> on (older) devices, in contiguous blocks/ranges with up to <script>W(VLAN_COUNT);</script> VLANs/VIDs. Set to '0' (zero) to disable this feature and VLANs will have the very same/identical value for its VID, as usual (from 0 to <script>W(MAX_VLAN_ID);</script>).</div>
		<br>
<!-- RTNPLUS-NO-END -->
/* TOMATO64-SKIP-BEGIN */
		Wireless bridging:
		<ul>
			<li><b>Wireless interface to LAN bridge</b> - Maps each wireless interface (physical/virtual) to its LAN bridge</li>
		</ul>
/* TOMATO64-SKIP-END */
		<br>
		Tips:
		<ul>
/* TOMATO64-SKIP-BEGIN */
			<li>Cross check settings on the <a href="basic-network.asp">Network</a> and <a href="advanced-wlanvifs.asp">Virtual Wireless</a> pages</li>
/* TOMATO64-SKIP-END */
/* TOMATO64-BEGIN */
			<li>Cross check settings on the <a href="basic-network.asp">Network</a> page</li>
/* TOMATO64-END */
/* TOMATO64-SKIP-BEGIN */
			<li>Default VID: 0 (some releases) or 1</li>
/* TOMATO64-SKIP-END */
			<li>Assign one VID to WAN bridges</li>
			<li>Select one default VID</li>
			<script>
/* MIPSR2P-BEGIN */
				if (trunk_vlan_supported) {
/* MIPSR2P-END */
/* MIPSR2P-NO-BEGIN */
				if ((trunk_vlan_supported) || (nvram.trunk_vlan_so == '1')) {
/* MIPSR2P-NO-END */
					W('<li>Avoid VID 0: 802.1Q treats it as untagged (priority only).<\/li>\n');
					W('<li>Trunking tip: Skip VID 1 (often reserved for management by other vendors).<\/li>\n');
				}
			</script>
		</ul>
	</div>

<!-- / / / -->

</div>

<!-- / / / -->

<script>writeFooter();</script>

</td></tr>
</table>
</form>
<script>earlyInit();</script>
</body>
</html>
