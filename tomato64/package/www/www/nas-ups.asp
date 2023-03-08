<!DOCTYPE html>
<!--
	Tomato GUI
	For use with Tomato Firmware only.
	No part of this file may be used without permission.
-->
<html lang="en-GB">
<head>
<meta http-equiv="content-type" content="text/html;charset=utf-8">
<meta name="robots" content="noindex,nofollow">
<title>[<% ident(); %>] NAS: UPS Monitor</title>
<link rel="stylesheet" type="text/css" href="tomato.css">
<% css(); %>
<script src="tomato.js"></script>

<script>

//	<% nvram("usb_apcupsd"); %>

function clientSideInclude(id, url) {
	var req = false;
	/* For Safari, Firefox, and other non-MS browsers */
	if (window.XMLHttpRequest) {
		try {
			req = new XMLHttpRequest();
		} catch (e) {
			req = false;
		}
	}
	else if (window.ActiveXObject) {
		/* For Internet Explorer on Windows */
		try {
			req = new ActiveXObject("Msxml2.XMLHTTP");
		} catch (e) {
			try {
				req = new ActiveXObject("Microsoft.XMLHTTP");
			} catch (e) {
				req = false;
			}
		}
	}
	var element = document.getElementById(id);

	if (!element) {
		alert("Bad id " + id + "passed to clientSideInclude. You need a div or span element with this id in your page.");
		return;
	}

	if (req) {
		/* Synchronous request, wait till we have it all */
		req.open('GET', url, false);
		req.send(null);
		element.innerHTML = req.responseText;
	}
	else {
		element.innerHTML = "Sorry, your browser does not support XMLHTTPRequest objects.";
	}
}

function init() {
	if (nvram.usb_apcupsd != '1') {
		E('upsmonitor').style.display = 'none';
		E('upsstatus').style.display = 'block';
		E('note-disabled').style.display = 'block';
		return;
	}
	clientSideInclude('ups-status', '/ext/cgi-bin/tomatoups.cgi');
	clientSideInclude('ups-data', '/ext/cgi-bin/tomatodata.cgi');
}
</script>
</head>

<body onload="init()">
<form id="t_fom" method="post" action="tomato.cgi">
<table id="container">
<tr><td colspan="2" id="header">
	<div class="title">FreshTomato</div>
	<div class="version">Version <% version(); %> on <% nv("t_model_name"); %></div>
</td></tr>
<tr id="body"><td id="navi"><script>navi()</script></td>
<td id="content">
<div id="ident"><% ident(); %> | <script>wikiLink();</script></div>

<!-- / / / -->

<div class="section-title" id="upsstatus" style="display:none">UPS Monitor</div>
<div id="upsmonitor">
	<input type="hidden" name="_nextpage" value="nas-ups.asp">
	<div class="section-title" id="ups-status-section">APC UPS Status</div>
	<div class="section">
		<span id="ups-status"></span>
	</div>
	<div class="section-title" id="ups-data-section">APC UPS Response</div>
	<div class="section">
		<span id="ups-data"></span>
	</div>
</div>

<!-- / / / -->

<div class="note-disabled" id="note-disabled" style="display:none"><b>UPS Monitor disabled.</b><br><br><a href="nas-usb.asp">Enable &raquo;</a></div>

<!-- / / / -->

<div id="footer">
	&nbsp;
</div>

</td></tr>
</table>
</form>
</body>
</html>
