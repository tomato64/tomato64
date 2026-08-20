<!DOCTYPE html>
<!--

	Fans Page
	Copyright (C) 2026 Lance Fredrickson
	lancethepants@gmail.com

-->
<html lang="en-GB">
<head>
<meta http-equiv="content-type" content="text/html;charset=utf-8">
<meta name="robots" content="noindex,nofollow">
<title>[<% ident(); %>] Admin: Fans</title>
<link rel="stylesheet" type="text/css" href="tomato.css?rel=<% version(); %>">
<% css(); %>
<script src="tomato.js?rel=<% version(); %>"></script>

<script>

//	<% nvram("fan_mode,fan_pwm,fan_trips,fan_curve,t_model_name"); %>

var cprefix = 'admin_fans';

/* Populated from "fanctl status". Everything on this page is discovered at run
   time, so the same code covers the pwm-fan thermal zones on the MediaTek
   boards, ACPI zones and Super-I/O hwmon chips on x86_64, and fanless devices. */
var zones = [];		/* { n, type, temp, policy, policies, mode, trips[], binds[] } */
var cdevs = [];		/* { n, type, cur, max } */
var fans = [];		/* { hwmon, name, idx, pwm, enable, rpm, rw } */
var curves = [];	/* { key, hwmon, idx, points[], src } - Super-I/O chip curves */
var curveDrawn = 0;

function isFanType(t) {
	return /fan/i.test(t);
}

/* millidegrees -> display string */
function mc2s(v) {
	if (v == '-' || v == '' || isNaN(v))
		return '&#8212;';

	return (v / 1000).toFixed(1) + ' &deg;C';
}

function mc2c(v) {
	return Math.round(v / 1000);
}

function zoneByNum(n) {
	for (var i = 0; i < zones.length; ++i)
		if (zones[i].n == n) return zones[i];

	return null;
}

function cdevByNum(n) {
	for (var i = 0; i < cdevs.length; ++i)
		if (cdevs[i].n == n) return cdevs[i];

	return null;
}

/* True once we have found something that can actually move air. */
function haveFan() {
	var i;

	for (i = 0; i < fans.length; ++i)
		if (fans[i].pwm != '-' || fans[i].rpm != '-') return 1;

	for (i = 0; i < cdevs.length; ++i)
		if (isFanType(cdevs[i].type)) return 1;

	return 0;
}

/* Zones that drive a fan cooling device - the ones whose trip points are worth
   showing as a curve. */
function fanZones() {
	var out = [], i, j, c;

	for (i = 0; i < zones.length; ++i) {
		for (j = 0; j < zones[i].binds.length; ++j) {
			c = cdevByNum(zones[i].binds[j].cdev);
			if (c && isFanType(c.type)) {
				out.push(zones[i]);
				break;
			}
		}
	}

	return out;
}

function parseStatus(text) {
	var lines = text.split('\n');
	var i, j, p, z;

	zones = [];
	cdevs = [];
	fans = [];
	curves = [];

	for (i = 0; i < lines.length; ++i) {
		p = lines[i].replace(/\s+$/, '').split(';;');
		if (p.length < 2) continue;

		switch (p[0]) {
		case 'Z':
			zones.push({ n: p[1], type: p[2], temp: p[3], policy: p[4], policies: p[5], mode: p[6], trips: [], binds: [] });
			break;
		case 'T':
			if ((z = zoneByNum(p[1])) != null)
				z.trips.push({ i: p[2], type: p[3], temp: p[4], hyst: p[5], rw: (p[6] == '1') });
			break;
		case 'B':
			if ((z = zoneByNum(p[1])) != null)
				z.binds.push({ trip: p[2], cdev: p[3], type: p[4] });
			break;
		case 'C':
			cdevs.push({ n: p[1], type: p[2], cur: p[3], max: p[4] });
			break;
		case 'F':
			fans.push({ hwmon: p[1], name: p[2], idx: p[3], pwm: p[4], enable: p[5], rpm: p[6], rw: (p[7] == '1') });
			break;
		case 'L':
			for (j = 0; j < fans.length; ++j)
				if ((fans[j].hwmon == p[1]) && (fans[j].idx == p[2])) {
					fans[j].cdev = p[3];
					fans[j].cmax = p[4] * 1;
					fans[j].inverted = (p[5] == '1');
					fans[j].levels = p[6];
				}
			break;
		case 'A':
			curveFor(p[1], p[2]).points.push({ p: p[3], temp: p[4], pwm: p[5], rw: (p[6] == '1') });
			break;
		case 'X':
			curveFor(p[1], p[2]).src = { kind: p[3], raw: p[4], label: (p[5] == '-' ? 'sensor ' + p[4] : p[5]), temp: p[6] };
			break;
		}
	}
}

