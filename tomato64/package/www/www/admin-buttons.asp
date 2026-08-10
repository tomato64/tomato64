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
<title>[<% ident(); %>] Admin: Buttons</title>
<link rel="stylesheet" type="text/css" href="tomato.css?rel=<% version(); %>">
<% css(); %>
<script src="tomato.js?rel=<% version(); %>"></script>

<script>

//	<% nvram("stealth_mode,stealth_iled,blink_wl,sesx_led,sesx_b0,sesx_b1,sesx_b2,sesx_b3,sesx_script,script_brau,t_model,t_features"); %>

var ses = features('ses');
/* BCMARM-NO-BEGIN */
var brau = features('brau');
var aoss = features('aoss');
var wham = features('wham');
/* BCMARM-NO-END */

function verifyFields(focused, quiet) {
/* BCMARM-BEGIN */
	var a = !E('_f_stealth_mode').checked;
	E('_f_stealth_iled').disabled = a;
/* BCMARM-END */
	return 1;
}

function save() {
	var n;
	var fom;

	fom = E('t_fom');
	n = 0;
	if (fom._led0.checked) n |= 1;
	if (fom._led1.checked) n |= 2;
	if (fom._led2.checked) n |= 4;
	if (fom._led3.checked) n |= 8;
	fom.sesx_led.value = n;
/* BCMARM-BEGIN */
	fom.blink_wl.value = E('_f_blink_wl').checked ? 1 : 0;
	fom.stealth_mode.value = E('_f_stealth_mode').checked ? 1 : 0;
	fom.stealth_iled.value = E('_f_stealth_iled').checked ? 1 : 0;
/* BCMARM-END */
	form.submit(fom, 1);
}

function earlyInit() {
/* BCMARM-BEGIN */
	if (!ses) {
		E('notice-msg').innerHTML = '<div id="notice">This feature is not supported on this router.<\/div>';
		E('save-button').disabled = 1;
		return;
	}
	else {
		E('sesdiv').style.display = 'block';
	}
/* BCMARM-END */
/* BCMARM-NO-BEGIN */
	if ((!brau) && (!ses)) {
		E('notice-msg').innerHTML = '<div id="notice">This feature is not supported on this router.<\/div>';
		E('save-button').disabled = 1;
		return;
	}
	else {
		E('sesdiv').style.display = 'block';
		if (brau) E('braudiv').style.display = 'block';
		if ((wham) || (aoss) || (brau)) E('leddiv').style.display = 'block';
	}
/* BCMARM-NO-END */
	insOvl();
}
</script>
</head>

<body>
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

<input type="hidden" name="_nextpage" value="admin-buttons.asp">
<input type="hidden" name="sesx_led" value="0">
<!-- BCMARM-BEGIN -->
<input type="hidden" name="blink_wl">
<input type="hidden" name="stealth_mode">
<input type="hidden" name="stealth_iled">
<!-- BCMARM-END -->

<!-- / / / -->

<div id="notice-msg"></div>

<!-- / / / -->

<div id="sesdiv" style="display:none">
	<div class="section-title">SES/WPS/AOSS Button</div>
	<div class="section">
		<script>
			a = [[0,'Do Nothing'],[1,'Toggle Wireless'],[2,'Reboot'],[3,'Halt'],
/* USB-BEGIN */
				[5,'Unmount all USB Drives'],
/* USB-END */
				[4,'Run Custom Script']];
			createFieldTable('', [
				{ title: "When Pushed For..." },
				{ title: '1-2 Seconds', indent: 2, name: 'sesx_b0', type: 'select', options: a, value: nvram.sesx_b0 || 0 },
				{ title: '4-6 Seconds', indent: 2, name: 'sesx_b1', type: 'select', options: a, value: nvram.sesx_b1 || 0 },
				{ title: '8-10 Seconds', indent: 2, name: 'sesx_b2', type: 'select', options: a, value: nvram.sesx_b2 || 0 },
				{ title: '12+ Seconds', indent: 2, name: 'sesx_b3', type: 'select', options: a, value: nvram.sesx_b3 || 0 },
				{ title: 'Custom Script', indent: 2, name: 'sesx_script', type: 'textarea', value: nvram.sesx_script }
			]);
		</script>
	</div>

