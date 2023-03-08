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
<title>[<% ident(); %>] Restarting...</title>
<link rel="stylesheet" type="text/css" href="/tomato.css">
<% css(); %>
<style>
div.tomato-grid.container-div {
	height: 90px;
}
#msg {
	border-bottom: 1px solid #aaa;
	margin-bottom: 10px;
	padding-bottom: 10px;
	font-weight: bold;
}
</style>
<script>
//	<% nvram("lan_ipaddr,dhcp_moveip"); %>

var n = 20;
function tick() {
	var txt = 'The router\'s new IP address is ';
	var e = document.getElementById("msg");

	if (nvram.dhcp_moveip == 0)
		txt += nvram.lan_ipaddr+'. You may need to release then renew your computer\'s DHCP lease before continuing.';
	else if (nvram.dhcp_moveip == 1)
		txt += '192.168.1.1 (nvram default).';
	else
		txt += '192.168.1.1 (default) while waiting for DHCP Server Infos. IP address will change to a.b.c.d (obtained IP via DHCP).'

	e.innerHTML = txt;

	e = document.getElementById("continue");
	e.value = n;
	if (n == 10) {
		e.style = "cursor:pointer";
		e.disabled = false;
	}
	if (n == 0)
		e.value = "Continue";
	else {
		--n;
		setTimeout(tick, 1000);
	}
}
function go() {
	var ip;
	if (nvram.dhcp_moveip == 0)
		ip = nvram.lan_ipaddr;
	else
		ip = '192.168.1.1';

	window.location = window.location.protocol+'//'+ip+'/';
}
</script>
</head>

<body onload="tick()">
<div class="tomato-grid container-div">
	<div class="wrapper1">
		<div class="wrapper2">
			<div class="info-centered">
				<form>
					<div id="msg"></div>
					<div id="but" style="display:inline-block">
						Please wait while the router restarts... &nbsp;
						<input type="button" value="" id="continue" onclick="go()" disabled="disabled">
					</div>
				</form>
			</div>
		</div>
	</div>
</div>
</body>
</html>
