<!DOCTYPE html>
<!--
	Tomato PPTPd GUI
	Copyright (C) 2012 Augusto Bott
	http://code.google.com/p/tomato-sdhc-vlan/

	Tomato GUI
	Copyright (C) 2006-2007 Jonathan Zarate
	http://www.polarcloud.com/tomato/
	For use with Tomato Firmware only.
	No part of this file may be used without permission.
-->
<html lang="en-GB">
<head>
<meta http-equiv="content-type" content="text/html;charset=utf-8">
<meta name="robots" content="noindex,nofollow">
<title>[<% ident(); %>] PPTP: Server</title>
<link rel="stylesheet" type="text/css" href="tomato.css">
<% css(); %>
<script src="isup.jsz"></script>
<script src="isup.js"></script>
<script src="tomato.js"></script>
<script src="interfaces.js"></script>

<script>

//	<% nvram("lan_ipaddr,lan_netmask,pptpd_enable,pptpd_remoteip,pptpd_chap,pptpd_forcemppe,pptpd_broadcast,pptpd_users,pptpd_dns1,pptpd_dns2,pptpd_wins1,pptpd_wins2,pptpd_mtu,pptpd_mru,pptpd_custom");%>

var cprefix = 'vpn_pptpd';
var changed = 0;
var serviceType = 'pptpd';

if (nvram.pptpd_remoteip == '')
	nvram.pptpd_remoteip = '172.19.0.1-6';
if (nvram.pptpd_forcemppe == '')
	nvram.pptpd_forcemppe = '1';

function v_pptpd_secret(e, quiet) {
	var s;
	var ok = 1;
	if ((e = E(e)) == null)
		return 0;

	s = e.value.trim().replace(/\s+/g, '');
	if (s.length < 1) {
		ferror.set(e, 'Username and password can not be empty', quiet || !ok);
		ok = 0;
	}
	if (s.length > 32) {
		ferror.set(e, 'Invalid entry: max 32 characters are allowed', quiet || !ok);
		ok = 0;
	}
	if (s.search(/^[.a-zA-Z0-9_\- ]+$/) == -1) {
		ferror.set(e, 'Invalid entry. Only characters "A-Z 0-9 . - _" are allowed', quiet || !ok);
		ok = 0;
	}
	e.value = s;
	ferror.clear(e);

	return ok;
}

function submit_complete() { /* to set pptpd_dns1 etc back to 0.0.0.0 after save, if it's empty in nvram */
	verifyFields(null, 1);
}

var ul = new TomatoGrid();

ul.resetNewEditor = function() {
	var f = fields.getAll(this.newEditor);
	ferror.clearAll(f);

	f[0].value = '';
	f[1].value = '';
}

ul.setup = function() {
	this.init('ul-grid', 'sort', 6, [
		{ type: 'text', maxlen: 32, size: 32 },
		{ type: 'password', peekaboo: 1, maxlen: 32, size: 32 }
	]);
	this.headerSet(['Username','Password']);

	var r = nvram.pptpd_users.split('>');
	for (var i = 0; i < r.length; ++i) {
		var l = r[i].split('<');
		if (l.length == 2)
			ul.insertData(-1, l);
	}

	ul.showNewEditor();
	ul.resetNewEditor();
}

ul.exist = function(f, v) {
	var data = this.getAllData();
	for (var i = 0; i < data.length; ++i) {
		if (data[i][f] == v)
			return true;
	}

	return false;
}

ul.existName = function(user) {
	return this.exist(0, user);
}

ul.rpDel = function(e) {
	changed = 1;
	e = PR(e);
	TGO(e).moving = null;
	e.parentNode.removeChild(e);
	this.recolor();
	this.resort();
	this.rpHide();
}

ul.verifyFields = function(row, quiet) {
	changed = 1;
	var ok = 1;
	var f, s;
	f = fields.getAll(row);

	if (!v_pptpd_secret(f[0], quiet || !ok))
		ok = 0;

	if (this.existName(f[0].value)) {
		ferror.set(f[0], 'Duplicate User', quiet || !ok);
		ok = 0;
	}

	if (!v_pptpd_secret(f[1], quiet || !ok))
		ok = 0;

	return ok;
}

