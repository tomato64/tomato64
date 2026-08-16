<!DOCTYPE html>
<!--
	Tomato64 GUI

	Bandwidth quotas. Ported from the quota feature of Gargoyle router
	firmware (gargoyle-router.com).

	Layout follows the WireGuard peers tab (vpn-wireguard.asp): the configured
	quotas sit in a grid, and a vertical editor underneath adds to it or - when
	you click a row - loads that row back for editing. A quota carries sixteen
	controls, which no grid row has the width for, and this keeps them on one
	page with one Save.

	This file exists only in Tomato64 and has no upstream Tomato
	counterpart, so it carries no TOMATO64 guards.
-->
<html lang="en-GB">
<head>
<meta http-equiv="content-type" content="text/html;charset=utf-8">
<meta name="robots" content="noindex,nofollow">
<title>[<% ident(); %>] Bandwidth Quotas</title>
<link rel="stylesheet" type="text/css" href="tomato.css?rel=<% version(); %>">
<% css(); %>
<style>
/* Column widths for the quota list. Every cell is rendered text - editing
   happens in the form below - so the columns only have to read well. */
#quota-grid .co1 { width: 4%; text-align: center; }	/* On (star) */
#quota-grid .co2 { width: 22%; }			/* Applies To */
#quota-grid .co3 { width: 18%; }			/* Limits */
#quota-grid .co4 { width: 15%; }			/* Resets */
#quota-grid .co5 { width: 15%; }			/* Active */
#quota-grid .co6 { width: 12%; }			/* When Exceeded */
#quota-grid .co7 { width: 14%; }			/* Description */
</style>
<script src="tomato.js?rel=<% version(); %>"></script>

<script>

//	<% nvram("quota_enable,quota_rules,quota_nextid,quota_path,quota_stime,bwl_enable,qos_enable,lan_ipaddr,lan_netmask,lan1_ipaddr,lan1_netmask,lan2_ipaddr,lan2_netmask,lan3_ipaddr,lan3_netmask,lan4_ipaddr,lan4_netmask,lan5_ipaddr,lan5_netmask,lan6_ipaddr,lan6_netmask,lan7_ipaddr,lan7_netmask"); %>

var cprefix = 'quota_settings';

/* must match the sentinels understood by rc/quotas.c */
var QUOTA_ALL           = 'ALL';
var QUOTA_OTHERS_COMB   = 'ALL_OTHERS_COMBINED';
var QUOTA_OTHERS_INDIV  = 'ALL_OTHERS_INDIVIDUAL';
/* a shared-pool record stores its host list prefixed with this in the ip field */
var QUOTA_SHARED_PREFIX = 'SHARED:';

var quota_scope = [
	[QUOTA_ALL,          'Entire Local Network'],
	['HOSTS',            'Only these Host(s)'],
	['HOSTS_SHARED',     'Shared pool for these Host(s)'],
	[QUOTA_OTHERS_INDIV, 'Each Host without a Quota'],
	[QUOTA_OTHERS_COMB,  'All Hosts without a Quota (shared)']
];

var quota_reset = [
	['hour',  'Every Hour'],
	['day',   'Every Day'],
	['week',  'Every Week'],
	['month', 'Every Month']
];

var quota_weekday = [
	['0','Sunday'],['1','Monday'],['2','Tuesday'],['3','Wednesday'],
	['4','Thursday'],['5','Friday'],['6','Saturday']
];

var quota_monthday = [];
for (var i = 1; i <= 31; ++i)
	quota_monthday.push([i+'', i+'']);

/*
 * The "On" dropdown is built with this superset (0-31) so it can hold any
 * stored day when a row is loaded for editing - a weekly rule's value can be 0
 * (Sunday), a monthly rule's 1-31, and an hourly or daily rule keeps whatever
 * it had even though rc ignores it. refreshEditor() then narrows and relabels
 * it for the chosen reset interval.
 */
var quota_dayopt = [];
for (var i = 0; i <= 31; ++i)
	quota_dayopt.push([i+'', i+'']);

var quota_hour = [];
for (var i = 0; i < 24; ++i)
	quota_hour.push([i+'', (i < 10 ? '0' : '')+i+':00']);

/* what happens to a rule's traffic once it is over quota. Must match the
   QUOTA_ACT_* values in rc/quotas.c */
var quota_action = [
	['0','Block Internet Access'],
	['1','Limit Speed']
];

/* when the quota is in force - the "mode" half of the active field */
var quota_active = [
	['always', 'Always'],
	['only',   'Only during...'],
	['except', 'At all times except...']
];

/* which controls describe the window */
var quota_active_type = [
	['hours',          'these hours'],
	['days',           'these days'],
	['days_and_hours', 'these days and hours'],
	['weekly_range',   'these times of the week']
];

/* Limits are entered in the unit the user picks and stored as bytes, because
   that is what the xt_bandwidth match counts in. */
var quota_unit = [['1048576','MB'],['1073741824','GB']];

/* three-letter day names, the only spelling libxt_timerange's parse_weekdays
   understands (it compares exactly three bytes, lowercased) */
var QUOTA_DAYNAMES = ['Sun','Mon','Tue','Wed','Thu','Fri','Sat'];

/* must match QUOTA_RANGE_MAX in rc/quotas.c */
var QUOTA_RANGE_MAX = 12;

function bytesToStr(v) {
	if (v == '' || v == '0')
		return '';
	v = v * 1;
	if (v >= 1073741824 && (v % 1073741824) == 0)
		return (v / 1073741824) + ' GB';
	if (v >= 1048576 && (v % 1048576) == 0)
		return (v / 1048576) + ' MB';

	return scaleSize(v);
}

/* value + unit select -> bytes, '' when left blank (= unlimited) */
function limitToBytes(vf, uf) {
	var v = E(vf).value.trim();
	if (v == '')
		return '';

	return '' + Math.round(v * (E(uf).value * 1));
}

