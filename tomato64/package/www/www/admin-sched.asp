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
<title>[<% ident(); %>] Admin: Scheduler</title>
<link rel="stylesheet" type="text/css" href="tomato.css?rel=<% version(); %>">
<% css(); %>
<script src="tomato.js?rel=<% version(); %>"></script>

<script>

//	<% nvram("sch_rboot,sch_rcon,sch_c1,sch_c1_cmd,sch_c2,sch_c2_cmd,sch_c3,sch_c3_cmd,sch_c4,sch_c4_cmd,sch_c5,sch_c5_cmd"); %>

var dowNames = ['Sun', 'Mon', 'Tue', 'Wed', 'Thu', 'Fri', 'Sat'];
var dowLow = ['sun', 'mon', 'tue', 'wed', 'thu', 'fri', 'sat'];
var scheds = []

tm = [];
tm.push([0, timeString(0)]);
tm.push([1, timeString(1)]);
for (i = 15; i < 1440; i += 15) {
	tm.push([i, timeString(i)]);
}
tm.push([1439, timeString(1439)]);

tm.push(
	[-1, 'Every minute'], [-3, 'Every 3 minutes'], [-5, 'Every 5 minutes'], [-15, 'Every 15 minutes'], [-30, 'Every 30 minutes'],
	[-60, 'Every hour'], [-(12 * 60), 'Every 12 hours'], [-(24 * 60), 'Every 24 hours'],
	['e', 'Every...']);

/* REMOVE-BEGIN
sch_* = en,time,days
REMOVE-END */

function makeSched(key, custom) {
	var s, v, w, a, t, i;
	var oe;

	scheds.push(key);

	s = nvram['sch_' + key] || '';
	if ((v = s.match(/^(0|1),(-?\d+),(\d+)$/)) == null) {
		v = custom ? ['', 0, -30, 0] : ['', 0, 0, 0];
	}
	w = v[3] * 1;
	if (w <= 0) w = 0xFF;

	key = key + '_';

	if (custom) {
		t = tm;
	}
	else {
		t = [];
		for (i = 0; i < tm.length; ++i) {
			if ((tm[i][0] >= 0) || (tm[i][0] <= -60) || (tm[i][0] == 'e')) t.push(tm[i]);
		}
	}

	oe = 1;
	for (i = 0; i < t.length; ++i) {
		if (v[2] == t[i][0]) {
			oe = 0;
			break;
		}
	}

	a = [
		{ title: 'Enabled', name: key + 'enabled', type: 'checkbox', value: v[1] == '1' },
		{ title: 'Time', multi: [
			{ name: key + 'time', type: 'select', options: t, value: oe ? 'e' : v[2] },
			{ name: key + 'every', type: 'text', maxlen: 10, size: 10, value: (v[2] < 0) ? -v[2] : 30,
				prefix: ' ', suffix: ' <small id="_' + key + 'mins"><i>minutes<\/i><\/small>' } ] },
		{ title: 'Days', multi: [
			{ name: key + 'sun', type: 'checkbox', suffix: ' Sun &nbsp; ', value: w & 1 },
			{ name: key + 'mon', type: 'checkbox', suffix: ' Mon &nbsp; ', value: w & 2 },
			{ name: key + 'tue', type: 'checkbox', suffix: ' Tue &nbsp; ', value: w & 4 },
			{ name: key + 'wed', type: 'checkbox', suffix: ' Wed &nbsp; ', value: w & 8 },
			{ name: key + 'thu', type: 'checkbox', suffix: ' Thu &nbsp; ', value: w & 16 },
			{ name: key + 'fri', type: 'checkbox', suffix: ' Fri &nbsp; ', value: w & 32 },
			{ name: key + 'sat', type: 'checkbox', suffix: ' Sat &nbsp; &nbsp;', value: w & 64 },
			{ name: key + 'everyday', type: 'checkbox', suffix: ' Everyday', value: (w & 0x7F) == 0x7F } ] }
	];

	if (custom) {
		a.push({ title: 'Command', name: 'sch_' + key + 'cmd', type: 'textarea', value: nvram['sch_' + key + 'cmd' ] });
	}

	createFieldTable('', a);
}

