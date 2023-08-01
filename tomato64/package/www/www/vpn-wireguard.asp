<!DOCTYPE html>
<!--
	FreshTomato GUI
	Copyright (C) 2023 pedro
	https://freshtomato.org/

	For use with FreshTomato Firmware only.
	No part of this file may be used without permission.
-->
<html lang="en-GB">
<head>
<meta http-equiv="content-type" content="text/html;charset=utf-8">
<meta name="robots" content="noindex,nofollow">
<title>[<% ident(); %>] Wireguard</title>
<link rel="stylesheet" type="text/css" href="tomato.css">
<% css(); %>
<script src="isup.jsz"></script>
<script src="isup.js"></script>
<script src="tomato.js"></script>

<script>

//	<% nvram("_http_id"); %>

var changed = 0;
var serviceType = 'wireguard';

function verifyFields(focused, quiet) {
	return ok;
}

function save_pre() {
	if (!verifyFields(null, 0))
		return 0;
	return 1;
}

function save(nomsg) {
	save_pre();
	if (!nomsg) show(); /* update '_service' field first */

	form.submit(fom, 1);

	changed = 0;
}

function earlyInit() {
	//show();
	//verifyFields(null, 1);
}

function init() {
	eventHandler();
	up.initPage(250, 5);
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

<div class="section-title">Wireguard</div>
<div class="section">
	Wireguard is included in this version/build of Freshtomato. A fully functional GUI is work in progress, in the meantime you can already set up your VPN tunnels via command line.
	Please consult the <a href="https://wiki.freshtomato.org/doku.php/wireguard_on_freshtomato" class="new_window">HOWTO on the Wiki</a> for additional information. 
</div>

<!-- / / / -->

<div id="footer">
<!--	<span id="footer-msg"></span>
	<input type="button" value="Save" id="save-button" onclick="save()">
	<input type="button" value="Cancel" id="cancel-button" onclick="reloadPage();"> -->
</div>

</td></tr>
</table>
</form>
<script>earlyInit();</script>
</body>
</html>
