<!DOCTYPE html>
<!--
	Tomato GUI
	Copyright (C) 2006-2010 Jonathan Zarate
	http://www.polarcloud.com/tomato/

	Filtering/Extensions on this QoS/Connection Details page
	Copyright (C) 2011 Augusto Bott
	http://code.google.com/p/tomato-sdhc-vlan/

	For use with Tomato Firmware only.
	No part of this file may be used without permission.
-->
<html lang="en-GB">
<head>
<meta http-equiv="content-type" content="text/html;charset=utf-8">
<meta name="robots" content="noindex,nofollow">
<title>[<% ident(); %>] QoS: View Details</title>
<link rel="stylesheet" type="text/css" href="tomato.css?rel=<% version(); %>">
<% css(); %>
<script src="tomato.js?rel=<% version(); %>"></script>
<script src="protocols.js?rel=<% version(); %>"></script>
<script src="interfaces.js?rel=<% version(); %>"></script>
<script src="qos-conn.js?rel=<% version(); %>"></script>

<script>

//	<% nvram('qos_enable,qos_classnames,lan_ipaddr,lan_netmask,t_hidelr'); %>

var cprefix = 'qos_detailed';
var Unclassified = ['Unclassified'];
var classNames = nvram.qos_classnames.split(' ');
var abc = Unclassified.concat(classNames);

var qosConnConfig = {
	src: 2,
	dst: 3,
	sport: 4,
	dport: 5,
	origin: 10,
	fixed: [8, 9],
	traffic: [6, 7]
};

initQoSConnectionNetworks();

var colors = ['F08080','E6E6FA','0066CC','8FBC8F','FAFAD2','ADD8E6','9ACD32','E0FFFF','90EE90','FF9933','FFF0F5'];
if ((viewClass = '<% cgi_get("class"); %>') == '')
	viewClass = -1;
else if ((isNaN(viewClass *= 1)) || (viewClass < 0) || (viewClass > 10))
	viewClass = 0;

var grid = new TomatoGrid();
setupQoSConnectionGrid(grid);

grid.dataToView = function(data) {
	var s, v = [];
	for (var col = 0; col < data.length; ++col) {
		switch (col) {
			case 5:		/* Class */
				s = abc[data[col]] || (''+data[col]);
			break;
			case 6:		/* Rule # */
				s = (data[col] * 1 > 0) ? (''+data[col]) : '';
			break;
			case 7:		/* Bytes out */
			case 8:		/* Bytes in */
				s = scaleSize(data[col] * 1);
			break;
			default:
				s = ''+data[col];
			break;
		}
		v.push(s);
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
	case 2:		/* S port */
	case 4:		/* D port */
	case 6:		/* Rule # */
	case 7:		/* Bytes out */
	case 8:		/* Bytes in */
		r = cmpInt(da[col], db[col]);
		break;
	case 5:		/* Class */
		r = cmpInt(da[col] ? da[col] : 10000, db[col] ? db[col] : 10000);
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
	this.init('qos-det-grid', 'sort');
	this.headerSet(['Protocol', 'Source', 'S Port', 'Destination', 'D Port', 'Class', 'Rule', 'Bytes Out', 'Bytes In']);
}

var ref = new TomatoRefresh('update.cgi', '', 0, 'qos_detailed');

ref.refresh = function(text) {
	var data;

	++lock;

	try {
		ctdump = [];
		eval(text);
		data = ctdump;
	}
	catch (ex) {
		ctdump = [];
		data = ctdump;
	}

	refreshQoSConnections(data, qosConnConfig);
}

function dofilter() {
	updateQoSConnectionFilters(true);
}

function verifyFields(focused, quiet) {
	saveQoSConnectionFilterState();
	dofilter();
	resolveChanged();

	return 1;
}


function init() {
	restoreQoSConnectionFilterState();

	if (viewClass != -1)
		E('stitle').firstChild.data = 'View Details: '+abc[viewClass]+' ';

	grid.setup();
	ref.postData = 'exec=ctdump&arg0='+viewClass;
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

<div class="section-title" id="stitle" onclick='document.location="qos-graphs.asp"' style="cursor:pointer">View Details: <span id="qos_numtotalconn"></span></div>
<div class="section" id="grid-head">
	<div class="tomato-grid" id="qos-det-grid"></div>

	<div id="loading">Loading...</div>
</div>

<!-- / / / -->

<script>writeToggleSectionTitle('Filters:', 'filters', 'filters-head');</script>
<div class="section" id="sesdiv_filters" style="display:none">
	<script>writeQoSConnectionFilters(false);</script>
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
