<!DOCTYPE html>
<!--
	Tomato GUI
	Copyright (C) 2006-2008 Jonathan Zarate
	http://www.polarcloud.com/tomato/

	Portions Copyright (C) 2008-2010 Keith Moyer, tomatovpn@keithmoyer.com

	For use with Tomato Firmware only.
	No part of this file may be used without permission.
-->
<html lang="en-GB">
<head>
<meta http-equiv="content-type" content="text/html;charset=utf-8">
<meta name="robots" content="noindex,nofollow">
<title>[<% ident(); %>] OpenVPN: Server</title>
<link rel="stylesheet" type="text/css" href="tomato.css">
<% css(); %>
<script src="isup.jsz"></script>
<script src="tomato.js"></script>
<script src="vpn.js"></script>

<script>

//	<% nvram("vpn_server_eas,vpn_server_dns,vpn_server1_poll,vpn_server1_if,vpn_server1_proto,vpn_server1_port,vpn_server1_firewall,vpn_server1_sn,vpn_server1_nm,vpn_server1_local,vpn_server1_remote,vpn_server1_dhcp,vpn_server1_r1,vpn_server1_r2,vpn_server1_crypt,vpn_server1_comp,vpn_server1_digest,vpn_server1_cipher,vpn_server1_ncp_ciphers,vpn_server1_reneg,vpn_server1_hmac,vpn_server1_plan,vpn_server1_plan1,vpn_server1_plan2,vpn_server1_plan3,vpn_server1_ccd,vpn_server1_c2c,vpn_server1_ccd_excl,vpn_server1_ccd_val,vpn_server1_pdns,vpn_server1_rgw,vpn_server1_userpass,vpn_server1_nocert,vpn_server1_users_val,vpn_server1_custom,vpn_server1_static,vpn_server1_ca,vpn_server1_ca_key,vpn_server1_crt,vpn_server1_crl,vpn_server1_key,vpn_server1_dh,vpn_server1_br,vpn_server2_poll,vpn_server2_if,vpn_server2_proto,vpn_server2_port,vpn_server2_firewall,vpn_server2_sn,vpn_server2_nm,vpn_server2_local,vpn_server2_remote,vpn_server2_dhcp,vpn_server2_r1,vpn_server2_r2,vpn_server2_crypt,vpn_server2_comp,vpn_server2_digest,vpn_server2_cipher,vpn_server2_ncp_ciphers,vpn_server2_reneg,vpn_server2_hmac,vpn_server2_plan,vpn_server2_plan1,vpn_server2_plan2,vpn_server2_plan3,vpn_server2_ccd,vpn_server2_c2c,vpn_server2_ccd_excl,vpn_server2_ccd_val,vpn_server2_pdns,vpn_server2_rgw,vpn_server2_userpass,vpn_server2_nocert,vpn_server2_users_val,vpn_server2_custom,vpn_server2_static,vpn_server2_ca,vpn_server2_ca_key,vpn_server2_crt,vpn_server2_crl,vpn_server2_key,vpn_server2_dh,vpn_server2_br,lan_ifname,lan1_ifname,lan2_ifname,lan3_ifname"); %>

var changed = 0, i;
var unitCount = OVPN_SERVER_COUNT;
var serviceType = 'vpnserver';
for (i = 1; i <= unitCount; i++) serviceLastUp.push('0');

function CCDGrid() {return this;}
CCDGrid.prototype = new TomatoGrid;

function UsersGrid() {return this;}
UsersGrid.prototype = new TomatoGrid;

var tabs =  [];
for (i = 1; i <= unitCount; ++i)
	tabs.push(['server'+i,'Server '+i]);
var sections = [['basic','Basic'],['advanced','Advanced'],['keys','Keys'],['status','Status']];

var ccdTables = [];
var usersTables = [];
var statusUpdaters = [];
for (i = 0; i < tabs.length; ++i) {
	ccdTables.push(new CCDGrid());
	ccdTables[i].servername = tabs[i][0];
	usersTables.push(new UsersGrid());
	usersTables[i].servername = tabs[i][0];
	statusUpdaters.push(new StatusUpdater());
}

var vpnciphers = vpnciphers.concat(['CAMELLIA-128-CBC'],['CAMELLIA-192-CBC'],['CAMELLIA-256-CBC']);
var ciphers = [['default','Use Default'],['none','None']];
for (i = 0; i < vpnciphers.length; ++i)
	ciphers.push([vpnciphers[i],vpnciphers[i]]);

var digests = [['default','Use Default'],['none','None']];
for (i = 0; i < vpndigests.length; ++i)
	digests.push([vpndigests[i],vpndigests[i]]);

function updateStatus(num) {
	var xob = new XmlHttp();
	xob.onCompleted = function(text, xml) {
		statusUpdaters[num].update(text);
		xob = null;
	}
	xob.onError = function(ex) {
		xob = null;
	}

	xob.post('vpnstatus.cgi', 'server='+(num + 1));
}

function tabSelect(name) {
	tgHideIcons();

	tabHigh(name);

	for (var i = 0; i < tabs.length; ++i)
		elem.display(tabs[i][0]+'-tab', (name == tabs[i][0]));

	cookie.set('vpn_server_tab', name);
}

function sectSelect(tab, section) {
	tgHideIcons();

	for (var i = 0; i < sections.length; ++i) {
		if (section == sections[i][0]) {
			elem.addClass(tabs[tab][0]+'-'+sections[i][0]+'-tab', 'active');
			elem.display(tabs[tab][0]+'-'+sections[i][0], true);
		}
		else {
			elem.removeClass(tabs[tab][0]+'-'+sections[i][0]+'-tab', 'active');
			elem.display(tabs[tab][0]+'-'+sections[i][0], false);
		}
	}

	cookie.set('vpn_server'+tab+'_section', section);
}

function updateForm(num) {
	var fom = E('t_fom');

	if (eval('isup.vpnserver'+num) && fom._service.value.indexOf('server'+num) < 0) {
		if (fom._service.value != '')
			fom._service.value += ',';

		fom._service.value += 'vpnserver'+num+'-restart';
	}
}

