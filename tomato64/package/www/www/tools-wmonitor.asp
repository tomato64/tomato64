<!DOCTYPE html>
<!--
	Tomato GUI
	Copyright (C) 2006-2010 Jonathan Zarate
	http://www.polarcloud.com/tomato/

	Wireless Monitor Web GUI (mt7996app dashboard)
	Copyright (C) 2026 Lance Fredrickson
	lancethepants@gmail.com

	For use with Tomato Firmware only.
	No part of this file may be used without permission.
-->
<html lang="en-GB">
<head>
<meta http-equiv="content-type" content="text/html;charset=utf-8">
<meta name="robots" content="noindex,nofollow">
<title>[<% ident(); %>] Tools: Wireless Monitor</title>
<link rel="stylesheet" type="text/css" href="tomato.css?rel=<% version(); %>">
<% css(); %>
<script src="tomato.js?rel=<% version(); %>"></script>

<script>

//	<% nvram("mt7996app_enable,mt7996app_history"); %>

var dashUrl = 'mt7996app/index.html';

function verifyFields(focused, quiet) {
	var e = E('_mt7996app_history');

	e.disabled = !E('_f_mt7996app_enable').checked;

	if (e.value != '' && e.value.charAt(0) != '/') {
		ferror.set(e, 'Must be an absolute path (starting with /), or empty', quiet);
		return 0;
	}
	ferror.clear(e);

	return v_length(e, quiet, 0, 128);
}

function save() {
	if (!verifyFields(null, false)) return;

	var fom = E('t_fom');

	fom.mt7996app_enable.value = E('_f_mt7996app_enable').checked ? 1 : 0;
	form.submit(fom, 1);
}

/* Size the frame to the dashboard's content, so the page has the only
 * vertical scroll bar. Same origin, so its document is readable; its body
 * height follows the content (no viewport units in its CSS), so this can
 * shrink as well as grow. Anything too wide for the content column scrolls
 * sideways inside the frame instead of being cut off; that scroll bar's
 * height is added so it does not cover the bottom of the dashboard.
 */
var dashObserver = null;

function fitDashboard() {
	var f = E('dashboard');
	var w, b, h;

	try {
		w = f.contentWindow;
		b = w.document.body;
	}
	catch (ex) {
		return;
	}
	if (!b) return;

	h = Math.ceil(b.getBoundingClientRect().height);
	if (h > 0)
		f.style.height = (h + Math.max(0, w.innerHeight - w.document.documentElement.clientHeight)) + 'px';
}

function dashboardLoaded() {
	var d, b;

	if (dashObserver) {
		dashObserver.disconnect();
		dashObserver = null;
	}

	try {
		d = E('dashboard').contentWindow.document;
		b = d.body;
	}
	catch (ex) {
		return;
	}
	if (!b) return;

	d.documentElement.style.overflowX = 'auto';
	d.documentElement.style.overflowY = 'hidden';
	fitDashboard();
	if (typeof ResizeObserver !== 'undefined') {
		dashObserver = new ResizeObserver(fitDashboard);
		dashObserver.observe(b);
	}
	else
		setInterval(fitDashboard, 1000);
}

/* (Re)load the dashboard, or blank it so a stopped daemon is not polled. */
function showDashboard(on) {
	E('dashboard').src = (on ? dashUrl : 'about:blank');
	elem.display('dashboard-section', on);
	elem.display('dashboard-off', !on);
}

function init() {
	showDashboard(nvram.mt7996app_enable == 1);
}

/* Called by form.submit() once the save (and the service restart) is done. */
function submit_complete() {
	showDashboard(E('t_fom').mt7996app_enable.value == 1);
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

<input type="hidden" name="_nextpage" value="tools-wmonitor.asp">
<input type="hidden" name="_service" value="mt7996app-restart">
<input type="hidden" name="mt7996app_enable">

<!-- / / / -->

<div class="section-title">Wireless Monitor</div>
<div class="section">
	<script>
		createFieldTable('', [
			{ title: 'Enable', name: 'f_mt7996app_enable', type: 'checkbox', value: (nvram.mt7996app_enable == 1), suffix: '&nbsp; <small>polls the radios every 2 seconds while enabled<\/small>' },
			{ title: 'History file', name: 'mt7996app_history', type: 'text', maxlen: 128, size: 50, value: nvram.mt7996app_history,
				suffix: '<br><small>keeps the traffic history (up to 1 year) across restarts; empty keeps it in RAM only. Saved every 15 minutes and on stop, about 1-2 MB: use USB or other storage that is mounted at boot, not internal flash<\/small>' }
		]);
	</script>
</div>

<!-- / / / -->

<div class="section-title">Dashboard</div>
<div class="section" id="dashboard-off">
	<div>Enable the monitor and save to see the dashboard.</div>
</div>
<div class="section" id="dashboard-section" style="display:none">
	<input type="button" value="Open in New Tab" class="new_window" onclick="window.open(dashUrl)">
	<iframe id="dashboard" title="Wireless Monitor" onload="dashboardLoaded()" style="width:100%;height:600px;border:0;margin-top:6px"></iframe>
</div>

<!-- / / / -->

<div class="section-title">Notes</div>
<div class="section">
	<ul>
		<li>Live view of the MediaTek (mt76) radios: client statistics (per link for Wi-Fi 7 MLO clients), airtime, traffic history, hardware counters, and channel advice (with puncturing on Wi-Fi 7). Advice is shown only; nothing is changed on the radios.</li>
		<li>The radios do not scan on their own, so channel advice only compares channels scanned in the last 15 minutes. Run a <a href="tools-survey.asp">Wireless Survey</a> first; the advice is then good for 15 minutes.</li>
		<li>Based on <a href="https://github.com/rmandrad/mt7996app" class="new_window">mt7996app</a>.</li>
	</ul>
</div>

<!-- / / / -->

<script>writeFooter();</script>

</td></tr>
</table>
</form>
<script>insOvl();verifyFields(null, true);</script>
</body>
</html>
