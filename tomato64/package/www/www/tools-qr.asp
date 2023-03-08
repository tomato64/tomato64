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
<title>[<% ident(); %>] Tools: WiFi QR code generator</title>
<link rel="stylesheet" type="text/css" href="tomato.css">
<% css(); %>
<style media="print">
@page {
	size: auto;  /* auto is the initial value */
	margin: 0mm; /* this affects the margin in the printer settings */
}
@media print {
	@page { margin: 0; }
	body { margin: 1.6cm; text-align: center;}
}
</style>
<style>
#qr-printable-area,
#qrcode {
	padding: 10px;
	text-align: center;
}
#qr-code-controls {
	text-align: right;
}
#logo-show-control,
#password-show-control {
	padding-right: 10px;
}
.qr-wifi-values {
	font-size: 170%;
	font-weight: bold;
}
.qr-wifi-labels {
	font-size: 130%;
}
</style>

<script src="tomato.js"></script>
<script src="wireless.jsx?_http_id=<% nv(http_id); %>"></script>
<script src="qrcode.js"></script>
<script src="html5-qrcode.js"></script>

<script>

//	<% nvram('wl_ssid,wl_security_mode,wl_closed,wl_passphrase,wl_wpa_psk'); %>

var cprefix = 'tools_qr';
var tabs = []
var authMap = {'disabled':'nopass','wep':'WEP','wpa_personal':'WPA','wpa_enterprise':'WPA','wpa2_personal':'WPA','wpa2_enterprise':'WPA','wpaX_personal':'WPA','wpaX_enterprise':'WPA'};

function getNvramWifiParameter(wifiInterface, paramName) {
	return nvram[wifiInterface+'_'+paramName];
}

function printDiv(divName) {
	var printContents = document.getElementById(divName).innerHTML;
	var originalContents = document.body.innerHTML;
	document.body.innerHTML = printContents;
	window.print();
	document.body.innerHTML = originalContents;

	E('togglePasswordCheckbox').checked = (E('passwd').style.display == 'inline-block' ? true : false);
}

for (var uidx = 0; uidx < wl_ifaces.length; ++uidx) {
	var u = wl_fface(uidx);

	var sunit = wl_sunit(uidx);
	var tabName = 'wl'+wl_unit(uidx)+(sunit >= 0 ? ('.'+sunit) : '');
	var displayName = getNvramWifiParameter(tabName, 'ssid')+' - '+wl_display_ifname(uidx);

	tabs.push([tabName, displayName]);
}

function tabSelect(name) {
	tabHigh(name);

	for (var i = 0; i < tabs.length; ++i) {
		if (name == tabs[i][0] ) {
			var ssid = getNvramWifiParameter(name, 'ssid');
			var enc = authMap[getNvramWifiParameter(name, 'security_mode')];
			var hidden = (getNvramWifiParameter(name, 'closed') == '1' ) ? 'true' : 'false';
			var pw = (enc == 'WEP') ? getNvramWifiParameter(name, 'passphrase') : getNvramWifiParameter(name, 'wpa_psk');

			E('wifi-network-ssid').innerHTML = ssid.replaceAll('<', '&lt;').replaceAll('>', '&gt;');
			E('wifi-network-password').innerHTML = pw.replaceAll('<', '&lt;').replaceAll('>', '&gt;');
			E('password-show-control').style.display = (enc === 'nopass') ? 'none' : 'inline';

			var qrCodeContent = `WIFI:S:${ssid};T:${enc};P:${pw.replaceAll(';', '\\;')};H:${hidden};;`;
			E('qrcode').replaceChild(showQRCode(qrCodeContent), qrcode.lastChild);
		}
	}

	cookie.set('qr-tab', name);
}

function toggleLogo() {
	var x = E('wifi-logo');
	var checkbox = E('toggleLogoCheckbox').checked;
	x.style.display = checkbox ? 'inline-block' : 'none';
}

