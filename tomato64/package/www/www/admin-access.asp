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
<title>[<% ident(); %>] Admin: Access</title>
<link rel="stylesheet" type="text/css" href="tomato.css">
<link rel="stylesheet" type="text/css" href="<% nv('web_css'); %>.css" id="guicss">
<script src="isup.jsz"></script>
<script src="tomato.js"></script>

<script>

//	<% nvram("http_enable,https_enable,http_lanport,https_lanport,remote_management,remote_mgt_https,remote_upgrade,web_wl_filter,web_css,web_adv_scripts,web_dir,ttb_css,ttb_loc,ttb_url,sshd_eas,sshd_pass,sshd_remote,telnetd_eas,http_wanport,http_wanport_bfm,sshd_authkeys,sshd_port,sshd_rport,sshd_forwarding,telnetd_port,rmgt_sip,https_crt_cn,https_crt_save,lan_ipaddr,ne_shlimit,sshd_motd,http_username,jffs2_auto_unmount"); %>

var cprefix = 'admin_access';
var changed = 0;
var serviceLastUp2 = [];
var countButton2 = 0;
var shlimit = nvram.ne_shlimit.split(',');
if (shlimit.length != 3)
	shlimit = [0,3,60];

var xmenus = [['Status', 'status'],['Bandwidth', 'bwm'],['IP Traffic', 'ipt'],['Tools', 'tools'],['Basic', 'basic'],['Advanced', 'advanced'],['Port Forwarding', 'forward'],['QoS', 'qos'],
/* USB-BEGIN */
              ['USB and NAS', 'nas'],
/* USB-END */
/* VPN-BEGIN */
              ['VPN Tunneling', 'vpn'],
/* VPN-END */
              ['Administration', 'admin']];

function show() {
	var e = E('_sshd_button');
	e.value = (isup.dropbear ? 'Stop' : 'Start')+' Now';
	e.setAttribute('onclick', 'javascript:toggle(\'sshd\','+isup.dropbear+');');
	countButton += 1;
	if (serviceLastUp[0] != isup.dropbear || countButton > 6) {
		serviceLastUp[0] = isup.dropbear;
		countButton = 0;
		e.disabled = 0;
		E('spin').style.display = 'none';
	}

	e = E('_telnetd_button');
	e.value = ((isup.telnetd) ? 'Stop' : 'Start')+' Now';
	e.setAttribute('onclick', 'javascript:toggle(\'telnetd\','+(isup.telnetd)+');');
	countButton2 += 1;
	if (serviceLastUp2[0] != isup.telnetd || countButton2 > 6) {
		serviceLastUp2[0] = isup.telnetd;
		countButton2 = 0;
		e.disabled = 0;
		E('spin2').style.display = 'none';
	}
}

function toggle(service, isup) {
	if (changed && !confirm("There are unsaved changes. Continue anyway?"))
		return;

	E('_'+service+'_button').disabled = 1;
	if (service == 'telnetd') {
		E('spin2').style.display = 'inline';
		serviceLastUp2[0] = isup;
		countButton2 = 0;
	}
	else {
		E('spin').style.display = 'inline';
		serviceLastUp2[0] = isup;
		countButton2 = 0;
	}

	var fom = E('t_fom');
	fom._service.value = 'firewall-restart,'+service+(isup ? '-stop' : '-start');
	fom._nofootermsg.value = 1;
	fom._nextwait.value = 1;

	form.submit(fom, 1, 'service.cgi');
}

