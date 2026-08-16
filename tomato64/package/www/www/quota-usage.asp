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
			/* rday/rhour qualify the reset interval, active is the time window;
			   all three sit inside the original 11 fields, so even the oldest
			   record carries them */
			reset: t[5], rday: t[6], rhour: t[7], active: t[8], desc: t[10],
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

/*
 * The bar and its percentage live in separate cells of a fixed-layout table, so
 * they line up down the page instead of starting wherever the host and byte
 * count happened to end. The bar fills its cell rather than carrying its own
 * width, which is what keeps every rule's bars the same length.
 */
function bar(p) {
	if (p < 0)
		return '';

	var cls = (p >= 100) ? 'q-over' : ((p >= 80) ? 'q-warn' : 'q-ok');

	return '<div class="q-bar"><div class="q-fill ' + cls + '" style="width:' + p + '%"><\/div><\/div>';
}

function pctText(p) {
	return (p < 0) ? '<small>no limit<\/small>' : (p + '%');
}

var quota_dow = ['Sunday','Monday','Tuesday','Wednesday','Thursday','Friday','Saturday'];

function hourLabel(h) {
	h = h * 1;
	if (isNaN(h) || h < 0 || h > 23)
		h = 0;

	return (h < 10 ? '0' : '') + h + ':00';
}

/*
 * When the counter rolls over, spelling out only the parts rc actually reads -
 * see quota_reset_args(): hourly ignores both the day and the hour, daily uses
 * the hour, weekly and monthly use both. The clamping matches it too, so a
 * hand-edited record reads the way it will behave.
 *
 * Returns the value only; the "Resets" label is a column of its own.
 */
function resetLabel(r) {
	var d;

	switch (r.reset) {
	case 'hour':
		return 'hourly';
	case 'day':
		return 'daily at ' + hourLabel(r.rhour);
	case 'week':
		d = r.rday * 1;
		if (isNaN(d) || d < 0 || d > 6)
			d = 0;

		return 'weekly on ' + quota_dow[d] + ' at ' + hourLabel(r.rhour);
	case 'month':
		d = r.rday * 1;
		if (isNaN(d) || d < 1 || d > 31)
			d = 1;

		return 'monthly on day ' + d + ' at ' + hourLabel(r.rhour);
	}

	return 'every ' + r.reset;
}

/*
 * The active window - field 8, "<mode>|<hours>|<weekdays>|<weekly_ranges>". Empty
 * means always, which is the normal case and gets no row of its own; a window
 * very much does, since it is the reason a counter can sit still while traffic
 * flows. See quota_active_args() in rc/quotas.c.
 *
 * Returns the value only; the "Active" label is a column of its own.
 */
