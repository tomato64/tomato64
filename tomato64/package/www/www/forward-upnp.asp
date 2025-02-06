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
<title>[<% ident(); %>] Forwarding: UPnP IGD &amp; PCP/NAT-PMP</title>
<link rel="stylesheet" type="text/css" href="tomato.css?rel=<% version(); %>">
<% css(); %>
<script src="isup.jsz?rel=<% version(); %>"></script>
<script src="tomato.js?rel=<% version(); %>"></script>

<script>

/* TOMATO64-REMOVE-BEGIN */
//	<% nvram("upnp_enable,upnp_secure,upnp_custom,upnp_lan,upnp_lan1,upnp_lan2,upnp_lan3,lan_ifname,lan1_ifname,lan2_ifname,lan3_ifname"); %>
/* TOMATO64-REMOVE-END */
/* TOMATO64-BEGIN */
//	<% nvram("upnp_enable,upnp_secure,upnp_custom,upnp_lan,upnp_lan1,upnp_lan2,upnp_lan3,upnp_lan4,upnp_lan5,upnp_lan6,upnp_lan7,lan_ifname,lan1_ifname,lan2_ifname,lan3_ifname,lan4_ifname,lan5_ifname,lan6_ifname,lan7_ifname"); %>
/* TOMATO64-END */

</script>
<script src="upnp.jsx?_http_id=<% nv(http_id); %>"></script>

<script>

var upnp = new TomatoRefresh('upnp.jsx?_http_id=<% nv(http_id); %>', '', 30);

nvram.upnp_enable = fixInt(nvram.upnp_enable, 0, 3, 0);

var xob = null;

function _upnpNvramAdd() {
	form.submitHidden('service.cgi', { _service: 'upnp_nvram-start', _sleep: 1 });
}

function upnpNvramAdd() {
	var sb, cb, msg;

	/* short check for upnp nvram var. If OK - nothing to do! */
	if ((nvram.upnp_secure.length > 0) || (nvram.upnp_enable > 0))
		return;

	E('_f_enable_upnp').disabled = 1;
	E('_f_enable_pcp_pmp').disabled = 1;
	if ((sb = E('save-button')) != null) sb.disabled = 1;
	if ((cb = E('cancel-button')) != null) cb.disabled = 1;

	if (!confirm("Add UPnP IGD & PCP/NAT-PMP autonomous port forwarding service to NVRAM?"))
		return;

	if (xob)
		return;

	if ((xob = new XmlHttp()) == null) {
		_upnpNvramAdd();
		return;
	}

	if ((msg = E('footer-msg')) != null) {
		msg.innerHTML = 'adding nvram values...';
		msg.style.display = 'inline';
	}

	xob.onCompleted = function(text, xml) {
		if (msg) {
			msg.innerHTML = 'nvram ready';
		}
		setTimeout(
			function() {
				E('_f_enable_upnp').disabled = 0;
				E('_f_enable_pcp_pmp').disabled = 0;
				if (sb) sb.disabled = 0;
				if (cb) cb.disabled = 0;
				if (msg) msg.style.display = 'none';
				setTimeout(reloadPage, 1000);
		}, 5000);
		xob = null;
	}
	xob.onError = function() {
		_upnpNvramAdd();
	}

	xob.post('service.cgi', '_service=upnp_nvram-start'+'&'+'_sleep=1'+'&'+'_ajax=1');
}

upnp.refresh = function(text) {
	try {
		eval(text);
	}
	catch (ex) {
	}
	refresh();
}

function show() {
	if (!isup.miniupnpd) {
		ug.removeAllData();
		ug.rpHide();
		if (upnp.running)
			upnp.stop();
	}
	else {
		refresh();
		if (!upnp.running)
			upnp.start();
	}
	elem.display(E('show_ports'), isup.miniupnpd);
}

function refresh() {
	ug.removeAllData();
	ug.populate();
	ug.recolor();
	ug.resort();
	ug.rpHide();
}

function submitDelete(proto, eport) {
	var fom = E('t_fom');

	fom.remove_proto.value = proto;
	fom.remove_eport.value = eport;
	fom._nofootermsg.value = 1;

	form.submit(fom, 1, 'upnp.cgi');

	upnp.start();
	refresh();
}

function deleteData(data) {
	if (!confirm('Delete port forward for '+data[0]+':'+data[1]+'/'+data[3]+' ('+data[5]+')?'))
		return;

	submitDelete(data[3], data[2]);
}

function deleteAll() {
	if (!confirm('Delete all port forwards?'))
		return;

	submitDelete('*', '0');
}

function padnum(num, length) {
	num = num.toString();
	while (num.length < length) num = "0" + num;
	return num;
}

var ug = new TomatoGrid();

ug.setup = function() {
	this.init('upnp-grid', 'sort delete');
	this.headerSet(['Client Address', 'Client Port', 'Ext Port', 'Protocol', 'Expires', 'Description']);
	ug.populate();
	this.sort(0);
}

