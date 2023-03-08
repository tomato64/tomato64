<!DOCTYPE html>
<!--
	Tomato MySQL GUI
	Copyright (C) 2014 Hyzoom, bwq518@gmail.com
	http://openlinksys.info
	For use with Tomato Shibby Firmware only.
	No part of this file may be used without permission.
-->
<html lang="en-GB">
<head>
<meta http-equiv="content-type" content="text/html;charset=utf-8">
<meta name="robots" content="noindex,nofollow">
<title>[<% ident(); %>] MySQL Database Server</title>
<link rel="stylesheet" type="text/css" href="tomato.css">
<% css(); %>
<script src="isup.jsz"></script>
<script src="isup.js"></script>
<script src="tomato.js"></script>

<script>

//	<% nvram("mysql_enable,mysql_sleep,mysql_check_time,mysql_binary,mysql_binary_custom,mysql_usb_enable,mysql_dlroot,mysql_datadir,mysql_tmpdir,mysql_server_custom,mysql_port,mysql_allow_anyhost,mysql_init_rootpass,mysql_username,mysql_passwd,mysql_key_buffer,mysql_max_allowed_packet,mysql_thread_stack,mysql_thread_cache_size,mysql_init_priv,mysql_table_open_cache,mysql_sort_buffer_size,mysql_read_buffer_size,mysql_query_cache_size,mysql_read_rnd_buffer_size,mysql_max_connections,nginx_port"); %>

//	<% usbdevices(); %>

var cprefix = 'web_mysql';
var changed = 0;
var reinit = 0;
var serviceType = 'mysqld';
var usb_disk_list = new Array();

function refresh_usb_disk() {
	var i, j, k, a, b, c, e, s, desc, d, parts, p;
	var partcount;
	var list = [];
	for (i = 0; i < list.length; ++i) {
		list[i].type = '';
		list[i].host = '';
		list[i].vendor = '';
		list[i].product = '';
		list[i].serial = '';
		list[i].discs = [];
		list[i].is_mounted = 0;
	}
	for (i = usbdev.length - 1; i >= 0; --i) {
		a = usbdev[i];
		e = {
			type: a[0],
			host: a[1],
			vendor: a[2],
			product: a[3],
			serial: a[4],
			discs: a[5],
			is_mounted: a[6]
		};
		list.push(e);
	}
	partcount = 0;
	for (i = list.length - 1; i >= 0; --i) {
		e = list[i];
		if (e.discs) {
			for (j = 0; j <= e.discs.length - 1; ++j) {
				d = e.discs[j];
				parts = d[1];
				for (k = 0; k <= parts.length - 1; ++k) {
					p = parts[k];
					if ((p) && (p[1] >= 1) && (p[3] != 'swap')) {
						usb_disk_list[partcount] = new Array();
						usb_disk_list[partcount][0] = p[2];
						usb_disk_list[partcount][1] = 'Partition '+p[0]+' mounted on '+p[2]+' ('+p[3]+ ' - '+doScaleSize(p[6])+ ' available, total '+doScaleSize(p[5])+')';
						partcount++;
					}
				}
			}
		}
	}
	list = [];
}