/* shell.cgi with nojs=1 answers in plain text, and TomatoRefresh hands the body
   straight to refresh() without eval'ing it, so no .jsx shim is needed.
   Fixed interval with no cookie tag, so no footer dropdown or button - same
   shape as admin-port-labels.asp. */
var ref = new TomatoRefresh('shell.cgi', 'action=execute&nojs=1&command=' + escapeCGI('fanctl status'), 3);

ref.refresh = function(text) {
	parseStatus(text);
	displayHardware();
	displayTemps();

	/* Drawn once. Rebuilding these on every poll would throw away a
	   half-typed trip temperature or duty cycle. */
	if (!curveDrawn) {
		displayCurve();
		displayManual();
		verifyFields(null, 1);
		curveDrawn = 1;
	}

	updateCurveStatus();
};

/* A cooling device carries no sysfs link back to the driver that created it -
   __thermal_cooling_device_register() sets neither device.parent nor of_node -
   so an hwmon fan and a cooling device can only be matched by name. pwm-fan
   registers as "pwmfan" for hwmon and "pwm-fan" for thermal (hwmon rejects '-'
   in a name), gpio-fan as "gpio_fan" and "gpio-fan", so dropping the
   punctuation lines them up. */
function normName(s) {
	return String(s).toLowerCase().replace(/[^a-z0-9]/g, '');
}

/* One row per physical fan where we can prove two sysfs entries are the same
   device, otherwise one row each. Merging only happens when the name is
   unambiguous on both sides, so two identical fans - or a Super-I/O chip
   sitting next to an unrelated ACPI fan - are never folded together. */
function mergeFans() {
	var rows = [], cd = [], hc = {}, cc = {}, used = [];
	var i, j, f, c, n;

	for (i = 0; i < cdevs.length; ++i)
		if (isFanType(cdevs[i].type)) cd.push(cdevs[i]);

	for (i = 0; i < fans.length; ++i) {
		if (fans[i].pwm == '-') continue;
		n = normName(fans[i].name);
		hc[n] = (hc[n] || 0) + 1;
	}
	for (i = 0; i < cd.length; ++i) {
		n = normName(cd[i].type);
		cc[n] = (cc[n] || 0) + 1;
	}

	for (i = 0; i < fans.length; ++i) {
		f = fans[i];
		n = normName(f.name);
		c = null;
		if (f.pwm != '-' && hc[n] == 1 && cc[n] == 1) {
			for (j = 0; j < cd.length; ++j) {
				if (normName(cd[j].type) != n) continue;
				c = cd[j];
				used.push(c.n);
				break;
			}
		}
		rows.push({ f: f, c: c });
	}

	for (i = 0; i < cd.length; ++i) {
		if (used.indexOf(cd[i].n) >= 0) continue;
		rows.push({ f: null, c: cd[i] });
	}

	return rows;
}

function displayHardware() {
	var rows = mergeFans();
	var html = '', i, r, name, where, pct;

	for (i = 0; i < rows.length; ++i) {
		r = rows[i];

		if (r.f) {
			name = r.f.name + ' #' + r.f.idx;
			where = r.f.hwmon;
		}
		else {
			name = r.c.type;
			where = 'cooling device ' + r.c.n;
		}

		html += '<tr>';
		html += '<td>' + escapeHTML(name) + ' <small>(' + escapeHTML(where) + ')<\/small><\/td>';

		if (r.f && r.f.pwm != '-') {
			pct = dutyPct(r.f, r.f.pwm);
			html += '<td>' + escapeHTML(r.f.pwm) + ' / 255 &nbsp;<small>(' + pct + '%' +
				(r.f.inverted ? ', inverted' : '') + ')<\/small><\/td>';
		}
		else
			html += '<td>&#8212;<\/td>';

		html += '<td>' + ((r.f && r.f.rpm != '-') ? escapeHTML(r.f.rpm) + ' RPM' : '&#8212;') + '<\/td>';
		html += '<\/tr>';
	}

	if (html == '')
		html = '<tr><td colspan="3"><b>No fan hardware detected on this device.<\/b><\/td><\/tr>';

	E('fan-hw').innerHTML = html;
}

function displayTemps() {
	var html = '', i, z;

	for (i = 0; i < zones.length; ++i) {
		z = zones[i];
		html += '<tr>';
		html += '<td>' + escapeHTML(z.type) + ' <small>(thermal_zone' + escapeHTML(z.n) + ')<\/small><\/td>';
		html += '<td>' + mc2s(z.temp) + '<\/td>';
		html += '<td>' + (z.policy == '-' ? '&#8212;' : escapeHTML(z.policy)) + '<\/td>';
		html += '<\/tr>';
	}

	if (html == '')
		html = '<tr><td colspan="3">No thermal zones reported by the kernel.<\/td><\/tr>';

	E('fan-temps').innerHTML = html;
}