function verifyFields(focused, quiet) {
	if (focused && focused != E('_f_pptpd_enable')) /* except on/off */
		changed = 1;

	var ok = 1;

	var a = E('_f_pptpd_startip');
	var b = E('_f_pptpd_endip');
	if (Math.abs((aton(a.value) - (aton(b.value)))) > 5) {
		ferror.set(a, 'Invalid range (max 6 IPs)', quiet);
		ferror.set(b, 'Invalid range (max 6 IPs)', quiet || !ok);
		elem.setInnerHTML('pptpd_count', '(?)');
		ok = 0;
	}
	else {
		ferror.clear(a);
		ferror.clear(b);
	}

	if (aton(a.value) > aton(b.value)) {
		var d = a.value;
		a.value = b.value;
		b.value = d;
	}

	elem.setInnerHTML('pptpd_count', '('+((aton(b.value) - aton(a.value)) + 1)+')');

	if (!v_ipz('_pptpd_dns1', quiet || !ok))
		ok = 0;
	if (!v_ipz('_pptpd_dns2', quiet || !ok))
		ok = 0;
	if (!v_ipz('_pptpd_wins1', quiet || !ok))
		ok = 0;
	if (!v_ipz('_pptpd_wins2', quiet || !ok))
		ok = 0;
	if (!v_range('_pptpd_mtu', quiet || !ok, 576, 1500))
		ok = 0;
	if (!v_range('_pptpd_mru', quiet || !ok, 576, 1500))
		ok = 0;
	if (!v_ip('_f_pptpd_startip', quiet || !ok))
		ok = 0;
	if (!v_ip('_f_pptpd_endip', quiet || !ok))
		ok = 0;

	return ok;
}

function save_pre() {
	if (ul.isEditing())
		return 0;
	if (!verifyFields(null, 0))
		return 0;
	if (ul.getDataCount() < 1) {
		alert('Cannot proceed: at least one user must be defined');
		return 0;
	}
	return 1;
}

function save(nomsg) {
	save_pre();
	if (!nomsg) show(); /* update '_service' field first */

	ul.resetNewEditor();
	var uldata = ul.getAllData();

	var s = '';
	for (var i = 0; i < uldata.length; ++i)
		s += uldata[i].join('<')+'>';

	var fom = E('t_fom');
	fom.pptpd_users.value = s;
	fom.pptpd_enable.value = fom._f_pptpd_enable.checked ? 1 : 0;
	fom._nofootermsg.value = (nomsg ? 1 : 0);

	var a = fom._f_pptpd_startip.value;
	var b = fom._f_pptpd_endip.value;
	if ((fixIP(a) != null) && (fixIP(b) != null)) {
		var c = b.split('.').splice(3, 1);
		fom.pptpd_remoteip.value = a+'-'+c;
	}

	if (fom.pptpd_dns1.value == '0.0.0.0')
		fom.pptpd_dns1.value = '';
	if (fom.pptpd_dns2.value == '0.0.0.0')
		fom.pptpd_dns2.value = '';
	if (fom.pptpd_wins1.value == '0.0.0.0')
		fom.pptpd_wins1.value = '';
	if (fom.pptpd_wins2.value == '0.0.0.0')
		fom.pptpd_wins2.value = '';

	form.submit(fom, 1);

	changed = 0;
}

function earlyInit() {
	show();

	if (nvram.pptpd_remoteip.indexOf('-') != -1) {
		var tmp = nvram.pptpd_remoteip.split('-');
		E('_f_pptpd_startip').value = tmp[0];
		E('_f_pptpd_endip').value = tmp[0].split('.').splice(0,3).join('.')+'.'+tmp[1];
	}

	ul.setup();
	verifyFields(null, 1);
}

