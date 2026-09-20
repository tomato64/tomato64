<!DOCTYPE html>
<!--

	Front Panel Screen Page
	Copyright (C) 2026 Lance Fredrickson
	lancethepants@gmail.com

-->
<html lang="en-GB">
<head>
<meta http-equiv="content-type" content="text/html;charset=utf-8">
<meta name="robots" content="noindex,nofollow">
<title>[<% ident(); %>] Admin: Screen</title>
<link rel="stylesheet" type="text/css" href="tomato.css?rel=<% version(); %>">
<% css(); %>
<script src="tomato.js?rel=<% version(); %>"></script>

<script>

//	<% nvram("stealth_mode,t_model_name,panel_pages,panel_auto_lock,panel_idle_mode,panel_brightness,panel_background,panel_list_opacity,panel_clock_24h,panel_temp_unit,panel_scroll,panel_test_pages,panel_pin,panel_latitude,panel_longitude"); %>

var cprefix = 'admin_screen';

/* The panel, in its own pixels. Everything sent to it is in this space, so
   the view can be scaled to any size without the device knowing. */
var PANEL_W = 320;
var PANEL_H = 240;

/* Captures are a re-render plus a PNG encode on the router, so the next one is
   only asked for once the last has arrived. This is the pause between them. */
var FRAME_GAP = 250;
var RETRY_GAP = 3000;

/* A drag would otherwise send a sample per mouse event. */
var MOVE_GAP = 40;

var live = 1;
var frameTimer = null;
var pending = 0;

var down = 0;
var lastMove = 0;
var lastPoint = null;

function panelImg() {
	return E('_screen_img');
}

function setStatus(msg, bad) {
	var e = E('_screen_status');
	e.innerHTML = msg;
	e.className = bad ? 'screen-bad' : 'screen-ok';
}

/* One in flight at a time: an img whose src is replaced mid-load cancels the
   old request, so a slow capture would otherwise pile requests on the router. */
function frameNext(gap) {
	if (frameTimer) {
		clearTimeout(frameTimer);
		frameTimer = null;
	}

	if (!live)
		return;

	frameTimer = setTimeout(frameGet, gap);
}

function frameGet() {
	frameTimer = null;

	if (pending)
		return;

	pending = 1;
	panelImg().src = 'panel/screen.png?_=' + new Date().getTime();
}

/* Hidden rather than removed: the img keeps its size, so the frame does not
   collapse while there is nothing to show in it. */
function showPanel(on) {
	panelImg().style.visibility = on ? 'visible' : 'hidden';
	E('_screen_off').style.display = on ? 'none' : '';
}

function frameLoaded() {
	pending = 0;
	showPanel(1);
	setStatus('Live', 0);
	frameNext(FRAME_GAP);
}

function frameFailed() {
	pending = 0;
	showPanel(0);
	setStatus('Not running', 1);
	frameNext(RETRY_GAP);
}

function toggleLive() {
	live = E('_screen_live').checked ? 1 : 0;

	if (live)
		frameGet();
	else {
		setStatus('Paused', 0);
		frameNext(0);
	}
}

/* Where the click landed on the panel itself, whatever size it is shown at. */
function pointOf(ev) {
	var img = panelImg();
	var r = img.getBoundingClientRect();
	var x = Math.round((ev.clientX - r.left) * PANEL_W / r.width);
	var y = Math.round((ev.clientY - r.top) * PANEL_H / r.height);

	if (x < 0) x = 0;
	if (y < 0) y = 0;
	if (x > PANEL_W - 1) x = PANEL_W - 1;
	if (y > PANEL_H - 1) y = PANEL_H - 1;

	return { x: x, y: y };
}

