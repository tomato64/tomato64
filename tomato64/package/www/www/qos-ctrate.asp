<!DOCTYPE html>
<!--
	Tomato GUI
	Copyright (C) 2006-2010 Jonathan Zarate
	http://www.polarcloud.com/tomato/

	Filtering/Extensions on this QoS/Transfer Rates page
	Copyright (C) 2011 Augusto Bott

	For use with Tomato Firmware only.
	No part of this file may be used without permission.
-->
<html lang="en-GB">
<head>
<meta http-equiv="content-type" content="text/html;charset=utf-8">
<meta name="robots" content="noindex,nofollow">
<title>[<% ident(); %>] QoS: View Per-Connection Transfer Rates</title>
<link rel="stylesheet" type="text/css" href="tomato.css?rel=<% version(); %>">
<% css(); %>
<script src="tomato.js?rel=<% version(); %>"></script>
<script src="protocols.js?rel=<% version(); %>"></script>
<script src="interfaces.js?rel=<% version(); %>"></script>
<script src="qos-conn.js?rel=<% version(); %>"></script>

<script>

//	<% nvram('lan_ipaddr,lan_netmask,t_hidelr'); %>

var cprefix = 'qos_ctrate';

var qosConnConfig = {
	src: 1,
	dst: 2,
	sport: 3,
	dport: 4,
	origin: 7,
	fixed: [],
	traffic: [5, 6]
};

initQoSConnectionNetworks();
readDelay = fixInt('<% cgi_get('delay'); %>', 2, 30, 2);

var thres = 0;

function thresChanged() {
	var a, b;

	b = E('_f_excludebythreshold').checked ? fixInt('<% cgi_get('thres'); %>', 100, 10000000, 100) : 0;
	if (b != thres) {
		thres = b;
		cookie.set(cprefix+'_thres', b);
		ref.postData = 'exec=ctrate&arg0='+readDelay+'&arg1='+thres;
		if (!ref.running)
			ref.once = 1;

		E('loading').style.display = 'block';
		ref.start();
	}
}

var grid = new TomatoGrid();
setupQoSConnectionGrid(grid);

grid.dataToView = function(data) {
	var s, v = [];
	for (var col = 0; col < data.length; ++col) {
		switch (col) {
		case 5:
		case 6:
			s = (data[col] / (readDelay * 1024)).toFixed(1);
			break;
		default:
			s = data[col];
			break;
		}
		v.push(''+s);
	}

	return v;
}

grid.sortCompare = function(a, b) {
	var obj = TGO(a);
	var col = obj.sortColumn;
	var da = a.getRowData();
	var db = b.getRowData();
	var r;

	switch (col) {
	case 2:
	case 4:
	case 5:
	case 6:
		r = cmpInt(da[col], db[col]);
		break;
	case 1:
	case 3:
		var a = fixIP(da[col]);
		var b = fixIP(db[col]);
		if ((a != null) && (b != null)) {
			r = aton(a) - aton(b);
			break;
		}
	default:
		r = cmpText(da[col], db[col]);
		break;
	}

	return obj.sortAscending ? r : -r;
}

grid.setup = function() {
	this.init('qosctrate-grid', 'sort');
	this.headerSet(['Protocol', 'Source', 'S Port', 'Destination', 'D Port', 'UL Rate', 'DL Rate']);
}

var ref = new TomatoRefresh('update.cgi', '', 0, 'qos_ctrate');

ref.refresh = function(text) {
	var data;

	++lock;

	try {
		ctrate = [];
		eval(text);
		data = ctrate;
	}
	catch (ex) {
		ctrate = [];
		data = ctrate;
	}

	refreshQoSConnections(data, qosConnConfig);
}

function dofilter() {
	updateQoSConnectionFilters(false);
}

function verifyFields(focused, quiet) {
	saveQoSConnectionFilterState();
	thresChanged();
	resolveChanged();
	dofilter();

	return 1;
}

function init() {
	restoreQoSConnectionFilterState();

	if (((thres = cookie.get(cprefix+'_thres')) == null) || (isNaN(thres *= 1)))
		thres = 0;

	E('_f_excludebythreshold').checked = (thres != 0);
	grid.setup();
	ref.postData = 'exec=ctrate&arg0='+readDelay+'&arg1='+thres;
	ref.initPage(250);

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
	<div class="title"><a href="/">Tomato64</a></div>
	<div class="version">Version <% version(); %> on <% nv("t_model_name"); %><span class="blinking bl2"><script><% anonupdate(); %> anon_update()</script>&nbsp;</span></div>
</td></tr>
<tr id="body"><td id="navi"><script>navi()</script></td>
<td id="content">
<div id="ident"><% ident(); %> | <script>wikiLink();</script></div>

<!-- / / / -->

<div class="section-title" id="stitle" onclick='document.location="qos-graphs.asp"' style="cursor:pointer">Transfer Rates: <span id="qos_numtotalconn"></span></div>
<div class="section">
	<div class="tomato-grid" id="qosctrate-grid"></div>

	<div id="loading">Loading...</div>
</div>

<!-- / / / -->

<script>writeToggleSectionTitle('Filters:', 'filters');</script>
<div class="section" id="sesdiv_filters" style="display:none">
	<script>writeQoSConnectionFilters(true);</script>
</div>

<!-- / / / -->

<div id="footer">
	<script>genStdRefresh(1,1,'ref.toggle()');</script>
</div>

</td></tr>
</table>
</form>
<script>insOvl()</script>
</body>
</html>
