<!DOCTYPE html>
<html lang="en-GB">
<head>
<meta http-equiv="content-type" content="text/html;charset=utf-8">
<meta name="robots" content="noindex,nofollow">
<title>[<% ident(); %>] Rebooting...</title>
<link rel="stylesheet" type="text/css" href="tomato.css?rel=<% version(); %>">
<% css(); %>
<style>
div.tomato-grid.container-div {
	height: 90px;
}
#msg {
	display: none;
	border-bottom: 1px dashed #888;
	margin-bottom: 10px;
	padding-bottom: 10px;
	font-weight: bold;
}
</style>
<script>
/* TOMATO64-REMOVE-BEGIN */
var n = 90 + parseInt('0<% nv("wait_time"); %>');
/* TOMATO64-REMOVE-END */
/* TOMATO64-BEGIN */
var n = 45 + parseInt('0<% nv("wait_time"); %>');
/* TOMATO64-END */
function tick() {
	var e = document.getElementById('continue');
	e.value = n--;
	if (n < 0) {
		e.value = 'Continue';
		return;
	}
/* TOMATO64-REMOVE-BEGIN */
	if (n == 69) {
/* TOMATO64-REMOVE-END */
/* TOMATO64-BEGIN */
	if (n == 14) {
/* TOMATO64-END */
		e.style = 'cursor:pointer';
		e.disabled = false;
	}
	setTimeout(tick, 1000);
}

function go() {
	window.location.replace('/');
}

function init() {
	var resmsg = '';
//	<% resmsg(); %>
	if (resmsg.length) {
		e = document.getElementById('msg');
		e.innerHTML = resmsg;
		e.style.display = 'block';
	}
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
					<div id="msg"></div>
					<div>Please wait while the router reboots... &nbsp;<input type="button" value="" id="continue" onclick="go()" disabled="disabled"></div>
				</form>
			</div>
		</div>
	</div>
</div>
</body>
</html>