/* bytes -> [value, unit] for editing */
function bytesToLimit(v) {
	if (v == '' || v == '0')
		return ['', '1073741824'];
	v = v * 1;
	if (v >= 1073741824 && (v % 1073741824) == 0)
		return ['' + (v / 1073741824), '1073741824'];

	return ['' + (v / 1048576), '1048576'];
}

/*
 * Rule ids.
 *
 * A rule's saved usage is filed under an id it carries in its own record, not
 * under its position in the list - deleting, reordering or disabling a rule must
 * not hand its consumption to a neighbour, and rc prunes usage files by id, so an
 * id must never be reused. quota_nextid is the high-water mark; it is submitted
 * with the rules that consumed it.
 *
 * A fresh id is issued in exactly two cases: a new rule, and a rule whose address
 * changed (it now meters someone else, so it starts from zero). Editing a limit,
 * schedule, window, description or action keeps the id, and with it the usage
 * accumulated so far.
 *
 * Ids are backfilled onto every rule as the grid is populated. A record written
 * before ids existed falls back in rc to its position in the list, so a
 * backfilled id could otherwise collide with some other rule's position - and
 * xt_bandwidth answers a duplicate --id by rejecting the whole mangle table.
 */
var quota_nextid = (nvram.quota_nextid * 1) || 1;

function quotaNewId() {
	return '' + (quota_nextid++);
}

function quotaBumpId(id) {
	var n = id * 1;

	if (!isNaN(n) && n >= quota_nextid)
		quota_nextid = n + 1;
}

/* ---------------------------------------------------------------- *
 * Active window
 *
 * Stored in field 8 as "<mode>|<hours>|<weekdays>|<weekly_ranges>",
 * empty for always. Read by quota_active_args() in rc/quotas.c.
 * ---------------------------------------------------------------- */

/*
 * One end of a range: "HH", "HH:MM", "HH:MM:SS", prefixed with a day for a
 * weekly range ("Sun 22:00"). Returns seconds from the start of the day (or of
 * the week), -1 for anything else.
 *
 * A deliberate mirror of quota_parse_clock() in rc/quotas.c. rc validates
 * independently and silently falls back to "always" on anything it rejects, so
 * if the two ever disagree the user gets a quota that quietly ignores its
 * window. Keep them in step.
 */
function quotaParseClock(s, weekly) {
	var mult = [3600, 60, 1];
	var secs = 0;

	s = s.replace(/^\s+/, '');

	if (weekly) {
		var day = -1;
		for (var i = 0; i < 7; ++i)
			if (s.substr(0, 3).toLowerCase() == QUOTA_DAYNAMES[i].toLowerCase()) {
				day = i;
				break;
			}
		if (day < 0)
			return -1;

		secs = day * 86400;
		s = s.substr(3).replace(/^\s+/, '');
	}

	for (var i = 0; i < 3; ++i) {
		var m = s.match(/^(\d{1,2})/);	/* HH/MM/SS, never a bare second count */
		if (m == null)
			return -1;

		secs += (m[1] * 1) * mult[i];
		s = s.substr(m[1].length);
		if (s.charAt(0) != ':')
			break;
		s = s.substr(1);
	}

	s = s.replace(/\s+$/, '');
	if (s != '')
		return -1;

	return (secs <= (weekly ? 604800 : 86400)) ? secs : -1;
}

/*
 * Validate a range list - "02:00-06:00,22:30-23:00", or weekly
 * "Fri 18:00-Sun 23:00" - exactly as quota_valid_ranges() does in rc/quotas.c,
 * which in turn mirrors what libxt_timerange will accept. Overlapping or
 * whole-span lists are refused: they are meaningless as a window, and the
 * extension used to crash on them.
 */
function quotaValidRanges(spec, weekly) {
	var span = weekly ? 604800 : 86400;
	var pieces = spec.split(',');
	var start = [], end = [];
	var covered = 0;
	var i, j;

	if (pieces.length > QUOTA_RANGE_MAX)
		return 0;

	for (i = 0; i < pieces.length; ++i) {
		var d = pieces[i].indexOf('-');
		/* exactly one dash, so exactly two endpoints */
		if (d < 0 || pieces[i].indexOf('-', d + 1) >= 0)
			return 0;

		var a = quotaParseClock(pieces[i].substr(0, d), weekly);
		var b = quotaParseClock(pieces[i].substr(d + 1), weekly);
		if (a < 0 || b < 0 || a == b)
			return 0;

		start.push(a);
		end.push(b);
	}
	if (start.length == 0)
		return 0;

	/* a range whose end precedes its start wraps past midnight (or Sunday) */
	for (i = 0; i < start.length; ++i) {
		var e1 = end[i] < start[i] ? end[i] + span : end[i];

		covered += e1 - start[i];

		for (j = 0; j < start.length; ++j) {
			if (j == i)
				continue;

			var e2 = end[j] < start[j] ? end[j] + span : end[j];
			if (start[i] < e2 && e1 > start[j])
				return 0;
		}
	}

	/* covering everything is not a window, it is "always" */
	if (covered >= span)
		return 0;

	return 1;
}

/* the editor controls -> the stored active field */
function fieldsToActive() {
	var mode = E('_f_q_active').value;
	if (mode == 'always')
		return '';

	var type = E('_f_q_atype').value;
	var hours = '', days = '', weekly = '';

	if (type == 'weekly_range')
		weekly = E('_f_q_weekly').value.trim();
	else {
		if (type == 'hours' || type == 'days_and_hours')
			hours = E('_f_q_hours').value.trim();
		if (type == 'days' || type == 'days_and_hours') {
			var a = [];
			for (var i = 0; i < 7; ++i)
				if (E('_f_q_day' + i).checked)
					a.push(QUOTA_DAYNAMES[i]);
			days = a.join(',');
		}
	}

	return mode + '|' + hours + '|' + days + '|' + weekly;
}

