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
<title>[<% ident(); %>] Admin: IP Traffic Monitoring</title>
<link rel="stylesheet" type="text/css" href="tomato.css?rel=<% version(); %>">
<% css(); %>
<script src="tomato.js?rel=<% version(); %>"></script>
<script src="admin-stats.js?rel=<% version(); %>"></script>

<script>

//	<% nvram("cstats_enable,cstats_path,cstats_stime,cstats_offset,cstats_exclude,cstats_include,cstats_sshut,lan_hwaddr,cifs1,cifs2,jffs2_on,cstats_bak,cstats_all,cstats_labels,t_model_name"); %>


statsAdminSetup('cstats', 'ipt', 'IP Traffic');

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

<div class="section-title">IP Traffic Monitoring</div>
<form id="t_fom" method="post" action="tomato.cgi">
	<div class="section" id="config-section">
		<input type="hidden" name="_nextpage" value="admin-iptraffic.asp">
		<input type="hidden" name="_service" value="cstats-restart,firewall-restart">
		<input type="hidden" name="cstats_enable">
		<input type="hidden" name="cstats_path">
		<input type="hidden" name="cstats_sshut">
		<input type="hidden" name="cstats_bak">
		<input type="hidden" name="cstats_all">
		<script>
			switch (nvram.cstats_path) {
			case '':
			case '*nvram':
			case '/jffs/':
			case '/cifs1/':
			case '/cifs2/':
				loc = nvram.cstats_path;
			break;
			default:
				loc = '*user';
			break;
			}
			createFieldTable('', [
				{ title: 'Enable', name: 'f_cstats_enable', type: 'checkbox', value: nvram.cstats_enable == '1' },
				{ title: 'Save History Location', multi: [
/* REMOVE-BEGIN
					{ name: 'f_loc', type: 'select', options: [['','RAM (Temporary)'],['*nvram','NVRAM'],['/jffs/','JFFS2'],['/cifs1/','CIFS 1'],['/cifs2/','CIFS 2'],['*user','Custom Path']], value: loc },
REMOVE-END */
					{ name: 'f_loc', type: 'select', options: [['','RAM (Temporary)'],
/* JFFS2-BEGIN */
					['/jffs/','JFFS2'],
/* JFFS2-END */
/* CIFS-BEGIN */
					['/cifs1/','CIFS 1'],['/cifs2/','CIFS 2'],
/* CIFS-END */
					['*user','Custom Path']], value: loc }, { name: 'f_user', type: 'text', maxlen: 48, size: 50, value: nvram.cstats_path }
				] },
					{ title: 'Save Frequency', indent: 2, name: 'cstats_stime', type: 'select', value: nvram.cstats_stime, options: [
						[1,'Every Hour'],[2,'Every 2 Hours'],[3,'Every 3 Hours'],[4,'Every 4 Hours'],[5,'Every 5 Hours'],[6,'Every 6 Hours'],
						[9,'Every 9 Hours'],[12,'Every 12 Hours'],[24,'Every 24 Hours'],[48,'Every 2 Days'],[72,'Every 3 Days'],[96,'Every 4 Days'],
						[120,'Every 5 Days'],[144,'Every 6 Days'],[168,'Every Week']] },
/* TOMATO64-REMOVE-BEGIN */
					{ title: 'Save On Halt', indent: 2, name: 'f_sshut', type: 'checkbox', value: nvram.cstats_sshut == '1' },
/* TOMATO64-REMOVE-END */
/* TOMATO64-BEGIN */
					{ title: 'Save On Shutdown', indent: 2, name: 'f_sshut', type: 'checkbox', value: nvram.cstats_sshut == '1' },
/* TOMATO64-END */
					{ title: 'Create New File<br><small>(Reset Data)<\/small>', indent: 2, name: 'f_new', type: 'checkbox', value: 0, suffix: ' &nbsp; <b id="newmsg" style="display:none"><small>(note: enable if this is a new file)<\/small><\/b>' },
					{ title: 'Create Backups', indent: 2, name: 'f_bak', type: 'checkbox', value: nvram.cstats_bak == '1' },
				{ title: 'First Day Of The Month', name: 'cstats_offset', type: 'text', value: nvram.cstats_offset, maxlen: 2, size: 4 },
				{ title: 'Do not monitor these IPs', name: 'cstats_exclude', type: 'text', value: nvram.cstats_exclude, maxlen: 512, size: 50, suffix: ' <small>(comma separated list)<\/small>' },
				{ title: 'Monitor these IPs', name: 'cstats_include', type: 'text', value: nvram.cstats_include, maxlen: 2048, size: 50, suffix: ' <small>(comma separated list)<\/small>' },
				{ title: 'Enable Auto-Discovery', name: 'f_all', type: 'checkbox', value: nvram.cstats_all == '1', suffix: ' <small>(automatically include new IPs in monitoring as soon as any traffic is detected)<\/small>' },
				{ title: 'Labels on graphics', name: 'cstats_labels', type: 'select', value: nvram.cstats_stime, options: [[0,'Show known hostnames and IPs'],[1,'Prefer to show only known hostnames, otherwise show IPs'],[2,'Show only IPs']], value: nvram.cstats_labels }
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
	<form id="restore-form" method="post" action="restorestats.cgi?_http_id=<% nv(http_id); %>&_what=ipt" enctype="multipart/form-data">
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
		<li><b>Generally</b> - This feature will only work for <b>IPv4</b> (IPv6 is not possible).</li>
		<li><b>NVRAM</b> - If IP Traffic Monitoring has been enabled, NVRAM values will be added. These NVRAM values will be removed after a reboot if the service is disabled.</li>
	</ul>
</div>

<!-- / / / -->

<script>writeFooter();</script>

</td></tr>
</table>
<script>insOvl();verifyFields(null, true);</script>
</body>
</html>
