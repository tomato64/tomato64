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
<title>[<% ident(); %>] QoS: View Graphs</title>
<link rel="stylesheet" type="text/css" href="tomato.css">
<% css(); %>
<script src="tomato.js"></script>

<script>

//	<% nvram("qos_enable,qos_mode,qos_classnames,web_svg"); %>

//	<% qrate(); %>

var colors = [];
for (var i = 1; i < 12; ++i)
	colors.push(getColor('.svg-color'+i));

var irates = [];
var orates = [];
var Unclassified = ['Unclassified'];
var Unused = ['Unused'];
var classNames = nvram.qos_classnames.split(' ');
var abc = Unclassified.concat(classNames, Unused);
var svgReady = 0;

var ref = new TomatoRefresh('update.cgi', 'exec=qrate', 5, 'qos_graphs');

ref.refresh = function(text) {
	irates = [];
	orates = [];
	nfmarks = [];

	for (var i = 1; i < 11; i++) {
/* DUALWAN-BEGIN */
		irates[i] = qrates1_in[i] + qrates2_in[i];
		orates[i] = qrates1_out[i] + qrates2_out[i];
/* DUALWAN-END */
/* MULTIWAN-BEGIN */
		irates[i] = qrates1_in[i] + qrates2_in[i] + qrates3_in[i] + qrates4_in[i];
		orates[i] = qrates1_out[i] + qrates2_out[i] + qrates3_out[i] + qrates4_out[i];
/* MULTIWAN-END */
	}
	try {
		eval(text);
	}
	catch (ex) {
		irates = [];
		orates = [];
	}

	showData();

	if (svgReady == 1) {
		updateCD(nfmarks, abc);
		updateBI(irates, abc);
		updateBO(orates, abc);
	}
}

function showData() {
	var i, n, p;
	var ct = 0, irt = 0, ort = 0;

	for (i = 0; i < 11; ++i) {
		ct += nfmarks[i] ? nfmarks[i] : 0;
		irt += irates[i] ? irates[i] : 0;
		ort += orates[i] ? orates[i] : 0;
	}

	for (i = 0; i < 11; ++i) {
		n = nfmarks[i];
		E('ccnt'+i).innerHTML = n;
		p = (ct > 0) ? (n / ct) * 100 : 0;
		elem.setInnerHTML('cpct'+i, p.toFixed(2)+'%');
	}

	for (i = 1; i < 11; ++i) {
		n = irates[i];
		elem.setInnerHTML('bicnt'+i, (n / 1000).toFixed(2));
		elem.setInnerHTML('bicntx'+i, (n / 8192).toFixed(2));
		p = (irt > 0) ? (n / irt) * 100 : 0;
		elem.setInnerHTML('bipct'+i, p.toFixed(2)+'%');

		n = orates[i];
		elem.setInnerHTML('bocnt'+i, (n / 1000).toFixed(2));
		elem.setInnerHTML('bocntx'+i, (n / 8192).toFixed(2));
		p = (ort > 0) ? (n / ort) * 100 : 0;
		elem.setInnerHTML('bopct'+i, p.toFixed(2)+'%');
	}

	E('ccnt-total').innerHTML = ct;
	elem.setInnerHTML('bicnt-total',(irt / 1000).toFixed(2));
	elem.setInnerHTML('bicntx-total', (irt / 8192).toFixed(2));
	elem.setInnerHTML('bocnt-total', (ort / 1000).toFixed(2));
	elem.setInnerHTML('bocntx-total', (ort / 8192).toFixed(2));
}

