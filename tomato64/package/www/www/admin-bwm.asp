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
<title>[<% ident(); %>] Admin: Bandwidth Monitoring</title>
<link rel="stylesheet" type="text/css" href="tomato.css?rel=<% version(); %>">
<% css(); %>
<script src="tomato.js?rel=<% version(); %>"></script>
<script src="admin-stats.js?rel=<% version(); %>"></script>

<script>

//	<% nvram("rstats_enable,rstats_path,rstats_stime,rstats_offset,rstats_exclude,rstats_sshut,lan_hwaddr,cifs1,cifs2,jffs2_on,rstats_bak,t_model_name"); %>

statsAdminSetup('rstats', 'bwm', 'Bandwidth Monitoring');

</script>
</head>

<body onload="init()">
<table id="container">
<tr><td colspan="2" id="header">
	<div class="title"><a href="/">Tomato64</a></div>
	<div class="version">Version <% version(); %> on <% nv("t_model_name"); %><span class="blinking bl2"><script><% anonupdate(); %> anon_update()</script>&nbsp;</span></div>
</td></tr>
<tr id="body"><td id="navi"><script>navi()</script></td>
<td id="content">
<div id="ident"><% ident(); %> | <script>wikiLink();</script></div>

<!-- / / / -->

<div class="section-title">Bandwidth Monitoring</div>
<form id="t_fom" method="post" action="tomato.cgi">
	<div class="section" id="config-section">
		<input type="hidden" name="_nextpage" value="admin-bwm.asp">
		<input type="hidden" name="_service" value="rstats-restart">
		<input type="hidden" name="rstats_enable">
		<input type="hidden" name="rstats_path">
		<input type="hidden" name="rstats_sshut">
		<input type="hidden" name="rstats_bak">
		<script>
			switch (nvram.rstats_path) {
				case '':
				case '*nvram':
				case '/jffs/':
				case '/cifs1/':
				case '/cifs2/':
					loc = nvram.rstats_path;
				break;
				default:
					loc = '*user';
				break;
			}
			createFieldTable('', [
				{ title: 'Enable', name: 'f_rstats_enable', type: 'checkbox', value: nvram.rstats_enable == '1' },
				{ title: 'Save History Location', multi: [ { name: 'f_loc', type: 'select', options: [['','RAM (Temporary)'],['*nvram','NVRAM'],
/* JFFS2-BEGIN */
					['/jffs/','JFFS2'],
/* JFFS2-END */
/* CIFS-BEGIN */
					['/cifs1/','CIFS 1'],['/cifs2/','CIFS 2'],
/* CIFS-END */
					['*user','Custom Path']], value: loc }, { name: 'f_user', type: 'text', maxlen: 48, size: 50, value: nvram.rstats_path }
				] },
					{ title: 'Save Frequency', indent: 2, name: 'rstats_stime', type: 'select', value: nvram.rstats_stime, options: [
						[1,'Every Hour'],[2,'Every 2 Hours'],[3,'Every 3 Hours'],[4,'Every 4 Hours'],[5,'Every 5 Hours'],[6,'Every 6 Hours'],
						[9,'Every 9 Hours'],[12,'Every 12 Hours'],[24,'Every 24 Hours'],[48,'Every 2 Days'],[72,'Every 3 Days'],[96,'Every 4 Days'],
						[120,'Every 5 Days'],[144,'Every 6 Days'],[168,'Every Week']] },
					{ title: 'Save On Halt/Reboot', indent: 2, name: 'f_sshut', type: 'checkbox', value: nvram.rstats_sshut == '1' },
					{ title: 'Create New File<br><small>(Reset Data)<\/small>', indent: 2, name: 'f_new', type: 'checkbox', value: 0, suffix: ' &nbsp; <b id="newmsg" style="display:none"><small>(note: enable if this is a new file)<\/small><\/b>' },
					{ title: 'Create Backups', indent: 2, name: 'f_bak', type: 'checkbox', value: nvram.rstats_bak == '1' },
				{ title: 'First Day Of The Month', name: 'rstats_offset', type: 'text', value: nvram.rstats_offset, maxlen: 2, size: 4 },
				{ title: 'Excluded Interfaces', name: 'rstats_exclude', type: 'text', value: nvram.rstats_exclude, maxlen: 64, size: 50, suffix: '&nbsp;<small>(comma separated list)<\/small>' }
			]);
		</script>
	</div>
</form>

<!-- / / / -->

<div class="section-title">Backup</div>
<div class="section" id="backup-section">
	<div>
		<script>
			writeStatsBackupName('<% version(); %>');
		</script>
		<div style="display:inline">.gz &nbsp;
			<input type="button" name="f_backup_button" id="backup-button" onclick="backupButton()" value="Backup">
		</div>
		<div><a href="#" id="backup-link">Link</a></div>
	</div>
</div>

<!-- / / / -->

<div class="section-title">Restore</div>
<div class="section" id="restore-section">
	<form id="restore-form" method="post" action="restorestats.cgi?_http_id=<% nv(http_id); %>&_what=bwm" enctype="multipart/form-data">
		<div>
			<input type="file" id="restore-name" name="restore-name">
			<input type="button" name="f_restore_button" id="restore-button" value="Restore" onclick="restoreButton()">
		</div>
	</form>
</div>

<!-- / / / -->

<div class="section-title">Notes</div>
<div class="section">
	<ul>
		<li><b>NVRAM</b> - If Bandwidth Monitoring has been enabled, NVRAM values will be added. These NVRAM values will be removed after a reboot if the service is disabled.</li>
	</ul>
</div>

<!-- / / / -->

<script>writeFooter();</script>

</td></tr>
</table>
<script>insOvl();verifyFields(null, 1);</script>
</body>
</html>
