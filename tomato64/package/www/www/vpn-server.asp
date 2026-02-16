<!DOCTYPE html>
<!--
	Tomato GUI
	Copyright (C) 2006-2008 Jonathan Zarate
	http://www.polarcloud.com/tomato/

	Portions Copyright (C) 2008-2010 Keith Moyer, tomatovpn@keithmoyer.com
	Copyright (C) 2018 - 2026 pedro https://freshtomato.org/

	For use with Tomato Firmware only.
	No part of this file may be used without permission.
-->
<html lang="en-GB">
<head>
<meta http-equiv="content-type" content="text/html;charset=utf-8">
<meta name="robots" content="noindex,nofollow">
<title>[<% ident(); %>] OpenVPN: Server</title>
<link rel="stylesheet" type="text/css" href="tomato.css?rel=<% version(); %>">
<% css(); %>
<script src="isup.jsx?_http_id=<% nv(http_id); %>"></script>
<script src="tomato.js?rel=<% version(); %>"></script>
<script src="vpn.js?rel=<% version(); %>"></script>

<script>

/* TOMATO64-REMOVE-BEGIN */
//	<% nvram("vpns_eas,vpns_dns,vpns1_poll,vpns1_if,vpns1_proto,vpns1_port,vpns1_firewall,vpns1_sn,vpns1_nm,vpns1_local,vpns1_remote,vpns1_dhcp,vpns1_r1,vpns1_r2,vpns1_crypt,vpns1_comp,vpns1_digest,vpns1_cipher,vpns1_ncp_ciphers,vpns1_reneg,vpns1_hmac,vpns1_plan,vpns1_ccd,vpns1_c2c,vpns1_ccd_excl,vpns1_ccd_val,vpns1_pdns,vpns1_rgw,vpns1_userpass,vpns1_nocert,vpns1_users_val,vpns1_custom,vpns1_static,vpns1_ca,vpns1_ca_key,vpns1_crt,vpns1_crl,vpns1_key,vpns1_dh,vpns1_br,vpns1_ecdh,vpns2_poll,vpns2_if,vpns2_proto,vpns2_port,vpns2_firewall,vpns2_sn,vpns2_nm,vpns2_local,vpns2_remote,vpns2_dhcp,vpns2_r1,vpns2_r2,vpns2_crypt,vpns2_comp,vpns2_digest,vpns2_cipher,vpns2_ncp_ciphers,vpns2_reneg,vpns2_hmac,vpns2_plan,vpns2_ccd,vpns2_c2c,vpns2_ccd_excl,vpns2_ccd_val,vpns2_pdns,vpns2_rgw,vpns2_userpass,vpns2_nocert,vpns2_users_val,vpns2_custom,vpns2_static,vpns2_ca,vpns2_ca_key,vpns2_crt,vpns2_crl,vpns2_key,vpns2_dh,vpns2_br,vpns2_ecdh,lan_ifname"); %>
/* TOMATO64-REMOVE-END */
/* TOMATO64-BEGIN */
//	<% nvram("vpns_eas,vpns_dns,vpns1_poll,vpns1_if,vpns1_proto,vpns1_port,vpns1_firewall,vpns1_sn,vpns1_nm,vpns1_local,vpns1_remote,vpns1_dhcp,vpns1_r1,vpns1_r2,vpns1_crypt,vpns1_comp,vpns1_digest,vpns1_cipher,vpns1_ncp_ciphers,vpns1_reneg,vpns1_hmac,vpns1_plan,vpns1_ccd,vpns1_c2c,vpns1_ccd_excl,vpns1_ccd_val,vpns1_pdns,vpns1_rgw,vpns1_userpass,vpns1_nocert,vpns1_users_val,vpns1_custom,vpns1_static,vpns1_ca,vpns1_ca_key,vpns1_crt,vpns1_crl,vpns1_key,vpns1_dh,vpns1_br,vpns1_ecdh,vpns2_poll,vpns2_if,vpns2_proto,vpns2_port,vpns2_firewall,vpns2_sn,vpns2_nm,vpns2_local,vpns2_remote,vpns2_dhcp,vpns2_r1,vpns2_r2,vpns2_crypt,vpns2_comp,vpns2_digest,vpns2_cipher,vpns2_ncp_ciphers,vpns2_reneg,vpns2_hmac,vpns2_plan,vpns2_ccd,vpns2_c2c,vpns2_ccd_excl,vpns2_ccd_val,vpns2_pdns,vpns2_rgw,vpns2_userpass,vpns2_nocert,vpns2_users_val,vpns2_custom,vpns2_static,vpns2_ca,vpns2_ca_key,vpns2_crt,vpns2_crl,vpns2_key,vpns2_dh,vpns2_br,vpns2_ecdh,vpns3_poll,vpns3_if,vpns3_proto,vpns3_port,vpns3_firewall,vpns3_sn,vpns3_nm,vpns3_local,vpns3_remote,vpns3_dhcp,vpns3_r1,vpns3_r2,vpns3_crypt,vpns3_comp,vpns3_digest,vpns3_cipher,vpns3_ncp_ciphers,vpns3_reneg,vpns3_hmac,vpns3_plan,vpns3_ccd,vpns3_c2c,vpns3_ccd_excl,vpns3_ccd_val,vpns3_pdns,vpns3_rgw,vpns3_userpass,vpns3_nocert,vpns3_users_val,vpns3_custom,vpns3_static,vpns3_ca,vpns3_ca_key,vpns3_crt,vpns3_crl,vpns3_key,vpns3_dh,vpns3_br,vpns3_ecdh,vpns4_poll,vpns4_if,vpns4_proto,vpns4_port,vpns4_firewall,vpns4_sn,vpns4_nm,vpns4_local,vpns4_remote,vpns4_dhcp,vpns4_r1,vpns4_r2,vpns4_crypt,vpns4_comp,vpns4_digest,vpns4_cipher,vpns4_ncp_ciphers,vpns4_reneg,vpns4_hmac,vpns4_plan,vpns4_ccd,vpns4_c2c,vpns4_ccd_excl,vpns4_ccd_val,vpns4_pdns,vpns4_rgw,vpns4_userpass,vpns4_nocert,vpns4_users_val,vpns4_custom,vpns4_static,vpns4_ca,vpns4_ca_key,vpns4_crt,vpns4_crl,vpns4_key,vpns4_dh,vpns4_br,vpns4_ecdh,vpns1_dco,vpns2_dco,vpns3_dco,vpns4_dco,lan_ifname"); %>
/* TOMATO64-END */

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
	tabs.push(['vpns'+i,'<span id="'+serviceType+i+'_tabicon" style="font-size:9px">▽ <\/span><span class="tabname">Server '+i+'<\/span>']);
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
/* KEYGEN-BEGIN */
var vpnciphers = vpnciphers.concat(['CAMELLIA-128-CBC'],['CAMELLIA-192-CBC'],['CAMELLIA-256-CBC']);
/* KEYGEN-END */
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

	for (var i = 0; i < tabs.length; ++i) {
		elem.display(tabs[i][0]+'-tab', (name == tabs[i][0]));
		elem.display(tabs[i][0]+'-tab-status-button', (name == tabs[i][0]));
	}

	cookie.set('vpn_vpns_tab', name);
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

	cookie.set('vpn_vpns'+tab+'_section', section);
}