function activeLabel(a) {
	var t = (a || '').split('|');
	var mode = t[0] || '';

	if (mode != 'only' && mode != 'except')
		return '';

	var hours = t[1] || '';
	var days = t[2] || '';
	var weekly = t[3] || '';
	var what = weekly;
	if (what == '') {
		what = days;
		if (hours != '')
			what = (what == '') ? hours : (what + ' ' + hours);
	}
	if (what == '')
		return '';

	return (mode == 'only' ? 'only ' : 'except ') + what;
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

		/* the title carries the identity only - everything else is a fact row
		   inside the table below, so it lines up instead of trailing a
		   description whose length varies from rule to rule */
		buf.push('<div class="section-title">' +
		         (r.desc != '' ? escapeHTML(r.desc) + ' &mdash; ' : '') +
		         escapeHTML(scopeLabel(r.ip)) + '<\/div>');
		buf.push('<div class="section">');

		/*
		 * One fixed-layout table per rule, so the fact rows, the cap headings
		 * and every host row share the same columns - and because the widths
		 * are fixed rather than content-driven, they line up from rule to rule
		 * as well.
		 */
		buf.push('<table class="q-table">' +
		         '<col class="q-c-host"><col class="q-c-used"><col class="q-c-bar"><col class="q-c-pct">');

		/*
		 * The facts that decide when the counters move and what happens when
		 * they run out. Each is only shown when it says something: an always-on
		 * quota has no window to report, and the over-quota speeds only exist
		 * for the Limit action.
		 *
		 * The name takes the first two columns and the value the last two, so
		 * every value starts exactly where the Reset button and the bars below
		 * it do. A long value simply runs past the end of the table rather than
		 * wrapping inside its cell (see .q-fval) - nothing sits to the right of
		 * the table, so there is nothing for it to collide with.
		 */
		var facts = [['Resets', escapeHTML(resetLabel(r))]];
		var act = activeLabel(r.active);
		if (act != '')
			facts.push(['Active', escapeHTML(act)]);
		if (r.action == '1') {
			var dl = r.tdl != '' ? '↓' + escapeHTML(r.tdl) : '';
			var ul = r.tul != '' ? '↑' + escapeHTML(r.tul) : '';
			facts.push(['Over quota', 'limit ' + dl + (dl && ul ? '/' : '') + ul + ' kbit/s']);
		}
		else
			facts.push(['Over quota', 'block']);

		for (k = 0; k < facts.length; ++k)
			buf.push('<tr class="q-fact">' +
			         '<td class="q-fname" colspan="2">' + facts[k][0] + '<\/td>' +
			         '<td class="q-fval" colspan="2">' + facts[k][1] + '<\/td><\/tr>');

		for (k = 0; k < caps.length; ++k) {
			var cap = caps[k];
			var limit = r.limit[cap];
			if (limit == '')
				continue;

			var id = 'quota_' + r.id + '_' + cap;
			var entries = byId[id] || [];

			/*
			 * The cap heading uses the grid too: the name and its limit share
			 * the first two columns, and the Reset button sits in the bar
			 * column so it lands directly above the bars it clears. Spanning
			 * two columns rather than one keeps "limit 512.00 MB" from
			 * overflowing a fixed 7em cell.
			 */
			buf.push('<tr class="q-caprow">' +
			         '<td colspan="2"><b>' + cap_name[cap] + '<\/b> ' +
			         '<small>limit ' + qsize(limit) + '<\/small><\/td>' +
			         '<td class="q-barcell">' +
			         '<input type="button" value="Reset" onclick="resetOne(\'' + id + '\')">' +
			         '<\/td><td><\/td><\/tr>');

			if (entries.length == 0) {
				buf.push('<tr><td colspan="4" class="q-none"><i>no usage recorded yet<\/i><\/td><\/tr>');
				continue;
			}

			/* biggest consumer first */
			entries.sort(function(a, b) { return b[1] - a[1]; });

			for (j = 0; j < entries.length; ++j) {
				var p = pct(entries[j][1], limit);
				buf.push('<tr><td class="q-ip">' + escapeHTML(entries[j][0]) + '<\/td>' +
				         '<td class="q-used">' + qsize(entries[j][1]) + '<\/td>' +
				         '<td class="q-barcell">' + bar(p) + '<\/td>' +
				         '<td class="q-pct">' + pctText(p) + '<\/td><\/tr>');
			}
		}
		buf.push('<\/table>');
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
/*
 * Fixed layout with explicit column widths. Auto layout sized each table to its
 * own contents, so the bar started at a different offset for every cap and every
 * rule; pinning the widths is what makes the bars share a column down the page.
 * Widths are in em so they track the theme's font rather than fighting it.
 */
.q-table { border-collapse:collapse; table-layout:fixed; width:36em; margin:0 0 10px 12px; }
.q-table col.q-c-host { width:13em; }
.q-table col.q-c-used { width:7em; }
.q-table col.q-c-bar { width:11em; }
.q-table col.q-c-pct { width:5em; }
.q-table td { padding:1px 8px 1px 0; }
/* the per-rule facts: name over the host+used columns, value over the bar+pct
   ones, so every value starts where the Reset button and the bars start */
.q-fact td { padding:0 8px 0 0; }
.q-fname { color:#888; }
/* let a long value run past the table rather than wrap inside a fixed cell -
   "limit down/up kbit/s" is wider than the two columns it sits over */
.q-fval { white-space:nowrap; }
/* the cap heading spans the row, and carries the gap between one cap and the next */
.q-caprow td { padding:8px 0 2px 0; }
.q-ip { font-family:monospace; white-space:nowrap; overflow:hidden; text-overflow:ellipsis; }
.q-used { text-align:right; }
.q-pct { text-align:right; }
.q-none { padding-left:12px; }
/* fills its cell, so every bar is the same length whatever sits beside it */
.q-bar { display:block; width:100%; height:10px; border:1px solid #888; box-sizing:border-box; }
.q-fill { height:100%; }
.q-ok { background:#4c9a4c; }
.q-warn { background:#c8a000; }
.q-over { background:#b03030; }
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
