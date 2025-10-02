<!DOCTYPE html>
<!--
	Tomato GUI
	Media Server Settings - !!TB

	For use with Tomato Firmware only.
	No part of this file may be used without permission.
-->
<html lang="en-GB">
<head>
<meta http-equiv="content-type" content="text/html;charset=utf-8">
<meta name="robots" content="noindex,nofollow">
<title>[<% ident(); %>] NAS: Media Server</title>
<link rel="stylesheet" type="text/css" href="tomato.css?rel=<% version(); %>">
<% css(); %>
<script src="isup.jsx?_http_id=<% nv(http_id); %>"></script>
<script src="isup.js?rel=<% version(); %>"></script>
<script src="tomato.js?rel=<% version(); %>"></script>

<script>

//	<% nvram("ms_enable,ms_port,ms_dirs,ms_dbdir,ms_ifname,ms_tivo,ms_stdlna,ms_sas,ms_autoscan,ms_custom,cifs1,cifs2,jffs2_on,lan_ifname,lan_ipaddr"); %>

var changed = 0;
var reinit = 0;
var serviceType = 'minidlna';
var mediatypes = [['','All Media Files'],['A','Audio only'],['V','Video only'],['P','Images only'],['PV','Images and Video']];
var http_if = '', http_port = 0;

var msg = new TomatoGrid();

msg.setup = function() {
	this.init('ms-grid', 'sort', 50, [ { type: 'text', maxlen: 256 },{ type: 'select', options: mediatypes } ]);
	this.headerSet(['Directory', 'Content Filter']);

	var s = (''+nvram.ms_dirs).split('>');
	for (var i = 0; i < s.length; ++i) {
		var t = s[i].split('<');
		if (t.length == 2)
			this.insertData(-1, t);
	}

	this.sort(0);
	this.showNewEditor();
	this.resetNewEditor();
}

msg.resetNewEditor = function() {
	var f;

	f = fields.getAll(this.newEditor);
	ferror.clearAll(f);
	f[0].value = '';
	f[1].selectedIndex = 0;
}

msg.dataToView = function(data) {
	var b = [];
	var i;

	b.push(escapeHTML(''+data[0]));
	for (i = 0; i < mediatypes.length; ++i) {
		if (mediatypes[i][0] == (''+data[1])) {
			b.push(mediatypes[i][1]);
			break;
		}
	}
	if (b.length < 2)
		b.push(mediatypes[0][1]);

	return b;
}

msg.rpDel = function(e) {
	changed = 1;
	e = PR(e);
	TGO(e).moving = null;
	e.parentNode.removeChild(e);
	this.recolor();
	this.resort();
	this.rpHide();
}

msg.verifyFields = function(row, quiet) {
	var ok = 1;
	var f;
	f = fields.getAll(row);

	if (!v_nodelim(f[0], quiet, 'Directory', 1) || !v_path(f[0], quiet, 1))
		ok = 0;

	changed |= ok;

	return ok;
}

var xob = null;

function updateNotice() {
	if (xob)
		return;

	xob = new XmlHttp();
	xob.onCompleted = function(text, xml) {
		if (text.length)
			text = '<div id="notice">'+text.replace(/\n/g, '<br>')+'<\/div><br style="clear:both">';

		elem.setInnerHTML('notice-msg', text);

		xob = null;
		setTimeout(updateNotice, 5000);
	}
	xob.onError = function(ex) { xob = null; }
	xob.post('update.cgi', 'exec=notice&arg0=dlna');
}

var cmd = null;

function updatePort() {
	if (cmd)
		return;

	cmd = new XmlHttp();
	cmd.onCompleted = function(text, xml) {
		http_port = text;
		cmd = null;
		setTimeout(updatePort, 2000);
	}
	cmd.onError = function(ex) { cmd = null; }
	var commands = '/bin/netstat -ptln | grep \'^tcp.*minidlna$\' | awk \'{print $4}\' | cut -d : -f2';
	cmd.post('shell.cgi', 'action=execute&nojs=1&command='+escapeCGI(commands.replace(/\r/g, '')));

}