function rewriteUsers(row, k) {
	for (var i = 0; i < tabs.length; ++i) {
		var users_option = '';
		var userdata = usersTables[i].getAllData();
		var t = usersTables[i].servername;

		if (row && i == k) {
			userdata.push(row);
			userdata.sort(cmpInt);
		}

		if (userdata.length) {
			for (var j = 0; j < userdata.length; ++j) {
				if (userdata[j].length == 4)
					users_option += '<option value="'+userdata[j][0]+'">#'+userdata[j][0]+' - '+userdata[j][2]+(userdata[j][1] == 1 ? '' : ' (disabled)')+'<\/option>';
			}
		}
		elem.setInnerHTML('_vpn_'+t+'_usergen', (users_option ? users_option : '<option value="0">none<\/option>'));
	}
}

function v_serial(e, quiet) {
	if ((e = E(e)) == null)
		return 0;

	var v = e.value;

	if (!v.match(/[0-9A-Fa-f]{2}/)) {
		ferror.set(e, 'Invalid key serial number. Valid range: "00" - "FF"', quiet);
		return 0;
	}
	e.value = v.toUpperCase();
	ferror.clear(e);

	return 1;
}

CCDGrid.prototype.fieldValuesToData = function(row) {
	var f = fields.getAll(row);

	for (var i = 0; i < tabs.length; ++i) {
		if (ccdTables[i] == this)
			break;
	}

	return [(f[0].value ? ''+f[0].value+'' : ''+(ccdTables[i].getDataCount() + 1)+''), (f[1].checked ? '1' : '0'), f[2].value, f[3].value, f[4].value, (f[5].checked ? '1' : '0')];
}

CCDGrid.prototype.dataToView = function(data) {
	var c2c = 0, v = [];
	for (var i = 0; i < tabs.length; ++i) {
		if (ccdTables[i] == this && E('_f_vpn_server'+(i+1)+'_c2c').checked)
			c2c = 1;
	}

	var temp = [data[0], '<input type="checkbox" disabled'+(data[1] != 0 ? ' checked' : '')+'>', data[2], data[3], data[4], c2c ? '<input type="checkbox" disabled'+(data[5] != 0 ? ' checked' : '')+'>' : 'N/A'];

	for (var i = 0; i < temp.length; ++i)
		v.push((i == 1 || i == 5) ? temp[i] : escapeHTML(''+temp[i]));

	return v;
}

CCDGrid.prototype.dataToFieldValues = function(data) {
	return [data[0], data[1] == 1, data[2], data[3], data[4], data[5] == 1];
}

CCDGrid.prototype.reDraw = function() {
	var i, j, header, data, view;
	data = this.getAllData();
	header = this.header ? this.header.rowIndex + 1 : 0;
	for (i = 0; i < data.length; ++i) {
		data[i][0] = ''+(i + 1)+'';
		view = this.dataToView(data[i]);
		for (j = 0; j < view.length; ++j)
			elem.setInnerHTML(this.tb.rows[i + header].cells[j], view[j]);
	}
}

CCDGrid.prototype.rpDel = function(e) {
	changed = 1;
	e = PR(e);
	TGO(e).moving = null;
	e.parentNode.removeChild(e);
	this.reDraw();
	this.recolor();
	this.resort();
	this.rpHide();
}

CCDGrid.prototype.onDelete = function() {
	changed = 1;
	this.removeEditor();
	elem.remove(this.source);
	this.source = null;
	this.disableNewEditor(false);
	this.clearTextarea();
	this.reDraw();
	this.recolor();
	this.resort();
	this.rpHide();
}

CCDGrid.prototype.verifyFields = function(row, quiet) {
	changed = 1;
	var ok = 1;

	/* When settings change, make sure we restart the right server */
	for (var i = 0; i < tabs.length; ++i) {
		if (ccdTables[i] == this)
			updateForm(i + 1);
	}

	var f = fields.getAll(row);

	if (f[2].value == '') {
		ferror.set(f[2], 'Common name is mandatory', quiet || !ok);
		ok = 0;
	}
	else if (f[2].value.indexOf('>') >= 0 || f[2].value.indexOf('<') >= 0) {
		ferror.set(f[2], 'Common name cannot contain "<" or ">" characters', quiet || !ok);
		ok = 0;
	}
	else
		ferror.clear(f[2]);

	if (f[3].value != '' && !v_ip(f[3], quiet || !ok, 0))
		ok = 0;
	else if (f[3].value == '' && f[4].value != '') {
		ferror.set(f[3], 'Either both or neither subnet and netmask must be provided', quiet || !ok);
		ok = 0;
	}
	else
		ferror.clear(f[3]);

	if (f[4].value != '' && !v_netmask(f[4], quiet || !ok))
		ok = 0;
	else if (f[4].value == '' && f[3].value != '') {
		ferror.set(f[4], 'Either both or neither subnet and netmask must be provided', quiet || !ok);
		ok = 0;
	}
	else
		ferror.clear(f[4]);

	if (f[5].checked && (f[3].value == '' || f[4].value == '')) {
		ferror.set(f[5], 'Cannot push routes if they\'re not given. Please provide subnet/netmask', quiet || !ok);
		ok = 0;
	}
	else
		ferror.clear(f[5]);

	return ok;
}

UsersGrid.prototype.reDraw = function() {
	var i, j, header, data, view;
	data = this.getAllData();
	header = this.header ? this.header.rowIndex + 1 : 0;
	for (i = 0; i < data.length; ++i) {
		data[i][0] = ''+(i + 1)+'';
		view = this.dataToView(data[i]);

		for (j = 0; j < view.length; ++j)
			elem.setInnerHTML(this.tb.rows[i + header].cells[j], view[j]);
	}
	rewriteUsers();
}

UsersGrid.prototype.fieldValuesToData = function(row) {
	var f = fields.getAll(row);

	for (var i = 0; i < tabs.length; ++i) {
		if (usersTables[i] == this)
			break;
	}
	var r = [(f[0].value ? ''+f[0].value+'' : ''+(usersTables[i].getDataCount() + 1)+''), (f[1].checked ? '1' : '0'), f[2].value, f[3].value];
	rewriteUsers(r, i);

	return r;
}

