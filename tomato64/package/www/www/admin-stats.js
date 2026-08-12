/*
	FreshTomato GUI
	Copyright (C) 2023 - 2026 pedro
	https://freshtomato.org/

	For use with FreshTomato Firmware only.
	No part of this file may be used without permission.
*/

var statsAdmin = {
	prefix: '',
	what: '',
	label: ''
};

var statsXob = null;

function statsAdminSetup(prefix, what, label) {
	statsAdmin.prefix = prefix;
	statsAdmin.what = what;
	statsAdmin.label = label;
}

function _statsNvramAdd() {
	form.submitHidden('service.cgi', { _service: statsAdmin.prefix+'_nvram-start', _sleep: 1 });
}

function statsNvramAdd() {
	var sb, cb, msg;
	var p = statsAdmin.prefix;

	if ((nvram[p+'_stime'].length > 0) &&
	    (nvram[p+'_offset'].length > 0))
		return;

	if (nvram[p+'_enable'] > 0)
		return;

	E('_f_'+p+'_enable').disabled = 1;
	if ((sb = E('save-button')) != null) sb.disabled = 1;
	if ((cb = E('cancel-button')) != null) cb.disabled = 1;

	if (!confirm('Add '+statsAdmin.label+' to nvram?'))
		return;

	if (statsXob)
		return;

	if ((statsXob = new XmlHttp()) == null) {
		_statsNvramAdd();
		return;
	}

	if ((msg = E('footer-msg')) != null) {
		msg.innerHTML = 'adding nvram values...';
		msg.style.display = 'inline';
	}

	statsXob.onCompleted = function(text, xml) {
		if (msg)
			msg.innerHTML = 'nvram ready';

		setTimeout(
			function() {
				E('_f_'+p+'_enable').disabled = 0;
				if (sb) sb.disabled = 0;
				if (cb) cb.disabled = 0;
				if (msg) msg.style.display = 'none';
				setTimeout(reloadPage, 1000);
			}, 5000);
		statsXob = null;
	}
	statsXob.onError = function() {
		_statsNvramAdd();
	}

	statsXob.post('service.cgi', '_service='+p+'_nvram-start'+'&'+'_sleep=1'+'&'+'_ajax=1');
}

function backupNameChanged() {
	if (location.href.match(/^(http.+?\/.+\/)/))
		E('backup-link').href = RegExp.$1+'stats/'+fixFile(E('backup-name').value)+'.gz?_http_id='+nvram.http_id+'&_what='+statsAdmin.what;
}

function backupButton() {
	var name = fixFile(E('backup-name').value);

	if (name.length <= 1) {
		alert('Invalid filename');
		return;
	}

	location.href = 'stats/'+name+'.gz?_http_id='+nvram.http_id+'&_what='+statsAdmin.what;
}

function restoreButton() {
	var name = fixFile(E('restore-name').value);

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
	var b, v, path, bak;
	var p = statsAdmin.prefix;
	var cstats = (p == 'cstats');
	var eLoc = E('_f_loc');
	var eUser = E('_f_user');
	var eTime = E('_'+p+'_stime');
	var eOfs = E('_'+p+'_offset');
	var eExc = E('_'+p+'_exclude');
	var eBak = E('_f_bak');
	var eInc, eAll, eLab;

	if (cstats) {
		eInc = E('_cstats_include');
		eAll = E('_f_all');
		eLab = E('_cstats_labels');
	}

	b = !E('_f_'+p+'_enable').checked;
	eLoc.disabled = b;
	eUser.disabled = b;
	eTime.disabled = b;
	eOfs.disabled = b;
	eExc.disabled = b;
	eBak.disabled = b;
	if (cstats) {
		eInc.disabled = b;
		eAll.disabled = b;
		eLab.disabled = b;
	}
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

	if (cstats)
		eInc.disabled = eAll.checked;

	path = getPath();
	E('newmsg').style.display = ((nvram[p+'_path'] != path) && (path != '*nvram') && (path != '')) ? (cstats ? 'block' : 'inline') : 'none';

	bak = 0;
	v = eLoc.value;
	b = (v == '*user');
	elem.display(eUser, b);
	if (b) {
		if (cstats) {
			if (!v_length(eUser, quiet, 2))
				return 0;
			if (path.substr(0, 1) != '/') {
				ferror.set(eUser, 'Please start at the / root directory.', quiet);
				return 0;
			}
		}
		else if (!v_path(eUser, quiet, 1))
			return 0;
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
	var fom, path, en, e;
	var p = statsAdmin.prefix;
	var cstats = (p == 'cstats');

	if (!verifyFields(null, 0))
		return;

	en = E('_f_'+p+'_enable').checked;
	fom = E('t_fom');
	fom._service.value = cstats ? 'cstats-restart,firewall-restart' : 'rstats-restart';
	if (en) {
		path = getPath();
		if (((E('_'+p+'_stime').value * 1) <= 48) && ((path == '*nvram')
/* JFFS2-BEGIN */
		    || (path == '/jffs/')
/* JFFS2-END */
		)) {
			if (!confirm('Frequent saving to NVRAM or JFFS2 is not recommended. Continue anyway?'))
				return;
		}
		if ((nvram[p+'_path'] != path) && (fom[p+'_path'].value != path) && (path != '') && (path != '*nvram') && (path.substr(path.length - 1, 1) != '/')) {
			if (!confirm('Note: '+path+' will be treated as a file. If this is a directory, please use a trailing /. Continue anyway?'))
				return;
		}
		fom[p+'_path'].value = path;

		if (E('_f_new').checked) {
			fom._service.value = cstats ? 'cstatsnew-restart,firewall-restart' : 'rstatsnew-restart';
			E('_f_new').checked = 0;
		}
	}

	fom[p+'_path'].disabled = !en;
	fom[p+'_enable'].value = en ? 1 : 0;
	fom[p+'_sshut'].value = E('_f_sshut').checked ? 1 : 0;
	fom[p+'_bak'].value = E('_f_bak').checked ? 1 : 0;

	if (cstats)
		fom.cstats_all.value = E('_f_all').checked ? 1 : 0;

	e = E('_'+p+'_exclude');
	e.value = e.value.replace(/\s+/g, ',').replace(/,+/g, ',');

	if (cstats) {
		e = E('_cstats_include');
		e.value = e.value.replace(/\s+/g, ',').replace(/,+/g, ',');
	}

	fields.disableAll(E('backup-section'), 1);
	fields.disableAll(E('restore-section'), 1);
	form.submit(fom, 1);
	if (en) {
		fields.disableAll(E('backup-section'), 0);
		fields.disableAll(E('restore-section'), 0);
	}
}

function writeStatsBackupName(version) {
	var now = new Date();

	W('<input type="text" size="60" maxlength="128" id="backup-name" name="backup_name" onchange="backupNameChanged()" value="Tomato64_'+statsAdmin.prefix+'_'+version.replace(/\./g, '_')+'~m'+nvram.lan_hwaddr.replace(/:/g, '').substring(6, 12)+'~'+nvram.t_model_name.replace(/\/| /g, '_')+'~'+now.getFullYear()+('0'+(now.getMonth()+1)).slice(-2)+('0'+now.getDate()).slice(-2)+'">');
}

function init() {
	statsNvramAdd();
	backupNameChanged();
}
