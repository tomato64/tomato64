<!DOCTYPE html>
<!--
	Tomato GUI
	Copyright (C) 2006-2010 Jonathan Zarate
	http://www.polarcloud.com/tomato/

	Tinc Web GUI
	Copyright (C) 2014-2022 Lance Fredrickson
	lancethepants@gmail.com

	For use with Tomato Firmware only.
	No part of this file may be used without permission.
-->
<html lang="en-GB">
<head>
<meta http-equiv="content-type" content="text/html;charset=utf-8">
<meta name="robots" content="noindex,nofollow">
<title>[<% ident(); %>] Tinc Mesh VPN</title>
<link rel="stylesheet" type="text/css" href="tomato.css">
<% css(); %>
<script src="isup.jsz"></script>
<script src="tomato.js"></script>

<script>

//	<% nvram("tinc_enable,tinc_name,tinc_devicetype,tinc_mode,tinc_vpn_netmask,tinc_private_rsa,tinc_private_ed25519,tinc_custom,tinc_hosts,tinc_firewall,tinc_manual_firewall,tinc_manual_tinc_up,tinc_poll,tinc_tinc_up,tinc_tinc_down,tinc_host_up,tinc_host_down,tinc_subnet_up,tinc_subnet_down"); %>

var cprefix = 'vpn_tinc';
var tabs = [['config','Config'],['hosts','Hosts'],['scripts','Scripts'],['keys','Generate Keys'],['status','Status']];
var tinc_compression = [['0','0 - None'],['1','1 - Fast zlib'],['2','2'],['3','3'],['4','4'],['5','5'],['6','6'],['7','7'],['8','8'],['9','9 - Best zlib'],['10','10 - Fast lzo'],['11','11 - Best lzo'],['12','12 - lz4']];
var cmd = null;
var cmdresult = '';
var changed = 0;

function show() {
	var d = isup.tincd;

	countButton += 1;
	for (var i = 1; i <= 4; i++) {
		var e = E('_tinc_button'+i);
		e.value = (d ? 'Stop' : 'Start')+' Now';
		e.setAttribute('onclick', 'javascript:toggle(\'tinc\','+d+');');
		if (serviceLastUp[0] != d || countButton > 6) {
			e.disabled = 0;
			E('spin'+i).style.display = 'none';
		}
	}
	if (serviceLastUp[0] != d || countButton > 6) {
		serviceLastUp[0] = d;
		countButton = 0;
	}

	E('_tinc_running').innerHTML = 'Tinc is currently '+(d ? 'running ' : 'stopped');
	E('edges').disabled = !d;
	E('connections').disabled = !d;
	E('subnets').disabled = !d;
	E('nodes').disabled = !d;
	E('info').disabled = !d;
	E('hostselect').disabled = !d;

	var fom = E('t_fom');
	if (d && changed) /* up and config changed? force restart on save */
		fom._service.value = 'tinc-restart';
	else
		fom._service.value = '';

	updateNodes();
}

function toggle(service, isup) {
	var data = th.getAllData();
	var s = '';
	for (var i = 0; i < data.length; ++i)
		s += data[i].join('<')+'>';

	if (nvram.tinc_hosts != s)
		changed = 1;

	if (!save_pre()) return;
	if (changed) alert("Configuration changes were detected - they will be saved");

	serviceLastUp[0] = isup;
	countButton = 0;

	for (var i = 1; i <= 4; i++) {
		E('_'+service+'_button'+i).disabled = 1;
		E('spin'+i).style.display = 'inline';
	}

	elem.display(E('result'), !isup);
	if (!isup)
		elem.setInnerHTML(E('result'), '');

	var fom = E('t_fom');
	fom._service.value = service+(isup ? '-stop' : '-start');

	save(1);
}

var th = new TomatoGrid();

th.resetNewEditor = function() {
	var f = fields.getAll(this.newEditor);
	f[0].checked = 0;
	f[1].value = '';
	f[2].value = '';
	f[3].value = '';
	f[4].selectedIndex = 0;
	f[5].value = '';
	E('_host_rsa_key').value = '';
	E('_host_ed25519_key').value = '';
	E('_host_custom').value = '';
	ferror.clearAll(fields.getAll(this.newEditor));
	ferror.clear(E('_host_ed25519_key'));
}

