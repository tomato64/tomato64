<!DOCTYPE html>
<!--
	Tomato GUI
	FTP Server - !!TB

	For use with Tomato Firmware only.
	No part of this file may be used without permission.
-->
<html lang="en-GB">
<head>
<meta http-equiv="content-type" content="text/html;charset=utf-8">
<meta name="robots" content="noindex,nofollow">
<title>[<% ident(); %>] NAS: FTP Server</title>
<link rel="stylesheet" type="text/css" href="tomato.css">
<% css(); %>
<script src="isup.jsz"></script>
<script src="isup.js"></script>
<script src="tomato.js"></script>

<script>

//	<% nvram("ftp_enable,ftp_super,ftp_anonymous,ftp_dirlist,ftp_port,ftp_max,ftp_ipmax,ftp_staytimeout,ftp_rate,ftp_anonrate,ftp_anonroot,ftp_pubroot,ftp_pvtroot,ftp_custom,ftp_users,ftp_sip,ftp_limit,ftp_tls,log_ftp"); %>

var changed = 0;
var serviceType = 'ftpd';

ftplimit = nvram.ftp_limit.split(',');
if (ftplimit.length != 3) ftplimit = [0,3,60];

var aftg = new TomatoGrid();

aftg.resetNewEditor = function() {
	var f = fields.getAll(this.newEditor);
	ferror.clearAll(f);

	f[0].value = '';
	f[1].value = '';
	f[2].selectedIndex = 0;
	f[3].value = '';
}

aftg.setup = function() {
	this.init('aft-grid', 'sort', 50, [
		{ type: 'text', maxlen: 50 },
		{ type: 'password', maxlen: 50, peekaboo: 1 },
		{ type: 'select', options: [['Read/Write','Read/Write'],['Read Only','Read Only'],['View Only','Browse Only'],['Private','Private']] },
		{ type: 'text', maxlen: 128 }
	]);
	this.headerSet(['Username','Password','Access','Root Directory*']);

	var s = nvram.ftp_users.split('>');
	for (var i = 0; i < s.length; ++i) {
		var t = s[i].split('<');
		if (t.length == 3) {
			t.push('');
		}
		if (t.length == 4) {
			this.insertData(-1, t);
		}
	}

	this.showNewEditor();
	this.resetNewEditor();
}

aftg.exist = function(f, v) {
	var data = this.getAllData();
	for (var i = 0; i < data.length; ++i) {
		if (data[i][f] == v)
			return true;
	}

	return false;
}

aftg.existName = function(name) {
	return this.exist(0, name);
}

aftg.sortCompare = function(a, b) {
	var da = a.getRowData();
	var db = b.getRowData();
	var r = cmpText(da[this.sortColumn], db[this.sortColumn]);

	return this.sortAscending ? r : -r;
}

aftg.rpDel = function(e) {
	changed = 1;
	e = PR(e);
	TGO(e).moving = null;
	e.parentNode.removeChild(e);
	this.recolor();
	this.resort();
	this.rpHide();
}

aftg.verifyFields = function(row, quiet) {
	changed = 1;
	var f, s;
	f = fields.getAll(row);

	ferror.clear(f[0]);
	ferror.clear(f[1]);
	ferror.clear(f[3]);

	if (!v_length(f[0], quiet, 1)) return 0;

	s = f[0].value.trim().replace(/\s+/g, ' ');
	if (s.length > 0) {
		if (s.search(/^[a-zA-Z0-9_\-]+$/) == -1) {
			ferror.set(f[0], 'Invalid user name. Only characters "A-Z 0-9 - _" are allowed.', quiet);
			return 0;
		}
		if (this.existName(s)) {
			ferror.set(f[0], 'Duplicate user name.', quiet);
			return 0;
		}
		if (s == 'root' || s == 'admin') {
			ferror.set(f[0], 'User names "root" and "admin" are not allowed.', quiet);
			return 0;
		}
		f[0].value = s;
	}

	if (!v_length(f[1], quiet, 1))
		return 0;
	if (!v_nodelim(f[1], quiet, 'Password', 1))
		return 0;
	if (f[2].value == 'Private') {
		f[3].value = '';
		f[3].disabled = true;
	}
	else {
		f[3].disabled = false;
		if (!v_nodelim(f[3], quiet, 'Root Directory', 1) || !v_path(f[3], quiet, 0))
			return 0;
	}

	return 1;
}

