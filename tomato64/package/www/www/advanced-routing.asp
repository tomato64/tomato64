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

/* TOMATO64-REMOVE-BEGIN */
//	<% nvram("routes_static,dhcpc_33,dhcpc_121,lan_ifname,lan1_ifname,lan2_ifname,lan3_ifname,wan_ifname,wan_iface,wan2_ifname,wan2_iface,wan3_ifname,wan3_iface,wan4_ifname,wan4_iface"); %>
/* TOMATO64-REMOVE-END */
/* TOMATO64-BEGIN */
//	<% nvram("routes_static,dhcpc_33,dhcpc_121,lan_ifname,lan1_ifname,lan2_ifname,lan3_ifname,lan4_ifname,lan5_ifname,lan6_ifname,lan7_ifname,wan_ifname,wan_iface,wan2_ifname,wan2_iface,wan3_ifname,wan3_iface,wan4_ifname,wan4_iface"); %>
/* TOMATO64-END */

//	<% activeroutes(); %>

/* TOMATO64-REMOVE-BEGIN */
var static_options = [['LAN','LAN0'],['LAN1','LAN1'],['LAN2','LAN2'],['LAN3','LAN3'],['WAN','WAN0'],['MAN','MAN0'],['WAN2','WAN1'],['MAN2','MAN1']
/* TOMATO64-REMOVE-END */
/* TOMATO64-BEGIN */
var static_options = [['LAN','LAN0'],['LAN1','LAN1'],['LAN2','LAN2'],['LAN3','LAN3'],['LAN4','LAN4'],['LAN5','LAN5'],['LAN6','LAN6'],['LAN7','LAN7'],['WAN','WAN0'],['MAN','MAN0'],['WAN2','WAN1'],['MAN2','MAN1']
/* TOMATO64-END */
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
/* TOMATO64-BEGIN */
		else if (a[0] == nvram.lan4_ifname)
			a[0] += ' (LAN4)';
		else if (a[0] == nvram.lan5_ifname)
			a[0] += ' (LAN5)';
		else if (a[0] == nvram.lan6_ifname)
			a[0] += ' (LAN6)';
		else if (a[0] == nvram.lan7_ifname)
			a[0] += ' (LAN7)';
/* TOMATO64-END */
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
/* TOMATO64-REMOVE-BEGIN */
		if (r = routes[i].match(/^(.+)<(.+)<(.+)<(\d+)<(LAN|LAN1|LAN2|LAN3|WAN|MAN|WAN2|MAN2|WAN3|MAN3|WAN4|MAN4)<(.*)$/)) {
/* TOMATO64-REMOVE-END */
/* TOMATO64-BEGIN */
		if (r = routes[i].match(/^(.+)<(.+)<(.+)<(\d+)<(LAN|LAN1|LAN2|LAN3|LAN4|LAN5|LAN6|LAN7|WAN|MAN|WAN2|MAN2|WAN3|MAN3|WAN4|MAN4)<(.*)$/)) {
/* TOMATO64-END */
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
/* TOMATO64-BEGIN */
	if (nvram.lan4_ifname.length < 1)
		e[4].options[4].disabled = 1;
	else
		e[4].options[4].disabled = 0;

	if (nvram.lan5_ifname.length < 1)
		e[4].options[5].disabled = 1;
	else
		e[4].options[5].disabled = 0;

	if (nvram.lan6_ifname.length < 1)
		e[4].options[6].disabled = 1;
	else
		e[4].options[6].disabled = 0;

	if (nvram.lan7_ifname.length < 1)
		e[4].options[7].disabled = 1;
	else
		e[4].options[7].disabled = 0;
/* TOMATO64-END */

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

	if (f[0].value == 'default')
		f[0].value = '0.0.0.0';

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
	fom.dhcpc_33.value = E('_f_dhcpc_33').checked ? '1' : '0';
	fom.dhcpc_121.value = E('_f_dhcpc_121').checked ? '1' : '0';
	fom._service.value = ((fom.dhcpc_33.value != nvram.dhcpc_33) || (fom.dhcpc_121.value != nvram.dhcpc_121)) ? 'wan-restart' : 'routing-restart';

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
	<div class="title">Tomato64</div>
	<div class="version">Version <% version(); %> on <% nv("t_model_name"); %></div>
</td></tr>
<tr id="body"><td id="navi"><script>navi()</script></td>
<td id="content">
<div id="ident"><% ident(); %> | <script>wikiLink();</script></div>

<!-- / / / -->

<input type="hidden" name="_nextpage" value="advanced-routing.asp">
<input type="hidden" name="_service" value="routing-restart">
<input type="hidden" name="routes_static">
<input type="hidden" name="dhcpc_33">
<input type="hidden" name="dhcpc_121">

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
			{ title: 'Accept DHCP Static Route<br>(option 33)', name: 'f_dhcpc_33', type: 'checkbox', value: nvram.dhcpc_33 != 0 },
			{ title: 'Accept DHCP Classless Routes<br>(option 121)', name: 'f_dhcpc_121', type: 'checkbox', value: nvram.dhcpc_121 != 0 }
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