function updateForm(num) {
	var fom = E('t_fom');

	if (isup['vpnserver'+num] && fom._service.value.indexOf('server'+num) < 0) {
		if (fom._service.value != '')
			fom._service.value += ',';

		fom._service.value += 'vpnserver'+num+'-restart';
	}
}

/* KEYGEN-BEGIN */
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
		elem.setInnerHTML('_'+t+'_usergen', (users_option ? users_option : '<option value="0">none<\/option>'));
	}
}

function v_serial(e, quiet) {
	if ((e = E(e)) == null)
		return 0;

	var v = e.value;

	if (!v.match(/[0-9A-Fa-f]{2}/)) {
		ferror.set(e, 'Invalid key serial number. Valid range: "01" - "FF"', quiet);
		return 0;
	}
	e.value = v.toUpperCase();
	ferror.clear(e);

	return 1;
}
/* KEYGEN-END */

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
		if (ccdTables[i] == this && E('_f_vpns'+(i + 1)+'_c2c').checked)
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
/* KEYGEN-BEGIN */
	rewriteUsers();
/* KEYGEN-END */
}

UsersGrid.prototype.fieldValuesToData = function(row) {
	var f = fields.getAll(row);

	for (var i = 0; i < tabs.length; ++i) {
		if (usersTables[i] == this)
			break;
	}
	var r = [(f[0].value ? ''+f[0].value+'' : ''+(usersTables[i].getDataCount() + 1)+''), (f[1].checked ? '1' : '0'), f[2].value, f[3].value];
/* KEYGEN-BEGIN */
	rewriteUsers(r, i);
/* KEYGEN-END */

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
		E('_vpns'+num+'_static').value = text;
		keyGenRequest = null;
		elem.display(E('server'+num+'_static_progress_div'), false);
		disableKeyButtons(num, false);
	}
	keyGenRequest.onError = function(ex) { keyGenRequest = null; }
	var crypt = E('_vpns'+num+'_crypt').value;
	var hmac = E('_vpns'+num+'_hmac').value;
	keyGenRequest.post('vpngenkey.cgi', '_mode='+((crypt == 'tls' && hmac == 4) ? 'static2' : 'static1')+'&_server='+num);
}

/* KEYGEN-BEGIN */
function generateDHParams(num) {
	if (keyGenRequest)
		return;

	if (confirm('WARNING: DH Parameters generation can take a long time.\nIf it freezes, refresh the page and try again.\n\nDo you want to proceed?')) {
		changed = 1;
		disableKeyButtons(num, true);
		elem.display(E('server'+num+'_dh_progress_div'), true);
		keyGenRequest = new XmlHttp();

		keyGenRequest.onCompleted = function(text, xml) {
			E('_f_vpns'+num+'_dh').value = text;
			keyGenRequest = null;
			elem.display(E('server'+num+'_dh_progress_div'), false);
			disableKeyButtons(num, false);
		}
		keyGenRequest.onError = function(ex) { keyGenRequest = null; }
		keyGenRequest.post('vpngenkey.cgi', '_mode=dh&_dhtype='+(E('_f_vpns'+num+'_dhtype').checked ? '1' : '0'));
	}
}

function generateKeys(num) {
	if (keyGenRequest)
		return;

	if (changed) {
		alert('Changes have been made. You need to save before continue!');
		return;
	}

	if (E('_vpns'+num+'_ca_key').value == '') {
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
		E('_vpns'+num+'_ca_key').value = cakey;
		E('_vpns'+num+'_ca').value = cacert;
		E('_vpns'+num+'_crt').value = generated_crt;
		E('_vpns'+num+'_key').value = generated_key;
		keyGenRequest = null;
		disableKeyButtons(num, false);
		showTLSProgressDivs(num, false);
	}
	keyGenRequest.onError = function(ex) { keyGenRequest = null; }
	keyGenRequest.post('vpngenkey.cgi', '_mode=key&_server='+num+'&_ecdh='+(E('_f_vpns'+num+'_ecdh').checked ? '1' : '0'));
}
/* KEYGEN-END */

function disableKeyButtons(num, state) {
	E('_vpn_keygen_static_vpns'+num+'_button').disabled = state;
/* KEYGEN-BEGIN */
	E('_vpn_keygen_vpns'+num+'_button').disabled = state;
	E('_vpn_dhgen_vpns'+num+'_button').disabled = state;
/* KEYGEN-END */
}