ug.populate = function() {
	var i, j, r, row, data;

	if (isup.miniupnpd && typeof(window.mupnp_data) != 'undefined') {
		var data = mupnp_data.split('\n');
		for (i = 0; i < data.length; ++i) {
			r = data[i].match(/^(UDP|TCP)\s+(\d+)\s+(.+?)\s+(\d+)\s+\[(.*)\](.*)$/);
			if (r == null)
				continue;

			var expires_sec, hour, minute, second, expires_str = '';
			expires_sec = (new Date(r[6] * 1000) - new Date().getTime()) / 1000;
			hour = Math.floor(expires_sec / 3600);
			minute = Math.floor(expires_sec % 3600 / 60);
			second = Math.floor(expires_sec % 60);
			if (hour > 0) {
				expires_str += hour + 'h ';
				expires_str += padnum(minute, 2) + 'm ';
				expires_str += padnum(second, 2) + 's';
			} else if (minute > 0) {
				expires_str += minute + 'm ';
				expires_str += padnum(second, 2) + 's';
			} else if (expires_sec > 0) expires_str += second + 's';

			row = this.insertData(-1, [r[3], r[4], r[2], r[1], expires_str, r[5]]);

			if (!r[0]) {
				for (j = 0; j < 6; ++j)
					elem.addClass(row.cells[j], 'disabled');
			}
			for (j = 0; j < 6; ++j)
				row.cells[j].title = 'Delete';
		}
	}
	E('upnp-delete-all').disabled = (ug.getDataCount() == 0);
}

ug.rpDel = function(e) {
	deleteData(PR(e).getRowData());
}

ug.onClick = function(cell) {
	deleteData(cell.parentNode.getRowData());
}