/* What a trip drives, from the zone's cdevN_trip_point bindings. The cooling
   level the trip commands comes from the device tree (cooling-device = <&fan
   lower upper>) and is not exposed in sysfs, so all we can honestly report is
   which cooling device the trip is wired to. */
function tripDrives(z, trip) {
	var out = [], j, c;

	for (j = 0; j < z.binds.length; ++j) {
		if (z.binds[j].trip != trip) continue;
		if ((c = cdevByNum(z.binds[j].cdev)) != null)
			out.push(escapeHTML(c.type));
	}

	return out.length ? out.join(', ') : '&#8212;';
}

/* Is the zone currently above this trip?

   The kernel compares against td->threshold, which drops to (temperature -
   hysteresis) once a trip has been crossed so the fan does not chatter at the
   boundary. That threshold is internal to the thermal core and is not exposed
   in sysfs, so this is the plain comparison: while the temperature is falling
   back through a trip we can read "below" for up to the hysteresis before the
   kernel agrees. */
function tripCrossed(z, t) {
	if (z.temp == '-' || t.temp == '-') return 0;

	return (z.temp * 1) >= (t.temp * 1);
}

/* The crossed fan-driving trip with the highest temperature - the one setting
   the current fan speed. */
function governingTrip(z) {
	var best = null, j, t;

	for (j = 0; j < z.trips.length; ++j) {
		t = z.trips[j];
		if (tripDrives(z, t.i) == '&#8212;') continue;
		if (!tripCrossed(z, t)) continue;
		if (best == null || (t.temp * 1) > (best.temp * 1)) best = t;
	}

	return best;
}

function displayCurve() {
	var fz = fanZones();
	var html = '', i, j, z, t, cls;

	if (fz.length == 0 && curves.length == 0) {
		E('fan-curve').innerHTML = '<div class="fields">No fan associated with a thermal zone was found.<\/div>';
		return;
	}

	for (i = 0; i < fz.length; ++i) {
		z = fz[i];
		html += '<div class="fields"><b>' + escapeHTML(z.type) + '<\/b> <small>(thermal_zone' + escapeHTML(z.n) + ')<\/small> <span id="zhdr_' + escapeHTML(z.n) + '"><\/span><\/div>';
		html += '<table class="fan-table">';
		html += '<tr><th>Trip<\/th><th>Type<\/th><th>Temperature<\/th><th>Hysteresis<\/th><th>Drives<\/th><\/tr>';

		for (j = 0; j < z.trips.length; ++j) {
			t = z.trips[j];
			cls = (t.type == 'critical' || t.type == 'hot') ? ' class="fan-crit"' : '';
			html += '<tr' + cls + '>';
			html += '<td>' + escapeHTML(t.i) + '<span id="act_' + escapeHTML(z.n) + '_' + escapeHTML(t.i) + '"><\/span><\/td>';
			html += '<td>' + escapeHTML(t.type) + '<\/td>';
			if (t.rw && t.temp != '-')
				html += '<td><input type="text" size="5" maxlength="3" onchange="verifyFields(this, 1)" id="trip_' + escapeHTML(z.n) + '_' + escapeHTML(t.i) + '" value="' + mc2c(t.temp) + '"> &deg;C<\/td>';
			else
				html += '<td>' + mc2s(t.temp) + ' <small>(read-only)<\/small><\/td>';
			html += '<td>' + mc2s(t.hyst) + '<\/td>';
			html += '<td>' + tripDrives(z, t.i) + '<\/td>';
			html += '<\/tr>';
		}
		html += '<\/table>';
	}

	E('fan-curve').innerHTML = html + displayChipCurves();
}

/* Refreshed on every poll. Kept separate from displayCurve() so that live
   updates never rebuild the table and throw away half-typed trip values. */
function updateCurveStatus() {
	var fz = fanZones();
	var i, j, z, t, gov, e;

	for (i = 0; i < fz.length; ++i) {
		z = fz[i];

		/* In manual mode the governor is not driving anything, so there is no
		   active trip to point at. Keyed off the zone's live governor rather
		   than nvram, so it follows what the hardware is actually doing. */
		gov = (z.policy == 'user_space') ? null : governingTrip(z);

		if ((e = E('zhdr_' + z.n)) != null)
			e.innerHTML = '&#8212; ' + mc2s(z.temp);

		for (j = 0; j < z.trips.length; ++j) {
			t = z.trips[j];

			if ((e = E('act_' + z.n + '_' + t.i)) != null)
				e.innerHTML = (gov != null && gov.i == t.i) ?
					'<span class="fan-act" title="Currently driving the fan">&#10003;<\/span>' : '';
		}
	}

	updateChipCurveStatus();
}

/* --------------------------------------------------------- chip fan curve ---
   x86 Super-I/O chips (nct6775, it87, f71882fg) run a fan curve in silicon,
   entirely outside the kernel thermal framework - on those boards no cooling
   device is bound to a thermal zone, so this is the only curve there is. The
   attributes are the same on all three: pwmN_auto_pointM_temp / _pwm, with
   pwmN_enable == 1 meaning manual and anything >= 2 an automatic mode. */