function verifyFields(focused, quiet) {
	if (focused && focused != E('_f_mysql_enable')) /* except on/off */
		changed = 1;
	if (focused && (focused == E('_f_mysql_init_priv') || focused == E('_f_mysql_init_rootpass'))) /* init tables, init password */
		reinit = 1;

	var ok = 1;
	var a = E('_f_mysql_usb_enable').checked;
	var b = E('_f_mysql_init_rootpass').checked;

	E('_mysql_username').disabled = true;
	E('_mysql_passwd').disabled = !b;
	E('_mysql_dlroot').disabled = !a;
	
	elem.display('_mysql_binary_custom', (E('_mysql_binary').value == 'custom'));
	elem.display('_mysql_dlroot', a);

	var x;
	if (b)
		x = '';
	else
		x = 'none';

	PR(E('_mysql_username')).style.display = x;
	PR(E('_mysql_passwd')).style.display = x;

	var e = E('_mysql_passwd');
	if (e.value.trim() == '') {
		ferror.set(e, 'Password can not be empty', quiet);
		ok = 0;
	}
	else
		ferror.clear(e);

	if (!v_port(E('_mysql_port'), quiet || !ok)) ok = 0;
	if (!v_range(E('_mysql_check_time'), quiet || !ok, 0, 55)) ok = 0;
	if (!v_range(E('_mysql_sleep'), quiet || !ok, 1, 60)) ok = 0;
	if (!v_range(E('_mysql_key_buffer'), quiet || !ok, 1, 1024)) ok = 0;
	if (!v_range(E('_mysql_max_allowed_packet'), quiet || !ok, 1, 1024000)) ok = 0;
	if (!v_range(E('_mysql_thread_stack'), quiet || !ok, 1, 1024000)) ok = 0;
	if (!v_range(E('_mysql_thread_cache_size'), quiet || !ok, 1, 999999)) ok = 0;
	if (!v_range(E('_mysql_table_open_cache'), quiet || !ok, 1, 999999)) ok = 0;
	if (!v_range(E('_mysql_query_cache_size'), quiet || !ok, 1, 1024)) ok = 0;
	if (!v_range(E('_mysql_sort_buffer_size'), quiet || !ok, 1, 1024000)) ok = 0;
	if (!v_range(E('_mysql_read_buffer_size'), quiet || !ok, 1, 1024000)) ok = 0;
	if (!v_range(E('_mysql_read_rnd_buffer_size'), quiet || !ok, 1, 1024000)) ok = 0;
	if (!v_range(E('_mysql_max_connections'), quiet || !ok, 1, 999999)) ok = 0;

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

	if (!isup.mysqld && !nomsg && (fom._f_mysql_init_priv.checked || fom._f_mysql_init_rootpass.checked)) {
		alert('MySQL is not running.\nTo (re-)init priv. table and/or root password, check config and click "Start Now"');
		return;
	}
	if (isup.mysqld && nomsg && (fom._f_mysql_init_priv.checked || fom._f_mysql_init_rootpass.checked)) {
		E('_mysqld_button').disabled = 0;
		E('_mysqld_interface').disabled = 0;
		E('spin').style.display = 'none';
		alert('Check config, and click "Save" to (re-)init priv. table and/or root password');
		return;
	}

	fom.mysql_enable.value = fom._f_mysql_enable.checked ? 1 : 0;
	fom.mysql_usb_enable.value = fom._f_mysql_usb_enable.checked ? 1 : 0;
	fom.mysql_init_priv.value = fom._f_mysql_init_priv.checked ? 1 : 0;
	fom.mysql_init_rootpass.value = fom._f_mysql_init_rootpass.checked ? 1 : 0;
	fom.mysql_allow_anyhost.value = fom._f_mysql_allow_anyhost.checked ? 1 : 0;
	fom._nofootermsg.value = (nomsg ? 1 : 0);

	form.submit(fom, 1);

	/* reset */
	changed = 0;
	reinit = 0;
	nvram.mysql_init_priv = 0;
	nvram.mysql_init_rootpass = 0;
	fom._f_mysql_init_priv.checked = 0;
	fom._f_mysql_init_rootpass.checked = 0;
	PR(E('_mysql_username')).style.display = 'none';
	PR(E('_mysql_passwd')).style.display = 'none';
}

function earlyInit() {
	show();
	verifyFields(null, 1);
}

