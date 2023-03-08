<!DOCTYPE html>
<!--
	Tomato GUI
	Copyright (C) 2006-2010 Jonathan Zarate
	http://www.polarcloud.com/tomato/

	IP Traffic enhancements
	Copyright (C) 2011 Augusto Bott
	http://code.google.com/p/tomato-sdhc-vlan/

	For use with Tomato Firmware only.
	No part of this file may be used without permission.
-->
<html lang="en-GB">
<head>
<meta http-equiv="content-type" content="text/html;charset=utf-8">
<meta name="robots" content="noindex,nofollow">
<title>[<% ident(); %>] IP Traffic: Last 24 Hours</title>
<link rel="stylesheet" type="text/css" href="tomato.css">
<% css(); %>
<script src="tomato.js"></script>
<script src="wireless.jsx?_http_id=<% nv(http_id); %>"></script>
<script src="bwm-common.js"></script>
<script src="bwm-hist.js"></script>
<script src="interfaces.js"></script>

<script>

//	<% devlist(); %>

var cprefix = 'ipt_';
var updateInt = 60;
var updateDiv = updateInt;
var updateMaxL = 1440;
var updateReTotal = 1;
var hours = 24;
var lastHours = 0;
var debugTime = 0;
var cstats_busy = 0;

var ipt_addr_shown = [];
var ipt_addr_hidden = [];
var hostnamecache = [];

var ref = new TomatoRefresh('update.cgi', 'exec=bandwidth&arg0=speed&arg1=ipt');

ref.refresh = function(text) {
	++updating;
	try {
		this.refreshTime = 1500;
		speed_history = {};
		try {
			eval(text);

			var i;
			for (i in speed_history) {
				if ((ipt_addr_hidden.find(i) == -1) && (ipt_addr_shown.find(i) == -1) && (i != '_next') && (i.trim() != '')) {
					ipt_addr_shown.push(i);
					var option = document.createElement('option');
					option.value = i;
					if (hostnamecache[i] != null)
						option.text = hostnamecache[i]+' ('+i+')';
					else
						option.text = i;

					E('_f_ipt_addr_shown').add(option, null);
					speed_history[i].hide = 0;
				}

				if (ipt_addr_hidden.find(i) != -1)
					speed_history[i].hide = 1;
				else
					speed_history[i].hide = 0;
			}

			if (cstats_busy)
				cstats_busy = 0;

			this.refreshTime = (fixInt(speed_history._next, 1, 60, 30) + 2) * 1000;
		}
		catch (ex) {
			speed_history = {};
			cstats_busy = 1;
		}
		if (debugTime)
			E('dtime').innerHTML = (ymdText(new Date()))+' '+(this.refreshTime / 1000);

		loadData();
	}
	catch (ex) {
/* REMOVE-BEGIN
		alert('ex='+ex);
REMOVE-END */
	}
	--updating;
}

ref.initX = function() {
	var a;

	a = fixInt(cookie.get(cprefix+'refresh'), 0, 1, 1);
	if (a) {
		ref.refreshTime = 100;
		ref.toggleX();
	}
}

ref.toggleX = function() {
	this.toggle();
	this.showState();
	cookie.set(cprefix+'refresh', this.running ? 1 : 0);
}

ref.showState = function() {
	E('refresh-button').value = this.running ? 'Stop' : 'Start';
}

function showHours() {
	if (hours == lastHours)
		return;

	showSelectedOption('hr', lastHours, hours);
	lastHours = hours;
}

function switchHours(h) {
	if ((!svgReady) || (updating))
		return;

	hours = h;
	updateMaxL = (1440 / 24) * hours;
	showHours();
	loadData();
	cookie.set(cprefix+'hrs', hours);
}

function verifyFields(focused, quiet) {
	var changed_addr_hidden = 0;
	if (focused != null) {
		if (focused.id == '_f_ipt_addr_shown') {
			ipt_addr_shown.remove(focused.options[focused.selectedIndex].value);
			ipt_addr_hidden.push(focused.options[focused.selectedIndex].value);
			var option = document.createElement('option');
			option.text = focused.options[focused.selectedIndex].text;
			option.value = focused.options[focused.selectedIndex].value;
			E('_f_ipt_addr_shown').remove(focused.selectedIndex);
			E('_f_ipt_addr_shown').selectedIndex = 0;
			E('_f_ipt_addr_hidden').add(option, null);
			changed_addr_hidden = 1;
		}

		if (focused.id == '_f_ipt_addr_hidden') {
			ipt_addr_hidden.remove(focused.options[focused.selectedIndex].value);
			ipt_addr_shown.push(focused.options[focused.selectedIndex].value);
			var option = document.createElement('option');
			option.text = focused.options[focused.selectedIndex].text;
			option.value = focused.options[focused.selectedIndex].value;
			E('_f_ipt_addr_hidden').remove(focused.selectedIndex);
			E('_f_ipt_addr_hidden').selectedIndex = 0;
			E('_f_ipt_addr_shown').add(option, null);
			changed_addr_hidden = 1;
		}
		if (changed_addr_hidden == 1) {
			cookie.set('ipt_addr_hidden', ipt_addr_hidden.join(','), 1);
			if (!ref.running) {
				ref.once = 1;
				ref.start();
			}
			else {
				ref.stop();
				ref.start();
			}
		}
	}

	if (E('_f_ipt_addr_hidden').length < 2)
		E('_f_ipt_addr_hidden').setAttribute('disabled', 'disabled');
	else
		E('_f_ipt_addr_hidden').removeAttribute('disabled');

	if (E('_f_ipt_addr_shown').length < 2)
		E('_f_ipt_addr_shown').setAttribute('disabled', 'disabled');
	else
		E('_f_ipt_addr_shown').removeAttribute('disabled');

	return 1;
}