function curveFor(hwmon, idx) {
	var key = hwmon + ':' + idx, i;

	for (i = 0; i < curves.length; ++i)
		if (curves[i].key == key) return curves[i];

	curves.push({ key: key, hwmon: hwmon, idx: idx, points: [], src: null });

	return curves[curves.length - 1];
}

/* Live duty and mode for the channel a curve belongs to. */
function curveFan(c) {
	var i;

	for (i = 0; i < fans.length; ++i)
		if ((fans[i].hwmon == c.hwmon) && (fans[i].idx == c.idx)) return fans[i];

	return null;
}

function curveTitle(c) {
	var f = curveFan(c);

	return (f ? f.name + ' #' + f.idx : c.hwmon + ' pwm' + c.idx);
}

/* Points the user can edit, in order. */
function curveInputs(c) {
	var out = [], j, et, ep;

	for (j = 0; j < c.points.length; ++j) {
		if (!c.points[j].rw) continue;
		et = E('cvt_' + c.hwmon + '_' + c.idx + '_' + c.points[j].p);
		ep = E('cvp_' + c.hwmon + '_' + c.idx + '_' + c.points[j].p);
		if (et == null || ep == null) continue;
		out.push({ pt: c.points[j], t: et, p: ep });
	}

	return out;
}

/* The driver only revalidates the curve when the automatic mode is reselected;
   nct6775's check_trip_points() then refuses it outright if temperatures or
   duties step downwards. Catch that here instead of letting the mode switch
   fail silently after the values are already written. */
function verifyCurves(focused, quiet) {
	var ok = 1, i, j, list, prevT, prevP, tv, pv;

	for (i = 0; i < curves.length; ++i) {
		list = curveInputs(curves[i]);
		prevT = null;
		prevP = null;

		for (j = 0; j < list.length; ++j) {
			if (!v_range(list[j].t, quiet || (focused != list[j].t), 0, 200)) { ok = 0; continue; }
			if (!v_range(list[j].p, quiet || (focused != list[j].p), 0, 255)) { ok = 0; continue; }

			tv = parseInt(list[j].t.value, 10);
			pv = parseInt(list[j].p.value, 10);

			if (prevT != null && tv < prevT) {
				ferror.set(list[j].t, 'Each point must be at least as warm as the one above it', quiet);
				ok = 0;
			}
			else ferror.clear(list[j].t);

			if (prevP != null && pv < prevP) {
				ferror.set(list[j].p, 'Each point must be at least as fast as the one above it', quiet);
				ok = 0;
			}
			else ferror.clear(list[j].p);

			prevT = tv;
			prevP = pv;
		}
	}

	return ok;
}

/* Overrides already saved, as { "hwmon:idx:point": "temp/pwm" }. */
function savedCurve() {
	var map = {}, parts = nvram.fan_curve.split(','), i, kv;

	for (i = 0; i < parts.length; ++i) {
		if ((kv = parts[i].split('=')).length != 2) continue;
		if (kv[0] == '' || kv[1].indexOf('/') < 0) continue;
		map[kv[0]] = kv[1];
	}

	return map;
}

/* "hwmonX:N:P=millidegrees/duty". Like the trip points, only rows the user
   actually changed are added, so opening the page and pressing Save does not
   pin the firmware's whole curve into NVRAM. */
function serializeCurve() {
	var map = savedCurve();
	var out = [], i, j, list, k, tv, pv;

	for (i = 0; i < curves.length; ++i) {
		list = curveInputs(curves[i]);
		for (j = 0; j < list.length; ++j) {
			tv = parseInt(list[j].t.value, 10) * 1000;
			pv = parseInt(list[j].p.value, 10);
			if ((tv == list[j].pt.temp) && (pv == list[j].pt.pwm)) continue;
			map[curves[i].key + ':' + list[j].pt.p] = tv + '/' + pv;
		}
	}

	for (k in map)
		out.push(k + '=' + map[k]);

	return out.join(',');
}

