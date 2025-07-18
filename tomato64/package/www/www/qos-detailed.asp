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

<script>

//	<% nvram('qos_enable,qos_classnames,lan_ipaddr,lan1_ipaddr,lan2_ipaddr,lan3_ipaddr,lan_netmask,lan1_netmask,lan2_netmask,lan3_netmask,t_hidelr'); %>

var cprefix = 'qos_detailed';
var Unclassified = ['Unclassified'];
var classNames = nvram.qos_classnames.split(' ');
var abc = Unclassified.concat(classNames);

var colors = ['F08080','E6E6FA','0066CC','8FBC8F','FAFAD2','ADD8E6','9ACD32','E0FFFF','90EE90','FF9933','FFF0F5'];
var filterip = [];
var filteripe = [];

if ((viewClass = '<% cgi_get("class"); %>') == '')
	viewClass = -1;
else if ((isNaN(viewClass *= 1)) || (viewClass < 0) || (viewClass > 10))
	viewClass = 0;

var queue = [];
var xob = null;
var cache = [];
var lock = 0;

function resolve() {
	if ((queue.length == 0) || (xob))
		return;

	xob = new XmlHttp();
	xob.onCompleted = function(text, xml) {
		eval(text);
		for (var i = 0; i < resolve_data.length; ++i) {
			var r = resolve_data[i];
			if (r[1] == '')
				r[1] = r[0];

			cache[r[0]] = r[1];
			if (lock == 0)
				grid.setName(r[0], r[1]);
		}
		if (queue.length == 0) {
			if ((lock == 0) && (resolveCB) && (grid.sortColumn == 4))
				grid.resort();
		}
		else
			setTimeout(resolve, 500);

		xob = null;
	}
	xob.onError = function(ex) {
		xob = null;
	}

	xob.post('resolve.cgi', 'ip='+queue.splice(0, 20).join(','));
}

var resolveCB = 0;
var bcastCB = 0;
var mcastCB = 0;

function resolveChanged() {
	var b;

	b = E('_f_autoresolve').checked ? 1 : 0;
	if (b != resolveCB) {
		resolveCB = b;
		cookie.set(cprefix+'_resolve', b);
	}
	if (b)
		grid.resolveAll();
}

var grid = new TomatoGrid();

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

grid.onClick = function(cell) {
	var row = PR(cell);
	var ip = row.getRowData()[3];
	if (this.lastClicked != row) {
		this.lastClicked = row;
		if (ip.indexOf('<') == -1) {
			queue.push(ip);
			row.style.cursor = 'wait';
			resolve();
		}
	}
	else
		this.resolveAll();
}

grid.resolveAll = function() {
	var i, ip, row, q, cols, j;

	q = [];
	cols = [1, 3];
	for (i = 1; i < this.tb.rows.length; ++i) {
		row = this.tb.rows[i];
		for (j = cols.length-1; j >= 0; j--) {
			ip = row.getRowData()[cols[j]];
			if (ip.indexOf('<') == -1) {
				if (!q[ip]) {
					q[ip] = 1;
					queue.push(ip);
				}
				row.style.cursor = 'wait';
			}
		}
	}
	q = null;
	resolve();
}

grid.setName = function(ip, name) {
	var i, row, data, cols, j;

	cols = [1, 3];
	for (i = this.tb.rows.length - 1; i > 0; --i) {
		row = this.tb.rows[i];
		data = row.getRowData();
		for (j = cols.length-1; j >= 0; j--) {
			if (data[cols[j]].indexOf(ip) != -1 ) {
				data[cols[j]] = name+((ip.indexOf(':') != -1) ? '<br>' : ' ')+'<small>('+ip+')<\/small>';
				row.setRowData(data);
				if (E('_f_shortcuts').checked)
					data[cols[j]] = data[cols[j]]+' <small class="pics"><a href="javascript:addExcludeList(\''+ip+'\')" title="Filter out this IP">[hide]<\/a><\/small>';

				row.cells[cols[j]].innerHTML = data[cols[j]];
				row.style.cursor = 'default';
			}
		}
	}
}

grid.setup = function() {
	this.init('qos-det-grid', 'sort');
	this.headerSet(['Protocol', 'Source', 'S Port', 'Destination', 'D Port', 'Class', 'Rule', 'Bytes Out', 'Bytes In']);
}

var ref = new TomatoRefresh('update.cgi', '', 0, 'qos_detailed');

var numconntotal = 0;
var numconnshown = 0;