function verifyFields(focused, quiet) {
	var ok = 1;
	var a, b, c;
	var i;

	var o = (E('_web_css').value == 'online');
	var p = nvram.ttb_css;
	elem.display(PR('_ttb_css'), o);
/* USB-BEGIN */
	var q = nvram.ttb_loc;
	elem.display(PR('_ttb_loc'), o);
	var r = nvram.ttb_url;
	elem.display(PR('_ttb_url'), o);
/* USB-END */

	try {
		a = E('_web_css').value;
		if (a == 'online') {
			E('guicss').href = 'ext/'+p+'.css';
			nvram.web_css = a;
/* USB-BEGIN */
			nvram.ttb_loc = q;
			nvram.ttb_url = r;
/* USB-END */
		}
		else {
			if (a != nvram.web_css) {
				E('guicss').href = a+'.css';
				nvram.web_css = a;
			}
		}
	}
	catch (ex) {
	}

	var s = (E('_web_css').value.match(/at-/g))
	elem.display(PR('_f_web_adv_scripts'), s);
	/* Warn if modern browser is required */
	elem.display(E('web_css_warn'), s);

	a = E('_f_http_local');
	b = E('_f_http_remote').value;
	if ((a.value != 3) && (b != 0) && (a.value != b)) {
		ferror.set(a, 'The local http/https must also be enabled when using remote access', quiet || !ok);
		ok = 0;
	}
	else
		ferror.clear(a);

	elem.display(PR('_http_lanport'), (a.value == 1) || (a.value == 3));
	elem.display(PR('_f_http_wireless'), a.value != 0);

	c = (a.value == 2) || (a.value == 3);
/* HTTPS-BEGIN */
	elem.display(PR('_https_lanport'), 'row_sslcert', PR('_https_crt_cn'), PR('_f_https_crt_save'), PR('_f_https_crt_gen'), c);

	if (c) {
		a = E('_https_crt_cn');
		a.value = a.value.replace(/(,+|\s+)/g, ' ').trim();
		if (a.value != nvram.https_crt_cn) E('_f_https_crt_gen').checked = 1;
	}
/* HTTPS-END */

	if ((!v_port('_http_lanport', quiet || !ok))
/* HTTPS-BEGIN */
	    || (!v_port('_https_lanport', quiet || !ok))
/* HTTPS-END */
	    ) ok = 0;

	b = b != 0;
	a = E('_http_wanport');
	elem.display(PR(a), b);
	if ((b) && (!v_port(a, quiet || !ok))) ok = 0;

	a = E('_f_http_wanport_bfm');
	elem.display(PR(a), b);

	if (!v_port('_telnetd_port', quiet || !ok)) ok = 0;

	a = E('_f_sshd_remote').checked;
	b = E('_sshd_rport');
	elem.display(PR(b), a);
	if ((a) && (!v_port(b, quiet || !ok))) ok = 0;

	a = E('_sshd_authkeys');
	if (!v_length(a, quiet || !ok, 0, 4096))
		ok = 0;
	else if (a.value != '') {
		var aa = a.value.split(/\r?\n/);
		for (i = 0; i < aa.length; i++) {
			if (aa[i].search(/(ssh-(dss|rsa|ed25519)|ecdsa-sha2)/) == -1) {
				ferror.set(a, 'Invalid SSH key(s). Also check for empty lines', quiet || !ok);
				ok = 0;
			}
		}
	}

	a = E('_f_rmgt_sip');
	if ((a.value.length) && (!_v_iptaddr(a, quiet || !ok, 15, 1, 1)))
		return 0;

	ferror.clear(a);

	if (!v_range('_f_limit_hit', quiet || !ok, 1, 19))
		return 0;
	if (!v_range('_f_limit_sec', quiet || !ok, 3, 3600))
		return 0;

	a = E('_set_password_1');
	b = E('_set_password_2');
	a.value = a.value.trim();
	b.value = b.value.trim();
	if (a.value != b.value) {
		ferror.set(b, 'Both passwords must match', quiet || !ok);
		ok = 0;
	}
	else if (a.value == '') {
		ferror.set(a, 'Password must not be empty', quiet || !ok);
		ok = 0;
	}
	else {
		ferror.clear(a);
		ferror.clear(b);
	}

	changed |= ok;

	return ok;
}