function sendPoint(p, isDown) {
	var cmd = new XmlHttp();

	cmd.onCompleted = function(text, xml) {
		/* Show the result of the press without waiting for the next
		   scheduled capture. frameGet() ignores this while one is
		   already on its way. */
		if (live)
			frameGet();
	};
	cmd.onError = function(ex) { };

	cmd.post('panel/input.cgi', 'x=' + p.x + '&y=' + p.y + '&down=' + (isDown ? 1 : 0));
}

function pressStart(ev) {
	ev.preventDefault();

	down = 1;
	lastPoint = pointOf(ev);
	lastMove = new Date().getTime();

	sendPoint(lastPoint, 1);
}

function pressMove(ev) {
	if (!down)
		return;

	ev.preventDefault();

	var now = new Date().getTime();

	lastPoint = pointOf(ev);

	/* LVGL reads the pointer as a stream, so a swipe needs samples along
	   the way - but not one per mouse event. */
	if (now - lastMove < MOVE_GAP)
		return;

	lastMove = now;
	sendPoint(lastPoint, 1);
}

function pressEnd(ev) {
	if (!down)
		return;

	down = 0;

	/* Released where it last was: a release reported somewhere else turns a
	   tap into a drag. */
	if (lastPoint)
		sendPoint(lastPoint, 0);
}

/* A finger on a phone works the same way, one touch at a time. */
function touchAt(ev) {
	var t = ev.touches[0] || ev.changedTouches[0];

	return { clientX: t.clientX, clientY: t.clientY, preventDefault: function() { ev.preventDefault(); } };
}

function touchStart(ev) { pressStart(touchAt(ev)); }
function touchMove(ev) { pressMove(touchAt(ev)); }
function touchEnd(ev) { pressEnd(ev); }

function wake() {
	var cmd = new XmlHttp();

	cmd.onCompleted = function(text, xml) { frameGet(); };
	cmd.onError = function(ex) { };

	cmd.post('panel/input.cgi', 'wake=1');
}

/* The panel lists its own pages, so this follows whatever is configured. */
function loadPages() {
	var cmd = new XmlHttp();

	cmd.onCompleted = function(text, xml) {
		var st, sel = E('_screen_page'), i;

		try { st = JSON.parse(text); }
		catch (ex) { return; }

		if (!st || !st.pages)
			return;

		sel.options.length = 0;
		sel.options[0] = new Option('Jump to page...', '');

		for (i = 0; i < st.pages.length; ++i)
			sel.options[sel.options.length] = new Option(st.pages[i], st.pages[i]);

		sel.style.display = '';
	};

	cmd.post('shell.cgi', 'action=execute&nojs=1&command=' + escapeCGI('ubus call lvgl status'));
}

function gotoPage(name) {
	/* The names come from the panel, and go back to it inside a shell
	   command, so anything unexpected is dropped rather than quoted. */
	if (!name.match(/^[a-z0-9_-]+$/))
		return;

	var cmd = new XmlHttp();

	cmd.onCompleted = function(text, xml) { frameGet(); };

	cmd.post('shell.cgi', 'action=execute&nojs=1&command=' +
		 escapeCGI('ubus call lvgl page \'{"name":"' + name + '"}\''));

	E('_screen_page').selectedIndex = 0;
}

function setZoom(z) {
	var img = panelImg();
	var frame = E('_screen_frame');

	img.style.width = frame.style.width = (PANEL_W * z) + 'px';
	img.style.height = frame.style.height = (PANEL_H * z) + 'px';

	cookie.set(cprefix + '_zoom', z);
}

/* The capture is the panel's own 320x240, so anything above 1x is the browser
   enlarging it: soft when the pixels are blended, blocky when each is kept
   square. Blended by default - it reads far better - and neither shows any
   more than the panel has. */
function setSmooth(on) {
	panelImg().style.imageRendering = on ? 'auto' : 'pixelated';

	cookie.set(cprefix + '_smooth', on ? 1 : 0);
}

