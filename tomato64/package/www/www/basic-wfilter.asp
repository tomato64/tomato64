<!DOCTYPE html>
<!--
	Tomato GUI
	Copyright (C) 2006-2010 Jonathan Zarate
	http://www.polarcloud.com/tomato/

	Tomato VLAN GUI
	Copyright (C) 2011 Augusto Bott
	http://code.google.com/p/tomato-sdhc-vlan/

	For use with Tomato Firmware only.
	No part of this file may be used without permission.
-->
<html lang="en-GB">
<head>
<meta http-equiv="content-type" content="text/html;charset=utf-8">
<meta name="robots" content="noindex,nofollow">
<title>[<% ident(); %>] Basic: Wireless Filter</title>
<link rel="stylesheet" type="text/css" href="tomato.css">
<% css(); %>
<script src="tomato.js"></script>
<script src="wireless.jsx?_http_id=<% nv(http_id); %>"></script>

<script>

//	<% nvram("wl_maclist,macnames"); %>

var smg = new TomatoGrid();

smg.verifyFields = function(row, quiet) {
	var f;
	f = fields.getAll(row);

	return v_mac(f[0], quiet) && v_nodelim(f[1], quiet, 'Description', 1);
}

smg.resetNewEditor = function() {
	var f, c, n;

	f = fields.getAll(this.newEditor);
	ferror.clearAll(f);

	if ((c = cookie.get('addmac')) != null) {
		cookie.set('addmac', '', 0);
		c = c.split(',');
		if (c.length == 2) {
			f[0].value = c[0];
			f[1].value = c[1];
			return;
		}
	}

	f[0].value = mac_null;
	f[1].value = '';
}

smg.setup = function() {
	var i, i, m, s, t, n;
	var macs, names;

	this.init('sm-grid', 'sort', 280, [
		{ type: 'text', maxlen: 17 },
		{ type: 'text', maxlen: 48 }
	]);
	this.headerSet(['MAC Address', 'Description']);
	macs = nvram.wl_maclist.split(/\s+/);
	names = nvram.macnames.split('>');
	for (i = 0; i < macs.length; ++i) {
		m = fixMAC(macs[i]);
		if ((m) && (!isMAC0(m))) {
			s = m.replace(/:/g, '');
			t = '';
			for (var j = 0; j < names.length; ++j) {
				n = names[j].split('<');
				if ((n.length == 2) && (n[0] == s)) {
					t = n[1];
					break;
				}
			}
			this.insertData(-1, [m, t]);
		}
	}
	this.sort(0);
	this.showNewEditor();
	this.resetNewEditor();
}

function save() {
	var fom;
	var d, i, macs, names, ma, na;
	var u;

	if (smg.isEditing()) return;

	fom = E('t_fom');

	macs = [];
	names = [];
	d = smg.getAllData();
	for (i = 0; i < d.length; ++i) {
		ma = d[i][0];
		na = d[i][1].replace(/[<>|]/g, '');

		macs.push(ma);
		if (na.length) {
			names.push(ma.replace(/:/g, '') + '<' + na);
		}
	}
	fom.wl_maclist.value = macs.join(' ');
	fom.macnames.value = names.join('>');

	for (i = 0; i < wl_ifaces.length; ++i) {
		u = wl_fface(i);
		E('t_wl'+u+'_maclist').value = fom.wl_maclist.value;
	}

	form.submit(fom, 1);
}

function earlyInit() {
	smg.setup();
}

function init() {
	smg.recolor();
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

<input type="hidden" name="_nextpage" value="basic-wfilter.asp">
<input type="hidden" name="_nextwait" value="10">
<input type="hidden" name="_service" value="wlgui-restart">
<input type="hidden" name="_force_commit" value="1">
<input type="hidden" name="wl_maclist">
<input type="hidden" name="macnames">

<script>
	for (var uidx = 0; uidx < wl_ifaces.length; ++uidx) {
		var u = wl_fface(uidx);
		W('<input type="hidden" id="t_wl'+u+'_maclist" name="wl'+u+'_maclist">');
	}
</script>

<!-- / / / -->

<div class="section-title">Wireless Client Filter</div>
<div class="section">
	<div class="tomato-grid" id="sm-grid"></div>
</div>

<!-- / / / -->

<div class="section-title">Notes</div>
<div class="section">
	<ul>
		<li>To specify how and on which interface this list should work, use the <a href="advanced-wlanvifs.asp" class="new_window">Virtual Wireless Interfaces</a> page.</li>
		<li>Warning: the filter supports only a certain number of MAC addresses, depending on the WL driver. Above this number, added addresses are not filtered!</li>
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
<script>earlyInit()</script>
</body>
</html>