function save() {
	var a, b, fom;

	if (!verifyFields(null, 0))
		return;

	fom = E('t_fom');
	a = E('_f_http_local').value * 1;
	if (a == 0) {
		if (!confirm('Warning: Web Admin is about to be disabled. If you decide to re-enable Web Admin at a later time, it must be done manually via Telnet, SSH or by performing a hardware reset. Are you sure you want to do this?'))
			return;

		fom._nextpage.value = 'about:blank';
	}
	fom.http_enable.value = (a & 1) ? 1 : 0;
/* HTTPS-BEGIN */
	fom.https_enable.value = (a & 2) ? 1 : 0;
/* HTTPS-END */

	nvram.lan_ipaddr = location.hostname;
	if ((a != 0) && (location.hostname == nvram.lan_ipaddr)) {
		if (location.protocol == 'https:') {
			b = 's';
			if ((a & 2) == 0)
				b = '';
		}
		else {
			b = '';
			if ((a & 1) == 0)
				b = 's';
		}

		a = 'http'+b+'://'+location.hostname;
		if (b == 's') {
			if (fom.https_lanport.value != 443)
				a += ':'+fom.https_lanport.value;
		}
		else {
			if (fom.http_lanport.value != 80)
				a += ':'+fom.http_lanport.value;
		}
		fom._nextpage.value = a+'/admin-access.asp';
	}

	a = fom._f_http_remote.value;
	fom.remote_management.value = (a != 0) ? 1 : 0;
/* HTTPS-BEGIN */
	fom.remote_mgt_https.value = (a == 2) ? 1 : 0;
/* HTTPS-END */
/*
	if ((a != 0) && (location.hostname != nvram.lan_ipaddr)) {
		if (location.protocol == 'https:') {
			if (a != 2) fom._nextpage.value = 'http://'+location.hostname+':'+fom.http_wanport.value+'/admin-access.asp';
		} else {
			if (a == 2) fom._nextpage.value = 'https://'+location.hostname+':'+fom.http_wanport.value+'/admin-access.asp';
		}
	}
*/
/* HTTPS-BEGIN */
	fom.https_crt_gen.value = fom._f_https_crt_gen.checked ? 1 : 0;
	fom.https_crt_save.value = fom._f_https_crt_save.checked ? 1 : 0;
/* HTTPS-END */

	fom.web_wl_filter.value = fom._f_http_wireless.checked ? 0 : 1;
	fom.remote_upgrade.value = fom._f_remote_upgrade.checked ? 1 : 0;
	fom.http_wanport_bfm.value = fom._f_http_wanport_bfm.checked ? 1 : 0;

	a = (fom._web_css.value.match(/at-/g));
	fom.web_adv_scripts.value = (fom._f_web_adv_scripts.checked && a) ? 1 : 0;

	fom.telnetd_eas.value = fom._f_telnetd_eas.checked ? 1 : 0;

	fom.sshd_eas.value = fom._f_sshd_eas.checked ? 1 : 0;
	fom.sshd_pass.value = fom._f_sshd_pass.checked ? 1 : 0;
	fom.sshd_remote.value = fom._f_sshd_remote.checked ? 1 : 0;
	fom.sshd_motd.value = fom._f_sshd_motd.checked ? 1 : 0;
	fom.sshd_forwarding.value = fom._f_sshd_forwarding.checked ? 1 : 0;
/* JFFS2-BEGIN */
	fom.jffs2_auto_unmount.value = fom._f_jffs2_auto_unmount.checked ? 1 : 0;
/* JFFS2-END */

	/* do not restart sshd if no changes in its configuration */
	if ((fom.sshd_pass.value == nvram.sshd_pass) && (fom.sshd_remote.value == nvram.sshd_remote) && (fom.sshd_motd.value == nvram.sshd_motd) &&
	    (fom.sshd_forwarding.value == nvram.sshd_forwarding) &&
	    (fom._set_password_1.value == "**********") && (fom._sshd_rport.value == nvram.sshd_rport) && (fom._sshd_port.value == nvram.sshd_port) && (fom._sshd_authkeys.value == nvram.sshd_authkeys)) {
		fom._service.value = 'adminnosshd-restart';
	}
	else
		fom._service.value = 'admin-restart';

	fom.rmgt_sip.value = fom.f_rmgt_sip.value.split(/\s*,\s*/).join(',');

	fom.ne_shlimit.value = ((fom._f_limit_ssh.checked ? 1 : 0) | (fom._f_limit_telnet.checked ? 2 : 0))+','+fom._f_limit_hit.value+','+fom._f_limit_sec.value;

	a = [];
	for (var i = 0; i < xmenus.length; ++i) {
		b = xmenus[i][1];
		if (E('_f_mx_'+b).checked)
			a.push(b);
	}
	fom.web_mx.value = a.join(',');
	fom._nofootermsg.value = 0;
	fom._nextwait.value = 15;

	localStorage.clear();

	form.submit(fom, 0);
}

