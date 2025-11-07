<!DOCTYPE html>
<html lang="en-GB">
<head>
<meta http-equiv="content-type" content="text/html;charset=utf-8">
<meta name="robots" content="noindex,nofollow">
<title>[<% ident(); %>] Please Wait...</title>
<link rel="stylesheet" type="text/css" href="tomato.css?rel=<% version(); %>">
<% css(); %>
<script src="tomato.js?rel=<% version(); %>"></script>
<script>

//	<% nvram(''); %>

var wait = parseInt('0<% cgi_get('_nextwait'); %>', 10);
if (isNaN(wait)) wait = 5;
var clock, l2, spun = 1;

var ref = new TomatoRefresh('httpd.jsx', '', 1);

ref.refresh = function(text) {
	isup = {};
	try {
		eval(text);
	}
	catch (ex) {
		isup = {};
	}
	if (isup.httpd)
		wait = 0;
}

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

function go() {
	clock.style.display = 'none';
	window.location.replace('<% cgi_get('_nextpage'); %>');
}

function init() {
	clock = E('sptime');
	l2 = E('l2');

	if (wait > 0) {
		l2.style.display = 'inline-block';
		clock.style.display = 'inline';
		E('spin').style.display = 'inline-block';
		tick();
		if (!spun) setSpin(0);
	}
	else {
		l1 = E('l1');
		l1.style.display = 'block';
	}
	ref.initPage(1000, 1);
}
</script>
</head>

<body onload="init()"">
<div class="tomato-grid container-div">
	<div class="wrapper1">
		<div class="wrapper2">
			<div class="info-centered">
				<form>
					<div id="l1" style="display:none"><b>Changes Saved... </b> <input type="button" value="Continue" onclick="go()"></div>
					<div id="l2" style="display:none"><b>Please Wait... </b> &nbsp;<div id="sptime"></div> &nbsp;<img src="spin.svg" alt="" id="spin"></div>
				</form>
			</div>
		</div>
	</div>
</div>
</body>
</html>
