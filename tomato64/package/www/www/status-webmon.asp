<!DOCTYPE html>
<!--
	Tomato GUI

	For use with Tomato Firmware only.
	No part of this file may be used without permission.
-->
<html lang="en-GB">
<head>
<meta http-equiv="content-type" content="text/html;charset=utf-8">
<meta name="robots" content="noindex,nofollow">
<title>[<% ident(); %>] Status: Web Usage</title>
<link rel="stylesheet" type="text/css" href="tomato.css">
<% css(); %>
<script src="tomato.js"></script>

<script>

//	<% nvram("log_wm,log_wmdmax,log_wmsmax"); %>

list = [];
wm_domains = [];
wm_searches = [];

var maxLimit = 0;
var maxCount = 50;
var lastMaxCount = -1;

var queue = [];
var xob = null;
var cache = [];
var new_cache = [];
var lock = 0;

function clearLog(clear) {
	if (xob) return;

	xob = new XmlHttp();
	xob.onCompleted = function(text, xml) {
		xob = null;
		E('clear' + clear).innerHTML = '&raquo; <a href="javascript:clearLog(' + clear + ')">Clear<\/a>';
		if (!ref.running) ref.once = 1;
		ref.start();
	}
	xob.onError = function(ex) {
		xob = null;
	}

	xob.post('webmon.cgi', 'clear=' + clear);
	E('clear' + clear).innerHTML = 'Please wait... <img src="spin.gif" style="vertical-align:top">';
}

function resolve() {
	if ((queue.length == 0) || (xob)) return;

	xob = new XmlHttp();
	xob.onCompleted = function(text, xml) {
		eval(text);
		for (var i = 0; i < resolve_data.length; ++i) {
			var r = resolve_data[i];
			if (r[1] == '') r[1] = r[0];
			cache[r[0]] = r[1];
			if (lock == 0) {
				dg.setName(r[0], r[1]);
				sg.setName(r[0], r[1]);
			}
		}
		xob = null;
	}
	xob.onError = function(ex) {
		xob = null;
	}

	xob.post('resolve.cgi', 'ip=' + queue.splice(0, 20).join(','));
}

var ref = new TomatoRefresh('update.cgi', '', 0, 'status_webmon');

ref.refresh = function(text) {
	++lock;

	try {
		eval(text);
	}
	catch (ex) {
		wm_domains = [];
		wm_searches = [];
	}

	new_cache = [];
	dg.populate();
	sg.populate();
	cache = new_cache;
	new_cache = null;

	--lock;
}

function showSelectedOption(prev, curr) {
	var e;

	/* safe if prev doesn't exist */
	elem.removeClass('mc' + prev, 'selected');
	if ((e = E('mc' + curr)) != null) {
		elem.addClass(e, 'selected');
		e.blur();
	}
}

function showMaxCount() {
	if (maxCount == lastMaxCount) return;
	showSelectedOption(lastMaxCount, maxCount);
	lastMaxCount = maxCount;
	ref.postData = 'exec=webmon&arg0=' + maxCount;
}

function switchMaxCount(c) {
	maxCount = c;
	showMaxCount();
	if (!ref.running) ref.once = 1;
	ref.start();
	cookie.set('webmon-maxcount', maxCount);
}

function WMGrid() { return this; }
WMGrid.prototype = new TomatoGrid;

WMGrid.prototype.resolveAll = function() {
	var i, ip, row, q;

	q = [];
	queue = [];
	for (i = 1; i < this.tb.rows.length; ++i) {
		row = this.tb.rows[i];
		ip = row.getRowData().ip;
		if (ip.indexOf('<') == -1) {
			if (!q[ip]) {
				q[ip] = 1;
				queue.push(ip);
			}
			row.style.cursor = 'wait';
		}
	}
	q = null;
	resolve();
}

WMGrid.prototype.onClick = function(cell) {
	if ((cell.cellIndex || 0) == 2 /* url */) return;

	var row = PR(cell);
	var ip = row.getRowData().ip;
	if (this.lastClicked != row) {
		this.lastClicked = row;
		queue = [];
		if (ip.indexOf('<') == -1) {
			queue.push(ip);
			row.style.cursor = 'wait';
			resolve();
		}
	}
	else {
		this.resolveAll();
	}
}