function verifyFields(focused, quiet) {
	if (focused)
		changed = 1;

	var a, b;
	var ok = 1;

	a = E('_ftp_enable').value;
	b = E('_f_limit').checked;

	elem.display(PR('_f_ftp_sip'), (a == 1));
	elem.display(PR('_f_limit_hit'), PR('_f_limit_sec'), (a == 1 && b));

	E('_f_limit').disabled = (a != 1);

	if (!v_port('_ftp_port', quiet || !ok)) ok = 0;
	if (!v_range('_ftp_max', quiet || !ok, 0, 12)) ok = 0;
	if (!v_range('_ftp_ipmax', quiet || !ok, 0, 12)) ok = 0;
	if (!v_range('_ftp_rate', quiet || !ok, 0, 99999)) ok = 0;
	if (!v_range('_ftp_anonrate', quiet || !ok, 0, 99999)) ok = 0;
	if (!v_range('_ftp_staytimeout', quiet || !ok, 0, 65535)) ok = 0;
	if (!v_length('_ftp_custom', quiet || !ok, 0, 2048)) ok = 0;
	if (!v_path('_ftp_pubroot', quiet || !ok, 0)) ok = 0;
	if (!v_path('_ftp_pvtroot', quiet || !ok, 0)) ok = 0;
	if (!v_path('_ftp_anonroot', quiet || !ok, 0)) ok = 0;
	if (a == 1 && b) {
		if (!v_range('_f_limit_hit', quiet || !ok, 1, 100)) ok = 0;
		if (!v_range('_f_limit_sec', quiet || !ok, 3, 3600)) ok = 0;
	}

	if (a == 1) {
		b = E('_f_ftp_sip');
		if ((b.value.length) && (!_v_iptaddr(b, quiet || !ok, 15, 1, 1)))
			ok = 0;
		else
			ferror.clear(b);
	}

	return ok;
}

function save_pre() {
	if (aftg.isEditing())
		return 0;
	if (!verifyFields(null, 0))
		return 0;
	return 1;
}

function save(nomsg) {
	save_pre();
	if (!nomsg) show(); /* update '_service' field first */

	var fom = E('t_fom');
	var data = aftg.getAllData();
	var r = [];
	for (var i = 0; i < data.length; ++i) r.push(data[i].join('<'));
	fom.ftp_users.value = r.join('>');

	fom.ftp_sip.value = fom.f_ftp_sip.value.split(/\s*,\s*/).join(',');
	fom.ftp_super.value = fom._f_ftp_super.checked ? 1 : 0;
	fom.ftp_tls.value = fom._f_ftp_tls.checked ? 1 : 0;
	fom.log_ftp.value = fom._f_log_ftp.checked ? 1 : 0;
	fom._nofootermsg.value = (nomsg ? 1 : 0);
	fom.ftp_limit.value = (fom._f_limit.checked ? 1 : 0)+','+fom._f_limit_hit.value+','+fom._f_limit_sec.value;

	form.submit(fom, 1);

	changed = 0;
}

