<!DOCTYPE html>
<!--
	Tomato GUI
	Copyright (C) 2006-2010 Jonathan Zarate
	http://www.polarcloud.com/tomato/

	Enhancements by Teaman
	Copyright (C) 2011 Augusto Bott
	http://code.google.com/p/tomato-sdhc-vlan/

	For use with Tomato Firmware only.
	No part of this file may be used without permission.
-->
<html lang="en-GB">
<head>
<meta http-equiv="content-type" content="text/html;charset=utf-8">
<meta name="robots" content="noindex,nofollow">
<title>[<% ident(); %>] Basic: DHCP Reservation</title>
<link rel="stylesheet" type="text/css" href="tomato.css">
<% css(); %>
<script src="tomato.js"></script>

<script>

//	<% nvram("lan_ipaddr,lan_netmask,dhcpd_static,cstats_include,dhcpd_slt"); %>

var cprefix = 'basic_static';

autonum = aton(nvram.lan_ipaddr) & aton(nvram.lan_netmask);

var sg = new TomatoGrid();

sg.resetNewEditor = function() {
	var f, c, n;

	f = fields.getAll(this.newEditor);
	ferror.clearAll(f);

	if ((c = cookie.get('addstatic')) != null) {
		cookie.set('addstatic', '', 0);
		c = c.split(',');
		if (c.length == 3) {
			f[0].value = c[0];
			f[1].value = mac_null;
			f[3].value = c[1];
/* TOMATO64-REMOVE-BEGIN */
			f[5].value = c[2];
/* TOMATO64-REMOVE-END */
/* TOMATO64-BEGIN */
			f[4].value = c[2];
/* TOMATO64-END */
			return;
		}
	}

	f[0].value = mac_null;
	f[1].value = mac_null;
	f[2].disabled = 1;
	f[2].checked = 0;
/* TOMATO64-REMOVE-BEGIN */
	f[4].checked = 0;
	f[5].value = '';
/* TOMATO64-REMOVE-END */
/* TOMATO64-BEGIN */
	f[4].value = '';
/* TOMATO64-END */

	n = 10;
	do {
		if (--n < 0) {
			f[3].value = '';
			return;
		}
		autonum++;
	} while (((c = fixIP(ntoa(autonum), 1)) == null) || (c == nvram.lan_ipaddr) || (this.inStatic(c)));

	f[3].value = c;
}

sg.setup = function() {
	this.init('bs-grid', 'sort', 250, [
		{ multi: [ { type: 'text', maxlen: 17 }, { type: 'text', maxlen: 17 } ] },
		{ type: 'checkbox', prefix: '<div class="centered">', suffix: '<\/div>' },
		{ type: 'text', maxlen: 17 },
/* TOMATO64-REMOVE-BEGIN */
		{ type: 'checkbox', prefix: '<div class="centered">', suffix: '<\/div>' },
/* TOMATO64-REMOVE-END */
		{ type: 'text', maxlen: 50 } ] );

/* TOMATO64-REMOVE-BEGIN */
	this.headerSet(['MAC Address', 'Static arp', 'IP Address', 'IPTraffic', 'Hostname']);
/* TOMATO64-REMOVE-END */
/* TOMATO64-BEGIN */
	this.headerSet(['MAC Address', 'Static arp', 'IP Address', 'Hostname']);
/* TOMATO64-END */

	var ipt = nvram.cstats_include.split(',');
	var s = nvram.dhcpd_static.split('>');
	for (var i = 0; i < s.length; ++i) {
/* TOMATO64-REMOVE-BEGIN */
		var h = '0';
/* TOMATO64-REMOVE-END */
		var t = s[i].split('<');
		if (t.length == 4) {
			var d = t[0].split(',');
/* TOMATO64-REMOVE-BEGIN */
			for (var j = 0; j < ipt.length; ++j) {
				if (t[1].length > 0 && t[1] == ipt[j]) {
					h = '1';
					break;
				}
			}
/* TOMATO64-REMOVE-END */

/* TOMATO64-REMOVE-BEGIN */
			this.insertData(-1, [d[0], ((d.length >= 2) ? d[1] : mac_null), t[3], t[1], h, t[2]]);
/* TOMATO64-REMOVE-END */
/* TOMATO64-BEGIN */
			this.insertData(-1, [d[0], ((d.length >= 2) ? d[1] : mac_null), t[3], t[1], t[2]]);
/* TOMATO64-END */
		}
	}
	this.sort(2);
	this.showNewEditor();
	this.resetNewEditor();
}

