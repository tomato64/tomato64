<!DOCTYPE html>
<!--
	Tomato GUI
	Copyright (C) 2006-2008 Jonathan Zarate
	http://www.polarcloud.com/tomato/

	Portions Copyright (C) 2008-2010 Keith Moyer, tomatovpn@keithmoyer.com
	Portions Copyright (C) 2010-2011 Jean-Yves Avenard, jean-yves@avenard.org
	Copyright (C) 2018 - 2026 pedro https://freshtomato.org/

	For use with Tomato Firmware only.
	No part of this file may be used without permission.
-->
<html lang="en-GB">
<head>
<meta http-equiv="content-type" content="text/html;charset=utf-8">
<meta name="robots" content="noindex,nofollow">
<title>[<% ident(); %>] OpenVPN: Client</title>
<link rel="stylesheet" type="text/css" href="tomato.css?rel=<% version(); %>">
<% css(); %>
<script src="isup.jsx?_http_id=<% nv(http_id); %>"></script>
<script src="tomato.js?rel=<% version(); %>"></script>
<script src="vpn.js?rel=<% version(); %>"></script>

<script>

/* TOMATO64-REMOVE-BEGIN */
//	<% nvram("vpnc_eas,vpnc1_poll,vpnc1_tchk,vpnc1_if,vpnc1_bridge,vpnc1_nat,vpnc1_proto,vpnc1_addr,vpnc1_port,vpnc1_retry,vpnc1_firewall,vpnc1_crypt,vpnc1_comp,vpnc1_cipher,vpnc1_ncp_ciphers,vpnc1_local,vpnc1_remote,vpnc1_nm,vpnc1_reneg,vpnc1_hmac,vpnc1_adns,vpnc1_rgw,vpnc1_gw,vpnc1_custom,vpnc1_static,vpnc1_ca,vpnc1_crt,vpnc1_key,vpnc1_userauth,vpnc1_username,vpnc1_password,vpnc1_useronly,vpnc1_tlsremote,vpnc1_cn,vpnc1_br,vpnc1_digest,vpnc1_routing_val,vpnc1_fw,vpnc1_tlsvername,vpnc1_prio,vpnc2_poll,vpnc2_tchk,vpnc2_if,vpnc2_bridge,vpnc2_nat,vpnc2_proto,vpnc2_addr,vpnc2_port,vpnc2_retry,vpnc2_firewall,vpnc2_crypt,vpnc2_comp,vpnc2_cipher,vpnc2_ncp_ciphers,vpnc2_local,vpnc2_remote,vpnc2_nm,vpnc2_reneg,vpnc2_hmac,vpnc2_adns,vpnc2_rgw,vpnc2_gw,vpnc2_custom,vpnc2_static,vpnc2_ca,vpnc2_crt,vpnc2_key,vpnc2_userauth,vpnc2_username,vpnc2_password,vpnc2_useronly,vpnc2_tlsremote,vpnc2_cn,vpnc2_br,vpnc2_digest,vpnc2_routing_val,vpnc2_fw,vpnc2_tlsvername,vpnc2_prio,vpnc3_poll,vpnc3_tchk,vpnc3_if,vpnc3_bridge,vpnc3_nat,vpnc3_proto,vpnc3_addr,vpnc3_port,vpnc3_retry,vpnc3_firewall,vpnc3_crypt,vpnc3_comp,vpnc3_cipher,vpnc3_ncp_ciphers,vpnc3_local,vpnc3_remote,vpnc3_nm,vpnc3_reneg,vpnc3_hmac,vpnc3_adns,vpnc3_rgw,vpnc3_gw,vpnc3_custom,vpnc3_static,vpnc3_ca,vpnc3_crt,vpnc3_key,vpnc3_userauth,vpnc3_username,vpnc3_password,vpnc3_useronly,vpnc3_tlsremote,vpnc3_cn,vpnc3_br,vpnc3_digest,vpnc3_routing_val,vpnc3_fw,vpnc3_tlsvername,vpnc3_prio,lan_ifname"); %>
/* TOMATO64-REMOVE-END */
/* TOMATO64-BEGIN */
//	<% nvram("vpnc_eas,vpnc1_poll,vpnc1_tchk,vpnc1_if,vpnc1_bridge,vpnc1_nat,vpnc1_proto,vpnc1_addr,vpnc1_port,vpnc1_retry,vpnc1_firewall,vpnc1_crypt,vpnc1_comp,vpnc1_cipher,vpnc1_ncp_ciphers,vpnc1_local,vpnc1_remote,vpnc1_nm,vpnc1_reneg,vpnc1_hmac,vpnc1_adns,vpnc1_rgw,vpnc1_gw,vpnc1_custom,vpnc1_static,vpnc1_ca,vpnc1_crt,vpnc1_key,vpnc1_userauth,vpnc1_username,vpnc1_password,vpnc1_useronly,vpnc1_tlsremote,vpnc1_cn,vpnc1_br,vpnc1_digest,vpnc1_routing_val,vpnc1_fw,vpnc1_tlsvername,vpnc1_prio,vpnc2_poll,vpnc2_tchk,vpnc2_if,vpnc2_bridge,vpnc2_nat,vpnc2_proto,vpnc2_addr,vpnc2_port,vpnc2_retry,vpnc2_firewall,vpnc2_crypt,vpnc2_comp,vpnc2_cipher,vpnc2_ncp_ciphers,vpnc2_local,vpnc2_remote,vpnc2_nm,vpnc2_reneg,vpnc2_hmac,vpnc2_adns,vpnc2_rgw,vpnc2_gw,vpnc2_custom,vpnc2_static,vpnc2_ca,vpnc2_crt,vpnc2_key,vpnc2_userauth,vpnc2_username,vpnc2_password,vpnc2_useronly,vpnc2_tlsremote,vpnc2_cn,vpnc2_br,vpnc2_digest,vpnc2_routing_val,vpnc2_fw,vpnc2_tlsvername,vpnc2_prio,vpnc3_poll,vpnc3_tchk,vpnc3_if,vpnc3_bridge,vpnc3_nat,vpnc3_proto,vpnc3_addr,vpnc3_port,vpnc3_retry,vpnc3_firewall,vpnc3_crypt,vpnc3_comp,vpnc3_cipher,vpnc3_ncp_ciphers,vpnc3_local,vpnc3_remote,vpnc3_nm,vpnc3_reneg,vpnc3_hmac,vpnc3_adns,vpnc3_rgw,vpnc3_gw,vpnc3_custom,vpnc3_static,vpnc3_ca,vpnc3_crt,vpnc3_key,vpnc3_userauth,vpnc3_username,vpnc3_password,vpnc3_useronly,vpnc3_tlsremote,vpnc3_cn,vpnc3_br,vpnc3_digest,vpnc3_routing_val,vpnc3_fw,vpnc3_tlsvername,vpnc3_prio,vpnc1_dco,vpnc2_dco,vpnc3_dco,lan_ifname"); %>
/* TOMATO64-END */