function earlyInit() {
	if ((nvram.qos_enable != '1') || (nvram.qos_enable == '1' && nvram.qos_mode == '2')) { /* off or cake */
		E('qosstats').style.display = 'none';
		E('qosstatsoff').style.display = 'block';

		if (nvram.qos_enable != '1')
			E('note-disabled').style.display = 'block';
		else
			E('note-cake').style.display = 'block';

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

<body>
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

<div class="section-title" id="qosstatsoff" style="display:none">View Graphs</div>
<div id="qosstats">
	<div class="section-title">Connections Distribution</div>
	<div class="section">
		<table class="qos-svg">
			<tr><td>
				<script>
					W('<table style="width:270px">');
					for (i = 0; i < 11; ++i) {
						W('<tr>'+
						  '<td class="qos-color" style="background:'+colors[i]+'">&nbsp;<\/td>'+
						  '<td class="title" style="width:45px"><a href="qos-detailed.asp?class='+i+'">'+abc[i]+'<\/a><\/td>'+
						  '<td id="ccnt'+i+'" class="qos-count" style="width:100px"><\/td>'+
						  '<td id="cpct'+i+'" class="qos-pct"><\/td><\/tr>');
					}
					W('<tr><td>&nbsp;<\/td><td class="qos-total">Total<\/td><td id="ccnt-total" class="qos-total qos-count"><\/td><td class="qos-total qos-pct">100%<\/td><\/tr>');
					W('<\/table>');
				</script>
			</td>
			<td>
				<script>
					if (nvram.web_svg != '0')
						W('<embed src="qos-graph.svg?n=0" style="width:310px;height:310px" id="svg0" type="image/svg+xml"><\/embed>');
				</script>
			</td></tr>
		</table>
	</div>

<!-- / / / -->

	<div class="section-title">Bandwidth Distribution (Inbound)</div>
	<div class="section">
		<table class="qos-svg">
			<tr><td>
				<script>
					W('<table style="width:270px">');
					W('<tr><td class="qos-color"><\/td><td class="title" style="width:45px">&nbsp;<\/td><td class="qos-thead qos-count">kbit/s<\/td><td class="qos-thead qos-count">KB/s<\/td><td class="qos-pct">&nbsp;');
					W('<\/td><\/tr>');
					for (i = 1; i < 11; ++i) {
						W('<tr>'+
						  '<td class="qos-color" style="background:'+colors[i]+'">&nbsp;<\/td>'+
						  '<td class="title" style="width:45px"><a href="qos-detailed.asp?class='+i+'">'+abc[i]+'<\/a><\/td>'+
						  '<td id="bicnt'+i+'" class="qos-count" style="width:60px"><\/td>' +
						  '<td id="bicntx'+i+'" class="qos-count" style="width:50px"><\/td>' +
						  '<td id="bipct'+i+'" class="qos-pct"><\/td><\/tr>');
					}
					W('<tr><td>&nbsp;<\/td><td class="qos-total">Total<\/td><td id="bicnt-total" class="qos-total qos-count"><\/td><td id="bicntx-total" class="qos-total qos-count"><\/td><td id="ratein" class="qos-total qos-pct">100%<\/td><\/tr>');
					W('<\/table>');
				</script>
			</td>
			<td>
				<script>
					if (nvram.web_svg != '0')
						W('<embed src="qos-graph.svg?n=1" style="width:310px;height:310px" id="svg1" type="image/svg+xml"><\/embed>');
				</script>
			</td></tr>
		</table>
	</div>

<!-- / / / -->

	<div class="section-title">Bandwidth Distribution (Outbound)</div>
	<div class="section">
		<table class="qos-svg">
			<tr><td>
				<script>
					W('<table style="width:270px">');
					W('<tr><td class="qos-color"><\/td><td class="title" style="width:45px">&nbsp;<\/td><td class="qos-thead qos-count">kbit/s<\/td><td class="qos-thead qos-count">KB/s<\/td><td class="qos-pct">&nbsp;');
					W('<\/td><\/tr>');
					for (i = 1; i < 11; ++i) {
						W('<tr>' +
						  '<td class="qos-color" style="background:'+colors[i]+'">&nbsp;<\/td>'+
						  '<td class="title" style="width:45px"><a href="qos-detailed.asp?class='+i+'">'+abc[i]+'<\/a><\/td>'+
						  '<td id="bocnt'+i+'" class="qos-count" style="width:60px"><\/td>'+
						  '<td id="bocntx'+i+'" class="qos-count" style="width:50px"><\/td>'+
						  '<td id="bopct'+i+'" class="qos-pct"><\/td><\/tr>');
					}
					W('<tr><td>&nbsp;<\/td><td class="qos-total">Total<\/td><td id="bocnt-total" class="qos-total qos-count"><\/td><td id="bocntx-total" class="qos-total qos-count"><\/td><td id="rateout" class="qos-total qos-pct">100%<\/td><\/tr>');
					W('<\/table>');
				</script>
			</td>
			<td>
				<script>
					if (nvram.web_svg != '0')
						W('<embed src="qos-graph.svg?n=2" style="width:310px;height:310px" id="svg2" type="image/svg+xml"><\/embed>');
				</script>
			</td></tr>
		</table>
	</div>

</div>

<!-- / / / -->

<div class="note-disabled" id="note-disabled" style="display:none"><b>QoS disabled.</b><br><br><a href="qos-settings.asp">Enable &raquo;</a></div>
<div class="note-disabled" id="note-cake" style="display:none"><b>Statistics not available in Cake mode.</b><br><br><a href="qos-settings.asp">Change mode &raquo;</a></div>

<!-- / / / -->

<div id="footer">
	<script>genStdRefresh(1,2,'ref.toggle()');</script>
</div>

</td></tr>
</table>
</form>
<script>earlyInit();</script>
</body>
</html>
