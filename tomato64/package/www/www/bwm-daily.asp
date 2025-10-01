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
<title>[<% ident(); %>] Bandwidth: Daily</title>
<link rel="stylesheet" type="text/css" href="tomato.css?rel=<% version(); %>">
<% css(); %>
<script src="tomato.js?rel=<% version(); %>"></script>
<script src="bwm-hist.js?rel=<% version(); %>"></script>

<script>

//	<% nvram("wan_ifname,lan_ifname,rstats_enable"); %>

try {
//	<% bandwidth("daily","bwm"); %>
}
catch (ex) {
	daily_history = [];
}
rstats_busy = 0;
if (typeof(daily_history) == 'undefined') {
	daily_history = [];
	rstats_busy = 1;
}

function save() {
	cookie.set('daily', scale, 31);
}

function genData() {
	var w, i, h, t;

	w = window.open('', 'tomato_data_d');
	w.document.writeln('<pre>');
	for (i = 0; i < daily_history.length; ++i) {
		h = daily_history[i];
		t = getYMD(h[0]);
		w.document.writeln([t[0], t[1] + 1, t[2], h[1], h[2]].join(','));
	}
	w.document.writeln('<\/pre>');
	w.document.close();
}

function getYMD(n) {
	// [y,m,d]
	return [(((n >> 16) & 0xFF) + 1900), ((n >>> 8) & 0xFF), (n & 0xFF)];
}

function redraw() {
	var h;
	var grid;
	var rows;
	var ymd;
	var d;
	var lastt;
	var lastu, lastd;

	if (daily_history.length > 0) {
		ymd = getYMD(daily_history[0][0]);
		d = new Date((new Date(ymd[0], ymd[1], ymd[2], 12, 0, 0, 0)).getTime() - ((30 - 1) * 86400000));
		E('last-dates').innerHTML = '('+ymdText(d.getFullYear(), d.getMonth(), d.getDate())+' to '+ymdText(ymd[0], ymd[1], ymd[2])+')';

		lastt = ((d.getFullYear() - 1900) << 16) | (d.getMonth() << 8) | d.getDate();
	}

	lastd = 0;
	lastu = 0;
	rows = 0;
	block = '';
	gn = 0;

	grid = '<table class="bwmg">';
	grid += makeRow('header', 'Date', 'Download', 'Upload', 'Total');

	for (i = 0; i < daily_history.length; ++i) {
		h = daily_history[i];
		ymd = getYMD(h[0]);
		grid += makeRow(((rows & 1) ? 'odd' : 'even'), ymdText(ymd[0], ymd[1], ymd[2]), rescale(h[1]), rescale(h[2]), rescale(h[1] + h[2]));
		++rows;

		if (h[0] >= lastt) {
			lastd += h[1];
			lastu += h[2];
		}
	}

	E('bwm-grid').innerHTML = grid+'<\/table>';

	E('last-dn').innerHTML = rescale(lastd);
	E('last-up').innerHTML = rescale(lastu);
	E('last-total').innerHTML = rescale(lastu + lastd);
}

function init() {
	var s;

	if (nvram.rstats_enable != '1') {
		E('refresh-button').disabled = 1;
		return;
	}

	if ((s = cookie.get('daily')) != null) {
		if (s.match(/^([0-2])$/)) {
			E('scale').value = scale = RegExp.$1 * 1;
		}
	}

	initDate('ymd');
	daily_history.sort(cmpHist);
	redraw();
}
</script>

</head>
<body onload="init()">
<form>
<table id="container">
<tr><td colspan="2" id="header">
	<div class="title"><a href="/">Tomato64</a></div>
	<div class="version">Version <% version(); %> on <% nv("t_model_name"); %></div>
</td></tr>
<tr id="body"><td id="navi"><script>navi()</script></td>
<td id="content">
<div id="ident"><% ident(); %> | <script>wikiLink();</script></div>

<!-- / / / -->

<div class="section-title">WAN Bandwidth - Daily</div>
<div class="section">

	<div id="rstats">
		<div id="bwm-grid"></div>

		<div id="bwm-ctrl">
			<table class="tomato-grid" id="bwm-last">
				<tr class="header"><td id="bwm-h-ctrl" colspan="2">Last 30 Days<br><span id="last-dates" style="font-weight:normal"></span></td></tr>
				<tr class="even"><td>Down</td><td id="last-dn">-</td></tr>
				<tr class="odd"><td>Up</td><td id="last-up">-</td></tr>
				<tr class="footer"><td>Total</td><td id="last-total">-</td></tr>
			</table>

			<b>Date</b> <select onchange='changeDate(this, "ymd")' id="dafm"><option value="0">yyyy-mm-dd</option><option value="1">mm-dd-yyyy</option><option value="2">mmm dd, yyyy</option><option value="3">dd.mm.yyyy</option></select><br>
			<b>Scale</b> <select onchange="changeScale(this)" id="scale"><option value="0">KB</option><option value="1">MB</option><option value="2" selected="selected">GB</option></select><br>
			<br>
			&raquo; <a href="javascript:genData()">Data</a>
			<br>
			&raquo; <a href="admin-bwm.asp">Configure</a>
		</div>
	</div>

</div>

<!-- / / / -->

<script>checkStats('rstats');</script>

<!-- / / / -->

<div id="footer">
	<input type="button" value="Refresh" id="refresh-button" onclick="reloadPage()">
</div>

</td></tr>
</table>
</form>
</body>
</html>
