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
<link rel="stylesheet" type="text/css" href="tomato.css?rel=<% version(); %>">
<% css(); %>
<script src="tomato.js?rel=<% version(); %>"></script>

<script>

//	<% nvram("tomatoanon_enable,tomatoanon_notify"); %>

var anon_link = '&nbsp;&nbsp;<a href="https://anon.freshtomato.org/index.php?search=9&routerid=<% nv('tomatoanon_id'); %>" class="new_window"><i>[Checkout my router]<\/i><\/a>';

function verifyFields(focused, quiet) {
	return 1;
}

function save() {
	if (verifyFields(null, 0) == 0) return;
	var fom = E('t_fom');

	fom.tomatoanon_notify.value = E('_f_tomatoanon_notify').checked ? 1 : 0;
	fom.tomatoanon_enable.value = E('_f_tomatoanon_enable').checked ? 1 : 0;

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
	<div class="title"><a href="/">Tomato64</a></div>
	<div class="version">Version <% version(); %> on <% nv("t_model_name"); %><span class="blinking bl2"><script><% anonupdate(); %> anon_update()</script>&nbsp;</span></div>
</td></tr>
<tr id="body"><td id="navi"><script>navi()</script></td>
<td id="content">
<div id="ident"><% ident(); %> | <script>wikiLink();</script></div>

<!-- / / / -->

<input type="hidden" name="_nextpage" value="admin-tomatoanon.asp">
<input type="hidden" name="_service" value="tomatoanon-restart">
<input type="hidden" name="tomatoanon_notify">
<input type="hidden" name="tomatoanon_enable">

<!-- / / / -->

<div class="section-title">TomatoAnon <script>W(anon_link);</script></div>
<div class="section">
	<div class="about">
		TomatoAnon sends (to the database) information about the router model and the installed version of Tomato.<br>
		The information submitted is 100% anonymous and will be used ONLY for statistical purposes.<br>
		TomatoAnon is open source and written in bash. Anyone can freely view the content sent to the database.<br>
		<br>
		<b>TomatoAnon does NOT send any private or personal information (such as MAC, IP addresses, etc.).</b><br>
		<br>
		The following data is sent:<br>
		- MD5SUM of WAN+LAN MAC addresses - this identifies the router, ex: 1c1dbd4202d794251ec1acf1211bb2c8<br>
		- Router model, ex: Asus RT-AC3200<br>
		- Tomato version installed, ex: 2025.2 K26ARM7 USB<br>
		- Buildtype, ex: AIO-128K<br>
		- Tomato MOD, in our case: Tomato64<br>
		- Uptime of your router, ex: 7 days<br>
		<br>
/* TOMATO64-REMOVE-BEGIN */
		The submitted results can be viewed on the <a href="https://anon.freshtomato.org" class="new_window"><b>anon.freshtomato.org</b></a> page.<br>
		<br>
		<div class="tomatoanon-note">
			Please consider including tomatoanon to support both myself (<a href="https://github.com/pedro0311" class="new_window"><b>pedro</b></a>) and the entire <a href="https://github.com/FreshTomato-Project" class="new_window"><b>FreshTomato project</b></a>.<br>
			Enabling tomatoanon would allow the community to better understand how many of us are actively using FT.<br>
			This insight is crucial - it can help demonstrate the project's relevance, attract new users, and potentially bring in new contributors or even maintainers in the future!<br><br>
			Thank you for your time, dedication, and for considering this request.
		</div>
/* TOMATO64-REMOVE-END */
/* TOMATO64-BEGIN */
		The submitted results can be viewed on the <a href="https://anon.freshtomato.org" class="new_window"><b>anon.freshtomato.org</b></a> page.
/* TOMATO64-END */
	</div>
	<script>
		createFieldTable('', [ { title: 'Enable', name: 'f_tomatoanon_enable', type: 'checkbox', value: nvram.tomatoanon_enable == 1 } ]);
	</script>
	<ul>
		<li>Note: If you have TomatoAnon enabled and want to update database now, click 'Save'.</li>
	</ul>
</div>

<!-- / / / -->

<div class="section-title">Update Notification System</div>
<div class="section">
	<script>
		createFieldTable('', [ { title: 'Enable', name: 'f_tomatoanon_notify', type: 'checkbox', value: nvram.tomatoanon_notify == 1 } ]);
	</script>
	<ul>
		<li>Note: As soon as a new version of Tomato64 is available, you will be notified on the Overview page.</li>
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
<script>insOvl();verifyFields(null, true);</script>
</body>
</html>