/* the stored active field -> the editor controls */
function activeToFields(active) {
	var t = (active || '').split('|');
	var mode = t[0] || '';
	var hours = t[1] || '';
	var days = t[2] || '';
	var weekly = t[3] || '';

	if (mode != 'only' && mode != 'except') {
		mode = 'always';
		hours = days = weekly = '';
	}

	/* which set of controls describes this window - the same mapping Gargoyle
	   derives from which of its three fields are populated */
	var type = 'hours';
	if (weekly != '')
		type = 'weekly_range';
	else if (hours != '' && days != '')
		type = 'days_and_hours';
	else if (days != '')
		type = 'days';

	E('_f_q_active').value = mode;
	E('_f_q_atype').value = type;
	E('_f_q_hours').value = hours;
	E('_f_q_weekly').value = weekly;

	for (var i = 0; i < 7; ++i)
		E('_f_q_day' + i).checked = 0;
	if (days != '') {
		var dl = days.split(',');
		for (var i = 0; i < dl.length; ++i) {
			var d = dl[i].trim().substr(0, 3).toLowerCase();
			if (d == 'all') {
				for (var j = 0; j < 7; ++j)
					E('_f_q_day' + j).checked = 1;
				break;
			}
			for (var j = 0; j < 7; ++j)
				if (d == QUOTA_DAYNAMES[j].toLowerCase())
					E('_f_q_day' + j).checked = 1;
		}
	}
}

/* a one-line summary of the window, for the grid's Active column */
function activeToStr(a) {
	var t = (a || '').split('|');
	var mode = t[0] || '';

	if (mode != 'only' && mode != 'except')
		return 'Always';

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
		return 'Always';

	return (mode == 'only' ? 'Only ' : 'Except ') + '<small>' + escapeHTML(what) + '<\/small>';
}

/* ---------------------------------------------------------------- *
 * The grid
 * ---------------------------------------------------------------- */

function QuotaGrid() { return this; }
QuotaGrid.prototype = new TomatoGrid;

var qg = new QuotaGrid();

QuotaGrid.prototype.setup = function() {
	/*
	 * editorFields has to be non-null for TomatoGrid to treat rows as editable -
	 * that is what gives each row its delete control and routes a click into
	 * edit() below. The descriptors themselves are never rendered: showNewEditor()
	 * is never called, so there is no inline editor row, and edit() is overridden
	 * to load the form underneath instead. Same arrangement as the WireGuard
	 * peers grid.
	 *
	 * "move" adds the up/down/move-to controls. Order is not cosmetic here: two
	 * Limit Speed rules that both cover a host each write the throttle band with
	 * the same full-byte mask, so the LAST one to match sets the speed. Blocking
	 * is a single bit and so order-free, as is a block alongside a limit - the
	 * bits are disjoint and the drop wins either way. Reordering is safe for the
	 * counters: usage is filed under the rule's id, never its position.
	 */
	this.init('quota-grid', 'move', 500, [
		{ type: 'text' }, { type: 'text' }, { type: 'text' }, { type: 'text' },
		{ type: 'text' }, { type: 'text' }, { type: 'text' }
	]);
	this.headerSet(['On','Applies To','Limits','Resets','Active','When Exceeded','Description']);

	if (nvram.quota_rules != '') {
		var raw = nvram.quota_rules.split('>');
		for (var i = 0; i < raw.length; ++i) {
			var t = raw[i].split('<');
			/* 11 fields predates the penalty speeds, 13 the rule id */
			if (t.length != 11 && t.length != 13 && t.length != 14)
				continue;

			while (t.length < 13)
				t.push('');
			if (t.length < 14)
				t.push(quotaNewId());
			else
				quotaBumpId(t[13]);

			this.insertData(-1, t);
		}
	}
}

/*
 * row is the full 14-field record. These are inserted as HTML (TomatoGrid's own
 * dataToView escapes for you, an override has to do it itself), so anything the
 * user typed has to be escaped on the way out. A star marks an enabled rule (as
 * on forward-basic.asp) - clearer than a greyed-out checkbox.
 */
QuotaGrid.prototype.dataToView = function(row) {
	var scope = row[1];
	var who = scope;
	if (scope.substr(0, QUOTA_SHARED_PREFIX.length) == QUOTA_SHARED_PREFIX)
		who = 'Shared pool: ' + scope.substr(QUOTA_SHARED_PREFIX.length);
	else
		for (var i = 0; i < quota_scope.length; ++i) {
			if (quota_scope[i][0] == scope) {
				who = quota_scope[i][1];
				break;
			}
		}

	/* the three caps as one cell */
	var lim = [];
	if (bytesToStr(row[2]) != '')
		lim.push('&#x2193;' + escapeHTML(bytesToStr(row[2])));
	if (bytesToStr(row[3]) != '')
		lim.push('&#x2191;' + escapeHTML(bytesToStr(row[3])));
	if (bytesToStr(row[4]) != '')
		lim.push('&#x21c5;' + escapeHTML(bytesToStr(row[4])));

	/* friendly reset label, then only the parts that apply (see the editor:
	   hourly has no day/hour, daily has an hour, weekly/monthly have both) */
	var when = row[5];
	for (var ri = 0; ri < quota_reset.length; ++ri)
		if (quota_reset[ri][0] == row[5]) { when = quota_reset[ri][1]; break; }

	if (row[5] == 'week')
		when += ' (' + quota_weekday[(row[6] * 1) % 7][1] + ')';
	else if (row[5] == 'month')
		when += ' (day ' + row[6] + ')';
	if (row[5] != 'hour')
		when += ' @ ' + (row[7] < 10 ? '0' : '') + row[7] + ':00';

	/* When Exceeded: Block, or Limit with the down/up penalty speeds */
	var act = 'Block';
	if (row[9] == '1') {
		var dl = (row[11] || '') != '' ? '&#x2193;' + escapeHTML(row[11]) : '';
		var ul = (row[12] || '') != '' ? '&#x2191;' + escapeHTML(row[12]) : '';
		act = 'Limit <small>' + dl + (dl && ul ? ' ' : '') + ul + ' kbit\/s<\/small>';
	}

	return [(row[0] != '0' ? '&#x2b50' : ''),
	        escapeHTML(who), lim.length ? lim.join(' ') : '<i>none<\/i>',
	        when, activeToStr(row[8]), act, escapeHTML(row[10])];
}