ref.refresh = function(text) {
	var i, b, d, cols, j;

	++lock;

	numconntotal = 0;
	numconnshown = 0;

	try {
		ctdump = [];
		eval(text);
	}
	catch (ex) {
		ctdump = [];
	}

	grid.lastClicked = null;
	grid.removeAllData();

	var c = [];
	var q = [];
	var cursor;
	var ip;

	var fskip;

	cols = [2, 3];

	for (i = 0; i < ctdump.length; ++i) {
		fskip=0;
		numconntotal++;
		b = ctdump[i];

		if (E('_f_excludegw').checked) {
			if ((b[2] == nvram.lan_ipaddr) || (b[3] == nvram.lan_ipaddr) ||
				(b[2] == nvram.lan1_ipaddr) || (b[3] == nvram.lan1_ipaddr) ||
				(b[2] == nvram.lan2_ipaddr) || (b[3] == nvram.lan2_ipaddr) ||
				(b[2] == nvram.lan3_ipaddr) || (b[3] == nvram.lan3_ipaddr) ||
				(b[2] == '127.0.0.1') || (b[3] == '127.0.0.1'))
				continue;
		}

		if (E('_f_excludebcast').checked) {
			if ((b[3] == getBroadcastAddress(getNetworkAddress(nvram.lan_ipaddr,nvram.lan_netmask),nvram.lan_netmask)) ||
				(b[3] == getBroadcastAddress(getNetworkAddress(nvram.lan1_ipaddr,nvram.lan1_netmask),nvram.lan1_netmask)) ||
				(b[3] == getBroadcastAddress(getNetworkAddress(nvram.lan2_ipaddr,nvram.lan2_netmask),nvram.lan2_netmask)) ||
				(b[3] == getBroadcastAddress(getNetworkAddress(nvram.lan3_ipaddr,nvram.lan3_netmask),nvram.lan3_netmask)) ||
				(b[3] == '255.255.255.255') || (b[3] == '0.0.0.0'))
				continue;
		}

		if (E('_f_excludemcast').checked) {
			var mmin = 3758096384;	/* aton('224.0.0.0') */
			var mmax = 4026531839;	/* aton('239.255.255.255') */
			if (((aton(b[2]) >= mmin) && (aton(b[2]) <= mmax)) || ((aton(b[3]) >= mmin) && (aton(b[3]) <= mmax)))
				continue;
		}

		if (filteripe.length>0) {
			fskip = 0;
			for (x = 0; x < filteripe.length; ++x) {
				if ((b[2] == filteripe[x]) || (b[3] == filteripe[x])) {
					fskip=1;
					break;
				}
			}
			if (fskip == 1)
				continue;
		}

		if (filterip.length>0) {
			fskip = 1;
			for (x = 0; x < filterip.length; ++x) {
				if ((b[2] == filterip[x]) || (b[3] == filterip[x])) {
					fskip=0;
					break;
				}
			}
			if (fskip == 1)
				continue;
		}

		for (j = cols.length - 1; j >= 0; j--) {
			ip = b[cols[j]];
			if (cache[ip] != null) {
				c[ip] = cache[ip];
				b[cols[j]] = cache[ip]+((ip.indexOf(':') != -1) ? '<br>' : ' ')+'<small>('+ip+')<\/small>';
				cursor = 'default';
			}
			else {
				if (resolveCB) {
					if (!q[ip]) {
						q[ip] = 1;
						queue.push(ip);
					}
					cursor = 'wait';
				}
				else
					cursor = null;
			}
			if (E('_f_bold').checked) {
				if ((b[10] == '0' && cols[j] == '2')  || (b[10] == '1' && cols[j] == '3')) {
					b[cols[j]] = '<b>' +b[cols[j]]+ '<\/b>';
				}
			}
			if (E('_f_shortcuts').checked) {
				b[cols[j]] = b[cols[j]]+' <small class="pics">';
				if (cache[ip] == null)
					b[cols[j]] = b[cols[j]]+'<a href="javascript:addToResolveQueue(\''+ip+'\')" title="Resolve the hostname of this address">[resolve]<\/a>';

				b[cols[j]] = b[cols[j]]+' <a href="javascript:addExcludeList(\''+ip+'\')" title="Filter out this IP">[hide]<\/a><\/small>';
			}
		}

		numconnshown++;

		if ((E('_f_originsource').checked) && b[10] == '1') {
			d = [protocols[b[0]] || b[0], b[3], b[5], b[2], b[4], b[8], b[9], b[7], b[6]];
		}
		else {
			d = [protocols[b[0]] || b[0], b[2], b[4], b[3], b[5], b[8], b[9], b[6], b[7]];
		}

		var row = grid.insertData(-1, d);
		if (cursor)
			row.style.cursor = cursor;
	}
	cache = c;
	c = null;
	q = null;

	grid.resort();
	setTimeout(function() { E('loading').style.display = 'none'; }, 100);

	--lock;

	if (resolveCB)
		resolve();

	if (numconnshown != numconntotal)
		E('qos_numtotalconn').innerHTML='(showing '+numconnshown+' out of '+numconntotal+' connections)';
	else
		E('qos_numtotalconn').innerHTML='('+numconntotal+' connections)';
}

