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
<title>[<% ident(); %>] Bandwidth: Monthly</title>
<link rel="stylesheet" type="text/css" href="tomato.css?rel=<% version(); %>">
<% css(); %>
<script src="tomato.js?rel=<% version(); %>"></script>
<script src="bwm-hist.js?rel=<% version(); %>"></script>

<script>

//	<% nvram("wan_ifname,wan2_ifname,wan3_ifname,wan4_ifname,lan_ifname,rstats_enable"); %>

try {
//	<% bandwidth("monthly","bwm"); %>
}
catch (ex) {
	monthly_history = [];
}
rstats_busy = 0;
if (typeof(monthly_history) == 'undefined') {
	monthly_history = [];
	rstats_busy = 1;
}

function genData() {
	var w, i, h;

	w = window.open('', 'tomato_data_m');
	w.document.writeln('<pre>');
	for (i = 0; i < monthly_history.length; ++i) {
		h = monthly_history[i];
		w.document.writeln([(((h[0] >> 16) & 0xFF) + 1900), (((h[0] >>> 8) & 0xFF) + 1), h[1], h[2]].join(','));
	}
	w.document.writeln('<\/pre>');
	w.document.close();
}

function save() {
	cookie.set('monthly', scale, 31);
}

function redraw() {
	var h;
	var grid;
	var rows;
	var yr, mo, da;

	rows = 0;
	block = '';
	gn = 0;

	grid = '<table class="bwmg">';
	grid += makeRow('header', 'Date', 'Download', 'Upload', 'Total');

	for (i = 0; i < monthly_history.length; ++i) {
		h = monthly_history[i];
		yr = (((h[0] >> 16) & 0xFF) + 1900);
		mo = ((h[0] >>> 8) & 0xFF);

		grid += makeRow(((rows & 1) ? 'odd' : 'even'), ymText(yr, mo), rescale(h[1]), rescale(h[2]), rescale(h[1] + h[2]));
		++rows;
	}

	E('bwm-grid').innerHTML = grid + '<\/table>';
}

function init() {
	var s;

	if (nvram.rstats_enable != '1') {
		E('refresh-button').disabled = 1;
		return;
	}

	if ((s = cookie.get('monthly')) != null) {
		if (s.match(/^([0-2])$/)) {
			E('scale').value = scale = RegExp.$1 * 1;
		}
	}

	initDate('ym');
	monthly_history.sort(cmpHist);
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

<div class="section-title">WAN Bandwidth - Monthly</div>
<div class="section">

	<div id="rstats">
		<div id="bwm-grid"></div>

		<div id="bwm-ctrl">
			<b>Date</b> <select onchange='changeDate(this, "ym")' id="dafm"><option value="0">yyyy-mm</option><option value="1">mm-yyyy</option><option value="2">mmm yyyy</option><option value="3">mm.yyyy</option></select><br>
			<b>Scale</b> <select onchange="changeScale(this)" id="scale"><option value="0">KB</option><option value="1">MB</option><option value="2" selected="selected">GB</option></select><br>
			<br>
			&raquo; <a href="javascript:genData()">Data</a>
			<br>
			&raquo; <a href="admin-bwm.asp">Configure</a>
		</div>
	</div style="clear:both">

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
