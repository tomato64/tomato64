<!DOCTYPE html>
<!--
	Tomato GUI
	Copyright (C) 2006-2007 Jonathan Zarate
	http://www.polarcloud.com/tomato/
	For use with Tomato Firmware only.
	No part of this file may be used without permission.
	LAN Access admin module by Augusto Bott
-->
<html lang="en-GB">
<head>
<meta http-equiv="content-type" content="text/html;charset=utf-8">
<meta name="robots" content="noindex,nofollow">
<title>[<% ident(); %>] Advanced: LAN Access</title>
<link rel="stylesheet" type="text/css" href="tomato.css?rel=<% version(); %>">
<% css(); %>
<script src="tomato.js?rel=<% version(); %>"></script>
<script src="wireless.jsx?_http_id=<% nv(http_id); %>"></script>

<script>

//	<% nvram ("lan_ifname,lan_access");%>

var cprefix = 'advanced_access';

var la = new TomatoGrid();
la.setup = function() {
/* TOMATO64-REMOVE-BEGIN */
	this.init('la-grid', 'sort', 50, [
/* TOMATO64-REMOVE-END */
/* TOMATO64-BEGIN */
	this.init('la-grid', 'sort', 200, [
/* TOMATO64-END */
	{ type: 'checkbox', prefix: '<div class="centered">', suffix: '<\/div>' },
/* TOMATO64-REMOVE-BEGIN */
	{ type: 'select', options: [[0,'LAN0 (br0)'],[1,'LAN1 (br1)'],[2,'LAN2 (br2)'],[3,'LAN3 (br3)']], prefix: '<div class="centered">', suffix: '<\/div>' },
/* TOMATO64-REMOVE-END */
/* TOMATO64-BEGIN */
	{ type: 'select', options: [[0,'LAN0 (br0)'],[1,'LAN1 (br1)'],[2,'LAN2 (br2)'],[3,'LAN3 (br3)'],[4,'LAN4 (br4)'],[5,'LAN5 (br5)'],[6,'LAN6 (br6)'],[7,'LAN7 (br7)']], prefix: '<div class="centered">', suffix: '<\/div>' },
/* TOMATO64-END */
	{ type: 'text', maxlen: 32 },
/* TOMATO64-REMOVE-BEGIN */
	{ type: 'select', options: [[0,'LAN0 (br0)'],[1,'LAN1 (br1)'],[2,'LAN2 (br2)'],[3,'LAN3 (br3)']], prefix: '<div class="centered">', suffix: '<\/div>' },
/* TOMATO64-REMOVE-END */
/* TOMATO64-BEGIN */
	{ type: 'select', options: [[0,'LAN0 (br0)'],[1,'LAN1 (br1)'],[2,'LAN2 (br2)'],[3,'LAN3 (br3)'],[4,'LAN4 (br4)'],[5,'LAN5 (br5)'],[6,'LAN6 (br6)'],[7,'LAN7 (br7)']], prefix: '<div class="centered">', suffix: '<\/div>' },
/* TOMATO64-END */
	{ type: 'text', maxlen: 32 },
	{ type: 'text', maxlen: 32 }]);
	this.headerSet(['On','Src','Src Address','Dst','Dst Address','Description']);

	var r = nvram.lan_access.split('>');
	for (var i = 0; i < r.length; ++i) {
		if (r[i].length) {
			var l = r[i].split('<');
			l[0] *= 1;
			l[1] *= 1;
			l[3] *= 1;
			la.insertData(-1, [ l[0],l[1],l[2],l[3],l[4],l[5] ] );
		}
	}

	la.recolor();
	la.showNewEditor();
	la.resetNewEditor();
}

la.sortCompare = function(a, b) {
	var col = this.sortColumn;
	var da = a.getRowData();
	var db = b.getRowData();
	var r;

	switch (col) {
		case 2: /* src */
		case 4: /* dst */
			r = cmpIP(da[col], db[col]);
		break;
		case 0: /* on */
		case 1: /* src br */
		case 3: /* dst br */
			r = cmpInt(da[col], db[col]);
		break;
		default:
			r = cmpText(da[col], db[col]);
		break;
	}

	return this.sortAscending ? r : -r;
}

la.resetNewEditor = function() {
	var f = fields.getAll(this.newEditor);
	f[0].checked=true;
	f[2].value='';
	f[4].value='';
	f[5].value='';
	var total=0;
	for (var i = 0; i <= MAX_BRIDGE_ID; i++) {
		var j = (i == 0) ? '' : i.toString();
		if (nvram['lan'+j+'_ifname'].length < 1) {
			f[1].options[i].disabled = 1;
			f[3].options[i].disabled = 1;
		} else
			++total;
	}
	if ((f[1].selectedIndex == f[3].selectedIndex) && (total > 1)) {
		while (f[1].selectedIndex == f[3].selectedIndex)
			f[3].selectedIndex = (f[3].selectedIndex%(MAX_BRIDGE_ID + 1) + 1);
	}
	ferror.clearAll(fields.getAll(this.newEditor));
}

la.verifyFields = function(row, quiet) {
	var f = fields.getAll(row);

	for (var i = 0; i <= MAX_BRIDGE_ID; i++) {
		var j = (i == 0) ? '' : i.toString();
		if (nvram['lan'+j+'_ifname'].length < 1) {
			f[1].options[i].disabled = 1;
			f[3].options[i].disabled = 1;
		}
	}

	if (f[1].selectedIndex == f[3].selectedIndex) {
		var m = 'Source and Destination interfaces must be different';
		ferror.set(f[1], m, quiet);
		ferror.set(f[3], m, quiet);
		return 0;
	}
	ferror.clear(f[1]);
	ferror.clear(f[3]);

	f[2].value = f[2].value.trim();
	f[4].value = f[4].value.trim();

	if ((f[2].value.length) && (!v_iptaddr(f[2], quiet))) return 0;
	if ((f[4].value.length) && (!v_iptaddr(f[4], quiet))) return 0;

	ferror.clear(f[2]);
	ferror.clear(f[4]);

	f[5].value = f[5].value.replace(/>/g, '_');
	if (!v_nodelim(f[5], quiet, 'Description')) return 0;

	return 1;
}

la.dataToView = function(data) {
/* TOMATO64-REMOVE-BEGIN */
	return [(data[0] != 0) ? '&#x2b50' : '', ['LAN0','LAN1','LAN2','LAN3'][data[1]],data[2],['LAN0','LAN1','LAN2','LAN3'][data[3]],data[4],data[5] ];
/* TOMATO64-REMOVE-END */
/* TOMATO64-BEGIN */
	return [(data[0] != 0) ? '&#x2b50' : '', ['LAN0','LAN1','LAN2','LAN3','LAN4','LAN5','LAN6','LAN7'][data[1]],data[2],['LAN0','LAN1','LAN2','LAN3','LAN4','LAN5','LAN6','LAN7'][data[3]],data[4],data[5] ];
/* TOMATO64-END */
}

la.dataToFieldValues = function (data) {
	return [(data[0] != 0) ? 'checked' : '',data[1],data[2],data[3],data[4],data[5] ];
}

la.fieldValuesToData = function(row) {
	var f = fields.getAll(row);
	return [f[0].checked ? 1 : 0,f[1].selectedIndex,f[2].value,f[3].selectedIndex,f[4].value,f[5].value ];
}

function save() {
	if (la.isEditing()) return;
	la.resetNewEditor();

	var fom = E('t_fom');
	var ladata = la.getAllData();

	var s = '';
	for (var i = 0; i < ladata.length; ++i) {
		s += ladata[i].join('<')+'>';
	}
	fom.lan_access.value = s;

	form.submit(fom, 1);
}

function init() {
	la.setup();
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
	<div class="title"><a href="/">Tomato64</a></div>
	<div class="version">Version <% version(); %> on <% nv("t_model_name"); %></div>
</td></tr>
<tr id="body"><td id="navi"><script>navi()</script></td>
<td id="content">
<div id="ident"><% ident(); %> | <script>wikiLink();</script></div>

<!-- / / / -->

<input type="hidden" name="_nextpage" value="advanced-access.asp">
<input type="hidden" name="_service" value="firewall-restart">
<input type="hidden" name="lan_access">

<!-- / / / -->

<div class="section-title">LAN Access</div>
<div class="section">
	<div class="tomato-grid" id="la-grid"></div>
</div>

<!-- / / / -->

<div class="section-title">Notes <small><i><a href="javascript:toggleVisibility(cprefix,'notes');" id="toggleLink-notes"><span id="sesdiv_notes_showhide">(Show)</span></a></i></small></div>
<div class="section" id="sesdiv_notes" style="display:none">
	<ul>
		<li><b>Src</b> - Source LAN bridge.</li>
		<li><b>Src Address</b> <i>(optional)</i> - Source address allowed. Ex: "1.2.3.4", "1.2.3.4-2.3.4.5", "1.2.3.0/24".</li>
		<li><b>Dst</b> - Destination LAN bridge.</li>
		<li><b>Dst Address</b> <i>(optional)</i> - Destination address inside the LAN.</li>
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
</body>
</html>