function verifyFields(focused, quiet) {
	var ok = 1;

	if (!v_range('_panel_brightness', quiet, 10, 100)) ok = 0;
	if (!v_range('_panel_list_opacity', quiet, 0, 100)) ok = 0;
	if (!v_length('_panel_pages', quiet, 1, 256)) ok = 0;

	/* Six digits or nothing: anything else is refused by the panel at
	   startup, and it would be refused silently. */
	var pin = E('_panel_pin');
	if (pin.value != '' && !pin.value.match(/^[0-9]{6}$/)) {
		ferror.set(pin, 'The PIN must be six digits, or empty for none.', quiet);
		ok = 0;
	}
	else ferror.clear(pin);

	/* Both or neither: one alone leaves the weather page without a place. */
	var lat = E('_panel_latitude'), lon = E('_panel_longitude');
	var located = (lat.value != '') + (lon.value != '');

	ferror.clear(lat);
	ferror.clear(lon);

	if (located == 1) {
		ferror.set(lat.value == '' ? lat : lon, 'The weather page needs both a latitude and a longitude.', quiet);
		ok = 0;
	}

	return ok;
}

/* What to do to the panel once the settings have actually been written. */
var panelAction = null;

function save() {
	if (!verifyFields(null, 0))
		return;

	var fom = E('t_fom');
	var on = E('_f_screen_enable').checked;

	fom.stealth_mode.value = on ? 0 : 1;
	fom.panel_clock_24h.value = E('_f_panel_clock_24h').checked ? 1 : 0;
	fom.panel_scroll.value = E('_f_panel_scroll').checked ? 1 : 0;
	fom.panel_test_pages.value = E('_f_panel_test_pages').checked ? 1 : 0;

	/* The panel reads its settings once, at startup, so restarting it is
	   what makes a change take effect - and stopping it is what the Enable
	   Screen box does. Both have to happen after the save, not before, or
	   the panel comes back reading the old values: form.submit() calls
	   submit_complete() when the write has gone through. */
	panelAction = on ? 'restart' : 'stop';

	form.submit(fom, 1);
}

function submit_complete() {
	if (!panelAction)
		return;

	var cmd = new XmlHttp();

	cmd.onCompleted = function(text, xml) { frameGet(); };
	cmd.onError = function(ex) { };

	cmd.post('shell.cgi', 'action=execute&nojs=1&command=' + escapeCGI('panel_ctl ' + panelAction));

	panelAction = null;
}

function earlyInit() {
	var c;
	if (((c = cookie.get(cprefix + '_notes_vis')) != null) && (c == '1'))
		toggleVisibility(cprefix, 'notes');

	var img = panelImg();

	img.onload = frameLoaded;
	img.onerror = frameFailed;
	img.onmousedown = pressStart;
	img.ondragstart = function() { return false; };

	img.addEventListener('touchstart', touchStart, false);
	img.addEventListener('touchmove', touchMove, false);
	img.addEventListener('touchend', touchEnd, false);

	/* On the document, so a drag that leaves the image still finishes. */
	document.addEventListener('mousemove', pressMove, false);
	document.addEventListener('mouseup', pressEnd, false);

	if (((c = cookie.get(cprefix + '_zoom')) == null) || (c < 1) || (c > 3))
		c = 2;

	E('_screen_zoom').value = c;
	setZoom(c);

	c = cookie.get(cprefix + '_smooth');
	E('_screen_smooth').checked = (c == null) || (c == '1');
	setSmooth(E('_screen_smooth').checked);

	showPanel(0);
	setStatus('', 0);

	loadPages();
	frameGet();
}

</script>

<style>
#screen-view {
	padding: 10px 0;
	text-align: center;
}
/* The frame stays the size of the panel whether or not there is a capture to
   put in it, so the page does not jump about as the screen comes and goes -
   and a failed capture shows the message below rather than the browser's
   broken image mark. */
