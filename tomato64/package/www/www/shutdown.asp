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
<title>[<% ident(); %>] Shutting down...</title>
<link rel="stylesheet" type="text/css" href="tomato.css">
<% css(); %>
<script>
var n = 31;
function tick() {
	if (--n > 0) {
		document.getElementById("sptime").innerHTML = n;
		setTimeout(tick, 1000);
	}
	else {
		document.getElementById("msg").innerHTML = "You can now unplug the router.";
	}
}
</script>
</head>

<body onload="tick()">
<div class="tomato-grid container-div">
	<div class="wrapper1">
		<div class="wrapper2">
			<div class="info-centered">
				<div id="msg">Please wait while the router shuts down... &nbsp;<div id="sptime"></div></div>
			</div>
		</div>
	</div>
</div>
</body>
</html>