WMGrid.prototype.setName = function(ip, name) {
	var i, row, data;

	for (i = this.tb.rows.length - 1; i > 0; --i) {
		row = this.tb.rows[i];
		data = row.getRowData();
		if (data.ip == ip) {
			data[1] = name + ((ip.indexOf(':') != -1) ? '<br>' : ' ') + '<small>(' + ip + ')<\/small>';
			row.setRowData(data);
			row.cells[1].innerHTML = data[1];
			row.style.cursor = 'default';
		}
	}
}

WMGrid.prototype.populateData = function(data, url) {
	var a, e, i;
	var maxl = 45;
	var cursor;

	list = [];
	this.lastClicked = null;
	this.removeAllData();
	for (i = 0; i < list.length; ++i) {
		list[i].time = 0;
		list[i].ip = '';
		list[i].value = '';
	}

	for (i = data.length - 1; i >= 0; --i) {
		a = data[i];
		e = {
			time: a[0],
			ip: a[1],
			value: a[2] + ''
		};
		list.push(e);
	}

	var dt = new Date();
	for (i = list.length - 1; i >= 0; --i) {
		e = list[i];
/* IPV6-BEGIN */
		a = CompressIPv6Address(e.ip);
		if (a != null) e.ip = a;
/* IPV6-END */
		if (cache[e.ip] != null) {
			new_cache[e.ip] = cache[e.ip];
			e.ip = cache[e.ip] + ((e.ip.indexOf(':') != -1) ? '<br>' : ' ') + '<small>(' + e.ip + ')<\/small>';
			cursor = 'default';
		}
		else
			cursor = null;

		if (url != 0) {
			e.value = '<a href="http://' + e.value + '" class="new_window">' +
				(e.value.length > maxl + 3 ? e.value.substr(0, maxl) + '...' : e.value) + '<\/a>';
		}
		else {
			e.value = e.value.replace(/\+/g, ' ');
			if (e.value.length > maxl + 3)
				e.value = e.value.substr(0, maxl) + '...';
		}
		dt.setTime(e.time * 1000);
		var row = this.insert(-1, e, [dt.toDateString() + ', ' + dt.toLocaleTimeString(),
			e.ip, e.value], false);
		if (cursor) row.style.cursor = cursor;
	}

	list = [];
	this.resort();
	this.recolor();
}

WMGrid.prototype.sortCompare = function(a, b) {
	var col = this.sortColumn;
	var ra = a.getRowData();
	var rb = b.getRowData();
	var r;

	switch (col) {
	case 0:
		r = -cmpInt(ra.time, rb.time);
		break;
	case 1:
		var aip = fixIP(ra.ip);
		var bip = fixIP(rb.ip);
		if ((aip != null) && (bip != null)) {
			r = aton(aip) - aton(bip);
			break;
		}
		// fall
	default:
		r = cmpText(a.cells[col].innerHTML, b.cells[col].innerHTML);
	}

	return this.sortAscending ? -r : r;
}

var dg = new WMGrid();

dg.setup = function() {
	this.init('dom-grid', 'sort');
	this.headerSet(['Last Access Time', 'IP Address', 'Domain Name']);
	this.sort(0);
}

dg.populate = function() {
	this.populateData(wm_domains, 1);
}

var sg = new WMGrid();

sg.setup = function() {
	this.init('srh-grid', 'sort');
	this.headerSet(['Search Time', 'IP Address', 'Search Criteria']);
	this.sort(0);
}

sg.populate = function() {
	this.populateData(wm_searches, 0);
}

var observer = window.MutationObserver || window.WebKitMutationObserver || window.MozMutationObserver;

function init() {
	if (observer)
		new observer(eventHandler).observe(E("dom-grid"), { childList: true, subtree: true });

	ref.initPage();

	if (!ref.running) ref.once = 1;
	ref.start();
}

