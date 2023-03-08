<!DOCTYPE html>
<!--
	Tomato RAF Transmission GUI
	Copyright (C) 2007-2011 Shibby
	http://openlinksys.info
	For use with Tomato RAF Firmware only.
	No part of this file may be used without permission.
-->
<html lang="en-GB">
<head>
<meta http-equiv="content-type" content="text/html;charset=utf-8">
<meta name="robots" content="noindex,nofollow">
<title>[<% ident(); %>] Nas: BitTorrent Client</title>
<link rel="stylesheet" type="text/css" href="tomato.css">
<% css(); %>
<script src="isup.jsz"></script>
<script src="isup.js"></script>
<script src="tomato.js"></script>

<script>

//	<% nvram("bt_enable,bt_binary,bt_binary_custom,bt_custom,bt_port,bt_dir,bt_settings,bt_settings_custom,bt_incomplete,bt_autoadd,bt_rpc_enable,bt_rpc_wan,bt_auth,bt_login,bt_password,bt_port_gui,bt_dl_enable,bt_dl,bt_ul_enable,bt_ul,bt_peer_limit_global,bt_peer_limit_per_torrent,bt_ul_slot_per_torrent,bt_ratio_enable,bt_ratio,bt_ratio_idle_enable,bt_ratio_idle,bt_dht,bt_pex,bt_lpd,bt_utp,bt_blocklist,bt_blocklist_url,bt_sleep,bt_check_time,bt_dl_queue_enable,bt_dl_queue_size,bt_ul_queue_enable,bt_ul_queue_size,bt_message,bt_log,bt_log_path"); %>

/* CIFS-BEGIN */
//	<% statfs("/cifs1", "cifs1"); %>

//	<% statfs("/cifs2", "cifs2"); %>
/* CIFS-END */
/* JFFS2-BEGIN */
//	<% statfs("/jffs", "jffs2"); %>
/* JFFS2-END */
/* JFFS2NAND-BEGIN */
//	<% statfs("/jffs", "brcmnand"); %>
/* JFFS2NAND-END */

var cprefix = 'nas_bittorrent';
var changed = 0;
var serviceType = 'transmission';

