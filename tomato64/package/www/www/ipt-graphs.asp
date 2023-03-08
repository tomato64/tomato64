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
<title>[<% ident(); %>] IP Traffic: View Graphs</title>
<link rel="stylesheet" type="text/css" href="tomato.css">
<% css(); %>
<script src="tomato.js"></script>
<script src="bwm-hist.js"></script>

<script>

//	<% nvram('cstats_enable,web_svg'); %>

//	<% iptraffic(); %>

var colors = [];
for (var i = 1; i < 12; ++i)
	colors.push(getColor('.svg-color'+i));

var irates = [];
var orates = [];
var nfmarks = [];
var abc = [];
var prevtimestamp = new Date().getTime();
var thistimestamp;
var difftimestamp;
var avgiptraffic = [];
var lastiptraffic = iptraffic;
var cstats_busy = 0;
var svgReady = 0;

var ref = new TomatoRefresh('update.cgi', 'exec=iptraffic', 5, 'ipt_graphs');

ref.refresh = function(text) {
	var b, i, j, k, l;
	irates = [];
	orates = [];
	nfmarks = [];
	abc = [];

	thistimestamp = new Date().getTime();

	iptraffic = [];
	try {
		eval(text);
	}
	catch (ex) {
		iptraffic = [];
	}

	difftimestamp = thistimestamp - prevtimestamp;
	prevtimestamp = thistimestamp;

	for (i = 0; i < iptraffic.length; ++i) {
		b = iptraffic[i];

		j = getArrayPosByElement(avgiptraffic, b[0]);
		if (j == -1) {
			j = avgiptraffic.length;
			avgiptraffic[j] = [b[0], 0, 0, 0, 0, 0, 0, 0, 0, b[9], b[10]];
		}

		k = getArrayPosByElement(lastiptraffic, b[0]);

		if (k == -1) {
			k = lastiptraffic.length;
			lastiptraffic[k] = b;
		}

		for (l = 1; l <= 8; ++l) {
			avgiptraffic[j][l] = ((b[l] - lastiptraffic[k][l]) / difftimestamp * 1000);
			lastiptraffic[k][l] = b[l];
		}

		avgiptraffic[j][9] = b[9];
		avgiptraffic[j][10] = b[10];
		lastiptraffic[k][9] = b[9];
		lastiptraffic[k][10] = b[10];
	}

	j = 0;
	for (i = 0; i < avgiptraffic.length; ++i) {
		if (iptraffic[j] != null && (avgiptraffic[i][9] + avgiptraffic[i][10] > 0)) { /* show only active connections */
			abc[j] = avgiptraffic[i][0]; /* IP */
			nfmarks[j] = avgiptraffic[i][9] + avgiptraffic[i][10]; /* TCP + UDP connections */
			irates[j] = avgiptraffic[i][1]; /* RX bytes */
			orates[j] = avgiptraffic[i][2]; /* TX bytes */
		}
		else
			continue;

		++j;
	}

	showData();

	if (svgReady == 1) {
		updateCD(nfmarks, abc);
		updateBI(irates, abc);
		updateBO(orates, abc);
	}
}

function showData() {
	var i, n, p, d, l;
	var ct = 0, irt = 0, ort = 0;

	for (i = 0; i < 11; ++i) {
		ct += nfmarks[i] ? nfmarks[i] : 0;
		irt += irates[i] ? irates[i] : 0;
		ort += orates[i] ? orates[i] : 0;
	}

	for (i = 0; i < 11; ++i) {
		d = (typeof(abc[i]) != 'undefined');

		n = nfmarks[i];
		E('ccnt'+i).innerHTML = d ? n : '';
		p = (ct > 0) ? (n / ct) * 100 : 0;
		elem.setInnerHTML('cpct'+i, d ? p.toFixed(2)+'%' : '');

		n = irates[i];
		elem.setInnerHTML('bcnt'+i, d ? (n / 125).toFixed(2) : '');
		elem.setInnerHTML('bcntx'+i, d ? (n / 1024).toFixed(2) : '');
		p = (irt > 0) ? (n / irt) * 100 : 0;
		elem.setInnerHTML('bpct'+i, d ? p.toFixed(2)+'%' : '');

		n = orates[i];
		elem.setInnerHTML('obcnt'+i, d ? (n / 125).toFixed(2) : '');
		elem.setInnerHTML('obcntx'+i, d ? (n / 1024).toFixed(2) : '');
		p = (ort > 0) ? (n / ort) * 100 : 0;
		elem.setInnerHTML('obpct'+i, d ? p.toFixed(2)+'%' : '');

		l = '<a href="ipt-details.asp?ipt_filterip='+abc[i]+'">'+abc[i]+'<\/a>';
		elem.setInnerHTML('cip'+i, d ? l : '');
		elem.setInnerHTML('bip'+i, d ? l : '');
		elem.setInnerHTML('obip'+i, d ? l : '');
	}

	E('ccnt-total').innerHTML = ct;
	elem.setInnerHTML('bcnt-total', (irt / 125).toFixed(2));
	elem.setInnerHTML('bcntx-total', (irt / 1024).toFixed(2));
	elem.setInnerHTML('obcnt-total', (ort / 125).toFixed(2));
	elem.setInnerHTML('obcntx-total', (ort / 1024).toFixed(2));
}

function getArrayPosByElement(haystack, needle) {
	for (var i = 0; i < haystack.length; ++i) {
		if (haystack[i][0] == needle)
			return i;
	}
	return -1;
}

