<!DOCTYPE html>
<!--
	Tomato64 GUI

	Bandwidth quota usage. Companion to quota-settings.asp; the counters come from
	the xt_bandwidth match via httpd's quotas.c.

	This file exists only in Tomato64 and has no upstream Tomato
	counterpart, so it carries no TOMATO64 guards.
-->
<html lang="en-GB">
<head>
<meta http-equiv="content-type" content="text/html;charset=utf-8">
<meta name="robots" content="noindex,nofollow">
<title>[<% ident(); %>] Quota Usage</title>
<link rel="stylesheet" type="text/css" href="tomato.css?rel=<% version(); %>">
<% css(); %>
<script src="tomato.js?rel=<% version(); %>"></script>

<script>

//	<% nvram("quota_enable,quota_rules,quota_path"); %>

/* the asp below emits quota_usage=[['<id>','<ip>',<bytes>], ...]; predeclare it
   so a page served before httpd has any counters still renders */
var quota_usage = [];
//	<% quotas(); %>

/* rc builds ids as quota_<id>_<d|u|c>; map them back to the rule they came
   from so usage can be shown per quota rather than per counter */
var cap_name = { d: 'Download', u: 'Upload', c: 'Combined' };

/*
 * The rule list, so usage can be labelled and limits shown alongside.
 *
 * The counter id is quota_<id>_<cap>, built from the id the rule carries in
 * its own record (field 14) - the same one rc reads. It is stable across edits
 * and reorders, so no attempt is made to reconstruct it from where the rule
 * sits in the list.
 *
 * Whether rc actually built rules for it still has to be worked out, though,
 * since a rule it skipped has no counters to show. That test does have to
 * match ipt_quotas(); keep the two in step.
 */
var valid_reset = { hour: 1, day: 1, week: 1, month: 1 };
var rules = [];
(function() {
	var raw = nvram.quota_rules.split('>');
	for (var i = 0; i < raw.length; ++i) {
		var t = raw[i].split('<');
		if (t.length != 11 && t.length != 13 && t.length != 14)
			continue;

		var r = {
			enabled: t[0], ip: t[1],
			limit: { d: t[2], u: t[3], c: t[4] },
			reset: t[5], desc: t[10],
			/* tdl/tul (t[11], t[12]) are absent on a pre-throttle record, the
			   id (t[13]) on one saved before rule ids existed - such a record
			   has no counters to look up until quota-settings.asp backfills it */
			action: t[9], tdl: t[11] || '', tul: t[12] || '', id: t[13] || ''
		};

		r.emitted = (r.enabled == '1') && valid_reset[r.reset] && (r.id != '') &&
		            (r.limit.d != '' || r.limit.u != '' || r.limit.c != '');

		rules.push(r);
	}
})();

function scopeLabel(ip) {
	switch (ip) {
	case 'ALL':                   return 'Entire Local Network';
	case 'ALL_OTHERS_INDIVIDUAL': return 'Each Host without a Quota';
	case 'ALL_OTHERS_COMBINED':   return 'All Hosts without a Quota (shared)';
	}

	if (ip.substr(0, 7) == 'SHARED:')
		return 'Shared pool: ' + ip.substr(7);

	return ip;
}

/*
 * Format a byte count, rolling up a unit at each 1024. tomato.js' scaleSize
 * only rolls over past 9999 of a unit, so a 2 GB quota would read "2048.00 MB";
 * quota usage sits in the low-GB range where GB is the natural unit, so scale
 * at the 1 GB boundary instead.
 */
function qsize(n) {
	var u = ['B', 'KB', 'MB', 'GB', 'TB'];
	var i = 0;

	n = n * 1;
	if (isNaN(n))
		return '-';
	while (n >= 1024 && i < u.length - 1) {
		n /= 1024;
		++i;
	}

	return (i == 0 ? n : comma(n.toFixed(2))) + ' ' + u[i];
}

function pct(used, limit) {
	if (limit == '' || limit * 1 == 0)
		return -1;

	return Math.min(100, Math.round((used / limit) * 100));
}

function bar(p) {
	if (p < 0)
		return '<small>no limit<\/small>';

	var cls = (p >= 100) ? 'q-over' : ((p >= 80) ? 'q-warn' : 'q-ok');

	return '<div class="q-bar"><div class="q-fill ' + cls + '" style="width:' + p + '%"><\/div><\/div>' +
	       '<small>' + p + '%<\/small>';
}

