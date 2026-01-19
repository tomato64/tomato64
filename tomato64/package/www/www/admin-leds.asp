<!DOCTYPE html>
<!--

	LED Configuration Page
        Copyright (C) 2026 Lance Fredrickson
        lancethepants@gmail.com

-->
<html lang="en-GB">
<head>
<meta http-equiv="content-type" content="text/html;charset=utf-8">
<meta name="robots" content="noindex,nofollow">
<title>[<% ident(); %>] Admin: LEDs</title>
<link rel="stylesheet" type="text/css" href="tomato.css?rel=<% version(); %>">
<% css(); %>
<script src="tomato.js?rel=<% version(); %>"></script>

<script>

//	<% nvram("stealth_mode,t_model_name"); %>

/* LED list will be populated by enumeration */
var ledList = [];
var cprefix = 'admin_leds';

function refreshLedStatus() {
	/* Fetch current LED states via shell command (nojs=1 returns raw text) */
	var cmd = new XmlHttp();
	cmd.onCompleted = function(text, xml) {
		var lines = text.split('\n');
		ledList = [];
		for (var i = 0; i < lines.length; i++) {
			var line = lines[i].trim();
			/* Use ;; as delimiter since LED names can contain colons */
			if (line.length > 0) {
				var parts = line.split(';;');
				if (parts.length >= 3) {
					ledList.push({
						name: parts[0],
						trigger: parts[1],
						brightness: parts[2],
						device: parts[3] || ''
					});
				}
			}
		}
		displayLeds();
	};
	cmd.onError = function(x) {
		E('led-list').innerHTML = '<tr><td colspan="4">Error reading LED status</td></tr>';
	};
	cmd.post('shell.cgi', 'action=execute&nojs=1&command=' + escapeCGI(
		'ls /sys/class/leds/ | while read n; do ' +
		'led="/sys/class/leds/$n"; ' +
		't=$(cat $led/trigger 2>/dev/null); t=${t##*[}; t=${t%%]*}; ' +
		'b=$(cat $led/brightness 2>/dev/null); ' +
		'd=""; if [ "$t" = "netdev" ]; then d=$(cat $led/device_name 2>/dev/null); fi; ' +
		'echo "$n;;$t;;$b;;$d"; ' +
		'done'
	));
}

function displayLeds() {
	var html = '';
	if (ledList.length == 0) {
		html = '<tr><td colspan="4">No LEDs detected on this device</td></tr>';
	} else {
		for (var i = 0; i < ledList.length; i++) {
			var led = ledList[i];
			var stateClass = (parseInt(led.brightness) > 0) ? 'led-on' : 'led-off';
			var triggerDisplay = led.trigger;
			if (led.trigger == 'netdev' && led.device) {
				triggerDisplay = 'netdev (' + led.device + ')';
			}
			html += '<tr>';
			html += '<td>' + escapeHTML(led.name) + '</td>';
			html += '<td>' + escapeHTML(triggerDisplay) + '</td>';
			html += '<td class="' + stateClass + '">' + (parseInt(led.brightness) > 0 ? 'ON' : 'OFF') + '</td>';
			html += '<td>';
			html += '<input type="button" value="On" onclick="setLed(\'' + escapeHTML(led.name) + '\', 1)">&nbsp;';
			html += '<input type="button" value="Off" onclick="setLed(\'' + escapeHTML(led.name) + '\', 0)">&nbsp;';
			html += '<input type="button" value="Blink" onclick="setLedBlink(\'' + escapeHTML(led.name) + '\')">&nbsp;';
			html += '<input type="button" value="Heartbeat" onclick="setLedHeartbeat(\'' + escapeHTML(led.name) + '\')">';
			html += '</td>';
			html += '</tr>';
		}
	}
	E('led-list').innerHTML = html;
}

function setLed(name, on) {
	var cmd = new XmlHttp();
	cmd.onCompleted = function(text, xml) {
		refreshLedStatus();
	};
	cmd.post('shell.cgi', 'action=execute&nojs=1&command=' + escapeCGI(
		'echo none > /sys/class/leds/' + name + '/trigger 2>/dev/null; ' +
		'echo ' + (on ? '255' : '0') + ' > /sys/class/leds/' + name + '/brightness 2>/dev/null'
	));
}

function setLedBlink(name) {
	var cmd = new XmlHttp();
	cmd.onCompleted = function(text, xml) {
		refreshLedStatus();
	};
	cmd.post('shell.cgi', 'action=execute&nojs=1&command=' + escapeCGI(
		'echo timer > /sys/class/leds/' + name + '/trigger 2>/dev/null; ' +
		'echo 500 > /sys/class/leds/' + name + '/delay_on 2>/dev/null; ' +
		'echo 500 > /sys/class/leds/' + name + '/delay_off 2>/dev/null'
	));
}