/* clicking a row loads it into the editor below, rather than opening an
   inline editor - see the note in setup() */
/*
 * The row currently loaded into the editor, held as the <tr> itself rather than
 * as its position. Move Up/Down/Move-to reorder the element, and Delete can
 * remove a row above it, so any index captured when editing began is stale by
 * the time Save is pressed - it would write the quota into whichever rule had
 * since slid into that slot.
 */
var editRow = null;

QuotaGrid.prototype.edit = function(cell) {
	var row = PR(cell);

	dataToFields(row.getRowData());
	editRow = row;

	var b = E('quota-add-button');
	b.value = 'Save Quota';
	b.setAttribute('onclick', 'saveQuota()');
	E('quota-editing').style.display = '';
	E('_f_q_desc').focus();
}

QuotaGrid.prototype.rowDel = function(e) {
	/* deleting the row being edited leaves the editor pointing at nothing */
	if (e == editRow)
		clearFields();

	this.moving = null;
	e.parentNode.removeChild(e);
	this.recolor();
	this.rpHide();
}

/* the delete control calls rpDel(), which in TomatoGrid does not route through
   rowDel() - so send it there, or the editor is never told the row went away */
QuotaGrid.prototype.rpDel = function(e) {
	this.rowDel(PR(e));
}

/* ---------------------------------------------------------------- *
 * The editor
 * ---------------------------------------------------------------- */

/* a 14-field record -> the editor controls */
function dataToFields(row) {
	var scope = row[1];
	var hosts = '';
	if (scope.substr(0, QUOTA_SHARED_PREFIX.length) == QUOTA_SHARED_PREFIX) {
		hosts = scope.substr(QUOTA_SHARED_PREFIX.length);
		scope = 'HOSTS_SHARED';
	}
	else if (scope != QUOTA_ALL && scope != QUOTA_OTHERS_COMB && scope != QUOTA_OTHERS_INDIV) {
		hosts = scope;
		scope = 'HOSTS';
	}

	var d = bytesToLimit(row[2]);
	var u = bytesToLimit(row[3]);
	var c = bytesToLimit(row[4]);

	E('_f_q_on').checked = (row[0] != '0');
	E('_f_q_desc').value = row[10];
	E('_f_q_scope').value = scope;
	E('_f_q_hosts').value = hosts;
	E('_f_q_dl').value = d[0]; E('_f_q_dlu').value = d[1];
	E('_f_q_ul').value = u[0]; E('_f_q_ulu').value = u[1];
	E('_f_q_cl').value = c[0]; E('_f_q_clu').value = c[1];
	E('_f_q_reset').value = row[5] == '' ? 'hour' : row[5];
	E('_f_q_action').value = row[9] == '' ? '0' : row[9];
	E('_f_q_tdl').value = row[11] || '';
	E('_f_q_tul').value = row[12] || '';
	activeToFields(row[8]);

	/*
	 * The day dropdown carries whichever list the reset interval calls for, so
	 * narrow it first and only then hand it the stored value - otherwise a
	 * weekday 0 would be rejected by the 1-31 list still in place.
	 */
	refreshEditor();
	E('_f_q_rday').value = row[6] == '' ? '1' : row[6];
	E('_f_q_rhour').value = row[7] == '' ? '0' : row[7];
	refreshEditor();

	/* the id travels with the row, invisibly - see the note above quotaNewId */
	E('_f_q_id').value = row[13] || '';
	E('_f_q_ip').value = row[1];
}

/* the editor controls -> a 14-field record */
function fieldsToData() {
	var scope = E('_f_q_scope').value;
	var ip;
	if (scope == 'HOSTS')
		ip = E('_f_q_hosts').value.trim();
	else if (scope == 'HOSTS_SHARED')
		ip = QUOTA_SHARED_PREFIX + E('_f_q_hosts').value.trim();
	else
		ip = scope;

	/* speeds only mean anything for the Limit action; store blank otherwise so
	   flipping back to Block doesn't leave stale numbers behind */
	var limit = (E('_f_q_action').value == '1');
	var tdl = limit ? E('_f_q_tdl').value.trim() : '';
	var tul = limit ? E('_f_q_tul').value.trim() : '';

	/* keep the id when an existing rule is edited, unless its address changed -
	   then it meters someone else and has to start clean */
	var oldid = E('_f_q_id').value;
	var id = (oldid != '' && E('_f_q_ip').value == ip) ? oldid : quotaNewId();

	return [E('_f_q_on').checked ? '1' : '0', ip,
	        limitToBytes('_f_q_dl', '_f_q_dlu'),
	        limitToBytes('_f_q_ul', '_f_q_ulu'),
	        limitToBytes('_f_q_cl', '_f_q_clu'),
	        E('_f_q_reset').value, E('_f_q_rday').value, E('_f_q_rhour').value,
	        fieldsToActive(), E('_f_q_action').value, E('_f_q_desc').value,
	        tdl, tul, id];
}

function clearFields() {
	E('_f_q_on').checked = 1;	/* new rules default to enabled (as on forward-basic.asp) */
	E('_f_q_desc').value = '';
	E('_f_q_scope').value = QUOTA_ALL;
	E('_f_q_hosts').value = '';
	E('_f_q_dl').value = ''; E('_f_q_dlu').value = '1073741824';
	E('_f_q_ul').value = ''; E('_f_q_ulu').value = '1073741824';
	E('_f_q_cl').value = ''; E('_f_q_clu').value = '1073741824';
	E('_f_q_reset').value = 'hour';	/* top option, and hides the day/hour dropdowns */
	E('_f_q_rday').value = '1';
	E('_f_q_rhour').value = '0';
	E('_f_q_action').value = '0';	/* Block */
	E('_f_q_tdl').value = '';
	E('_f_q_tul').value = '';
	activeToFields('');
	E('_f_q_id').value = '';
	E('_f_q_ip').value = '';

	ferror.clearAll(fields.getAll(E('quota-editor')));

	editRow = null;

	var b = E('quota-add-button');
	b.value = 'Add Quota';
	b.setAttribute('onclick', 'addQuota()');
	E('quota-editing').style.display = 'none';

	refreshEditor();
}