var changed = 0, i;
var unitCount = OVPN_CLIENT_COUNT;
var serviceType = 'vpnclient';
for (i = 1; i <= unitCount; i++) serviceLastUp.push('0');

function RouteGrid() {return this;}
RouteGrid.prototype = new TomatoGrid;

var tabs =  [];
for (i = 1; i <= unitCount; ++i)
	tabs.push(['vpnc'+i,'<span id="'+serviceType+i+'_tabicon" style="font-size:9px">▽ <\/span><span class="tabname">Client '+i+'<\/span>']);
var sections = [['basic','Basic'],['advanced','Advanced'],['keys','Keys'],['policy','Routing Policy'],['status','Status']];

var routingTables = [];
var statusUpdaters = [];
for (i = 0; i < tabs.length; ++i) {
	statusUpdaters.push(new StatusUpdater());
	routingTables.push(new RouteGrid());
}

var ciphers = [['default','Use Default'],['none','None']];
for (i = 0; i < vpnciphers.length; ++i)
	ciphers.push([vpnciphers[i],vpnciphers[i]]);

var vpndigests = vpndigests.concat(['DSA','DSA-SHA','DSA-SHA1','DSA-SHA1-old','ecdsa-with-SHA1','RSA-MD5','RSA-RIPEMD160','RSA-SHA','RSA-SHA1','RSA-SHA1-2','RSA-SHA224','RSA-SHA256','RSA-SHA384','RSA-SHA512','SHA']);
/* KEYGEN-BEGIN */
var vpndigests = vpndigests.concat(['MD4']);
/* KEYGEN-END */
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

	xob.post('vpnstatus.cgi', 'client='+(num + 1));
}

