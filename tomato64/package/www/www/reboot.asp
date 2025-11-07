<!DOCTYPE html>
<html lang="en-GB">
<head>
<meta http-equiv="content-type" content="text/html;charset=utf-8">
<meta name="robots" content="noindex,nofollow">
<title>[<% ident(); %>] Rebooting...</title>
<link rel="stylesheet" type="text/css" href="tomato.css?rel=<% version(); %>">
<% css(); %>
<script>
/* TOMATO64-REMOVE-BEGIN */
var n = 90 + parseInt('0<% nv("wait_time"); %>');
/* TOMATO64-REMOVE-END */
/* TOMATO64-BEGIN */
var n = 45 + parseInt('0<% nv("wait_time"); %>');
/* TOMATO64-END */
var resreset = 0;
function tick() {
	var e = document.getElementById('continue');
	e.value = n--;
	if (n < 0) {
		e.value = 'Continue';
		return;
	}
/* TOMATO64-REMOVE-BEGIN */
	if (n == 79) {
/* TOMATO64-REMOVE-END */
/* TOMATO64-BEGIN */
        if (n == 14) {
/* TOMATO64-END */
		e.style = 'cursor:pointer';
		e.disabled = 0;
	}
	setTimeout(tick, 1000);
}

function go() {
	var uri = '';

	if (resreset)
		uri = 'http://192.168.1.1';

	window.location.replace(uri+'/about.asp');
}

function init() {
	var e, msg2 = '', resmsg = '';
//	<% resmsg(); %>
//	<% resreset(); %>
	if (resmsg.length) {
		e = document.getElementById('msg1');
		e.innerHTML = resmsg;
		e.style.display = 'block';
	}

	if (resreset) {
		e = document.getElementById('inf');
		e.style.display = 'block';
		msg2 = ' and defaults are restored';
		e = document.getElementById('rboot');
		e.style.height = '120px';
	}

	msg2 = 'Please wait while the router reboots'+msg2+'... &nbsp;';
	e = document.getElementById('msg2');
	e.innerHTML = msg2;

	tick();
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
					<div id="inf" style="display:none">The router will reset its address back to 192.168.1.1. You may need to renew your computer's DHCP or reboot your computer before continuing.</div>
				</form>
			</div>
		</div>
	</div>
</div>
</body>
</html>