function verifySched(focused, quiet, key) {
	var e, f, i, n, b;
	var eTime, eEvery, eEveryday, eCmd;

	key = '_' + key + '_';

	eTime = E(key + 'time');
	eEvery = E(key + 'every');
	eEvery.style.display = E(key + 'mins').style.display = ((eTime.value == 'e') ? 'inline-block' : 'none');

	eCmd = E('_sch' + key + 'cmd');
	eEveryday = E(key + 'everyday');

	if (E(key + 'enabled').checked) {
		eEveryday.disabled = 0;
		eTime.disabled = 0;
		eEvery.disabled = 0;
		if (eCmd) eCmd.disabled = 0;

		if (focused == eEveryday) {
			for (i = 0; i < 7; ++i) {
				f = E(key + dowLow[i]);
				f.disabled = 0;
				f.checked = eEveryday.checked;
			}
		}
		else {
			n = 0;
			for (i = 0; i < 7; ++i) {
				f = E(key + dowLow[i]);
				f.disabled = 0;
				if (f.checked) ++n;
			}
			eEveryday.checked = (n == 7);
		}

		if ((eTime.value == 'e') && (!v_mins(eEvery, quiet, eCmd ? 1 : 60, 60 * 24 * 60))) return 0;

		if ((eCmd) && (!v_length(eCmd, quiet, quiet ? 0 : 1, 2048))) return 0;
	}
	else {
		eEveryday.disabled = 1;
		eTime.disabled = 1;
		eEvery.disabled = 1;
		for (i = 0; i < 7; ++i) {
			E(key + dowLow[i]).disabled = 1;
		}
		if (eCmd) eCmd.disabled = 1;
	}

	if (eCmd) {
		if ((eCmd.value.length) || (!eTime.disabled)) {
			elem.removeClass(eCmd, 'empty');
		}
		else {
			elem.addClass(eCmd, 'empty');
		}
	}

	return 1;
}

function verifyFields(focused, quiet) {
	for (var i = 0; i < scheds.length; ++i) {
		if (!verifySched(focused, quiet, scheds[i]))
			return 0;
	}

	return 1;
}

function saveSched(fom, key) {
	var s, i, n, k, en, e;

	k = '_' + key + '_';

	en = E(k + 'enabled').checked;
	s = en ? '1' : '0';
	s += ',';

	e = E(k + 'time').value;
	if (e == 'e')
		s += -(E(k + 'every').value * 1);
	else
		s += e;

	n = 0;
	for (i = 0; i < 7; ++i) {
		if (E(k + dowLow[i]).checked)
			n |= (1 << i);
	}
	if (n == 0) {
		n = 0x7F;
		e = E(k + 'everyday');
		e.checked = 1;
		verifySched(e, key);
	}

	e = fom['sch_' + key];
	e.value = s + ',' + n;
}

function save() {
	var fom, i

	if (!verifyFields(null, false)) return;

	fom = E('t_fom');
	for (i = 0; i < scheds.length; ++i) {
		saveSched(fom, scheds[i]);
	}

	form.submit(fom, 1);
}

function init() {
	verifyFields(null, 1);
}
</script>
</head>

<body onload="init()">
<form name="t_fom" id="t_fom" method="post" action="tomato.cgi">
<table id="container">
<tr><td colspan="2" id="header">
	<div class="title"><a href="/">Tomato64</a></div>
	<div class="version">Version <% version(); %> on <% nv("t_model_name"); %></div>
</td></tr>
<tr id="body"><td id="navi"><script>navi()</script></td>
<td id="content">
<div id="ident"><% ident(); %> | <script>wikiLink();</script></div>

<!-- / / / -->

<input type="hidden" name="_nextpage" value="admin-sched.asp">
<input type="hidden" name="_service" value="sched-restart">
<input type="hidden" name="sch_rboot" value="">
<input type="hidden" name="sch_rcon" value="">
<input type="hidden" name="sch_c1" value="">
<input type="hidden" name="sch_c2" value="">
<input type="hidden" name="sch_c3" value="">
<input type="hidden" name="sch_c4" value="">
<input type="hidden" name="sch_c5" value="">

<!-- / / / -->

<div class="section-title">Reboot</div>
<div class="section">
	<script>
		makeSched('rboot');
	</script>
</div>

<!-- / / / -->

<div class="section-title">Reconnect</div>
<div class="section">
	<script>
		makeSched('rcon');
	</script>
</div>

<!-- / / / -->

<div class="section-title">Custom 1</div>
<div class="section">
	<script>
		makeSched('c1', 1);
	</script>
</div>

<!-- / / / -->

<div class="section-title">Custom 2</div>
<div class="section">
	<script>
		makeSched('c2', 1);
	</script>
</div>

<!-- / / / -->

<div class="section-title">Custom 3</div>
<div class="section">
	<script>
		makeSched('c3', 1);
	</script>
</div>

<!-- / / / -->

<div class="section-title">Custom 4</div>
<div class="section">
	<script>
		makeSched('c4', 1);
	</script>
</div>

<!-- / / / -->

<div class="section-title">Custom 5</div>
<div class="section">
	<script>
		makeSched('c5', 1);
	</script>
</div>

<!-- / / / -->

<div id="footer">
	<span id="footer-msg"></span>
	<input type="button" value="Save" id="save-button" onclick="save()">
	<input type="button" value="Cancel" id="cancel-button" onclick="reloadPage();">
</div>

</td></tr>
</table>
</form>
</body>
</html>
