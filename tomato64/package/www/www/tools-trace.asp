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
<title>[<% ident(); %>] Tools: Traceroute</title>
<link rel="stylesheet" type="text/css" href="tomato.css?rel=<% version(); %>">
<% css(); %>
<script src="tomato.js?rel=<% version(); %>"></script>

<script>

//	<% nvram(''); %>	// http_id

var tracedata = '';

var tg = new TomatoGrid();
tg.setup = function() {
	this.init('tools-grid');
	this.headerSet(['Hop', 'Address', 'Min (ms)', 'Max (ms)', 'Avg (ms)', '+/- (ms)']);
}

tg.populate = function() {
	var seq = 1;
	var buf = tracedata.split('\n');
	var i, j, k;
	var s, f;
	var addr, emsg, min, max, avg;
	var time;
	var last = -1;

	this.removeAllData();
	for (i = 0; i < buf.length; ++i) {
		if (!buf[i].match(/^\s*(\d+)\s+(.+)$/)) continue;
		if (RegExp.$1 != seq) continue;

		s = RegExp.$2;

		if (s.match(/^([\w\.:\-]+)\s+\(([\d\.:A-Fa-f]+)\)/)) {
			addr = RegExp.$1;
			if (addr != RegExp.$2) addr += ' (' + RegExp.$2 + ')';
		}
		else {
			addr = '*';
		}

		min = max = avg = '';
		change = '';
		if (time = s.match(/(\d+\.\d+) ms/g)) { /* odd: captures 'ms' */
			min = 0xFFFF;
			avg = max = 0;
			k = 0;
			for (j = 0; j < time.length; ++j) {
				f = parseFloat(time[j]);
				if (isNaN(f)) continue;
				if (f < min) min = f;
				if (f > max) max = f;
				avg += f;
				++k
			}
			if (k) {
				avg /= k;
				if (last >= 0) {
					change = avg - last;
					change = change.toFixed(2);
				}
				last = avg;
				min = min.toFixed(2);
				max = max.toFixed(2);
				avg = avg.toFixed(2);
			}
			else {
				min = max = avg = '';
				last = -1;
			}
		}
		else {
			last = -1;
		}

		if (s.match(/ (![<>\w+-]+)/))
			emsg = RegExp.$1;
		else
			emsg = null;

		this.insertData(-1, [seq, addr, min, max, avg, change])
		++seq;
	}

	E('debug').value = tracedata;
	tracedata = '';
	spin(0);
}

function verifyFields(focused, quiet) {
	var s;
	var e;

	e = E('_f_addr');
	s = e.value.trim();
	if (!s.match(/^[\w\-\.\:]+$/)) {
		ferror.set(e, 'Invalid hostname/address', quiet);
		return 0;
	}
	ferror.clear(e);

	return v_range('_f_hops', quiet, 2, 40) && v_range('_f_wait', quiet, 2, 10);
}

var tracer = null;

function spin(x) {
	E('traceb').disabled = x;
	E('_f_addr').disabled = x;
	E('_f_hops').disabled = x;
	E('_f_wait').disabled = x;
	E('spin').style.display = (x ? 'inline-block' : 'none');
	if (!x) tracer = null;
}

function trace() {
	/* Opera 8 sometimes sends 2 clicks */
	if (tracer) return;

	if (!verifyFields(null, 0)) return;
	spin(1);
	E('trace-error').style.display = 'none';

	tracer = new XmlHttp();
	tracer.onCompleted = function(text, xml) {
		eval(text);
		tg.populate();
	}
	tracer.onError = function(x) {
		spin(0);
		E('trace-error').innerHTML = 'ERROR: ' + E('_f_addr').value + ' - ' + x;
		E('trace-error').style.display = 'inline-block';
	}

	var addr = E('_f_addr').value;
	var hops = E('_f_hops').value;
	var wait = E('_f_wait').value;
	tracer.post('trace.cgi', 'addr=' + addr + '&hops=' + hops + '&wait=' + wait);

	cookie.set('traceaddr', addr);
	cookie.set('tracehops', hops);
	cookie.set('tracewait', wait);
}

function init() {
	var s;

	if ((s = cookie.get('traceaddr')) != null) E('_f_addr').value = s;
	if ((s = cookie.get('tracehops')) != null) E('_f_hops').value = s;
	if ((s = cookie.get('tracewait')) != null) E('_f_wait').value = s;

	E('_f_addr').onkeypress = function(ev) { if (checkEvent(ev).keyCode == 13) trace(); }
}
</script>
</head>

<body onload="init()">
<form action="javascript:{}">
<table id="container">
<tr><td colspan="2" id="header">
	<div class="title"><a href="/">Tomato64</a></div>
	<div class="version">Version <% version(); %> on <% nv("t_model_name"); %></div>
</td></tr>
<tr id="body"><td id="navi"><script>navi()</script></td>
<td id="content">
<div id="ident"><% ident(); %> | <script>wikiLink();</script></div>

<!-- / / / -->

<div class="section-title">Traceroute</div>
<div class="section">
	<script>
		createFieldTable('', [
			{ title: 'Address', name: 'f_addr', type: 'text', maxlen: 64, size: 32, value: '', suffix: ' <input type="button" value="Trace" onclick="trace()" id="traceb"> <img src="spin.gif" alt="" id="spin">' },
			{ title: 'Maximum Hops', name: 'f_hops', type: 'text', maxlen: 2, size: 4, value: '20' },
			{ title: 'Maximum Wait Time', name: 'f_wait', type: 'text', maxlen: 2, size: 4, value: '3', suffix: ' <small>(seconds per hop)<\/small>' }
		]);
	</script>
</div>

<!-- / / / -->

<div id="trace-error" style="display:none"></div>

<!-- / / / -->

<div class="tomato-grid" id="tools-grid"></div>

<!-- / / / -->

<div style="height:10px" onclick='E("debug").style.display=""'></div>
<textarea id="debug" style="display:none"></textarea>

<!-- / / / -->

<div id="footer">
	&nbsp;
</div>

</td></tr>
</table>
</form>
<script>tg.setup()</script>
</body>
</html>