/* rebuild a <select>'s options, keeping the given value if it still exists */
function quotaSetOpts(sel, opts, keep) {
	var s = '';
	for (var i = 0; i < opts.length; ++i)
		s += '<option value="' + opts[i][0] + '">' + opts[i][1] + '<\/option>';
	sel.innerHTML = s;
	sel.value = keep;
	if (sel.selectedIndex < 0)
		sel.selectedIndex = 0;
}

function showRow(id, vis) {
	PR(E(id)).style.display = vis ? 'table-row' : 'none';
}

/* true for the scopes that take a typed host/subnet list */
function quotaScopeHasHosts(v) {
	return (v == 'HOSTS' || v == 'HOSTS_SHARED');
}

/*
 * Show only the controls that mean anything for the current selections.
 *
 * Reset (matching Gargoyle and what quota_reset_args() actually reads):
 *   hour  - resets at the top of the hour; day and hour are ignored -> hide both
 *   day   - resets at the chosen hour             -> hide the day, keep the hour
 *   week  - day is a day of the week (0=Sunday)   -> show both, weekday names
 *   month - day is a day of the month (1-31)      -> show both, 1-31
 */
function refreshEditor() {
	var rt = E('_f_q_reset').value;

	if (rt == 'week')
		quotaSetOpts(E('_f_q_rday'), quota_weekday, (E('_f_q_rday').value * 1 <= 6) ? E('_f_q_rday').value : '0');
	else if (rt == 'month')
		quotaSetOpts(E('_f_q_rday'), quota_monthday, (E('_f_q_rday').value * 1 >= 1) ? E('_f_q_rday').value : '1');
	else
		quotaSetOpts(E('_f_q_rday'), quota_dayopt, E('_f_q_rday').value);

	showRow('_f_q_hosts', quotaScopeHasHosts(E('_f_q_scope').value));
	showRow('_f_q_rday', (rt == 'week' || rt == 'month'));
	showRow('_f_q_rhour', (rt != 'hour'));

	/* active window: the type selector and then only its own controls */
	var timed = (E('_f_q_active').value != 'always');
	var at = E('_f_q_atype').value;
	showRow('_f_q_atype', timed);
	showRow('_f_q_day0', timed && (at == 'days' || at == 'days_and_hours'));
	showRow('_f_q_hours', timed && (at == 'hours' || at == 'days_and_hours'));
	showRow('_f_q_weekly', timed && (at == 'weekly_range'));

	/* the two penalty speeds only apply to the Limit action */
	var limit = (E('_f_q_action').value == '1');
	showRow('_f_q_tdl', limit);
	showRow('_f_q_tul', limit);
}

/* ---------------------------------------------------------------- *
 * Host validation - a quota may name an IP, a range or a CIDR subnet
 * on any LAN bridge.
 * ---------------------------------------------------------------- */

/*
 * Validate a host entry against EVERY configured LAN bridge, not just the
 * primary. v_macip only knows one subnet, so a br1-br7 address would fail its
 * "outside of LAN" check even though the quota rules handle it fine. Try each
 * bridge in turn and accept the first that matches - v_macip leaves the field
 * untouched on a failed subnet check, so re-trying is safe, and on success it
 * normalises the value (and clears the error) as usual. A MAC passes here too;
 * the caller rejects that separately.
 */
function v_macip_anylan(e, quiet) {
	for (var i = 0; i <= MAX_BRIDGE_ID; ++i) {
		var j = (i == 0) ? '' : i.toString();
		var ip = nvram['lan' + j + '_ipaddr'];
		var mask = nvram['lan' + j + '_netmask'];
		if (!ip || !mask)
			continue;
		if (v_macip(e, 1, 0, ip, mask))	/* quiet: suppress the per-bridge miss */
			return true;
	}
	ferror.set(e, 'IP address is not within any LAN', quiet);
	return false;
}

/*
 * Host/network field: a CIDR subnet (192.168.2.0/24), or - via v_macip_anylan -
 * a single IP or range, in any configured LAN bridge. CIDR lets a quota target
 * a whole network in one entry; rc passes it straight to iptables. The subnet
 * must sit inside a LAN bridge (a broader block spanning the WAN is rejected),
 * and its host bits are dropped so the stored value is the network address.
 */
function v_quota_host(e, quiet) {
	if ((e = E(e)) == null)
		return 0;

	var m = e.value.replace(/\s+/g, '').match(/^(\d+\.\d+\.\d+\.\d+)\/(\d+)$/);
	if (m == null)
		return v_macip_anylan(e, quiet);	/* IP, range or MAC */

	var base = fixIP(m[1]);
	var bits = m[2] * 1;
	if ((base == null) || (bits < 0) || (bits > 32)) {
		ferror.set(e, 'Invalid subnet (expected a.b.c.d/0-32)', quiet);
		return false;
	}

	for (var i = 0; i <= MAX_BRIDGE_ID; ++i) {
		var j = (i == 0) ? '' : i.toString();
		var ip = nvram['lan' + j + '_ipaddr'];
		var mask = nvram['lan' + j + '_netmask'];
		if (!ip || !mask)
			continue;
		if ((aton(base) & aton(mask)) == (aton(ip) & aton(mask))) {
			var cmask = (bits == 0) ? 0 : (-1 << (32 - bits));
			e.value = ntoa(aton(base) & cmask) + '/' + bits;
			ferror.clear(e);
			return true;
		}
	}
	ferror.set(e, 'Subnet is not within any LAN', quiet);
	return false;
}

/*
 * Validate a comma/space separated list of hosts/ranges/subnets (rc splits the
 * stored ip field on ", "). Each entry is checked with v_quota_host by driving
 * the real field one token at a time - on failure its value is left as the
 * offending token so the highlight lands on it; on success the field is rebuilt
 * as a normalised comma list. Rejects a MAC (a quota can't be keyed on one).
 */