function displayChipCurves() {
	var html = '', i, j, c, pt, id;

	for (i = 0; i < curves.length; ++i) {
		c = curves[i];
		if (c.points.length == 0) continue;

		html += '<div class="fields"><b>' + escapeHTML(curveTitle(c)) + '<\/b> <small>(' +
			escapeHTML(c.hwmon) + ' pwm' + escapeHTML(c.idx) + ')<\/small> ' +
			'<span id="cvh_' + escapeHTML(c.hwmon) + '_' + escapeHTML(c.idx) + '"><\/span><\/div>';
		html += '<table class="fan-table">';
		html += '<tr><th>Point<\/th><th>Temperature<\/th><th>Duty Cycle<\/th><\/tr>';

		for (j = 0; j < c.points.length; ++j) {
			pt = c.points[j];
			id = escapeHTML(c.hwmon) + '_' + escapeHTML(c.idx) + '_' + escapeHTML(pt.p);
			html += '<tr><td>' + escapeHTML(pt.p) + '<\/td>';
			if (pt.rw && pt.temp != '-' && pt.pwm != '-') {
				html += '<td><input type="text" size="5" maxlength="3" onchange="verifyFields(this, 1)" id="cvt_' + id + '" value="' + mc2c(pt.temp) + '"> &deg;C<\/td>';
				html += '<td><input type="text" size="5" maxlength="3" onchange="verifyFields(this, 1)" id="cvp_' + id + '" value="' + escapeHTML(pt.pwm) + '"> / 255<\/td>';
			}
			else {
				html += '<td>' + mc2s(pt.temp) + ' <small>(read-only)<\/small><\/td>';
				html += '<td>' + escapeHTML(pt.pwm) + ' / 255<\/td>';
			}
			html += '<\/tr>';
		}
		html += '<\/table>';
	}

	return html;
}

/* Refreshed on every poll, like the thermal-zone headers. */
function updateChipCurveStatus() {
	var i, c, f, e, txt;

	for (i = 0; i < curves.length; ++i) {
		c = curves[i];
		if ((e = E('cvh_' + c.hwmon + '_' + c.idx)) == null) continue;

		f = curveFan(c);
		txt = '';
		if (c.src != null) {
			txt = '&#8212; follows ' + escapeHTML(c.src.label);
			if (c.src.temp != '-') txt += ' (' + mc2s(c.src.temp) + ')';
		}
		if (f != null && f.pwm != '-')
			txt += (txt === '' ? '&#8212; ' : ', ') + 'now at ' + escapeHTML(f.pwm) + ' / 255';
		if (f != null && f.enable == '1')
			txt += ' <b>[manual - curve not in use]<\/b>';

		e.innerHTML = txt;
	}
}


/* fan_pwm is either a bare number - one speed for every fan, which is what
   configs saved before per-fan control contain - or a list of "hwmonX:N=duty"
   items. Returns the per-channel map plus the bare number to fall back on. */
function savedPwm() {
	var map = {}, global = null;
	var parts = nvram.fan_pwm.split(','), i, kv;


	for (i = 0; i < parts.length; ++i) {
		if ((kv = parts[i].split('=')).length == 2) {
			if (kv[0] != '' && (kv[1] == 'auto' || !isNaN(kv[1]))) map[kv[0]] = kv[1];
		}
		else if (parts[i] != '' && !isNaN(parts[i]))
			global = parts[i];
	}

	/* hasGlobal distinguishes "every fan manual at this speed" - which is what
	   a pre-per-fan config contains - from "nothing said", where a channel that
	   is not named should be left automatic. */
	return { map: map, global: (global == null) ? '128' : global, hasGlobal: (global != null) };
}

/* A fan bound to a thermal cooling device is addressed by cooling level, not by
   duty cycle.

   That is not cosmetic. Boards whose PWM line is inverted describe it with
   descending cooling-levels - BPI-R3 <255 40 0>, BPI-R3 Mini <255 128 80 0> -
   so on those, duty 255 is fan OFF and 0 is full speed. Worse,
   pwm_fan_update_state() assumes the list ascends, so writing a duty makes the
   driver record the opposite cooling state; set_cur_state() then early-returns
   believing it is already there, and the fan cannot be moved at all until the
   state genuinely changes. Levels avoid all of it: 0 is always least cooling. */
function fanKey(f) {
	return f.hwmon + ':' + f.idx;
}

/* How fast the fan is actually turning, as a percentage, given that a higher
   duty means a slower fan on the inverted boards. */
function dutyPct(f, duty) {
	return Math.round((f.inverted ? (255 - duty) : (duty * 1)) * 100 / 255);
}

/* Every pwm channel the kernel will let us write, with its Auto/Manual selector
   and its speed control. */
function manualInputs() {
	var out = [], i, f, e;

	for (i = 0; i < fans.length; ++i) {
		f = fans[i];
		if (!f.rw) continue;
		if ((e = E('mpwm_' + f.hwmon + '_' + f.idx)) == null) continue;
		out.push({ key: fanKey(f), e: e, m: E('mmode_' + f.hwmon + '_' + f.idx) });
	}

	return out;
}