function addExcludeList(address) {
	if (E('_f_filter_ipe').value.length < 6)
		E('_f_filter_ipe').value = address;
	else {
		if (E('_f_filter_ipe').value.indexOf(address) < 0)
			E('_f_filter_ipe').value = E('_f_filter_ipe').value+','+address;
	}
	dofilter();
}

function addToResolveQueue(ip) {
	queue.push(ip);
	resolve();
}

function dofilter() {
	if (E('_f_filter_ip').value.length > 6)
		filterip = E('_f_filter_ip').value.split(',');
	else
		filterip = [];

	if (E('_f_filter_ipe').value.length > 6)
		filteripe = E('_f_filter_ipe').value.split(',');
	else
		filteripe = [];

	if (!ref.running)
		ref.once = 1;
	ref.start();
}

function verifyFields(focused, quiet) {
	var b;

	b = E('_f_excludebcast').checked ? 1 : 0;
	if (b != bcastCB) {
		bcastCB = b;
		cookie.set(cprefix+'_bcast', b);
	}

	b = E('_f_excludemcast').checked ? 1 : 0;
	if (b != mcastCB) {
		mcastCB = b;
		cookie.set(cprefix+'_mcast', b);
	}

	cookie.set(cprefix+'_shortcuts', (E('_f_shortcuts').checked ? '1' : '0'), 1);

	cookie.set(cprefix+'_bold', (E('_f_bold').checked ? '1' : '0'), 1);

	cookie.set(cprefix+'_originsource', (E('_f_originsource').checked ? '1' : '0'), 1);

	dofilter();
	resolveChanged();

	return 1;
}


function init() {
	var c;

	if ((c = cookie.get(cprefix+'_filterip')) != null) {
		cookie.set(cprefix+'_filterip', '', 0);
		if (c.length>6) {
			E('_f_filter_ip').value = c;
			filterip = c.split(',');
		}
	}

	if (((c = cookie.get(cprefix+'_resolve')) != null) && (c == '1'))
		E('_f_autoresolve').checked = resolveCB = 1;

	if (((c = cookie.get(cprefix+'_bcast')) != null) && (c == '1'))
		E('_f_excludebcast').checked = bcastCB = 1;

	if (((c = cookie.get(cprefix+'_mcast')) != null) && (c == '1'))
		E('_f_excludemcast').checked = mcastCB = 1;

	if (((c = cookie.get(cprefix+'_filters_vis')) != null) && (c == '1'))
		toggleVisibility(cprefix, 'filters');

	if (viewClass != -1)
		E('stitle').firstChild.data = 'View Details: '+abc[viewClass]+' ';

	E('_f_shortcuts').checked = (((c = cookie.get(cprefix+'_shortcuts')) != null) && (c == '1'));

	E('_f_bold').checked = (((c = cookie.get(cprefix+'_bold')) != null) && (c == '1'));

	E('_f_originsource').checked = (((c = cookie.get(cprefix+'_originsource')) != null) && (c == '1'));

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
	<div class="version">Version <% version(); %> on <% nv("t_model_name"); %></div>
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

<div class="section-title" id="filters-head">Filters: <small><i><a href="javascript:toggleVisibility(cprefix,'filters');" id="toggleLink-filters"><span id="sesdiv_filters_showhide">(Show)</span></a></i></small></div>
<div class="section" id="sesdiv_filters" style="display:none">
	<script>
		var c;
		c = [];
		c.push({ title: 'Show only these IPs', name: 'f_filter_ip', size: 50, maxlen: 255, type: 'text', suffix: ' <small>(Comma separated list)<\/small>' });
		c.push({ title: 'Exclude these IPs', name: 'f_filter_ipe', size: 50, maxlen: 255, type: 'text', suffix: ' <small>(Comma separated list)<\/small>' });
		c.push({ title: 'Exclude gateway traffic', name: 'f_excludegw', type: 'checkbox', value: ((nvram.t_hidelr) == '1' ? 1 : 0) });
		c.push({ title: 'Exclude IPv4 broadcast', name: 'f_excludebcast', type: 'checkbox' });
		c.push({ title: 'Exclude IPv4 multicast', name: 'f_excludemcast', type: 'checkbox' });
		c.push({ title: 'Auto resolve addresses', name: 'f_autoresolve', type: 'checkbox' });
		c.push({ title: 'Show shortcuts', name: 'f_shortcuts', type: 'checkbox' });
		c.push({ title: 'Bold connection originator', name: 'f_bold', type: 'checkbox' });
		c.push({ title: 'Show connection originator always as Source', name: 'f_originsource', type: 'checkbox' });
		createFieldTable('',c);
	</script>
</div>

<!-- / / / -->

<div id="footer">
	<script>genStdRefresh(1,1,'ref.toggle()');</script>
</div>

</td></tr>
</table>
</form>
</body>
</html>