sg.exist = function(f, v) {
	var data = this.getAllData();
	for (var i = 0; i < data.length; ++i) {
		if (data[i][f] == v)
			return true;
	}
	return false;
}

sg.existMAC = function(mac) {
	if (isMAC0(mac))
		return false;

	return this.exist(0, mac) || this.exist(1, mac);
}

sg.existName = function(name) {
/* TOMATO64-REMOVE-BEGIN */
	return this.exist(5, name);
/* TOMATO64-REMOVE-END */
/* TOMATO64-BEGIN */
	return this.exist(4, name);
/* TOMATO64-END */
}

sg.inStatic = function(n) {
	return this.exist(3, n);
}

sg.dataToView = function(data) {
	var v = [];
	var s = (data[0] == mac_null) ? '' : data[0];
	if (!isMAC0(data[1]))
		s += '<br>'+data[1];

	v.push((s == '') ? '<center><small><i>(unset)<\/i><\/small><\/center>' : s);
	v.push((data[2].toString() != '0') ? '<small><i>Enabled<\/i><\/small>' : '');
	v.push(escapeHTML(''+data[3]));
/* TOMATO64-REMOVE-BEGIN */
	v.push((data[4].toString() != '0') ? '<small><i>Enabled<\/i><\/small>' : '');
	v.push(escapeHTML(''+data[5]));
/* TOMATO64-REMOVE-END */
/* TOMATO64-BEGIN */
	v.push(escapeHTML(''+data[4]));
/* TOMATO64-END */

	return v;
}

sg.dataToFieldValues = function (data) {
/* TOMATO64-REMOVE-BEGIN */
	return ([data[0],data[1],(data[2].toString() != '0') ? 'checked' : '',data[3],(data[4].toString() != '0') ? 'checked' : '',data[5]]);
/* TOMATO64-REMOVE-END */
/* TOMATO64-BEGIN */
	return ([data[0],data[1],(data[2].toString() != '0') ? 'checked' : '',data[3], data[4]]);
/* TOMATO64-END */
}

sg.fieldValuesToData = function(row) {
	var f = fields.getAll(row);

/* TOMATO64-REMOVE-BEGIN */
	return ([f[0].value,f[1].value,f[2].checked ? '1' : '0',f[3].value,f[4].checked ? '1' : '0',f[5].value]);
/* TOMATO64-REMOVE-END */
/* TOMATO64-BEGIN */
	return ([f[0].value,f[1].value,f[2].checked ? '1' : '0',f[3].value,f[4].value]);
/* TOMATO64-END */
}

sg.sortCompare = function(a, b) {
	var da = a.getRowData();
	var db = b.getRowData();
	var r = 0;
	switch (this.sortColumn) {
		case 0:
			r = cmpText(da[0], db[0]);
		break;
		case 1:
			r = cmpInt(da[2], db[2]);
		break;
		case 2:
			r = cmpIP(da[3], db[3]);
		break;
/* TOMATO64-REMOVE-BEGIN */
		case 3:
			r = cmpInt(da[4], db[4]);
/* TOMATO64-REMOVE-END */
		break;
	}
	if (r == 0)
/* TOMATO64-REMOVE-BEGIN */
		r = cmpText(da[5], db[5]);
/* TOMATO64-REMOVE-END */
/* TOMATO64-BEGIN */
		r = cmpText(da[4], db[4]);
/* TOMATO64-END */

	return this.sortAscending ? r : -r;
}

