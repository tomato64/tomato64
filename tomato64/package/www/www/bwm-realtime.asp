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
<title>[<% ident(); %>] Bandwidth: Real-Time</title>
<link rel="stylesheet" type="text/css" href="tomato.css">
<% css(); %>
<script src="tomato.js"></script>
<script src="wireless.jsx?_http_id=<% nv(http_id); %>"></script>
<script src="bwm-common.js"></script>
<script src="bwm-hist.js"></script>
<script src="interfaces.js"></script>

<script>

//	<% nvram("wan_ifname,wan_iface,wan2_ifname,wan2_iface,wan3_ifname,wan3_iface,wan4_ifname,wan4_iface,lan_ifname,lan1_ifname,lan2_ifname,lan3_ifname,wan_proto,wan2_proto,wan3_proto,wan4_proto,web_svg,web_css,rstats_enable,wl_nband"); %>

var cprefix = 'bw_r';
var updateInt = 2;
var updateDiv = updateInt;
var updateMaxL = 300;
var updateReTotal = 1;
var prev = [];
var debugTime = 0;
var avgMode = 0;
var unitMode = 0;
var wdog = null;
var wdogWarn = null;
var rstats_busy = 0;

var ref = new TomatoRefresh('update.cgi', 'exec=netdev', updateInt);

ref.stop = function() {
	this.timer.start(1000);
}

ref.refresh = function(text) {
	var c, i, h, n, j, k;

	watchdogReset();

	++updating;
	try {
		netdev = null;
		eval(text);

		n = (new Date()).getTime();
		if (this.timeExpect) {
			if (debugTime) E('dtime').innerHTML = (this.timeExpect - n) + ' ' + ((this.timeExpect + 1000*updateInt) - n);
			this.timeExpect += 1000*updateInt;
			this.refreshTime = MAX(this.timeExpect - n, 500);
		}
		else {
			this.timeExpect = n + 1000*updateInt;
		}

		for (i in netdev) {
			c = netdev[i];
			if ((p = prev[i]) != null) {
				h = speed_history[i];

				h.rx.splice(0, 1);
				h.rx.push((c.rx < p.rx) ? (c.rx + (0xFFFFFFFF - p.rx + 0x00000001)) : (c.rx - p.rx));

				h.tx.splice(0, 1);
				h.tx.push((c.tx < p.tx) ? (c.tx + (0xFFFFFFFF - p.tx + 0x00000001)) : (c.tx - p.tx));
			}
			else if (!speed_history[i]) {
				speed_history[i] = {};
				h = speed_history[i];
				h.rx = [];
				h.tx = [];
				for (j = 300; j > 0; --j) {
					h.rx.push(0);
					h.tx.push(0);
				}
				h.count = 0;
			}
			prev[i] = c;
		}
		loadData();
	}
	catch (ex) {
	}
	--updating;
}

function watchdog() {
	watchdogReset();
	ref.stop();
	wdogWarn.style.display = '';
}

function watchdogReset() {
	if (wdog) clearTimeout(wdog)
	wdog = setTimeout(watchdog, 5000*updateInt);
	wdogWarn.style.display = 'none';
}

function init() {
	if (nvram.rstats_enable != '1') return;
	speed_history = [];

	initCommon(2, 1, 0, 1);

	wdogWarn = E('warnwd');
	watchdogReset();

	ref.start();
}
</script>
</head>

<body onload="init()">
<form>
<table id="container">
<tr><td colspan="2" id="header">
	<div class="title">FreshTomato</div>
	<div class="version">Version <% version(); %> on <% nv("t_model_name"); %></div>
</td></tr>
<tr id="body"><td id="navi"><script>navi()</script></td>
<td id="content">
<div id="ident"><% ident(); %> | <script>wikiLink();</script></div>

<!-- / / / -->

<div class="section-title">WAN Bandwidth - Real-Time</div>
<div class="section">

	<div id="rstats">
		<div id="tab-area"></div>

		<script>
			if ((nvram.web_svg != '0') && (nvram.rstats_enable == '1')) {
				var vWidth = 760;
				var vHeight = 300;
				if (nvram.web_css.match(/at-/g)) {
					vWidth = 1200;
					vHeight = 500;
				}
				W('<div id="graph"><embed src="bwm-graph.svg?vwidth='+vWidth+'&vheight='+vHeight+'" type="image/svg+xml" style="width:'+vWidth+'px;height:'+vHeight+'px"><\/embed><\/div>');
			}
		</script>

		<div id="bwm-controls">
			<small>(10 minute window, 2 second interval)</small><br>

			<br>

			Avg:&nbsp;
			<a href="javascript:switchAvg(1)" id="avg1">Off</a>,
			<a href="javascript:switchAvg(2)" id="avg2">2x</a>,
			<a href="javascript:switchAvg(4)" id="avg4">4x</a>,
			<a href="javascript:switchAvg(6)" id="avg6">6x</a>,
			<a href="javascript:switchAvg(8)" id="avg8">8x</a><br>
			Max:&nbsp;
			<a href="javascript:switchScale(0)" id="scale0">Uniform</a>,
			<a href="javascript:switchScale(1)" id="scale1">Per IF</a><br>
			Unit:&nbsp;
			<a href="javascript:switchUnit(0)" id="unit0">kbit/KB</a>,
			<a href="javascript:switchUnit(1)" id="unit1">Mbit/MB</a><br>
			Display:&nbsp;
			<a href="javascript:switchDraw(0)" id="draw0">Solid</a>,
			<a href="javascript:switchDraw(1)" id="draw1">Line</a><br>
			Color:&nbsp; <a href="javascript:switchColor()" id="drawcolor">-</a><br>
			<small><a href="javascript:switchColor(1)" id="drawrev">[reverse]</a></small><br>

			<br><br>

			&nbsp; &raquo; <a href="admin-bwm.asp">Configure</a>
		</div>

		<br><br>
<!-- / / / -->

		<table id="bwm-stats">
			<tr>
				<td><b id="rx-name">RX</b></td>
				<td><span id="rx-current"></span></td>
				<td><b>Avg</b></td>
				<td id="rx-avg"></td>
				<td><b>Peak</b></td>
				<td id="rx-max"></td>
				<td><b>Total</b></td>
				<td id="rx-total"></td>
			</tr>
			<tr>
				<td><b id="tx-name">TX</b></td>
				<td><span id="tx-current"></span></td>
				<td><b>Avg</b></td>
				<td id="tx-avg"></td>
				<td><b>Peak</b></td>
				<td id="tx-max"></td>
				<td><b>Total</b></td>
				<td id="tx-total"></td>
			</tr>
		</table>

<!-- / / / -->

	</div>

</div>

<!-- / / / -->

<script>checkStats('rstats');</script>

<!-- / / / -->

<div id="footer">
	<span id="warnwd" style="display:none">Warning: 10 second timeout, restarting...&nbsp;</span>
	<span id="dtime"></span>
	<img src="spin.gif" id="refresh-spinner" alt="" onclick="debugTime=1">
</div>

</td></tr>
</table>
</form>
</body>
</html>