function init() {
	if (nvram.cstats_enable != '1') {
		E('refresh-time').setAttribute('disabled', 'disabled');
		E('refresh-button').setAttribute('disabled', 'disabled');
		return;
	}
	showData();
	checkSVG();
	ref.initPage(2000, 5);
	if (!ref.running)
		ref.once = 1;

	ref.start();
}
</script>
</head>

<body onload="init()">
<form id="t_fom" action="javascript:{}">
<table id="container">
<tr><td colspan="2" id="header">
	<div class="title">FreshTomato</div>
	<div class="version">Version <% version(); %> on <% nv("t_model_name"); %></div>
</td></tr>
<tr id="body"><td id="navi"><script>navi()</script></td>
<td id="content">
<div id="ident"><% ident(); %> | <script>wikiLink();</script></div>

<!-- / / / -->

<div class="section-title">Connections Distribution (TCP/UDP)</div>

<div id="cstats" class="cstats">

	<div class="section">
		<table class="bwm-svg">
			<tr><td>
				<script>
					W('<table style="width:270px">');
					for (i = 0; i < 11; ++i) {
						W('<tr>'+
						  '<td class="bwm-color" style="background:'+colors[i]+'">&nbsp;<\/td>'+
						  '<td id="cip'+i+'" class="title" style="width:45px"><\/td>'+
						  '<td id="ccnt'+i+'" class="bwm-count" style="width:100px"><\/td>'+
						  '<td id="cpct'+i+'" class="bwm-pct"><\/td><\/tr>');
					}
					W('<tr><td>&nbsp;<\/td><td class="bwm-total">Total<\/td><td id="ccnt-total" class="bwm-total bwm-count"><\/td><td class="bwm-total bwm-pct">100%<\/td><\/tr>');
					W('<\/table>');
				</script>
			</td>
			<td>
				<script>
					if (nvram.web_svg != '0')
						W('<embed src="ipt-graph.svg?n=0" style="width:310px;height:310px" id="svg0" type="image/svg+xml"><\/embed>');
				</script>
			</td></tr>
		</table>
	</div>

<!-- / / / -->

	<div class="section-title">Bandwidth Distribution (Inbound)</div>
	<div class="section">
		<table class="bwm-svg">
			<tr><td>
				<script>
					W('<table style="width:270px">');
					W('<tr><td class="bwm-color"><\/td><td class="title" style="width:45px">&nbsp;<\/td><td class="bwm-thead bwm-count">kbit/s<\/td><td class="bwm-thead bwm-count">KB/s<\/td><td class="bwm-pct">&nbsp;');
					W('<\/td><\/tr>');
					for (i = 0; i < 11; ++i) {
						W('<tr>'+
						  '<td class="bwm-color" style="background:'+colors[i]+'">&nbsp;<\/td>'+
						  '<td id="bip'+i+'" class="title" style="width:45px"><\/td>'+
						  '<td id="bcnt'+i+'" class="bwm-count" style="width:60px"><\/td>'+
						  '<td id="bcntx'+i+'" class="bwm-count" style="width:50px"><\/td>'+
						  '<td id="bpct'+i+'" class="bwm-pct"><\/td><\/tr>');
					}
					W('<tr><td>&nbsp;<\/td><td class="bwm-total">Total<\/td><td id="bcnt-total" class="bwm-total bwm-count"><\/td><td id="bcntx-total" class="bwm-total bwm-count"><\/td><td class="bwm-total bwm-pct">100%<\/td><\/tr>');
					W('<\/table>');
				</script>
			</td>
			<td>
				<script>
					if (nvram.web_svg != '0')
						W('<embed src="ipt-graph.svg?n=1" style="width:310px;height:310px" id="svg1" type="image/svg+xml"><\/embed>');
				</script>
			</td></tr>
		</table>
	</div>

<!-- / / / -->

	<div class="section-title">Bandwidth Distribution (Outbound)</div>
	<div class="section">
		<table class="bwm-svg">
			<tr><td>
				<script>
					W('<table style="width:270px">');
					W('<tr><td class="bwm-color"><\/td><td class="title" style="width:45px">&nbsp;<\/td><td class="bwm-thead bwm-count">kbit/s<\/td><td class="bwm-thead bwm-count">KB/s<\/td><td class="bwm-pct">&nbsp;');
					W('<\/td><\/tr>');
					for (i = 0; i < 11; ++i) {
						W('<tr style="cursor:pointer">'+
						  '<td class="bwm-color" style="background:'+colors[i]+'">&nbsp;<\/td>'+
						  '<td id="obip'+i+'" class="title" style="width:45px"><\/td>'+
						  '<td id="obcnt'+i+'" class="bwm-count" style="width:60px"><\/td>'+
						  '<td id="obcntx'+i+'" class="bwm-count" style="width:50px"><\/td>'+
						  '<td id="obpct'+i+'" class="bwm-pct"><\/td><\/tr>');
					}
					W('<tr><td>&nbsp;<\/td><td class="bwm-total">Total<\/td><td id="obcnt-total" class="bwm-total bwm-count"><\/td><td id="obcntx-total" class="bwm-total bwm-count"><\/td><td class="bwm-total bwm-pct">100%<\/td><\/tr>');
					W('<\/table>');
				</script>
			</td>
			<td>
				<script>
					if (nvram.web_svg != '0')
						W('<embed src="ipt-graph.svg?n=2" style="width:310px;height:310px" id="svg2" type="image/svg+xml"><\/embed>');
				</script>
			</td></tr>
		</table>
	</div>

</div>

<!-- / / / -->

<script>checkStats('cstats');</script>

<!-- / / / -->

<div id="footer">
	<script>genStdRefresh(1,1,'ref.toggle()');</script>
</div>

</td></tr>
</table>
</form>
</body>
</html>