function displayManual() {
	var saved = savedPwm();
	var html = '', i, f, key, val, auto;

	for (i = 0; i < fans.length; ++i) {
		f = fans[i];
		if (!f.rw) continue;
		key = fanKey(f);
		val = (saved.map[key] != null) ? saved.map[key] :
			(saved.hasGlobal ? saved.global : 'auto');
		auto = (val == 'auto');
		if (auto) val = saved.global;

		html += '<tr><td>' + escapeHTML(f.name) + ' #' + escapeHTML(f.idx) + ' <small>(' + escapeHTML(f.hwmon) + ')<\/small><\/td>';
		html += '<td><select onchange="verifyFields(this, 1)" id="mmode_' + escapeHTML(f.hwmon) + '_' + escapeHTML(f.idx) + '">';
		html += '<option value="auto"' + (auto ? ' selected="selected"' : '') + '>Automatic<\/option>';
		html += '<option value="manual"' + (auto ? '' : ' selected="selected"') + '>Manual<\/option>';
		html += '<\/select><\/td>';
		html += '<td><input type="text" size="5" maxlength="3" onchange="verifyFields(this, 1)" id="mpwm_' + escapeHTML(f.hwmon) + '_' + escapeHTML(f.idx) + '" value="' + escapeHTML(val) + '"> <small>' +
			(f.inverted ? '(0 - 255, <b>0<\/b> = full speed on this board)' : '(0 - 255, 255 = full speed)') +
			'<\/small><\/td><\/tr>';
	}

	if (html == '') {
		E('fan-manual').innerHTML = '<div class="fields">No fan on this device accepts a duty cycle.<\/div>';
		return;
	}

	E('fan-manual').innerHTML = '<table class="fan-table"><tr><th>Fan<\/th><th>Control<\/th><th>Duty Cycle<\/th><\/tr>' + html + '<\/table>';
}

/* "hwmonX:N=duty" list. Written even for a single fan so the saved value keeps
   naming what it applies to. */
function serializePwm() {
	var list = manualInputs(), out = [], i;

	if (list.length == 0)
		return nvram.fan_pwm;	/* nothing discovered - do not wipe the setting */

	for (i = 0; i < list.length; ++i)
		out.push(list[i].key + '=' +
			((list[i].m && list[i].m.value == 'auto') ? 'auto' : parseInt(list[i].e.value, 10)));

	return out.join(',');
}

/* Overrides already saved, as { "zone:trip": millidegrees }. A saved override is
   live by the time the page loads (fanctl applied it at boot), so it reads back
   as the current trip temperature and has to be carried forward explicitly -
   otherwise the next Save would silently drop it. */
function savedTrips() {
	var map = {}, parts = nvram.fan_trips.split(','), i, kv;

	for (i = 0; i < parts.length; ++i) {
		if ((kv = parts[i].split('=')).length != 2) continue;
		if (kv[0] == '' || isNaN(kv[1])) continue;
		map[kv[0]] = kv[1];
	}

	return map;
}

/* "zone:trip=millidegrees", comma separated - the format fanctl apply reads.
   Only rows the user changed are added, so opening the page and pressing Save
   does not pin the whole device-tree curve into NVRAM. */
function serializeTrips() {
	var fz = fanZones();
	var map = savedTrips();
	var out = [], i, j, z, t, e, k, v;

	for (i = 0; i < fz.length; ++i) {
		z = fz[i];
		for (j = 0; j < z.trips.length; ++j) {
			t = z.trips[j];
			if (!t.rw || t.temp == '-') continue;
			if ((e = E('trip_' + z.n + '_' + t.i)) == null) continue;
			v = parseInt(e.value, 10) * 1000;
			if (v == t.temp) continue;
			map[z.n + ':' + t.i] = v;
		}
	}

	for (k in map)
		out.push(k + '=' + map[k]);

	return out.join(',');
}

function verifyFields(focused, quiet) {
	var fz = fanZones();
	var ok = 1, i, j, z, t, e, mi, man, auto;

	for (i = 0; i < fz.length; ++i) {
		z = fz[i];
		for (j = 0; j < z.trips.length; ++j) {
			t = z.trips[j];
			if (!t.rw) continue;
			if ((e = E('trip_' + z.n + '_' + t.i)) == null) continue;
			if (!v_range(e, quiet || (focused != e), 20, 200)) ok = 0;
		}
	}

	if (!verifyCurves(focused, quiet)) ok = 0;

	mi = manualInputs();
	man = E('_f_fan_manual') ? E('_f_fan_manual').checked : 0;
	for (i = 0; i < mi.length; ++i) {
		auto = (mi[i].m != null && mi[i].m.value == 'auto');
		if (mi[i].m != null) mi[i].m.disabled = !man;
		mi[i].e.disabled = !man || auto;
		if (man && !auto && mi[i].e.tagName != 'SELECT' &&
		    !v_range(mi[i].e, quiet || (focused != mi[i].e), 0, 255)) ok = 0;
	}

	return ok;
}

/* form.submit() greys out save-button and cancel-button by id, and re-enables
   them from the submit_complete() hook. This page has a third footer button, so
   it rides along on the same hook rather than teaching tomato.js about it. */
function lockFooter(lock) {
	var e;

	if ((e = E('defaults-button')) != null)
		e.disabled = lock ? 1 : 0;
}