sg.verifyFields = function(row, quiet) {
	var f, s, i;

	f = fields.getAll(row);

	if (!v_macz(f[0], quiet))
		return 0;
	if (!v_macz(f[1], quiet))
		return 0;

	if (isMAC0(f[0].value)) {
		f[0].value = f[1].value;
		f[1].value = mac_null;
	}
	else if (f[0].value == f[1].value)
		f[1].value = mac_null;
	else if ((!isMAC0(f[1].value)) && (f[0].value > f[1].value)) {
		s = f[1].value;
		f[1].value = f[0].value;
		f[0].value = s;
	}

	f[1].disabled = f[2].checked;

	for (i = 0; i < 2; ++i) {
		if (this.existMAC(f[i].value)) {
			ferror.set(f[i], 'Duplicate MAC address', quiet);
			return 0;
		}
	}

	if (f[3].value != '') {
		if (!v_ip(f[3], 1)) {
			ferror.set(f[3], 'Invalid IP address', quiet);
			return 0;
		}

		/* 0 and 255 in last octet is not allowed */
		if ((i = f[3].value.lastIndexOf('.')) != -1) {
			s = parseInt(f[3].value.substr(i + 1), 10);
			if ((s <= 0) || (s >= 255)) {
				ferror.set(f[3], 'Invalid IP address', quiet);
				return 0;
			}
		}

		if (this.inStatic(f[3].value)) {
			ferror.set(f[3], 'Duplicate IP address', quiet);
			return 0;
		}
	}

/* REMOVE-BEGIN
	if (!v_hostname(f[5], quiet, 5))
		return 0;
REMOVE-END */

/* TOMATO64-REMOVE-BEGIN */
	s = f[5].value.trim().replace(/\s+/g, ' ');
/* TOMATO64-REMOVE-END */
/* TOMATO64-BEGIN */
	s = f[4].value.trim().replace(/\s+/g, ' ');
/* TOMATO64-END */

	if (s.length > 0) {
		if (f[3].value == '') {
			if (s.search(/^[a-zA-Z0-9_\-]+$/) == -1) {
/* TOMATO64-REMOVE-BEGIN */
				ferror.set(f[5], 'Invalid hostname. Only a single hostname containing the characters "A-Z 0-9 - _" is allowed', quiet);
/* TOMATO64-REMOVE-END */
/* TOMATO64-BEGIN */
				ferror.set(f[4], 'Invalid hostname. Only a single hostname containing the characters "A-Z 0-9 - _" is allowed', quiet);
/* TOMATO64-END */
				return 0;
			}
		}
		else {
			if (s.indexOf('.') != -1) {
				if (s.search(/^[a-zA-Z0-9_\-\.]+$/) == -1) {
					ferror.set(f[5], 'Invalid hostname. Only a single hostname containing the characters "A-Z 0-9 - _ ." is allowed', quiet);
					return 0;
				}
				if (!quiet) {
					if (!confirm('Hostname contains dot(s). Are you sure to continue?'))
						return 0;
				}
			}
			else {
				if (s.search(/^[a-zA-Z0-9_\- ]+$/) == -1) {
/* TOMATO64-REMOVE-BEGIN */
					ferror.set(f[5], 'Invalid hostname. Only characters "A-Z 0-9 - _" are allowed', quiet);
/* TOMATO64-REMOVE-END */
/* TOMATO64-BEGIN */
					ferror.set(f[4], 'Invalid hostname. Only characters "A-Z 0-9 - _" are allowed', quiet);
/* TOMATO64-END */
					return 0;
				}
			}
		}
		if (this.existName(s)) {
/* TOMATO64-REMOVE-BEGIN */
			ferror.set(f[5], 'Duplicate hostname', quiet);
/* TOMATO64-REMOVE-END */
/* TOMATO64-BEGIN */
			ferror.set(f[4], 'Duplicate hostname', quiet);
/* TOMATO64-END */
			return 0;
		}
/* TOMATO64-REMOVE-BEGIN */
		f[5].value = s;
/* TOMATO64-REMOVE-END */
/* TOMATO64-BEGIN */
		f[4].value = s;
/* TOMATO64-END */
	}

	if (isMAC0(f[0].value)) {
		if (s == '') {
			s = 'Both MAC address and name fields must not be empty';
			ferror.set(f[0], s, 1);
/* TOMATO64-REMOVE-BEGIN */
			ferror.set(f[5], s, quiet);
/* TOMATO64-REMOVE-END */
/* TOMATO64-BEGIN */
			ferror.set(f[4], s, quiet);
/* TOMATO64-END */
			return 0;
		}
		else {
			ferror.clear(f[0]);
/* TOMATO64-REMOVE-BEGIN */
			ferror.clear(f[5]);
/* TOMATO64-REMOVE-END */
/* TOMATO64-BEGIN */
			ferror.clear(f[4]);
/* TOMATO64-END */
		}
	}

	if (((f[0].value == mac_null) || (f[1].value == mac_null)) && (f[0].value == f[1].value)) {
		f[2].disabled = 1;
		f[2].checked = 0;
	}
	else
		f[2].disabled = 0;

	return 1;
}

function verifyFields(focused, quiet) {
	var v;

	v = (E('_f_dhcpd_sltsel').value == 1);
	elem.display('_dhcpd_sltman', v);

	if ((v) && (!v_range('_f_dhcpd_slt', quiet, 1, 43200)))
		return 0;

	return 1;
}