function earlyInit() {
	show();
	verifyFields(null, 1);
}

function init() {
	var c;
	if (((c = cookie.get(cprefix+'_notes_vis')) != null) && (c == '1'))
		toggleVisibility(cprefix, 'notes');

	changed = 0;
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

<input type="hidden" name="_nextpage" value="admin-access.asp">
<input type="hidden" name="_nextwait" value="">
<input type="hidden" name="_service" value="">
<input type="hidden" name="_nofootermsg" value="">
<input type="hidden" name="http_enable">
<!-- HTTPS-BEGIN -->
<input type="hidden" name="https_enable">
<input type="hidden" name="https_crt_save">
<input type="hidden" name="https_crt_gen">
<input type="hidden" name="remote_mgt_https">
<!-- HTTPS-END -->
<input type="hidden" name="remote_management">
<input type="hidden" name="web_wl_filter">
<input type="hidden" name="remote_upgrade">
<input type="hidden" name="http_wanport_bfm">
<input type="hidden" name="web_adv_scripts">
<input type="hidden" name="telnetd_eas">
<input type="hidden" name="sshd_eas">
<input type="hidden" name="sshd_pass">
<input type="hidden" name="sshd_remote">
<input type="hidden" name="sshd_motd">
<input type="hidden" name="ne_shlimit">
<input type="hidden" name="rmgt_sip">
<input type="hidden" name="sshd_forwarding">
<input type="hidden" name="web_mx">
<!-- JFFS2-BEGIN -->
<input type="hidden" name="jffs2_auto_unmount">
<!-- JFFS2-END -->

<!-- / / / -->

<div class="section-title">Web Admin</div>
<div class="section">
	<script>
		var m = [
			{ title: 'Local Access', name: 'f_http_local', type: 'select', options: [[0,'Disabled'],[1,'HTTP']
/* HTTPS-BEGIN */
			          ,[2,'HTTPS'],[3,'HTTP &amp; HTTPS']
/* HTTPS-END */
			          ],
				value:
/* HTTPS-BEGIN */
				      ((nvram.https_enable != 0) ? 2 : 0) |
/* HTTPS-END */
				       ((nvram.http_enable != 0) ? 1 : 0) },
				{ title: 'HTTP Port', indent: 2, name: 'http_lanport', type: 'text', maxlen: 5, size: 7, value: fixPort(nvram.http_lanport, 80) },
/* HTTPS-BEGIN */
				{ title: 'HTTPS Port', indent: 2, name: 'https_lanport', type: 'text', maxlen: 5, size: 7, value: fixPort(nvram.https_lanport, 443) },
/* HTTPS-END */
				{ title: 'Allow Wireless Access', indent: 2, name: 'f_http_wireless', type: 'checkbox', value: nvram.web_wl_filter == 0 },
			null,
/* JFFS2-BEGIN */
			{ title: 'Unmount JFFS during upgrade', name: 'f_jffs2_auto_unmount', type: 'checkbox', suffix: '&nbsp;<small>Warning! In some peculiar cases the content of JFFS might not be preserved after a firmware upgrade<\/small>', value: nvram.jffs2_auto_unmount == 1 },
/* JFFS2-END */
			{ title: 'Allow Remote Upgrade', name: 'f_remote_upgrade', type: 'checkbox', suffix: '&nbsp;<small>keep disabled for smaller memory footprint during upgrade<\/small>', value: nvram.remote_upgrade == 1 },
			{ title: 'Remote Access', name: 'f_http_remote', type: 'select', options: [[0,'Disabled'],[1,'HTTP']
/* HTTPS-BEGIN */
			          ,[2,'HTTPS']
/* HTTPS-END */
			          ],
				value: (nvram.remote_management == 1) ? (
/* HTTPS-BEGIN */
				        (nvram.remote_mgt_https == 1) ? 2 :
/* HTTPS-END */
				        1) : 0 },
				{ title: 'Port', indent: 2, name: 'http_wanport', type: 'text', maxlen: 5, size: 7, value:  fixPort(nvram.http_wanport, 8080) },
				{ title: 'Port Protection', indent: 2, name: 'f_http_wanport_bfm', type: 'checkbox', suffix: '&nbsp;<small>(enable brute force mitigation rule)<\/small>', value: nvram.http_wanport_bfm == 1 },
/* HTTPS-BEGIN */
			null,
			{ title: 'SSL Certificate', rid: 'row_sslcert' },
				{ title: 'Common Name (CN)', indent: 2, name: 'https_crt_cn', type: 'text', maxlen: 64, size: 40, suffix: '&nbsp;<small>(optional; space separated)<\/small>', value: nvram.https_crt_cn },
				{ title: 'Regenerate', indent: 2, name: 'f_https_crt_gen', type: 'checkbox', value: 0 },
				{ title: 'Save In NVRAM', indent: 2, name: 'f_https_crt_save', type: 'checkbox', value: nvram.https_crt_save == 1 },
/* HTTPS-END */
			null,
			{ title: 'UI files path', name: 'web_dir', type: 'select',
				options: [['default','Default: /www'], ['jffs', 'Custom: /jffs/www (Experts Only!)'], ['opt', 'Custom: /opt/www (Experts Only!)'], ['tmp', 'Custom: /tmp/www (Experts Only!)']], suffix: '<br>&nbsp;<small>Please be sure of your decision before change this settings!<\/small>', value: nvram.web_dir },
			{ title: 'Theme UI', name: 'web_css', type: 'select',
				options: [['default','Default'],['usbred','USB Red'],['red','Tomato'],['black','Black'],['blue','Blue'],['bluegreen','Blue &amp; Green (Lighter)'],['bluegreen2','Blue &amp; Green (Darker)'],
					  ['brown','Brown'],['cyan','Cyan'],['olive','Olive'],['pumpkin','Pumpkin'],['asus','Asus RT-N16'],['rtn66u','Asus RT-N66U'],['asusred','Asus Red'],['linksysred','Linksys Red'],
					  ['at-dark','Advanced Dark'],['at-red','Advanced Red'],['at-blue','Advanced Blue'],['at-green','Advanced Green'],
					  ['ext/custom','Custom (ext/custom.css)'], ['online', 'Online from TTB (TomatoThemeBase)']], suffix: '&nbsp;<small id="web_css_warn">(requires a modern browser)<\/small>', value: nvram.web_css },
				{ title: 'Dynamic BW/IPT charts', indent: 2, name: 'f_web_adv_scripts', type: 'checkbox', suffix: '&nbsp;<small>(JS based, supported only by modern browsers)<\/small>', value: nvram.web_adv_scripts == 1 },
				{ title: 'TTB theme name', indent: 2, name: 'ttb_css', type: 'text', maxlen: 25, size: 35, suffix: '&nbsp;<small>TTB theme <a href="https://freshtomato.org/tomatothemebase/wp-content/uploads/themes.txt" class="new_window">list<\/a> and full <a href="https://freshtomato.org/tomatothemebase/" class="new_window">gallery<\/a><\/small>', value: nvram.ttb_css },
/* USB-BEGIN */
				{ title: 'TTB save folder', indent: 2, name: 'ttb_loc', type: 'text', maxlen: 35, size: 35, suffix: '&nbsp;/TomatoThemeBase <small>(optional)<\/small>', value: nvram.ttb_loc },
				{ title: 'TTB URL', indent: 2, name: 'ttb_url', type: 'text', maxlen: 128, size: 70, suffix: '&nbsp;<small>(space separated)<\/small>', value: nvram.ttb_url },
/* USB-END */
			null,
			{ title: 'Open Menus' }
		];

		var webmx = get_config('web_mx', '').toLowerCase();
		for (var i = 0; i < xmenus.length; ++i)
			m.push({ title: xmenus[i][0], indent: 2, name: 'f_mx_'+xmenus[i][1], type: 'checkbox', value: (webmx.indexOf(xmenus[i][1]) != -1) });

		createFieldTable('', m);
	</script>
</div>

<!-- / / / -->

<div class="section-title">SSH Daemon</div>
<div class="section">
	<script>
		createFieldTable('', [
			{ title: 'Enable at Startup', name: 'f_sshd_eas', type: 'checkbox', value: nvram.sshd_eas == 1 },
			{ title: 'Extended MOTD', name: 'f_sshd_motd', type: 'checkbox', value: nvram.sshd_motd == 1 },
			{ title: 'Remote Access', name: 'f_sshd_remote', type: 'checkbox', value: nvram.sshd_remote == 1 },
				{ title: 'Remote Port', indent: 2, name: 'sshd_rport', type: 'text', maxlen: 5, size: 7, value: nvram.sshd_rport },
			{ title: 'Remote Forwarding', name: 'f_sshd_forwarding', type: 'checkbox', value: nvram.sshd_forwarding == 1 },
			{ title: 'Port', name: 'sshd_port', type: 'text', maxlen: 5, size: 7, value: nvram.sshd_port },
			{ title: 'Allow Password Login', name: 'f_sshd_pass', type: 'checkbox', value: nvram.sshd_pass == 1 },
			{ title: 'Authorized Keys', name: 'sshd_authkeys', type: 'textarea', value: nvram.sshd_authkeys }
		]);
	</script>
	<input type="button" value="" onclick="" id="_sshd_button">&nbsp; <img src="spin.gif" alt="" id="spin"></div>
</div>

<!-- / / / -->

<div class="section-title">Telnet Daemon</div>
<div class="section">
	<script>
		createFieldTable('', [
			{ title: 'Enable at Startup', name: 'f_telnetd_eas', type: 'checkbox', value: nvram.telnetd_eas == 1 },
			{ title: 'Port', name: 'telnetd_port', type: 'text', maxlen: 5, size: 7, value: nvram.telnetd_port }
		]);
	</script>
	<input type="button" value="" onclick="" id="_telnetd_button">&nbsp; <img src="spin.gif" alt="" id="spin2"></div>
</div>

<!-- / / / -->

<div class="section-title">Admin Restrictions</div>
<div class="section">
	<script>
		createFieldTable('', [
			{ title: 'Allowed Remote<br>IP Address', name: 'f_rmgt_sip', type: 'text', maxlen: 512, size: 64, suffix: '<br>&nbsp;<small>(optional; ex: "1.1.1.1", "1.1.1.0/24", "1.1.1.1 - 2.2.2.2" or "me.example.com")<\/small>', value: nvram.rmgt_sip },
			{ title: 'Limit Connection Attempts', multi: [
				{ suffix: '&nbsp; SSH &nbsp; / &nbsp;', name: 'f_limit_ssh', type: 'checkbox', value: (shlimit[0] & 1) != 0 },
				{ suffix: '&nbsp; Telnet &nbsp;', name: 'f_limit_telnet', type: 'checkbox', value: (shlimit[0] & 2) != 0 }
			] },
				{ title: '', indent: 2, multi: [
					{ name: 'f_limit_hit', type: 'text', maxlen: 4, size: 6, suffix: '&nbsp; every &nbsp;', value: shlimit[1] },
					{ name: 'f_limit_sec', type: 'text', maxlen: 4, size: 6, suffix: '&nbsp; seconds', value: shlimit[2] }
			] }
		]);
	</script>
</div>

<!-- / / / -->

<div class="section-title">Username / Password</div>
<div class="section">
	<script>
		createFieldTable('', [
			{ title: 'Username', name: 'http_username', type: 'text', maxlen: 20, value: nvram.http_username, suffix: '&nbsp;<small>(empty field means "root")<\/small>' },
			null,
			{ title: 'Password', name: 'set_password_1', type: 'password', maxlen: 60, value: '**********' },
				{ title: '<i>(re-enter to confirm)<\/i>', indent: 2, name: 'set_password_2', type: 'password', maxlen: 60, value: '**********' }
		]);
	</script>
</div>

<!-- / / / -->

<div class="section-title">Notes <small><i><a href="javascript:toggleVisibility(cprefix,'notes');"><span id="sesdiv_notes_showhide">(Show)</span></a></i></small></div>
<div class="section" id="sesdiv_notes" style="display:none">
	SSH Daemon (dropbear) also accepts additional configuration in the following files:<br>
	<ul>
		<li>/etc/shadow.custom</li>
		<li>/etc/passwd.custom</li>
		<li>/etc/gshadow.custom</li>
		<li>/etc/group.custom</li>
	</ul>
	These files are appended to the automatically generated configuration files resulting from the settings in the GUI.
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