function togglePwd() {
	var x = E('passwd');
	var checkbox = E('togglePasswordCheckbox').checked;
	x.style.display = checkbox ? 'inline-block' : 'none';
}

function init() {
	var c;
	if (((c = cookie.get(cprefix+'_notes_vis')) != null) && (c == '1'))
		toggleVisibility(cprefix, 'notes')

	var tabSelectParamter = window.location.search.substr(1).split('=');
	var openedTab = cookie.get('qr-tab') || 'wl0';

	if (tabSelectParamter.length > 0 && tabSelectParamter.includes('wl'))
		openedTab = 'wl'+tabSelectParamter[1];

	tabSelect(openedTab);
}
</script>
</head>
<body onload=init()>
<form id="t_fom" action="javascript:{}">
<table id="container">
<tr><td colspan="2" id="header">
	<div class="title">FreshTomato</div>
	<div class="version">Version <% version(); %> on <% nv("t_model_name"); %></div>
</td></tr>
<tr id="body"><td id="navi"><script>navi()</script></td>
<td id="content">
<div id="ident"><% ident(); %> | <script>wikiLink();</script></div>

<!-- / / / -->

<div class="section-title" id="qr-codes">WiFi QR Codes</div>
<script>tabCreate.apply(this, tabs);</script>
<div class="section">

<!-- / / / -->

	<div id="qr-code-controls">
		<span id="logo-show-control">
			Show WiFi Logo &nbsp;<input type="checkbox" onchange="toggleLogo()" id="toggleLogoCheckbox" checked>
		</span>
		<span id="password-show-control">
			Show WiFi password &nbsp;<input type="checkbox" onchange="togglePwd()" id="togglePasswordCheckbox">
		</span>
		&nbsp;<input type="button" onclick="printDiv('qr-printable-area')" value="Print" />
	</div>