function verifyFields(focused, quiet) {
	if (focused && focused != E('_f_ms_enable')) /* except on/off */
		changed = 1;
	if (focused && focused == E('_f_ms_rescan')) /* rescan */
		reinit = 1;

	var ok = 1;
	var b, v, i, n, eLoc, eUser, once = 1;

	for (i = 0; i <= MAX_BRIDGE_ID; ++i) {
		n = (i == 0 ? '' : i.toString());
		E('_f_ms_lan'+i).disabled = (nvram['lan'+n+'_ifname'].length < 1);
		if (nvram['lan'+n+'_ifname'].length < 1)
			E('_f_ms_lan'+i).checked = 0;
		else if (E('_f_ms_lan'+i).checked && once) {
			http_if = nvram['lan'+n+'_ipaddr'];
			once = 0;
		}
	}

	eLoc = E('_f_loc');
	eUser = E('_f_user');

	ferror.clear(eLoc);
	ferror.clear(eUser);

	v = eLoc.value;
	b = (v == '*user');
	elem.display(eUser, b);
	elem.display(PR('_f_ms_sas'), (v != ''));

	if (b) {
		if (!v_path(eUser, quiet || !ok, 1))
			ok = 0;
	}
/* JFFS2-BEGIN */
	else if (v == '/jffs/dlna') {
		if (nvram.jffs2_on != '1') {
			ferror.set(eLoc, 'JFFS is not enabled', quiet || !ok);
			ok = 0;
		}
		else
			ferror.clear(eLoc);
	}
/* JFFS2-END */

	if (!v_range('_ms_port', quiet || !ok, 0, 65535))
		ok = 0;

	if (!v_length('_ms_custom', quiet || !ok, 0, 4096))
		ok = 0;

	return ok;
}

function save_pre() {
	if (msg.isEditing())
		return 0;
	if (!verifyFields(null, 0))
		return 0;
	return 1;
}

function save(nomsg) {
	save_pre();
	if (!nomsg) show(); /* update '_service' field first */

	var fom = E('t_fom');

	if (!isup.minidlna && !nomsg && fom._f_ms_rescan.checked) {
		alert('minidlna is not running.\nTo rescan media, check config and click "Start Now"');
		return;
	}
	if (isup.minidlna && nomsg && fom._f_ms_rescan.checked) {
		E('_minidlna_button').disabled = 0;
		E('_minidlna_status').disabled = 0;
		E('spin').style.display = 'none';
		alert('Check config, and click "Save" to rescan media');
		return;
	}

	fom.ms_enable.value = fom._f_ms_enable.checked ? 1 : 0;
	nvram.ms_enable = fom._f_ms_enable.checked ? 1 : 0;
	fom.ms_tivo.value = fom._f_ms_tivo.checked ? 1 : 0;
	fom.ms_stdlna.value = fom._f_ms_stdlna.checked ? 1 : 0;
	fom.ms_rescan.value = fom._f_ms_rescan.checked ? 1 : 0;
	fom.ms_sas.value = fom._f_ms_sas.checked ? 1 : 0;
	fom.ms_autoscan.value = fom._f_ms_autoscan.checked ? 1 : 0;
	fom._nofootermsg.value = (nomsg ? 1 : 0);

	var ms_ifnames = '';
	for (var i = 0; i <= MAX_BRIDGE_ID; ++i)
		E('_f_ms_lan'+i.toString()).checked ? ms_ifnames += (ms_ifnames == '' ? '' : ',')+'br'+i.toString() : '';

	fom.ms_ifname.value = ms_ifnames;

	var s = fom._f_loc.value;
	fom.ms_dbdir.value = (s == '*user') ? fom._f_user.value : s;

	var data = msg.getAllData();
	var r = [];
	for (var i = 0; i < data.length; ++i)
		r.push(data[i].join('<'));

	fom.ms_dirs.value = r.join('>');

	form.submit(fom, 1);

	/* reset */
	changed = 0;
	reinit = 0;
	nvram.ms_rescan = 0;
	fom.f_ms_rescan.checked = 0;
}

function earlyInit() {
	show();
	msg.setup();
	verifyFields(null, 1);
}

function init() {
	up.initPage(250, 5);
	updateNotice();
	updatePort();
}
</script>
</head>

<body onload="init()">
<form id="t_fom" method="post" action="tomato.cgi">
<table id="container">
<tr><td colspan="2" id="header">
	<div class="title"><a href="/">Tomato64</a></div>
	<div class="version">Version <% version(); %> on <% nv("t_model_name"); %></div>
</td></tr>
<tr id="body"><td id="navi"><script>navi()</script></td>
<td id="content">
<div id="ident"><% ident(); %> | <script>wikiLink();</script></div>

<!-- / / / -->

<input type="hidden" name="_nextpage" value="nas-media.asp">
<input type="hidden" name="_service" value="">
<input type="hidden" name="_nofootermsg">
<input type="hidden" name="ms_enable">
<input type="hidden" name="ms_ifname">
<input type="hidden" name="ms_dirs">
<input type="hidden" name="ms_dbdir">
<input type="hidden" name="ms_tivo">
<input type="hidden" name="ms_stdlna">
<input type="hidden" name="ms_rescan">
<input type="hidden" name="ms_sas">
<input type="hidden" name="ms_autoscan">

<!-- / / / -->

<div class="section-title">Status</div>
<div class="section">
	<div class="fields">
		<span id="_minidlna_notice"></span><input type="button" id="_minidlna_button" value="">
		<input type="button" id="_minidlna_status" value="Open status page in new tab" class="new_window" onclick="window.open('http://'+http_if+':'+http_port+'')">
		&nbsp; <img src="spin.gif" alt="" id="spin">
	</div>
