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

<script>

//	<% nvram("cstats_enable,cstats_path,cstats_stime,cstats_offset,cstats_exclude,cstats_include,cstats_sshut,lan_hwaddr,cifs1,cifs2,jffs2_on,cstats_bak,cstats_all,cstats_labels,t_model_name"); %>


var xob = null;
const now = new Date();

function _cstatsNvramAdd() {
	form.submitHidden('service.cgi', { _service: 'cstats_nvram-start', _sleep: 1 });
}

function cstatsNvramAdd() {
	var sb, cb, msg;

	/* short check for cstats nvram var. If OK - nothing to do! */
	if ((nvram.cstats_stime.length > 0) &&
	    (nvram.cstats_offset.length > 0))
		return;

	/* check already enabled? - nothing to do! */
	if (nvram.cstats_enable > 0)
		return;

	E('_f_cstats_enable').disabled = 1;
	if ((sb = E('save-button')) != null) sb.disabled = 1;
	if ((cb = E('cancel-button')) != null) cb.disabled = 1;

	if (!confirm("Add IP Traffic to nvram?"))
		return;

	if (xob)
		return;

	if ((xob = new XmlHttp()) == null) {
		_cstatsNvramAdd();
		return;
	}

	if ((msg = E('footer-msg')) != null) {
		msg.innerHTML = 'adding nvram values...';
		msg.style.display = 'inline';
	}

	xob.onCompleted = function(text, xml) {
		if (msg) {
			msg.innerHTML = 'nvram ready';
		}
		setTimeout(
			function() {
				E('_f_cstats_enable').disabled = 0;
				if (sb) sb.disabled = 0;
				if (cb) cb.disabled = 0;
				if (msg) msg.style.display = 'none';
				setTimeout(reloadPage, 1000);
		}, 5000);
		xob = null;
	}
	xob.onError = function() {
		_cstatsNvramAdd();
	}

	xob.post('service.cgi', '_service=cstats_nvram-start'+'&'+'_sleep=1'+'&'+'_ajax=1');
}

function fix(name) {
	var i;
	if (((i = name.lastIndexOf('/')) > 0) || ((i = name.lastIndexOf('\\')) > 0))
		name = name.substring(i + 1, name.length);
	return name;
}

function backupNameChanged() {
	if (location.href.match(/^(http.+?\/.+\/)/))
		E('backup-link').href = RegExp.$1+'stats/'+fix(E('backup-name').value)+'.gz?_http_id='+nvram.http_id+'&_what=ipt';
}

function backupButton() {
	var name;

	name = fix(E('backup-name').value);
	if (name.length <= 1) {
		alert('Invalid filename');
		return;
	}
	location.href = 'stats/'+name+'.gz?_http_id='+nvram.http_id+'&_what=ipt';
}

function restoreButton() {
	var fom;
	var name;
	var i;

	name = fix(E('restore-name').value);
	name = name.toLowerCase();
	if ((name.length <= 3) || (name.substring(name.length - 3, name.length).toLowerCase() != '.gz')) {
		alert('Incorrect filename. Expecting a ".gz" file.');
		return;
	}
	if (!confirm('Restore data from '+name+'?'))
		return;

	E('restore-button').disabled = 1;
	fields.disableAll(E('config-section'), 1);
	fields.disableAll(E('backup-section'), 1);
	fields.disableAll(E('footer'), 1);

	E('restore-form').submit();
}

function getPath() {
	var s = E('_f_loc').value;

	return (s == '*user') ? E('_f_user').value : s;
}

