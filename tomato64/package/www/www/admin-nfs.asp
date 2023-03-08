<!DOCTYPE html>
<!--
	Tomato GUI
	Copyright (C) 2007-2011 Shibby
	http://openlinksys.info
	For use with Tomato Firmware only.
	No part of this file may be used without permission.
-->
<html lang="en-GB">
<head>
<meta http-equiv="content-type" content="text/html;charset=utf-8">
<meta name="robots" content="noindex,nofollow">
<title>[<% ident(); %>] Admin: NFS Server</title>
<link rel="stylesheet" type="text/css" href="tomato.css">
<% css(); %>
<script src="tomato.js"></script>

<script>

//	<% nvram("nfs_enable,nfs_enable_v2,nfs_exports"); %>

var access = [['rw', 'Read/Write'], ['ro', 'Read only']];
var sync = [['sync', 'Yes'], ['async', 'No']];
var subtree = [['subtree_check', 'Yes'], ['no_subtree_check', 'No']];

var nfsg = new TomatoGrid();

nfsg.exist = function(f, v) {
	var data = this.getAllData();
	for (var i = 0; i < data.length; ++i) {
		if (data[i][f] == v) return true;
	}
	return false;
}

nfsg.dataToView = function(data) {
	return [data[0], data[1], data[2],data[3], data[4], data[5]];
}

nfsg.verifyFields = function(row, quiet) {
	var ok = 1;

	return ok;
}

nfsg.resetNewEditor = function() {
	var f;

	f = fields.getAll(this.newEditor);
	ferror.clearAll(f);
	f[0].value = '';
	f[1].value = '';
	f[2].selectedIndex = 0;
	f[3].selectedIndex = 0;
	f[4].selectedIndex = 1;
	f[5].value = 'no_root_squash';
}

nfsg.setup = function() {
	this.init('nfsg-grid', '', 50, [
		{ type: 'text', maxlen: 50 },
		{ type: 'text', maxlen: 30 },
		{ type: 'select', options: access },
		{ type: 'select', options: sync },
		{ type: 'select', options: subtree },
		{ type: 'text', maxlen: 50 }
	]);
	this.headerSet(['Directory', 'IP Address/Subnet', 'Access', 'Sync', 'Subtree Check', 'Other Options']);
	var s = nvram.nfs_exports.split('>');
	for (var i = 0; i < s.length; ++i) {
		var t = s[i].split('<');
		if (t.length == 6) this.insertData(-1, t);
	}
	this.showNewEditor();
	this.resetNewEditor();
}

function verifyFields(focused, quiet) {
	var ok = 1;

	return ok;
}

function save() {
	var data = nfsg.getAllData();
	var exports = '';
	var i;

	if (data.length != 0) exports += data[0].join('<');
	for (i = 1; i < data.length; ++i) {
		exports += '>' + data[i].join('<');
	}

	var fom = E('t_fom');
	fom.nfs_enable.value = E('_f_nfs_enable').checked ? 1 : 0;
	fom.nfs_enable_v2.value = E('_f_nfs_enable_v2').checked ? 1 : 0;
	fom.nfs_exports.value = exports;
	form.submit(fom, 1);
}

function init() {
	nfsg.recolor();
	eventHandler();
}

function earlyInit() {
	nfsg.setup();
	verifyFields(null, true);
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

<input type="hidden" name="_nextpage" value="admin-nfs.asp">
<input type="hidden" name="_service" value="nfs-restart">
<input type="hidden" name="nfs_enable">
<input type="hidden" name="nfs_enable_v2">
<input type="hidden" name="nfs_exports">

<!-- / / / -->

<div class="section-title">NFS Server</div>
<div class="section">
	<script>
		createFieldTable('', [
			{ title: 'Enable NFS Server', name: 'f_nfs_enable', type: 'checkbox', value: nvram.nfs_enable != '0' },
			{ title: 'Enable legacy (NFS V2) support', indent: 2, name: 'f_nfs_enable_v2', type: 'checkbox', value: nvram.nfs_enable_v2 != '0' }
		]);
	</script>
</div>

<!-- / / / -->

<div class="section-title">Exports</div>
<div class="section">
	<div class="tomato-grid" id="nfsg-grid"></div>
	<ul>
		<li>More information on the proper configuration of the NFS <a href="http://nfs.sourceforge.net/nfs-howto/" class="new_window">HERE</a>.</li>
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