UsersGrid.prototype.dataToView = function(data) {
	var v = [];
	for (var i = 0; i < data.length; ++i) {
		var s = escapeHTML('' + data[i]);
		if (this.editorFields && this.editorFields.length > i) {
			var ef = this.editorFields[i].multi;
			if (!ef) ef = [this.editorFields[i]];
			var f = (ef && ef.length > 0 ? ef[0] : null);
			if (f && f.type == 'password') {
				if (!f.peekaboo || get_config('web_pb', '1') != '0')
					s = s.replace(/./g, '&#x25CF;');
			}
		}
		if (i == 1) s = '<input type="checkbox" disabled'+(s != 0 ? ' checked' : '')+'>'
		v.push(s);
	}
	return v;
}

UsersGrid.prototype.dataToFieldValues = function(data) {
	return [data[0], data[1] == 1, data[2], data[3]];
}

UsersGrid.prototype.rpDel = function(e) {
	changed = 1;
	e = PR(e);
	TGO(e).moving = null;
	e.parentNode.removeChild(e);
	this.reDraw();
	this.recolor();
	this.resort();
	this.rpHide();
}

UsersGrid.prototype.onDelete = function() {
	changed = 1;
	this.removeEditor();
	elem.remove(this.source);
	this.source = null;
	this.disableNewEditor(false);
	this.clearTextarea();
	this.reDraw();
	this.recolor();
	this.resort();
	this.rpHide();
}

UsersGrid.prototype.verifyFields = function(row, quiet) {
	changed = 1;
	var ok = 1;

	/* When settings change, make sure we restart the right server */
	for (var i = 0; i < tabs.length; ++i) {
		if (usersTables[i] == this)
			updateForm(i + 1);
	}
	var f = fields.getAll(row);

	if (f[2].value == '') {
		ferror.set(f[2], 'username is mandatory', quiet || !ok);
		ok = 0;
	}
	else if (f[2].value.indexOf('>') >= 0 || f[2].value.indexOf('<') >= 0) {
		ferror.set(f[2], 'username cannot contain "<" or ">" characters', quiet || !ok);
		ok = 0;
	}
	else
		ferror.clear(f[2]);

	if (f[3].value == '' ) {
		ferror.set(f[3], 'password is mandatory', quiet || !ok);
		ok = 0;
	}
	else if (f[3].value.indexOf('>') >= 0 || f[3].value.indexOf('<') >= 0) {
		ferror.set(f[3], 'password cannot contain "<" or ">" characters', quiet || !ok);
		ok = 0;
	}
	else
		ferror.clear(f[3]);

	return ok;
}

var keyGenRequest = null

function updateStaticKey(num) {
	if (keyGenRequest)
		return;

	disableKeyButtons(num, true);
	changed = 1;
	elem.display(E('server'+num+'_static_progress_div'), true);
	keyGenRequest = new XmlHttp();

	keyGenRequest.onCompleted = function(text, xml) {
		E('_vpn_server'+num+'_static').value = text;
		keyGenRequest = null;
		elem.display(E('server'+num+'_static_progress_div'), false);
		disableKeyButtons(num, false);
	}
	keyGenRequest.onError = function(ex) { keyGenRequest = null; }
	var crypt = E('_vpn_server'+num+'_crypt').value;
	var hmac = E('_vpn_server'+num+'_hmac').value;
	keyGenRequest.post('vpngenkey.cgi', '_mode='+((crypt == 'tls' && hmac == 4) ? 'static2' : 'static1')+'&_server='+num);
}

function generateDHParams(num) {
	if (keyGenRequest)
		return;

	if (confirm('WARNING: DH Parameters generation can take a long time.\nIf it freezes, refresh the page and try again.\n\nDo you want to proceed?')) {
		changed = 1;
		disableKeyButtons(num, true);
		elem.display(E('server'+num+'_dh_progress_div'), true);
		keyGenRequest = new XmlHttp();

		keyGenRequest.onCompleted = function(text, xml) {
			E('_vpn_server'+num+'_dh').value = text;
			keyGenRequest = null;
			elem.display(E('server'+num+'_dh_progress_div'), false);
			disableKeyButtons(num, false);
		}
		keyGenRequest.onError = function(ex) { keyGenRequest = null; }
		keyGenRequest.post('vpngenkey.cgi', '_mode=dh');
	}
}

function generateKeys(num) {
	if (keyGenRequest)
		return;

	if (changed) {
		alert('Changes have been made. You need to save before continue!');
		return;
	}

	if (E('_vpn_server'+num+'_ca_key').value == '') {
		if (!confirm('WARNING: You haven\'t provided Certificate Authority Key.\n'+
		             'This means, that CA Key needs to be regenerated, but it WILL break ALL your existing client certificates.\n'+
		             'You will need to reconfigure all your existing VPN clients!\n Are you sure to continue?'))
			return;
	}

	changed = 1;
	disableKeyButtons(num, true);
	showTLSProgressDivs(num, true);
	var cakey, cacert, generated_crt, generated_key;
	keyGenRequest = new XmlHttp();

	keyGenRequest.onCompleted = function(text, xml) {
		eval(text);
		E('_vpn_server'+num+'_ca_key').value = cakey;
		E('_vpn_server'+num+'_ca').value = cacert;
		E('_vpn_server'+num+'_crt').value = generated_crt;
		E('_vpn_server'+num+'_key').value = generated_key;
		keyGenRequest = null;
		disableKeyButtons(num, false);
		showTLSProgressDivs(num, false);
	}
	keyGenRequest.onError = function(ex) { keyGenRequest = null; }
	keyGenRequest.post('vpngenkey.cgi', '_mode=key&_server='+num);
}

function disableKeyButtons(num, state) {
	E('_vpn_keygen_static_server'+num+'_button').disabled = state;
	E('_vpn_keygen_server'+num+'_button').disabled = state;
	E('_vpn_dhgen_server'+num+'_button').disabled = state;
}

function showTLSProgressDivs(num, state) {
	elem.display(E('server'+num+'_key_progress_div'), state);
	elem.display(E('server'+num+'_cert_progress_div'), state);
	elem.display(E('server'+num+'_ca_progress_div'), state);
	elem.display(E('server'+num+'_ca_key_progress_div'), state);
}