function tabSelect(name) {
	tgHideIcons();
	tabHigh(name);

	for (var i = 0; i < tabs.length; ++i) {
		var on = (name == tabs[i][0]);
		elem.display(tabs[i][0]+'-tab', on);
		elem.display(tabs[i][0]+'-tab-status-button', on);
	}

	cookie.set('vpn_vpnc_tab', name);
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

	cookie.set('vpn_vpnc'+tab+'_section', section);
}

function updateForm(num, fw) {
	var fom = E('t_fom');

	if (fw && fom._service.value.indexOf('firewall') < 0) {
		if (fom._service.value != '')
			fom._service.value += ',';

		fom._service.value += 'firewall-restart';
	}

	if (isup['vpnclient'+num] && fom._service.value.indexOf('client'+num) < 0) {
		if (fom._service.value != '')
			fom._service.value += ',';

		fom._service.value += 'vpnclient'+num+'-restart';
	}
}

RouteGrid.prototype.fieldValuesToData = function(row) {
	var f = fields.getAll(row);

	return [(f[0].checked ? 1 : 0), f[1].value, f[2].value,f[3].checked?1:0];
}

RouteGrid.prototype.dataToView = function(data) {
	var temp = ['<input type="checkbox" disabled'+(data[0] != 0 ? ' checked' : '')+'>',['From Source IP','To Destination IP','To Domain'][data[1] - 1],data[2],'<input type="checkbox" disabled'+(data[3] != 0 ? ' checked' : '')+'>'];
	var v = [];
	for (var i = 0; i < temp.length; ++i)
		v.push(((i == 0) || (i == 3)) ? temp[i] : escapeHTML(''+temp[i]));

	return v;
}

RouteGrid.prototype.dataToFieldValues = function(data) {
	return [data[0] == 1, data[1], data[2],data[3] == 1];
}

RouteGrid.prototype.rpDel = function(row) {
	changed = 1;
	row = PR(row);
	TGO(row).moving = null;
	row.parentNode.removeChild(row);
	this.recolor();
	this.rpHide();

	for (i = 0; i < tabs.length; ++i) {
		if (routingTables[i] == this)
			updateForm(i + 1, 1);
	}
}

RouteGrid.prototype.verifyFields = function(row, quiet) {
	changed = 1;
	var ok = 1;
	var clientnum = 1;
	for (i = 0; i < tabs.length; ++i) {
		if (routingTables[i] == this)
			updateForm(i + 1, 1);
	}
	var f = fields.getAll(row);

	f[2].value = f[2].value.trim();
	ferror.clear(f[2]);

	/* Verify fields in this row of the table */
	if (f[2].value == '') {
		ferror.set(f[2], 'Value is mandatory', quiet || !ok);
		ok = 0;
	}
	else if (f[2].value.indexOf('>') >= 0 || f[2].value.indexOf('<') >= 0) {
		ferror.set(f[2], 'Value cannot contain "<" or ">" characters', quiet || !ok);
		ok = 0;
	}
	else if (f[2].value.indexOf(' ') >= 0 || f[2].value.indexOf(',') >= 0 || f[2].value.indexOf('\t') >= 0 || f[2].value.indexOf(':') >= 0) {
		ferror.set(f[2], 'Value cannot contain "space", "tab", ":" or "," characters. Only one IP or Domain per entry', quiet || !ok);
		ok = 0;
	}
	else if (f[2].value.indexOf('-') >= 0 && f[1].value == 2) {
		ferror.set(f[2], 'Value cannot contain "-" character. IP range is not supported', quiet || !ok);
		ok = 0;
	}
	else
		ferror.clear(f[2]);

	if (f[1].value != 3 && (!v_iptaddr(f[2], quiet || !ok)))
		ok = 0;

	return ok;
}