th.setup = function() {
	this.init('th-grid', '', 50, [
		{ type: 'checkbox', prefix: '<div class="centered">', suffix: '<\/div>' },
		{ type: 'text', maxlen: 30 },
		{ type: 'text', maxlen: 100 },
		{ type: 'text', maxlen: 5 },
		{ type: 'select', options: tinc_compression },
		{ type: 'text', maxlen: 20 },
		{ type: 'textarea', proxy: '_host_rsa_key' },
		{ type: 'textarea', proxy: '_host_ed25519_key' },
		{ type: 'textarea', proxy: '_host_custom' }
	]);
	this.headerSet(['ConnectTo','Name','Address','Port','Compression','Subnet']);
	var nv = nvram.tinc_hosts.split('>');
	for (var i = 0; i < nv.length; ++i) {
		var t = nv[i].split('<');
		if (t.length == 9) {
			t[0] *= 1;
			this.insertData(-1, t);
		}
	}
	th.showNewEditor();
}

th.dataToView = function(data) {
	return [(data[0] != '0') ? 'On' : '', data[1], data[2], data[3], data[4] ,data[5]];
}

th.fieldValuesToData = function(row) {
	var f = fields.getAll(row);

	return [f[0].checked ? 1 : 0, f[1].value, f[2].value, f[3].value, f[4].value, f[5].value, E('_host_rsa_key').value, E('_host_ed25519_key').value, E('_host_custom').value];
}

th.rpDel = function(e) {
	changed = 1;
	e = PR(e);
	TGO(e).moving = null;
	e.parentNode.removeChild(e);
	this.recolor();
	this.resort();
	this.rpHide();
}

th.verifyFields = function(row, quiet) {
	var f = fields.getAll(row);
	changed = 1;
	var ok = 1;

	if (f[1].value == '') {
		ferror.set(f[1], 'Host Name is required', quiet || !ok);
		ok = 0;
	}
	else
		ferror.clear(f[1]);

	if (f[0].checked && f[2].value == '') {
		ferror.set(f[2], 'Address must be supplied when ConnectTo is checked', quiet || !ok);
		ok = 0;
	}
	else
		ferror.clear(f[2]);

	if (!f[3].value == '') {
		if (!v_port(f[3], quiet || !ok))
			ok = 0;
	}

	if (E('_tinc_devicetype').value == 'tun') {
		if ((!v_subnet(f[5], 1)) && (!v_ip(f[5], 1))) {
			ferror.set(f[5], 'Invalid Subnet or IP address', quiet || !ok);
			ok = 0;
		}
		else
			ferror.clear(f[5]);
	}
	else if (E('_tinc_devicetype').value == 'tap') {
		if (f[5].value != '') {
			ferror.set(f[5], 'Subnet must be left blank when using the TAP Interface Type', quiet || !ok);
			ok = 0;
		}
		else
			ferror.clear(f[5]);
	}

	if (E('_host_ed25519_key').value == '') {
		ferror.set(E('_host_ed25519_key'), 'Ed25519 Public Key is required', quiet || !ok);
		ok = 0;
	}
	else
		ferror.clear(E('_host_ed25519_key'));

	return ok;
}