function downloadClientConfig(num) {
	if (keyGenRequest)
		return;

	var warn = 0, userid = 0;
	var caKey = E('_vpn_server'+num+'_ca_key').value;
	var ca = E('_vpn_server'+num+'_ca').value;
	var serverCrt = E('_vpn_server'+num+'_crt').value;
	var serverCrtKey = E('_vpn_server'+num+'_key').value;
	var staticKey = E('_vpn_server'+num+'_static').value;
	var crypt = E('_vpn_server'+num+'_crypt').value;
	var hmac = E('_vpn_server'+num+'_hmac').value;
	var userpass = E('_f_vpn_server'+num+'_userpass').checked;
	var isUser = E('_vpn_server'+num+'_usergen').value;

	if (crypt == 'secret') {
		if (staticKey == '')
			warn = 1;
	}
	else if (crypt == 'tls' && hmac >= 0) {
		if (staticKey == '' || caKey == '' || ca == '' || serverCrt == '' || serverCrtKey == '')
			warn = 1;
	}
	else {
		if (caKey == '' || ca == '' || serverCrt == '' || serverCrtKey == '')
			warn = 1;
	}

	if (warn) {
		alert('Not all key fields have been entered!');
		return;
	}
	if (changed) {
		alert('Changes have been made. You need to save before continue!');
		return;
	}

	if (crypt == 'tls') {
		if (userpass && isUser) {
			var userdata = usersTables[(num - 1)].getAllData();
			if (userdata.length)
				userid = userdata[isUser - 1][0];
		}
		else {
			if (!v_serial('_vpn_server'+num+'_serial', 0))
				return;

			userid = parseInt(E('_vpn_server'+num+'_serial').value, 16);
		}
	}

	elem.display(E('server'+num+'_gen_progress_div'), true);
	keyGenRequest = new XmlHttp();
	keyGenRequest.onCompleted = function(text, xml) {
		elem.display(E('server'+num+'_gen_progress_div'), false);
		keyGenRequest = null;

		var dlFileFakeLink = document.createElement('a');
		dlFileFakeLink.setAttribute('href', 'data:application/tomato-binary-file,'+escapeCGI(text));
		dlFileFakeLink.setAttribute('download', 'ClientConfig.tgz');

		dlFileFakeLink.style.display = 'none';
		document.body.appendChild(dlFileFakeLink);

		dlFileFakeLink.click();

		document.body.removeChild(dlFileFakeLink);
	}
	keyGenRequest.onError = function(ex) { keyGenRequest = null; }
	keyGenRequest.responseType = 'blob';
	keyGenRequest.get('vpn/ClientConfig.tgz','_server='+num+((userid) ? '&_userid='+userid : ''));
}

function verifyFields(focused, quiet) {
	tgHideIcons();

	var i, j, t, ok = 1;

	/* When settings change, make sure we restart the right services */
	if (focused) {
		changed = 1;

		var fom = E('t_fom');
		var serveridx = focused.name.indexOf('server');
		if (serveridx >= 0) {
			var num = focused.name.substring(serveridx + 6, serveridx + 7);

			updateForm(num);

			if ((focused.name.indexOf('_dns') >= 0 || (focused.name.indexOf('_if') >= 0 && E('_f_vpn_server'+num+'_dns').checked)) && fom._service.value.indexOf('dnsmasq') < 0) {
				if (fom._service.value != '')
					fom._service.value += ',';

				fom._service.value += 'dnsmasq-restart';
			}

			if (focused.name.indexOf('_c2c') >= 0)
				ccdTables[num - 1].reDraw();
		}
	}

	/* Element varification */
	for (i = 0; i < tabs.length; ++i) {
		t = tabs[i][0];

		if (!v_range('_vpn_'+t+'_poll', quiet || !ok, 0, 30))
			ok = 0;
		if (!v_port('_vpn_'+t+'_port', quiet || !ok))
			ok = 0;
		if (!v_ip('_vpn_'+t+'_sn', quiet || !ok, 0))
			ok = 0;
		if (!v_netmask('_vpn_'+t+'_nm', quiet || !ok))
			ok = 0;
		if (!v_ip('_vpn_'+t+'_r1', quiet || !ok, 1))
			ok = 0;
		if (!v_ip('_vpn_'+t+'_r2', quiet || !ok, 1))
			ok = 0;
		if (!v_ip('_vpn_'+t+'_local', quiet || !ok, 1))
			ok = 0;
		if (!v_ip('_vpn_'+t+'_remote', quiet || !ok, 1))
			ok = 0;
		if (!v_range('_vpn_'+t+'_reneg', quiet || !ok, -1, 2147483647))
			ok = 0;
		if (E('_vpn_'+t+'_crypt').value == 'tls' && !E('_f_vpn_'+t+'_userpass').checked && !v_serial('_vpn_'+t+'_serial', quiet || !ok))
			ok = 0;
	}

	/* Visibility changes */
	for (i = 0; i < tabs.length; ++i) {
		t = tabs[i][0];

		var auth = E('_vpn_'+t+'_crypt').value;
		var iface = E('_vpn_'+t+'_if').value;
		var hmac = E('_vpn_'+t+'_hmac').value;
		var dhcp = E('_f_vpn_'+t+'_dhcp').checked;
		var ccd = E('_f_vpn_'+t+'_ccd').checked;
		var userpass = E('_f_vpn_'+t+'_userpass').checked;
		var dns = E('_f_vpn_'+t+'_dns').checked;
		var comp = E('_vpn_'+t+'_comp').value;

		elem.display(PR('_vpn_'+t+'_ca'), PR('_vpn_'+t+'_ca_key'), PR('_vpn_'+t+'_ca_key_div_help'),
			     PR('_vpn_dhgen_'+t+'_button'), PR('_vpn_'+t+'_crt'), PR('_vpn_'+t+'_crl'), PR('_vpn_'+t+'_dh'),
			     PR('_vpn_'+t+'_key'), PR('_vpn_'+t+'_hmac'), PR('_f_vpn_'+t+'_rgw'),
			     PR('_vpn_'+t+'_reneg'), auth == 'tls');
		elem.display(PR('_vpn_'+t+'_static'), auth == 'secret' || (auth == 'tls' && hmac >= 0));
		elem.display(PR('_vpn_keygen_static_'+t+'_button'), auth == 'secret' || (auth == 'tls' && hmac >= 0));
		elem.display(E(t+'_custom_crypto_text'), auth == 'custom');
		elem.display(PR('_vpn_keygen_'+t+'_button'), auth == 'tls');
		elem.display(PR('_vpn_'+t+'_sn'), PR('_f_vpn_'+t+'_plan'), PR('_f_vpn_'+t+'_plan1'),
		             PR('_f_vpn_'+t+'_plan2'), PR('_f_vpn_'+t+'_plan3'), auth == 'tls' && iface == 'tun');
		elem.display(PR('_f_vpn_'+t+'_dhcp'), auth == 'tls' && iface == 'tap');
		elem.display(PR('_vpn_'+t+'_br'), iface == 'tap');
		elem.display(E(t+'_range'), !dhcp);
		elem.display(PR('_vpn_'+t+'_local'), auth == 'secret' && iface == 'tun');
		elem.display(PR('_f_vpn_'+t+'_ccd'), auth == 'tls');
		elem.display(PR('_f_vpn_'+t+'_userpass'), auth == 'tls');
		elem.display(PR('_f_vpn_'+t+'_nocert'), PR('table_'+t+'_users'), auth == 'tls' && userpass);
		elem.display(PR('_f_vpn_'+t+'_c2c'), PR('_f_vpn_'+t+'_ccd_excl'), PR('table_'+t+'_ccd'), auth == 'tls' && ccd);
		elem.display(PR('_f_vpn_'+t+'_pdns'), auth == 'tls' && dns );
		elem.display(PR('_vpn_'+t+'_ncp_ciphers'), auth == 'tls');
		elem.display(PR('_vpn_'+t+'_cipher'), auth == 'secret');
		elem.display(PR('_vpn_client_gen_'+t+'_button'), auth != 'custom');
		elem.display(PR('_vpn_'+t+'_serial'), auth == 'tls' && !userpass);
		elem.display(PR('_vpn_'+t+'_usergen'), auth == 'tls' && userpass);

		var keyHelp = E(t+'-keyhelp');
		switch (auth) {
		case 'tls':
			keyHelp.href = helpURL['TLSKeys'];
		break;
		case 'secret':
			keyHelp.href = helpURL['staticKeys'];
		break;
		default:
			keyHelp.href = helpURL['howto'];
		break;
		}
	}

	for (i = 0; i < tabs.length; ++i) {
		for (j = 0; j <= 3; ++j) {
			t = (j == 0 ? '' : j);

			if (eval('nvram.lan'+t+'_ifname.length') < 1) {
				E('_vpn_server'+(i + 1)+'_br').options[j].disabled = 1;
				/* also disable and un-check push lanX (*_plan) */
				E('_f_vpn_server'+(i + 1)+'_plan'+t).checked = 0;
				E('_f_vpn_server'+(i + 1)+'_plan'+t).disabled = 1;
			}
		}
	}

	return ok;
}