function init() {
	var c;
	if (((c = cookie.get(cprefix+'_notes_vis')) != null) && (c == '1'))
		toggleVisibility(cprefix, 'notes');

	up.initPage(250, 5);
	eventHandler();
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

<input type="hidden" name="_nextpage" value="web-mysql.asp">
<input type="hidden" name="_service" value="">
<input type="hidden" name="_nofootermsg">
<input type="hidden" name="mysql_enable">
<input type="hidden" name="mysql_usb_enable">
<input type="hidden" name="mysql_init_priv">
<input type="hidden" name="mysql_init_rootpass">
<input type="hidden" name="mysql_allow_anyhost">

<!-- / / / -->

<div class="section-title">Status</div>
<div class="section">
	<div class="fields">
		<span id="_mysqld_notice"></span>
		<input type="button" id="_mysqld_button">
		<input type="button" id="_mysqld_interface" value="Open admin interface in new tab" class="new_window" onclick="window.open('http://'+location.hostname+':'+nvram.nginx_port+'/adminer.php')">
		&nbsp; <img src="spin.gif" alt="" id="spin">
	</div>
</div>

<!-- / / / -->

<div class="section-title">Basic Settings</div>
<div class="section">
	<script>
		refresh_usb_disk();

		createFieldTable('', [
			{ title: 'Enable on Start', name: 'f_mysql_enable', type: 'checkbox', value: nvram.mysql_enable == 1 },
			{ title: 'MySQL binary path', multi: [
				{ name: 'mysql_binary', type: 'select', options: [
					['internal','Internal (/usr/bin)'],
					['optware','Optware (/opt/bin)'],
					['custom','Custom'] ], value: nvram.mysql_binary },
				{ name: 'mysql_binary_custom', type: 'text', maxlen: 40, size: 40, value: nvram.mysql_binary_custom }
			] },
			{ title: 'Poll Interval', name: 'mysql_check_time', type: 'text', maxlen: 5, size: 7, value: nvram.mysql_check_time, suffix: ' <small>minutes; range: 0 - 55; default: 5; 0 to disable<\/small>' },
			{ title: 'Delay at startup', name: 'mysql_sleep', type: 'text', maxlen: 5, size: 7, value: nvram.mysql_sleep, suffix: ' <small>seconds; range: 1 - 60; default: 2<\/small>' },
			{ title: 'MySQL listen port', name: 'mysql_port', type: 'text', maxlen: 5, size: 7, value: nvram.mysql_port, suffix: ' <small> default: 3306<\/small>' },
			{ title: 'Allow Anyhost to access', name: 'f_mysql_allow_anyhost', type: 'checkbox', value: nvram.mysql_allow_anyhost == 1 },
			{ title: 'Re-init priv. table', name: 'f_mysql_init_priv', type: 'checkbox', value: nvram.mysql_init_priv == 1 },
			{ title: 'Re-init root password', name: 'f_mysql_init_rootpass', type: 'checkbox', value: nvram.mysql_init_rootpass == 1 },
			{ title: 'root user name', name: 'mysql_username', type: 'text', maxlen: 32, size: 16, value: nvram.mysql_username, suffix: ' <small>username connected to server (default: root)<\/small>' },
			{ title: 'root password', name: 'mysql_passwd', type: 'password', maxlen: 32, size: 16, peekaboo: 1, value: nvram.mysql_passwd, suffix: ' <small>empty not allowed (default: admin)<\/small>' },
			{ title: 'Enable USB Partition', multi: [
				{ name: 'f_mysql_usb_enable', type: 'checkbox', value: nvram.mysql_usb_enable == 1, suffix: '&nbsp; ' },
				{ name: 'mysql_dlroot', type: 'select', options: usb_disk_list, value: nvram.mysql_dlroot} ] },
			{ title: 'Data dir.', indent: 2, name: 'mysql_datadir', type: 'text', maxlen: 50, size: 40, value: nvram.mysql_datadir },
			{ title: 'Tmp dir.', indent: 2, name: 'mysql_tmpdir', type: 'text', maxlen: 50, size: 40, value: nvram.mysql_tmpdir }
		]);
	</script>
</div>

<!-- / / / -->

<div class="section-title">Advanced Settings</div>
<div class="section">
	<script>
		createFieldTable('', [
			{ title: 'Key buffer', name: 'mysql_key_buffer', type: 'text', maxlen: 10, size: 10, value: nvram.mysql_key_buffer, suffix: ' <small>MB; range: 1 - 1024; default: 16<\/small>' },
			{ title: 'Max allowed packet', name: 'mysql_max_allowed_packet', type: 'text', maxlen: 10, size: 10, value: nvram.mysql_max_allowed_packet, suffix: ' <small>MB; range: 1 - 1024; default: 4<\/small>' },
			{ title: 'Thread stack', name: 'mysql_thread_stack', type: 'text', maxlen: 10, size: 10, value: nvram.mysql_thread_stack, suffix: ' <small>KB; range: 1 - 1024000; default: 128<\/small>' },
			{ title: 'Thread cache size', name: 'mysql_thread_cache_size', type: 'text', maxlen: 10, size: 10, value: nvram.mysql_thread_cache_size, suffix: ' <small>range: 1 - 999999; default: 8<\/small>' },
			{ title: 'Table open cache', name: 'mysql_table_open_cache', type: 'text', maxlen: 10, size: 10, value: nvram.mysql_table_open_cache, suffix: ' <small>range: 1 - 999999; default: 4<\/small>' },
			{ title: 'Query cache size', name: 'mysql_query_cache_size', type: 'text', maxlen: 10, size: 10, value: nvram.mysql_query_cache_size, suffix: ' <small>MB; range: 0 - 1024; default: 16<\/small>' },
			{ title: 'Sort buffer size', name: 'mysql_sort_buffer_size', type: 'text', maxlen: 10, size: 10, value: nvram.mysql_sort_buffer_size, suffix: ' <small>KB; range: 0 - 1024000; default: 128<\/small>' },
			{ title: 'Read buffer size', name: 'mysql_read_buffer_size', type: 'text', maxlen: 10, size: 10, value: nvram.mysql_read_buffer_size, suffix: ' <small>KB; range: 0 - 1024000; default: 128<\/small>' },
			{ title: 'Read rand buffer size', name: 'mysql_read_rnd_buffer_size', type: 'text', maxlen: 10, size: 10, value: nvram.mysql_read_rnd_buffer_size, suffix: ' <small>KB; range: 1 - 1024000; default: 256<\/small>' },
			{ title: 'Max connections', name: 'mysql_max_connections', type: 'text', maxlen: 10, size: 10, value: nvram.mysql_max_connections, suffix: ' <small>range: 0 - 999999; default: 100<\/small>' },
			{ title: 'Custom Config', name: 'mysql_server_custom', type: 'textarea', value: nvram.mysql_server_custom }
		]);
	</script>
</div>

<!-- / / / -->

<div class="section-title">Notes <small><i><a href='javascript:toggleVisibility(cprefix,"notes");'><span id="sesdiv_notes_showhide">(Show)</span></a></i></small></div>
<div class="section" id="sesdiv_notes" style="display:none">
	<ul>
		<li><b>Status Button</b> - Quick Start-Stop Service.</li>
		<li><b>Open admin interface in new tab</b> - Warning! nginx must be configured and running!</li>
		<li><b>Enable on Start</b> - Check to activate the mysqld at the router start. Caution! If your router has only 32MB of RAM, you'll have to use swap.</li>
		<li><b>MySQL binary path</b> - Path to the directory containing mysqld etc. Do not include program name (/mysqld).</li>
		<li><b>Poll Interval</b> - If enabled, mysqld will be checked at the specified interval and will re-launch after a crash.</li>
		<li><b>Allow Anyhost to access</b> - Allow any hosts to access database server.</li>
		<li><b>Re-init priv. table</b> -  If checked, privileges table will be forced to re-initialize by mysql_install_db.</li>
		<li><b>Re-init root password</b> - If checked, root password will be forced to re-initialize.</li>
		<li><b>Data and Tmp dir</b> - Subdirectory under mounted partition. Attention! Do not use NAND for datadir and tmpdir.</li>
		<li><b>Custom Config</b> - Input like: param=value, e.g. connect_timeout=10</li>
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