function v_quota_hostlist(e, quiet) {
	if ((e = E(e)) == null)
		return 0;

	var parts = e.value.split(/[\s,]+/);
	var out = [];
	for (var i = 0; i < parts.length; ++i) {
		if (parts[i] == '')
			continue;
		e.value = parts[i];
		if (!v_quota_host(e, quiet))		/* leaves e.value = the bad token */
			return false;
		if (e.value.indexOf(':') >= 0) {
			ferror.set(e, 'Quotas must use an IP address, range or subnet - a MAC address cannot match downloads', quiet);
			return false;
		}
		out.push(e.value);
	}
	if (out.length == 0) {
		ferror.set(e, 'You must specify an IP address, range or subnet', quiet);
		return false;
	}

	e.value = out.join(',');
	ferror.clear(e);
	return true;
}

/*
 * Validate the editor. Deliberately separate from verifyFields(), which every
 * control calls on every keystroke: an empty editor is a perfectly normal state
 * for this page, so it must not light up red until you actually try to add.
 */
function verifyQuotaFields(quiet) {
	var ok = 1;

	refreshEditor();

	/*
	 * The host scopes need an address list; a shared pool is where listing
	 * several subnets makes the most sense (they share one counter). v_macip
	 * would accept a MAC, but a quota can't be keyed on one - download traffic
	 * arrives from the WAN with the host's MAC nowhere in the packet, so only
	 * uploads could ever match. Gargoyle is IP-only for the same reason.
	 */
	if (quotaScopeHasHosts(E('_f_q_scope').value)) {
		if (E('_f_q_hosts').value.trim() == '') {
			ferror.set('_f_q_hosts', 'You must specify an IP address, range or subnet', quiet);
			ok = 0;
		}
		else if (!v_quota_hostlist('_f_q_hosts', quiet))
			ok = 0;
	}
	else
		ferror.clear('_f_q_hosts');

	/* at least one cap, otherwise the quota can never be exceeded */
	var caps = ['_f_q_dl', '_f_q_ul', '_f_q_cl'];
	var any = 0;
	for (var i = 0; i < caps.length; ++i) {
		if (E(caps[i]).value.trim() == '') {
			ferror.clear(caps[i]);
			continue;
		}
		if (!v_range(caps[i], quiet, 1, 999999)) {
			ok = 0;
			continue;
		}
		any = 1;
	}
	if (ok && !any) {
		ferror.set('_f_q_dl', 'Set at least one of the download, upload or combined limits', quiet);
		ok = 0;
	}

	/*
	 * Limit-speed rules need both a download and an upload penalty speed, the
	 * way Gargoyle does: a combined cap can only throttle when both directions
	 * are shapeable, and rc reads each direction's speed independently.
	 */
	if (E('_f_q_action').value == '1') {
		if (!v_range('_f_q_tdl', quiet, 1, 10000000))
			ok = 0;
		if (!v_range('_f_q_tul', quiet, 1, 10000000))
			ok = 0;
	}
	else {
		ferror.clear('_f_q_tdl');
		ferror.clear('_f_q_tul');
	}

	/*
	 * The active window, checked the same way rc will check it. rc falls back to
	 * "always" on anything it cannot read, without telling anyone but the syslog -
	 * so catching it here is the difference between an error message and a quota
	 * that quietly ignores its schedule.
	 */
	if (E('_f_q_active').value != 'always') {
		var at = E('_f_q_atype').value;

		if (at == 'weekly_range') {
			if (!quotaValidRanges(E('_f_q_weekly').value.trim(), 1)) {
				ferror.set('_f_q_weekly', 'Expected up to ' + QUOTA_RANGE_MAX + ' non-overlapping ranges, e.g. "Fri 18:00-Sun 23:00". A window covering the whole week is the same as Always.', quiet);
				ok = 0;
			}
			else
				ferror.clear('_f_q_weekly');
		}
		else {
			if (at == 'hours' || at == 'days_and_hours') {
				if (!quotaValidRanges(E('_f_q_hours').value.trim(), 0)) {
					ferror.set('_f_q_hours', 'Expected up to ' + QUOTA_RANGE_MAX + ' non-overlapping ranges, e.g. "02:00-06:00,22:30-23:00". A window covering the whole day is the same as Always.', quiet);
					ok = 0;
				}
				else
					ferror.clear('_f_q_hours');
			}
			else
				ferror.clear('_f_q_hours');

			if (at == 'days' || at == 'days_and_hours') {
				var nd = 0;
				for (var i = 0; i < 7; ++i)
					if (E('_f_q_day' + i).checked)
						nd++;
				if (nd == 0) {
					ferror.set('_f_q_day0', 'Select at least one day', quiet);
					ok = 0;
				}
				else
					ferror.clear('_f_q_day0');
			}
			else
				ferror.clear('_f_q_day0');
		}
	}
	else {
		ferror.clear('_f_q_hours');
		ferror.clear('_f_q_weekly');
		ferror.clear('_f_q_day0');
	}

	/* records are ">" separated and fields "<" separated - neither may appear */
	if (!v_nodelim('_f_q_desc', quiet, 'Description', 1))
		ok = 0;

	return ok;
}

function addQuota() {
	if (!verifyQuotaFields(0))
		return;

	qg.insertData(-1, fieldsToData());
	clearFields();
}

function saveQuota() {
	if (editRow == null)		/* the row was deleted while it was being edited */
		return;
	if (!verifyQuotaFields(0))
		return;

	/* read the position now, from the element - it may have been moved since */
	var row = editRow;
	var at = row.rowIndex;
	var data = fieldsToData();

	/* replace in place so the list keeps its order; rowDel() clears the editor,
	   which is why the record and position are captured first */
	qg.rowDel(row);
	qg.insertData(at, data);
	clearFields();
}

/* ---------------------------------------------------------------- *
 * Page
 * ---------------------------------------------------------------- */

