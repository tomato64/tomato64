<!DOCTYPE html>
<html lang="en-GB">
<head>
<meta http-equiv="content-type" content="text/html;charset=utf-8">
<meta name="robots" content="noindex,nofollow">
<title>[<% ident(); %>] Restarting...</title>
<link rel="stylesheet" type="text/css" href="tomato.css?rel=<% version(); %>">
<% css(); %>
<script src="tomato.js?rel=<% version(); %>"></script>
<script>

//	<% nvram("lan_ipaddr,dhcp_moveip"); %>

var wait = 30;

function tick() {
	var e = E('continue');

	if (e.value == 'Continue')
		return;

	e.value = wait--;
	if (wait < 0) {
		e.style = 'cursor:pointer';
		e.disabled = 0;
		e.value = 'Continue';
		return;
	}
	if (wait == 20) {
		e.style = 'cursor:pointer';
		e.disabled = 0;
	}
	setTimeout(tick, 1000);
}

function go() {
	var ip;

	if (nvram.dhcp_moveip == 0)
		ip = nvram.lan_ipaddr;
	else
		ip = '192.168.1.1';

	window.location = window.location.protocol+'//'+ip+'/';
}

function init() {
	var txt = 'The router\'s new IP address is ';
	var e = E('msg1');

	if (nvram.dhcp_moveip == 0)
		txt += nvram.lan_ipaddr+'. You may need to release then renew your computer\'s DHCP lease before continuing.';
	else if (nvram.dhcp_moveip == 1)
		txt += '192.168.1.1 (nvram default).';
	else
		txt += '192.168.1.1 (default) while waiting for DHCP Server Infos. IP address will change to a.b.c.d (obtained IP via DHCP).'

	e.innerHTML = txt;
	e.style = 'display:inline-block';

	tick();
}
</script>
</head>

<body onload="init()">
<div class="tomato-grid container-div rboot">
	<div class="wrapper1">
		<div class="wrapper2">
			<div class="info-centered">
				<form>
					<div id="msg1"></div>
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