function save() {
	if (!verifyFields(null, 0))
		return;

	var i, j, t, n;
	var fom = E('t_fom');

	E('vpn_server_eas').value = '';
	E('vpn_server_dns').value = '';

	for (i = 0; i < tabs.length; ++i) {
		if (ccdTables[i].isEditing())
			return;
		if (usersTables[i].isEditing())
			return;

		t = tabs[i][0];

		var crypt = E('_vpn_'+t+'_crypt').value;
		var hmac = E('_vpn_'+t+'_hmac').value;
		var key = E('_vpn_'+t+'_static').value;
		if (key != '') {
			if (((crypt == 'secret' || (crypt == 'tls' && hmac >= 0 && hmac < 4)) && key.indexOf('OpenVPN Static key V1') === -1) ||
			    (crypt == 'tls' && hmac == 4 && key.indexOf('OpenVPN tls-crypt-v2') === -1)) {
				alert('Keys->Static Key is in the wrong format for the selected Auth Method - Re-Generate it!');
				return;
			}
		}

		if (E('_f_vpn_'+t+'_eas').checked)
			E('vpn_server_eas').value += ''+(i + 1)+',';

		if (E('_f_vpn_'+t+'_dns').checked)
			E('vpn_server_dns').value += ''+(i + 1)+',';

		var data = ccdTables[i].getAllData();
		var ccd = '';
		for (j = 0; j < data.length; ++j) {
			var c = data[j].join('<')+'>';
			n = c.indexOf("<");
			ccd += c.substring(n + 1);
		}

		var userdata = usersTables[i].getAllData();
		var users = '';
		for (j = 0; j < userdata.length; ++j) {
			var u = userdata[j].join('<')+'>';
			n = u.indexOf("<");
			users += u.substring(n + 1);
		}

		E('vpn_'+t+'_dhcp').value = E('_f_vpn_'+t+'_dhcp').checked ? 1 : 0;
		E('vpn_'+t+'_plan').value = E('_f_vpn_'+t+'_plan').checked ? 1 : 0;
		E('vpn_'+t+'_plan1').value = E('_f_vpn_'+t+'_plan1').checked ? 1 : 0;
		E('vpn_'+t+'_plan2').value = E('_f_vpn_'+t+'_plan2').checked ? 1 : 0;
		E('vpn_'+t+'_plan3').value = E('_f_vpn_'+t+'_plan3').checked ? 1 : 0;
		E('vpn_'+t+'_ccd').value = E('_f_vpn_'+t+'_ccd').checked ? 1 : 0;
		E('vpn_'+t+'_c2c').value = E('_f_vpn_'+t+'_c2c').checked ? 1 : 0;
		E('vpn_'+t+'_ccd_excl').value = E('_f_vpn_'+t+'_ccd_excl').checked ? 1 : 0;
		E('vpn_'+t+'_ccd_val').value = ccd;
		E('vpn_'+t+'_userpass').value = E('_f_vpn_'+t+'_userpass').checked ? 1 : 0;
		E('vpn_'+t+'_nocert').value = E('_f_vpn_'+t+'_nocert').checked ? 1 : 0;
		E('vpn_'+t+'_users_val').value = users;
		E('vpn_'+t+'_pdns').value = E('_f_vpn_'+t+'_pdns').checked ? 1 : 0;
		E('vpn_'+t+'_rgw').value = E('_f_vpn_'+t+'_rgw').checked ? 1 : 0;
	}
	fom._nofootermsg.value = 0;

	form.submit(fom, 1);

	changed = 0;
}