function save() {
	if (sg.isEditing())
		return;

	var data = sg.getAllData();
	var sdhcp = '';
	var ipt = '';
	var i;

	for (i = 0; i < data.length; ++i) {
		var d = data[i];
		sdhcp += d[0];
		if (!isMAC0(d[1]))
			sdhcp += ','+d[1];

/* TOMATO64-REMOVE-BEGIN */
		sdhcp += '<'+d[3]+'<'+d[5]+'<'+d[2]+'>';
/* TOMATO64-REMOVE-END */
/* TOMATO64-BEGIN */
		sdhcp += '<'+d[3]+'<'+d[4]+'<'+d[2]+'>';
/* TOMATO64-BEGIN */
/* TOMATO64-REMOVE-BEGIN */
		if (d[4] == '1')
			ipt += ((ipt.length > 0) ? ',' : '')+d[3];
/* TOMATO64-REMOVE-END */
	}

	var fom = E('t_fom');
	var a = fom._f_dhcpd_sltsel.value;

	fom.dhcpd_slt.value = (a != 1) ? a : fom._f_dhcpd_slt.value;
	fom.dhcpd_static.value = sdhcp;
	fom.cstats_include.value = ipt;
	form.submit(fom, 1);
}

function earlyInit() {
	sg.setup();
	verifyFields(null, 1);
}

function init() {
	var c;
	if (((c = cookie.get(cprefix+'_notes_vis')) != null) && (c == '1'))
		toggleVisibility(cprefix, 'notes');

	sg.recolor();
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

<input type="hidden" name="_nextpage" value="basic-static.asp">
<input type="hidden" name="_service" value="arpbind-restart,cstats-restart,dnsmasq-restart">
<input type="hidden" name="dhcpd_static">
<input type="hidden" name="dhcpd_slt">
<input type="hidden" name="cstats_include">

<!-- / / / -->

<div class="section-title">DHCP Reservation</div>
<div class="section">
	<div class="tomato-grid" id="bs-grid"></div>
</div>

<!-- / / / -->

<div class="section-title">Options</div>
<div class="section">
	<script>
		createFieldTable('', [
			{ title: 'Static lease time', multi: [
				{ name: 'f_dhcpd_sltsel', type: 'select', options: [[0,'Same as normal lease time'],[-1,'"Infinite"'],[1,'Custom']], value: (nvram.dhcpd_slt < 1) ? nvram.dhcpd_slt : 1 },
				{ name: 'f_dhcpd_slt', type: 'text', maxlen: 5, size: 8, prefix: '<span id="_dhcpd_sltman"> ', suffix: ' <i>(minutes)<\/i><\/span>', value: (nvram.dhcpd_slt >= 1) ? nvram.dhcpd_slt : 3600 } ] }
		]);
	</script>
</div>

<!-- / / / -->

<div class="section-title">Notes <small><i><a href="javascript:toggleVisibility(cprefix,'notes');"><span id="sesdiv_notes_showhide">(Show)</span></a></i></small></div>
<div class="section" id="sesdiv_notes" style="display:none">
	<ul>
		<li><b>MAC Address</b> - Unique identifier associated to a network interface on this particular device.</li>
		<li><b>Static arp</b> - Enforce static ARP binding of this particular IP/MAC address pair.</li>
		<li><b>IP Address</b> - Network address assigned to this device on the local network.</li>
		<li><b>IPTraffic</b> - Keep track of bandwidth usage for this IP address.</li>
		<li><b>Hostname</b> - Human-readable nickname/label assigned to this device on the network.</li>
	</ul>
	<ul>
		<li><small><b>Other relevant notes/hints:</b></small>
			<ul>
				<li><small>To associate a name to a MAC address (useful for the Device list page) while retaining dynamic DHCP allocation, just leave the IP address field empty.</small></li>
				<li><small>To specify multiple hostnames for a device, separate them with spaces (not available when IP address is empty).</small></li>
				<li><small>To enable/enforce static ARP binding for a particular device, it must have only one MAC associated with that particular IP address (i.e. you can't have two MAC addresses linked to the same hostname/device in the table above).</small></li>
				<li><small>When ARP binding is enabled for a particular MAC/IP address pair, that device will always be shown as "active" in the <a href="tools-wol.asp">Wake On LAN</a> table.</small></li>
				<li><small>See also the <a href="advanced-dhcpdns.asp">Advanced DHCP/DNS/TFTP</a> settings page for more DHCP-related configuration options.</small></li>
			</ul>
		</li>
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