function verifyFields(focused, quiet) {
	if (focused && focused != E('_f_bt_enable')) /* except on/off */
		changed = 1;

	var ok = 1;
	var a = E('_f_bt_rpc_enable').checked;
	var b = E('_f_bt_auth').checked;

	E('_bt_port_gui').disabled = !a;
	E('_f_bt_auth').disabled = !a;
	E('_bt_login').disabled = !a || !b;
	E('_bt_password').disabled = !a | !b;
	E('_f_bt_rpc_wan').disabled = !a || !b;
	E('_bt_dl').disabled = !E('_f_bt_dl_enable').checked;
	E('_bt_ul').disabled = !E('_f_bt_ul_enable').checked;
	E('_bt_ratio').disabled = !E('_f_bt_ratio_enable').checked;
	E('_bt_ratio_idle').disabled = !E('_f_bt_ratio_idle_enable').checked;
	E('_bt_dl_queue_size').disabled = !E('_f_bt_dl_queue_enable').checked;
	E('_bt_ul_queue_size').disabled = !E('_f_bt_ul_queue_enable').checked;
	E('_bt_blocklist_url').disabled = !E('_f_bt_blocklist').checked;
	E('_bt_log_path').disabled = !E('_f_bt_log').checked;

/* CIFS-BEGIN */
	if (cifs1.size == 0) E('_bt_settings').options[2].disabled = 1;
	if (cifs2.size == 0) E('_bt_settings').options[3].disabled = 1;
/* CIFS-END */
/* JFFS2-BEGIN */
	if (jffs2.size == 0 && !jffs2.mnt) E('_bt_settings').options[1].disabled = 1;
/* JFFS2-END */
/* JFFS2NAND-BEGIN */
	if (brcmnand.size == 0 && !brcmnand.mnt) E('_bt_settings').options[1].disabled = 1;
/* JFFS2NAND-END */

	elem.display('_bt_settings_custom', (E('_bt_settings').value == 'custom'));
	elem.display('_bt_binary_custom', (E('_bt_binary').value == 'custom'));

	if (!v_length('_bt_custom', quiet, 0, 2048)) ok = 0;
	if (!v_range('_bt_check_time', quiet || !ok, 0, 55)) ok = 0;
	if (!v_range('_bt_sleep', quiet || !ok, 1, 60)) ok = 0;
	if (!v_range('_bt_peer_limit_global', quiet || !ok, 10, 1000)) ok = 0;
	if (!v_range('_bt_peer_limit_per_torrent', quiet || !ok, 1, 200)) ok = 0;
	if (!v_range('_bt_ul_slot_per_torrent', quiet || !ok, 1, 50)) ok = 0;
	if (!v_port('_bt_port', quiet || !ok)) ok = 0;
	if (!v_port('_bt_port_gui', quiet || !ok)) ok = 0;
	if (!v_nodelim('_bt_dir', quiet || !ok, 'Directory', 1) || !v_path('_bt_dir', quiet || !ok, 1)) ok = 0;

	if (E('_f_bt_ratio_idle_enable').checked) {
		if (!v_range('_bt_ratio_idle', quiet || !ok, 1, 55)) ok = 0;
	}
	else
		ferror.clear(E('_bt_ratio_idle'));

	if (E('_f_bt_dl_queue_enable').checked) {
		if (!v_range('_bt_dl_queue_size', quiet || !ok, 1, 30)) ok = 0;
	}
	else
		ferror.clear(E('_bt_dl_queue_size'));

	if (E('_f_bt_ul_queue_enable').checked) {
		if (!v_range('_bt_ul_queue_size', quiet || !ok, 1, 30)) ok = 0;
	}
	else
		ferror.clear(E('_bt_ul_queue_size'));

	if (E('_f_bt_log').checked) {
		if (!v_nodelim('_bt_log_path', quiet || !ok, 'Directory', 1) || !v_path('_bt_log_path', quiet || !ok, 1)) ok = 0;
	}
	else
		ferror.clear(E('_bt_log_path'));

	var c = E('_bt_login');
	var d = E('_bt_password');
	if (E('_f_bt_auth').checked) {
		if (c.value.trim().replace(/\s+/g, '').length < 1) {
			ferror.set(c, 'Username can not be empty', quiet || !ok);
			ok = 0;
		}
		else
			ferror.clear(c);

		if (d.value.trim().replace(/\s+/g, '').length < 1) {
			ferror.set(d, 'Password can not be empty', quiet || !ok);
			ok = 0;
		}
		else
			ferror.clear(d);
	}
	else {
		ferror.clear(c);
		ferror.clear(d);
	}

	var s = E('_bt_custom');
	var searchArray = ['rpc-enable','peer-port','speed-limit-down-enabled','speed-limit-up-enabled','speed-limit-down','speed-limit-up','rpc-port','rpc-username','rpc-password',
	                   'download-dir','incomplete-dir-enabled','watch-dir-enabled','peer-limit-global','peer-limit-per-torrent','upload-slots-per-torrent','dht-enabled','pex-enabled',
	                   'lpd-enabled','utp-enabled','ratio-limit-enabled','ratio-limit','rpc-authentication-required','blocklist-enabled','blocklist-url','download-queue-enabled',
	                   'download-queue-size','seed-queue-enabled','seed-queue-size','idle-seeding-limit-enabled','idle-seeding-limit','message-level'];

	for (var i = 0; i < searchArray.length; i++) {
		var regex = new RegExp('"'+searchArray[i]+'":', 'g');
		if (s.value.search(regex) !== -1) {
			ferror.set(s, 'The "'+searchArray[i]+'" option is not allowed here. You can set it in FreshTomato GUI', quiet || !ok);
			ok = 0;
		}
	}

	if (s.value.search(/"rpc-whitelist-enabled":/) != -1) {
		ferror.set(s, 'The "rpc-whitelist-enabled" option is not allowed here. Whitelist is always disabled', quiet || !ok);
		ok = 0;
	}

	if (s.value.search(/"incomplete-dir":/) != -1) {
		ferror.set(s, 'The "incomplete-dir" option is not allowed here. If incomplete dir is enabled, all incomplete files will be downloaded to .incomplete directory', quiet || !ok);
		ok = 0;
	}
/*
	if (s.value.search(/"watch-dir":/) != -1) {
		ferror.set(s, 'The "watch-dir" option is not allowed here. If Autoadd .torrents is enabled, Transmission will watch them in download directory', quiet || !ok);
		ok = 0;
	}
*/

	return ok;
}