/* KEYGEN-BEGIN */
function showTLSProgressDivs(num, state) {
	elem.display(E('vpns'+num+'_key_progress_div'), state);
	elem.display(E('vpns'+num+'_cert_progress_div'), state);
	elem.display(E('vpns'+num+'_ca_progress_div'), state);
	elem.display(E('vpns'+num+'_ca_key_progress_div'), state);
}

function downloadClientConfig(num) {
	if (keyGenRequest)
		return;

	var warn = 0, userid = 0;
	var caKey = E('_vpns'+num+'_ca_key').value;
	var ca = E('_vpns'+num+'_ca').value;
	var serverCrt = E('_vpns'+num+'_crt').value;
	var serverCrtKey = E('_vpns'+num+'_key').value;
	var staticKey = E('_vpns'+num+'_static').value;
	var crypt = E('_vpns'+num+'_crypt').value;
	var hmac = E('_vpns'+num+'_hmac').value;
	var userpass = E('_f_vpns'+num+'_userpass').checked;
	var isUser = E('_vpns'+num+'_usergen').value;

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
			if (!v_serial('_vpns'+num+'_serial', 0))
				return;

			userid = parseInt(E('_vpns'+num+'_serial').value, 16);
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
	keyGenRequest.get('vpn/ClientConfig.tgz','_server='+num+(userid ? '&_userid='+userid : '')+'&_ecdh='+(E('_f_vpns'+num+'_ecdh').checked ? '1' : '0'));
}
/* KEYGEN-END */

