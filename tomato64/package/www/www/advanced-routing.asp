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
<title>[<% ident(); %>] Advanced: Routing</title>
<link rel="stylesheet" type="text/css" href="tomato.css">
<% css(); %>
<script src="tomato.js"></script>

<script>

//	<% nvram("routes_static,dhcp_routes,lan_ifname,lan1_ifname,lan2_ifname,lan3_ifname,wan_ifname,wan_iface,wan2_ifname,wan2_iface,wan3_ifname,wan3_iface,wan4_ifname,wan4_iface"); %>

//	<% activeroutes(); %>

var static_options = [['LAN','LAN0'],['LAN1','LAN1'],['LAN2','LAN2'],['LAN3','LAN3'],['WAN','WAN0'],['MAN','MAN0'],['WAN2','WAN1'],['MAN2','MAN1']
/* MULTIWAN-BEGIN */
                      ,['WAN3','WAN2'],['MAN3','MAN2'],['WAN4','WAN3'],['MAN4','MAN3']
/* MULTIWAN-END */
		     ];

var ara = new TomatoGrid();

ara.setup = function() {
	var i, a;

	this.init('ara-grid', 'sort');
	this.headerSet(['Destination', 'Gateway / Next Hop', 'Subnet Mask', 'Metric', 'Interface']);
	for (i = 0; i < activeroutes.length; ++i) {
		a = activeroutes[i];
		if (a[0] == nvram.lan_ifname)
			a[0] += ' (LAN0)';
		else if (a[0] == nvram.lan1_ifname)
			a[0] += ' (LAN1)';
		else if (a[0] == nvram.lan2_ifname)
			a[0] += ' (LAN2)';
		else if (a[0] == nvram.lan3_ifname)
			a[0] += ' (LAN3)';
		else if (a[0] == nvram.wan_iface)
			a[0] += ' (WAN0)';
		else if (a[0] == nvram.wan_ifname)
			a[0] += ' (MAN0)';
		else if (a[0] == nvram.wan2_iface)
			a[0] += ' (WAN1)';
		else if (a[0] == nvram.wan2_ifname)
			a[0] += ' (MAN1)';
/* MULTIWAN-BEGIN */
		else if (a[0] == nvram.wan3_iface)
			a[0] += ' (WAN2)';
		else if (a[0] == nvram.wan3_ifname)
			a[0] += ' (MAN2)';
		else if (a[0] == nvram.wan4_iface)
			a[0] += ' (WAN3)';
		else if (a[0] == nvram.wan4_ifname)
			a[0] += ' (MAN3)';
/* MULTIWAN-END */
		this.insertData(-1, [a[1],a[2],a[3],a[4],a[0]]);
	}
}

var ars = new TomatoGrid();

ars.setup = function() {
	this.init('ars-grid', '', 20, [
		{ type: 'text', maxlen: 15 }, { type: 'text', maxlen: 15 }, { type: 'text', maxlen: 15 },{ type: 'text', maxlen: 10 },
		{ type: 'select', options: static_options }, { type: 'text', maxlen: 32 }]);

	this.headerSet(['Destination', 'Gateway', 'Subnet Mask', 'Metric', 'Interface', 'Description']);
	var routes = nvram.routes_static.split('>');
	for (var i = 0; i < routes.length; ++i) {
		var r;
		if (r = routes[i].match(/^(.+)<(.+)<(.+)<(\d+)<(LAN|LAN1|LAN2|LAN3|WAN|MAN|WAN2|MAN2|WAN3|MAN3|WAN4|MAN4)<(.*)$/)) {
			this.insertData(-1, [r[1],r[2],r[3],r[4],r[5],r[6]]);
		}
	}
	this.showNewEditor();
	this.resetNewEditor();
}

ars.resetNewEditor = function() {
	var i, e;

	e = fields.getAll(this.newEditor);

	if (nvram.lan_ifname.length < 1)
		e[4].options[0].disabled = 1;
	else
		e[4].options[0].disabled = 0;

	if (nvram.lan1_ifname.length < 1)
		e[4].options[1].disabled = 1;
	else
		e[4].options[1].disabled = 0;

	if (nvram.lan2_ifname.length < 1)
		e[4].options[2].disabled = 1;
	else
		e[4].options[2].disabled = 0;

	if (nvram.lan3_ifname.length < 1)
		e[4].options[3].disabled = 1;
	else
		e[4].options[3].disabled = 0;

	ferror.clearAll(e);
	for (i = 0; i < e.length; ++i) {
		var f = e[i];
		if (f.selectedIndex)
			f.selectedIndex = 0;
		else
			f.value = '';
	}
	try {
		if (e.length)
			e[0].focus();
	}
	catch (er) { }
}

ars.dataToView = function(data) {
	return [data[0],data[1],data[2],data[3],fix_iface(data[4]),escapeHTML(''+data[5])];
}

ars.verifyFields = function(row, quiet) {
	var f = fields.getAll(row);
	f[5].value = f[5].value.replace('>', '_');

	return v_ip(f[0], quiet) && v_ip(f[1], quiet) && v_netmask(f[2], quiet) && v_range(f[3], quiet, 0, 4294967295) && v_nodelim(f[5], quiet, 'Description');
}

function fix_iface(in_if) {
	for (var i = 0; i < static_options.length; ++i) {
		if (static_options[i][0] == in_if)
			return static_options[i][1];
	}
	return in_if;
}

function submit_complete() {
	reloadPage();
}

function save() {
	if (ars.isEditing())
		return;

	var fom = E('t_fom');
	var data = ars.getAllData();
	var r = [];
	for (var i = 0; i < data.length; ++i)
		r.push(data[i].join('<'));

	fom.routes_static.value = r.join('>');
	fom.dhcp_routes.value = E('_f_dhcp_routes').checked ? '1' : '0';
	fom._service.value = (fom.dhcp_routes.value != nvram.dhcp_routes) ? 'wan-restart' : 'routing-restart';

	form.submit(fom, 1);
}

function earlyInit() {
	ara.setup();
	ars.setup();
}

function init() {
	ara.recolor();
	ars.recolor();
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

<input type="hidden" name="_nextpage" value="advanced-routing.asp">
<input type="hidden" name="_service" value="routing-restart">
<input type="hidden" name="routes_static">
<input type="hidden" name="dhcp_routes">

<!-- / / / -->

<div class="section-title">Current Routing Table</div>
<div class="section">
	<div class="tomato-grid" id="ara-grid"></div>
</div>

<!-- / / / -->

<div class="section-title">Static Routing Table</div>
<div class="section">
	<div class="tomato-grid" id="ars-grid"></div>
</div>

<!-- / / / -->

<div class="section-title">Miscellaneous</div>
<div class="section">
	<script>
		createFieldTable('', [
			{ title: 'Accept DHCP Routes<br>on WAN interface', name: 'f_dhcp_routes', type: 'checkbox', value: nvram.dhcp_routes != '0' }
		]);
	</script>
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
