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
<title>[<% ident(); %>] Tools: Wake on LAN</title>
<link rel="stylesheet" type="text/css" href="tomato.css">
<% css(); %>
<script src="tomato.js"></script>

<script>

//	<% arplist(); %>

//	<% nvram("dhcpd_static,lan_ifname,lan1_ifname,lan2_ifname,lan3_ifname"); %>

var lan_ifnames = [nvram.lan_ifname, nvram.lan1_ifname, nvram.lan2_ifname, nvram.lan3_ifname];
var refresher = null;
var running = 0;

var timer = new TomatoTimer(refresh);

function refresh() {
	if (!running)
		return;

	timer.stop();

	refresher = new XmlHttp();
	refresher.onCompleted = function(text, xml) {
		eval(text);
		wg.populate();
		timer.start(5000);
		refresher = null;
	}
	refresher.onError = function(ex) { alert(ex); reloadPage(); }
	refresher.post('update.cgi', 'exec=arplist');
}

var wg = new TomatoGrid();

wg.setup = function() {
	this.init('wol-grid','sort');
	this.headerSet(['MAC Address','IP Address','Status','Name']);
	this.sort(3);
}

wg.populate = function() {
	var i, j, r, s;

	this.removeAllData();

	s = [];
	var q = nvram.dhcpd_static.split('>');
	for (i = 0; i < q.length; ++i) {
		var e = q[i].split('<');
		if (e.length == 4 && e[1] != '') {
			var m = e[0].split(',');
			for (j = 0; j < m.length; ++j) {
				s.push([m[j], e[1], e[2]]);
			}
		}
	}

	/* show entries in static dhcp list */
	for (i = 0; i < s.length; ++i) {
		var t = s[i];
		var active = '-';
		for (j = 0; j < arplist.length; ++j) {
			if ((lan_ifnames.indexOf(arplist[j][2]) != -1) && (t[0] == arplist[j][1])) {
				active = 'Active (in ARP)';
				arplist[j][1] = '!';
				break;
			}
		}
		if (t.length == 3) {
			r = this.insertData(-1, [t[0], t[1], active, t[2]]);
			for (j = 0; j < 4; ++j)
				r.cells[j].title = 'Click to wake up';
		}
	}

	/* show anything else in ARP that is awake */
	for (i = 0; i < arplist.length; ++i) {
		if ((lan_ifnames.indexOf(arplist[i][2]) == -1) || (arplist[i][1].length != 17))
			continue;

		r = this.insertData(-1, [arplist[i][1], arplist[i][0], 'Active (in ARP)', '']);
		for (j = 0; j < 4; ++j)
			r.cells[j].title = 'Click to wake up';
	}

	this.resort(2);
}

wg.sortCompare = function(a, b) {
	var da = a.getRowData();
	var db = b.getRowData();
	var r = 0;
	var c = this.sortColumn;

	if (c == 1)
		r = cmpIP(da[c], db[c]);
	else
		r = cmpText(da[c], db[c]);

	return this.sortAscending ? r : -r;
}

wg.onClick = function(cell) {
	wake(PR(cell).getRowData()[0]);
}

function wake(mac) {
	if (!mac) {
		if (!verifyFields(null, 1))
			return;

		mac = E('t_f_mac').value;
		cookie.set('wakemac', mac);
	}
	E('t_mac').value = mac;

	form.submit('t_fom', 1);
}

function refreshClick() {
	running ^= 1;
	E('refresh-button').value = running ? 'Stop' : 'Refresh';
	E('refresh-spinner').style.display = (running ? 'inline-block' : 'none');

	if (running)
		refresh();
}

function verifyFields(focused, quiet) {
	var e;

	e = E('t_f_mac');
	e.value = e.value.replace(/[\t ]+/g, ' ');

	return (e.value ? 1 : 0);
}

function init() {
	wg.recolor();
}
</script>
</head>

<body onload="init()">
<form id="t_fom" action="wakeup.cgi" method="post">
<table id="container">
<tr><td colspan="2" id="header">
	<div class="title">FreshTomato</div>
	<div class="version">Version <% version(); %> on <% nv("t_model_name"); %></div>
</td></tr>
<tr id="body"><td id="navi"><script>navi()</script></td>
<td id="content">
<div id="ident"><% ident(); %> | <script>wikiLink();</script></div>

<!-- / / / -->

<input type="hidden" name="_redirect" value="tools-wol.asp">
<input type="hidden" name="_nextwait" value="1">
<input type="hidden" name="mac" value="" id="t_mac">

<!-- / / / -->

<div class="section-title">Wake on LAN</div>
<div class="section">
	<div class="tomato-grid" id="wol-grid"></div>

	<div><input type="button" value="Refresh" onclick="refreshClick()" id="refresh-button"> &nbsp; <img src="spin.gif" alt="" id="refresh-spinner"></div>
</div>

<!-- / / / -->

<div class="section-title"></div>
<div class="section">
	<script>
		createFieldTable('', [
			{ title: 'MAC Address List', name: 'f_mac', id: 't_f_mac', type: 'textarea', value: cookie.get('wakemac') || '' },
		]);
	</script>
	<div><input id="save-button" type="button" value="Wake Up" onclick="wake(null)"></div>
</div>

<!-- / / / -->

<div id="footer">
	&nbsp;
</div>

</td></tr>
</table>
</form>
<script>wg.setup();wg.populate();</script>
</body>
</html>
