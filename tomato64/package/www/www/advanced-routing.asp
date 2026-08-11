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
<link rel="stylesheet" type="text/css" href="tomato.css?rel=<% version(); %>">
<% css(); %>
<script src="tomato.js?rel=<% version(); %>"></script>
<script src="grid-backup.js?rel=<% version(); %>"></script>

<script>

//	<% nvram("routes_static,dhcpc_33,dhcpc_121,lan_ifname,wan_ifname,wan_iface,dr_lan_rx,dr_wan_rx,wan_proto,mwan_num,t_model_name,os_version"); %>

//	<% activeroutes(); %>

var static_options = [];
var static_ifaces = [];

for (var i = 0; i <= MAX_BRIDGE_ID; ++i) {
	var p = i ? i : '';
	static_options.push(['LAN'+p, 'LAN'+i]);
	static_ifaces.push('LAN'+p);
}

for (var i = 1; i <= MAXWAN_NUM; ++i) {
	var p = (i > 1) ? i : '';
	static_options.push(['WAN'+p, 'WAN'+(i - 1)]);
	static_options.push(['MAN'+p, 'MAN'+(i - 1)]);
	static_ifaces.push('WAN'+p);
	static_ifaces.push('MAN'+p);
}

var static_route_re = new RegExp(
	'^(.+)<(.+)<(.+)<(\\d+)<(' + static_ifaces.join('|') + ')<(.*)$'
);

function label_iface(ifname) {
	var i, p;

	for (i = 0; i <= MAX_BRIDGE_ID; ++i) {
		p = 'lan'+(i ? i : '');
		if (ifname == nvram[p+'_ifname'])
			return ifname+' (LAN'+i+')';
	}

	for (i = 1; i <= MAXWAN_NUM; ++i) {
		p = 'wan'+(i > 1 ? i : '');
		if (ifname == nvram[p+'_iface'])
			return ifname+' (WAN'+(i - 1)+')';
		if (ifname == nvram[p+'_ifname'])
			return ifname+' (MAN'+(i - 1)+')';
	}

	return ifname;
}

var ara = new TomatoGrid();

ara.setup = function() {
	var i, a;

	this.init('ara-grid', 'sort');
	this.headerSet(['Destination','Gateway / Next Hop','Subnet Mask','Metric','Interface']);
	for (i = 0; i < activeroutes.length; ++i) {
		a = activeroutes[i];
		a[0] = label_iface(a[0]);
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
		if (r = routes[i].match(static_route_re)) {
			this.insertData(-1, [r[1],r[2],r[3],r[4],r[5],r[6]]);
		}
	}
	this.showNewEditor();
	this.resetNewEditor();
}

