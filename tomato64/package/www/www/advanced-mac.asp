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
<link rel="stylesheet" type="text/css" href="tomato.css">
<% css(); %>
<script src="tomato.js"></script>
<script src="wireless.jsx?_http_id=<% nv(http_id); %>"></script>

<script>

//	<% nvram("lan_hwaddr,wan_mac,wan2_mac,wan3_mac,wan4_mac,mwan_num,wl_macaddr,wl_hwaddr,wl_nband"); %>

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
	if (which == 'wan')  return et0plus(16);
	if (which == 'wan2') return et0plus(17);
/* MULTIWAN-BEGIN */
	if (which == 'wan3') return et0plus(18);
	if (which == 'wan4') return et0plus(19);
/* MULTIWAN-END */
	else {
/* REMOVE-BEGIN
// align to wlconf setup AND FreshTomato initial mac setup
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
	var uidx, u1, u2, a1, a2;

	for (uidx = 1; uidx <= nvram.mwan_num; ++uidx) {
		for (uidx2 = uidx; uidx2 <= nvram.mwan_num; ++ uidx2 ) {
			u1 = (uidx > 1) ? uidx : '';
			a1 = E('_f_wan'+u+'_hwaddr');
			u2 = (uidx2 > 1) ? uidx2 : '';
			a2 = E('_f_wan'+u+'_hwaddr');
			if (a1 && a2 && (a1.value == a2.value)) {
				ferror.set(a1, 'Addresses must be unique', true);
				ferror.set(a2, 'Addresses must be unique', true);
			}
		}
	}

	for (uidx = 0; uidx <= wl_ifaces.length; ++uidx) {
		for (uidx2 = uidx; uidx2 <= wl_ifaces.length; ++ uidx2 ) {
			if (uidx != uidx2) {
				a1 = E('_f_wl'+uidx+'_hwaddr');
				a2 = E('_f_wl'+uidx2+'_hwaddr');
				if (a1 && a2 && (a1.value == a2.value)) {
					ferror.set(a1, 'Addresses must be unique', true);
					ferror.set(a2, 'Addresses must be unique', true);
				}
			}
		}
	}
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

		checkUniqueMac();
	}

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
	var c;
	if (((c = cookie.get(cprefix+'_notes_vis')) != null) && (c == '1'))
		toggleVisibility(cprefix, 'notes');
}

</script>
</head>

<body onload="init()">
<form id="t_fom" method="post" action="tomato.cgi">
<table id="container">
<tr><td colspan="2" id="header">
	<div class="title">FreshTomato</div>
	<div class="version">Version <% version(); %> on <% nv("t_model_name"); %></div>
</td></tr>
<tr id="body"><td id="navi"><script>navi()</script></td>
<td id="content">
<div id="ident"><% ident(); %> | <script>wikiLink();</script></div>

<!-- / / / -->

<input type="hidden" name="_nextpage" value="advanced-mac.asp">
<input type="hidden" name="_nextwait" value="10">
<input type="hidden" name="_service" value="*">
<input type="hidden" name="wan_mac">
<input type="hidden" name="wan2_mac">
<!-- MULTIWAN-BEGIN -->
<input type="hidden" name="wan3_mac">
<input type="hidden" name="wan4_mac">
<!-- MULTIWAN-END -->
<script>
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
					suffix: ' <input type="button" value="Default" onclick="bdefault(\'wan'+u+'\')"> <input type="button" value="Random LLA" onclick="brand(\'wan'+u+'\',false)"> <input type="button" value="OUI + Random UAA" onclick="brand(\'wan'+u+'\',true)"> <input type="button" value="Clone PC" onclick="bclone(\'wan'+u+'\')">',
					value: nvram['wan'+u+'_mac'] || defmac('wan'+u) }
			);
		}

		for (var uidx = 0; uidx < wl_ifaces.length; ++uidx) {
			var u = wl_fface(uidx);
			f.push(
				{ title: 'WL '+((wl_ifaces.length > 1) ? wl_display_ifname(uidx) : ''), indent: 1, name: 'f_wl'+u+'_hwaddr', type: 'text', maxlen: 17, size: 20,
					suffix:' <input type="button" value="Default" onclick="bdefault(\'wl'+u+'\')"> <input type="button" value="Random LLA" onclick="brand(\'wl'+u+'\',false)"> <input type="button" value="OUI + Random UAA" onclick="brand(\'wl'+u+'\',true)"> <input type="button" value="Clone PC" onclick="bclone(\'wl'+u+'\')">',
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
<div class="section-title">Notes <small><i><a href="javascript:toggleVisibility(cprefix,'notes');"><span id="sesdiv_notes_showhide">(Show)</span></a></i></small></div>
<div class="section" id="sesdiv_notes" style="display:none">
	<ul>
		<li><b>Default</b> - Reset the MAC address to the Burn-in address, this is defined by the vendor</li>
		<li><b>Random LLA</b> - XY:XX:XX:XX:XX:XX - Randomize the MAC to a locally administered address will randomise the full address apart from the I/B bit</li>
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

<div id="footer">
	<span id="footer-msg"></span>
	<input type="button" value="Save" id="save-button" onclick="save()">
	<input type="button" value="Cancel" id="cancel-button" onclick="reloadPage();">
</div>

</td></tr>
</table>
</form>
<script>verifyFields(null, true);</script>
</body>
</html>
