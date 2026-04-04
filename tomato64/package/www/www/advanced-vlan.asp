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
//	<% nvram ("t_model_name,vlan0ports,vlan1ports,vlan2ports,vlan3ports,vlan4ports,vlan5ports,vlan6ports,vlan7ports,vlan8ports,vlan9ports,vlan10ports,vlan11ports,vlan12ports,vlan13ports,vlan14ports,vlan15ports,vlan0hwname,vlan1hwname,vlan2hwname,vlan3hwname,vlan4hwname,vlan5hwname,vlan6hwname,vlan7hwname,vlan8hwname,vlan9hwname,vlan10hwname,vlan11hwname,vlan12hwname,vlan13hwname,vlan14hwname,vlan15hwname,wan_ifnameX,manual_boot_nv,boardtype,boardflags,lan_ifname,lan_ifnames,vlan0tag,vlan0vid,vlan1vid,vlan2vid,vlan3vid,vlan4vid,vlan5vid,vlan6vid,vlan7vid,vlan8vid,vlan9vid,vlan10vid,vlan11vid,vlan12vid,vlan13vid,vlan14vid,vlan15vid,model,wl_ssid,wl_radio,wl_net_mode,wl_nband,boardnum,boardrev,trunk_vlan_so");%>
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

/* TOMATO64-SKIP-BEGIN */
function calcWanPortCount() {
	var count = 0;
	var maxWan = parseInt(nvram.mwan_num, 10);
	if (isNaN(maxWan) || (maxWan < 0)) maxWan = 0;

	for (var uidx = 1; uidx <= maxWan; ++uidx) {
		var u = (uidx > 1) ? uidx : '';
		if ((nvram['wan'+u+'_sta'] == '') && (nvram['wan'+u+'_proto'] != 'lte') && (nvram['wan'+u+'_proto'] != 'ppp3g'))
			++count;
	}
	return count;
}

function portCaption(i) {
	var wanCount = calcWanPortCount();

	var orig;
	if (i < wanCount)
		orig = (i === 0 && (nvram.model == 'DSL-AC68U')) ? 'DSL' : ('WAN'+i);

	if (!orig)
		orig = (wanCount > 0) ? ('LAN'+(i - 1)) : ('LAN'+i);

	/* Return only first and last character when the caption is longer than 1
	 * (e.g. "LAN0" -> "L0", "WAN1" -> "W1"). Single-char captions stay
	 * as-is.
	 */
	if (orig && orig.length > 1)
		return String(orig).charAt(0) + String(orig).charAt(orig.length - 1);

	return orig;
}

/* caption/rendering is handled by ethernet-icon.js (renderEthIcon)
 * keep the VLAN page markup minimal and call into renderEthIcon() from show()
 */