function submit_complete() {
	lockFooter(0);

	/* Reload rather than just unlocking the buttons. The curve and manual
	   tables are drawn once from what the hardware reports, and a save - in
	   particular Restore Defaults - changes that behind them, so they would
	   otherwise keep showing the old values. It also re-reads nvram.fan_*,
	   which the carry-forward in serializeTrips()/serializeCurve()/
	   serializePwm() starts from: stale there, the next Save would re-emit
	   overrides that Restore Defaults had just cleared. Same approach as
	   admin-sdhc.asp and advanced-routing.asp. */
	reloadPage();
}

function save() {
	if (!verifyFields(null, 0)) return;

	var fom = E('t_fom');
	var trips = serializeTrips();
	var curve = serializeCurve();

	if (trips.length > 512) {
		alert('Too many trip points to store. Please report this.');
		return;
	}

	fom.fan_mode.value = E('_f_fan_manual').checked ? 1 : 0;
	fom.fan_pwm.value = serializePwm();
	fom.fan_trips.value = trips;
	fom.fan_curve.value = curve;
	fom._service.value = 'fan-restart';
	lockFooter(1);
	form.submit(fom, 1);
}

function restoreDefaults() {
	if (!confirm('Clear all saved trip point overrides and go back to the firmware curve?')) return;

	var fom = E('t_fom');
	fom.fan_mode.value = 0;
	fom.fan_pwm.value = nvram.fan_pwm;
	fom.fan_trips.value = '';
	fom.fan_curve.value = '';
	fom._service.value = 'fan-restart';
	lockFooter(1);
	form.submit(fom, 1);
}

function earlyInit() {
	var c;

	if (((c = cookie.get(cprefix+'_notes_vis')) != null) && (c == '1'))
		toggleVisibility(cprefix, 'notes');

	verifyFields(null, 1);
	ref.initPage(0, 3);
}

</script>

<style>
.fan-table {
	width: 100%;
}
.fan-table th {
	text-align: left;
	padding: 5px;
	border-bottom: 1px solid #444;
}
.fan-table td {
	padding: 5px;
	border-bottom: 1px solid #333;
}
.fan-crit td {
	color: #ff6f59;
}
/* The base themes give every footer button a flat 80px, which truncates this
   one to "Restore Defa". The advanced themes already use width:unset. */
#footer input#defaults-button {
	width: auto;
	padding-left: 10px;
	padding-right: 10px;
}
.fan-act {
	display: inline-block;
	min-width: 1em;
	margin-left: 6px;
	padding: 0 4px;
	text-align: center;
	color: #ffffff;
	background: #6fbf3d;
	border-radius: 3px;
	font-weight: bold;
}
#sesdiv_notes ul > li {
	margin-bottom: 8px;
}
#sesdiv_notes ul ul > li {
	margin-bottom: 2px;
}
</style>

</head>

<body>
<form id="t_fom" method="post" action="tomato.cgi">
<table id="container">
<tr><td colspan="2" id="header">
	<div class="title"><a href="/">Tomato64</a></div>
	<div class="version">Version <% version(); %> on <% nv("t_model_name"); %></div>
</td></tr>
<tr id="body"><td id="navi"><script>navi()</script></td>
<td id="content">
<div id="ident"><% ident(); %> | <script>wikiLink();</script></div>

<!-- / / / -->

<input type="hidden" name="_nextpage" value="admin-fans.asp">
<input type="hidden" name="_service" value="">
<input type="hidden" name="fan_mode">
<input type="hidden" name="fan_pwm">
<input type="hidden" name="fan_trips">
<input type="hidden" name="fan_curve">

<!-- / / / -->

<div class="section-title">Fan Hardware</div>
<div class="section">
	<table class="fan-table">
		<tr>
			<th>Fan</th>
			<th>Duty Cycle</th>
			<th>Speed</th>
		</tr>
		<tbody id="fan-hw">
			<tr><td colspan="3">Loading...</td></tr>
		</tbody>
	</table>
</div>

<!-- / / / -->

<div class="section-title">Temperatures</div>
<div class="section">
	<table class="fan-table">
		<tr>
			<th>Thermal Zone</th>
			<th>Temperature</th>
			<th>Governor</th>
		</tr>
		<tbody id="fan-temps">
			<tr><td colspan="3">Loading...</td></tr>
		</tbody>
	</table>
</div>

<!-- / / / -->

<div class="section-title">Fan Curve</div>
<div class="section" id="fan-curve">
	Loading...
</div>

<!-- / / / -->

<div class="section-title">Manual Speed</div>
<div class="section">
	<script>
		createFieldTable('', [
			{ title: 'Set fan speed manually', name: 'f_fan_manual', type: 'checkbox', value: (nvram.fan_mode == '1'), suffix: '&nbsp;<small>(hands the fan to you instead of the kernel)<\/small>' }
		]);
	</script>
	<div id="fan-manual">Loading...</div>