function render() {
	var i, j, k;
	var byId = {};

	/* group the flat list by counter id */
	for (i = 0; i < quota_usage.length; ++i) {
		var row = quota_usage[i];
		if (!byId[row[0]])
			byId[row[0]] = [];
		byId[row[0]].push([row[1], row[2]]);
	}

	var buf = [];
	var shown = 0;

	for (i = 0; i < rules.length; ++i) {
		var r = rules[i];
		var caps = ['d', 'u', 'c'];

		/* no rules were generated for this one, so it has no counters */
		if (!r.emitted)
			continue;

		shown++;

		/* how the rule behaves once over quota: block, or limit to its speeds */
		var overtxt = '(resets every ' + escapeHTML(r.reset) + ')';
		if (r.action == '1') {
			var dl = r.tdl != '' ? '↓' + escapeHTML(r.tdl) : '';
			var ul = r.tul != '' ? '↑' + escapeHTML(r.tul) : '';
			overtxt += ' — over quota: limit ' + dl + (dl && ul ? '/' : '') + ul + ' kbit/s';
		}

		buf.push('<div class="section-title">' +
		         (r.desc != '' ? escapeHTML(r.desc) + ' &mdash; ' : '') +
		         escapeHTML(scopeLabel(r.ip)) +
		         ' <small>' + overtxt + '<\/small><\/div>');
		buf.push('<div class="section">');

		for (k = 0; k < caps.length; ++k) {
			var cap = caps[k];
			var limit = r.limit[cap];
			if (limit == '')
				continue;

			var id = 'quota_' + r.id + '_' + cap;
			var entries = byId[id] || [];

			buf.push('<div class="q-cap"><b>' + cap_name[cap] + '<\/b> ' +
			         '<small>limit ' + qsize(limit) + '<\/small> ' +
			         '<input type="button" value="Reset" onclick="resetOne(\'' + id + '\')">' +
			         '<\/div>');

			if (entries.length == 0) {
				buf.push('<div class="q-row"><i>no usage recorded yet<\/i><\/div>');
				continue;
			}

			/* biggest consumer first */
			entries.sort(function(a, b) { return b[1] - a[1]; });

			buf.push('<table class="q-table">');
			for (j = 0; j < entries.length; ++j) {
				var p = pct(entries[j][1], limit);
				buf.push('<tr><td class="q-ip">' + escapeHTML(entries[j][0]) + '<\/td>' +
				         '<td class="q-used">' + qsize(entries[j][1]) + '<\/td>' +
				         '<td class="q-pct">' + bar(p) + '<\/td><\/tr>');
			}
			buf.push('<\/table>');
		}
		buf.push('<\/div>');
	}

	if (nvram.quota_enable == '0')
		buf = ['<div class="section"><div class="about"><b>Quotas are disabled.<\/b> Enable them on <a href="quota-settings.asp">Bandwidth Quotas<\/a>.<\/div><\/div>'];
	else if (shown == 0)
		buf = ['<div class="section"><div class="about">No quotas with limits are configured. Add one on <a href="quota-settings.asp">Bandwidth Quotas<\/a>.<\/div><\/div>'];

	E('quota-body').innerHTML = buf.join('');
}

/* asp "quotas" re-emits quota_usage=[...]; TomatoRefresh drives the polling
   and owns the refresh-time control in the footer. Default interval 1s. */
var ref = new TomatoRefresh('update.cgi', 'exec=quotas', 1, 'quota_usage_refresh');

ref.refresh = function(text) {
	try {
		eval(text);
	}
	catch (ex) {
		return;
	}
	render();
}

function resetOne(id) {
	if (!confirm('Reset the usage counter for this limit to zero?'))
		return;

	var xob = new XmlHttp();
	xob.onCompleted = function(text) {
		try {
			eval(text);
			render();
		}
		catch (ex) { }
	}
	xob.onError = function() { };
	xob.post('update.cgi', 'exec=quotas&arg0=reset&arg1=' + escapeCGI(id));
}

function resetAll() {
	if (!confirm('Reset every quota usage counter to zero?'))
		return;

	var xob = new XmlHttp();
	xob.onCompleted = function(text) {
		try {
			eval(text);
			render();
		}
		catch (ex) { }
	}
	xob.onError = function() { };
	xob.post('update.cgi', 'exec=quotas&arg0=reset&arg1=*');
}

function init() {
	render();
	/* default to refreshing every 1s, already running - a saved choice
	   (interval, or a stopped state) from a previous visit still wins */
	ref.initPage(1000, 1);
}
</script>

<style>
.q-bar { display:inline-block; width:120px; height:10px; border:1px solid #888; vertical-align:middle; margin-right:6px; }
.q-fill { height:100%; }
.q-ok { background:#4c9a4c; }
.q-warn { background:#c8a000; }
.q-over { background:#b03030; }
.q-cap { margin:6px 0 2px 0; }
.q-table { border-collapse:collapse; margin:2px 0 8px 12px; }
.q-table td { padding:1px 10px 1px 0; }
.q-ip { font-family:monospace; }
.q-used { text-align:right; }
.q-row { margin:2px 0 8px 12px; }
</style>
</head>

<body onload="init()">
<form id="t_fom" method="post" action="tomato.cgi">
<table id="container">
<tr><td colspan="2" id="header">
	<div class="title"><a href="/">Tomato64</a></div>
	<div class="version">Version <% version(); %> on <% nv("t_model_name"); %><span class="blinking bl2"><script><% anonupdate(); %> anon_update()</script>&nbsp;</span></div>
</td></tr>
<tr id="body"><td id="navi"><script>navi()</script></td>
<td id="content">
<div id="ident"><% ident(); %> | <script>wikiLink();</script></div>

<!-- / / / -->

<div id="quota-body"></div>

<!-- / / / -->

</td></tr>
<tr><td id="footer" colspan="2">
	<span id="footer-msg"></span>
	<input type="button" value="Reset All" onclick="resetAll()">
	<script>genStdRefresh(1, 1, 'ref.toggle()');</script>
	<span id="debug"></span>
</td></tr>
</table>
</form>
</body>
</html>