function earlyInit() {
	if (nvram.log_wm == '1' && (nvram.log_wmdmax != '0' || nvram.log_wmsmax != '0')) {
		E('webmon-grids').style.display = 'block';
		E('webmon-refresh').style.display = 'inline-block';
		E('webmon-off').style.display = 'none';

		maxLimit = nvram.log_wmdmax * 1;
		if (nvram.log_wmsmax * 1 > maxLimit) maxLimit = nvram.log_wmsmax * 1;
		if (maxLimit <= 10)
			E('webmon-mc').style.display = 'none';
		else {
			if (maxLimit <= 50) E('mc50').style.display = 'none';
			if (maxLimit <= 100) E('mc100').style.display = 'none';
			if (maxLimit <= 200) E('mc200').style.display = 'none';
			if (maxLimit <= 500) E('mc500').style.display = 'none';
			if (maxLimit <= 1000) E('mc1000').style.display = 'none';
			if (maxLimit <= 2000) E('mc2000').style.display = 'none';
			if (maxLimit <= 5000) E('mc5000').style.display = 'none';
		}

		if (nvram.log_wmdmax == '0') E('webmon-domains').style.display = 'none';
		if (nvram.log_wmsmax == '0') E('webmon-searches').style.display = 'none';
	}
	else {
		E('note-disabled').style.display = 'block';
		return;
	}
	dg.setup();
	sg.setup();

	maxCount = fixInt(cookie.get('webmon-maxcount'), 0, maxLimit, 50);
	showMaxCount();
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

<div class="section-title" id="webmon-off">Web Usage</div>
<div id="webmon-grids" style="display:none">
	<div id="webmon-domains">
		<div class="section-title">Recently Visited Web Sites</div>
		<div class="section">
			<div class="tomato-grid" id="dom-grid"></div>

			<div id="clear1" class="webmon-clear">&raquo; <a href="javascript:clearLog(1)">Clear</a></div>

			<div class="webmon-dl">&raquo; <a href="webmon_recent_domains?_http_id=<% nv(http_id) %>">Download</a></div>

		</div>
	</div>

	<div id="webmon-searches">
		<div class="section-title">Recent Web Searches</div>
		<div class="section">
			<div class="tomato-grid" id="srh-grid"></div>

			<div id="clear2" class="webmon-clear">&raquo; <a href="javascript:clearLog(2)">Clear</a></div>

			<div class="webmon-dl">&raquo; <a href="webmon_recent_searches?_http_id=<% nv(http_id) %>">Download</a></div>

		</div>
	</div>

	<div class="section" id="webmon-controls">
		<div id="webmon-mc">
			Show up to&nbsp;
			<a href="javascript:switchMaxCount(10);" id="mc10">10</a>
			<a href="javascript:switchMaxCount(50);" id="mc50">50</a>
			<a href="javascript:switchMaxCount(100);" id="mc100">100</a>
			<a href="javascript:switchMaxCount(200);" id="mc200">200</a>
			<a href="javascript:switchMaxCount(500);" id="mc500">500</a>
			<a href="javascript:switchMaxCount(1000);" id="mc1000">1000</a>
			<a href="javascript:switchMaxCount(2000);" id="mc2000">2000</a>
			<a href="javascript:switchMaxCount(5000);" id="mc5000">5000</a>
			<a href="javascript:switchMaxCount(0);" id="mc0">All</a>&nbsp;
			<small>available entries</small>
		</div>

		<div id="webmon-config">&raquo; <a href="admin-log.asp">Web Monitor Configuration</a></div>
	</div>
</div>

<!-- / / / -->

<div class="note-disabled" id="note-disabled" style="display:none"><b>Web Monitoring disabled.</b><br><br><a href="admin-log.asp">Enable &raquo;</a></div>

<!-- / / / -->

<div id="footer">
	<div id="webmon-refresh" style="display:none"><script>genStdRefresh(1,3,'ref.toggle()')</script></div>
</div>

</td></tr>
</table>
</form>
<script>earlyInit()</script>
</body>
</html>
