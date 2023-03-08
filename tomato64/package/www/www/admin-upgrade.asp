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
<link rel="stylesheet" type="text/css" href="tomato.css">
<% css(); %>
<script src="tomato.js"></script>

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
	if (name.search(/\.(bin|trx|chk)$/i) == -1) {
		alert('Expecting a ".bin" or ".trx" file.');
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
	form.addIdAction(fom);

	localStorage.clear();
	fom.submit();
}

function earlyInit() {
	if (nvram.remote_upgrade == 1)
		E('upgradenotice').style.display = 'none';
	else
		E('upgradenotice').style.display = 'block';

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
	<div class="title">FreshTomato</div>
	<div class="version">Version <% version(); %> on <% nv("t_model_name"); %></div>
</td></tr>
<tr id="body"><td id="navi"><script>navi()</script></td>
<td id="content">
<div id="ident"><% ident(); %> | <script>wikiLink();</script></div>

<!-- / / / -->

<div id="afu-input">
	<div class="section-title">Upgrade Firmware</div>
	<div class="section">
		<div class="fields" id="upgradenotice" style="display:none"><div class="about"><b>Note: Remote upgrade is disabled. You can enable it (not recommended) <a href="admin-access.asp">here</a>.</b></div></div>
		<div>
			<div class="afu-form">Select the file to use:</div>
			<form name="form_upgrade" method="post" action="upgrade.cgi" enctype="multipart/form-data">
				<div class="afu-form">
					<input type="file" name="file"> <input type="button" id="afu-upgrade-button" value="Upgrade" onclick="upgrade()">
				</div>
			</form>
			<form name="form_reset" action="javascript:{}">
				<div class="afu-form">
					<input type="checkbox" id="f_reset">&nbsp; After flashing, erase all data in NVRAM memory
				</div>
			</form>

			<table class="afu-info-table"><tr>
				<td>Current Version:</td>
				<td>&nbsp; <% version(1); %></td>
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
<div class="note-warning" id="afu-warn" style="display:none">
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
