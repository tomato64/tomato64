<!DOCTYPE html>
<!--
	Tomato GUI
	Copyright (C) 2012 Shibby
	http://openlinksys.info
	For use with Tomato Firmware only.
	No part of this file may be used without permission.
-->
<html lang="en-GB">
<head>
<meta http-equiv="content-type" content="text/html;charset=utf-8">
<meta name="robots" content="noindex,nofollow">
<title>[<% ident(); %>] Admin: TomatoAnon</title>
<link rel="stylesheet" type="text/css" href="tomato.css">
<% css(); %>
<script src="tomato.js"></script>

<script>

//	<% nvram("tomatoanon_enable,tomatoanon_answer,tomatoanon_id,tomatoanon_notify"); %>

var anon_link = '&nbsp;&nbsp;<a href="http://anon.groov.pl/index.php?search=9&routerid=<% nv('tomatoanon_id'); %>" class="new_window"><i>[Checkout my router]<\/i><\/a>';

function verifyFields(focused, quiet) {
	var o = (E('_tomatoanon_answer').value == '1');
	var s = (E('_tomatoanon_enable').value == '1');

	E('_tomatoanon_enable').disabled = !o;
	E('_f_tomatoanon_notify').disabled = !o || !s;

	return 1;
}

function save() {
	if (verifyFields(null, 0) == 0) return;
	var fom = E('t_fom');

	fom.tomatoanon_notify.value = E('_f_tomatoanon_notify').checked ? 1 : 0;

	fom._service.value = 'tomatoanon-restart';
	form.submit('t_fom', 1);
}

function init() {
	eventHandler();
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

<input type="hidden" name="_nextpage" value="admin-tomatoanon.asp">
<input type="hidden" name="_service" value="tomatoanon-restart">
<input type="hidden" name="tomatoanon_notify">

<!-- / / / -->

<div class="section-title">TomatoAnon</div>
<div class="section">
	<div class="about">
		The script sends (to the database) information about the router model and the installed version of Tomato.<br>
		The information submitted is 100% anonymous and will be used ONLY for statistical purposes.<br>
		<b>This script does NOT send any private or personal information (such as MAC, IP addresses, etc.)</b><br>
		The script is fully open and written in bash. Everyone can freely view the content sent to the database.<br>
		<br>
		The submitted results can be viewed on the <a href="http://anon.groov.pl" class="new_window"><b>http://anon.groov.pl</b></a> page.<br>
		<br>
		<br>
		The following data is sent by TomatoAnon:<br>
		- MD5SUM of WAN+LAN MAC addresses - this identifies the router, ex: 1c1dbd4202d794251ec1acf1211bb2c8<br>
		- Router model, ex: Asus RT-AC3200<br>
		- Tomato version installed, ex: 2019.3 K26ARM USB<br>
		- Builtype, ex: AIO-64K<br>
		- Tomato MOD, ex: FreshTomato<br>
		- Uptime of your router, ex: 7 days
	</div>
</div>

<!-- / / / -->

<div class="section-title">TomatoAnon Settings <script>W(anon_link);</script></div>
<div class="section">
	<script>
		createFieldTable('', [
			{ title: 'Do you know what TomatoAnon does?', name: 'tomatoanon_answer', type: 'select', options: [ ['0','No, I don\'t. Have to read all information, before I will make a choice'], ['1','Yes, I do and want to make a choice'] ], value: nvram.tomatoanon_answer, suffix: ' '},
			{ title: 'Do you want to enable TomatoAnon?', name: 'tomatoanon_enable', type: 'select', options: [ ['-1','I\'m not sure right now'], ['1','Yes, I\'m sure I do'], ['0','No, I definitely won\'t enable it'] ], value: nvram.tomatoanon_enable, suffix: ' '}
		]);
	</script>
</div>

<!-- / / / -->

<div class="section-title">FreshTomato Update Notification System</div>
<div class="section">
	<script>
		createFieldTable('', [
			{ title: 'Enable', name: 'f_tomatoanon_notify', type: 'checkbox', value: nvram.tomatoanon_notify == '1' }
		]);
	</script>
	<ul>
		<li>When a new version of FreshTomato is available, you will be notified on the status-overview page.</li>
	</ul>
</div>

<!-- / / / -->

<div id="footer">
	<span id="footer-msg"></span>
	<input type="button" value="Save" id="save-button" onclick="save()">
	<input type="button" value="Cancel" id="cancel-button" onclick="reloadPage();">
</div>

</td></tr>
</table>
</form>
<script>verifyFields(null, true);</script>
</body>
</html>
