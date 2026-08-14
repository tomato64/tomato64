<!DOCTYPE html>
<!--
	Tomato64 GUI

	Bandwidth quotas. Ported from the quota feature of Gargoyle router
	firmware (gargoyle-router.com).

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
/* Column widths for the quota grid. Auto table layout, so these bias the
   header and data cells; the editor inputs are sized (size=) to match. The
   limit columns need little room, so give the spare width to Applies To. */
#quota-grid .co1 { width: 4%; text-align: center; }	/* On (star) */
#quota-grid .co2 { width: 24%; }			/* Applies To */
#quota-grid .co3,
#quota-grid .co4,
#quota-grid .co5 { width: 8%; }				/* Download / Upload / Combined */
#quota-grid .co6 { width: 15%; }			/* Resets */
#quota-grid .co7 { width: 12%; }			/* When Exceeded */
#quota-grid .co8 { width: 13%; }			/* Description */
/* the download/upload penalty-speed inputs stack vertically under the action
   dropdown (block display puts each on its own line, keeping them out of the
   Description column); refreshActionControls toggles display none/'' to hide
   them for the Block action, and '' falls back to this block rule */
#quota-grid .q-spd { display: block; white-space: nowrap; }
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
 * The "on" dropdown is built with this superset (0-31) so it can hold any
 * stored day at editor-creation time - a weekly rule's value can be 0
 * (Sunday), a monthly rule's 1-31. refreshResetControls() then narrows and
 * relabels it to weekdays or days-of-month for the chosen reset interval.
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

/* Limits are entered in the unit the user picks and stored as bytes, because
   that is what the xt_bandwidth match counts in. */
var quota_unit = [['1048576','MB'],['1073741824','GB']];

function bytesToStr(v) {
	if (v == '' || v == '0')
		return 'Unlimited';
	v = v * 1;
	if (v >= 1073741824 && (v % 1073741824) == 0)
		return (v / 1073741824) + ' GB';
	if (v >= 1048576 && (v % 1048576) == 0)
		return (v / 1048576) + ' MB';

	return scaleSize(v);
}