function verifyFields(focused, quiet) {
	var a = !E('_f_quota_enable').checked;

	E('_quota_path').disabled = a;
	E('_quota_stime').disabled = a;

	refreshEditor();

	/*
	 * Quotas and the bandwidth limiter can now run together - block rules always
	 * coexist. Only speed limiting contends for the tc queues those features own:
	 * the limiter takes the LAN bridges (so download limiting falls back to a
	 * block while it is on), and the limiter or QoS take the WAN (so upload
	 * limiting falls back to a block while either is on). Surface that as advice
	 * rather than disabling anything.
	 */
	E('bwlnotice').style.display = (nvram.bwl_enable != '0') ? 'block' : 'none';
	E('qosnotice').style.display = (nvram.qos_enable != '0') ? 'block' : 'none';

	return 1;
}

function save() {
	if (E('quota-add-button').value != 'Add Quota' &&
	    !confirm('The quota in the editor has not been added to the list. Save anyway and discard it?'))
		return;

	var data = qg.getAllData();
	var rules = '';

	for (var i = 0; i < data.length; ++i)
		rules += (i ? '>' : '') + data[i].join('<');

	var fom = E('t_fom');
	fom.quota_enable.value = fom._f_quota_enable.checked ? 1 : 0;
	fom.quota_rules.value = rules;
	/* persist the id high-water mark alongside the rules that consumed it */
	fom.quota_nextid.value = quota_nextid;
	form.submit(fom, 1);
}

function earlyInit() {
	qg.setup();
	clearFields();
	verifyFields(null, 1);
	insOvl();
}

function init() {
	var c;
	if (((c = cookie.get(cprefix+'_notes_vis')) != null) && (c == '1'))
		toggleVisibility(cprefix, 'notes');

	qg.recolor();
}
</script>
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

<input type="hidden" name="_nextpage" value="quota-settings.asp">
<input type="hidden" name="_nextwait" value="10">
<input type="hidden" name="_service" value="firewall-restart">
<input type="hidden" name="quota_enable">
<input type="hidden" name="quota_rules">
<input type="hidden" name="quota_nextid">

<!-- the edited row's id and address, so fieldsToData() can tell an edit that
     keeps its usage from one that has been pointed at a different host -->
<input type="hidden" id="_f_q_id" value="">
<input type="hidden" id="_f_q_ip" value="">

<!-- / / / -->

<div class="section-title">Bandwidth Quotas</div>
<div class="section">
	<div class="fields" id="bwlnotice" style="display:none"><div class="about"><a href="bwlimit.asp">The Bandwidth Limiter is enabled</a>. Quotas still work, but <b>Limit Speed</b> rules fall back to blocking (the limiter owns the traffic queues). Turn it off for speed limiting.</div></div>
	<div class="fields" id="qosnotice" style="display:none"><div class="about"><a href="qos-settings.asp">QoS is enabled</a>. <b>Limit Speed</b> still throttles downloads, but upload and combined limits fall back to blocking, since QoS owns the WAN queue.</div></div>
	<script>
		createFieldTable('', [
			{ title: 'Enable Quotas', name: 'f_quota_enable', type: 'checkbox', value: nvram.quota_enable != '0' },
			{ title: 'Save Usage To', indent: 2, name: 'quota_path', type: 'text', maxlen: 48, size: 48, value: nvram.quota_path,
			  suffix: ' <small>leave empty to keep usage in RAM only (lost on reboot)<\/small>' },
			{ title: 'Save Every', indent: 2, name: 'quota_stime', type: 'text', maxlen: 2, size: 4, value: nvram.quota_stime,
			  suffix: ' <small>hours<\/small>' }
		]);
	</script>
</div>

<!-- / / / -->

<div class="section-title">Quotas</div>
<div class="section">
	<div class="tomato-grid" id="quota-grid"></div>
	<div class="about"><small>Click a quota to load it into the editor below.</small></div>
</div>

<!-- / / / -->