function escapeText(s) {
	function esc(c) {
		return '&#'+c.charCodeAt(0)+';';
	}

	return s.replace(/[&"'<>]/g, esc).replace(/\n/g, ' <br>').replace(/ /g, '&nbsp;');
}

function spin(x, which) {
	E(which).style.display = (x ? 'inline-block' : 'none');
	if (!x)
		cmd = null;
}

/* Borrowed from http://snipplr.com/view/14074/ */
String.prototype.between = function(prefix, suffix) {
	s = this;
	var i = s.indexOf(prefix);
	if (i >= 0)
		s = s.substring(i+prefix.length);
	else
		return '';

	if (suffix) {
		i = s.indexOf(suffix);
		if (i >= 0)
			s = s.substring(0, i);
		else
			return '';
	}

	return s;
}

function displayKeys() {
	E('_rsa_private_key').value = '-----BEGIN RSA PRIVATE KEY-----\n'+cmdresult.between('-----BEGIN RSA PRIVATE KEY-----\n','\n-----END RSA PRIVATE KEY-----')+'\n-----END RSA PRIVATE KEY-----';
	E('_rsa_public_key').value = '-----BEGIN RSA PUBLIC KEY-----\n'+cmdresult.between('-----BEGIN RSA PUBLIC KEY-----\n','\n-----END RSA PUBLIC KEY-----')+'\n-----END RSA PUBLIC KEY-----';
	E('_ed25519_private_key').value = '-----BEGIN ED25519 PRIVATE KEY-----\n'+cmdresult.between('-----BEGIN ED25519 PRIVATE KEY-----\n','\n-----END ED25519 PRIVATE KEY-----')+'\n-----END ED25519 PRIVATE KEY-----';
	E('_ed25519_public_key').value = cmdresult.between('-----END ED25519 PRIVATE KEY-----\n','\n');

	cmdresult = '';
	spin(0, 'generateWait');
	E('execb').disabled = 0;
}

function generateKeys() {
	E('execb').disabled = 1;
	spin(1, 'generateWait');

	E('_rsa_private_key').value = '';
	E('_rsa_public_key').value = '';
	E('_ed25519_private_key').value = '';
	E('_ed25519_public_key').value = '';

	cmd = new XmlHttp();
	cmd.onCompleted = function(text, xml) {
		eval(text);
		displayKeys();
	}
	cmd.onError = function(x) {
		cmdresult = 'ERROR: '+x;
		displayKeys();
	}

	var c = '/bin/rm -rf /etc/keys \n\
		/bin/mkdir /etc/keys \n\
		/bin/echo -e \'\n\n\n\n\' | /usr/sbin/tinc -c /etc/keys generate-keys \n\
		/bin/cat /etc/keys/rsa_key.priv \n\
		/bin/cat /etc/keys/rsa_key.pub \n\
		/bin/cat /etc/keys/ed25519_key.priv \n\
		/bin/cat /etc/keys/ed25519_key.pub';
	cmd.post('shell.cgi', 'action=execute&command='+escapeCGI(c.replace(/\r/g, '')));
}

function displayStatus() {
	elem.setInnerHTML(E('result'), '<tt>'+escapeText(cmdresult)+'<\/tt>');
	cmdresult = '';
	spin(0, 'statusWait');
}

function updateStatus(type) {
	elem.setInnerHTML(E('result'), '');
	spin(1, 'statusWait');

	cmd = new XmlHttp();
	cmd.onCompleted = function(text, xml) {
		eval(text);
		displayStatus();
	}
	cmd.onError = function(x) {
		cmdresult = 'ERROR: '+x;
		displayStatus();
	}

	if (type != 'info')
		var c = '/usr/sbin/tinc dump '+type+'\n';
	else {
		var selects = document.getElementById('hostselect');
		var c = '/usr/sbin/tinc '+type+' '+selects.options[selects.selectedIndex].text+'\n';
	}

	cmd.post('shell.cgi', 'action=execute&command='+escapeCGI(c.replace(/\r/g, '')));
	updateNodes();
}

function displayNodes() {
	var hostselect = document.getElementById('hostselect');
	var selected = hostselect.value;

	while (hostselect.firstChild)
		hostselect.removeChild(hostselect.firstChild);

	var hosts = cmdresult.split('\n');

	for (var i = 0; i < hosts.length; ++i) {
		if (hosts[i] != '') {
			hostselect.options[hostselect.options.length] = new Option(hosts[i],hosts[i]);
			if (hosts[i] == selected)
				hostselect.value = selected;
		}
	}
	cmdresult = '';
}

function updateNodes() {
	if (isup.tincd) {
		cmd = new XmlHttp();
		cmd.onCompleted = function(text, xml) {
			eval(text);
			displayNodes();
		}
		cmd.onError = function(x) {
			cmdresult = 'ERROR: '+x;
			displayNodes();
		}

		var c = '/usr/sbin/tinc dump nodes | /bin/busybox awk \'{print $1}\'';
		cmd.post('shell.cgi', 'action=execute&command='+escapeCGI(c.replace(/\r/g, '')));
	}
}

function displayVersion() {
	elem.setInnerHTML(E('version'), escapeText(cmdresult.substring(0, cmdresult.length - 1)));
	cmdresult = '';
}

function getVersion() {
	cmd = new XmlHttp();
	cmd.onCompleted = function(text, xml) {
		eval(text);
		displayVersion();
	}
	cmd.onError = function(x) {
		cmdresult = 'ERROR: '+x;
		displayVersion();
	}

	var c = '/usr/sbin/tinc --version | /bin/busybox awk \'NR==1 {print $3}\'';
	cmd.post('shell.cgi', 'action=execute&command='+escapeCGI(c.replace(/\r/g, '')));
}

function tabSelect(name) {
	tgHideIcons();
	cookie.set(cprefix+'_tab', name);
	tabHigh(name);

	for (var i = 0; i < tabs.length; ++i)
		elem.display(tabs[i][0]+'-tab', (name == tabs[i][0]));
}

function verifyFields(focused, quiet) {
	if (focused && focused != E('_f_tinc_enable')) /* except on/off */
		changed = 1;

	var ok = 1;

	/* Visibility Changes */
	var vis = {
		_tinc_mode: 1,
		_tinc_vpn_netmask: 1,
	};

	switch (E('_tinc_devicetype').value) {
		case 'tun':
			vis._tinc_mode = 0;
			vis._tinc_vpn_netmask = 1;
		break;
		case 'tap':
			vis._tinc_mode = 1;
			vis._tinc_vpn_netmask = 0;
		break;
	}

	switch (E('_tinc_manual_tinc_up').value) {
		case '0':
			E('_tinc_tinc_up').disabled = 1;
		break;
		case '1':
			E('_tinc_tinc_up').disabled = 0;
		break;
	}

	switch (E('_tinc_manual_firewall').value) {
		case '0':
			E('_tinc_firewall').disabled = 1;
		break;
		default:
			E('_tinc_firewall').disabled = 0;
		break;
	}

	for (a in vis) {
		b = E(a);
		c = vis[a];
		b.disabled = (c != 1);
		PR(b).style.display = c ? '' : 'none';
	}

	/* Element Verification */
	if (E('_tinc_name').value == '' && E('_f_tinc_enable').checked) {
		ferror.set(E('_tinc_name'), 'Host Name is required when \'Enable on Start\' is checked', quiet || !ok);
		ok = 0;
	}
	else
		ferror.clear(E('_tinc_name'));

	if (E('_tinc_private_ed25519').value == '' && E('_tinc_custom').value == '' && E('_f_tinc_enable').checked) {
		ferror.set(E('_tinc_private_ed25519'), 'Ed25519 Private Key is required when \'Enable on Start\' is checked', quiet || !ok);
		ok = 0;
	}
	else
		ferror.clear(E('_tinc_private_ed25519'));

	if (!v_netmask('_tinc_vpn_netmask', quiet || !ok))
		ok = 0;

	if (!v_range('_tinc_poll', quiet || !ok, 0, 1440))
		ok = 0;

	if (!E('_host_ed25519_key').value == '')
		ferror.clear(E('_host_ed25519_key'));

	var hostdefined = 0;
	var hosts = th.getAllData();
	for (var i = 0; i < hosts.length; ++i) {
		if (hosts[i][1] == E('_tinc_name').value) {
			hostdefined = 1;
			break;
		}
	}

	if (!hostdefined && E('_f_tinc_enable').checked) {
		ferror.set(E('_tinc_name'), 'Host Name \''+E('_tinc_name').value+'\' must be defined in the hosts area when \'Enable on Start\' is checked', quiet || !ok);
		ok = 0;
	}
	else
		ferror.clear(E('_tinc_name'));

	return ok;
}

function save_pre() {
	if (th.isEditing())
		return 0;
	if (!verifyFields(null, 0))
		return 0;
	return 1;
}

function save(nomsg) {
	save_pre();
	if (!nomsg) show(); /* update '_service' field first */

	var data = th.getAllData();
	var s = '';
	for (var i = 0; i < data.length; ++i)
		s += data[i].join('<')+'>';

	var fom = E('t_fom');
	fom.tinc_hosts.value = s;
	nvram.tinc_hosts = s;
	fom.tinc_enable.value = fom.f_tinc_enable.checked ? 1 : 0;
	fom._nofootermsg.value = (nomsg ? 1 : 0);

	form.submit(fom, 1);

	changed = 0;
}

function earlyInit() {
	tabSelect(cookie.get(cprefix+'_tab') || 'config');
	getVersion();
	show();
	verifyFields(null, 1);
}

function init() {
	th.recolor();
	th.resetNewEditor();
	var c;
	if (((c = cookie.get(cprefix+'_hosts_vis')) != null) && (c == '1'))
		toggleVisibility(cprefix, 'hosts');

	updateNodes();
	up.initPage(250, 5);
	eventHandler();
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

<input type="hidden" name="_nextpage" value="vpn-tinc.asp">
<input type="hidden" name="_service" value="">
<input type="hidden" name="_nofootermsg">
<input type="hidden" name="tinc_enable">
<input type="hidden" name="tinc_hosts">

<!-- / / / -->

<div class="section-title" id="tinc-title">Tinc <span id="version"></span> Configuration</div>
<script>
	tabCreate.apply(this, tabs);

	W('<div id="tab-area"><\/div>');

	// -------- BEGIN CONFIG TAB -----------
	W('<div id="config-tab">');
	W('<div class="section">');
	createFieldTable('', [
		{ title: 'Enable on Start', name: 'f_tinc_enable', type: 'checkbox', value: (nvram.tinc_enable == 1) },
		{ title: 'Interface Type', name: 'tinc_devicetype', type: 'select', options: [['tun','TUN'],['tap','TAP']], value: nvram.tinc_devicetype },
		{ title: 'Mode', name: 'tinc_mode', type: 'select', options: [['switch','Switch'],['hub','Hub']], value: nvram.tinc_mode },
		{ title: 'VPN Netmask', name: 'tinc_vpn_netmask', type: 'text', maxlen: 15, size: 25, value: nvram.tinc_vpn_netmask,  suffix: ' <small>netmask for the entire VPN network<\/small>' },
		{ title: 'Host Name', name: 'tinc_name', type: 'text', maxlen: 30, size: 25, value: nvram.tinc_name, suffix: ' <small>must also be defined in the \'Hosts\' area<\/small>' },
		{ title: 'Poll Interval', name: 'tinc_poll', type: 'text', maxlen: 4, size: 5, value: nvram.tinc_poll, suffix: ' <small>minutes; 0 to disable<\/small>' },
		{ title: 'Ed25519 Private Key', name: 'tinc_private_ed25519', type: 'textarea', value: nvram.tinc_private_ed25519 },
		{ title: 'RSA Private Key *', name: 'tinc_private_rsa', type: 'textarea', value: nvram.tinc_private_rsa },
		{ title: 'Custom', name: 'tinc_custom', type: 'textarea', value: nvram.tinc_custom }
	]);

	W('<small><b style="font-size: 1.5em">*<\/b> Only required to create legacy connections with tinc1.0 nodes.<\/small>');
	W('<div class="vpn-start-stop"><input type="button" value="" onclick="" id="_tinc_button1">&nbsp; <img src="spin.gif" alt="" id="spin1"><\/div>');
	W('<\/div><\/div>');
	// -------- END CONFIG TAB -----------


	// -------- BEGIN HOSTS TAB -----------
	W('<div id="hosts-tab">');
	W('<div class="section">');
	W('<div class="tomato-grid" id="th-grid"><\/div>');

	th.setup();

	createFieldTable('', [
		{ title: 'Ed25519 Public Key', name: 'host_ed25519_key', type: 'textarea' },
		{ title: 'RSA Public Key *', name: 'host_rsa_key', type: 'textarea' },
		{ title: 'Custom', name: 'host_custom', type: 'textarea' }
	]);

	W('<small><b style="font-size: 1.5em">*<\/b> Only required to create legacy connections with tinc1.0 nodes.<\/small>');
	W('<div class="vpn-start-stop"><input type="button" value="" onclick="" id="_tinc_button2">&nbsp; <img src="spin.gif" alt="" id="spin2"><\/div>');
	W('<\/div>');

	W('<div class="section-title">Notes <small><i><a href="javascript:toggleVisibility(cprefix,\'hosts\');"><span id="sesdiv_hosts_showhide">(Show)<\/span><\/a><\/i><\/small><\/div>');
	W('<div class="section" id="sesdiv_hosts" style="display:none">');
	W('<ul>');
	W('<li><b>ConnectTo<\/b> - Tinc will try to establish a meta-connection to the host. Requires the Address field.');
	W('<li><b>Name<\/b> - Name of the host. There must be an entry for this host.');
	W('<li><b>Address<\/b> <i>(optional)<\/i> - Must resolve to the external IP address where the host can be reached.');
	W('<li><b>Port<\/b> <i>(optional)<\/i> - Port the host listens on. If empty the default value (655) is used.');
	W('<li><b>Compression<\/b> - Level of compression used for UDP packets. Possible values are ');
	W('0 (off), 1 (fast zlib) and any integer up to 9 (best zlib), 10 (fast lzo), 11 (best lzo), and 12 (lz4)');
	W('<li><b>Subnet<\/b> - Subnet which the host will serve.');
	W('<\/ul>');
	W('<\/div><\/div>');
	// ---------- END HOSTS TAB ------------


	// -------- BEGIN SCRIPTS TAB -----------
	W('<div id="scripts-tab">');
	W('<div class="section">');

	createFieldTable('', [
		{ title: 'Firewall Rules', name: 'tinc_manual_firewall', type: 'select', options: [['0','Automatic'],['1','Additional'],['2','Manual']], value: nvram.tinc_manual_firewall },
		{ title: 'Firewall', name: 'tinc_firewall', type: 'textarea', value: nvram.tinc_firewall },
		{ title: 'tinc-up creation', name: 'tinc_manual_tinc_up', type: 'select', options: [['0','Automatic'],['1','Manual']], value: nvram.tinc_manual_tinc_up },
		{ title: 'tinc-up', name: 'tinc_tinc_up', type: 'textarea', value: nvram.tinc_tinc_up },
		{ title: 'tinc-down', name: 'tinc_tinc_down', type: 'textarea', value: nvram.tinc_tinc_down },
		{ title: 'host-up', name: 'tinc_host_up', type: 'textarea', value: nvram.tinc_host_up },
		{ title: 'host-down', name: 'tinc_host_down', type: 'textarea', value: nvram.tinc_host_down },
		{ title: 'subnet-up', name: 'tinc_subnet_up', type: 'textarea', value: nvram.tinc_subnet_up },
		{ title: 'subnet-down', name: 'tinc_subnet_down', type: 'textarea', value: nvram.tinc_subnet_down }
	]);

	W('<div class="vpn-start-stop"><input type="button" value="" onclick="" id="_tinc_button3">&nbsp; <img src="spin.gif" alt="" id="spin3"><\/div>');
	W('<\/div><\/div>');
	// -------- END SCRIPTS TAB -----------


	// -------- BEGIN KEYS TAB -----------
	W('<div id="keys-tab">');
	W('<div class="section">');

	createFieldTable('', [
		{ title: 'Ed25519 Private Key', name: 'ed25519_private_key', type: 'textarea', value: "" },
		{ title: 'Ed25519 Public Key', name: 'ed25519_public_key', type: 'textarea', value: "" },
		{ title: 'RSA Private Key', name: 'rsa_private_key', type: 'textarea', value: "" },
		{ title: 'RSA Public Key', name: 'rsa_public_key', type: 'textarea', value: "" }
	]);

	W('<input type="button" value="Generate Keys" onclick="generateKeys()" id="execb"> ');
	W('<div style="display:none" id="generateWait"> Please wait... <img src="spin.gif" alt="" style="vertical-align:middle"><\/div>');
	W('<\/div><\/div>');
	// -------- END KEYS TAB -----------


	// -------- BEGIN STATUS TAB -----------
	W('<div id="status-tab">');
	W('<div class="fields">');

	W('<div class="section">');
	W('<div class="vpn-start-stop"><span id="_tinc_running"><\/span> <input type="button" value="" onclick="" id="_tinc_button4">&nbsp; <img src="spin.gif" alt="" id="spin4"><\/div>');
	W('<\/div>');

	W('<div class="section">');
	W('<div class="vpn-status-btn"><input type="button" value="Edges" onclick="updateStatus(\'edges\')" id="edges" style="min-width:85px"><\/div>');
	W('<div class="vpn-status-btn"><input type="button" value="Subnets" onclick="updateStatus(\'subnets\')" id="subnets" style="min-width:85px"><\/div>');
	W('<div class="vpn-status-btn"><input type="button" value="Connections" onclick="updateStatus(\'connections\')" id="connections" style="min-width:85px"><\/div>');
	W('<div class="vpn-status-btn"><input type="button" value="Nodes" onclick="updateStatus(\'nodes\')" id="nodes" style="min-width:85px"><\/div>');
	W('<div style="display:none;padding-left:5px" id="statusWait"> Please wait... <img src="spin.gif" alt="" style="vertical-align:top"><\/div>');
	W('<\/div>');

	W('<div class="section">');
	W('<div><input type="button" value="Info" onclick="updateStatus(\'info\')" id="info" style="min-width:85px"> <select id="hostselect" style="min-width:85px"><\/select><\/div>');
	W('<\/div>');

	W('<pre id="result"><\/pre>');

	W('<\/div><\/div>');
	// -------- END KEY TAB -----------
</script>

<!-- / / / -->

<div class="section-title" id="tinc-tutorial">Tutorial</div>
<div class="section">
	<ul>
		<li><a href="https://www.linksysinfo.org/index.php?threads/tinc-mesh-vpn.70257/" class="new_window">At this address</a> you can find a good one.</li>
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
