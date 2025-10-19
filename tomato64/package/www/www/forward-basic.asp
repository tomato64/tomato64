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
<title>[<% ident(); %>] Forwarding: Basic</title>
<link rel="stylesheet" type="text/css" href="tomato.css?rel=<% version(); %>">
<% css(); %>
<script src="isup.jsx?_http_id=<% nv(http_id); %>"></script>
<script src="tomato.js?rel=<% version(); %>"></script>

<script>

//	<% nvram("portforward"); %>

function show() {
	elem.setInnerHTML('notice_container', '<div id="notice">'+isup.notice_iptables.replace(/\n/g, '<br>')+'<\/div><br style="clear:both">');
	elem.display('notice_container', isup.notice_iptables != '');
}

var fog = new TomatoGrid();

fog.setup = function() {
	this.init('fo-grid', 'sort', 100, [
		{ type: 'checkbox', prefix: '<div class="centered">', suffix: '<\/div>' },
		{ type: 'select', options: [[1,'TCP'],[2,'UDP'],[3,'Both']] },
		{ type: 'text', maxlen: 32 },
		{ type: 'text', maxlen: 16 },
		{ type: 'text', maxlen: 5 },
		{ type: 'text', maxlen: 15 },
		{ type: 'text', maxlen: 32 }]);

	this.headerSet(['On','Protocol','Src Address','Ext Ports','Int Port','Int Address','Description']);
	var nv = nvram.portforward.split('>');
	for (var i = 0; i < nv.length; ++i) {
		var r;

		if (r = nv[i].match(/^(\d)<(\d)<(.*)<(.+?)<(\d*)<(.*)<(.*)$/)) {
			r[1] *= 1;
			r[2] *= 1;
			r[4] = r[4].replace(/:/g, '-');

			fog.insertData(-1, r.slice(1, 8));
		}
	}
	fog.sort(6);
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
	f[6].value = '';
	ferror.clearAll(fields.getAll(this.newEditor));
}

fog.dataToView = function(data) {
	return [(data[0] != '0' ? '&#x2b50' : ''),['TCP','UDP','Both'][data[1] - 1],(data[2].match(/(.+)-(.+)/) ? (RegExp.$1+' -<br>'+RegExp.$2) : data[2]),data[3],data[4],data[5],data[6]];
}

fog.fieldValuesToData = function(row) {
	var f = fields.getAll(row);

	return [(f[0].checked ? 1 : 0),f[1].value,f[2].value,f[3].value,f[4].value,f[5].value,f[6].value];
}

fog.sortCompare = function(a, b) {
	var col = this.sortColumn;
	var da = a.getRowData();
	var db = b.getRowData();
	var r;

	switch (col) {
		case 0:
		case 1:
		case 3:
		case 4:
			r = cmpInt(da[col], db[col]);
		break;
		case 5:
			r = cmpIP(da[col], db[col]);
		break;
		default:
			r = cmpText(da[col], db[col]);
		break;
	}

	return this.sortAscending ? r : -r;
}

fog.verifyFields = function(row, quiet) {
	var f = fields.getAll(row);
	var s;

	f[2].value = f[2].value.trim();
	ferror.clear(f[2]);
	if ((f[2].value.length) && (!v_iptaddr(f[2], quiet)))
		return 0;

	if (!v_iptport(f[3], quiet))
		return 0;

	ferror.clear(f[4]);
	if (f[3].value.search(/[-:,]/) != -1) {
		f[4].value = '';
		f[4].disabled = true;
	}
	else {
		f[4].disabled = false;
		f[4].value = f[4].value.trim();
		if (f[4].value != '') {
			if (!v_port(f[4], quiet))
				return 0;
		}
	}
	ferror.clear(f[4]);

	s = f[5].value.trim();
	if (!v_ip(f[5], quiet, 1))
		return 0;

	f[6].value = f[6].value.replace(/>/g, '_');
	if (!v_nodelim(f[6], quiet, 'Description'))
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
		data[i][3] = data[i][3].replace(/-/g, ':');
		s += data[i].join('<')+'>';
	}

	var fom = E('t_fom');
	fom.portforward.value = s;

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

<input type="hidden" name="_nextpage" value="forward-basic.asp">
<input type="hidden" name="_service" value="firewall-restart">
<input type="hidden" name="portforward">

<!-- / / / -->

<div class="section-title">Port Forwarding</div>
<div class="section">
	<div class="tomato-grid" id="fo-grid"></div>
</div>

<!-- / / / -->

<div class="section-title">Notes</div>
<div class="section">
	<ul>
		<li><b>Src Address</b> <i>(optional)</i> - Forward only if from this address. Ex: "1.2.3.4", "1.2.3.4-2.3.4.5", "1.2.3.0/24", "me.example.com".</li>
		<li><b>Ext Ports</b> - The ports to be forwarded, as seen from the WAN. Ex: "2345", "200,300", "200-300,400".</li>
		<li><b>Int Port</b> <i>(optional)</i> - The destination port inside the LAN. If blank, the destination port is the same as <i>Ext Ports</i>. Only one port per entry is supported when forwarding to a different internal port.</li>
		<li><b>Int Address</b> - The destination address inside the LAN.</li>
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