function save_pre() {
	if (!verifyFields(null, 0))
		return 0;
	return 1;
}

function save(nomsg) {
	save_pre();
	if (!nomsg) show(); /* update '_service' field first */

	var fom = E('t_fom');
	fom.bt_enable.value = fom._f_bt_enable.checked ? 1 : 0;
	fom.bt_incomplete.value = fom._f_bt_incomplete.checked ? 1 : 0;
	fom.bt_autoadd.value = fom._f_bt_autoadd.checked ? 1 : 0;
	fom.bt_rpc_enable.value = fom._f_bt_rpc_enable.checked ? 1 : 0;
	fom.bt_auth.value = fom._f_bt_auth.checked ? 1 : 0;
	fom.bt_rpc_wan.value = fom._f_bt_rpc_wan.checked ? 1 : 0;
	fom.bt_dl_enable.value = fom._f_bt_dl_enable.checked ? 1 : 0;
	fom.bt_ul_enable.value = fom._f_bt_ul_enable.checked ? 1 : 0;
	fom.bt_ratio_enable.value = fom._f_bt_ratio_enable.checked ? 1 : 0;
	fom.bt_ratio_idle_enable.value = fom._f_bt_ratio_idle_enable.checked ? 1 : 0;
	fom.bt_dht.value = fom._f_bt_dht.checked ? 1 : 0;
	fom.bt_pex.value = fom._f_bt_pex.checked ? 1 : 0;
	fom.bt_lpd.value = fom._f_bt_lpd.checked ? 1 : 0;
	fom.bt_utp.value = fom._f_bt_utp.checked ? 1 : 0;
	fom.bt_blocklist.value = fom._f_bt_blocklist.checked ? 1 : 0;
	fom.bt_log.value = fom._f_bt_log.checked ? 1 : 0;
	fom.bt_dl_queue_enable.value = fom._f_bt_dl_queue_enable.checked ? 1 : 0;
	fom.bt_ul_queue_enable.value = fom._f_bt_ul_queue_enable.checked ? 1 : 0;
	fom._nofootermsg.value = (nomsg ? 1 : 0);

	form.submit(fom, 1);

	changed = 0;
}

function earlyInit() {
	show();
	verifyFields(null, 1);
}

