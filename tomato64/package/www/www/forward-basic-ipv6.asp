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
<title>[<% ident(); %>] Forwarding: Basic IPv6</title>
<link rel="stylesheet" type="text/css" href="tomato.css?rel=<% version(); %>">
<% css(); %>
<script src="isup.jsz?_http_id=<% nv(http_id); %>"></script>
<script src="tomato.js?rel=<% version(); %>"></script>

<script>

//	<% nvram("ipv6_portforward"); %>

function show() {
	elem.setInnerHTML('notice_container', '<div id="notice">'+isup.notice_ip6tables.replace(/\n/g, '<br>')+'<\/div><br style="clear:both">');
	elem.display('notice_container', isup.notice_ip6tables != '');
}

var fog = new TomatoGrid();

fog.setup = function() {
	this.init('fo-grid6', 'sort', 100, [
		{ type: 'checkbox', prefix: '<div class="centered">', suffix: '<\/div>' },
		{ type: 'select', options: [[1, 'TCP'],[2, 'UDP'],[3,'Both']] },
		{ type: 'text', maxlen: 140 },
		{ type: 'text', maxlen: 140 },
		{ type: 'text', maxlen: 16 },
		{ type: 'text', maxlen: 32 }]);
	this.headerSet(['On','Protocol','Src Address','Dest Address','Dest Ports','Description']);
	var nv = nvram.ipv6_portforward.split('>');
	for (var i = 0; i < nv.length; ++i) {
		var r;

		if (r = nv[i].match(/^(\d)<(\d)<(.*)<(.*)<(.+?)<(.*)$/)) {
			r[1] *= 1;
			r[2] *= 1;
			r[5] = r[5].replace(/:/g, '-');

			fog.insertData(-1, r.slice(1, 7));
		}
	}
	fog.sort(5);
	fog.showNewEditor();
}

fog.resetNewEditor = function() {
	var f = fields.getAll(this.newEditor);
	f[0].checked = 1;
	f[1].selectedIndex = 0;
	f[2].value = '';
	f[3].value = '';
	f[4].value = '';
	f[5].value = '';
	ferror.clearAll(fields.getAll(this.newEditor));
}

fog.dataToView = function(data) {
	return [(data[0] != '0') ? '&#x2b50' : '', ['TCP', 'UDP', 'Both'][data[1] - 1], (data[2].match(/(.+)-(.+)/)) ? (RegExp.$1 + ' -<br>' + RegExp.$2) : data[2], data[3], data[4], data[5]];
}

fog.fieldValuesToData = function(row) {
	var f = fields.getAll(row);

	return [f[0].checked ? 1 : 0,f[1].value,f[2].value,f[3].value,f[4].value,f[5].value];
}

fog.sortCompare = function(a, b) {
	var col = this.sortColumn;
	var da = a.getRowData();
	var db = b.getRowData();
	var r;

	switch (col) {
	case 0:
	case 1:
	case 4:
		r = cmpInt(da[col], db[col]);
		break;
	default:
		r = cmpText(da[col], db[col]);
		break;
	}

	return this.sortAscending ? r : -r;
}

fog.verifyFields = function(row, quiet) {
	var f = fields.getAll(row);

	f[2].value = f[2].value.trim();
	if ((f[2].value.length) && (!_v_iptaddr(f[2], quiet, 0, 0, 1)))
		return 0;

	f[3].value = f[3].value.trim();	
	if ((f[3].value.length) && !v_hostname(f[3], 1)) {
		if (!_v_iptaddr(f[3], quiet, 0, 0, 1))
			return 0;
	}

	if (!v_iptport(f[4], quiet))
		return 0;

	f[5].value = f[5].value.replace(/>/g, '_');
	if (!v_nodelim(f[5], quiet, 'Description'))
		return 0;

	return 1;
}

function srcSort(a, b) {
	if (a[2].length)
		return -1;
	if (b[2].length)
		return 1;

	return 0;
}

function save() {
	if (fog.isEditing())
		return;

	var data = fog.getAllData().sort(srcSort);
	var s = '';
	for (var i = 0; i < data.length; ++i) {
		data[i][4] = data[i][4].replace(/-/g, ':');
		s += data[i].join('<') + '>';
	}
	var fom = E('t_fom');
	fom.ipv6_portforward.value = s;

	form.submit(fom, 1, 'tomato.cgi');
}

function init() {
	fog.recolor();
	fog.resetNewEditor();
	up.initPage(250, 5);
}
</script>
</head>

<body onload="init()">
<form id="t_fom" method="post" action="javascript:{}">
<table id="container">
<tr><td colspan="2" id="header">
	<div class="title"><a href="/">Tomato64</a></div>
	<div class="version">Version <% version(); %> on <% nv("t_model_name"); %></div>
</td></tr>
<tr id="body"><td id="navi"><script>navi()</script></td>
<td id="content">
<div id="ident"><% ident(); %> | <script>wikiLink();</script></div>

<!-- / / / -->

<input type="hidden" name="_nextpage" value="forward-basic-ipv6.asp">
<input type="hidden" name="_service" value="firewall-restart">
<input type="hidden" name="ipv6_portforward">

<!-- / / / -->

<div class="section-title">IPv6 Port Forwarding</div>
<div class="section">
	<div class="tomato-grid" id="fo-grid6"></div>
</div>

<!-- / / / -->

<div class="section-title">Notes</div>
<div class="section">
	<i>Opens access to ports on machines inside the LAN, but does <b>not</b> re-map ports:</i><br>
	<ul>
		<li><b>Src Address</b> <i>(optional)</i> - Forward only if from this address. Ex: "2001:4860:800b::/48", "me.example.com".</li>
		<li><b>Dest Address</b> <i>(optional)</i> - The destination address inside the LAN.</li>
		<li><b>Dest Ports</b> - The ports to be opened for forwarding. Ex: "2345", "200,300", "200-300,400".</li>
	</ul>
</div>

<!-- / / / -->

<div id="notice_container" style="display:none">&nbsp;</div>

<!-- / / / -->

<div id="footer">
	<span id="footer-msg"></span>
	<input type="button" value="Save" id="save-button" onclick="save()">
	<input type="button" value="Cancel" id="cancel-button" onclick="reloadPage();">
</div>

</td></tr>
</table>
</form>
<script>fog.setup();</script>
</body>
</html>