function verifyFields(focused, quiet) {
	var b, v;
	var path;
	var eLoc, eUser, eTime, eOfs;
	var bak;
	var eInc, eExc, eAll, eBak, eLab;

	eLoc = E('_f_loc');
	eUser = E('_f_user');
	eTime = E('_cstats_stime');
	eOfs = E('_cstats_offset');

	eInc = E('_cstats_include');
	eExc = E('_cstats_exclude');
	eAll = E('_f_all');
	eBak = E('_f_bak');

	eLab = E('_cstats_labels');

	b = !E('_f_cstats_enable').checked;
	eLoc.disabled = b;
	eUser.disabled = b;
	eTime.disabled = b;
	eOfs.disabled = b;
	eInc.disabled = b;
	eExc.disabled = b;
	eAll.disabled = b;
	eBak.disabled = b;
	eLab.disabled = b;
	E('_f_new').disabled = b;
	E('_f_sshut').disabled = b;
	E('backup-button').disabled = b;
	E('backup-name').disabled = b;
	E('restore-button').disabled = b;
	E('restore-name').disabled = b;
	ferror.clear(eLoc);
	ferror.clear(eUser);
	ferror.clear(eOfs);
	if (b)
		return 1;

	eInc.disabled = eAll.checked;

	path = getPath();
	E('newmsg').style.display = ((nvram.cstats_path != path) && (path != '*nvram') && (path != '')) ? 'block' : 'none';

	bak = 0;
	v = eLoc.value;
	b = (v == '*user');
	elem.display(eUser, b);
	if (b) {
		if (!v_length(eUser, quiet, 2))
			return 0;
		if (path.substr(0, 1) != '/') {
			ferror.set(eUser, 'Please start at the / root directory.', quiet);
			return 0;
		}
	}
/* JFFS2-BEGIN */
	else if (v == '/jffs/') {
		if (nvram.jffs2_on != '1') {
			ferror.set(eLoc, 'JFFS2 is not enabled.', quiet);
			return 0;
		}
	}
/* JFFS2-END */
/* CIFS-BEGIN */
	else if (v.match(/^\/cifs(1|2)\/$/)) {
		if (nvram['cifs'+RegExp.$1].substr(0, 1) != '1') {
			ferror.set(eLoc, 'CIFS #'+RegExp.$1+' is not enabled.', quiet);
			return 0;
		}
	}
/* CIFS-END */
	else
		bak = 1;

	E('_f_bak').disabled = bak;

	return v_range(eOfs, quiet, 1, 31);
}

function save() {
	var fom, path, en, e, aj;

	if (!verifyFields(null, 0))
		return;

	aj = 1;
	en = E('_f_cstats_enable').checked;
	fom = E('t_fom');
	fom._service.value = 'cstats-restart,firewall-restart';
	if (en) {
		path = getPath();
		if (((E('_cstats_stime').value * 1) <= 48) && ((path == '*nvram')
/* JFFS2-BEGIN */
		    || (path == '/jffs/')
/* JFFS2-END */
		)) {
			if (!confirm('Frequent saving to NVRAM or JFFS2 is not recommended. Continue anyway?'))
				return;
		}
		if ((nvram.cstats_path != path) && (fom.cstats_path.value != path) && (path != '') && (path != '*nvram') && (path.substr(path.length - 1, 1) != '/')) {
			if (!confirm('Note: '+path+' will be treated as a file. If this is a directory, please use a trailing /. Continue anyway?'))
				return;
		}
		fom.cstats_path.value = path;

		if (E('_f_new').checked) {
			fom._service.value = 'cstatsnew-restart';
			aj = 0;
		}
	}

	fom.cstats_path.disabled = !en;
	fom.cstats_enable.value = en ? 1 : 0;
	fom.cstats_sshut.value = E('_f_sshut').checked ? 1 : 0;
	fom.cstats_bak.value = E('_f_bak').checked ? 1 : 0;
	fom.cstats_all.value = E('_f_all').checked ? 1 : 0;

	e = E('_cstats_exclude');
	e.value = e.value.replace(/\s+/g, ',').replace(/,+/g, ',');

	e = E('_cstats_include');
	e.value = e.value.replace(/\s+/g, ',').replace(/,+/g, ',');

	fields.disableAll(E('backup-section'), 1);
	fields.disableAll(E('restore-section'), 1);
	form.submit(fom, aj);
	if (en) {
		fields.disableAll(E('backup-section'), 0);
		fields.disableAll(E('restore-section'), 0);
	}
}

function init() {
	cstatsNvramAdd();
	backupNameChanged();
}
</script>

</head>
<body onload="init()">
<table id="container">
<tr><td colspan="2" id="header">
	<div class="title">Tomato64</div>
	<div class="version">Version <% version(); %> on <% nv("t_model_name"); %></div>
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
			W('<input type="text" size="60" maxlength="128" id="backup-name" name="backup_name" onchange="backupNameChanged()" value="Tomato64_cstats_'+('<% version(); %>'.replace(/\./g, '_'))+'~m'+nvram.lan_hwaddr.replace(/:/g, '').substring(6, 12)+'~'+nvram.t_model_name.replace(/\/| /g, '_')+'~'+now.getFullYear()+('0'+(now.getMonth()+1)).slice(-2)+('0'+now.getDate()).slice(-2)+'">');
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
		<li><b>NVRAM</b> - If IP Traffic Monitoring has been enabled, nvram values will be added. Nvram values will be removed after reboot in case IP Traffic Monitoring will be disabled.</li>
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
<script>verifyFields(null, true);</script>
</body>
</html>