ars.resetNewEditor = function() {
	var i, p, e;

	e = fields.getAll(this.newEditor);

	for (i = 0; i <= MAX_BRIDGE_ID; ++i) {
		p = 'lan'+(i ? i : '');
		e[4].options[i].disabled = (nvram[p+'_ifname'].length < 1);
	}

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

function verifyFields(focused, quiet) {
/* ZEBRA-BEGIN */
	var i, p, u, uidx, field;

	for (i = 0; i <= MAX_BRIDGE_ID; ++i) {
		p = i ? i : '';
		field = E('_f_dr_lan'+p);
		field.disabled = (nvram['lan'+p+'_ifname'].length < 1);
		if (field.disabled)
			field.checked = false;
	}

	for (uidx = 1; uidx <= MAXWAN_NUM; ++uidx) {
		u = (uidx > 1) ? uidx : '';
		field = E('_f_dr_wan'+u);
		field.disabled = (uidx > nvram.mwan_num) ||
			(nvram['wan'+u+'_proto'] == 'disabled');
		if (field.disabled)
			field.checked = false;
	}
/* ZEBRA-END */
	return 1;
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

/* ZEBRA-BEGIN */
	for (var i = 0; i <= MAX_BRIDGE_ID; ++i) {
		var p = i ? i : '';
		fom['dr_lan'+p+'_tx'].value = fom['dr_lan'+p+'_rx'].value =
			E('_f_dr_lan'+p).checked ? '1 2' : '0';
	}

	for (var uidx = 1; uidx <= MAXWAN_NUM; ++uidx) {
		var u = (uidx > 1) ? uidx : '';
		fom['dr_wan'+u+'_tx'].value = fom['dr_wan'+u+'_rx'].value =
			E('_f_dr_wan'+u).checked ? '1 2' : '0';
	}
/* ZEBRA-END */

	form.submit(fom, 1);
}

function earlyInit() {
	ara.setup();
	ars.setup();
	insOvl();
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
	<div class="title"><a href="/">Tomato64</a></div>
	<div class="version">Version <% version(); %> on <% nv("t_model_name"); %><span class="blinking bl2"><script><% anonupdate(); %> anon_update()</script>&nbsp;</span></div>
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
<!-- ZEBRA-BEGIN -->
<script>
for (var i = 0; i <= MAX_BRIDGE_ID; ++i) {
	var p = i ? i : '';
	W('<input type="hidden" name="dr_lan'+p+'_tx">');
	W('<input type="hidden" name="dr_lan'+p+'_rx">');
}

for (var uidx = 1; uidx <= MAXWAN_NUM; ++uidx) {
	var u = (uidx > 1) ? uidx : '';
	W('<input type="hidden" name="dr_wan'+u+'_tx">');
	W('<input type="hidden" name="dr_wan'+u+'_rx">');
}
</script>
<!-- ZEBRA-END -->

<!-- / / / -->

<div class="section-title">Current Routing Table</div>
<div class="section">
	<div class="tomato-grid" id="ara-grid"></div>
</div>

<!-- / / / -->

<div class="section-title">Static Routing Table</div>
<div class="section">
	<div class="tomato-grid" id="ars-grid"></div>
	<input type="button" value="Backup" id="backup-button" onclick="backupGrid()">
	<input type="button" value="Restore" id="restore-button" onclick="restoreGrid()">
	<input type="button" value="Clear Table" id="clear-button" onclick="clearGrid()">
</div>

<!-- / / / -->

<div class="section-title">WAN Miscellaneous</div>
<div class="section">
	<script>
		var routing_fields = [];
/* ZEBRA-BEGIN */
		routing_fields.push({ title: 'RIPv1 &amp; v2' });

		for (var i = 0; i <= MAX_BRIDGE_ID; ++i) {
			var p = i ? i : '';
			routing_fields.push({
				title: 'LAN'+p, indent: 2,
				name: 'f_dr_lan'+p, type: 'checkbox',
				value: ((nvram['dr_lan'+p+'_rx'] != '0') && (nvram['dr_lan'+p+'_rx'] != ''))
			});
		}

		for (var uidx = 1; uidx <= MAXWAN_NUM; ++uidx) {
			var u = (uidx > 1) ? uidx : '';
			routing_fields.push({
				title: 'WAN'+u, indent: 2,
				name: 'f_dr_wan'+u, type: 'checkbox',
				value: ((nvram['dr_wan'+u+'_rx'] != '0') && (nvram['dr_wan'+u+'_rx'] != ''))
			});
		}
/* ZEBRA-END */
		routing_fields.push(
			{ title: 'Accept DHCP Static Route<br>(option 33)', name: 'f_dhcpc_33', type: 'checkbox', value: nvram.dhcpc_33 != 0 },
			{ title: 'Accept DHCP Classless Routes<br>(option 121)', name: 'f_dhcpc_121', type: 'checkbox', value: nvram.dhcpc_121 != 0 }
		);

		createFieldTable('', routing_fields);
	</script>
</div>

<!-- / / / -->

<script>writeFooter();</script>

</td></tr>
</table>
</form>
<script>earlyInit();</script>
</body>
</html>