<!-- / / / -->

<!-- BCMARM-BEGIN -->
	<div class="section-title">Stealth Mode</div>
	<div class="section">
		<script>
			createFieldTable('', [
				{ title: 'Enable Stealth Mode', name: 'f_stealth_mode', type: 'checkbox', value: (nvram.stealth_mode == '1'), suffix: '&nbsp;<small>(this option requires a reboot to become effective)<\/small>' },
				{ title: 'Exclude INTERNET LED', name: 'f_stealth_iled', type: 'checkbox', value: (nvram.stealth_iled == '1') }
			]);
		</script>
	</div>

<!-- / / / -->

	<div class="section-title">Startup LED</div>
	<div class="section">
		<script>
			createFieldTable('', [
				{ title: 'Amber', name: '_led0', type: 'checkbox', value: (nvram.sesx_led & 0x01) },
				{ title: 'White', name: '_led1', type: 'checkbox', value: (nvram.sesx_led & 0x02) },
				{ title: 'AOSS', name: '_led2', type: 'checkbox', value: (nvram.sesx_led & 0x04) },
				{ title: 'Bridge', name: '_led3', type: 'checkbox', value: (nvram.sesx_led & 0x08) },
				{ title: 'Enable blink', name: 'f_blink_wl', type: 'checkbox', value: (nvram.blink_wl == '1'), suffix: ' <small> (for WiFi)<\/small>' }
			]);
		</script>
	</div>

<!-- / / / -->

	<div class="section-title">Notes</div>
	<div class="section">
		<i>Startup LED:</i><br>
		<ul>
			<li><b>Amber</b> - Enable LED Amber at Startup (No use case right now).</li>
			<li><b>White</b> - Enable LED White (Internet LED) at Startup.</li>
			<li><b>AOSS</b> - Enable LED AOSS (Power LED for Asus Router; Wifi Summary LED for Netgear Router) at Startup.</li>
			<li><b>Bridge</b> - Enable LED Bridge (WAN & LAN Port X LED(s)) at Startup.</li>
			<li><b>Enable blink</b> - Enable blink for WiFi LEDs.</li>
			<li><b>Other hints</b> - LED function and blink support is router dependent. Check command <i>led [LED_NAME/help] [on/off]</i> for advanced LED control, see <a href="tools-shell.asp">Web Shell</a>.</li>
		</ul>
	</div>
<!-- BCMARM-END -->

<!-- BCMARM-NO-BEGIN -->
	<div id="braudiv" style="display:none">
		<div class="section-title">Bridge/Auto Switch</div>
		<div class="section">
			<script>
				createFieldTable('', [
					{ title: 'Custom Script', indent: 2, name: 'script_brau', type: 'textarea', value: nvram.script_brau }
				]);
				</script>
		</div>
	</div>

<!-- / / / -->

	<div id="leddiv" style="display:none">
		<div class="section-title">Startup LED</div>
		<div class="section">
			<script>
				createFieldTable('', [
					{ title: 'Amber SES', name: '_led0', type: 'checkbox', value: nvram.sesx_led & 1, hidden: !wham },
					{ title: 'White SES', name: '_led1', type: 'checkbox', value: nvram.sesx_led & 2, hidden: !wham },
					{ title: 'AOSS', name: '_led2', type: 'checkbox', value: nvram.sesx_led & 4, hidden: !aoss },
					{ title: 'Bridge', name: '_led3', type: 'checkbox', value: nvram.sesx_led & 8, hidden: !brau }
					]);
			</script>
		</div>
	</div>
<!-- BCMARM-NO-END -->

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
<script>earlyInit()</script>
</body>
</html>
