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
<title>[<% ident(); %>] Restoring Defaults...</title>
<link rel="stylesheet" type="text/css" href="/tomato.css">
<% css(); %>
<style>
div.tomato-grid.container-div {
	height: 90px;
}
#msg {
	border-top: 1px dashed #888;
	margin-top: 10px;
	padding-top: 10px;
	font-weight: bold;
}
</style>
<script>
var n = 90;
function tick() {
	var e = document.getElementById('continue');
	e.value = n--;
	if (n < 0) {
		e.value = "Continue";
		return;
	}
	if (n == 59) {
		e.style = "cursor:pointer";
		e.disabled = false;
	}
	setTimeout(tick, 1000);
}
function go() {
	window.location = "http://192.168.1.1/";
}
function init() {
	tick();
}
</script>
</head>

<body onload="init()">
<div class="tomato-grid container-div">
	<div class="wrapper1">
		<div class="wrapper2">
			<div class="info-centered">
				<form>
					<div>Please wait while the defaults are restored... &nbsp;<input type="button" value="" id="continue" onclick="go()" disabled="disabled"></div>
					<div id="msg">The router will reset its address back to 192.168.1.1. You may need to renew your computer's DHCP or reboot your computer before continuing.</div>
				</form>
			</div>
		</div>
	</div>
</div>
</body>
</html>