function show() {
	var state = [];
	var port = etherstates.port0;
	if (port == 'disabled')
		return 0;

	for (var i = 0 ; i <= MAX_PORT_ID ; i++) {
		port = etherstates['port'+i];
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
		var cap = portCaption(i);
		var o = E('ethsvg_'+i);
		var captionText = renderEthIcon(o, sp, du, cap);
		if (o)
			o.title = captionText || state[1] || '';
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
	case 'GL.iNet GL-MT6000':
	case 'Banana Pi BPI-R3':
	case 'Banana Pi BPI-R3 Mini':
	case 'Raspberry Pi 4 Model B':
	case 'FriendlyElec NanoPi R6S':
	case 'FriendlyElec NanoPi R5S':
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
/* EXTSW-BEGIN */
		COL_P5N = '5';
/* EXTSW-END */
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
var COL_P1  = 3;
var COL_P2  = 4;
var COL_P3  = 5;
var COL_P4  = 6;
/* TOMATO64-SKIP-BEGIN */
var COL_P5;
var COL_VID_DEF;
var COL_BRI;
/* EXTSW-NO-BEGIN */
COL_VID_DEF = 7;
COL_BRI = 8;
/* EXTSW-NO-END */
/* EXTSW-BEGIN */
COL_P5  = 7;
COL_VID_DEF = 8;
COL_BRI = 9;
/* EXTSW-END */
/* TOMATO64-SKIP-END */
/* TOMATO64-BEGIN */
var COL_P5  = 7;
var COL_P6  = 8;
var COL_P7  = 9;
var COL_P8  = 10;
var COL_VID_DEF = 11;
var COL_BRI = 12;
/* TOMATO64-END */

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
		var cols = [
			{ type: 'select', options: [[0,'0'],[1,'1'],[2,'2'],[3,'3'],[4,'4'],[5,'5'],[6,'6'],[7,'7'],[8,'8'],[9,'9'],[10,'10'],[11,'11'],[12,'12'],[13,'13'],[14,'14'],[15,'15']], prefix: '<div class="centered">', suffix: '<\/div>' },
			{ type: 'text', maxlen: 4, prefix: '<div class="centered">', suffix: '<\/div>' },
			{ type: 'select', options: portOptions, prefix: '<div class="centered">', suffix: '<\/div>' },
			{ type: 'select', options: portOptions, prefix: '<div class="centered">', suffix: '<\/div>' },
			{ type: 'select', options: portOptions, prefix: '<div class="centered">', suffix: '<\/div>' },
			{ type: 'select', options: portOptions, prefix: '<div class="centered">', suffix: '<\/div>' },
			{ type: 'select', options: portOptions, prefix: '<div class="centered">', suffix: '<\/div>' }
		];

/* TOMATO64-SKIP-BEGIN */
/* EXTSW-BEGIN */
		cols.push({ type: 'select', options: portOptions, prefix: '<div class="centered">', suffix: '<\/div>' });
/* EXTSW-END */
/* TOMATO64-SKIP-END */

/* TOMATO64-BEGIN */
		cols.push({ type: 'select', options: portOptions, prefix: '<div class="centered">', suffix: '<\/div>' });
		cols.push({ type: 'select', options: portOptions, prefix: '<div class="centered">', suffix: '<\/div>' });
		cols.push({ type: 'select', options: portOptions, prefix: '<div class="centered">', suffix: '<\/div>' });
		cols.push({ type: 'select', options: portOptions, prefix: '<div class="centered">', suffix: '<\/div>' });
/* TOMATO64-END */

		/* Default VLAN */
		cols.push({ type: 'checkbox', prefix: '<div class="centered">', suffix: '<\/div>' });

/* TOMATO64-SKIP-BEGIN */
		var bridgeOptions = [[1,'none'],[2,'WAN0'],[3,'LAN0 (br0)'],[4,'LAN1 (br1)'],[5,'LAN2 (br2)'],[6,'LAN3 (br3)'],[7,'WAN1']];
/* MULTIWAN-BEGIN */
		bridgeOptions.push([8,'WAN2'],[9,'WAN3']);
/* MULTIWAN-END */
/* TOMATO64-SKIP-END */
/* TOMATO64-BEGIN */
		var bridgeOptions = [[1,'none'],[2,'WAN0'],[3,'LAN0 (br0)'],[4,'LAN1 (br1)'],[5,'LAN2 (br2)'],[6,'LAN3 (br3)'],[7,'LAN4 (br4)'],[8,'LAN5 (br5)'],[9,'LAN6 (br6)'],[10,'LAN7 (br7)'],[11,'WAN1']];
/* MULTIWAN-BEGIN */
		bridgeOptions.push([12,'WAN2'],[13,'WAN3']);
/* MULTIWAN-END */
/* TOMATO64-END */
		cols.push({ type: 'select', options: bridgeOptions, prefix: '<div class="centered">', suffix: '<\/div>' });

		this.init('vlan-grid', 'sort', (MAX_VLAN_ID + 1), cols);

/* TOMATO64-SKIP-BEGIN */
		var ethIconScale = 100; /* percentage */
		var ethIconW = Math.round(46 * ethIconScale / 100);
		var ethIconH = Math.round(35 * ethIconScale / 100);

		var headers = ['VLAN', 'VID',
		               '<div id="vport_0"><span class="eth-icon" id="ethsvg_0" data-w="'+ethIconW+'" data-h="'+ethIconH+'"><\/span><\/div>',
		               '<div id="vport_1"><span class="eth-icon" id="ethsvg_1" data-w="'+ethIconW+'" data-h="'+ethIconH+'"><\/span><\/div>',
		               '<div id="vport_2"><span class="eth-icon" id="ethsvg_2" data-w="'+ethIconW+'" data-h="'+ethIconH+'"><\/span><\/div>',
		               '<div id="vport_3"><span class="eth-icon" id="ethsvg_3" data-w="'+ethIconW+'" data-h="'+ethIconH+'"><\/span><\/div>',
		               '<div id="vport_4"><span class="eth-icon" id="ethsvg_4" data-w="'+ethIconW+'" data-h="'+ethIconH+'"><\/span><\/div>'
		];
/* EXTSW-BEGIN */
		headers.push('<div id="vport_5"><span class="eth-icon" id="ethsvg_5" data-w="'+ethIconW+'" data-h="'+ethIconH+'"><\/span><\/div>');
/* EXTSW-END */
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

		for (var i = 0 ; i <= MAX_BRIDGE_ID ; i++) {
			var j = (i == 0) ? '' : i.toString();
/* TOMATO64-SKIP-BEGIN */
			var l = nvram['lan'+j+'_ifnames'].split(' ');
/* TOMATO64-SKIP-END */
/* TOMATO64-BEGIN */
			var l = nvram['lan'+j+'_ifnames_vlan'].split(' ');
/* TOMATO64-END */
/* REMOVE-BEGIN
			alert('lan'+j+'_ifnames='+l);
REMOVE-END */
			for (var k = 0 ; k < l.length; k++) {
/* REMOVE-BEGIN
				alert("bridge br"+i+"=vlan"+parseInt(l[k].replace('vlan','')));
REMOVE-END */
				if (l[k].indexOf('vlan') != -1) {
/* REMOVE-BEGIN
					alert('lan'+j+'_ifname=br'+nvram['lan'+j+'_ifname'].replace('br',''));
REMOVE-END */
					if (nvram['lan'+j+'_ifname'] != '')
						bridged[parseInt(l[k].replace('vlan',''))] = (3 + parseInt(nvram['lan'+j+'_ifname'].replace('br',''))).toString();
					else
						bridged[parseInt(l[k].replace('vlan',''))] = '1';
				}
/* WLAN */
				for (var uidx = 0; uidx < wl_ifaces.length; ++uidx) {
					if (l[k].indexOf(wl_ifaces[uidx][0]) != -1)
						E('_f_bridge_wlan'+uidx+'_to').selectedIndex = i;
				}
			}
		}

/* WAN port */
/* TOMATO64-SKIP-BEGIN */
		bridged[parseInt(nvram['wan_ifnameX'].replace('vlan',''))] = '2';
		bridged[parseInt(nvram['wan2_ifnameX'].replace('vlan',''))] = '7';
/* MULTIWAN-BEGIN */
		bridged[parseInt(nvram['wan3_ifnameX'].replace('vlan',''))] = '8';
		bridged[parseInt(nvram['wan4_ifnameX'].replace('vlan',''))] = '9';
/* MULTIWAN-END */
/* TOMATO64-SKIP-END */

/* TOMATO64-BEGIN */
		bridged[parseInt(nvram['wan_ifnameX_vlan'].replace('vlan',''))] = '2';
		bridged[parseInt(nvram['wan2_ifnameX_vlan'].replace('vlan',''))] = '11';
/* MULTIWAN-BEGIN */
		bridged[parseInt(nvram['wan3_ifnameX_vlan'].replace('vlan',''))] = '12';
		bridged[parseInt(nvram['wan4_ifnameX_vlan'].replace('vlan',''))] = '13';
/* MULTIWAN-END */
/* TOMATO64-END */

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
				for (var j = 0; j < (m.length) ; j++) {
					port[parseInt(m[j].charAt(0))] = '1';
					tagged[parseInt(m[j].charAt(0))] = (((trunk_vlan_supported) || (PORT_VLAN_SUPPORT_OVERRIDE)) && (m[j].indexOf('t') != -1)) ? '1' : '0';
				}

				if (port_vlan_supported) {
					if ((nvram['vlan'+i+'ports']).indexOf('*') != -1)
						SWITCH_INTERNAL_PORT = (nvram['vlan'+i+'ports']).charAt((nvram['vlan'+i+'ports']).indexOf('*') - 1);

					var pt = function(n) {
						return (port[n] == '1') ? ((((trunk_vlan_supported) || (PORT_VLAN_SUPPORT_OVERRIDE)) && (tagged[n] == '1')) ? '2' : '1') : '0';
					}

					var row = [
						i.toString(),
						((nvram['vlan'+i+'vid'] != '') && (nvram['vlan'+i+'vid'] > 0)) ? (nvram['vlan'+i+'vid']).toString() : '0',
						pt(COL_P0N),
						pt(COL_P1N),
						pt(COL_P2N),
						pt(COL_P3N),
						pt(COL_P4N)
					];
/* TOMATO64-SKIP-BEGIN */
/* EXTSW-BEGIN */
					row.push(pt(COL_P5N));
/* EXTSW-END */
/* TOMATO64-SKIP-END */
/* TOMATO64-BEGIN */
					row.push(pt(COL_P5N));
					row.push(pt(COL_P6N));
					row.push(pt(COL_P7N));
					row.push(pt(COL_P8N));
/* TOMATO64-END */
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

	vlg.countWan = function() {
		return this.countElem(COL_BRI, 2);
	}

	vlg.countWan2 = function() {
/* TOMATO64-SKIP-BEGIN */
		return this.countElem(COL_BRI, 7);
/* TOMATO64-SKIP-END */
/* TOMATO64-BEGIN */
		return this.countElem(COL_BRI, 11);
/* TOMATO64-END */
	}

/* MULTIWAN-BEGIN */
	vlg.countWan3 = function() {
/* TOMATO64-SKIP-BEGIN */
		return this.countElem(COL_BRI, 8);
/* TOMATO64-SKIP-END */
/* TOMATO64-BEGIN */
		return this.countElem(COL_BRI, 12);
/* TOMATO64-END */
	}

	vlg.countWan4 = function() {
/* TOMATO64-SKIP-BEGIN */
		return this.countElem(COL_BRI, 9);
/* TOMATO64-SKIP-END */
/* TOMATO64-BEGIN */
		return this.countElem(COL_BRI, 13);
/* TOMATO64-END */
	}
/* MULTIWAN-END */

	vlg.countLan = function(l) {
		return this.countElem(COL_BRI, l + 3);
	}

	vlg.verifyFields = function(row, quiet) {
/* TOMATO64-REMOVE-BEGIN */
		var i, j, old, oldP0, oldP1, oldP2, oldP3, oldP4, oldP5, me, checkNative, valid = 1;
/* TOMATO64-REMOVE-END */
/* TOMATO64-BEGIN */
		var i, j, old, oldP0, oldP1, oldP2, oldP3, oldP4, oldP5, oldP6, oldP7, oldP8, me, checkNative, valid = 1;
/* TOMATO64-END */
		var f = fields.getAll(row);

		for (i = 0; i<= MAX_VLAN_ID ; i++)
			f[COL_VID].options[i].disabled = (this.countVID(i) > 0);

		for (i = 0; i <= MAX_BRIDGE_ID; i++) {
			j = (i == 0) ? '' : i.toString();
			f[COL_BRI].options[i + 2].disabled = (nvram['lan'+j+'_ifname'].length < 1);
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
		enforcePortState(COL_P0);
		enforcePortState(COL_P1);
		enforcePortState(COL_P2);
		enforcePortState(COL_P3);
		enforcePortState(COL_P4);
/* TOMATO64-SKIP-BEGIN */
/* EXTSW-BEGIN */
		enforcePortState(COL_P5);
/* EXTSW-END */
/* TOMATO64-SKIP-END */
/* TOMATO64-BEGIN */
		enforcePortState(COL_P5);
		enforcePortState(COL_P6);
		enforcePortState(COL_P7);
		enforcePortState(COL_P8);
/* TOMATO64-END */

		/* Modifications to enable Native VLAN support (allow one untagged vlan per port) by default */
		var err_vlan = 'Only one untagged VLAN per port is allowed (Native VLAN)';
		old = ((row == this.editor) && this.source) ? this.source.getRowData() : null;
		oldP0 = (old && (old.length > COL_P0)) ? old[COL_P0] : '0';
		oldP1 = (old && (old.length > COL_P1)) ? old[COL_P1] : '0';
		oldP2 = (old && (old.length > COL_P2)) ? old[COL_P2] : '0';
		oldP3 = (old && (old.length > COL_P3)) ? old[COL_P3] : '0';
		oldP4 = (old && (old.length > COL_P4)) ? old[COL_P4] : '0';
/* TOMATO64-SKIP-BEGIN */
/* EXTSW-BEGIN */
		oldP5 = (old && (old.length > COL_P5)) ? old[COL_P5] : '0';
/* EXTSW-END */
/* TOMATO64-SKIP-END */
/* TOMATO64-BEGIN */
		oldP5 = (old && (old.length > COL_P5)) ? old[COL_P5] : '0';
		oldP6 = (old && (old.length > COL_P6)) ? old[COL_P6] : '0';
		oldP7 = (old && (old.length > COL_P7)) ? old[COL_P7] : '0';
		oldP8 = (old && (old.length > COL_P8)) ? old[COL_P8] : '0';
/* TOMATO64-END */
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

		checkNative(COL_P0, oldP0);
		checkNative(COL_P1, oldP1);
		checkNative(COL_P2, oldP2);
		checkNative(COL_P3, oldP3);
		checkNative(COL_P4, oldP4);
/* TOMATO64-SKIP-BEGIN */
/* EXTSW-BEGIN */
		checkNative(COL_P5, oldP5);
/* EXTSW-END */
/* TOMATO64-SKIP-END */
/* TOMATO64-BEGIN */
		checkNative(COL_P5, oldP5);
		checkNative(COL_P6, oldP6);
		checkNative(COL_P7, oldP7);
		checkNative(COL_P8, oldP8);
/* TOMATO64-END */

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

		if ((this.countWan() > 0) && (f[COL_BRI].selectedIndex == 1)) {
			ferror.set(f[COL_BRI], 'Only one VID can be used as WAN0 at any time', quiet);
			valid = 0;
		}
		else
			ferror.clear(f[COL_BRI]);

/* TOMATO64-SKIP-BEGIN */
		if ((this.countWan2() > 0) && (f[COL_BRI].selectedIndex == 6)) {
/* TOMATO64-SKIP-END */
/* TOMATO64-BEGIN */
		if ((this.countWan2() > 0) && (f[COL_BRI].selectedIndex == 10)) {
/* TOMATO64-END */
			ferror.set(f[COL_BRI], 'Only one VID can be used as WAN1 at any time', quiet);
			valid = 0;
		}
		else
			ferror.clear(f[COL_BRI]);

/* MULTIWAN-BEGIN */
/* TOMATO64-SKIP-BEGIN */
		if ((this.countWan3() > 0) && (f[COL_BRI].selectedIndex == 7)) {
/* TOMATO64-SKIP-END */
/* TOMATO64-BEGIN */
		if ((this.countWan3() > 0) && (f[COL_BRI].selectedIndex == 11)) {
/* TOMATO64-END */
			ferror.set(f[COL_BRI], 'Only one VID can be used as WAN2 at any time', quiet);
			valid = 0;
		}
		else
			ferror.clear(f[COL_BRI]);

/* TOMATO64-SKIP-BEGIN */
		if ((this.countWan4() > 0) && (f[COL_BRI].selectedIndex == 8)) {
/* TOMATO64-SKIP-END */
/* TOMATO64-BEGIN */
		if ((this.countWan4() > 0) && (f[COL_BRI].selectedIndex == 12)) {
/* TOMATO64-END */
			ferror.set(f[COL_BRI], 'Only one VID can be used as WAN3 at any time', quiet);
			valid = 0;
		}
		else
			ferror.clear(f[COL_BRI]);
/* MULTIWAN-END */

/* TOMATO64-SKIP-BEGIN */
		for (i = 0; i < 4; i++) {
/* TOMATO64-SKIP-END */
/* TOMATO64-BEGIN */
		for (i = 0; i < 8; i++) {
/* TOMATO64-END */
			if ((this.countLan(i) > 0) && (f[COL_BRI].selectedIndex == (i + 2))) {
				ferror.set(f[COL_BRI], 'One and only one VID can be used for LAN'+i+' (br'+i+') at any time', quiet);
				valid = 0;
			}
			else
				ferror.clear(f[COL_BRI]);
		}

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
			((data[COL_MAP].toString() == '') || (data[COL_MAP].toString() == '0')) ? (parseInt(E('_vlan0tag').value) * 1 + data[COL_VID] * 1).toString() : data[COL_MAP].toString(),
/* RTNPLUS-NO-END */
/* RTNPLUS-BEGIN */
			((data[COL_MAP].toString() == '') || (data[COL_MAP].toString() == '0')) ? (data[COL_VID] * 1).toString() : data[COL_MAP].toString(),
/* RTNPLUS-END */
			pv(data[COL_P0]),
			pv(data[COL_P1]),
			pv(data[COL_P2]),
			pv(data[COL_P3]),
			pv(data[COL_P4])
		];
/* TOMATO64-SKIP-BEGIN */
/* EXTSW-BEGIN */
		view.push(pv(data[COL_P5]));
/* EXTSW-END */
/* TOMATO64-SKIP-END */
/* TOMATO64-BEGIN */
		view.push(pv(data[COL_P5]));
		view.push(pv(data[COL_P6]));
		view.push(pv(data[COL_P7]));
		view.push(pv(data[COL_P8]));
/* TOMATO64-END */
		view.push(
			(data[COL_VID_DEF].toString() != '0') ? '🌑' : '',
/* TOMATO64-SKIP-BEGIN */
			['','WAN0','LAN0 (br0)','LAN1 (br1)','LAN2 (br2)','LAN3 (br3)','WAN1'
/* TOMATO64-SKIP-END */
/* TOMATO64-BEGIN */
			['','WAN0','LAN0 (br0)','LAN1 (br1)','LAN2 (br2)','LAN3 (br3)','LAN4 (br4)','LAN5 (br5)','LAN6 (br6)','LAN7 (br7)','WAN1'
/* TOMATO64-END */
/* MULTIWAN-BEGIN */
			,'WAN2','WAN3'
/* MULTIWAN-END */
			][data[COL_BRI] - 1]
		);
		return view;
	}

	vlg.dataToFieldValues = function (data) {
		var values = [data[COL_VID], data[COL_MAP], data[COL_P0], data[COL_P1], data[COL_P2], data[COL_P3], data[COL_P4]];
/* TOMATO64-SKIP-BEGIN */
/* EXTSW-BEGIN */
		values.push(data[COL_P5]);
/* EXTSW-END */
/* TOMATO64-SKIP-END */
/* TOMATO64-BEGIN */
		values.push(data[COL_P5]);
		values.push(data[COL_P6]);
		values.push(data[COL_P7]);
		values.push(data[COL_P8]);
/* TOMATO64-END */
		values.push((data[COL_VID_DEF] != 0) ? 1 : 0, data[COL_BRI]);
		return values;
	}

	vlg.fieldValuesToData = function(row) {
		var f = fields.getAll(row);
		var data = [f[COL_VID].value,
			f[COL_MAP].value,
			parseInt(f[COL_P0].value, 10) || 0,
			parseInt(f[COL_P1].value, 10) || 0,
			parseInt(f[COL_P2].value, 10) || 0,
			parseInt(f[COL_P3].value, 10) || 0,
			parseInt(f[COL_P4].value, 10) || 0
		];
/* TOMATO64-SKIP-BEGIN */
/* EXTSW-BEGIN */
		data.push(parseInt(f[COL_P5].value, 10) || 0);
/* EXTSW-END */
/* TOMATO64-SKIP-END */
/* TOMATO64-BEGIN */
		data.push(parseInt(f[COL_P5].value, 10) || 0);
		data.push(parseInt(f[COL_P6].value, 10) || 0);
		data.push(parseInt(f[COL_P7].value, 10) || 0);
		data.push(parseInt(f[COL_P8].value, 10) || 0);
/* TOMATO64-END */
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

		for (var i = 0; i <= MAX_BRIDGE_ID; i++) {
			var j = ((i == 0) ? '' : i.toString());
			f[COL_BRI].options[i + 2].disabled = (nvram['lan'+j+'_ifname'].length < 1);
		}

		f[COL_MAP].value = '0';

		f[COL_VID].selectedIndex = 0;
		var t = MAX_VLAN_ID;
		while ((this.countVID(f[COL_VID].selectedIndex) > 0) && (t > 0)) {
			f[COL_VID].selectedIndex = (f[COL_VID].selectedIndex%(MAX_VLAN_ID)) + 1;
			t--;
		}

		for (var i = 0; i <= MAX_VLAN_ID ; i++)
			f[COL_VID].options[i].disabled = (this.countVID(i) > 0);

		f[COL_P0].value = '0';
		f[COL_P1].value = '0';
		f[COL_P2].value = '0';
		f[COL_P3].value = '0';
		f[COL_P4].value = '0';
/* TOMATO64-SKIP-BEGIN */
/* EXTSW-BEGIN */
		f[COL_P5].value = '0';
/* EXTSW-END */
/* TOMATO64-SKIP-END */
/* TOMATO64-BEGIN */
		f[COL_P5].value = '0';
		f[COL_P6].value = '0';
		f[COL_P7].value = '0';
		f[COL_P8].value = '0';
/* TOMATO64-END */
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
	PORT_VLAN_SUPPORT_OVERRIDE=(E('_f_trunk_vlan_so').checked ? 1 : 0);
/* MIPSR2P-NO-END */
	for (var uidx = 0; uidx < wl_ifaces.length; ++uidx) {
		var wlan = E('_f_bridge_wlan'+uidx+'_to');
		if (nvram.lan_ifname.length < 1)
			wlan.options[0].disabled = 1;

		if (nvram.lan1_ifname.length < 1)
			wlan.options[1].disabled = 1;

		if (nvram.lan2_ifname.length < 1)
			wlan.options[2].disabled = 1;

		if (nvram.lan3_ifname.length < 1)
			wlan.options[3].disabled = 1;
	}

/* RTNPLUS-NO-BEGIN */
	if (!v_range('_vlan0tag', quiet, 0, 4080))
		return 0;

	var e = E('_vlan0tag');
	var v = parseInt(e.value);
	e.value = v - (v % 16);
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

	for (i = 0; i <= MAX_BRIDGE_ID; i++) {
		j = (i == 0) ? '' : i;
		fom['lan'+j+'_ifnames'].value = '';
/* TOMATO64-BEGIN */
		fom['lan'+j+'_ifnames_vlan'].value = '';
/* TOMATO64-END */
	}

	for (i = 1; i <= MAXWAN_NUM; ++i) {
		j = (i > 1) ? i : '';
		fom['wan'+j+'_ifnameX'].value = '';
/* TOMATO64-BEGIN */
		fom['wan'+j+'_ifnameX_vlan'].value = '';
/* TOMATO64-END */
		fom['wan'+j+'_iface'].value = '';
		fom['wan'+j+'_iface'].disabled = 1;
		fom['wan'+j+'_ifname'].value = '';
		fom['wan'+j+'_ifname'].disabled = 1;
		fom['wan'+j+'_hwaddr'].value = '';
		fom['wan'+j+'_hwaddr'].disabled = 1;
		fom['wan'+j+'_proto'].value = '';
		fom['wan'+j+'_proto'].disabled = 1;
	}

	d = vlg.getAllData();

	for (i = 0; i < d.length; ++i) {
		p = '';
		p += (d[i][COL_P0].toString() != '0') ? COL_P0N : '';
		p += (((trunk_vlan_supported || PORT_VLAN_SUPPORT_OVERRIDE)) && (d[i][COL_P0].toString() == '2')) ? 't' : '';
		p += trailingSpace(p);

		p += (d[i][COL_P1].toString() != '0') ? COL_P1N : '';
		p += (((trunk_vlan_supported || PORT_VLAN_SUPPORT_OVERRIDE)) && (d[i][COL_P1].toString() == '2')) ? 't' : '';
		p += trailingSpace(p);

		p += (d[i][COL_P2].toString() != '0') ? COL_P2N : '';
		p += (((trunk_vlan_supported || PORT_VLAN_SUPPORT_OVERRIDE)) && (d[i][COL_P2].toString() == '2')) ? 't' : '';
		p += trailingSpace(p);

		p += (d[i][COL_P3].toString() != '0') ? COL_P3N : '';
		p += (((trunk_vlan_supported || PORT_VLAN_SUPPORT_OVERRIDE)) && (d[i][COL_P3].toString() == '2')) ? 't' : '';
		p += trailingSpace(p);

		p += (d[i][COL_P4].toString() != '0') ? COL_P4N : '';
		p += (((trunk_vlan_supported || PORT_VLAN_SUPPORT_OVERRIDE)) && (d[i][COL_P4].toString() == '2')) ? 't' : '';
		p += trailingSpace(p);

/* TOMATO64-SKIP-BEGIN */
/* EXTSW-BEGIN */
		p += (d[i][COL_P5].toString() != '0') ? COL_P5N : '';
		p += (((trunk_vlan_supported || PORT_VLAN_SUPPORT_OVERRIDE)) && (d[i][COL_P5].toString() == '2')) ? 't' : '';
		p += trailingSpace(p);
/* EXTSW-END */
/* TOMATO64-SKIP-END */

/* TOMATO64-BEGIN */
		p += (d[i][COL_P5].toString() != '0') ? COL_P5N : '';
		p += (((trunk_vlan_supported || PORT_VLAN_SUPPORT_OVERRIDE)) && (d[i][COL_P5].toString() == '2')) ? 't' : '';
		p += trailingSpace(p);

		p += (d[i][COL_P6].toString() != '0') ? COL_P6N : '';
		p += (((trunk_vlan_supported || PORT_VLAN_SUPPORT_OVERRIDE)) && (d[i][COL_P6].toString() == '2')) ? 't' : '';
		p += trailingSpace(p);

		p += (d[i][COL_P7].toString() != '0') ? COL_P7N : '';
		p += (((trunk_vlan_supported || PORT_VLAN_SUPPORT_OVERRIDE)) && (d[i][COL_P7].toString() == '2')) ? 't' : '';
		p += trailingSpace(p);

		p += (d[i][COL_P8].toString() != '0') ? COL_P8N : '';
		p += (((trunk_vlan_supported || PORT_VLAN_SUPPORT_OVERRIDE)) && (d[i][COL_P8].toString() == '2')) ? 't' : '';
		p += trailingSpace(p);
/* TOMATO64-END */

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

/* TOMATO64-SKIP-BEGIN */
		fom['wan_ifnameX'].value += (d[i][COL_BRI] == '2') ? 'vlan'+d[i][0] : '';
		fom['lan_ifnames'].value += (d[i][COL_BRI] == '3') ? 'vlan'+d[i][0] : '';
/* REMOVE-BEGIN
		fom['lan_ifnames'].value += trailingSpace(fom['lan_ifnames'].value);
		alert('vlan'+d[i][0]+'ports='+fom['vlan'+d[i][0]+'ports'].value+'\nvlan'+d[i][0]+'hwname='+fom['vlan'+d[i][0]+'hwname'].value);
REMOVE-END */
		fom['lan1_ifnames'].value += (d[i][COL_BRI] == '4') ? 'vlan'+d[i][0] : '';
		fom['lan2_ifnames'].value += (d[i][COL_BRI] == '5') ? 'vlan'+d[i][0] : '';
		fom['lan3_ifnames'].value += (d[i][COL_BRI] == '6') ? 'vlan'+d[i][0] : '';
		fom['wan2_ifnameX'].value += (d[i][COL_BRI] == '7') ? 'vlan'+d[i][0] : '';
/* MULTIWAN-BEGIN */
		fom['wan3_ifnameX'].value += (d[i][COL_BRI] == '8') ? 'vlan'+d[i][0] : '';
		fom['wan4_ifnameX'].value += (d[i][COL_BRI] == '9') ? 'vlan'+d[i][0] : '';
/* MULTIWAN-END */
/* TOMATO64-SKIP-END */

/* TOMATO64-BEGIN */
		fom['wan_ifnameX_vlan'].value += (d[i][COL_BRI] == '2') ? 'vlan'+d[i][0] : '';
		fom['lan_ifnames_vlan'].value += (d[i][COL_BRI] == '3') ? 'vlan'+d[i][0] : '';

		for(var port=0; port <= MAX_PORT_ID; port++) {
			if (d[i][2 + port] == 1) {
				fom['wan_ifnameX'].value += (d[i][COL_BRI] == '2') ? 'eth' + port : '';
				fom['lan_ifnames'].value += (d[i][COL_BRI] == '3') ? 'eth' + port + ' ' : '';
				fom['lan1_ifnames'].value += (d[i][COL_BRI] == '4') ? 'eth' + port + ' ' : '';
				fom['lan2_ifnames'].value += (d[i][COL_BRI] == '5') ? 'eth' + port + ' ' : '';
				fom['lan3_ifnames'].value += (d[i][COL_BRI] == '6') ? 'eth' + port + ' ' : '';
				fom['lan4_ifnames'].value += (d[i][COL_BRI] == '7') ? 'eth' + port + ' ' : '';
				fom['lan5_ifnames'].value += (d[i][COL_BRI] == '8') ? 'eth' + port + ' ' : '';
				fom['lan6_ifnames'].value += (d[i][COL_BRI] == '9') ? 'eth' + port + ' ' : '';
				fom['lan7_ifnames'].value += (d[i][COL_BRI] == '10') ? 'eth' + port + ' ' : '';
				fom['wan2_ifnameX'].value += (d[i][COL_BRI] == '11') ? 'eth' + port : '';
/* MULTIWAN-BEGIN */
				fom['wan3_ifnameX'].value += (d[i][COL_BRI] == '12') ? 'eth' + port : '';
				fom['wan4_ifnameX'].value += (d[i][COL_BRI] == '13') ? 'eth' + port : '';
/* MULTIWAN-END */
			}
			else if (d[i][2 + port] == 2) {
				var vid = ((d[i][COL_MAP].toString() != '') && (d[i][COL_MAP].toString() != '0')) ? d[i][COL_MAP].toString() : d[i][COL_VID].toString();
				fom['vlan'+d[i][COL_VID]+'hwname'].value += 'eth' + port + ' ';
				fom['wan_ifnameX'].value += (d[i][COL_BRI] == '2') ? 'eth' + port + '.' + vid : '';
				fom['lan_ifnames'].value += (d[i][COL_BRI] == '3') ? 'eth' + port + '.' + vid + ' ' : '';
				fom['lan1_ifnames'].value += (d[i][COL_BRI] == '4') ? 'eth' + port + '.' + vid + ' ' : '';
				fom['lan2_ifnames'].value += (d[i][COL_BRI] == '5') ? 'eth' + port + '.' + vid + ' ' : '';
				fom['lan3_ifnames'].value += (d[i][COL_BRI] == '6') ? 'eth' + port + '.' + vid + ' ' : '';
				fom['lan4_ifnames'].value += (d[i][COL_BRI] == '7') ? 'eth' + port + '.' + vid + ' ' : '';
				fom['lan5_ifnames'].value += (d[i][COL_BRI] == '8') ? 'eth' + port + '.' + vid + ' ' : '';
				fom['lan6_ifnames'].value += (d[i][COL_BRI] == '9') ? 'eth' + port + '.' + vid + ' ' : '';
				fom['lan7_ifnames'].value += (d[i][COL_BRI] == '10') ? 'eth' + port + '.' + vid + ' ' : '';
				fom['wan2_ifnameX'].value += (d[i][COL_BRI] == '11') ? 'eth' + port + '.' + vid : '';
/* MULTIWAN-BEGIN */
				fom['wan3_ifnameX'].value += (d[i][COL_BRI] == '12') ? 'eth' + port + '.' + vid : '';
				fom['wan4_ifnameX'].value += (d[i][COL_BRI] == '13') ? 'eth' + port + '.' + vid : '';
/* MULTIWAN-END */
			}
		}
		fom['vlan'+d[i][COL_VID]+'hwname'].value = fom['vlan'+d[i][COL_VID]+'hwname'].value.trim();
		fom['lan_ifnames'].value = fom['lan_ifnames'].value.trim();
		fom['lan1_ifnames'].value = fom['lan1_ifnames'].value.trim();
		fom['lan2_ifnames'].value = fom['lan2_ifnames'].value.trim();
		fom['lan3_ifnames'].value = fom['lan3_ifnames'].value.trim();
		fom['lan4_ifnames'].value = fom['lan4_ifnames'].value.trim();
		fom['lan5_ifnames'].value = fom['lan5_ifnames'].value.trim();
		fom['lan6_ifnames'].value = fom['lan6_ifnames'].value.trim();
		fom['lan7_ifnames'].value = fom['lan7_ifnames'].value.trim();

		fom['lan1_ifnames_vlan'].value += (d[i][COL_BRI] == '4') ? 'vlan'+d[i][0] : '';
		fom['lan2_ifnames_vlan'].value += (d[i][COL_BRI] == '5') ? 'vlan'+d[i][0] : '';
		fom['lan3_ifnames_vlan'].value += (d[i][COL_BRI] == '6') ? 'vlan'+d[i][0] : '';
		fom['lan4_ifnames_vlan'].value += (d[i][COL_BRI] == '7') ? 'vlan'+d[i][0] : '';
		fom['lan5_ifnames_vlan'].value += (d[i][COL_BRI] == '8') ? 'vlan'+d[i][0] : '';
		fom['lan6_ifnames_vlan'].value += (d[i][COL_BRI] == '9') ? 'vlan'+d[i][0] : '';
		fom['lan7_ifnames_vlan'].value += (d[i][COL_BRI] == '10') ? 'vlan'+d[i][0] : '';
		fom['wan2_ifnameX_vlan'].value += (d[i][COL_BRI] == '11') ? 'vlan'+d[i][0] : '';
/* MULTIWAN-BEGIN */
		fom['wan3_ifnameX_vlan'].value += (d[i][COL_BRI] == '12') ? 'vlan'+d[i][0] : '';
		fom['wan4_ifnameX_vlan'].value += (d[i][COL_BRI] == '13') ? 'vlan'+d[i][0] : '';
/* MULTIWAN-END */
/* TOMATO64-END */
	}

	/* count active WANs / wipe out relevant fields for inactive or just disabled WAN - needed in various places for the proper operation of FW */
	k = 0;
	for (i = 1; i <= MAXWAN_NUM; ++i) {
		j = (i > 1) ? i : '';
/* TOMATO64-SKIP-BEGIN */
		if (fom['wan'+j+'_ifnameX'].value.length > 1)
/* TOMATO64-SKIP-END */
/* TOMATO64-BEGIN */
		if (fom['wan'+j+'_ifnameX'].value.length > 1 || fom['wan'+j+'_ifnameX_vlan'].value.length > 1)
/* TOMATO64-END */
			k++;
		else {
			fom['wan'+j+'_iface'].disabled = 0;
			fom['wan'+j+'_iface'].value = '';
			fom['wan'+j+'_ifname'].disabled = 0;
			fom['wan'+j+'_ifname'].value = '';
			fom['wan'+j+'_hwaddr'].disabled = 0;
			fom['wan'+j+'_hwaddr'].value = '';
			fom['wan'+j+'_proto'].disabled = 0;
			fom['wan'+j+'_proto'].value = 'disabled';
		}
	}
	//fom.mwan_num.value = (k < 1 ? 1 : k);
	fom.mwan_num.value = 1; /* just reset mwan_num to 1 to avoid problems */

	for (i = 0; i < wl_ifaces.length; ++i) {
		var wlan = E('_f_bridge_wlan'+i+'_to');
/* REMOVE-BEGIN
		alert(wlan.selectedIndex);
REMOVE-END */
		switch (parseInt(wlan.selectedIndex)) {
			case 0:
				fom['lan_ifnames'].value += ' '+wl_ifaces[i][0];
			break;
			case 1:
				fom['lan1_ifnames'].value += ' '+wl_ifaces[i][0];
			break;
			case 2:
				fom['lan2_ifnames'].value += ' '+wl_ifaces[i][0];
			break;
			case 3:
				fom['lan3_ifnames'].value += ' '+wl_ifaces[i][0];
			break;
		}
	}
/* REMOVE-BEGIN
	var lif = nvram['lan_ifnames'].split(' ');
	for (var j = 0; j < lif.length; j++) {
		fom['lan_ifnames'].value += (lif[j].indexOf('vlan') != -1) ? '' : lif[j];
		fom['lan_ifnames'].value += trailingSpace(fom['lan_ifnames'].value);
	}
	alert('lan_ifnames='+fom['lan_ifnames'].value+'\n' +
		'lan1_ifnames='+fom['lan1_ifnames'].value+'\n' +
		'lan2_ifnames='+fom['lan2_ifnames'].value+'\n' +
		'lan3_ifnames='+fom['lan3_ifnames'].value);
REMOVE-END */

	/* Prevent vlan reset to default at init */
	fom['manual_boot_nv'].value = 1;

	e = E('footer-msg');

	if (vlg.countWan() != 1) {
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
		var c;
		if (((c = cookie.get(cprefix+'_notes_vis')) != null) && (c == '1'))
			toggleVisibility(cprefix, 'notes');
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
<input type="hidden" name="vlan0ports">
<input type="hidden" name="vlan1ports">
<input type="hidden" name="vlan2ports">
<input type="hidden" name="vlan3ports">
<input type="hidden" name="vlan4ports">
<input type="hidden" name="vlan5ports">
<input type="hidden" name="vlan6ports">
<input type="hidden" name="vlan7ports">
<input type="hidden" name="vlan8ports">
<input type="hidden" name="vlan9ports">
<input type="hidden" name="vlan10ports">
<input type="hidden" name="vlan11ports">
<input type="hidden" name="vlan12ports">
<input type="hidden" name="vlan13ports">
<input type="hidden" name="vlan14ports">
<input type="hidden" name="vlan15ports">
<input type="hidden" name="vlan0hwname">
<input type="hidden" name="vlan1hwname">
<input type="hidden" name="vlan2hwname">
<input type="hidden" name="vlan3hwname">
<input type="hidden" name="vlan4hwname">
<input type="hidden" name="vlan5hwname">
<input type="hidden" name="vlan6hwname">
<input type="hidden" name="vlan7hwname">
<input type="hidden" name="vlan8hwname">
<input type="hidden" name="vlan9hwname">
<input type="hidden" name="vlan10hwname">
<input type="hidden" name="vlan11hwname">
<input type="hidden" name="vlan12hwname">
<input type="hidden" name="vlan13hwname">
<input type="hidden" name="vlan14hwname">
<input type="hidden" name="vlan15hwname">
<input type="hidden" name="wan_ifnameX">
<!-- TOMATO64-BEGIN -->
<input type="hidden" name="wan_ifnameX_vlan">
<!-- TOMATO64-END -->
<input type="hidden" name="wan2_ifnameX">
<!-- TOMATO64-BEGIN -->
<input type="hidden" name="wan2_ifnameX_vlan">
<!-- TOMATO64-END -->
<!-- MULTIWAN-BEGIN -->
<input type="hidden" name="wan3_ifnameX">
<!-- TOMATO64-BEGIN -->
<input type="hidden" name="wan3_ifnameX_vlan">
<!-- TOMATO64-END -->
<input type="hidden" name="wan4_ifnameX">
<!-- TOMATO64-BEGIN -->
<input type="hidden" name="wan4_ifnameX_vlan">
<!-- TOMATO64-END -->
<input type="hidden" name="wan3_iface" value="" disabled="disabled">
<input type="hidden" name="wan4_iface" value="" disabled="disabled">
<input type="hidden" name="wan3_ifname" value="" disabled="disabled">
<input type="hidden" name="wan4_ifname" value="" disabled="disabled">
<input type="hidden" name="wan3_hwaddr" value="" disabled="disabled">
<input type="hidden" name="wan4_hwaddr" value="" disabled="disabled">
<input type="hidden" name="wan3_proto" value="" disabled="disabled">
<input type="hidden" name="wan4_proto" value="" disabled="disabled">
<!-- MULTIWAN-END -->
<input type="hidden" name="wan_iface" value="" disabled="disabled">
<input type="hidden" name="wan2_iface" value="" disabled="disabled">
<input type="hidden" name="wan_ifname" value="" disabled="disabled">
<input type="hidden" name="wan2_ifname" value="" disabled="disabled">
<input type="hidden" name="wan_hwaddr" value="" disabled="disabled">
<input type="hidden" name="wan2_hwaddr" value="" disabled="disabled">
<input type="hidden" name="wan_proto" value="" disabled="disabled">
<input type="hidden" name="wan2_proto" value="" disabled="disabled">
<input type="hidden" name="mwan_num">
<input type="hidden" name="manual_boot_nv">
<input type="hidden" name="lan_ifnames">
<input type="hidden" name="lan1_ifnames">
<input type="hidden" name="lan2_ifnames">
<input type="hidden" name="lan3_ifnames">
<!-- MIPSR2P-NO-BEGIN -->
<input type="hidden" name="trunk_vlan_so">
<!-- MIPSR2P-NO-END -->
<!-- TOMATO64-BEGIN -->
<input type="hidden" name="lan4_ifnames">
<input type="hidden" name="lan5_ifnames">
<input type="hidden" name="lan6_ifnames">
<input type="hidden" name="lan7_ifnames">
<input type="hidden" name="lan_ifnames_vlan">
<input type="hidden" name="lan1_ifnames_vlan">
<input type="hidden" name="lan2_ifnames_vlan">
<input type="hidden" name="lan3_ifnames_vlan">
<input type="hidden" name="lan4_ifnames_vlan">
<input type="hidden" name="lan5_ifnames_vlan">
<input type="hidden" name="lan6_ifnames_vlan">
<input type="hidden" name="lan7_ifnames_vlan">
<!-- TOMATO64-END -->
<input type="hidden" name="vlan0vid">
<input type="hidden" name="vlan1vid">
<input type="hidden" name="vlan2vid">
<input type="hidden" name="vlan3vid">
<input type="hidden" name="vlan4vid">
<input type="hidden" name="vlan5vid">
<input type="hidden" name="vlan6vid">
<input type="hidden" name="vlan7vid">
<input type="hidden" name="vlan8vid">
<input type="hidden" name="vlan9vid">
<input type="hidden" name="vlan10vid">
<input type="hidden" name="vlan11vid">
<input type="hidden" name="vlan12vid">
<input type="hidden" name="vlan13vid">
<input type="hidden" name="vlan14vid">
<input type="hidden" name="vlan15vid">

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
				{ title: 'First 802.1Q VLAN tag', name: 'vlan0tag', type: 'text', maxlen: 4, size: 6, value: fixInt(nvram.vlan0tag, 0, 4080, 0), suffix: '&nbsp; <small><i>(range: 0 - 4080; must be a multiple of 16; set to 0 to disable)<\/i><\/small>' }
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
								options: [[0,'LAN0 (br0)'],[1,'LAN1 (br1)'],[2,'LAN2 (br2)'],[3,'LAN3 (br3)'],[4,'none']], suffix: '&nbsp;&nbsp;⇔&nbsp; SSID: '+ssid, value: 4, prefix: '⇔ &nbsp;&nbsp;&nbsp;' } );
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

	<div class="section-title">Notes <small><i><a href="javascript:toggleVisibility(cprefix,'notes');" id="toggleLink-notes"><span id="sesdiv_notes_showhide">(Show)</span></a></i></small></div>
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
			<li><b>Native VLAN</b>: Default VLAN for untagged ingress frames = 🌑</li>
			<li><b>Bridge</b>: One VLAN per bridge. WAN bridge (logical) ≠ WAN port (physical)</li>
		</ul>
		<br>
<!-- RTNPLUS-NO-BEGIN -->
		<div><i>VID Offset:</i> First 802.1Q VLAN tag to be used as <i>base/initial tag/VID</i> for VLAN and VID assignments. This allows using VIDs larger than 15 on (older) devices, in contiguous blocks/ranges with up to 16 VLANs/VIDs. Set to '0' (zero) to disable this feature and VLANs will have the very same/identical value for its VID, as usual (from 0 to 15).</div>
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
			<li>Default VID: 0 (some releases) or 1</li>
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
