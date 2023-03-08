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
<title>[<% ident(); %>] Admin: Configuration</title>
<link rel="stylesheet" type="text/css" href="tomato.css">
<% css(); %>
<script src="tomato.js"></script>

<script>

//	<% nvram("lan_hwaddr,t_features,t_model_name"); %>

//	<% nvstat(); %>

var free_mem = nvstat.free / nvstat.size * 100.0;
const now = new Date();

function backupNameChanged() {
	var name = fixFile(E('backup-name').value);

	if (name.length > 1)
		E('backup-link').href = 'cfg/'+name+'.cfg?_http_id='+nvram.http_id;
	else
		E('backup-link').href = '?';
}

function backupButton() {
	var name = fixFile(E('backup-name').value);

	if (name.length <= 1) {
		alert('Invalid filename');
		return;
	}
	location.href = 'cfg/'+name+'.cfg?_http_id='+nvram.http_id;
}

function restoreButton() {
	var name, i, f;

	name = fixFile(E('restore-name').value);
	name = name.toLowerCase();
	if ((name.indexOf('.cfg') != (name.length - 4)) && (name.indexOf('.cfg.gz') != (name.length - 7))) {
		alert('Incorrect filename. Expecting a ".cfg" file');
		return;
	}
	if (!confirm('Are you sure?'))
		return;

	E('restore-button').disabled = 1;

	f = E('restore-form');
	form.addIdAction(f);

	f.submit();
}

function resetButton() {
	var i;

	i = E('restore-mode').value;
	if (i == 0)
		return;

	if ((i == 2) && (features('!nve'))) {
		if (!confirm('WARNING: Erasing the NVRAM on a '+nvram.t_model_name+' router may be harmful. It may not be able to re-setup the NVRAM correctly after a complete erase. Proceeed anyway?'))
			return;
	}
	if (!confirm('Are you sure?'))
		return;

	E('reset-button').disabled = 1;

	form.submit('reset-form');
}

function init() {
	backupNameChanged();

	if (free_mem <= 5)
		E('notice-msg').innerHTML = '<div id="notice">The NVRAM free space is very low. It is strongly recommended to erase all data in NVRAM memory, and reconfigure the router manually in order to clean up all unused and obsolete entries.<\/div>';
}
</script>
</head>

<body onload="init()">
<table id="container">
<tr><td colspan="2" id="header">
	<div class="title">FreshTomato</div>
	<div class="version">Version <% version(); %> on <% nv("t_model_name"); %></div>
</td></tr>
<tr id="body"><td id="navi"><script>navi()</script></td>
<td id="content">
<div id="ident"><% ident(); %> | <script>wikiLink();</script></div>

<!-- / / / -->

<div class="section-title">Backup Configuration</div>
<div class="section">
	<div>
		<script>
			W('<input type="text" size="60" maxlength="128" id="backup-name" onchange="backupNameChanged()" value="FreshTomato_'+('<% version(); %>'.replace(/\./g, '_'))+'~m'+nvram.lan_hwaddr.replace(/:/g, '').substring(6, 12)+'~'+nvram.t_model_name.replace(/\/| /g, '_')+'~'+now.getFullYear()+('0'+(now.getMonth()+1)).slice(-2)+('0'+now.getDate()).slice(-2)+'">');
		</script>
		<div style="display:inline">.cfg &nbsp;
			<input type="button" name="f_backup_button" id="backup-button" onclick="backupButton()" value="Backup">
		</div>
		<div><a href="#" id="backup-link">Link</a></div>
	</div>
</div>

<!-- / / / -->

<div class="section-title">Restore Configuration</div>
<div class="section">
	<form id="restore-form" method="post" action="cfg/restore.cgi" enctype="multipart/form-data">
		<div>
			Select the configuration file to restore:<br><br>
			<input type="file" id="restore-name" name="filename"> 
			<input type="button" name="f_restore_button" id="restore-button" value="Restore" onclick="restoreButton()">
		</div>
	</form>
</div>

<!-- / / / -->

<div class="section-title">Restore Default Configuration</div>
<div class="section">
	<form id="reset-form" method="post" action="cfg/defaults.cgi">
		<div>
			<select name="mode" id="restore-mode">
				<option value="0">Select...</option>
				<option value="1">Restore default router settings (normal)</option>
				<option value="2">Erase all data in NVRAM memory (thorough)</option>
			</select>
			<input type="button" value="OK" onclick="resetButton()" id="reset-button">
		</div>
	</form>
</div>

<!-- / / / -->

<div class="section-title"></div>
<div class="section">
	<script>
		createFieldTable('', [ { title: 'Total / Free NVRAM:', text: scaleSize(nvstat.size)+' / '+scaleSize(nvstat.free)+' <small>('+(free_mem).toFixed(2)+'%)<\/small>' } ]);
	</script>
</div>

<!-- / / / -->

<div id="notice-msg"></div>

<!-- / / / -->

<div id="footer">
	&nbsp;
</div>

</td></tr>
</table>
</body>
</html>