<div class="section-title">Quota Editor <small id="quota-editing" style="display:none"><i>(editing an existing quota)</i></small></div>
<div class="section" id="quota-editor">
	<script>
		/* createFieldTable wires onchange/onclick to verifyFields(this, 1) on every
		   control it builds, and verifyFields() calls refreshEditor(), so the
		   show/hide cascade needs no per-field handlers. Values are all set by
		   clearFields() from earlyInit(), so none are supplied here. */
		createFieldTable('', [
			{ title: 'Enabled', name: 'f_q_on', type: 'checkbox', value: 1 },
			{ title: 'Description', name: 'f_q_desc', type: 'text', maxlen: 31, size: 40, value: '' },

			{ title: 'Applies To', name: 'f_q_scope', type: 'select', options: quota_scope, value: QUOTA_ALL },
				{ title: 'Host(s)', indent: 2, name: 'f_q_hosts', type: 'text', maxlen: 128, size: 48, value: '',
				  suffix: ' <small>IP, range or subnet; separate several with commas<\/small>' },

			{ title: 'Download Limit', multi: [
				{ name: 'f_q_dl', type: 'text', maxlen: 10, size: 10, value: '' },
				{ name: 'f_q_dlu', type: 'select', options: quota_unit, value: '1073741824', prefix: '&nbsp;',
				  suffix: ' <small>leave blank for unlimited<\/small>' } ] },
			{ title: 'Upload Limit', multi: [
				{ name: 'f_q_ul', type: 'text', maxlen: 10, size: 10, value: '' },
				{ name: 'f_q_ulu', type: 'select', options: quota_unit, value: '1073741824', prefix: '&nbsp;' } ] },
			{ title: 'Combined Limit', multi: [
				{ name: 'f_q_cl', type: 'text', maxlen: 10, size: 10, value: '' },
				{ name: 'f_q_clu', type: 'select', options: quota_unit, value: '1073741824', prefix: '&nbsp;',
				  suffix: ' <small>up and down metered together<\/small>' } ] },

			{ title: 'Resets', name: 'f_q_reset', type: 'select', options: quota_reset, value: 'hour' },
				{ title: 'On', indent: 2, name: 'f_q_rday', type: 'select', options: quota_dayopt, value: '1' },
				{ title: 'At', indent: 2, name: 'f_q_rhour', type: 'select', options: quota_hour, value: '0' },

			{ title: 'Quota is Active', name: 'f_q_active', type: 'select', options: quota_active, value: 'always' },
				{ title: '', indent: 2, name: 'f_q_atype', type: 'select', options: quota_active_type, value: 'hours' },
				{ title: 'Days', indent: 2, multi: [
					{ name: 'f_q_day0', type: 'checkbox', suffix: ' Sun &nbsp; ', value: 0 },
					{ name: 'f_q_day1', type: 'checkbox', suffix: ' Mon &nbsp; ', value: 0 },
					{ name: 'f_q_day2', type: 'checkbox', suffix: ' Tue &nbsp; ', value: 0 },
					{ name: 'f_q_day3', type: 'checkbox', suffix: ' Wed &nbsp; ', value: 0 },
					{ name: 'f_q_day4', type: 'checkbox', suffix: ' Thu &nbsp; ', value: 0 },
					{ name: 'f_q_day5', type: 'checkbox', suffix: ' Fri &nbsp; ', value: 0 },
					{ name: 'f_q_day6', type: 'checkbox', suffix: ' Sat', value: 0 } ] },
				{ title: 'Hours', indent: 2, name: 'f_q_hours', type: 'text', maxlen: 160, size: 48, value: '',
				  suffix: ' <small>e.g. 02:00-06:00,22:30-23:00<\/small>' },
				{ title: 'Times', indent: 2, name: 'f_q_weekly', type: 'text', maxlen: 160, size: 48, value: '',
				  suffix: ' <small>e.g. Fri 18:00-Sun 23:00<\/small>' },

			{ title: 'When Exceeded', name: 'f_q_action', type: 'select', options: quota_action, value: '0' },
				{ title: 'Download Speed', indent: 2, name: 'f_q_tdl', type: 'text', maxlen: 8, size: 10, value: '',
				  suffix: ' <small>kbit\/s<\/small>' },
				{ title: 'Upload Speed', indent: 2, name: 'f_q_tul', type: 'text', maxlen: 8, size: 10, value: '',
				  suffix: ' <small>kbit\/s<\/small>' }
		]);
	</script>
	<div class="fields">
		<input type="button" value="Add Quota" id="quota-add-button" onclick="addQuota()">
		<input type="button" value="Clear" id="quota-clear-button" onclick="clearFields()">
	</div>
</div>

<!-- / / / -->

<div class="section-title">Notes <small><i><a href="javascript:toggleVisibility(cprefix,'notes');" id="toggleLink-notes"><span id="sesdiv_notes_showhide">(Show)</span></a></i></small></div>
<div class="section" id="sesdiv_notes" style="display:none">
	<ul>
		<li>Build a quota in the editor and press <b>Add Quota</b>. Click a row in the list to load it back for editing, then press <b>Save Quota</b>. Nothing reaches the router until you press <b>Save</b> at the foot of the page.</li>
		<li>Hover a row for its controls: <b>move up</b>, <b>move down</b>, <b>move</b> (then click where it should go) and <b>delete</b>. Order matters only when two <b>Limit Speed</b> quotas cover the same traffic and both are over their cap - the one further down the list sets the speed. Blocking is unaffected by order, and a block always wins over a speed limit. Reordering never disturbs recorded usage, which is filed under the quota itself rather than its position.</li>
		<li><b>Applies To</b> - listed in the same order as the drop-down:
			<ul>
				<li><b>Entire Local Network</b> - one shared counter for all LAN traffic.</li>
				<li><b>Only these Host(s)</b> - a separate counter for each listed host. Enter an IP address (192.168.1.5), a range (192.168.1.10-20) or a whole subnet in CIDR notation (192.168.2.0/24), on any LAN bridge; separate several with commas. MAC addresses are not supported: download traffic arrives from the WAN, so a MAC could only ever match uploads.</li>
				<li><b>Shared pool for these Host(s)</b> - as above, but one shared counter for everything listed. List several subnets (192.168.3.0/24, 192.168.4.0/24) to give them a single combined pool, or use one subnet per rule to give each its own.</li>
				<li><b>Each Host without a Quota</b> - a separate counter per host, for every host not covered by an explicit quota above.</li>
				<li><b>All Hosts without a Quota (shared)</b> - as above, but they share a single counter between them.</li>
			</ul>
		</li>
		<li>Limits are per reset period (&#x2193; download, &#x2191; upload, &#x21c5; combined). Leave a limit blank for unlimited, but set at least one.</li>
		<li>Usage is kept in kernel memory. Set <b>Save Usage To</b> to a path on JFFS or USB storage for it to survive a reboot.</li>
		<li><b>Quota is Active</b> - restrict when the quota counts and enforces. Traffic outside the window is neither metered nor blocked, so an off-peak window is a free allowance rather than a second quota. Ranges may cross midnight (22:00-02:00) and must not overlap.</li>
		<li><b>When Exceeded</b> - <b>Block Internet Access</b> drops the traffic, or <b>Limit Speed</b> caps it to the download (&#x2193;) and upload (&#x2191;) speeds you set, in kbit/s. Speed limiting needs the queue it shapes on to be free: downloads need the Bandwidth Limiter off, uploads (and combined limits) need both the Limiter and QoS off - otherwise that direction falls back to blocking.</li>
		<li>Changing a quota's address starts its usage from zero, since it now meters a different host. Every other edit keeps the usage accumulated so far.</li>
	</ul>
</div>

<!-- / / / -->

</td></tr>
<tr><td id="footer" colspan="2">
	<span id="footer-msg"></span>
	<input type="button" value="Save" id="save-button" onclick="save()">
	<input type="button" value="Cancel" id="cancel-button" onclick="javascript:reloadPage();">
	<span id="debug"></span>
</td></tr>
</table>
</form>
<script>earlyInit()</script>
</body>
</html>
