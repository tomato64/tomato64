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
<title>[<% ident(); %>] Please Wait...</title>
<link rel="stylesheet" type="text/css" href="tomato.css?rel=<% version(); %>">
<% css(); %>
<script>
var spun = 1;
var wait = parseInt('<% cgi_get('_nextwait'); %>', 10);
if (isNaN(wait)) wait = 5;
function tick() {
	clock.innerHTML = wait;
	if (--wait >= 0)
		setTimeout(tick, 1000);
	else
		go();
}
function setSpin(x) {
	l2.style.display = (x ? 'inline-block' : 'none');
	spun = x;
}
function init() {
	if (wait > 0) {
		l2 = document.getElementById('l2');
		l2.style.display = 'inline-block';
		clock = document.getElementById('sptime');
		clock.style.display = "inline";
		spin = document.getElementById('spin');
		spin.style.display = 'inline-block';
		tick();
		if (!spun) setSpin(0);
	}
	else {
		l1 = document.getElementById('l1');
		l1.style.display = 'block';
	}
}
function go() {
	clock.style.display = 'none';
	window.location.replace('<% cgi_get('_nextpage'); %>');
}
</script>
</head>

<body onload="init()" onclick="go()">
<div class="tomato-grid container-div">
	<div class="wrapper1">
		<div class="wrapper2">
			<div class="info-centered">
				<form>
					<div id="l1" style="display:none"><b>Changes Saved... </b> <input type="button" value="Continue" onclick="go()"></div>
					<div id="l2" style="display:none"><b>Please Wait... </b> &nbsp;<div id="sptime"></div> &nbsp;<img src="spin.gif" alt="" id="spin"></div>
				</form>
			</div>
		</div>
	</div>
</div>
</body>
</html>