function earlyInit() {
	show();
	aftg.setup();
	verifyFields(null, 1);
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

<input type="hidden" name="_nextpage" value="nas-ftp.asp">
<input type="hidden" name="_service" value="">
<input type="hidden" name="_nofootermsg">
<input type="hidden" name="ftp_super">
<input type="hidden" name="ftp_tls">
<input type="hidden" name="log_ftp">
<input type="hidden" name="ftp_users">
<input type="hidden" name="ftp_sip">
<input type="hidden" name="ftp_limit">

<!-- / / / -->

<div class="section-title">Status</div>
<div class="section">
	<div class="fields">
		<span id="_ftpd_notice"></span>
		<input type="button" id="_ftpd_button" value="">
		&nbsp; <img src="spin.gif" alt="" id="spin">
	</div>
</div>

<!-- / / / -->

<div class="section-title">FTP Server Configuration</div>
<div class="section">
	<script>
		createFieldTable('', [
			{ title: 'Enable on Start', name: 'ftp_enable', type: 'select', options: [['0', 'No'],['1', 'Yes, WAN and LAN'],['2', 'Yes, LAN only']], value: nvram.ftp_enable },
/* HTTPS-BEGIN */
			{ title: 'TLS support', indent: 2, name: 'f_ftp_tls', type: 'checkbox', suffix: ' <small>uses router httpd cert/key<\/small>', value: nvram.ftp_tls == 1 },
/* HTTPS-END */
			{ title: 'FTP Port', indent: 2, name: 'ftp_port', type: 'text', maxlen: 5, size: 7, value: fixPort(nvram.ftp_port, 21) },
			{ title: 'Allowed Remote<br>Address(es)', indent: 2, name: 'f_ftp_sip', type: 'text', maxlen: 512, size: 64, value: nvram.ftp_sip, suffix: '<br><small>optional; ex: "1.1.1.1", "1.1.1.0/24", "1.1.1.1 - 2.2.2.2" or "me.example.com"<\/small>' },
			{ title: 'Anonymous Users Access', name: 'ftp_anonymous', type: 'select', options: [['0', 'Disabled'],['1', 'Read/Write'],['2', 'Read Only'],['3', 'Write Only']], value: nvram.ftp_anonymous },
			{ title: 'Allow Admin Login*', name: 'f_ftp_super', type: 'checkbox', suffix: ' <small>allows users to connect with admin account<\/small>', value: nvram.ftp_super == 1 },
			{ title: 'Log FTP requests and responses', name: 'f_log_ftp', type: 'checkbox', value: nvram.log_ftp == 1 }
		]);
	</script>
	<div><small>*&nbsp;Avoid using this option when FTP server is enabled for WAN. IT PROVIDES FULL ACCESS TO THE ROUTER FILE SYSTEM!</small></div>
</div>

<!-- / / / -->

<div class="section-title">Directories</div>
<div class="section">
	<script>
		createFieldTable('', [
			{ title: 'Anonymous Root Directory*', name: 'ftp_anonroot', type: 'text', maxlen: 256, size: 32, suffix: ' <small>for anonymous connections<\/small>', value: nvram.ftp_anonroot },
			{ title: 'Public Root Directory*', name: 'ftp_pubroot', type: 'text', maxlen: 256, size: 32, suffix: ' <small>for authenticated users access, if not specified for the user<\/small>', value: nvram.ftp_pubroot },
			{ title: 'Private Root Directory**', name: 'ftp_pvtroot', type: 'text', maxlen: 256, size: 32, suffix: ' <small>for authenticated users access in private mode<\/small>', value: nvram.ftp_pvtroot },
			{ title: 'Directory Listings', name: 'ftp_dirlist', type: 'select', options: [['0', 'Enabled'],['1', 'Disabled'],['2', 'Disabled for Anonymous']], suffix: ' <small>always enabled for Admin<\/small>', value: nvram.ftp_dirlist }
		]);
	</script>
	<div><small>*&nbsp;&nbsp;When no directory is specified, /mnt is used as a root directory.</small></div>
	<div><small>**&nbsp;In private mode, the root directory is the directory under the "Private Root Directory" with the name matching the name of the user.</small></div>
</div>

<!-- / / / -->

<div class="section-title">Limits</div>
<div class="section">
	<script>
		createFieldTable('', [
			{ title: 'Maximum Users Allowed to Log in', name: 'ftp_max', type: 'text', maxlen: 5, size: 7, suffix: ' <small>0 - unlimited<\/small>', value: nvram.ftp_max },
			{ title: 'Maximum Connections from the same IP', name: 'ftp_ipmax', type: 'text', maxlen: 5, size: 7, suffix: ' <small>0 - unlimited<\/small>', value: nvram.ftp_ipmax },
			{ title: 'Maximum Bandwidth for Anonymous Users', name: 'ftp_anonrate', type: 'text', maxlen: 5, size: 7, suffix: ' <small>KBytes/sec; 0 - unlimited<\/small>', value: nvram.ftp_anonrate },
			{ title: 'Maximum Bandwidth for Authenticated Users', name: 'ftp_rate', type: 'text', maxlen: 5, size: 7, suffix: ' <small>KBytes/sec; 0 - unlimited<\/small>', value: nvram.ftp_rate },
			{ title: 'Idle Timeout', name: 'ftp_staytimeout', type: 'text', maxlen: 5, size: 7, suffix: ' <small>seconds (0 - no timeout)<\/small>', value: nvram.ftp_staytimeout },
			{ title: 'Limit Connection Attempts', name: 'f_limit', type: 'checkbox', value: ftplimit[0] != 0 },
			{ title: '', multi: [
				{ name: 'f_limit_hit', type: 'text', maxlen: 4, size: 6, suffix: '&nbsp; <small>every<\/small> &nbsp;', value: ftplimit[1] },
				{ name: 'f_limit_sec', type: 'text', maxlen: 4, size: 6, suffix: '&nbsp; <small>seconds<\/small>', value: ftplimit[2] }
			] }
		]);
	</script>
</div>

<!-- / / / -->

<div class="section-title">Custom Configuration</div>
<div class="section">
	<script>
		createFieldTable('', [
			{ title: '<a href="http://vsftpd.beasts.org/vsftpd_conf.html" class="new_window">Vsftpd<\/a><br>Custom Configuration', name: 'ftp_custom', type: 'textarea', value: nvram.ftp_custom }
		]);
	</script>
</div>

<!-- / / / -->

<div class="section-title">User Accounts</div>
<div class="section">
	<div class="tomato-grid" id="aft-grid"></div>
	<div><small>*&nbsp;&nbsp;When no Root Directory is specified for the user, the default "Public Root Directory" is used.</small></div>
</div>

<!-- / / / -->

<div class="section-title">Notes</div>
<div class="section">
	<ul>
		<li>Don't use the same username as when logging into the web panel - it will be rejected until you select <br>'Allow Admin Login' (with all the consequences).</li>
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
<script>earlyInit();</script>
</body>
</html>