function init() {
	var c;
	if (((c = cookie.get(cprefix+'_notes_vis')) != null) && (c == '1'))
		toggleVisibility(cprefix, 'notes');

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

<input type="hidden" name="_nextpage" value="vpn-pptpd.asp">
<input type="hidden" name="_service" value="">
<input type="hidden" name="_nofootermsg">
<input type="hidden" name="pptpd_users">
<input type="hidden" name="pptpd_enable">
<input type="hidden" name="pptpd_remoteip">

<!-- / / / -->

<div class="section-title">PPTP Server Configuration</div>
<div class="section">
	<script>
		createFieldTable('', [
			{ title: 'Enable on Start', name: 'f_pptpd_enable', type: 'checkbox', value: nvram.pptpd_enable == '1' },
			{ title: 'Local IP Address/Netmask', text: (nvram.lan_ipaddr+' / '+nvram.lan_netmask) },
			{ title: 'Remote IP Address Range', multi: [
				{ name: 'f_pptpd_startip', type: 'text', maxlen: 15, size: 17, value: nvram.dhcpd_startip, suffix: '&nbsp;-&nbsp;' },
				{ name: 'f_pptpd_endip', type: 'text', maxlen: 15, size: 17, value: nvram.dhcpd_endip, suffix: ' <i id="pptpd_count"><\/i>' }
			] },
			{ title: 'Broadcast Relay Mode', name: 'pptpd_broadcast', type: 'select', options: [['disable','Disabled'],['br0','LAN to VPN Clients'],['ppp','VPN Clients to LAN'],['br0ppp','Both']], value: nvram.pptpd_broadcast },
			{ title: 'Authentication', name: 'pptpd_chap', type: 'select', options: [[0,'Auto'],[1,'MS-CHAPv1'],[2,'MS-CHAPv2']], value: nvram.pptpd_chap },
			{ title: 'Encryption', name: 'pptpd_forcemppe', type: 'select', options: [[0,'None'],[1,'MPPE-128']], value: nvram.pptpd_forcemppe },
			{ title: 'DNS Servers', name: 'pptpd_dns1', type: 'text', maxlen: 15, size: 17, value: nvram.pptpd_dns1 },
			{ title: '', name: 'pptpd_dns2', type: 'text', maxlen: 15, size: 17, value: nvram.pptpd_dns2 },
			{ title: 'WINS Servers', name: 'pptpd_wins1', type: 'text', maxlen: 15, size: 17, value: nvram.pptpd_wins1 },
			{ title: '', name: 'pptpd_wins2', type: 'text', maxlen: 15, size: 17, value: nvram.pptpd_wins2 },
			{ title: 'MTU', name: 'pptpd_mtu', type: 'text', maxlen: 4, size: 6, value: (nvram.pptpd_mtu ? nvram.pptpd_mtu : 1400) },
			{ title: 'MRU', name: 'pptpd_mru', type: 'text', maxlen: 4, size: 6, value: (nvram.pptpd_mru ? nvram.pptpd_mru : 1400) },
			{ title: '<a href="http://poptop.sourceforge.net/" class="new_window">Poptop<\/a><br>Custom configuration', name: 'pptpd_custom', type: 'textarea', value: nvram.pptpd_custom }
		]);
	</script>
	<div class="vpn-start-stop"><input type="button" value="" onclick="" id="_pptpd_button">&nbsp; <img src="spin.gif" alt="" id="spin"></div>
</div>

<!-- / / / -->

<div class="section-title">PPTP User List</div>
<div class="section">
	<div class="tomato-grid" id="ul-grid"></div>

	<div id="pptp-ctrl">
		&raquo; <a href="vpn-pptp-online.asp">PPTP Online</a>
	</div>
</div>

<!-- / / / -->

<div class="section-title">Notes <small><i><a href='javascript:toggleVisibility(cprefix,"notes");'><span id="sesdiv_notes_showhide">(Show)</span></a></i></small></div>
<div class="section" id="sesdiv_notes" style="display:none">
	<ul>
		<li><b>Local IP Address/Netmask</b> - Address to be used at the local end of the tunnelled PPP links between the server and the VPN clients.</li>
		<li><b>Remote IP Address Range</b> - Remote IP addresses to be used on the tunnelled PPP links (max 6).</li>
		<li><b>Broadcast Relay Mode</b> - Turns on broadcast relay between VPN clients and LAN interface.</li>
		<li><b>Enable Encryption</b> - Enabling this option will turn on VPN channel encryption, but it might lead to reduced channel bandwidth.</li>
		<li><b>DNS Servers</b> - Allows defining DNS servers manually (if none are set, the router/local IP address should be used by VPN clients).</li>
		<li><b>WINS Servers</b> - Allows configuring extra WINS servers for VPN clients, in addition to the WINS server defined on <a href="advanced-dhcpdns.asp">Advanced: DHCP / DNS</a>.</li>
		<li><b>MTU</b> - Maximum Transmission Unit. Max packet size the PPTP interface will be able to send without packet fragmentation.</li>
		<li><b>MRU</b> - Maximum Receive Unit. Max packet size the PPTP interface will be able to receive without packet fragmentation.</li>
	</ul>
	<br>
	<ul>
		<li><b>Other relevant notes/hints:</b></li>
		<li style="list-style:none">
			<ul>
				<li>Try to avoid any conflicts and/or overlaps between the address ranges configured/available for DHCP and VPN clients on your local networks.</li>
				<li>You can add your own ip-up/ip-down scripts which are executed after those built by GUI - relevant NVRAM variables are "pptpd_ipup_script" / "pptpd_ipdown_script".</li>
			</ul>
		</li>
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