function verifyFields(focused, quiet) {
	var i, ok = 1;
	var restart = 1;
	tgHideIcons();

	for (i = 1; i <= unitCount; ++i) {
		if (focused && focused == E('_f_vpnc'+i+'_eas')) /* except on/off */
			restart = 0;
	}

	/* When settings change, make sure we restart the right client */
	if (focused) {
		changed = 1;

		var clientidx = focused.name.indexOf('vpnc');
		if (clientidx >= 0) {
			var clientnum = focused.name.substring(clientidx + 4, clientidx + 5);
			var stripped = focused.name.substring(0, clientidx + 4) + focused.name.substring(clientidx + 5);

			if (stripped == 'vpnc_local')
				E('_f_vpnc'+clientnum+'_local').value = focused.value;
			else if (stripped == 'f_vpnc_local')
				E('_vpnc'+clientnum+'_local').value = focused.value;

			if (restart) { /* except on/off */
				/* check if we need to restart firewall */
				if ((nvram['vpnc'+clientnum+'_rgw'] < 2 && E('_vpnc'+clientnum+'_rgw').value > 1) || (nvram['vpnc'+clientnum+'_rgw'] > 1 && E('_vpnc'+clientnum+'_rgw').value < 2))
					updateForm(clientnum, 1);
				else
					updateForm(clientnum, 0);
			}
		}
	}

	/* Element verification */
	for (i = 0; i < tabs.length; ++i) {
		var t = tabs[i][0];

		if (!v_range('_'+t+'_poll', quiet || !ok, 0, 30))
			ok = 0;
		if (!v_ip('_'+t+'_addr', true) && !v_domain('_'+t+'_addr', true)) {
			ferror.set(E('_'+t+'_addr'), 'Invalid server address', quiet || !ok);
			ok = 0;
		}
		else
			ferror.clear(E('_'+t+'_addr'));

		/* verify priority */
		var priority = E('_'+t+'_prio');
		if (priority.value != '' && !v_range('_'+t+'_prio', quiet || !ok, 1, 32766)) {
			ferror.set(priority, 'The priority must be in the range 1 - 32766', quiet || !ok);
			ok = 0;
		}
		else
			ferror.clear(priority);

		if (!v_port('_'+t+'_port', quiet || !ok))
			ok = 0;
		if (!v_ip('_'+t+'_local', quiet || !ok, 1))
			ok = 0;
		if (!v_ip('_f_'+t+'_local', true, 1))
			ok = 0;
		if (!v_ip('_'+t+'_remote', quiet || !ok, 1))
			ok = 0;
		if (!v_netmask('_'+t+'_nm', quiet || !ok))
			ok = 0;
		if (!v_range('_'+t+'_retry', quiet || !ok, -1, 32767))
			ok = 0;
		if (!v_range('_'+t+'_reneg', quiet || !ok, -1, 2147483647))
			ok = 0;
		if (E('_'+t+'_gw').value.length > 0 && !v_ip('_'+t+'_gw', quiet || !ok, 1))
			ok = 0;
	}

	/* Visibility changes */
	for (i = 0; i < tabs.length; ++i) {
		t = tabs[i][0];

		var fw = E('_'+t+'_firewall').value;
		var auth = E('_'+t+'_crypt').value;
		var iface = E('_'+t+'_if').value;
		var bridge = E('_f_'+t+'_bridge').checked;
		var nat = E('_f_'+t+'_nat').checked;
		var hmac = E('_'+t+'_hmac').value;
		var rgw = E('_'+t+'_rgw').value;

		E('_'+t+'_rgw').options[2].disabled = (iface == 'tap');
		E('_'+t+'_rgw').options[3].disabled = (iface == 'tap');
		if (iface == 'tap' && rgw >= 2) {
			rgw = 1;
			E('_'+t+'_rgw').value = 1;
		}

		var rtable = (iface == 'tun' && rgw >= 2);
		var userauth =  E('_f_'+t+'_userauth').checked && auth == 'tls';
		var useronly = userauth && E('_f_'+t+'_useronly').checked;

		/* Page Basic */
		elem.display(PR('_f_'+t+'_userauth'), auth == 'tls');
		elem.display(PR('_'+t+'_username'), PR('_'+t+'_password'), userauth );
		elem.display(PR('_f_'+t+'_useronly'), userauth);
		elem.display(E(t+'_ca_warn_text'), useronly);
		elem.display(PR('_'+t+'_hmac'), auth == 'tls');
		elem.display(E(t+'_custom_crypto_text'), auth == 'custom');
		elem.display(PR('_f_'+t+'_bridge'), iface == 'tap');
		elem.display(PR('_'+t+'_br'), iface == 'tap' && bridge > 0);
		elem.display(E(t+'_bridge_warn_text'), !bridge);
		elem.display(PR('_f_'+t+'_nat'), fw != 'custom' && (iface == 'tun' || !bridge));
		elem.display(PR('_f_'+t+'_fw'), fw != 'custom' && (iface == 'tun' || !bridge));
		elem.display(E(t+'_nat_warn_text'), fw != 'custom' && (!nat || (auth == 'secret' && iface == 'tun')));
		elem.display(PR('_'+t+'_local'), iface == 'tun' && auth == 'secret');
		elem.display(PR('_f_'+t+'_local'), iface == 'tap' && !bridge && auth != 'custom');

		/* Page Advanced */
		elem.display(PR('_'+t+'_adns'), PR('_'+t+'_reneg'), auth == 'tls');
		elem.display(E(t+'_gateway'), iface == 'tap' && rgw > 0);
		elem.display(PR('_'+t+'_ncp_ciphers'), auth == 'tls');
		elem.display(PR('_'+t+'_cipher'), auth == 'secret');
		elem.display(PR('_'+t+'_prio'), rgw > 1);
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

		/* Page Routing Policy */
		elem.display(E('_'+t+'_routing_div_help'), !rtable);

		/* Page Key */
		elem.display(PR('_'+t+'_static'), auth == 'secret' || (auth == 'tls' && hmac >= 0));
		elem.display(PR('_'+t+'_ca'), auth == 'tls');
		elem.display(PR('_'+t+'_crt'), PR('_'+t+'_key'), auth == 'tls' && !useronly);
		elem.display(PR('_f_'+t+'_tlsremote'), auth == 'tls');
		elem.display(PR('_'+t+'_tlsvername'), auth == 'tls');
		elem.display(E('client_'+t+'_cn'), (auth == 'tls') && (E('_'+t+'_tlsvername').value > 0));

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
		for (var j = 0; j <= MAX_BRIDGE_ID; ++j) {
			t = (j == 0  ? '' : j);

			if (nvram['lan'+t+'_ifname'].length < 1)
				E('_vpnc'+(i + 1)+'_br').options[j].disabled = 1;
		}
	}

	return ok;
}

