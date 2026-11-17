<!DOCTYPE html>
<html lang="en-GB">
<head>
<meta http-equiv="content-type" content="text/html;charset=utf-8">
<meta name="robots" content="noindex,nofollow">
<title>[<% ident(); %>] Rebooting...</title>
/* TOMATO64-REMOVE-BEGIN */
<link rel="stylesheet" type="text/css" href="tomato.css">
<% css(); %>
/* TOMATO64-REMOVE-END */
/* TOMATO64-BEGIN */
<% inline_favicon(); %>
<% inline_css(); %>
/* TOMATO64-END */
<script>
/* TOMATO64-REMOVE-BEGIN */
var n = 90 + parseInt('0<% nv("wait_time"); %>', 10);
/* TOMATO64-REMOVE-END */
/* TOMATO64-BEGIN */
var n = 45 + parseInt('0<% nv("wait_time"); %>', 10);
/* TOMATO64-END */
var resreset = 0;
var isReset = false;
var differentOrigin = false;
var ready = false;
var offlineReplies = 0;
var onlineReplies = 0;
var wentOffline = false;
var manualShown = false;
var startedAt = 0;

function showManualHelp() {
	if (manualShown)
		return;
	manualShown = true;

	var link = document.getElementById('manualLink');
	if (isReset) {
		link.href = 'http://192.168.1.1/about.asp';
		link.textContent = 'Open router at 192.168.1.1';
		document.getElementById('msg2').textContent = 'Automatic detection is not available or the router has not responded yet.';
	}
	else {
		link.href = window.location.protocol+'//'+window.location.host+'/about.asp';
		link.textContent = 'Try opening the router web interface';
		document.getElementById('msg2').textContent = 'The router web server has not been confirmed online.';
	}
	document.getElementById('manual').style.display = 'block';
	document.getElementById('rboot').style.height = '160px';
}

function tick() {
	if (ready)
		return;

	var elapsed = Math.floor((new Date().getTime() - startedAt) / 1000);
	var remaining = Math.max(0, n - elapsed);
	document.getElementById('continue').value = remaining ? remaining : 'Waiting...';

	if (!remaining)
		showManualHelp();

	setTimeout(tick, 1000);
}

function pollRouter() {
	if (ready)
		return;

	var xhr = new XMLHttpRequest();
	var finished = false;
	var timeout;

	function complete(online) {
		if (finished)
			return;

		finished = true;
		clearTimeout(timeout);

		if (!online) {
			if (++offlineReplies >= 2)
				wentOffline = true;

			onlineReplies = 0;
		}
		else {
			offlineReplies = 0;
			if (wentOffline && ++onlineReplies >= 2) {
				ready = true;
				var e = document.getElementById('continue');
				e.value = 'Continue';
				e.disabled = false;
				e.style.cursor = 'pointer';
				document.getElementById('msg2').textContent = 'The router web server is responding again.';
				document.getElementById('manual').style.display = 'none';
				return;
			}
		}
		setTimeout(pollRouter, 2000);
	}

	xhr.onreadystatechange = function() {
		if (xhr.readyState != 4)
			return;

		var server = '', type = '';
		try {
			server = xhr.getResponseHeader('Server') || '';
			type = xhr.getResponseHeader('Content-Type') || '';
		}
		catch (ex) { }

		complete(server.toLowerCase() == 'httpd' && ((xhr.status == 200 && type.indexOf('text/css') != -1) || xhr.status == 401));
	};

	try {
		xhr.open('GET', 'tomato.css?reboot_check=' + new Date().getTime(), true);
		xhr.setRequestHeader('Cache-Control', 'no-cache');
		timeout = setTimeout(function() {
			complete(false);
			xhr.abort();
		}, 4000);
		xhr.send(null);
	}
	catch (ex) {
		complete(false);
	}
}

function go() {
	if (!ready)
		return;

	window.location.replace((isReset ? 'http://192.168.1.1' : '')+'/about.asp');
}

function init() {
	var e, msg2 = '', resmsg = '';
//	<% resmsg(); %>
//	<% resreset(); %>
	isReset = String(resreset) == '1';
	differentOrigin = isReset && (window.location.protocol != 'http:' || window.location.hostname != '192.168.1.1' || (window.location.port != '' && window.location.port != '80'));

	if (resmsg.length) {
		e = document.getElementById('msg1');
		e.innerHTML = resmsg;
		e.style.display = 'block';
	}

	if (isReset) {
		e = document.getElementById('inf');
		e.style.display = 'block';
		msg2 = ' and defaults are restored';
		document.getElementById('rboot').style.height = '140px';
	}

	document.getElementById('msg2').textContent = 'Please wait while the router reboots'+msg2+'...';

	startedAt = new Date().getTime();
	tick();

	/* After an NVRAM reset, the old origin may no longer exist. */
	if (!differentOrigin)
		setTimeout(pollRouter, 1500);
}
</script>
</head>

<body onload="init()">
<div class="tomato-grid container-div rboot" id="rboot">
	<div class="wrapper1">
		<div class="wrapper2">
			<div class="info-centered">
				<form>
					<div id="msg1"></div>
					<div id="msg2" style="display:inline-block"></div><div style="display:inline-block"><input type="button" value="" id="continue" onclick="go()" disabled="disabled"></div>
					<div id="inf" style="display:none">The router will reset its address back to 192.168.1.1. You may need to renew your computer's DHCP lease or temporarily use a static address such as 192.168.1.2/24 before reconnecting.</div>
					<div id="manual" style="display:none; margin-top:10px"><a id="manualLink" href=""></a><div>If the router's address changed after restoring settings, use its new address instead. The link does not confirm the upgrade succeeded.</div></div>
				</form>
			</div>
		</div>
	</div>
</div>
</body>
</html>
