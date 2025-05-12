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
<title>[<% ident(); %>] Admin: Upgrade</title>
<link rel="stylesheet" type="text/css" href="tomato.css?rel=<% version(); %>">
<% css(); %>
<script src="tomato.js?rel=<% version(); %>"></script>

<script>

//	<% nvram("jffs2_on,jffs2_auto_unmount,remote_upgrade"); %>

//	<% sysinfo(); %>

function clock() {
	var t = ((new Date()).getTime() - startTime) / 1000;
	elem.setInnerHTML('afu-time', Math.floor(t / 60)+':'+Number(Math.floor(t % 60)).pad(2));
}

function upgrade() {
	var name;
	var i;
	var fom = document.form_upgrade;
	var ext;

	name = fixFile(fom.file.value);
/* TOMATO64-REMOVE-BEGIN */
	if (name.search(/\.(bin|trx|chk)$/i) == -1) {
		alert('Expecting a ".bin" or ".trx" file');
/* TOMATO64-REMOVE-END */
/* TOMATO64-BEGIN */
	if (name.search(/\.(tzst)$/i) == -1) {
		alert('Expecting a ".tzst" file');
/* TOMATO64-END */
		return;
	}
	if (!confirm('Are you sure you want to upgrade using '+name+'?'))
		return;

	E('afu-upgrade-button').disabled = 1;

	elem.display('afu-input', 0);
	E('content').style.verticalAlign = 'middle';
	elem.display('afu-progress', 1);
	elem.display('navi', 0)
	elem.display('ident', 0)

	startTime = (new Date()).getTime();
	setInterval('clock()', 800);

	fom.action += '?_reset='+(E('f_reset').checked ? 1 : 0);
/* TOMATO64-X86_64-BEGIN */
	fom.action += '&_fastreboot='+(E('f_fastreboot').checked ? 1 : 0);
/* TOMATO64-X86_64-END */
	form.addIdAction(fom);

	fom.submit();
}

function earlyInit() {
	E('upgradenotice').style.display = (nvram.remote_upgrade == 1 ? 'none' : 'block');
	E('afu-size').innerHTML = '&nbsp; '+scaleSize(sysinfo.totalfreeram)+'&nbsp; <small>(aprox. size that can be buffered completely in RAM)<\/small>';
/* JFFS2-BEGIN */
	if (nvram.jffs2_on != 0 && nvram.jffs2_auto_unmount == 0) {
		E('afu-warn').style.display = 'block';
		E('afu-input').style.display = 'none';
	}
/* JFFS2-END */
}
</script>
</head>

<body>
<table id="container">
<tr><td colspan="2" id="header">
	<div class="title"><a href="/">Tomato64</a></div>
	<div class="version">Version <% version(); %> on <% nv("t_model_name"); %></div>
</td></tr>
<tr id="body"><td id="navi"><script>navi()</script></td>
<td id="content">
<div id="ident"><% ident(); %> | <script>wikiLink();</script></div>

<!-- / / / -->

<div id="afu-input">
	<div class="section-title">Upgrade Firmware</div>
	<div class="section">
<!--USBAP-BEGIN -->
		<div class="fields"><div class="about"><b>WARNING! This router (with wl_high module) requires Wi-Fi to be turned off for the upgrade to work properly, so only update it using a cable connection!</b></div></div>
<!--USBAP-END -->
		<div class="fields" id="upgradenotice" style="display:none"><div class="about"><b>Note: Remote upgrade is disabled. You can enable it (not recommended) <a href="admin-access.asp">here</a>.</b></div></div>
		<div>
			<form name="form_reset" action="javascript:{}">
				<div class="afu-form">
					<input type="checkbox" id="f_reset">&nbsp; &nbsp; Erase all data in NVRAM. Optional. This is performed between the firmware upload and the reboot.
/* TOMATO64-X86_64-BEGIN */
					<br><input type="checkbox" id="f_fastreboot">&nbsp; &nbsp; After flashing, perform a Fast Reboot <small>(Run locally the first time to ensure correct functionality)</small>
/* TOMATO64-X86_64-END */
				</div>
			</form>
/* TOMATO64-REMOVE-BEGIN */
			<div class="afu-form">Select a valid <a href="https://freshtomato.org/downloads/">firmware</a> to install (.trx or .bin):</div>
/* TOMATO64-REMOVE-END */
/* TOMATO64-BEGIN */
			<div class="afu-form">Select a valid <a href="https://tomato64.org/files/">firmware</a> to install (.tzst):</div>
/* TOMATO64-END */
			<form name="form_upgrade" method="post" action="upgrade.cgi" enctype="multipart/form-data">
				<div class="afu-form">
					<input type="file" name="file" class="upgrade-file">
					<p>
					<input type="button" id="afu-upgrade-button" value="Upgrade" onclick="upgrade()">
				</div>
			</form>
			<table class="afu-info-table"><tr>
				<td>Current Version:</td>
/* TOMATO64-REMOVE-BEGIN */
				<td>&nbsp; <% version(1); %></td>
/* TOMATO64-REMOVE-END */
/* TOMATO64-BEGIN */
				<td>&nbsp; <% version(1); %><% version(4); %></td>
/* TOMATO64-END */
			</tr>
			<tr>
				<td>Free Memory:</td>
				<td id="afu-size"></td>
			</tr></table>
		</div>

	</div>
</div>

<!-- / / / -->

<!--JFFS2-BEGIN -->
<div class="note-warning" id="afu-warn">
	<b>Upgrading firmware with JFFS enabled is not possible. You might now:</b><br><br>
	- <a href="admin-jffs2.asp">Manually unmount the JFFS partition</a> first before re-attempting the upgrade<br><br>
	- <a href="admin-access.asp">Enable the Unmount JFFS during upgrade</a> option<br><br>
	In either cases make sure you have a backup of the content as, due to the operation performed, it is not guaranteed the content of JFFS will be preserved. 
</div>
<!--JFFS2-END -->

<!-- / / / -->

<div id="afu-progress" style="display:none;margin:auto">
	<img src="spin.gif" alt="" style="vertical-align:baseline"> &nbsp;<span id="afu-time">0:00</span><br>
	Please wait while the firmware is uploaded &amp; flashed.<br>
	<b>Warning:</b> Do not interrupt this browser or the router!<br>
</div>

<!-- / / / -->

<div id="footer">
	&nbsp;
</div>

</td></tr>
</table>
<script>earlyInit();</script>
</body>
</html>