#_screen_frame {
	position: relative;
	display: inline-block;
	background: #000;
	border: 1px solid #444;
	vertical-align: top;
}
/* No image-rendering here: the Smooth control sets it, defaulting to the
   browser's own blending, which reads far better than square pixels once the
   320x240 capture is enlarged. */
#_screen_img {
	display: block;
	cursor: crosshair;
	touch-action: none;
	user-select: none;
	-webkit-user-select: none;
}
#_screen_off {
	position: absolute;
	left: 0;
	top: 50%;
	width: 100%;
	transform: translateY(-50%);
	text-align: center;
	color: #c8c8c8;
	line-height: 1.6em;
}
.screen-ok {
	color: #00b000;
}
.screen-bad {
	color: #c00000;
	font-weight: bold;
}
#screen-controls {
	text-align: center;
}
#screen-controls > * {
	margin: 0 4px;
}
#sesdiv_notes ul > li {
	margin-bottom: 8px;
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

<input type="hidden" name="_nextpage" value="admin-screen.asp">
<!-- The panel is restarted from submit_complete(), which runs this long after
     the save; the default five seconds is a long time to watch a dead screen. -->
<input type="hidden" name="_nextwait" value="1">
<input type="hidden" name="stealth_mode">
<input type="hidden" name="panel_clock_24h">
<input type="hidden" name="panel_scroll">
<input type="hidden" name="panel_test_pages">

<!-- / / / -->

<div class="section-title">Front Panel</div>
<div class="section">
	<div id="screen-view">
		<div id="_screen_frame">
			<img id="_screen_img" width="320" height="240" alt="">
			<div id="_screen_off">Screen not running<br><small>Enable it below, or check the panel.</small></div>
		</div>
	</div>
	<div id="screen-controls">
		<label><input type="checkbox" id="_screen_live" checked onclick="toggleLive()"> Live</label>
		<label>Size
			<select id="_screen_zoom" onchange="setZoom(this.value)">
				<option value="1">1x</option>
				<option value="2">2x</option>
				<option value="3">3x</option>
			</select>
		</label>
		<select id="_screen_page" style="display:none" onchange="gotoPage(this.value)">
			<option value="">Jump to page...</option>
		</select>
		<label><input type="checkbox" id="_screen_smooth" checked onclick="setSmooth(this.checked)"> Smooth</label>
		<input type="button" value="Refresh" onclick="frameGet()">
		<input type="button" value="Wake" onclick="wake()">
		<span id="_screen_status"></span>
	</div>
</div>

<!-- / / / -->

<div class="section-title">Screen</div>
<div class="section">
	<script>
		createFieldTable('', [
			{ title: 'Enable Screen', name: 'f_screen_enable', type: 'checkbox', value: (nvram.stealth_mode != '1'), suffix: '&nbsp;<small>(this device has no LEDs, so this is what stealth mode switches)<\/small>' },
			{ title: 'Brightness', name: 'panel_brightness', type: 'text', maxlen: 3, size: 5, value: nvram.panel_brightness, suffix: '&nbsp;<small>10 - 100 %. The panel writes this back when you change it there.<\/small>' },
			{ title: 'Go idle after', name: 'panel_auto_lock', type: 'select', options: [
				['0','Never'],
				['1','1 minute'],['2','2 minutes'],['3','3 minutes'],['5','5 minutes'],
				['10','10 minutes'],['15','15 minutes'],['30','30 minutes']],
				value: nvram.panel_auto_lock, suffix: '&nbsp;<small>without being touched. Never leaves the panel on the page it is showing.<\/small>' },
			{ title: 'When idle', name: 'panel_idle_mode', type: 'select', options: [
				['on','Show the clock'],['blank','Turn the screen off']],
				value: nvram.panel_idle_mode },
			{ title: '24 hour clock', name: 'f_panel_clock_24h', type: 'checkbox', value: (nvram.panel_clock_24h == '1') },
			{ title: 'Temperature', name: 'panel_temp_unit', type: 'select', options: [
				['c','Celsius'],['f','Fahrenheit']],
				value: nvram.panel_temp_unit, suffix: '&nbsp;<small>the system gauge and the weather<\/small>' },
			{ title: 'Screen PIN', name: 'panel_pin', type: 'text', maxlen: 6, size: 8, value: nvram.panel_pin, suffix: '&nbsp;<small>six digits, asked for on waking. Empty for none.<\/small>' }
		]);
	</script>