function verifyFields(focused, quiet) {
	var enable = (E('_f_enable_upnp').checked || E('_f_enable_pcp_pmp').checked);

	E('_f_upnp_allow_third_party').disabled = !enable;
	E('_upnp_custom').disabled = !enable;

	for (var i = 0 ; i <= MAX_BRIDGE_ID ; i++) {
		var j = (i == 0) ? '' : i.toString();
		E('_f_upnp_lan'+j).disabled = ((nvram['lan'+j+'_ifname'].length < 1) || !enable);
		if (E('_f_upnp_lan'+j).disabled)
			E('_f_upnp_lan'+j).checked = 0;
	}

/* TOMATO64-REMOVE-BEGIN */
	if ((enable) && (!E('_f_upnp_lan').checked) && (!E('_f_upnp_lan1').checked) && (!E('_f_upnp_lan2').checked) && (!E('_f_upnp_lan3').checked)) {
/* TOMATO64-REMOVE-END */
/* TOMATO64-BEGIN */
	if ((enable) && (!E('_f_upnp_lan').checked) && (!E('_f_upnp_lan1').checked) && (!E('_f_upnp_lan2').checked) && (!E('_f_upnp_lan3').checked) &&
			(!E('_f_upnp_lan4').checked) && (!E('_f_upnp_lan5').checked) && (!E('_f_upnp_lan6').checked) && (!E('_f_upnp_lan7').checked)) {
/* TOMATO64-END */
		if ((E('_f_enable_pcp_pmp').checked) || (E('_f_enable_upnp').checked)) {
			var m = 'Must be enabled on at least one LAN interface if no custom configuration is used, otherwise the service will not run';
			ferror.set('_f_enable_pcp_pmp', m, quiet);
			ferror.set('_f_enable_upnp', m, 1);
			ferror.set('_f_upnp_lan', m, 1);
			ferror.set('_f_upnp_lan1', m, 1);
			ferror.set('_f_upnp_lan2', m, 1);
			ferror.set('_f_upnp_lan3', m, 1);
/* TOMATO64-BEGIN */
			ferror.set('_f_upnp_lan4', m, 1);
			ferror.set('_f_upnp_lan5', m, 1);
			ferror.set('_f_upnp_lan6', m, 1);
			ferror.set('_f_upnp_lan7', m, 1);
/* TOMATO64-END */
		}
		return 1;
	}
	else {
		ferror.clear('_f_enable_pcp_pmp');
		ferror.clear('_f_enable_upnp');
		ferror.clear('_f_upnp_lan');
		ferror.clear('_f_upnp_lan1');
		ferror.clear('_f_upnp_lan2');
		ferror.clear('_f_upnp_lan3');
/* TOMATO64-BEGIN */
		ferror.clear('_f_upnp_lan4');
		ferror.clear('_f_upnp_lan5');
		ferror.clear('_f_upnp_lan6');
		ferror.clear('_f_upnp_lan7');
/* TOMATO64-END */
	}

	return 1;
}

function save() {
	if (!verifyFields(null, 0))
		return;

	var fom = E('t_fom');

	fom.upnp_enable.value = 0;
	if (fom.f_enable_upnp.checked)
		fom.upnp_enable.value = 1;
	if (fom.f_enable_pcp_pmp.checked)
		fom.upnp_enable.value |= 2;

	fom.upnp_secure.value = fom._f_upnp_allow_third_party.checked ? 0 : 1;

	fom.upnp_lan.value = fom._f_upnp_lan.checked ? 1 : 0;
	fom.upnp_lan1.value = fom._f_upnp_lan1.checked ? 1 : 0;
	fom.upnp_lan2.value = fom._f_upnp_lan2.checked ? 1 : 0;
	fom.upnp_lan3.value = fom._f_upnp_lan3.checked ? 1 : 0;
/* TOMATO64-BEGIN */
	fom.upnp_lan4.value = fom._f_upnp_lan4.checked ? 1 : 0;
	fom.upnp_lan5.value = fom._f_upnp_lan5.checked ? 1 : 0;
	fom.upnp_lan6.value = fom._f_upnp_lan6.checked ? 1 : 0;
	fom.upnp_lan7.value = fom._f_upnp_lan7.checked ? 1 : 0;
/* TOMATO64-END */

	fom._nofootermsg.value = 0;

	form.submit(fom, 1);
}

function earlyInit() {
	ug.setup();
	show();
	verifyFields(null, 1);
}

function init() {
	upnpNvramAdd();
	ug.recolor();
	up.initPage(250, 5);
	upnp.initPage(250, 30);
}
</script>

</head>
<body onload="init()">
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

<input type="hidden" name="_nextpage" value="forward-upnp.asp">
<input type="hidden" name="_service" value="upnp-restart">
<input type="hidden" name="_nofootermsg" value="">
<input type="hidden" name="upnp_enable">
<input type="hidden" name="upnp_secure">
<input type="hidden" name="upnp_lan">
<input type="hidden" name="upnp_lan1">
<input type="hidden" name="upnp_lan2">
<input type="hidden" name="upnp_lan3">
/* TOMATO64-BEGIN */
<input type="hidden" name="upnp_lan4">
<input type="hidden" name="upnp_lan5">
<input type="hidden" name="upnp_lan6">
<input type="hidden" name="upnp_lan7">
/* TOMATO64-END */
<input type="hidden" name="remove_proto">
<input type="hidden" name="remove_eport">

<!-- / / / -->

<div id="show_ports">
	<div class="section-title">Active Port Forwards</div>
	<div class="section">
		<div class="tomato-grid" id="upnp-grid"></div>
		<div style="width:100%;text-align:right"><img src="spin.gif" id="refresh-spinner" alt=""> &nbsp;<input type="button" value="Delete All" onclick="deleteAll()" id="upnp-delete-all"></div>
	</div>
</div>

<!-- / / / -->

<div class="section-title">UPnP IGD &amp; PCP/NAT-PMP Service Settings</div>
<div class="section">
	<script>
		createFieldTable('', [
			{ title: 'Enable UPnP IGD', name: 'f_enable_upnp', type: 'checkbox', suffix: ' <small>This protocol is often used by Microsoft-compatible systems<\/small>', value: (nvram.upnp_enable & 1) },
			{ title: 'Enable PCP/NAT-PMP', name: 'f_enable_pcp_pmp', type: 'checkbox', suffix: ' <small>These protocols are often used by Apple-compatible systems<\/small>', value: (nvram.upnp_enable & 2) },
			{ title: 'Enabled on' },
				{ title: 'LAN0', indent: 2, name: 'f_upnp_lan', type: 'checkbox', value: (nvram.upnp_lan == 1) },
				{ title: 'LAN1', indent: 2, name: 'f_upnp_lan1', type: 'checkbox', value: (nvram.upnp_lan1 == 1) },
				{ title: 'LAN2', indent: 2, name: 'f_upnp_lan2', type: 'checkbox', value: (nvram.upnp_lan2 == 1) },
				{ title: 'LAN3', indent: 2, name: 'f_upnp_lan3', type: 'checkbox', value: (nvram.upnp_lan3 == 1) },
/* TOMATO64-BEGIN */
				{ title: 'LAN4', indent: 2, name: 'f_upnp_lan4', type: 'checkbox', value: (nvram.upnp_lan4 == 1) },
				{ title: 'LAN5', indent: 2, name: 'f_upnp_lan5', type: 'checkbox', value: (nvram.upnp_lan5 == 1) },
				{ title: 'LAN6', indent: 2, name: 'f_upnp_lan6', type: 'checkbox', value: (nvram.upnp_lan6 == 1) },
				{ title: 'LAN7', indent: 2, name: 'f_upnp_lan7', type: 'checkbox', value: (nvram.upnp_lan7 == 1) },
/* TOMATO64-END */
			{ title: 'Allow third-party forwarding', name: 'f_upnp_allow_third_party', type: 'checkbox', suffix: ' <small>Allow adding port forwards for non-requesting IP addresses<\/small>', value: (nvram.upnp_secure == 0) },
			{ title: 'Custom Configuration', name: 'upnp_custom', type: 'textarea', value: nvram.upnp_custom }
		]);
	</script>
</div>

<div class="section-title">Notes</div>
<div class="section">
	<ul>
		<li><b>NVRAM</b> - If UPnP IGD or PCP/NAT-PMP has been enabled, NVRAM values will be added. These NVRAM values will be removed after a reboot if the service is disabled.</li>
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
