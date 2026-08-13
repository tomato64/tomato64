<!DOCTYPE html>
<!--
	Tomato GUI
	Copyright (C) 2006-2010 Jonathan Zarate
	http://www.polarcloud.com/tomato/

	For use with Tomato Firmware only.
	No part of this file may be used without permission.
-->
<html lang="en-GB">
<head>
<meta http-equiv="content-type" content="text/html;charset=utf-8">
<meta name="robots" content="noindex,nofollow">
<title>[<% ident(); %>] Advanced: MAC Address</title>
<link rel="stylesheet" type="text/css" href="tomato.css?rel=<% version(); %>">
<% css(); %>
<script src="tomato.js?rel=<% version(); %>"></script>
<script src="wireless.jsx?_http_id=<% nv(http_id); %>"></script>

<script>
/* TOMATO64-BEGIN */
/* Tomato64 doesn't handle wifi macs on this page */
wl_ifaces=[];
/* TOMATO64-END */

//	<% nvram("lan_hwaddr,wan_mac,mwan_num,wl_macaddr,wl_hwaddr,wl_nband"); %>

var cprefix = 'advanced_mac';

function et0plus(plus) {
	var mac = nvram.lan_hwaddr.split(':');
	if (mac.length != 6) return '';
	while (plus-- > 0) {
		for (var i = 5; i >= 3; --i) {
			var n = (parseInt(mac[i], 16) + 1) & 0xFF;
			mac[i] = n.hex(2);
			if (n != 0) break;
		}
	}

	return mac.join(':');
}

function defmac(which) {
	for (var uidx = 1; uidx <= MAXWAN_NUM; ++uidx) {
		var u = (uidx > 1) ? uidx : '';
		if (which == 'wan'+u)
			return et0plus(15 + uidx);
	}
	if (which.indexOf('wl') == 0) {
/* REMOVE-BEGIN
// align to wlconf setup AND Tomato64 initial mac setup
REMOVE-END */
		var unit, subunit, t, v;
		unit = which.substr(2, 1) * 1;
		if (which.indexOf('.') < 0) { /* wlX */
			subunit = 0;
		}
		else { /* wlX.Y */
			subunit = which.substr(which.indexOf('.')+1, 1) * 1;
		}
		t = et0plus(2 + unit * 4 + subunit).split(':'); /* do not overlap with VIFs (4x VIFs for each wl radio unit) */
		v = (parseInt(t[0], 16) | ((subunit > 0) ? 2 : 0)) & 0xFF; /* set local bit for our VIF base addr */
		t[0] = v.hex(2);
		return t.join(':');
	}
}

function bdefault(which) {
	E('_f_' + which + '_hwaddr').value = defmac(which);
	verifyFields(null, true);
}

function brand(which, biaonly) {
	var mac;
	var i;

	var UAA_validILBit=['2','6','A','E'];
	mac = E('_f_'+which+'_hwaddr').value.split(':');
	for (i = 5; i > (biaonly ? 2 : 0); --i)
		mac[i] = Math.floor(Math.random() * 255).hex(2);

	if (!biaonly) {
		/* Let's make sure UL Bit in MAC is correctly set Unicast */
		mac[0] = mac[0].substr(0,1) + UAA_validILBit[Math.floor(Math.random() * UAA_validILBit.length)];
	}

	E('_f_' + which + '_hwaddr').value = mac.join(':');
	verifyFields(null, true);
}

function bclone(which) {
	E('_f_' + which + '_hwaddr').value = '<% compmac(); %>';
	verifyFields(null, true);
}

function checkUniqueMac() {
	var uidx, uidx2, u1, u2, a1, a2;
	var retValue = 1;

	for (uidx = 1; uidx <= nvram.mwan_num; ++uidx) {
		u1 = (uidx > 1) ? uidx : '';
		a1 = E('_f_wan'+u1+'_hwaddr');
		for (uidx2 = uidx + 1; uidx2 <= nvram.mwan_num; ++uidx2) {
			u2 = (uidx2 > 1) ? uidx2 : '';
			a2 = E('_f_wan'+u2+'_hwaddr');
			if (a1 && a2 && (a1.value == a2.value)) {
				ferror.set(a1, 'Addresses must be unique', true);
				ferror.set(a2, 'Addresses must be unique', true);
				retValue = 0;
			}
		}
	}

	for (uidx = 0; uidx < wl_ifaces.length; ++uidx) {
		u1 = wl_fface(uidx);
		a1 = E('_f_wl'+u1+'_hwaddr');
		for (uidx2 = uidx + 1; uidx2 < wl_ifaces.length; ++uidx2) {
			u2 = wl_fface(uidx2);
			a2 = E('_f_wl'+u2+'_hwaddr');
			if (a1 && a2 && (a1.value == a2.value)) {
				ferror.set(a1, 'Addresses must be unique', true);
				ferror.set(a2, 'Addresses must be unique', true);
				retValue = 0;
			}
		}
	}

	return retValue;
}

function verifyFields(focused, quiet) {
	var uidx, u, a;
	var retValue = 1;

	for (uidx = 1; uidx <= nvram.mwan_num; ++uidx){
		u = (uidx > 1) ? uidx : '';
		a = E('_f_wan'+u+'_hwaddr');
		if (!v_mac(a, quiet)) retValue = 0;
	}

	for (uidx = 0; uidx < wl_ifaces.length; ++uidx) {
		u = wl_fface(uidx);
		a = E('_f_wl'+u+'_hwaddr');
		if (!v_mac(a, quiet)) retValue = 0;
	}

	if (!checkUniqueMac()) retValue = 0;

	return retValue;
}