function save() {
	if (!verifyFields(null, 0))
		return;

	var fom = E('t_fom');

	fom.vpnc_eas.value = '';

	for (var i = 0; i < tabs.length; ++i) {
		if (routingTables[i].isEditing())
			return;

		var t = tabs[i][0];

		if (E('_f_'+t+'_eas').checked)
			fom.vpnc_eas.value += ''+(i + 1)+',';

		var routedata = routingTables[i].getAllData();
		var routing = '';
		for (j = 0; j < routedata.length; ++j)
			routing += routedata[j].join('<')+'>';

		fom[t+'_bridge'].value = E('_f_'+t+'_bridge').checked ? 1 : 0;
		fom[t+'_nat'].value = E('_f_'+t+'_nat').checked ? 1 : 0;
		fom[t+'_fw'].value = E('_f_'+t+'_fw').checked ? 1 : 0;
		fom[t+'_userauth'].value = E('_f_'+t+'_userauth').checked ? 1 : 0;
		fom[t+'_useronly'].value = E('_f_'+t+'_useronly').checked ? 1 : 0;
		fom[t+'_tlsremote'].value = E('_f_'+t+'_tlsremote').checked ? 1 : 0;
		fom[t+'_tchk'].value = E('_f_'+t+'_tchk').checked ? 1 : 0;
		fom[t+'_routing_val'].value = routing;
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

		nvram[t+'_rgw'] = E('_'+t+'_rgw').value;
	}
	fom._nofootermsg.value = 0;

	form.submit(fom, 1);

	changed = 0;
	fom._service.value = '';
}

function earlyInit() {
	var i, t, s, j, routingVal;

	show();
	tabSelect(cookie.get('vpn_vpnc_tab') || tabs[0][0]);

	for (i = 0; i < tabs.length; ++i) {
		sectSelect(i, cookie.get('vpn_vpnc'+i+'_section') || sections[i][0]);

		t = tabs[i][0];

		routingTables[i].init('table_'+t+'_routing','sort', 0,[ { type: 'checkbox', prefix: '<div class="centered">', suffix: '<\/div>' },
		                                                        { type: 'select', options: [[1,'From Source IP'],[2,'To Destination IP'],[3,'To Domain']] },
		                                                        { type: 'text', maxlen: 50 },
		                                                        { type: 'checkbox', prefix: '<div class="centered">', suffix: '<\/div>' }]);
		routingTables[i].headerSet(['Enable','Type','Value','Kill Switch']);
		routingVal = nvram[t+'_routing_val'];
		if (routingVal.length) {
			s = routingVal.split('>');
			for (j = 0; j < s.length; ++j) {
				if (!s[j].length)
					continue;

				row = s[j].split('<');
				if (row.length == 3)
					row[3] = 0;
				if (row.length == 4)
					routingTables[i].insertData(-1, row);
			}
		}
		routingTables[i].showNewEditor();
		routingTables[i].resetNewEditor();

		statusUpdaters[i].init(null,null,t+'-status-stats-table',t+'-status-time',t+'-status-content',t+'-no-status');
	}

	verifyFields(null, 1);
	insOvl();
}