</div>

<!-- / / / -->

<div class="section-title">Media / DLNA Server</div>
<div class="section">
	<script>
		switch (nvram.ms_dbdir) {
			case '':
			case '/jffs/dlna':
			case '/cifs1/dlna':
			case '/cifs2/dlna':
				loc = nvram.ms_dbdir;
			break;
			default:
				loc = '*user';
			break;
		}

		var lan_arr = nvram.ms_ifname.split(',');
/* TOMATO64-REMOVE-BEGIN */
		var ms_lan = [0,0,0,0];
/* TOMATO64-REMOVE-END */
/* TOMATO64-BEGIN */
		var ms_lan = [0,0,0,0,0,0,0,0];
/* TOMATO64-END */
		for (var i = 0; i <= MAX_BRIDGE_ID; ++i) {
			for (var j = 0; j < lan_arr.length; ++j) {
				if (lan_arr[j] == 'br'+i.toString())
					ms_lan[i] = 1;
			}
		}

		createFieldTable('', [
			{ title: 'Enable on Start', name: 'f_ms_enable', type: 'checkbox', value: nvram.ms_enable == 1 },
				{ title: 'LAN0', name: 'f_ms_lan0', type: 'checkbox', value: ms_lan[0] == 1 },
				{ title: 'LAN1', name: 'f_ms_lan1', type: 'checkbox', value: ms_lan[1] == 1 },
				{ title: 'LAN2', name: 'f_ms_lan2', type: 'checkbox', value: ms_lan[2] == 1 },
				{ title: 'LAN3', name: 'f_ms_lan3', type: 'checkbox', value: ms_lan[3] == 1 },
/* TOMATO64-BEGIN */
				{ title: 'LAN4', name: 'f_ms_lan4', type: 'checkbox', value: ms_lan[4] == 1 },
				{ title: 'LAN5', name: 'f_ms_lan5', type: 'checkbox', value: ms_lan[5] == 1 },
				{ title: 'LAN6', name: 'f_ms_lan6', type: 'checkbox', value: ms_lan[6] == 1 },
				{ title: 'LAN7', name: 'f_ms_lan7', type: 'checkbox', value: ms_lan[7] == 1 },
/* TOMATO64-END */
			{ title: 'Port', name: 'ms_port', type: 'text', maxlen: 5, size: 6, value: nvram.ms_port, suffix: ' <small>range: 0 - 65535; default (random) set 0<\/small>' },
			{ title: 'Database Location', multi: [
				{ name: 'f_loc', type: 'select', options: [['','RAM (Temporary)'],
/* JFFS2-BEGIN */
					['/jffs/dlna','JFFS'],
/* JFFS2-END */
					['*user','Custom Path']], value: loc },
				{ name: 'f_user', type: 'text', maxlen: 256, size: 60, value: nvram.ms_dbdir }
			] },
				{ title: 'Scan Media at Startup*', indent: 2, name: 'f_ms_sas', type: 'checkbox', value: nvram.ms_sas == 1, hidden: 1 },
				{ title: 'Rescan on the next run*', indent: 2, name: 'f_ms_rescan', type: 'checkbox', value: 0 },
				{ title: 'Auto scan media', indent: 2, name: 'f_ms_autoscan', type: 'checkbox', value: nvram.ms_autoscan == 1, suffix: ' <small>10 minutes interval<\/small>' },
			null,
			{ title: 'TiVo Support', name: 'f_ms_tivo', type: 'checkbox', value: nvram.ms_tivo == 1 },
			{ title: 'Strictly adhere to DLNA standards', name: 'f_ms_stdlna', type: 'checkbox', value: nvram.ms_stdlna == 1 },
			{ title: 'Custom configuration', name: 'ms_custom', type: 'textarea', value: nvram.ms_custom }
		]);
	</script>

	<small>* media scan may take considerable time to complete.</small>
</div>

<!-- / / / -->

<div id="notice-msg"></div>

<!-- / / / -->

<div class="section-title">Media Directories</div>
<div class="section">
	<div class="tomato-grid" id="ms-grid"></div>
</div>

<!-- / / / -->

<div class="section-title">Notes</div>
<div class="section">
	<ul>
		<li>To exclude a given subdirectory from the scan, place a file with the name <i>.minidlnaignore</i> in it.</li>
		<li>You can use your own custom config file (/etc/minidlna.alt).</li>
	</ul>
</div>

<!-- / / / -->

<div id="footer">
	<span id="footer-msg"></span>
	<input type="button" value="Save" id="save-button" onclick="save(0)">
	<input type="button" value="Cancel" id="cancel-button" onclick="reloadPage();">
</div>

</td></tr>
</table>
</form>
<script>earlyInit();</script>
</body>
</html>