</div>

<!-- / / / -->

<div class="section-title">Appearance</div>
<div class="section">
	<script>
		createFieldTable('', [
			{ title: 'Wallpaper', name: 'panel_background', type: 'select', options: [
				['0','None (black)'],['1','Vertical lines'],['2','Horizontal lines'],
				['3','Vertical waves'],['4','Horizontal waves'],['5','Diagonal, steep'],
				['6','Diagonal, shallow'],['7','Diagonal waves, steep'],['8','Diagonal waves, shallow']],
				value: nvram.panel_background },
			{ title: 'List opacity', name: 'panel_list_opacity', type: 'text', maxlen: 3, size: 5, value: nvram.panel_list_opacity, suffix: '&nbsp;<small>0 - 100. 100 is half solid, so the wallpaper always shows through a little.<\/small>' },
			{ title: 'Follow the finger', name: 'f_panel_scroll', type: 'checkbox', value: (nvram.panel_scroll == '1'), suffix: '&nbsp;<small>pages track a swipe; unticked they change in one step<\/small>' }
		]);
	</script>
</div>

<!-- / / / -->

<div class="section-title">Pages</div>
<div class="section">
	<script>
		createFieldTable('', [
			{ title: 'Pages', name: 'panel_pages', type: 'text', maxlen: 256, size: 64, value: nvram.panel_pages, suffix: '&nbsp;<small>names separated by spaces, in swipe order. menu goes first: it is where the clock screen lands.<\/small>' },
			{ title: 'Weather latitude', name: 'panel_latitude', type: 'text', maxlen: 12, size: 12, value: nvram.panel_latitude, suffix: '&nbsp;<small>decimal degrees, e.g. 40.7128<\/small>' },
			{ title: 'Weather longitude', name: 'panel_longitude', type: 'text', maxlen: 12, size: 12, value: nvram.panel_longitude, suffix: '&nbsp;<small>both are needed, or the weather page is left out<\/small>' },
			{ title: 'Test pages', name: 'f_panel_test_pages', type: 'checkbox', value: (nvram.panel_test_pages == '1'), suffix: '&nbsp;<small>appends the touch, fonts and colours pages<\/small>' }
		]);
	</script>
</div>

<!-- / / / -->

<div class="section-title">Notes <small><i><a href="javascript:toggleVisibility(cprefix,'notes');" id="toggleLink-notes"><span id="sesdiv_notes_showhide">(Show)</span></a></i></small></div>
<div class="section" id="sesdiv_notes" style="display:none">
	<ul>
		<li><b>Front Panel</b> - A capture of what the screen is showing right now, taken a few times a
			second while Live is ticked. Each one is drawn fresh by the panel, so it always matches what
			is on the glass.</li>
		<li><b>Size and Smooth</b> - The capture is the panel's own 320x240, so any size above 1x is the
			browser enlarging it. Smooth, on by default, blends the enlarged pixels; unticking it keeps
			each one a hard square, which is truer to where things really are. Neither shows any more
			detail than the panel has.</li>
		<li><b>Using it</b> - Click the picture to tap the screen, and click and drag to swipe it, the same
			as touching the device. Wake turns the screen back on after it has dimmed, and the page list
			jumps straight to a page without swiping there.</li>
		<li><b>Enable Screen</b> - Unticking this is stealth mode: the screen and its backlight go off, and
			stay off at the next boot. Saving applies it straight away.</li>
	</ul>
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
<script>earlyInit()</script>
</body>
</html>