function verifyFields(focused, quiet) {
	var i, j, t, ok = 1;
	var restart = 1;
	tgHideIcons();

	for (i = 1; i <= unitCount; ++i) {
		if (focused && focused == E('_f_vpns'+i+'_eas')) /* except on/off */
			restart = 0;
	}

	/* When settings change, make sure we restart the right services */
	if (focused) {
		changed = 1;

		var fom = E('t_fom');
		var serveridx = focused.name.indexOf('server');
		if (serveridx >= 0) {
			var num = focused.name.substring(serveridx + 6, serveridx + 7);

			if (focused.name.indexOf('_c2c') >= 0)
				ccdTables[num - 1].reDraw();

			if (restart) { /* except on/off */
				updateForm(num);

				if ((focused.name.indexOf('_dns') >= 0 || (focused.name.indexOf('_if') >= 0 && E('_f_vpns'+num+'_dns').checked)) && fom._service.value.indexOf('dnsmasq') < 0) {
					if (fom._service.value != '')
						fom._service.value += ',';

					fom._service.value += 'dnsmasq-restart';
				}
			}
		}
	}

	/* Element varification */
	for (i = 0; i < tabs.length; ++i) {
		t = tabs[i][0];

		if (!v_range('_'+t+'_poll', quiet || !ok, 0, 30))
			ok = 0;
		if (!v_port('_'+t+'_port', quiet || !ok))
			ok = 0;
		if (!v_ip('_'+t+'_sn', quiet || !ok, 0))
			ok = 0;
		if (!v_netmask('_'+t+'_nm', quiet || !ok))
			ok = 0;
		if (!v_ip('_'+t+'_r1', quiet || !ok, 1))
			ok = 0;
		if (!v_ip('_'+t+'_r2', quiet || !ok, 1))
			ok = 0;
		if (!v_ip('_'+t+'_local', quiet || !ok, 1))
			ok = 0;
		if (!v_ip('_'+t+'_remote', quiet || !ok, 1))
			ok = 0;
		if (!v_range('_'+t+'_reneg', quiet || !ok, -1, 2147483647))
			ok = 0;
/* KEYGEN-BEGIN */
		if (E('_'+t+'_crypt').value == 'tls' && !E('_f_'+t+'_userpass').checked && !v_serial('_'+t+'_serial', quiet || !ok))
			ok = 0;
/* KEYGEN-END */
	}

	/* Visibility changes */
	for (i = 0; i < tabs.length; ++i) {
		t = tabs[i][0];

/* SIZEOPTMORE-NO-BEGIN */
		if (E('_'+t+'_crypt').value == 'tls')
			E('_'+t+'_crypt').value = 'secret';
		E('_'+t+'_crypt').options[0].disabled = 1;
/* SIZEOPTMORE-NO-END */
		var auth = E('_'+t+'_crypt').value;
		var iface = E('_'+t+'_if').value;
		var hmac = E('_'+t+'_hmac').value;
		var dhcp = E('_f_'+t+'_dhcp').checked;
		var ccd = E('_f_'+t+'_ccd').checked;
		var userpass = E('_f_'+t+'_userpass').checked;
		var dns = E('_f_'+t+'_dns').checked;
/* SIZEOPTMORE-BEGIN */
		var comp = E('_'+t+'_comp').value;
/* SIZEOPTMORE-END */

		elem.display(PR('_'+t+'_ca'), PR('_'+t+'_ca_key'), PR('_'+t+'_ca_key_div_help'),
/* KEYGEN-BEGIN */
			     PR('_vpn_dhgen_'+t+'_button'),
/* KEYGEN-END */
			     PR('_'+t+'_crt'), PR('_'+t+'_crl'), PR('_f_'+t+'_dh'),
			     PR('_'+t+'_key'), PR('_'+t+'_hmac'), PR('_f_'+t+'_rgw'),
			     PR('_'+t+'_reneg'), auth == 'tls');
		elem.display(PR('_'+t+'_static'), auth == 'secret' || (auth == 'tls' && hmac >= 0));
		elem.display(PR('_vpn_keygen_static_'+t+'_button'), auth == 'secret' || (auth == 'tls' && hmac >= 0));
		elem.display(E(t+'_custom_crypto_text'), auth == 'custom');
/* KEYGEN-BEGIN */
		elem.display(PR('_vpn_keygen_'+t+'_button'), auth == 'tls');
/* KEYGEN-END */
		elem.display(PR('_'+t+'_sn'), PR('_f_'+t+'_plan'), PR('_f_'+t+'_plan1'),
/* TOMATO64-BEGIN */
		             PR('_f_'+t+'_plan4'), PR('_f_'+t+'_plan5'), PR('_f_'+t+'_plan6'), PR('_f_'+t+'_plan7'),
/* TOMATO64-END */
		             PR('_f_'+t+'_plan2'), PR('_f_'+t+'_plan3'), auth == 'tls' && iface == 'tun');
		elem.display(PR('_f_'+t+'_dhcp'), auth == 'tls' && iface == 'tap');
		elem.display(PR('_'+t+'_br'), iface == 'tap');
		elem.display(E(t+'_range'), !dhcp);
		elem.display(PR('_'+t+'_local'), auth == 'secret' && iface == 'tun');
		elem.display(PR('_f_'+t+'_ccd'), auth == 'tls');
		elem.display(PR('_f_'+t+'_userpass'), auth == 'tls');
		elem.display(PR('_f_'+t+'_nocert'), PR('table_'+t+'_users'), auth == 'tls' && userpass);
		elem.display(PR('_f_'+t+'_c2c'), PR('_f_'+t+'_ccd_excl'), PR('table_'+t+'_ccd'), auth == 'tls' && ccd);
		elem.display(PR('_f_'+t+'_pdns'), auth == 'tls' && dns );
		elem.display(PR('_'+t+'_ncp_ciphers'), auth == 'tls');
		elem.display(PR('_'+t+'_cipher'), auth == 'secret');
/* TOMATO64-BEGIN */
		var proto = E('_'+t+'_proto').value;
		var dco_vis = iface == 'tun' && auth == 'tls' && proto.indexOf('udp') >= 0;
		elem.display(PR('_f_'+t+'_dco'), dco_vis);
		var dco_checked = E('_f_'+t+'_dco').checked;
		if (dco_vis && dco_checked) {
			E('_'+t+'_comp').value = '-1';
			E('_'+t+'_comp').disabled = 1;
		} else {
			E('_'+t+'_comp').disabled = 0;
		}
/* TOMATO64-END */
/* KEYGEN-BEGIN */
		elem.display(PR('_vpn_client_gen_'+t+'_button'), auth != 'custom');
		elem.display(PR('_'+t+'_serial'), auth == 'tls' && !userpass);
		elem.display(PR('_'+t+'_usergen'), auth == 'tls' && userpass);

		if (E('_f_'+t+'_ecdh').checked) {
			E('_f_'+t+'_dh').disabled = 1;
			E('_vpn_dhgen_'+t+'_button').disabled = 1;
			E('_f_'+t+'_dhtype').disabled = 1;
		}
		else {
			E('_f_'+t+'_dh').disabled = 0;
			E('_vpn_dhgen_'+t+'_button').disabled = 0;
			E('_f_'+t+'_dhtype').disabled = 0;
		}
/* TOMATO64-BEGIN */
		E('_f_'+t+'_dhtype').checked = 1;
/* TOMATO64-END */
/* KEYGEN-END */

		var keyHelp = E(t+'-keyhelp');
		keyHelp.className = 'new_window';
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
		for (j = 0; j <= MAX_BRIDGE_ID; ++j) {
			t = (j == 0 ? '' : j);

			if (nvram['lan'+t+'_ifname'].length < 1) {
				E('_vpns'+(i + 1)+'_br').options[j].disabled = 1;
				/* also disable and un-check push lanX (*_plan) */
				E('_f_vpns'+(i + 1)+'_plan'+t).checked = 0;
				E('_f_vpns'+(i + 1)+'_plan'+t).disabled = 1;
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

	fom.vpns_eas.value = '';
	fom.vpns_dns.value = '';

	for (i = 0; i < tabs.length; ++i) {
		if (ccdTables[i].isEditing())
			return;
		if (usersTables[i].isEditing())
			return;

		t = tabs[i][0];

/* SIZEOPTMORE-BEGIN */
		var crypt = E('_'+t+'_crypt').value;
		var hmac = E('_'+t+'_hmac').value;
		var key = E('_'+t+'_static').value;
		if (key != '') {
			if (((crypt == 'secret' || (crypt == 'tls' && hmac >= 0 && hmac < 4)) && key.indexOf('OpenVPN Static key V1') === -1) ||
			    (crypt == 'tls' && hmac == 4 && key.indexOf('OpenVPN tls-crypt-v2') === -1)) {
				alert('Keys->Static Key is in the wrong format for the selected Auth Method - Re-Generate it!');
				return;
			}
		}
/* SIZEOPTMORE-END */

		if (E('_f_'+t+'_eas').checked)
			fom.vpns_eas.value += ''+(i + 1)+',';

		if (E('_f_'+t+'_dns').checked)
			fom.vpns_dns.value += ''+(i + 1)+',';

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
			n = u.indexOf('<');
			users += u.substring(n + 1);
		}

		fom[t+'_dhcp'].value = E('_f_'+t+'_dhcp').checked ? 1 : 0;
		fom[t+'_plan'].value = E('_f_'+t+'_plan').checked ? 1 : 0;
		fom[t+'_plan1'].value = E('_f_'+t+'_plan1').checked ? 1 : 0;
		fom[t+'_plan2'].value = E('_f_'+t+'_plan2').checked ? 1 : 0;
		fom[t+'_plan3'].value = E('_f_'+t+'_plan3').checked ? 1 : 0;
/* TOMATO64-BEGIN */
		fom[t+'_plan4'].value = E('_f_'+t+'_plan4').checked ? 1 : 0;
		fom[t+'_plan5'].value = E('_f_'+t+'_plan5').checked ? 1 : 0;
		fom[t+'_plan6'].value = E('_f_'+t+'_plan6').checked ? 1 : 0;
		fom[t+'_plan7'].value = E('_f_'+t+'_plan7').checked ? 1 : 0;
/* TOMATO64-END */
		fom[t+'_ccd'].value = E('_f_'+t+'_ccd').checked ? 1 : 0;
		fom[t+'_c2c'].value = E('_f_'+t+'_c2c').checked ? 1 : 0;
		fom[t+'_ccd_excl'].value = E('_f_'+t+'_ccd_excl').checked ? 1 : 0;
		fom[t+'_ccd_val'].value = ccd;
		fom[t+'_userpass'].value = E('_f_'+t+'_userpass').checked ? 1 : 0;
		fom[t+'_nocert'].value = E('_f_'+t+'_nocert').checked ? 1 : 0;
		fom[t+'_users_val'].value = users;
		fom[t+'_pdns'].value = E('_f_'+t+'_pdns').checked ? 1 : 0;
		fom[t+'_rgw'].value = E('_f_'+t+'_rgw').checked ? 1 : 0;
/* TOMATO64-BEGIN */
		fom[t+'_dco'].value = E('_f_'+t+'_dco').checked ? 1 : 0;
		if (E('_f_'+t+'_dco').checked) {
			if (E('_'+t+'_comp').value != '-1') {
				alert('DCO requires compression to be Disabled.');
				return;
			}
			var ciphers = E('_'+t+'_ncp_ciphers').value;
			if (ciphers != '') {
				var valid_ciphers = ['AES-128-GCM','AES-192-GCM','AES-256-GCM','CHACHA20-POLY1305'];
				var cipher_list = ciphers.split(':');
				for (var ci = 0; ci < cipher_list.length; ci++) {
					var c = cipher_list[ci].trim().toUpperCase();
					if (c != '' && valid_ciphers.indexOf(c) < 0) {
						alert('DCO only supports AEAD ciphers (AES-128-GCM, AES-256-GCM, CHACHA20-POLY1305).\nIncompatible cipher found: ' + cipher_list[ci].trim());
						return;
					}
				}
			}
		}
/* TOMATO64-END */
/* KEYGEN-BEGIN */
		var is_rsa = 0, is_ecdh = 0;
		var ca_key = E('_'+t+'_ca_key').value;

		if (E('_f_'+t+'_ecdh').checked && ca_key > '' && ca_key.indexOf('BEGIN EC') == -1)
			is_rsa = 1;

		if (!E('_f_'+t+'_ecdh').checked && ca_key > '' && ca_key.indexOf('BEGIN PRIVATE') == -1)
			is_ecdh = 1;

		if (is_rsa || is_ecdh) {
			if (!confirm('WARNING: Certificate Authority Key is in the wrong format! \n'+
			             'Maybe you just need to '+(is_rsa ? 'un' : '')+'check "use ECDH keys" or delete (and re-generate) "Certificate Authority Key" if it\'s in wrong format? \n'+
			             'Are you sure to continue?'))
				return;
		}

		fom[t+'_ecdh'].value = E('_f_'+t+'_ecdh').checked ? 1 : 0;
		if (E('_f_'+t+'_ecdh').checked)
			E('_f_'+t+'_dh').value = '';
/* KEYGEN-END */
		fom[t+'_dh'].value = E('_f_'+t+'_dh').value;
	}
	fom._nofootermsg.value = 0;

	form.submit(fom, 1);

	changed = 0;
	fom._service.value = '';
}

function earlyInit() {
	show();
	tabSelect(cookie.get('vpn_vpns_tab') || tabs[0][0]);

	for (var i = 0; i < tabs.length; ++i) {
		sectSelect(i, cookie.get('vpn_vpns'+i+'_section') || sections[0][0]);
		var t = tabs[i][0];

		ccdTables[i].init('table_'+t+'_ccd','sort',0,[{ type: 'text', maxlen: 2, attrib: 'hidden' },{ type: 'checkbox', prefix: '<div class="centered">', suffix: '<\/div>' },{ type: 'text', maxlen: 30 },{ type: 'text', maxlen: 15 },{ type: 'text', maxlen: 15 },{ type: 'checkbox', prefix: '<div class="centered">', suffix: '<\/div>' }]);
		ccdTables[i].headerSet(['#','Enable','Common Name','Subnet','Netmask','Push']);
		var ccdVal = nvram[t+'_ccd_val'];
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

		usersTables[i].init('table_'+t+'_users','sort',0,[ { type: 'text', maxlen: 2, attrib: 'hidden' },{ type: 'checkbox', prefix: '<div class="centered">', suffix: '<\/div>' },{ type: 'text', maxlen: 25 },{ type: 'password', peekaboo: 1, maxlen: 15 }]);
		usersTables[i].headerSet(['#','Enable','Username','Password']);
		var usersVal = nvram[t+'_users_val'];
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
	insOvl();
}

function init() {
/* KEYGEN-BEGIN */
	rewriteUsers();
/* KEYGEN-END */
	up.initPage(250, 5);
	eventHandler();
}
</script>
</head>

<body onload="init()">
<form id="t_fom" method="post" action="tomato.cgi">
<table id="container">
<tr><td colspan="2" id="header">
	<div class="title"><a href="/">Tomato64</a></div>
	<div class="version">Version <% version(); %> on <% nv("t_model_name"); %><span class="blinking bl2"><script><% anonupdate(); %> anon_update()</script>&nbsp;</span></div>
</td></tr>
<tr id="body"><td id="navi"><script>navi()</script></td>
<td id="content">
<div id="ident"><% ident(); %> | <script>wikiLink();</script></div>

<!-- / / / -->

<input type="hidden" name="_nextpage" value="vpn-server.asp">
<input type="hidden" name="_service" value="">
<input type="hidden" name="_nofootermsg">
<input type="hidden" name="vpns_eas">
<input type="hidden" name="vpns_dns">

<!-- / / / -->

<div class="section-title">Status</div>
<div class="section">
	<div class="fields">
		<script>
			for (i = 0; i < tabs.length; ++i) {
				t = tabs[i][0];

				W('<div id="'+t+'-tab-status-button">');
				W('<span id="_'+serviceType+(i + 1)+'_notice"><\/span>');
				W('<input type="button" id="_'+serviceType+(i + 1)+'_button">&nbsp; <img src="spin.svg" alt="" id="spin'+(i + 1)+'">');
				W('<\/div>');
			}
		</script>
	</div>
</div>

<!-- / / / -->

<div class="section-title"><span class="openvpnsvg">&nbsp;</span>OpenVPN Server Configuration</div>
<div class="section">
	<script>
		tabCreate.apply(this, tabs);

		for (i = 0; i < tabs.length; ++i) {
			t = tabs[i][0];
			W('<div id="'+t+'-tab">');
			W('<input type="hidden" name="'+t+'_dhcp">');
			W('<input type="hidden" name="'+t+'_plan">');
			W('<input type="hidden" name="'+t+'_plan1">');
			W('<input type="hidden" name="'+t+'_plan2">');
			W('<input type="hidden" name="'+t+'_plan3">');
/* TOMATO64-BEGIN */
			W('<input type="hidden" name="'+t+'_plan4">');
			W('<input type="hidden" name="'+t+'_plan5">');
			W('<input type="hidden" name="'+t+'_plan6">');
			W('<input type="hidden" name="'+t+'_plan7">');
/* TOMATO64-END */
			W('<input type="hidden" name="'+t+'_ccd">');
			W('<input type="hidden" name="'+t+'_c2c">');
			W('<input type="hidden" name="'+t+'_ccd_excl">');
			W('<input type="hidden" name="'+t+'_ccd_val">');
			W('<input type="hidden" name="'+t+'_userpass">');
			W('<input type="hidden" name="'+t+'_nocert">');
			W('<input type="hidden" name="'+t+'_users_val">');
			W('<input type="hidden" name="'+t+'_pdns">');
			W('<input type="hidden" name="'+t+'_rgw">');
			W('<input type="hidden" name="'+t+'_dh">');
/* KEYGEN-BEGIN */
			W('<input type="hidden" name="'+t+'_ecdh">');
/* KEYGEN-END */
/* TOMATO64-BEGIN */
			W('<input type="hidden" name="'+t+'_dco">');
/* TOMATO64-END */

			W('<ul class="tabs">');
			for (j = 0; j < sections.length; j++) {
				W('<li><a href="javascript:sectSelect('+i+',\''+sections[j][0]+'\')" id="'+t+'-'+sections[j][0]+'-tab">'+sections[j][1]+'<\/a><\/li>');
			}
			W('<\/ul><div class="tabs-bottom"><\/div>');

			W('<div id="'+t+'-basic">');
			createFieldTable('', [
				{ title: 'Enable on Start', name: 'f_'+t+'_eas', type: 'checkbox', value: nvram.vpns_eas.indexOf(''+(i + 1)) >= 0 },
				{ title: 'Interface Type', name: t+'_if', type: 'select', options: [['tap','TAP'],['tun','TUN']], value: nvram[t+'_if'] },
/* TOMATO64-REMOVE-BEGIN */
					{ title: 'Bridge TAP with', indent: 2, name: t+'_br', type: 'select', options: [['br0','LAN0 (br0)*'],['br1','LAN1 (br1)'],['br2','LAN2 (br2)'],['br3','LAN3 (br3)']],
/* TOMATO64-REMOVE-END */
/* TOMATO64-BEGIN */
					{ title: 'Bridge TAP with', indent: 2, name: t+'_br', type: 'select', options: [['br0','LAN0 (br0)*'],['br1','LAN1 (br1)'],['br2','LAN2 (br2)'],['br3','LAN3 (br3)'],
																['br4','LAN4 (br4)'],['br5','LAN5 (br5)'],['br6','LAN6 (br6)'],['br7','LAN7 (br7)']],
/* TOMATO64-END */
						value: nvram[t+'_br'], suffix: ' <small>* default<\/small> ' },
				{ title: 'Protocol', name: t+'_proto', type: 'select', options: [['udp','UDP'],['tcp-server','TCP'],['udp4','UDP4'],['tcp4-server','TCP4'],['udp6','UDP6'],['tcp6-server','TCP6']], value: nvram[t+'_proto'] },
				{ title: 'Port', name: t+'_port', type: 'text', maxlen: 5, size: 10, value: nvram[t+'_port'] },
				{ title: 'Firewall', name: t+'_firewall', type: 'select', options: [['auto','Automatic'],['external','External Only'],['custom','Custom']], value: nvram[t+'_firewall'] },
				{ title: 'Authorization Mode', name: t+'_crypt', type: 'select', options: [['tls','TLS'],['secret','Static Key'],['custom','Custom']], value: nvram[t+'_crypt'],
					suffix: ' <small id="'+t+'_custom_crypto_text">must be configured manually<\/small>' },
				{ title: 'TLS control channel security <small>(tls-auth/tls-crypt)<\/small>', name: t+'_hmac', type: 'select', options: [[-1,'Disabled'],[2,'Bi-directional Auth'],[0,'Incoming Auth (0)'],[1,'Outgoing Auth (1)'],[3,'Encrypt Channel']
/* SIZEOPTMORE-BEGIN */
				          ,[4,'Encrypt Channel V2']
/* SIZEOPTMORE-END */
				          ], value: nvram[t+'_hmac'] },
				{ title: 'Auth digest', name: t+'_digest', type: 'select', options: digests, value: nvram[t+'_digest'] },
				{ title: 'VPN subnet/netmask', multi: [
					{ name: t+'_sn', type: 'text', maxlen: 15, size: 17, value: nvram[t+'_sn'], suffix: ' ' },
					{ name: t+'_nm', type: 'text', maxlen: 15, size: 17, value: nvram[t+'_nm'] } ] },
				{ title: 'Client address pool', multi: [
					{ name: 'f_'+t+'_dhcp', type: 'checkbox', value: nvram[t+'_dhcp'] != 0, suffix: '&nbsp; DHCP ' },
					{ name: t+'_r1', type: 'text', maxlen: 15, size: 17, value: nvram[t+'_r1'], prefix: '<span id="'+t+'_range">', suffix: ' - ' },
					{ name: t+'_r2', type: 'text', maxlen: 15, size: 17, value: nvram[t+'_r2'], suffix: '<\/span>' } ] },
				{ title: 'Local/remote endpoint addresses', multi: [
					{ name: t+'_local', type: 'text', maxlen: 15, size: 17, value: nvram[t+'_local'], suffix: ' ' },
					{ name: t+'_remote', type: 'text', maxlen: 15, size: 17, value: nvram[t+'_remote'] } ] }
			]);
			W('<\/div>');

			W('<div id="'+t+'-advanced">');
			createFieldTable('', [
				null,
				{ title: 'Poll Interval', name: t+'_poll', type: 'text', maxlen: 2, size: 5, value: nvram[t+'_poll'], suffix: ' <small>minutes; 0 to disable<\/small>' },
				{ title: 'Push LAN0 (br0) to clients', name: 'f_'+t+'_plan', type: 'checkbox', value: nvram[t+'_plan'] != 0 },
				{ title: 'Push LAN1 (br1) to clients', name: 'f_'+t+'_plan1', type: 'checkbox', value: nvram[t+'_plan1'] != 0 },
				{ title: 'Push LAN2 (br2) to clients', name: 'f_'+t+'_plan2', type: 'checkbox', value: nvram[t+'_plan2'] != 0 },
				{ title: 'Push LAN3 (br3) to clients', name: 'f_'+t+'_plan3', type: 'checkbox', value: nvram[t+'_plan3'] != 0 },
/* TOMATO64-BEGIN */
				{ title: 'Push LAN4 (br4) to clients', name: 'f_'+t+'_plan4', type: 'checkbox', value: nvram[t+'_plan4'] != 0 },
				{ title: 'Push LAN5 (br5) to clients', name: 'f_'+t+'_plan5', type: 'checkbox', value: nvram[t+'_plan5'] != 0 },
				{ title: 'Push LAN6 (br6) to clients', name: 'f_'+t+'_plan6', type: 'checkbox', value: nvram[t+'_plan6'] != 0 },
				{ title: 'Push LAN7 (br7) to clients', name: 'f_'+t+'_plan7', type: 'checkbox', value: nvram[t+'_plan7'] != 0 },
/* TOMATO64-END */
				{ title: 'Direct clients to<br>redirect Internet traffic', name: 'f_'+t+'_rgw', type: 'checkbox', value: nvram[t+'_rgw'] != 0 },
				{ title: 'Respond to DNS', name: 'f_'+t+'_dns', type: 'checkbox', value: nvram.vpns_dns.indexOf(''+(i + 1)) >= 0 },
				{ title: 'Advertise DNS to clients', name: 'f_'+t+'_pdns', type: 'checkbox', value: nvram[t+'_pdns'] != 0 },
				{ title: 'Data ciphers', name: t+'_ncp_ciphers', type: 'text', size: 70, maxlen: 127, value: nvram[t+'_ncp_ciphers'] },
				{ title: 'Cipher', name: t+'_cipher', type: 'select', options: ciphers, value: nvram[t+'_cipher'] },
				{ title: 'Compression', name: t+'_comp', type: 'select', options: [['-1','Disabled'],['no','None']
/* SIZEOPTMORE-BEGIN */
				         ,['lz4','LZ4'],['lz4-v2','LZ4-V2']
/* SIZEOPTMORE-END */
				         ], value: nvram[t+'_comp'] },
				{ title: 'TLS Renegotiation Time', name: t+'_reneg', type: 'text', maxlen: 10, size: 7, value: nvram[t+'_reneg'], suffix: ' <small> seconds; -1 for default<\/small>' },
				{ title: 'Manage Client-Specific Options', name: 'f_'+t+'_ccd', type: 'checkbox', value: nvram[t+'_ccd'] != 0 },
				{ title: 'Allow Client<->Client', name: 'f_'+t+'_c2c', type: 'checkbox', value: nvram[t+'_c2c'] != 0 },
				{ title: 'Allow Only These Clients', name: 'f_'+t+'_ccd_excl', type: 'checkbox', value: nvram[t+'_ccd_excl'] != 0 },
				{ title: '', suffix: '<div class="tomato-grid" id="table_'+t+'_ccd"><\/div>' },
				{ title: 'Allow User/Pass Auth', name: 'f_'+t+'_userpass', type: 'checkbox', value: nvram[t+'_userpass'] != 0 },
				{ title: 'Allow Only User/Pass (without cert) Auth', name: 'f_'+t+'_nocert', type: 'checkbox', value: nvram[t+'_nocert'] != 0 },
				{ title: '', suffix: '<div class="tomato-grid" id="table_'+t+'_users"><\/div>' },
/* TOMATO64-BEGIN */
				{ title: 'Data Channel Offload (DCO)', name: 'f_'+t+'_dco', type: 'checkbox', value: nvram[t+'_dco'] != 0,
					suffix: ' <small>requires TUN, UDP, and AEAD ciphers<\/small>' },
/* TOMATO64-END */
				{ title: 'Custom Configuration', name: t+'_custom', type: 'textarea', value: nvram[t+'_custom'] }
			]);
			W('<\/div>');

			W('<div id="'+t+'-keys">');
			W('<p class="vpn-keyhelp">For help generating keys, refer to the OpenVPN <a id="'+t+'-keyhelp">HOWTO<\/a>. All 6 keys take about 14kB of NVRAM, so check first if there is enough free space!<\/p>');
			createFieldTable('', [
				null,
				{ title: 'Static Key', name: t+'_static', type: 'textarea', value: nvram[t+'_static'],
					prefix: '<div id="'+t+'_static_progress_div" style="display:none"><p class="keyhelp">Please wait - generating static key...<img src="spin.svg" alt=""><\/p><\/div>' },
				{ title: '', custom: '<input type="button" value="Generate static key" onclick="updateStaticKey('+(i + 1)+')" id="_vpn_keygen_static_'+t+'_button">' }
			]);
			createFieldTable('', [
				null,
				{ title: 'Certificate Authority Key', name: t+'_ca_key', type: 'textarea', value: nvram[t+'_ca_key']
/* KEYGEN-BEGIN */
					, prefix: '<div id="'+t+'_ca_key_progress_div" style="display:none"><p class="keyhelp">Please wait - generating CA key...<img src="spin.svg" alt=""><\/p><\/div>'
/* KEYGEN-END */
				},
				{ title: '', custom: '<div id="_'+t+'_ca_key_div_help"><p class="keyhelp">Optional, only used for client certificate generation.<br> Unencrypted (-noenc) private keys are supported.<\/p><\/div>' },
				{ title: 'Certificate Authority', name: t+'_ca', type: 'textarea', value: nvram[t+'_ca'],
					prefix: '<div id="'+t+'_ca_progress_div" style="display:none"><p class="keyhelp">Please wait - generating CA certificate...<img src="spin.svg" alt=""><\/p><\/div>' },
				{ title: 'Server Certificate', name: t+'_crt', type: 'textarea', value: nvram[t+'_crt']
/* KEYGEN-BEGIN */
					, prefix: '<div id="'+t+'_cert_progress_div" style="display:none"><p class="keyhelp">Please wait - generating certificate...<img src="spin.svg" alt=""><\/p><\/div>'
/* KEYGEN-END */
				},
				{ title: 'Server Key', name: t+'_key', type: 'textarea', value: nvram[t+'_key']
/* KEYGEN-BEGIN */
					, prefix: '<div id="'+t+'_key_progress_div" style="display:none"><p class="keyhelp">Please wait - generating key...<img src="spin.svg" alt=""><\/p><\/div>'
/* KEYGEN-END */
				},
				{ title: 'CRL file', name: t+'_crl', type: 'textarea', value: nvram[t+'_crl'] }
/* KEYGEN-BEGIN */
				, { title: '', multi: [
					{ custom: '<input type="button" value="Generate keys" onclick="generateKeys('+(i + 1)+')" id="_vpn_keygen_'+t+'_button">', suffix: '&nbsp; &nbsp;' },
					{ name: 'f_'+t+'_ecdh', type: 'checkbox', value: (nvram[t+'_ecdh'] != 0 || nvram[t+'_ca_key'] == ''), suffix: '&nbsp; <small>use <a href="https://en.wikipedia.org/wiki/Elliptic-curve_Diffie%E2%80%93Hellman" class="new_window">ECDH keys<\/a><\/small>' } ] }
/* KEYGEN-END */
			]);
			createFieldTable('', [
				null,
				{ title: 'Diffie-Hellman parameters', name: 'f_'+t+'_dh', type: 'textarea', value: nvram[t+'_dh']
/* KEYGEN-BEGIN */
					, prefix: '<div id="'+t+'_dh_progress_div" style="display:none"><p class="keyhelp">Please wait - generating DH parameters...<img src="spin.svg" alt=""><\/p><\/div>' },
				{ title: '', multi: [
					{ custom: '<input type="button" value="Generate DH Params" onclick="generateDHParams('+(i + 1)+')" id="_vpn_dhgen_'+t+'_button">', suffix: '&nbsp; &nbsp;' },
					{ name: 'f_'+t+'_dhtype', type: 'checkbox', value: 0, suffix: '&nbsp; <small>use 2048 instead of 1024 bytes. Warning! It may take a very long time!<\/small>' } ] }
			]);
			createFieldTable('', [
				null,
				{ title: 'Serial number', custom: '<input type="text" name="'+t+'_serial" value="01" maxlength="2" size="2" id="_'+t+'_serial">', suffix: '&nbsp; <small>in hex (01 - FF)<\/small>' },
				{ title: 'User', custom: '<select name="'+t+'_usergen" id="_'+t+'_usergen"><\/select>' },
				{ title: '', custom: '<input type="button" value="Generate client config" onclick="downloadClientConfig('+(i + 1)+')" id="_vpn_client_gen_'+t+'_button">',
					suffix: '<div id="'+t+'_gen_progress_div" style="display:none"><p class="keyhelp">Please wait while the configuration is being generated...<img src="spin.svg" alt=""><\/p><\/div>'
/* KEYGEN-END */
				}
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
			W('<\/div>');
		}
	</script>
</div>

<!-- / / / -->

<div id="footer">
	<span id="footer-msg"></span>
	<input type="button" value="Save" id="save-button" onclick="save()">
	<input type="button" value="Cancel" id="cancel-button" onclick="reloadPage()">
</div>

</td></tr>
</table>
</form>
<script>earlyInit();</script>
</body>
</html>