function earlyInit() {
	show();
	tabSelect(cookie.get('vpn_server_tab') || tabs[0][0]);

	for (var i = 0; i < tabs.length; ++i) {
		sectSelect(i, cookie.get('vpn_server'+i+'_section') || sections[0][0]);
		var t = tabs[i][0];

		ccdTables[i].init('table_'+t+'_ccd','sort',0,[{ type: 'text', maxlen: 2, attrib: 'hidden' },{ type: 'checkbox', prefix: '<div class="centered">', suffix: '<\/div>' },{ type: 'text', maxlen: 30 },{ type: 'text', maxlen: 15 },{ type: 'text', maxlen: 15 },{ type: 'checkbox', prefix: '<div class="centered">', suffix: '<\/div>' }]);
		ccdTables[i].headerSet(['#','Enable','Common Name','Subnet','Netmask','Push']);
		var ccdVal = eval('nvram.vpn_'+t+'_ccd_val');
		if (ccdVal.length) {
			var s = ccdVal.split('>');
			for (var j = 0; j < s.length; ++j) {
				if (!s[j].length)
					continue;

				var row = [''+(j + 1)+''];
				row = row.concat(s[j].split('<'));
				if (row.length == 6)
					ccdTables[i].insertData(-1, row);
			}
		}
		ccdTables[i].showNewEditor();
		ccdTables[i].resetNewEditor();

		usersTables[i].init('table_'+t+'_users','sort',0,[
				{ type: 'text', maxlen: 2, attrib: 'hidden' },
				{ type: 'checkbox', prefix: '<div class="centered">', suffix: '<\/div>' },
				{ type: 'text', maxlen: 25 },
				{ type: 'password', peekaboo: 1, maxlen: 15 }]);
		usersTables[i].headerSet(['#','Enable','Username','Password']);
		var usersVal = eval('nvram.vpn_'+t+'_users_val');
		if (usersVal.length) {
			var s = usersVal.split('>');
			for (var j = 0; j < s.length; ++j) {
				if (!s[j].length)
					continue;

				var row = [''+(j + 1)+''];
				row = row.concat(s[j].split('<'));
				if (row.length == 4)
					usersTables[i].insertData(-1, row);
			}
		}
		usersTables[i].showNewEditor();
		usersTables[i].resetNewEditor();

		statusUpdaters[i].init(t+'-status-clients-table',t+'-status-routing-table',t+'-status-stats-table',t+'-status-time',t+'-status-content',t+'-no-status');
	}

	verifyFields(null, 1);
}

