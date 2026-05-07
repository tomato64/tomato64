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
<link rel="stylesheet" type="text/css" href="tomato.css?rel=<% version(); %>">
<% css(); %>
<script src="tomato.js?rel=<% version(); %>"></script>

<script>

//	<% nvram("usb_apcupsd"); %>

var ref = new TomatoRefresh('/ext/cgi-bin/tomatoups.cgi', '', 3, 'nas_ups');

ref.refresh = function(text) {
	E('ups-status').innerHTML = text;

	var http = new XmlHttp();
	http.onCompleted = function(t) { E('ups-data').innerHTML = t; };
	http.onError = function() { };
	http.get('/ext/cgi-bin/tomatodata.cgi', null);
}

function init() {
	if (nvram.usb_apcupsd != '1') {
		E('upsmonitor').style.display = 'none';
		E('upsstatus').style.display = 'block';
		E('note-disabled').style.display = 'block';
		E('footer').style.display = 'none';
		return;
	}
	ref.initPage(0, 3);
	if (!ref.running) ref.once = 1;
	ref.start();
}
</script>
</head>

<body onload="init()">
<form id="t_fom" method="post" action="tomato.cgi">
<table id="container">
<tr><td colspan="2" id="header">
	<div class="title"><a href="/">Tomato64</a></div>
	<div class="version">Version <% version(); %> on <% nv("t_model_name"); %><span class="blinking bl2"><script><% anonupdate(); %> anon_update()</script>&nbsp;</span></div>
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

<div class="note-disabled" id="note-disabled"><b>UPS Monitor disabled.</b><br><br><a href="nas-usb.asp">Enable &raquo;</a></div>

<!-- / / / -->

<div id="footer">
	<img src="spin.svg" alt="" id="refresh-spinner">
	<script>genStdTimeList('refresh-time', 'One off', 0);</script>
	<input type="button" value="Refresh" onclick="ref.toggle()" id="refresh-button">
</div>

</td></tr>
</table>
</form>
<script>insOvl()</script>
</body>
</html>