function init() {
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

<input type="hidden" name="_nextpage" value="vpn-client.asp">
<input type="hidden" name="_service" value="">
<input type="hidden" name="_nofootermsg">
<input type="hidden" name="vpnc_eas">

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

<div class="section-title"><span class="openvpnsvg">&nbsp;</span>OpenVPN Client Configuration</div>
<div class="section">
	<script>
		tabCreate.apply(this, tabs);

		for (i = 0; i < tabs.length; ++i) {
			t = tabs[i][0];
			W('<div id="'+t+'-tab">');
			W('<input type="hidden" name="'+t+'_nat">');
			W('<input type="hidden" name="'+t+'_fw">');
			W('<input type="hidden" name="'+t+'_userauth">');
			W('<input type="hidden" name="'+t+'_useronly">');
			W('<input type="hidden" name="'+t+'_bridge">');
			W('<input type="hidden" name="'+t+'_tlsremote">');
			W('<input type="hidden" name="'+t+'_tchk">');
			W('<input type="hidden" name="'+t+'_routing_val">');
/* TOMATO64-BEGIN */
			W('<input type="hidden" name="'+t+'_dco">');
/* TOMATO64-END */

			W('<ul class="tabs">');
			for (j = 0; j < sections.length; j++)
				W('<li><a href="javascript:sectSelect('+i+',\''+sections[j][0]+'\')" id="'+t+'-'+sections[j][0]+'-tab">'+sections[j][1]+'<\/a><\/li>');
			W('<\/ul><div class="tabs-bottom"><\/div>');

			W('<div id="'+t+'-basic">');
			createFieldTable('', [
				{ title: 'Enable on Start', name: 'f_'+t+'_eas', type: 'checkbox', value: nvram.vpnc_eas.indexOf(''+(i + 1)) >= 0 },
				{ title: 'Interface Type', name: t+'_if', type: 'select', options: [['tap','TAP'],['tun','TUN']], value: nvram[t+'_if'] },
/* TOMATO64-REMOVE-BEGIN */
					{ title: 'Bridge TAP with', indent: 2, name: t+'_br', type: 'select', options: [['br0','LAN (br0)*'],['br1','LAN1 (br1)'],['br2','LAN2 (br2)'],['br3','LAN3 (br3)']], value: nvram[t+'_br'], suffix: ' <small>* default<\/small>' },
/* TOMATO64-REMOVE-END */
/* TOMATO64-BEGIN */
					{ title: 'Bridge TAP with', indent: 2, name: t+'_br', type: 'select', options: [['br0','LAN (br0)*'],['br1','LAN1 (br1)'],['br2','LAN2 (br2)'],['br3','LAN3 (br3)'],['br4','LAN4 (br4)'],['br5','LAN5 (br5)'],['br6','LAN6 (br6)'],['br7','LAN7 (br7)']], value: nvram[t+'_br'], suffix: ' <small>* default<\/small>' },
/* TOMATO64-END */
				{ title: 'Protocol', name: t+'_proto', type: 'select', options: [['udp','UDP'],['tcp-client','TCP'],['udp4','UDP4'],['tcp4-client','TCP4'],['udp6','UDP6'],['tcp6-client','TCP6']], value: nvram[t+'_proto'] },
				{ title: 'Server Address/Port', multi: [
					{ name: t+'_addr', type: 'text', maxlen: 60, size: 17, value: nvram[t+'_addr'] },
					{ name: t+'_port', type: 'text', maxlen: 5, size: 7, value: nvram[t+'_port'] } ] },
				{ title: 'Firewall', name: t+'_firewall', type: 'select', options: [['auto','Automatic'],['custom','Custom']], value: nvram[t+'_firewall'] },
					{ title: 'Create NAT on tunnel', indent: 2, name: 'f_'+t+'_nat', type: 'checkbox', value: nvram[t+'_nat'] != 0, suffix: ' <small id="'+t+'_nat_warn_text">routes must be configured manually<\/small>' },
					{ title: 'Inbound Firewall', indent: 2, name: 'f_'+t+'_fw', type: 'checkbox', value: nvram[t+'_fw'] != 0 },
				{ title: 'Authorization Mode', name: t+'_crypt', type: 'select', options: [['tls','TLS'],['secret','Static Key'],['custom','Custom']], value: nvram[t+'_crypt'],
					suffix: ' <small id="'+t+'_custom_crypto_text">must be configured manually<\/small>' },
				{ title: 'TLS control channel security <small>(tls-auth/tls-crypt)<\/small>', name: t+'_hmac', type: 'select', options: [[-1,'Disabled'],[2,'Bi-directional Auth'],[0,'Incoming Auth (0)'],[1,'Outgoing Auth (1)'],[3,'Encrypt Channel']
/* SIZEOPTMORE-BEGIN */
				         ,[4,'Encrypt Channel V2']
/* SIZEOPTMORE-END */
				         ], value: nvram[t+'_hmac'] },
				{ title: 'Username/Password Authentication', name: 'f_'+t+'_userauth', type: 'checkbox', value: nvram[t+'_userauth'] != 0 },
					{ title: 'Username: ', indent: 2, name: t+'_username', type: 'text', maxlen: 50, size: 54, value: nvram[t+'_username'] },
					{ title: 'Password: ', indent: 2, name: t+'_password', type: 'password', maxlen: 70, size: 54, peekaboo:1, value: nvram[t+'_password'] },
					{ title: 'Username Authen. Only', indent: 2, name: 'f_'+t+'_useronly', type: 'checkbox', value: nvram[t+'_useronly'] != 0, suffix: ' <small id="'+t+'_ca_warn_text">warning: must define Certificate Authority<\/small>' },
				{ title: 'Auth digest', name: t+'_digest', type: 'select', options: digests, value: nvram[t+'_digest'] },
				{ title: 'Server is on the same subnet', name: 'f_'+t+'_bridge', type: 'checkbox', value: nvram[t+'_bridge'] != 0,
					suffix: ' <small style="color:red" id="'+t+'_bridge_warn_text">warning: cannot bridge distinct subnets. Defaulting to routed mode<\/small>' },
				{ title: 'Local/remote endpoint addresses', multi: [
					{ name: t+'_local', type: 'text', maxlen: 15, size: 17, value: nvram[t+'_local'] },
					{ name: t+'_remote', type: 'text', maxlen: 15, size: 17, value: nvram[t+'_remote'] } ] },
				{ title: 'Tunnel address/netmask', multi: [
					{ name: 'f_'+t+'_local', type: 'text', maxlen: 15, size: 17, value: nvram[t+'_local'] },
					{ name: t+'_nm', type: 'text', maxlen: 15, size: 17, value: nvram[t+'_nm'] } ] }
			]);
			W('<\/div>');

			W('<div id="'+t+'-advanced">');
			createFieldTable('', [
				{ title: 'Poll Interval', name: t+'_poll', type: 'text', maxlen: 2, size: 5, value: nvram[t+'_poll'], suffix: ' <small>minutes; 0 to disable<\/small>' },
					{ title: 'Also check out the tunnel', indent: 2, name: 'f_'+t+'_tchk', type: 'checkbox', value: nvram[t+'_tchk'] != 0, suffix: ' <small>does not work in all configurations<\/small>' },
				{ title: 'Redirect Internet traffic', multi: [
					{ name: t+'_rgw', type: 'select', options: [[0,'No'],[1,'All'],[2,'Routing Policy'],[3,'Routing Policy (strict)']], value: nvram[t+'_rgw'] },
					{ name: t+'_gw', type: 'text', maxlen: 15, size: 17, value: nvram[t+'_gw'], prefix: '<span id="'+t+'_gateway"> &nbsp;Gateway:&nbsp', suffix: '<\/span>'} ] },
					{ title: 'Priority', indent: 2, name: t+'_prio', type: 'text', maxlen: 5, size: 5, placeholder: (90 + i), suffix: '&nbsp;<small>(1 - 32766) lower number = higher priority<\/small>', value: nvram[t+'_prio'] },
				{ title: 'Accept DNS configuration', name: t+'_adns', type: 'select', options: [[0,'Disabled'],[1,'Relaxed'],[2,'Strict'],[3,'Exclusive']], value: nvram[t+'_adns'] },
				{ title: 'Data ciphers', name: t+'_ncp_ciphers', type: 'text', size: 70, maxlen: 127, value: nvram[t+'_ncp_ciphers'] },
				{ title: 'Cipher', name: t+'_cipher', type: 'select', options: ciphers, value: nvram[t+'_cipher'] },
				{ title: 'Compression', name: t+'_comp', type: 'select', options: [['-1','Disabled'],['no','None']
/* SIZEOPTMORE-BEGIN */
				          ,['lz4','LZ4'],['lz4-v2','LZ4-V2'],['stub','Stub'],['stub-v2','Stub-V2']
/* SIZEOPTMORE-END */
				          ], value: nvram[t+'_comp'] },
				{ title: 'TLS Renegotiation Time', name: t+'_reneg', type: 'text', maxlen: 10, size: 7, value: nvram[t+'_reneg'], suffix: ' <small>seconds; -1 for default<\/small>' },
				{ title: 'Connection retry', name: t+'_retry', type: 'text', maxlen: 5, size: 7, value: nvram[t+'_retry'], suffix: ' <small>seconds; -1 for infinite<\/small>' },
				{ title: 'Verify Certificate<br>(remote-cert-tls server)', name: 'f_'+t+'_tlsremote', type: 'checkbox', value: nvram[t+'_tlsremote'] != 0 },
				{ title: 'Verify Server Certificate Name<br>(verify-x509-name)', multi: [
					{ name: t+'_tlsvername', type: 'select', options: [[0,'No'],[1,'Common Name'],[2,'Common Name Prefix'],[3,'Subject']], value: nvram[t+'_tlsvername'] },
					{ name: t+'_cn', type: 'text', maxlen: 255, size: 54, value: nvram[t+'_cn'], prefix: '<span id="client_'+t+'_cn">&nbsp;:&nbsp', suffix: '<\/span>'} ] },
/* TOMATO64-BEGIN */
				{ title: 'Data Channel Offload (DCO)', name: 'f_'+t+'_dco', type: 'checkbox', value: nvram[t+'_dco'] != 0,
					suffix: ' <small>requires TUN, UDP, and AEAD ciphers<\/small>' },
/* TOMATO64-END */
				{ title: 'Custom Configuration', name: t+'_custom', type: 'textarea', value: nvram[t+'_custom'] }
			]);
			W('<\/div>');

			W('<div id="'+t+'-policy">');
			W('<div class="tomato-grid" id="table_'+t+'_routing"><\/div>');
			W('<div id="_'+t+'_routing_div_help"><div class="fields"><div class="about"><b>To use Routing Policy, you have to choose TUN as interface and "Routing Policy"/"Routing Policy (strict)" in "Redirect Internet Traffic".<\/b><\/div><\/div><\/div>');
			W('<div>');
			W('<ul>');
			W('<li><b>Type -> From Source IP<\/b> - Ex: "1.2.3.4", "1.2.3.4-2.3.4.5", "1.2.3.0/24".<\/li>');
			W('<li><b>Type -> To Destination IP<\/b> - Ex: "1.2.3.4" or "1.2.3.0/24".<\/li>');
			W('<li><b>Type -> To Domain<\/b> - Ex: "domain.com". Please enter one domain per line.<\/li>');
			W('<li><b>IMPORTANT!<\/b> - Kill Switch: the iptables rules (when "KS" is enabled for a given policy entry) are always applied, even if the VPN instance is down and Policy-Based Routing (PBR) mode is active. This implements the so-called "strict Kill Switch" behavior.<\/li>');
			W('<\/ul>');
			W('<\/div>');
			W('<\/div>');

			W('<div id="'+t+'-keys">');
			W('<p class="vpn-keyhelp">For help generating keys, refer to the OpenVPN <a id="'+t+'-keyhelp">HOWTO<\/a>.<\/p>');
			createFieldTable('', [
				{ title: 'Static Key', name: t+'_static', type: 'textarea', value: nvram[t+'_static'] },
				{ title: 'Certificate Authority', name: t+'_ca', type: 'textarea', value: nvram[t+'_ca'] },
				{ title: 'Client Certificate', name: t+'_crt', type: 'textarea', value: nvram[t+'_crt'] },
				{ title: 'Client Key', name: t+'_key', type: 'textarea', value: nvram[t+'_key'] },
			]);
			W('<\/div>');

			W('<div id="'+t+'-status">');
			W('<div id="'+t+'-no-status"><p>Client is not running or status could not be read.<\/p><\/div>');
			W('<div id="'+t+'-status-content" style="display:none" class="status-content">');
			W('<div id="'+t+'-status-header" class="vpn-status-header"><p>Data current as of <span id="'+t+'-status-time"><\/span><\/p><\/div>');
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