function save() {
	var u, uidx;

	if (!verifyFields(null, false)) return;
	if (!confirm("Warning: Changing the MAC address may require that you reboot all devices, computers or modem connected to this router. Continue anyway?")) return;

	var fom = E('t_fom');
	for (uidx = 1; uidx <= nvram.mwan_num; ++uidx){
		u = (uidx > 1) ? uidx : '';
		fom['wan'+u+'_mac'].value = E('_f_wan'+u+'_hwaddr').value; /* save always (not matter if default/random or not!) */
	}

	for (uidx = 0; uidx < wl_ifaces.length; ++uidx) {
		u = wl_fface(uidx);
		E('_wl'+u+'_hwaddr').value = E('_f_wl'+u+'_hwaddr').value; /* save always (not matter if default/random or not!) */
	}

	form.submit(fom, 1);
}

function init() {
	restoreVisibility(cprefix, 'notes');
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

<input type="hidden" name="_nextpage" value="advanced-mac.asp">
<input type="hidden" name="_nextwait" value="10">
<input type="hidden" name="_service" value="*">
<script>
	for (var uidx = 1; uidx <= MAXWAN_NUM; ++uidx) {
		var u = (uidx > 1) ? uidx : '';
		W('<input type="hidden" name="wan'+u+'_mac">');
	}
	for (var uidx = 0; uidx < wl_ifaces.length; ++uidx) {
		var u = wl_fface(uidx);
		W('<input type="hidden" id="_wl'+u+'_hwaddr" name="wl'+u+'_hwaddr">');
	}
</script>

<!-- / / / -->

<div class="section-title">MAC Address</div>
<div class="section">
	<script>
		var f = [];
		for (var uidx = 1; uidx <= nvram.mwan_num; ++uidx) {
			var u = (uidx > 1) ? uidx : '';
			f.push(
				{ title: 'WAN'+(uidx - 1)+' Port', indent: 1, name: 'f_wan'+u+'_hwaddr', type: 'text', maxlen: 17, size: 20,
					suffix: ' <input type="button" value="Default" onclick="bdefault(\'wan'+u+'\')"> <input type="button" value="Random LAA" onclick="brand(\'wan'+u+'\',false)"> <input type="button" value="OUI + Random UAA" onclick="brand(\'wan'+u+'\',true)"> <input type="button" value="Clone PC" onclick="bclone(\'wan'+u+'\')">',
					value: nvram['wan'+u+'_mac'] || defmac('wan'+u) }
			);
		}

		for (var uidx = 0; uidx < wl_ifaces.length; ++uidx) {
			var u = wl_fface(uidx);
			f.push(
				{ title: 'WL '+((wl_ifaces.length > 1) ? wl_display_ifname(uidx) : ''), indent: 1, name: 'f_wl'+u+'_hwaddr', type: 'text', maxlen: 17, size: 20,
					suffix:' <input type="button" value="Default" onclick="bdefault(\'wl'+u+'\')"> <input type="button" value="Random LAA" onclick="brand(\'wl'+u+'\',false)"> <input type="button" value="OUI + Random UAA" onclick="brand(\'wl'+u+'\',true)"> <input type="button" value="Clone PC" onclick="bclone(\'wl'+u+'\')">',
					value: nvram['wl'+u+'_hwaddr'] || defmac('wl' + u) }
			);
		}

		createFieldTable('', f);
	</script>

	<table style="border:none;padding:1px;padding-top:10px">
		<tr><td>Router's LAN MAC Address:</td><td><b><script>W(('<% nv('lan_hwaddr'); %>').toUpperCase());</script></b></td></tr>
		<tr><td>Computer's MAC Address:</td><td><b><script>W(('<% compmac(); %>').toUpperCase());</script></b></td></tr>
	</table>
</div>
<script>writeToggleSectionTitle('Notes', 'notes');</script>
<div class="section" id="sesdiv_notes" style="display:none">
	<ul>
		<li><b>Default</b> - Reset the MAC address to the Burn-in address, this is defined by the vendor</li>
		<li><b>Random LAA</b> - XY:XX:XX:XX:XX:XX - Randomize the MAC to a locally administered address will randomise the full address apart from the I/B bit</li>
		<li><b>OUI + Random UAA</b> - YY:YY:YY:XX:XX:XX - This retains the first 6 vendor specific HEX digits (OUI) and randomizes the last 6 digits (UAA) only</li>
		<li><b>Clone PC</b> - If the computer's MAC address is detected it will set the relevant interface's MAC to its clone</li>
	</ul>
	<br>
	<ul>
		<li><b>Router's br0 MAC Address</b> - This is the MAC address of the router's br0 interface</li>
		<li><b>Computer's MAC Address</b> - If connected to your router from the LAN you will see your device MAC address appearing here</li>
	</ul>
</div>

<!-- / / / -->

<script>writeFooter();</script>

</td></tr>
</table>
</form>
<script>insOvl();verifyFields(null, true);</script>
</body>
</html>