<!-- / / / -->

	<div id="qr-printable-area">
		<svg version="1.1" xmlns="http://www.w3.org/2000/svg" width="197" height="145" viewBox="0 -54.469 276.93 194.93" id="wifi-logo">
			<path d="M0 61.45V26.6S-.379 11.574 14.647-3.452s29.295-13.384 29.295-13.384H63.64S88.39-53.96 137.381-53.96s77.023 36.618 77.023 36.618h22.225s5.61-.254 12.66 3.287c7.127 3.58 13.54 7.704 20.067 17.835 6.546 10.16 7.425 17.48 7.425 22.819v38.133c0 7.95-.758 14.142-14.142 27.526s-30.558 11.869-30.558 11.869h-20.454s-25.549 36.826-73.489 36.826c-47.947 0-72.479-36.573-72.479-36.573H42.931s-15.026.884-29.547-13.638S.252 61.45 0 61.45zm24.117-50.13L38.26 72.31h15.53l6.945-35.103 6.692 35.103h15.91L97.48 11.7H80.56l-4.924 33.21-6.945-33.335H53.412l-6.944 33.714-5.177-33.84-17.174-.127zm81.355 17.063v43.872h14.925V28.383zm17.186-12.393c0 4.244-4.214 7.689-9.408 7.689s-9.407-3.445-9.407-7.69 4.215-7.688 9.407-7.688 9.408 3.445 9.408 7.689zm3.618 79.51s18.634-14.141 18.634-35.187v-35.46c0-5.7 2.923-13.777 11.668-22.522S173.03-7.8 178.378-7.8h55.54c9.44.13 16.776 4.088 23.52 10.402 6.247 6.564 10.311 14.79 10.311 23.79v33.92c0 6.678-1.79 16.355-10.674 24.514S241.346 95.5 234.28 95.5zm102.58-67.072v43.87h14.926v-43.87zm17.187-12.393c0 4.244-4.214 7.688-9.407 7.688s-9.408-3.444-9.408-7.688 4.215-7.69 9.408-7.69 9.407 3.446 9.407 7.69zM166.08 71.983h16.734V53.71h31.841V40.05h-31.932V25.76h36.092V11.558h-52.826z"/>
			<path d="M38.354 71.393c-.448-1.335-13.575-58.5-13.575-59.115 0-.485 1.98-.64 8.187-.64 4.503 0 8.186.172 8.185.384-.004.648 4.627 31 4.938 32.365.271 1.188.318 1.205.663.256.204-.562 1.85-8.225 3.654-17.026l3.283-16.003 7.398.14 7.398.142 3.351 16.207 3.664 16.52c.411.41.49-.037 3.053-17.504l2.197-14.967h8.088c4.45 0 8.09.146 8.09.325 0 .37-13.216 57.27-13.622 58.648-.236.8-1.067.895-7.87.895s-7.635-.094-7.874-.895c-.147-.492-1.663-8.248-3.369-17.235l-3.337-16.576c-.4-.4-.644.693-3.975 17.82l-3.234 16.63-7.538.143c-5.758.105-7.587-.015-7.755-.514zm67.273-21.121V28.524h14.328v43.494h-14.328zm4.25-27.403c-.899-.275-2.434-1.253-3.41-2.175-3.388-3.195-2.849-7.712 1.244-10.421 1.771-1.171 2.733-1.434 5.286-1.441 11.802-.033 12.578 13.618.817 14.386-1.266.083-3.038-.074-3.937-.35zm21.444 68.17c6-6.28 9.773-12.395 12.213-19.788 1.143-3.465 1.191-4.345 1.486-26.864.328-25.023.288-24.638 3.2-30.446 1.701-3.393 5.826-8.65 9.525-12.14 4.095-3.862 8.245-6.492 12.868-8.155l3.325-1.197h31.726c34.91 0 34.395-.04 41.447 3.265 8.947 4.193 17.216 14.283 19.441 23.726 1.125 4.77 1.114 44.373-.014 48.762-2.334 9.086-8.82 17.199-18.26 22.84-7.166 4.28-2.63 4.004-65.244 4.004h-55.54zm112.46-40.767v-22.26h-15.352V72.53h15.352zm-60.893 12.792V54.11h32.237V39.782h-32.237V25.966h36.331v-14.84h-53.888l.336 8.526c.185 4.688.337 18.389.337 30.446v21.92h16.884zm57.45-39.686c5.506-1.53 7.773-8.267 4.051-12.031-4.336-4.382-11.795-4.26-15.596.258-2.588 3.075-2.456 6.605.35 9.41 2.64 2.64 6.931 3.547 11.194 2.363z" fill="#fff"/>
		</svg>

		<div>
			<br>
			<span class="qr-wifi-labels">WiFi network :</span><br>
			<span id="wifi-network-ssid" class="qr-wifi-values" />
		</div>

		<div id="passwd" style="display:none">
			<br>
			<span class="qr-wifi-labels">Password :</span><br>
			<span id="wifi-network-password" class="qr-wifi-values" />
		</div>

		<div id="qrcode"><img alt="qrcode"></div>

		<div class="qr-wifi-labels" contentEditable="true" title="Message">
			Point your mobile phone camera<br>
			here above to connect automatically
		</div>
	</div>

</div>

<!-- / / / -->

<div class="section-title">Notes <small><i><a href="javascript:toggleVisibility(cprefix,'notes');"><span id="sesdiv_notes_showhide">(Show)</span></a></i></small></div>
<div class="section" id="sesdiv_notes" style="display:none">
	<ul>
		<li><b>Show WIFI logo</b> - Select this to make the standard WiFi logo appear in the page/print</li>
		<li><b>Show WiFi password</b> - Select this to display both traditional password and QR code. Works only on visible SSIDs</li>
		<li><b>Print</b> - Only the relevant part of the page will be considered for printing</li>
		<li><b>Message</b> - The message below the QR code itself is editable inline. It's effective for printing but won't be saved</li>
		<li>If you want to remove any automatically added text like <i>page title, date, URL, page number, etc</i> you will need to modify the browser configuration</li>
	</ul>
</div>

<!-- / / / -->

</td></tr>
</table>
</form>
</body>
</html>