function init() {
	if (nvram.cstats_enable != '1') {
		E('refresh-button').setAttribute('disabled', 'disabled');
		return;
	}

	populateCache();

	var c, i;
	if ((c = cookie.get('ipt_addr_hidden')) != null) {
		c = c.split(',');
		for (var i = 0; i < c.length; ++i) {
			if (c[i].trim() != '') {
				ipt_addr_hidden.push(c[i]);
				var option = document.createElement('option');
				option.value = c[i];
				if (hostnamecache[c[i]] != null)
					option.text = hostnamecache[c[i]]+' ('+c[i]+')';
				else
					option.text = c[i];

				E('_f_ipt_addr_hidden').add(option, null);
			}
		}
	}

	try {
	//	<% bandwidth("speed","ipt"); %>

		for (i in speed_history) {
			if ((ipt_addr_hidden.find(i) == -1) && (ipt_addr_shown.find(i) == -1) && (i != '_next') && (i.trim() != '')) {
				ipt_addr_shown.push(i);
				var option = document.createElement('option');
				option.value = i;
				if (hostnamecache[i] != null)
					option.text = hostnamecache[i]+' ('+i+')';
				else
					option.text = i;

				E('_f_ipt_addr_shown').add(option, null);
				speed_history[i].hide = 0;
			}

			if (ipt_addr_hidden.find(i) != -1)
				speed_history[i].hide = 1;
			else
				speed_history[i].hide = 0;
		}
	}
	catch (ex) {
/* REMOVE-BEGIN
		speed_history = {};
REMOVE-END */
	}

	cstats_busy = 0;
	if (typeof(speed_history) == 'undefined') {
		speed_history = {};
		cstats_busy = 1;
	}

	hours = fixInt(cookie.get(cprefix+'hrs'), 1, 24, 24);
	updateMaxL = (1440 / 24) * hours;
	showHours();

	initCommon(1, 0, 0, 1);

	verifyFields(null, 1);

	ref.initX();
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

<div class="section-title">IP Traffic - Last 24 Hours</div>
<div class="section">

	<div id="cstats">
		<div id="tab-area"></div>

		<script>
			if ((nvram.web_svg != '0') && (nvram.cstats_enable == '1')) {
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
			<small>(1 minute interval)</small><br>

			<br>

			Hours:&nbsp;
			<a href="javascript:switchHours(1);" id="hr1">1</a>,
			<a href="javascript:switchHours(2);" id="hr2">2</a>,
			<a href="javascript:switchHours(4);" id="hr4">4</a>,
			<a href="javascript:switchHours(6);" id="hr6">6</a>,
			<a href="javascript:switchHours(12);" id="hr12">12</a>,
			<a href="javascript:switchHours(18);" id="hr18">18</a>,
			<a href="javascript:switchHours(24);" id="hr24">24</a><br>
			Avg:&nbsp;
			<a href="javascript:switchAvg(1)" id="avg1">Off</a>,
			<a href="javascript:switchAvg(2)" id="avg2">2x</a>,
			<a href="javascript:switchAvg(4)" id="avg4">4x</a>,
			<a href="javascript:switchAvg(6)" id="avg6">6x</a>,
			<a href="javascript:switchAvg(8)" id="avg8">8x</a><br>
			Max:&nbsp;
			<a href="javascript:switchScale(0)" id="scale0">Uniform</a>,
			<a href="javascript:switchScale(1)" id="scale1">Per Address</a><br>
			Unit:&nbsp;
			<a href="javascript:switchUnit(0)" id="unit0">kbit/KB</a>,
			<a href="javascript:switchUnit(1)" id="unit1">Mbit/MB</a><br>
			Display:&nbsp;
			<a href="javascript:switchDraw(0)" id="draw0">Solid</a>,
			<a href="javascript:switchDraw(1)" id="draw1">Line</a><br>
			Color:&nbsp; <a href="javascript:switchColor()" id="drawcolor">-</a><br>
			<small><a href="javascript:switchColor(1)" id="drawrev">[reverse]</a></small><br>

			<br><br>

			&nbsp; &raquo; <a href="admin-iptraffic.asp">Configure</a>
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

		<div>
			<script>
				createFieldTable('', [
					{ title: 'IPs currently on graphic', name: 'f_ipt_addr_shown', type: 'select', options: [[0,'Select']], suffix: ' <small>(Click/select a device from this list to hide it)<\/small>' },
					{ title: 'Hidden addresses', name: 'f_ipt_addr_hidden', type: 'select', options: [[0,'Select']], suffix: ' <small>(Click/select to show it again)<\/small>' }
				]);
			</script>
		</div>

<!-- / / / -->

	</div>

</div>

<!-- / / / -->

<script>checkStats('cstats');</script>

<!-- / / / -->

<div id="footer">
	<span id="dtime"></span>
	<img src="spin.gif" id="refresh-spinner" alt="" onclick="debugTime=1">
	<input type="button" value="Refresh" id="refresh-button" onclick="ref.toggleX()">
</div>

</td></tr>
</table>
</form>
</body>
</html>