function setLedHeartbeat(name) {
	var cmd = new XmlHttp();
	cmd.onCompleted = function(text, xml) {
		refreshLedStatus();
	};
	cmd.post('shell.cgi', 'action=execute&nojs=1&command=' + escapeCGI(
		'echo heartbeat > /sys/class/leds/' + name + '/trigger 2>/dev/null'
	));
}

function setAllLeds(on) {
	var cmd = new XmlHttp();
	cmd.onCompleted = function(text, xml) {
		refreshLedStatus();
	};
	var command = '';
	for (var i = 0; i < ledList.length; i++) {
		var name = ledList[i].name;
		command += 'echo none > /sys/class/leds/' + name + '/trigger 2>/dev/null; ';
		command += 'echo ' + (on ? '255' : '0') + ' > /sys/class/leds/' + name + '/brightness 2>/dev/null; ';
	}
	cmd.post('shell.cgi', 'action=execute&nojs=1&command=' + escapeCGI(command));
}

function verifyFields(focused, quiet) {
	return 1;
}

function save() {
	var fom = E('t_fom');
	fom.stealth_mode.value = E('_f_stealth_mode').checked ? 1 : 0;
	form.submit(fom, 1);
}

function earlyInit() {
	var c;
	if (((c = cookie.get(cprefix+'_notes_vis')) != null) && (c == '1'))
		toggleVisibility(cprefix, 'notes');

	refreshLedStatus();
}

</script>

<style>
.led-on {
	color: #00ff00;
	font-weight: bold;
}
.led-off {
	color: #888888;
}
#led-table {
	width: 100%;
}
#led-table th {
	text-align: left;
	padding: 5px;
	border-bottom: 1px solid #444;
}
#led-table td {
	padding: 5px;
	border-bottom: 1px solid #333;
}
#led-table input[type="button"] {
	padding: 2px 8px;
	margin: 1px;
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

<input type="hidden" name="_nextpage" value="admin-leds.asp">
<input type="hidden" name="stealth_mode">

<!-- / / / -->

<div class="section-title">LED Status</div>
<div class="section">
	<table id="led-table">
		<tr>
			<th>LED Name</th>
			<th>Trigger</th>
			<th>State</th>
			<th>Control</th>
		</tr>
		<tbody id="led-list">
			<tr><td colspan="4">Loading...</td></tr>
		</tbody>
	</table>
	<br>
	<input type="button" value="Refresh" onclick="refreshLedStatus()">
	<input type="button" value="All On" onclick="setAllLeds(1)">
	<input type="button" value="All Off" onclick="setAllLeds(0)">
</div>

<!-- / / / -->

<div class="section-title">Stealth Mode</div>
<div class="section">
	<script>
		createFieldTable('', [
			{ title: 'Enable Stealth Mode', name: 'f_stealth_mode', type: 'checkbox', value: (nvram.stealth_mode == '1'), suffix: '&nbsp;<small>(turns off all LEDs, equivalent to clicking "All Off")<\/small>' }
		]);
	</script>
</div>

<!-- / / / -->

<div class="section-title">Notes <small><i><a href="javascript:toggleVisibility(cprefix,'notes');" id="toggleLink-notes"><span id="sesdiv_notes_showhide">(Show)</span></a></i></small></div>
<div class="section" id="sesdiv_notes" style="display:none">
	<ul>
		<li><b>About</b> - This page is for informational and debugging uses. The LED Status table is a playground
			for experimenting with LEDs on your device. Changes made here are temporary and will be reset on reboot.
			The only persistent setting is Stealth Mode.</li>
		<li><b>LED Status</b> - Shows all LED entries registered with the Linux LED subsystem. Not all entries may
			have a physical LED attached, and some LEDs (like power indicators or Ethernet PHY LEDs) may be
			hardware-controlled and not respond to software changes.</li>
		<li><b>Trigger</b> - Current LED trigger mode:
			<ul>
				<li><i>none</i> - Manual control (on/off)</li>
				<li><i>timer</i> - Blinking at configurable rate</li>
				<li><i>heartbeat</i> - Blinks with system load</li>
				<li><i>netdev</i> - Network interface activity</li>
				<li><i>default-on</i> - Always on</li>
				<li><i>phy*tpt / phy*radio</i> - WiFi driver-controlled (activity/radio state)</li>
			</ul>
		</li>
		<li><b>Stealth Mode</b> - When enabled, all LEDs are turned off at boot (equivalent to clicking "All Off"
			above). This is the only persistent configuration option on this page. 
			Note that some LEDs like power indicators and hardware-controlled LEDs may not be addressable.</li>
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