/* value + unit select -> bytes, '' when left blank (= unlimited) */
function limitToBytes(vf, uf) {
	var v = vf.value.trim();
	if (v == '')
		return '';

	return '' + Math.round(v * (uf.value * 1));
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
 * under its position in the list - deleting, reordering or disabling a rule
 * must not hand its consumption to a neighbour, and rc prunes usage files by
 * id, so an id must never be reused. quota_nextid is the high-water mark; it
 * is submitted with the form so the allocation survives the save.
 *
 * A fresh id is issued in exactly two cases: a new rule, and a rule whose
 * address changed (it now meters someone else, so it starts from zero -
 * Gargoyle gets the same effect with an ignore_backup_at_next_restore flag).
 * Editing a limit, schedule, description or action keeps the id, and with it
 * the usage accumulated so far.
 */
var quota_nextid = (nvram.quota_nextid * 1) || 1;

function quotaNewId() {
	return '' + (quota_nextid++);
}

/*
 * Keep the counter ahead of every id already in use. It should be there
 * already - both are written by the same save - but if the rules ever landed
 * without the counter, reissuing a live id would give two rules the same
 * --id, and xt_bandwidth answers that by rejecting the whole mangle table.
 */
function quotaBumpId(id) {
	var n = id * 1;

	if (!isNaN(n) && n >= quota_nextid)
		quota_nextid = n + 1;
}

var qg = new TomatoGrid();

qg.setup = function() {
	/*
	 * The editor has 16 controls but a row only displays 8 values - a limit
	 * and its unit belong in one cell, as do the scope and its host list, the
	 * three reset controls, and the exceeded action with its two speed inputs.
	 * TomatoGrid builds exactly one <td> per entry in this list and has no
	 * colspan, so anything grouped for display has to be grouped here too with
	 * "multi" (see qos-classify.asp) or the saved rows will not line up under
	 * the header.
	 *
	 * fields.getAll() still returns the controls flattened in this order, so
	 * the f[0]..f[15] indexes used below are unaffected by the grouping:
	 *   0 enable  1 scope  2 hosts  3-4 dl+unit  5-6 ul+unit  7-8 comb+unit
	 *   9 reset  10 day  11 hour  12 action  13 tdl  14 tul  15 description
	 */
	this.init('quota-grid', '', 40, [
		{ type: 'checkbox', prefix: '<div class="centered">', suffix: '<\/div>' },
		{ multi: [
			{ type: 'select', options: quota_scope },
			{ type: 'text', maxlen: 128, attrib: 'size="30"', prefix: '<span class="q-hosts"><br>', suffix: '<\/span>' } ] },
		{ multi: [
			{ type: 'text', maxlen: 10, attrib: 'size="7"' },
			{ type: 'select', options: quota_unit, prefix: '&nbsp;' } ] },
		{ multi: [
			{ type: 'text', maxlen: 10, attrib: 'size="7"' },
			{ type: 'select', options: quota_unit, prefix: '&nbsp;' } ] },
		{ multi: [
			{ type: 'text', maxlen: 10, attrib: 'size="7"' },
			{ type: 'select', options: quota_unit, prefix: '&nbsp;' } ] },
		{ multi: [
			{ type: 'select', options: quota_reset },
			{ type: 'select', options: quota_dayopt, prefix: '<span class="q-on">&nbsp;on&nbsp;', suffix: '<\/span>' },
			{ type: 'select', options: quota_hour, prefix: '<span class="q-at">&nbsp;at&nbsp;', suffix: '<\/span>' } ] },
		{ multi: [
			{ type: 'select', options: quota_action },
			{ type: 'text', maxlen: 8, attrib: 'size="6"', prefix: '<span class="q-spd">&#x2193;&nbsp;', suffix: '<small> kbit\/s<\/small><\/span>' },
			{ type: 'text', maxlen: 8, attrib: 'size="6"', prefix: '<span class="q-spd">&#x2191;&nbsp;', suffix: '<small> kbit\/s<\/small><\/span>' } ] },
		{ type: 'text', maxlen: 31 }]);

	this.headerSet(['On','Applies To','Download','Upload','Combined',
	                'Resets','When Exceeded','Description']);

	/* enabled<ip<dlimit<ulimit<climit<reset<rday<rhour<active<action<desc<tdl<tul<id
	   - tdl/tul are absent on a record saved before speed limits, id on one saved
	   before rule ids; backfill both so every row here is the current shape */
	var rules = nvram.quota_rules.split('>');
	for (var i = 0; i < rules.length; ++i) {
		var t = rules[i].split('<');
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
	this.showNewEditor();
	this.resetNewEditor();
}

/* grid row (14 stored fields) -> the 16 editor widgets; the id has no widget */
qg.dataToFieldValues = function(row) {
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

	/* tdl/tul (row[11], row[12]) are absent on a pre-throttle record */
	return [row[0] == 1, scope, hosts,
	        d[0], d[1], u[0], u[1], c[0], c[1],
	        row[5], row[6] == '' ? '1' : row[6], row[7] == '' ? '0' : row[7],
	        row[9], row[11] || '', row[12] || '', row[10]];
}

qg.fieldValuesToData = function(row) {
	var f = fields.getAll(row);
	var scope = f[1].value;
	var ip;
	if (scope == 'HOSTS')
		ip = f[2].value.trim();
	else if (scope == 'HOSTS_SHARED')
		ip = QUOTA_SHARED_PREFIX + f[2].value.trim();
	else
		ip = scope;

	/* speeds only mean anything for the Limit action; store blank otherwise so
	   flipping back to Block doesn't leave stale numbers behind */
	var limit = (f[12].value == '1');
	var tdl = limit ? f[13].value.trim() : '';
	var tul = limit ? f[14].value.trim() : '';

	/*
	 * Keep the id when an existing rule is edited, unless its address changed -
	 * then it meters someone else and has to start clean. Adding a rule gets a
	 * brand new id.
	 *
	 * this.source is the row being edited; it is null when adding, but test the
	 * editor we were handed as well. Two rules sharing an id is not a cosmetic
	 * problem - xt_bandwidth refuses a duplicate --id and iptables-restore then
	 * rejects the whole mangle table.
	 */
	var old = (row == this.editor && this.source && this.source.getRowData) ?
	          this.source.getRowData() : null;
	var id = (old && old[1] == ip && old[13]) ? old[13] : quotaNewId();

	/* field 8 (active) stays empty until the timerange match is wired up.
	   Order: enabled<ip<dl<ul<comb<reset<rday<rhour<active<action<desc<tdl<tul<id */
	return [f[0].checked ? '1' : '0', ip,
	        limitToBytes(f[3], f[4]), limitToBytes(f[5], f[6]), limitToBytes(f[7], f[8]),
	        f[9].value, f[10].value, f[11].value, '', f[12].value, f[15].value, tdl, tul, id];
}

qg.dataToView = function(row) {
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

	/*
	 * One cell per header column - see the note on the field list above.
	 * These are inserted as HTML (TomatoGrid's own dataToView escapes for
	 * you, an override has to do it itself), so anything the user typed has
	 * to be escaped on the way out. A star marks an enabled rule (as on
	 * forward-basic.asp) - clearer than a greyed-out checkbox.
	 */
	/* When Exceeded: Block, or Limit with the down/up penalty speeds */
	var act = 'Block';
	if (row[9] == '1') {
		var dl = (row[11] || '') != '' ? '&#x2193;' + escapeHTML(row[11]) : '';
		var ul = (row[12] || '') != '' ? '&#x2191;' + escapeHTML(row[12]) : '';
		act = 'Limit <small>' + dl + (dl && ul ? ' ' : '') + ul + ' kbit\/s<\/small>';
	}

	return [(row[0] != '0' ? '&#x2b50' : ''),
	        escapeHTML(who), bytesToStr(row[2]), bytesToStr(row[3]), bytesToStr(row[4]),
	        when, act, escapeHTML(row[10])];
}

qg.resetNewEditor = function() {
	var f = fields.getAll(this.newEditor);
	ferror.clearAll(f);

	f[0].checked = 1;	/* new rules default to enabled (as on forward-basic.asp) */
	f[1].value = QUOTA_ALL;
	f[2].value = '';
	f[3].value = ''; f[4].value = '1073741824';
	f[5].value = ''; f[6].value = '1073741824';
	f[7].value = ''; f[8].value = '1073741824';
	f[9].value = 'hour';	/* top option, and hides the day/hour dropdowns for a cleaner default */
	f[10].value = '1';
	f[11].value = '0';
	f[12].value = '0';	/* action: Block */
	f[13].value = '';	/* download speed (kbit/s) */
	f[14].value = '';	/* upload speed (kbit/s) */
	f[15].value = '';	/* description */

	this.onScopeChange();
}

/* true for the scopes that take a typed host/subnet list */
function quotaScopeHasHosts(v) {
	return (v == 'HOSTS' || v == 'HOSTS_SHARED');
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

/*
 * Show only the reset parts that matter for the chosen interval - matching
 * Gargoyle and what rc actually reads (see quota_reset_args):
 *   hour  - resets at the top of the hour; day and hour are ignored -> hide both
 *   day   - resets at the chosen hour             -> hide the day, keep the hour
 *   week  - day is a day of the week (0=Sunday)   -> show both, weekday names
 *   month - day is a day of the month (1-31)      -> show both, 1-31
 */
function refreshResetControls(f) {
	var rt = f[9].value;

	if (rt == 'week')
		quotaSetOpts(f[10], quota_weekday, (f[10].value * 1 <= 6) ? f[10].value : '0');
	else if (rt == 'month')
		quotaSetOpts(f[10], quota_monthday, (f[10].value * 1 >= 1) ? f[10].value : '1');

	f[10].parentNode.style.display = (rt == 'week' || rt == 'month') ? '' : 'none';
	f[11].parentNode.style.display = (rt == 'hour') ? 'none' : '';
}

/* the two penalty-speed inputs only apply to the Limit action */
function refreshActionControls(f) {
	var limit = (f[12].value == '1');
	f[13].parentNode.style.display = limit ? '' : 'none';
	f[14].parentNode.style.display = limit ? '' : 'none';
}

/* the IP list only makes sense for an explicit host quota */
qg.onScopeChange = function() {
	var rows = [this.newEditor];
	if (this.editor)
		rows.push(this.editor);

	for (var r = 0; r < rows.length; ++r) {
		if (!rows[r])
			continue;
		var f = fields.getAll(rows[r]);
		if (f.length < 16)
			continue;
		/* the host/subnet box only applies to the two "these host(s)" scopes;
		   hide it (with its line break) otherwise, like the reset controls */
		f[2].parentNode.style.display = quotaScopeHasHosts(f[1].value) ? '' : 'none';
		refreshResetControls(f);
		refreshActionControls(f);
	}
}

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

qg.verifyFields = function(row, quiet) {
	var f = fields.getAll(row);
	var ok = 1;

	this.onScopeChange();

	/*
	 * The host scopes need an address list; a shared pool is where listing
	 * several subnets makes the most sense (they share one counter). v_macip
	 * would accept a MAC, but a quota can't be keyed on one - download traffic
	 * arrives from the WAN with the host's MAC nowhere in the packet, so only
	 * uploads could ever match. Gargoyle is IP-only for the same reason.
	 */
	if (quotaScopeHasHosts(f[1].value)) {
		if (f[2].value.trim() == '') {
			ferror.set(f[2], 'You must specify an IP address, range or subnet', quiet);
			ok = 0;
		}
		else if (!v_quota_hostlist(f[2], quiet)) {
			ok = 0;
		}
	}
	else
		ferror.clear(f[2]);

	/* at least one cap, otherwise the quota can never be exceeded */
	var caps = [[f[3], f[4]], [f[5], f[6]], [f[7], f[8]]];
	var any = 0;
	for (var i = 0; i < caps.length; ++i) {
		var v = caps[i][0].value.trim();
		if (v == '') {
			ferror.clear(caps[i][0]);
			continue;
		}
		if (!v_range(caps[i][0], quiet, 1, 999999)) {
			ok = 0;
			continue;
		}
		any = 1;
	}
	if (ok && !any) {
		ferror.set(f[3], 'Set at least one of the download, upload or combined limits', quiet);
		ok = 0;
	}

	/*
	 * Limit-speed rules need both a download and an upload penalty speed, the
	 * way Gargoyle does: a combined cap can only throttle when both directions
	 * are shapeable, and rc reads each direction's speed independently. Clear
	 * the fields when Block is selected so a stale value can't block a save.
	 */
	if (f[12].value == '1') {
		if (!v_range(f[13], quiet, 1, 10000000))
			ok = 0;
		if (!v_range(f[14], quiet, 1, 10000000))
			ok = 0;
	}
	else {
		ferror.clear(f[13]);
		ferror.clear(f[14]);
	}

	/* records are ">" separated and fields "<" separated - neither may appear */
	if (!v_nodelim(f[15], quiet, 'Description', 1))
		ok = 0;

	return ok;
}

function verifyFields(focused, quiet) {
	var a = !E('_f_quota_enable').checked;

	E('_quota_path').disabled = a;
	E('_quota_stime').disabled = a;

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
	if (qg.isEditing())
		return;

	var data = qg.getAllData();
	var rules = '';
	var i;

	for (i = 0; i < data.length; ++i)
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

	<div class="tomato-grid" id="quota-grid"></div>
</div>

<!-- / / / -->

<div class="section-title">Notes <small><i><a href="javascript:toggleVisibility(cprefix,'notes');" id="toggleLink-notes"><span id="sesdiv_notes_showhide">(Show)</span></a></i></small></div>
<div class="section" id="sesdiv_notes" style="display:none">
	<ul>
		<li><b>Applies To</b> - listed in the same order as the drop-down:
			<ul>
				<li><b>Entire Local Network</b> - one shared counter for all LAN traffic.</li>
				<li><b>Only these Host(s)</b> - a separate counter for each listed host. Enter an IP address (192.168.1.5), a range (192.168.1.10-20) or a whole subnet in CIDR notation (192.168.2.0/24), on any LAN bridge; separate several with commas. MAC addresses are not supported: download traffic arrives from the WAN, so a MAC could only ever match uploads.</li>
				<li><b>Shared pool for these Host(s)</b> - as above, but one shared counter for everything listed. List several subnets (192.168.3.0/24, 192.168.4.0/24) to give them a single combined pool, or use one subnet per rule to give each its own.</li>
				<li><b>Each Host without a Quota</b> - a separate counter per host, for every host not covered by an explicit quota above.</li>
				<li><b>All Hosts without a Quota (shared)</b> - as above, but they share a single counter between them.</li>
			</ul>
		</li>
		<li>Limits are per reset period. Leave a limit blank for unlimited.</li>
		<li>Usage is kept in kernel memory. Set <b>Save Usage To</b> to a path on JFFS or USB storage for it to survive a reboot.</li>
		<li><b>When Exceeded</b> - <b>Block Internet Access</b> drops the traffic, or <b>Limit Speed</b> caps it to the download (&#x2193;) and upload (&#x2191;) speeds you set, in kbit/s. Speed limiting needs the queue it shapes on to be free: downloads need the Bandwidth Limiter off, uploads (and combined limits) need both the Limiter and QoS off - otherwise that direction falls back to blocking.</li>
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