function init() {
	eventHandler();
	up.initPage(250, 5);
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

<input type="hidden" name="_nextpage" value="nas-bittorrent.asp">
<input type="hidden" name="_service" value="">
<input type="hidden" name="_nofootermsg">
<input type="hidden" name="bt_enable">
<input type="hidden" name="bt_incomplete">
<input type="hidden" name="bt_autoadd">
<input type="hidden" name="bt_rpc_enable">
<input type="hidden" name="bt_auth">
<input type="hidden" name="bt_rpc_wan">
<input type="hidden" name="bt_dl_enable">
<input type="hidden" name="bt_ul_enable">
<input type="hidden" name="bt_blocklist">
<input type="hidden" name="bt_log">
<input type="hidden" name="bt_ratio_enable">
<input type="hidden" name="bt_ratio_idle_enable">
<input type="hidden" name="bt_dht">
<input type="hidden" name="bt_pex">
<input type="hidden" name="bt_lpd">
<input type="hidden" name="bt_utp">
<input type="hidden" name="bt_dl_queue_enable">
<input type="hidden" name="bt_ul_queue_enable">

<!-- / / / -->

<div class="section-title">Status</div>
<div class="section">
	<div class="fields">
		<span id="_transmission_notice"></span>
		<input type="button" id="_transmission_button" value="">
		<input type="button" id="_transmission_status" value="Open Transmission GUI in new tab" class="new_window" onclick="window.open('http://'+location.hostname+':'+nvram.bt_port_gui+'')">
		&nbsp; <img src="spin.gif" alt="" id="spin">
	</div>
</div>

<!-- / / / -->

<div class="section-title">Basic Settings</div>
<div class="section">
	<script>
		createFieldTable('', [
			{ title: 'Enable on Start', name: 'f_bt_enable', type: 'checkbox', value: nvram.bt_enable == 1 },
			{ title: 'Transmission binary path', multi: [
				{ name: 'bt_binary', type: 'select', options: [
/* BBT-BEGIN */
					['internal','Internal (/usr/bin)'],
/* BBT-END */
					['optware','Optware/Entware (/opt/bin)'],
					['custom','Custom'] ], value: nvram.bt_binary },
				{ name: 'bt_binary_custom', type: 'text', maxlen: 40, size: 40, value: nvram.bt_binary_custom } ] },
			{ title: 'Poll Interval', name: 'bt_check_time', type: 'text', maxlen: 5, size: 7, value: nvram.bt_check_time, suffix: ' <small>minutes; range: 0 - 55; default: 15; 0 to disable<\/small>' },
			{ title: 'Delay at startup', name: 'bt_sleep', type: 'text', maxlen: 5, size: 7, value: nvram.bt_sleep, suffix: ' <small>seconds; range: 1 - 60; default: 10<\/small>' },
			{ title: 'Listening port', name: 'bt_port', type: 'text', maxlen: 5, size: 7, value: nvram.bt_port },
			{ title: 'Download directory', name: 'bt_dir', type: 'text', maxlen: 40, size: 40, value: nvram.bt_dir },
			{ title: 'Use .incomplete/', indent: 2, name: 'f_bt_incomplete', type: 'checkbox', value: nvram.bt_incomplete == 1 },
			{ title: 'Autoadd .torrents', indent: 2, name: 'f_bt_autoadd', type: 'checkbox', value: nvram.bt_autoadd == 1, suffix: ' <small>search .torrent files in Download directory by default<\/small>' }
		]);
	</script>
</div>

<!-- / / / -->

<div class="section-title">GUI Access</div>
<div class="section">
	<script>
		createFieldTable('', [
			{ title: 'Enable', name: 'f_bt_rpc_enable', type: 'checkbox', value: nvram.bt_rpc_enable == 1 },
			{ title: 'Listening port', indent: 2, name: 'bt_port_gui', type: 'text', maxlen: 32, size: 5, value: nvram.bt_port_gui },
			{ title: 'Authentication required', name: 'f_bt_auth', type: 'checkbox', value: nvram.bt_auth == 1 },
			{ title: 'Username', indent: 2, name: 'bt_login', type: 'text', maxlen: 32, size: 15, value: nvram.bt_login },
			{ title: 'Password', indent: 2, name: 'bt_password', type: 'password', maxlen: 32, size: 15, peekaboo: 1, value: nvram.bt_password },
			{ title: 'Allow remote access', name: 'f_bt_rpc_wan', type: 'checkbox', value: nvram.bt_rpc_wan == 1 }
		]);
	</script>
</div>

<!-- / / / -->

<div class="section-title">Limits</div>
<div class="section">
	<script>
		createFieldTable('', [
			{ title: 'Download limit', multi: [
				{ name: 'f_bt_dl_enable', type: 'checkbox', value: nvram.bt_dl_enable == 1, suffix: ' ' },
				{ name: 'bt_dl', type: 'text', maxlen: 10, size: 7, value: nvram.bt_dl, suffix: ' <small>kB/s<\/small>' } ] },
			{ title: 'Upload limit', multi: [
				{ name: 'f_bt_ul_enable', type: 'checkbox', value: nvram.bt_ul_enable == 1, suffix: ' ' },
				{ name: 'bt_ul', type: 'text', maxlen: 10, size: 7, value: nvram.bt_ul, suffix: ' <small>kB/s<\/small>' } ] },
			{ title: 'Stop seeding at ratio', multi: [
				{ name: 'f_bt_ratio_enable', type: 'checkbox', value: nvram.bt_ratio_enable == 1, suffix: ' ' },
				{ name: 'bt_ratio', type: 'select', options: [['0.0000','0.0'],['0.1000','0.1'],['0.2000','0.2'],['0.5000','0.5'],['1.0000','1.0'],['1.5000','1.5'],['2.0000','2.0'],['2.5000','2.5'],['3.0000','3.0']], value: nvram.bt_ratio } ] },
			{ title: 'Stop seeding if idle for', multi: [
				{ name: 'f_bt_ratio_idle_enable', type: 'checkbox', value: nvram.bt_ratio_idle_enable == 1, suffix: ' ' },
				{ name: 'bt_ratio_idle', type: 'text', maxlen: 10, size: 7, value: nvram.bt_ratio_idle, suffix: ' <small>minutes; range: 1 - 55; default: 30<\/small>' } ] },
			{ title: 'Global peer limit', name: 'bt_peer_limit_global', type: 'text', maxlen: 10, size: 7, value: nvram.bt_peer_limit_global, suffix: ' <small>range: 10 - 1000; default: 150<\/small>' },
			{ title: 'Peer limit per torrent', name: 'bt_peer_limit_per_torrent', type: 'text', maxlen: 10, size: 7, value: nvram.bt_peer_limit_per_torrent, suffix: ' <small>range: 1 - 200; default: 30<\/small>' },
			{ title: 'Upload slots per torrent', name: 'bt_ul_slot_per_torrent', type: 'text', maxlen: 10, size: 7, value: nvram.bt_ul_slot_per_torrent, suffix: ' <small>range: 1 - 50; default: 10<\/small>' }
		]);
	</script>
</div>

<!-- / / / -->

<div class="section-title">Queue torrents</div>
<div class="section">
	<script>
		createFieldTable('', [
			{ title: 'Downloads queuing', multi: [
				{ name: 'f_bt_dl_queue_enable', type: 'checkbox', value: nvram.bt_dl_queue_enable == 1, suffix: ' ' },
				{ name: 'bt_dl_queue_size', type: 'text', maxlen: 5, size: 7, value: nvram.bt_dl_queue_size, suffix: ' <small>range: 1 - 30; default: 5<\/small>' } ] },
			{ title: 'Seeds queuing', multi: [
				{ name: 'f_bt_ul_queue_enable', type: 'checkbox', value: nvram.bt_ul_queue_enable == 1, suffix: ' ' },
				{ name: 'bt_ul_queue_size', type: 'text', maxlen: 5, size: 7, value: nvram.bt_ul_queue_size, suffix: ' <small>range: 1 - 30; default: 5<\/small>' } ] }
		]);
	</script>
</div>

<!-- / / / -->

<div class="section-title">Advanced Settings</div>
<div class="section">
	<script>
		createFieldTable('', [
			{ title: 'Find more peers using', multi: [
				{ suffix: '&nbsp; DHT &nbsp;&nbsp;&nbsp;', name: 'f_bt_dht', type: 'checkbox', value: nvram.bt_dht == 1 },
				{ suffix: '&nbsp; PEX &nbsp;&nbsp;&nbsp;', name: 'f_bt_pex', type: 'checkbox', value: nvram.bt_pex == 1 },
				{ suffix: '&nbsp; LPD &nbsp;&nbsp;&nbsp;', name: 'f_bt_lpd', type: 'checkbox', value: nvram.bt_lpd == 1 },
				{ suffix: '&nbsp; uTP &nbsp;&nbsp;&nbsp;', name: 'f_bt_utp', type: 'checkbox', value: nvram.bt_utp == 1 } ] },
			{ title: 'Message level', name: 'bt_message', type: 'select', options: [ ['0','None'], ['1','Error'], ['2','Info'], ['3','Debug'] ], value: nvram.bt_message, suffix: ' ' },
			{ title: 'Save settings location', multi: [
				{ name: 'bt_settings', type: 'select', options: [
					['down_dir','In the Download directory (Recommended)'],
/* JFFS2-BEGIN */
					['/jffs','JFFS2'],
/* JFFS2-END */
/* CIFS-BEGIN */
					['/cifs1','CIFS 1'],['/cifs2','CIFS 2'],
/* CIFS-END */
					['/tmp','RAM (Temporary)'], ['custom','Custom'] ], value: nvram.bt_settings, suffix: ' ' },
				{ name: 'bt_settings_custom', type: 'text', maxlen: 60, size: 40, value: nvram.bt_settings_custom } ] },
			{ title: 'Blocklist', multi: [
				{ name: 'f_bt_blocklist', type: 'checkbox', value: nvram.bt_blocklist == 1, suffix: ' ' },
				{ name: 'bt_blocklist_url', type: 'text', maxlen: 80, size: 60, value: nvram.bt_blocklist_url } ] },
			{ title: 'Custom Log File Path', multi: [
				{ name: 'f_bt_log', type: 'checkbox', value: nvram.bt_log == 1, suffix: ' ' },
				{ name: 'bt_log_path', type: 'text', maxlen: 80, size: 60, value: nvram.bt_log_path, suffix: ' /transmission.log' } ] },
			null,
			{ title: '<a href="https://github.com/transmission/transmission/wiki/Editing-Configuration-Files" class="new_window">Transmission<\/a><br>Custom configuration', name: 'bt_custom', type: 'textarea', value: nvram.bt_custom }
		]);
	</script>
</div>

<!-- / / / -->

<div class="section-title">Notes <small><i><a href="javascript:toggleVisibility(cprefix,'notes');"><span id="sesdiv_notes_showhide">(Show)</span></a></i></small></div>
<div class="section" id="sesdiv_notes" style="display:none">
	<ul>
		<li><b>Enable on Start</b> - Caution! If your router only has 32MB of RAM, you'll have to use swap.</li>
		<li><b>Transmission binary path</b> - Path to the directory containing transmission-daemon etc.</li>
		<li><b>Keep alive</b> - If enabled, transmission-daemon will be checked at the specified interval and will re-launch after a crash.</li>
		<li><b>Listening port</b> - Port used for torrent client. Make sure this port is not in use.</li>
		<li><b>Autoadd .torrents</b> - Search and add .torrent files from Download directory. Other watch directory can be setup with 'watch-dir' parameter in "Custom configuration".</li>
		<li><b>Listening GUI port</b> - Port used for Transmission GUI. Make sure this port is not in use.</li>
		<li><b>Authentication required</b> - Authentication is <b><i>highly recomended</i></b>. GUI will prompt for user/pass.</li>
		<li><b>Allow remote access</b> - This option will open the Transmission GUI port from the WAN side and allow the GUI to be accessed from the internet.</li>
		<li><b>Downloads queuing</b> - If enabled, this option will limit how many torrents can be downloaded at once.</li>
		<li><b>Seeds queuing</b> - If enabled, this option will limit how many torrents can be uploaded/seeded at once.</li>
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