function init() {
	rewriteUsers();
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

<input type="hidden" name="_nextpage" value="vpn-server.asp">
<input type="hidden" name="_service" value="">
<input type="hidden" name="_nofootermsg">
<input type="hidden" name="vpn_server_eas" id="vpn_server_eas">
<input type="hidden" name="vpn_server_dns" id="vpn_server_dns">

<!-- / / / -->

<div class="section-title">OpenVPN Server Configuration</div>
<div class="section">
	<script>
		tabCreate.apply(this, tabs);

		for (i = 0; i < tabs.length; ++i) {
			t = tabs[i][0];
			W('<div id="'+t+'-tab">');
			W('<input type="hidden" id="vpn_'+t+'_dhcp" name="vpn_'+t+'_dhcp">');
			W('<input type="hidden" id="vpn_'+t+'_plan" name="vpn_'+t+'_plan">');
			W('<input type="hidden" id="vpn_'+t+'_plan1" name="vpn_'+t+'_plan1">');
			W('<input type="hidden" id="vpn_'+t+'_plan2" name="vpn_'+t+'_plan2">');
			W('<input type="hidden" id="vpn_'+t+'_plan3" name="vpn_'+t+'_plan3">');
			W('<input type="hidden" id="vpn_'+t+'_ccd" name="vpn_'+t+'_ccd">');
			W('<input type="hidden" id="vpn_'+t+'_c2c" name="vpn_'+t+'_c2c">');
			W('<input type="hidden" id="vpn_'+t+'_ccd_excl" name="vpn_'+t+'_ccd_excl">');
			W('<input type="hidden" id="vpn_'+t+'_ccd_val" name="vpn_'+t+'_ccd_val">');
			W('<input type="hidden" id="vpn_'+t+'_userpass" name="vpn_'+t+'_userpass">');
			W('<input type="hidden" id="vpn_'+t+'_nocert" name="vpn_'+t+'_nocert">');
			W('<input type="hidden" id="vpn_'+t+'_users_val" name="vpn_'+t+'_users_val">');
			W('<input type="hidden" id="vpn_'+t+'_pdns" name="vpn_'+t+'_pdns">');
			W('<input type="hidden" id="vpn_'+t+'_rgw" name="vpn_'+t+'_rgw">');

			W('<ul class="tabs">');
			for (j = 0; j < sections.length; j++) {
				W('<li><a href="javascript:sectSelect('+i+',\''+sections[j][0]+'\')" id="'+t+'-'+sections[j][0]+'-tab">'+sections[j][1]+'<\/a><\/li>');
			}
			W('<\/ul><div class="tabs-bottom"><\/div>');

			W('<div id="'+t+'-basic">');
			createFieldTable('', [
				{ title: 'Enable on Start', name: 'f_vpn_'+t+'_eas', type: 'checkbox', value: nvram.vpn_server_eas.indexOf(''+(i+1)) >= 0 },
				{ title: 'Interface Type', name: 'vpn_'+t+'_if', type: 'select', options: [['tap','TAP'],['tun','TUN']], value: eval('nvram.vpn_'+t+'_if') },
					{ title: 'Bridge TAP with', indent: 2, name: 'vpn_'+t+'_br', type: 'select', options: [['br0','LAN0 (br0)*'],['br1','LAN1 (br1)'],['br2','LAN2 (br2)'],['br3','LAN3 (br3)']],
						value: eval ('nvram.vpn_'+t+'_br'), suffix: ' <small>* default<\/small> ' },
				{ title: 'Protocol', name: 'vpn_'+t+'_proto', type: 'select', options: [['udp','UDP'],['tcp-server','TCP'],['udp4','UDP4'],['tcp4-server','TCP4'],['udp6','UDP6'],['tcp6-server','TCP6']], value: eval('nvram.vpn_'+t+'_proto') },
				{ title: 'Port', name: 'vpn_'+t+'_port', type: 'text', maxlen: 5, size: 10, value: eval('nvram.vpn_'+t+'_port') },
				{ title: 'Firewall', name: 'vpn_'+t+'_firewall', type: 'select', options: [['auto','Automatic'],['external','External Only'],['custom','Custom']], value: eval('nvram.vpn_'+t+'_firewall') },
				{ title: 'Authorization Mode', name: 'vpn_'+t+'_crypt', type: 'select', options: [['tls','TLS'],['secret','Static Key'],['custom','Custom']], value: eval('nvram.vpn_'+t+'_crypt'),
					suffix: ' <small id="'+t+'_custom_crypto_text">must be configured manually<\/small>' },
				{ title: 'TLS control channel security <small>(tls-auth/tls-crypt)<\/small>', name: 'vpn_'+t+'_hmac', type: 'select', options: [[-1,'Disabled'],[2,'Bi-directional Auth'],[0,'Incoming Auth (0)'],[1,'Outgoing Auth (1)'],[3,'Encrypt Channel'],[4,'Encrypt Channel V2']], value: eval('nvram.vpn_'+t+'_hmac') },
				{ title: 'Auth digest', name: 'vpn_'+t+'_digest', type: 'select', options: digests, value: eval('nvram.vpn_'+t+'_digest') },
				{ title: 'VPN subnet/netmask', multi: [
					{ name: 'vpn_'+t+'_sn', type: 'text', maxlen: 15, size: 17, value: eval('nvram.vpn_'+t+'_sn'), suffix: ' ' },
					{ name: 'vpn_'+t+'_nm', type: 'text', maxlen: 15, size: 17, value: eval('nvram.vpn_'+t+'_nm') } ] },
				{ title: 'Client address pool', multi: [
					{ name: 'f_vpn_'+t+'_dhcp', type: 'checkbox', value: eval('nvram.vpn_'+t+'_dhcp') != 0, suffix: '&nbsp; DHCP ' },
					{ name: 'vpn_'+t+'_r1', type: 'text', maxlen: 15, size: 17, value: eval('nvram.vpn_'+t+'_r1'), prefix: '<span id="'+t+'_range">', suffix: ' - ' },
					{ name: 'vpn_'+t+'_r2', type: 'text', maxlen: 15, size: 17, value: eval('nvram.vpn_'+t+'_r2'), suffix: '<\/span>' } ] },
				{ title: 'Local/remote endpoint addresses', multi: [
					{ name: 'vpn_'+t+'_local', type: 'text', maxlen: 15, size: 17, value: eval('nvram.vpn_'+t+'_local') },
					{ name: 'vpn_'+t+'_remote', type: 'text', maxlen: 15, size: 17, value: eval('nvram.vpn_'+t+'_remote') } ] }
			]);
			W('<\/div>');

			W('<div id="'+t+'-advanced">');
			createFieldTable('', [
				null,
				{ title: 'Poll Interval', name: 'vpn_'+t+'_poll', type: 'text', maxlen: 2, size: 5, value: eval('nvram.vpn_'+t+'_poll'), suffix: ' <small>minutes; 0 to disable<\/small>' },
				{ title: 'Push LAN0 (br0) to clients', name: 'f_vpn_'+t+'_plan', type: 'checkbox', value: eval('nvram.vpn_'+t+'_plan') != 0 },
				{ title: 'Push LAN1 (br1) to clients', name: 'f_vpn_'+t+'_plan1', type: 'checkbox', value: eval('nvram.vpn_'+t+'_plan1') != 0 },
				{ title: 'Push LAN2 (br2) to clients', name: 'f_vpn_'+t+'_plan2', type: 'checkbox', value: eval('nvram.vpn_'+t+'_plan2') != 0 },
				{ title: 'Push LAN3 (br3) to clients', name: 'f_vpn_'+t+'_plan3', type: 'checkbox', value: eval('nvram.vpn_'+t+'_plan3') != 0 },
				{ title: 'Direct clients to<br>redirect Internet traffic', name: 'f_vpn_'+t+'_rgw', type: 'checkbox', value: eval('nvram.vpn_'+t+'_rgw') != 0 },
				{ title: 'Respond to DNS', name: 'f_vpn_'+t+'_dns', type: 'checkbox', value: nvram.vpn_server_dns.indexOf(''+(i+1)) >= 0 },
				{ title: 'Advertise DNS to clients', name: 'f_vpn_'+t+'_pdns', type: 'checkbox', value: eval('nvram.vpn_'+t+'_pdns') != 0 },
				{ title: 'Data ciphers', name: 'vpn_'+t+'_ncp_ciphers', type: 'text', size: 70, maxlen: 127, value: eval('nvram.vpn_'+t+'_ncp_ciphers') },
				{ title: 'Cipher', name: 'vpn_'+t+'_cipher', type: 'select', options: ciphers, value: eval('nvram.vpn_'+t+'_cipher') },
				{ title: 'Compression', name: 'vpn_'+t+'_comp', type: 'select', options: [['-1','Disabled'],['no','None'],['yes','LZO'],['adaptive','LZO Adaptive'],['lz4','LZ4'],['lz4-v2','LZ4-V2']], value: eval('nvram.vpn_'+t+'_comp') },
				{ title: 'TLS Renegotiation Time', name: 'vpn_'+t+'_reneg', type: 'text', maxlen: 10, size: 7, value: eval('nvram.vpn_'+t+'_reneg'), suffix: ' <small> seconds; -1 for default<\/small>' },
				{ title: 'Manage Client-Specific Options', name: 'f_vpn_'+t+'_ccd', type: 'checkbox', value: eval('nvram.vpn_'+t+'_ccd') != 0 },
				{ title: 'Allow Client<->Client', name: 'f_vpn_'+t+'_c2c', type: 'checkbox', value: eval('nvram.vpn_'+t+'_c2c') != 0 },
				{ title: 'Allow Only These Clients', name: 'f_vpn_'+t+'_ccd_excl', type: 'checkbox', value: eval('nvram.vpn_'+t+'_ccd_excl') != 0 },
				{ title: '', suffix: '<div class="tomato-grid" id="table_'+t+'_ccd"><\/div>' },
				{ title: 'Allow User/Pass Auth', name: 'f_vpn_'+t+'_userpass', type: 'checkbox', value: eval('nvram.vpn_'+t+'_userpass') != 0 },
				{ title: 'Allow Only User/Pass (without cert) Auth', name: 'f_vpn_'+t+'_nocert', type: 'checkbox', value: eval('nvram.vpn_'+t+'_nocert') != 0 },
				{ title: '', suffix: '<div class="tomato-grid" id="table_'+t+'_users"><\/div>' },
				{ title: 'Custom Configuration', name: 'vpn_'+t+'_custom', type: 'textarea', value: eval('nvram.vpn_'+t+'_custom') }
			]);
			W('<\/div>');

			W('<div id="'+t+'-keys">');
			W('<p class="vpn-keyhelp">For help generating keys, refer to the OpenVPN <a id="'+t+'-keyhelp">HOWTO<\/a>. All 6 keys take about 14kB of NVRAM, so check first if there is enough free space!<\/p>');
			createFieldTable('', [
				null,
				{ title: 'Static Key', name: 'vpn_'+t+'_static', type: 'textarea', value: eval('nvram.vpn_'+t+'_static'),
					prefix: '<div id="'+t+'_static_progress_div" style="display:none"><p class="keyhelp">Please wait - generating static key...<img src="spin.gif" alt=""><\/p><\/div>' },
				{ title: '', custom: '<input type="button" value="Generate static key" onclick="updateStaticKey('+(i+1)+')" id="_vpn_keygen_static_'+t+'_button">' }
			]);
			createFieldTable('', [
				null,
				{ title: 'Certificate Authority Key', name: 'vpn_'+t+'_ca_key', type: 'textarea', value: eval('nvram.vpn_'+t+'_ca_key'),
					prefix: '<div id="'+t+'_ca_key_progress_div" style="display:none"><p class="keyhelp">Please wait - generating CA key...<img src="spin.gif" alt=""><\/p><\/div>' },
				{ title: '', custom: '<div id="_vpn_'+t+'_ca_key_div_help"><p class="keyhelp">Optional, only used for client certificate generation.<br>Uncrypted (-nodes) private keys are supported.<\/p><\/div>' },
				{ title: 'Certificate Authority', name: 'vpn_'+t+'_ca', type: 'textarea', value: eval('nvram.vpn_'+t+'_ca'),
					prefix: '<div id="'+t+'_ca_progress_div" style="display:none"><p class="keyhelp">Please wait - generating CA certificate...<img src="spin.gif" alt=""><\/p><\/div>' },
				{ title: 'Server Certificate', name: 'vpn_'+t+'_crt', type: 'textarea', value: eval('nvram.vpn_'+t+'_crt'),
					prefix: '<div id="'+t+'_cert_progress_div" style="display: none"><p class="keyhelp">Please wait - generating certificate...<img src="spin.gif" alt=""><\/p><\/div>' },
				{ title: 'Server Key', name: 'vpn_'+t+'_key', type: 'textarea', value: eval('nvram.vpn_'+t+'_key'),
					prefix: '<div id="'+t+'_key_progress_div" style="display: none"><p class="keyhelp">Please wait - generating key...<img src="spin.gif" alt=""><\/p><\/div>' },
				{ title: 'CRL file', name: 'vpn_'+t+'_crl', type: 'textarea', value: eval('nvram.vpn_'+t+'_crl') },
				{ title: '', custom: '<input type="button" value="Generate keys" onclick="generateKeys('+(i+1)+')" id="_vpn_keygen_'+t+'_button">' }
			]);
			createFieldTable('', [
				null,
				{ title: 'Diffie Hellman parameters', name: 'vpn_'+t+'_dh', type: 'textarea', value: eval('nvram.vpn_'+t+'_dh'),
					prefix: '<div id="'+t+'_dh_progress_div" style="display:none"><p class="keyhelp">Please wait - generating DH parameters...<img src="spin.gif" alt=""><\/p><\/div>' },
				{ title: '', custom: '<input type="button" value="Generate DH Params" onclick="generateDHParams('+(i+1)+')" id="_vpn_dhgen_'+t+'_button">' }
			]);
			createFieldTable('', [
				null,
				{ title: 'Serial number', custom: '<input type="text" name="vpn_'+t+'_serial" value="00" maxlength="2" size="2" id="_vpn_'+t+'_serial">' },
				{ title: 'User', custom: '<select name="vpn_'+t+'_usergen" id="_vpn_'+t+'_usergen"><\/select>' },
				{ title: '', custom: '<input type="button" value="Generate client config" onclick="downloadClientConfig('+(i+1)+')" id="_vpn_client_gen_'+t+'_button">',
					suffix: '<div id="'+t+'_gen_progress_div" style="display:none"><p class="keyhelp">Please wait while the configuration is being generated...<img src="spin.gif" alt=""><\/p><\/div>' }
			]);
			W('<\/div>');

			W('<div id="'+t+'-status">');
			W('<div id="'+t+'-no-status"><p>Server is not running or status could not be read.<\/p><\/div>');
			W('<div id="'+t+'-status-content" style="display:none" class="status-content">');
			W('<div id="'+t+'-status-header" class="vpn-status-header"><p>Data current as of <span id="'+t+'-status-time"><\/span><\/p><\/div>');
			W('<div id="'+t+'-status-clients"><div class="section-title">Client List<\/div><div class="tomato-grid vpn-status-table" id="'+t+'-status-clients-table"><\/div><br><\/div>');
			W('<div id="'+t+'-status-routing"><div class="section-title">Routing Table<\/div><div class="tomato-grid vpn-status-table" id="'+t+'-status-routing-table"><\/div><br><\/div>');
			W('<div id="'+t+'-status-stats"><div class="section-title">General Statistics<\/div><div class="tomato-grid vpn-status-table" id="'+t+'-status-stats-table"><\/div><br><\/div>');
			W('<\/div>');
			W('<\/div>');
			W('<div class="vpn-start-stop"><input type="button" value="" onclick="" id="_vpn'+t+'_button">&nbsp; <img src="spin.gif" alt="" id="spin'+(i+1)+'"><\/div>');
			W('<\/div>');
		}
	</script>
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