</div>

<!-- / / / -->

<div class="section-title">Notes <small><i><a href="javascript:toggleVisibility(cprefix,'notes');" id="toggleLink-notes"><span id="sesdiv_notes_showhide">(Show)</span></a></i></small></div>
<div class="section" id="sesdiv_notes" style="display:none">
	<ul>
		<li><b>About</b> - Everything here is read from the kernel at run time. On device-tree boards the fan
			is a cooling device on a thermal zone, driven by the trip points below. On x86 it belongs to a
			Super-I/O chip (Nuvoton, ITE, Fintek, Winbond) running its own curve, and appears only if the
			kernel has a driver for it.</li>
		<li><b>Duty Cycle</b> - The raw PWM register, 0 - 255, and the only unit used here.</li>

		<li><b>Inverted fans</b> - On some boards a higher duty means a <i>slower</i> fan, and 0 is full speed.
			This is read from the board's description, not assumed, so the field says which way round it is
			and the percentage shown is real fan speed rather than the raw register.</li>

		<li><b>Usable range</b> - Few fans use all of 0 - 255. Past some point at the slow end the fan stops
			turning altogether: a low duty on a normal board, a high one on an inverted board. Where that
			sits depends on the fan, not the board, so find it by ear.</li>
		<li><b>Green check</b> - Marks the trip currently setting the fan speed: the highest one the zone has
			climbed above that drives the fan. It is a plain comparison of the zone temperature against each
			trip. The kernel adds hysteresis once a trip has been crossed, and that adjusted threshold is not
			exposed in sysfs, so while the temperature is falling back through a trip the mark can move down
			up to the hysteresis before the kernel acts on it. No trip is marked in manual mode, because then
			the governor is not choosing the speed at all.</li>
		<li><b>Fan Curve</b> - Each trip point tells the kernel to change the fan speed once the
			zone reaches that temperature. Lower a trip to make the fan spin up sooner and run cooler and
			louder; raise it for a quieter box. Values are saved to NVRAM and reapplied on every boot.</li>
		<li><b>Fan Curve on x86</b> - Super-I/O chips (Nuvoton, ITE, Fintek) run their own fan curve in
			silicon, and on x86 no cooling device is bound to a thermal zone, so that chip curve is the only
			curve there is. Each point says "at this temperature, run at this duty"; the chip interpolates
			between them. Points must step upwards in both columns - the driver refuses the whole curve
			otherwise. The curve lives in the chip and survives a reboot on its own; saving here just means
			Tomato64 can put it back after a BIOS reset.</li>
		<li><b>critical and hot trips</b> - Shown in red. The <i>critical</i> trip shuts the device down as soon
			as it is crossed, and <i>hot</i> notifies userspace; neither one drives the fan. Do not lower them
			to a temperature the device reaches in normal use or it will power off.</li>
		<li><b>Restore Defaults</b> - Clears the saved overrides and puts the firmware's own settings back
			straight away - trip points on a device-tree board, curve points on a Super-I/O chip. Neither the
			kernel nor the chip keeps a copy once a value has been overwritten, so <tt>fanctl</tt> records
			them once per boot, before applying anything, and replays them from there.</li>
		<li><b>Manual Speed</b> - Switches the fan's thermal zone to the <i>user_space</i> governor and writes the
			duty cycle directly. Critical and hot trips are still handled by the kernel, so thermal shutdown
			protection stays active - but nothing will spin the fan up for you when the device gets hot.
			Leave this off unless you have a reason. Each fan is set independently, and one left on
			<i>Automatic</i> is not touched at all.</li>
		<li><b>Mixing Automatic and Manual</b> - On a Super-I/O chip (x86) this is per channel: a fan left on
			Automatic keeps whatever mode the BIOS gave it. A fan driven by a thermal zone is different,
			because a governor drives every cooling device bound to its zone and there is no way to release
			just one. So a zone is only handed over when no fan in it is left on Automatic - leave one
			automatic and the kernel stays in charge of that whole zone, which may overrule a manual speed
			set on another fan in the same zone.</li>
		<li><b>x86_64</b> - If no fan is listed but the board has one, the fan is most likely controlled entirely
			by the BIOS/EC. Check <tt>dmesg</tt> for an "ACPI region conflict" from a hwmon driver; if you see
			one, add <tt>acpi_enforce_resources=lax</tt> to the kernel command line.</li>
	</ul>
</div>

<!-- / / / -->

<div id="footer">
	<span id="footer-msg"></span>
	<input type="button" value="Restore Defaults" id="defaults-button" onclick="restoreDefaults()">
	<input type="button" value="Save" id="save-button" onclick="save()">
	<input type="button" value="Cancel" id="cancel-button" onclick="reloadPage();">
</div>

</td></tr>
</table>
</form>
<script>earlyInit()</script>
</body>
</html>
