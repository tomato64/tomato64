<!DOCTYPE html>
<!--
	Tomato GUI
	Copyright (C) 2006-2008 Jonathan Zarate
	http://www.polarcloud.com/tomato/

	Portions Copyright (C) 2008-2010 Keith Moyer, tomatovpn@keithmoyer.com
	Portions Copyright (C) 2010-2011 Jean-Yves Avenard, jean-yves@avenard.org

	For use with Tomato Firmware only.
	No part of this file may be used without permission.
-->
<html lang="en-GB">
<head>
<meta http-equiv="content-type" content="text/html;charset=utf-8">
<meta name="robots" content="noindex,nofollow">
<title>[<% ident(); %>] OpenVPN: Client</title>
<link rel="stylesheet" type="text/css" href="tomato.css">
<% css(); %>
<script src="isup.jsz"></script>
<script src="tomato.js"></script>
<script src="vpn.js"></script>

<script>

//	<% nvram("vpn_client_eas,vpn_client1_poll,vpn_client1_if,vpn_client1_bridge,vpn_client1_nat,vpn_client1_proto,vpn_client1_addr,vpn_client1_port,vpn_client1_retry,vpn_client1_firewall,vpn_client1_crypt,vpn_client1_comp,vpn_client1_cipher,vpn_client1_ncp_ciphers,vpn_client1_local,vpn_client1_remote,vpn_client1_nm,vpn_client1_reneg,vpn_client1_hmac,vpn_client1_adns,vpn_client1_rgw,vpn_client1_gw,vpn_client1_custom,vpn_client1_static,vpn_client1_ca,vpn_client1_crt,vpn_client1_key,vpn_client1_userauth,vpn_client1_username,vpn_client1_password,vpn_client1_useronly,vpn_client1_tlsremote,vpn_client1_cn,vpn_client1_br,vpn_client1_digest,vpn_client1_routing_val,vpn_client1_fw,vpn_client1_tlsvername,vpn_client2_poll,vpn_client2_if,vpn_client2_bridge,vpn_client2_nat,vpn_client2_proto,vpn_client2_addr,vpn_client2_port,vpn_client2_retry,vpn_client2_firewall,vpn_client2_crypt,vpn_client2_comp,vpn_client2_cipher,vpn_client2_ncp_ciphers,vpn_client2_local,vpn_client2_remote,vpn_client2_nm,vpn_client2_reneg,vpn_client2_hmac,vpn_client2_adns,vpn_client2_rgw,vpn_client2_gw,vpn_client2_custom,vpn_client2_static,vpn_client2_ca,vpn_client2_crt,vpn_client2_key,vpn_client2_userauth,vpn_client2_username,vpn_client2_password,vpn_client2_useronly,vpn_client2_tlsremote,vpn_client2_cn,vpn_client2_br,vpn_client2_digest,vpn_client2_routing_val,vpn_client2_fw,vpn_client2_tlsvername,vpn_client3_poll,vpn_client3_if,vpn_client3_bridge,vpn_client3_nat,vpn_client3_proto,vpn_client3_addr,vpn_client3_port,vpn_client3_retry,vpn_client3_firewall,vpn_client3_crypt,vpn_client3_comp,vpn_client3_cipher,vpn_client3_ncp_ciphers,vpn_client3_local,vpn_client3_remote,vpn_client3_nm,vpn_client3_reneg,vpn_client3_hmac,vpn_client3_adns,vpn_client3_rgw,vpn_client3_gw,vpn_client3_custom,vpn_client3_static,vpn_client3_ca,vpn_client3_crt,vpn_client3_key,vpn_client3_userauth,vpn_client3_username,vpn_client3_password,vpn_client3_useronly,vpn_client3_tlsremote,vpn_client3_cn,vpn_client3_br,vpn_client3_digest,vpn_client3_routing_val,vpn_client3_fw,vpn_client3_tlsvername,lan_ifname,lan1_ifname,lan2_ifname,lan3_ifname"); %>

var changed = 0, i;
var unitCount = OVPN_CLIENT_COUNT;
var serviceType = 'vpnclient';
for (i = 1; i <= unitCount; i++) serviceLastUp.push('0');

function RouteGrid() {return this;}
RouteGrid.prototype = new TomatoGrid;

var tabs =  [];
for (i = 1; i <= unitCount; ++i)
	tabs.push(['client'+i,'Client '+i]);
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

var vpndigests = vpndigests.concat(['DSA','DSA-SHA','DSA-SHA1','DSA-SHA1-old','ecdsa-with-SHA1','MD4','MDC2','RSA-MD5','RSA-MDC2','RSA-RIPEMD160','RSA-SHA','RSA-SHA1','RSA-SHA1-2','RSA-SHA224','RSA-SHA256','RSA-SHA384','RSA-SHA512','SHA','whirlpool']);
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
	}

	cookie.set('vpn_client_tab', name);
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

	cookie.set('vpn_client'+tab+'_section', section);
}

function updateForm(num, fw) {
	var fom = E('t_fom');

	if (fw && fom._service.value.indexOf('firewall') < 0) {
		if (fom._service.value != '')
			fom._service.value += ',';

		fom._service.value += 'firewall-restart';
	}

	if (eval('isup.vpnclient'+num) && fom._service.value.indexOf('client'+num) < 0) {
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
	else if (f[2].value.indexOf(' ') >= 0 || f[2].value.indexOf(',') >= 0 || f[2].value.indexOf('\t') >= 0) {
		ferror.set(f[2], 'Value cannot contain "space", "tab"  or "," characters. Only one IP or Domain per entry', quiet || !ok);
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
	var ok = 1;
	tgHideIcons();

	/* When settings change, make sure we restart the right client */
	if (focused) {
		changed = 1;

		var clientidx = focused.name.indexOf('client');
		if (clientidx >= 0) {
			var clientnum = focused.name.substring(clientidx + 6, clientidx + 7);
			var stripped = focused.name.substring(0, clientidx + 6) + focused.name.substring(clientidx + 7);

			if (stripped == 'vpn_client_local')
				E('_f_vpn_client'+clientnum+'_local').value = focused.value;
			else if (stripped == 'f_vpn_client_local')
				E('_vpn_client'+clientnum+'_local').value = focused.value;

			updateForm(clientnum, 0);
		}
	}

	/* Element varification */
	for (var i = 0; i < tabs.length; ++i) {
		var t = tabs[i][0];

		if (!v_range('_vpn_'+t+'_poll', quiet || !ok, 0, 30))
			ok = 0;
		if (!v_ip('_vpn_'+t+'_addr', true) && !v_domain('_vpn_'+t+'_addr', true)) {
			ferror.set(E('_vpn_'+t+'_addr'), 'Invalid server address', quiet || !ok);
			ok = 0;
		}
		else
			ferror.clear(E('_vpn_'+t+'_addr'));

		if (!v_port('_vpn_'+t+'_port', quiet || !ok))
			ok = 0;
		if (!v_ip('_vpn_'+t+'_local', quiet || !ok, 1))
			ok = 0;
		if (!v_ip('_f_vpn_'+t+'_local', true, 1))
			ok = 0;
		if (!v_ip('_vpn_'+t+'_remote', quiet || !ok, 1))
			ok = 0;
		if (!v_netmask('_vpn_'+t+'_nm', quiet || !ok))
			ok = 0;
		if (!v_range('_vpn_'+t+'_retry', quiet || !ok, -1, 32767))
			ok = 0;
		if (!v_range('_vpn_'+t+'_reneg', quiet || !ok, -1, 2147483647))
			ok = 0;
		if (E('_vpn_'+t+'_gw').value.length > 0 && !v_ip('_vpn_'+t+'_gw', quiet || !ok, 1))
			ok = 0;
	}

	/* Visibility changes */
	for (i = 0; i < tabs.length; ++i) {
		t = tabs[i][0];

		var fw = E('_vpn_'+t+'_firewall').value;
		var auth = E('_vpn_'+t+'_crypt').value;
		var iface = E('_vpn_'+t+'_if').value;
		var bridge = E('_f_vpn_'+t+'_bridge').checked;
		var nat = E('_f_vpn_'+t+'_nat').checked;
		var hmac = E('_vpn_'+t+'_hmac').value;
		var rgw = E('_vpn_'+t+'_rgw').value;

		E('_vpn_'+t+'_rgw').options[2].disabled = (iface == 'tap');
		E('_vpn_'+t+'_rgw').options[3].disabled = (iface == 'tap');
		if (iface == 'tap' && rgw >= 2) {
			rgw = 1;
			E('_vpn_'+t+'_rgw').value = 1;
		}

		var rtable = (iface == 'tun' && rgw >= 2);
		var userauth =  E('_f_vpn_'+t+'_userauth').checked && auth == 'tls';
		var useronly = userauth && E('_f_vpn_'+t+'_useronly').checked;

		/* Page Basic */
		elem.display(PR('_f_vpn_'+t+'_userauth'), auth == 'tls');
		elem.display(PR('_vpn_'+t+'_username'), PR('_vpn_'+t+'_password'), userauth );
		elem.display(PR('_f_vpn_'+t+'_useronly'), userauth);
		elem.display(E(t+'_ca_warn_text'), useronly);
		elem.display(PR('_vpn_'+t+'_hmac'), auth == 'tls');
		elem.display(E(t+'_custom_crypto_text'), auth == 'custom');
		elem.display(PR('_f_vpn_'+t+'_bridge'), iface == 'tap');
		elem.display(PR('_vpn_'+t+'_br'), iface == 'tap' && bridge > 0);
		elem.display(E(t+'_bridge_warn_text'), !bridge);
		elem.display(PR('_f_vpn_'+t+'_nat'), fw != 'custom' && (iface == 'tun' || !bridge));
		elem.display(PR('_f_vpn_'+t+'_fw'), fw != 'custom' && (iface == 'tun' || !bridge));
		elem.display(E(t+'_nat_warn_text'), fw != 'custom' && (!nat || (auth == 'secret' && iface == 'tun')));
		elem.display(PR('_vpn_'+t+'_local'), iface == 'tun' && auth == 'secret');
		elem.display(PR('_f_vpn_'+t+'_local'), iface == 'tap' && !bridge && auth != 'custom');

		/* Page Advanced */
		elem.display(PR('_vpn_'+t+'_adns'), PR('_vpn_'+t+'_reneg'), auth == 'tls');
		elem.display(E(t+'_gateway'), iface == 'tap' && rgw > 0);
		elem.display(PR('_vpn_'+t+'_ncp_ciphers'), auth == 'tls');
		elem.display(PR('_vpn_'+t+'_cipher'), auth == 'secret');

		/* Page Routing Policy */
		elem.display(E('table_'+t+'_routing'), rtable);
		elem.display(E('_vpn_'+t+'_routing_div_help'), !rtable);

		/* Page Key */
		elem.display(PR('_vpn_'+t+'_static'), auth == 'secret' || (auth == 'tls' && hmac >= 0));
		elem.display(PR('_vpn_'+t+'_ca'), auth == 'tls');
		elem.display(PR('_vpn_'+t+'_crt'), PR('_vpn_'+t+'_key'), auth == 'tls' && !useronly);
		elem.display(PR('_f_vpn_'+t+'_tlsremote'), auth == 'tls');
		elem.display(PR('_vpn_'+t+'_tlsvername'), auth == 'tls');
		elem.display(E('client_'+t+'_cn'), (auth == 'tls') && (E('_vpn_'+t+'_tlsvername').value > 0));

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
		for (var j = 0; j <= 3; ++j) {
			t = (j == 0  ? '' : j);

			if (eval('nvram.lan'+t+'_ifname.length') < 1)
				E('_vpn_client'+(i + 1)+'_br').options[j].disabled = 1;
		}
	}

	return ok;
}

function save() {
	if (!verifyFields(null, 0))
		return;

	var fom = E('t_fom');

	fom.vpn_client_eas.value = '';

	for (var i = 0; i < tabs.length; ++i) {
		if (routingTables[i].isEditing())
			return;

		var t = tabs[i][0];

		if (E('_f_vpn_'+t+'_eas').checked)
			fom.vpn_client_eas.value += ''+(i + 1)+',';

		var routedata = routingTables[i].getAllData();
		var routing = '';
		for (j = 0; j < routedata.length; ++j)
			routing += routedata[j].join('<')+'>';

		E('vpn_'+t+'_bridge').value = E('_f_vpn_'+t+'_bridge').checked ? 1 : 0;
		E('vpn_'+t+'_nat').value = E('_f_vpn_'+t+'_nat').checked ? 1 : 0;
		E('vpn_'+t+'_fw').value = E('_f_vpn_'+t+'_fw').checked ? 1 : 0;
		E('vpn_'+t+'_userauth').value = E('_f_vpn_'+t+'_userauth').checked ? 1 : 0;
		E('vpn_'+t+'_useronly').value = E('_f_vpn_'+t+'_useronly').checked ? 1 : 0;
		E('vpn_'+t+'_tlsremote').value = E('_f_vpn_'+t+'_tlsremote').checked ? 1 : 0;
		E('vpn_'+t+'_routing_val').value = routing;
	}
	fom._nofootermsg.value = 0;

	form.submit(fom, 1);

	changed = 0;
}

function earlyInit() {
	show();
	tabSelect(cookie.get('vpn_client_tab') || tabs[0][0]);

	for (var i = 0; i < tabs.length; ++i) {
		sectSelect(i, cookie.get('vpn_client'+i+'_section') || sections[i][0]);

		var t = tabs[i][0];

		routingTables[i].init('table_'+t+'_routing','sort', 0,[ { type: 'checkbox', prefix: '<div class="centered">', suffix: '<\/div>' },
		                                                        { type: 'select', options: [[1,'From Source IP'],[2,'To Destination IP'],[3,'To Domain']] },
		                                                        { type: 'text', maxlen: 50 },
		                                                        { type: 'checkbox', prefix: '<div class="centered">', suffix: '<\/div>' }]);
		routingTables[i].headerSet(['Enable','Type','Value','Kill Switch']);
		var routingVal = eval('nvram.vpn_'+t+'_routing_val');
		if (routingVal.length) {
			var s = routingVal.split('>');
			for (var j = 0; j < s.length; ++j) {
				if (!s[j].length)
					continue;

				var row = s[j].split('<');
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
}

function init() {
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

<input type="hidden" name="_nextpage" value="vpn-client.asp">
<input type="hidden" name="_service" value="">
<input type="hidden" name="_nofootermsg">
<input type="hidden" name="vpn_client_eas">

<!-- / / / -->

<div class="section-title">OpenVPN Client Configuration</div>
<div class="section">
	<script>
		tabCreate.apply(this, tabs);

		for (i = 0; i < tabs.length; ++i) {
			t = tabs[i][0];
			W('<div id="'+t+'-tab">');
			W('<input type="hidden" id="vpn_'+t+'_nat" name="vpn_'+t+'_nat">');
			W('<input type="hidden" id="vpn_'+t+'_fw" name="vpn_'+t+'_fw">');
			W('<input type="hidden" id="vpn_'+t+'_userauth" name="vpn_'+t+'_userauth">');
			W('<input type="hidden" id="vpn_'+t+'_useronly" name="vpn_'+t+'_useronly">');
			W('<input type="hidden" id="vpn_'+t+'_bridge" name="vpn_'+t+'_bridge">');
			W('<input type="hidden" id="vpn_'+t+'_tlsremote" name="vpn_'+t+'_tlsremote">');
			W('<input type="hidden" id="vpn_'+t+'_routing_val" name="vpn_'+t+'_routing_val">');

			W('<ul class="tabs">');
			for (j = 0; j < sections.length; j++)
				W('<li><a href="javascript:sectSelect('+i+',\''+sections[j][0]+'\')" id="'+t+'-'+sections[j][0]+'-tab">'+sections[j][1]+'<\/a><\/li>');
			W('<\/ul><div class="tabs-bottom"><\/div>');

			W('<div id="'+t+'-basic">');
			createFieldTable('', [
				{ title: 'Enable on Start', name: 'f_vpn_'+t+'_eas', type: 'checkbox', value: nvram.vpn_client_eas.indexOf(''+(i+1)) >= 0 },
				{ title: 'Interface Type', name: 'vpn_'+t+'_if', type: 'select', options: [['tap','TAP'],['tun','TUN']], value: eval('nvram.vpn_'+t+'_if') },
				{ title: 'Bridge TAP with', indent: 2, name: 'vpn_'+t+'_br', type: 'select', options: [['br0','LAN (br0)*'],['br1','LAN1 (br1)'],['br2','LAN2 (br2)'],['br3','LAN3 (br3)']],
					value: eval ('nvram.vpn_'+t+'_br'), suffix: ' <small>* default<\/small>' },
				{ title: 'Protocol', name: 'vpn_'+t+'_proto', type: 'select', options: [['udp','UDP'],['tcp-client','TCP'],['udp4','UDP4'],['tcp4-client','TCP4'],['udp6','UDP6'],['tcp6-client','TCP6']], value: eval('nvram.vpn_'+t+'_proto') },
				{ title: 'Server Address/Port', multi: [
					{ name: 'vpn_'+t+'_addr', type: 'text', maxlen: 60, size: 17, value: eval('nvram.vpn_'+t+'_addr') },
					{ name: 'vpn_'+t+'_port', type: 'text', maxlen: 5, size: 7, value: eval('nvram.vpn_'+t+'_port') } ] },
				{ title: 'Firewall', name: 'vpn_'+t+'_firewall', type: 'select', options: [['auto','Automatic'],['custom','Custom']], value: eval('nvram.vpn_'+t+'_firewall') },
				{ title: 'Create NAT on tunnel', name: 'f_vpn_'+t+'_nat', type: 'checkbox', value: eval('nvram.vpn_'+t+'_nat') != 0, suffix: ' <small id="'+t+'_nat_warn_text">routes must be configured manually<\/small>' },
				{ title: 'Inbound Firewall', name: 'f_vpn_'+t+'_fw', type: 'checkbox', value: eval('nvram.vpn_'+t+'_fw') != 0 },
				{ title: 'Authorization Mode', name: 'vpn_'+t+'_crypt', type: 'select', options: [['tls','TLS'],['secret','Static Key'],['custom','Custom']], value: eval('nvram.vpn_'+t+'_crypt'),
					suffix: ' <small id="'+t+'_custom_crypto_text">must be configured manually<\/small>' },
				{ title: 'TLS control channel security <small>(tls-auth/tls-crypt)<\/small>', name: 'vpn_'+t+'_hmac', type: 'select', options: [[-1,'Disabled'],[2,'Bi-directional Auth'],[0,'Incoming Auth (0)'],[1,'Outgoing Auth (1)'],[3,'Encrypt Channel'],[4,'Encrypt Channel V2']], value: eval('nvram.vpn_'+t+'_hmac') },
				{ title: 'Username/Password Authentication', name: 'f_vpn_'+t+'_userauth', type: 'checkbox', value: eval('nvram.vpn_'+t+'_userauth') != 0 },
				{ title: 'Username: ', indent: 2, name: 'vpn_'+t+'_username', type: 'text', maxlen: 50, size: 54, value: eval('nvram.vpn_'+t+'_username') },
				{ title: 'Password: ', indent: 2, name: 'vpn_'+t+'_password', type: 'password', maxlen: 70, size: 54, peekaboo:1, value: eval('nvram.vpn_'+t+'_password') },
				{ title: 'Username Authen. Only', indent: 2, name: 'f_vpn_'+t+'_useronly', type: 'checkbox', value: eval('nvram.vpn_'+t+'_useronly') != 0,
					suffix: ' <small style="color:red" id="'+t+'_ca_warn_text">warning: must define Certificate Authority<\/small>' },
				{ title: 'Auth digest', name: 'vpn_'+t+'_digest', type: 'select', options: digests, value: eval('nvram.vpn_'+t+'_digest') },
				{ title: 'Server is on the same subnet', name: 'f_vpn_'+t+'_bridge', type: 'checkbox', value: eval('nvram.vpn_'+t+'_bridge') != 0,
					suffix: ' <small style="color:red" id="'+t+'_bridge_warn_text">warning: cannot bridge distinct subnets. Defaulting to routed mode<\/small>' },
				{ title: 'Local/remote endpoint addresses', multi: [
					{ name: 'vpn_'+t+'_local', type: 'text', maxlen: 15, size: 17, value: eval('nvram.vpn_'+t+'_local') },
					{ name: 'vpn_'+t+'_remote', type: 'text', maxlen: 15, size: 17, value: eval('nvram.vpn_'+t+'_remote') } ] },
				{ title: 'Tunnel address/netmask', multi: [
					{ name: 'f_vpn_'+t+'_local', type: 'text', maxlen: 15, size: 17, value: eval('nvram.vpn_'+t+'_local') },
					{ name: 'vpn_'+t+'_nm', type: 'text', maxlen: 15, size: 17, value: eval('nvram.vpn_'+t+'_nm') } ] }
			]);
			W('<\/div>');

			W('<div id="'+t+'-advanced">');
			createFieldTable('', [
				{ title: 'Poll Interval', name: 'vpn_'+t+'_poll', type: 'text', maxlen: 2, size: 5, value: eval('nvram.vpn_'+t+'_poll'), suffix: ' <small>minutes; 0 to disable<\/small>' },
				{ title: 'Redirect Internet traffic', multi: [
					{ name: 'vpn_'+t+'_rgw', type: 'select', options: [[0,'No'],[1,'All'],[2,'Routing Policy'],[3,'Routing Policy (strict)']], value: eval('nvram.vpn_'+t+'_rgw') },
					{ name: 'vpn_'+t+'_gw', type: 'text', maxlen: 15, size: 17, value: eval('nvram.vpn_'+t+'_gw'), prefix: '<span id="'+t+'_gateway"> &nbsp;Gateway:&nbsp', suffix: '<\/span>'} ] },
				{ title: 'Accept DNS configuration', name: 'vpn_'+t+'_adns', type: 'select', options: [[0,'Disabled'],[1,'Relaxed'],[2,'Strict'],[3,'Exclusive']], value: eval('nvram.vpn_'+t+'_adns') },
				{ title: 'Data ciphers', name: 'vpn_'+t+'_ncp_ciphers', type: 'text', size: 70, maxlen: 127, value: eval('nvram.vpn_'+t+'_ncp_ciphers') },
				{ title: 'Cipher', name: 'vpn_'+t+'_cipher', type: 'select', options: ciphers, value: eval('nvram.vpn_'+t+'_cipher') },
				{ title: 'Compression', name: 'vpn_'+t+'_comp', type: 'select', options: [['-1','Disabled'],['no','None'],['yes','LZO'],['adaptive','LZO Adaptive'],['lz4','LZ4'],['lz4-v2','LZ4-V2'],['stub','Stub'],['stub-v2','Stub-V2']], value: eval('nvram.vpn_'+t+'_comp') },
				{ title: 'TLS Renegotiation Time', name: 'vpn_'+t+'_reneg', type: 'text', maxlen: 10, size: 7, value: eval('nvram.vpn_'+t+'_reneg'), suffix: ' <small>seconds; -1 for default<\/small>' },
				{ title: 'Connection retry', name: 'vpn_'+t+'_retry', type: 'text', maxlen: 5, size: 7, value: eval('nvram.vpn_'+t+'_retry'), suffix: ' <small>seconds; -1 for infinite<\/small>' },
				{ title: 'Verify Certificate<br>(remote-cert-tls server)', name: 'f_vpn_'+t+'_tlsremote', type: 'checkbox', value: eval('nvram.vpn_'+t+'_tlsremote') != 0 },
				{ title: 'Verify Server Certificate Name<br>(verify-x509-name)', multi: [
					{ name: 'vpn_'+t+'_tlsvername', type: 'select', options: [[0,'No'],[1,'Common Name'],[2,'Common Name Prefix'],[3,'Subject']], value: eval('nvram.vpn_'+t+'_tlsvername') },
					{ name: 'vpn_'+t+'_cn', type: 'text', maxlen: 255, size: 54, value: eval('nvram.vpn_'+t+'_cn'), prefix: '<span id="client_'+t+'_cn">&nbsp;:&nbsp', suffix: '<\/span>'} ] },
				{ title: 'Custom Configuration', name: 'vpn_'+t+'_custom', type: 'textarea', value: eval('nvram.vpn_'+t+'_custom') }
			]);
			W('<\/div>');

			W('<div id="'+t+'-policy">');
			W('<div class="tomato-grid" id="table_'+t+'_routing"><\/div>');
			W('<div id="_vpn_'+t+'_routing_div_help"><div class="fields"><div class="about"><b>To use Routing Policy, you have to choose TUN as interface and "Routing Policy" in "Redirect Internet Traffic".<\/b><\/div><\/div><\/div>');
			W('<div>');
			W('<ul>');
			W('<li><b>Type -> From Source IP<\/b> - Ex: "1.2.3.4", "1.2.3.4 - 2.3.4.5", "1.2.3.0/24".');
			W('<li><b>Type -> To Destination IP<\/b> - Ex: "1.2.3.4" or "1.2.3.0/24".');
			W('<li><b>Type -> To Domain<\/b> - Ex: "domain.com". Please enter one domain per line');
			W('<\/ul>');
			W('<\/div>');
			W('<\/div>');

			W('<div id="'+t+'-keys">');
			W('<p class="vpn-keyhelp">For help generating keys, refer to the OpenVPN <a id="'+t+'-keyhelp">HOWTO<\/a>.<\/p>');
			createFieldTable('', [
				{ title: 'Static Key', name: 'vpn_'+t+'_static', type: 'textarea', value: eval('nvram.vpn_'+t+'_static') },
				{ title: 'Certificate Authority', name: 'vpn_'+t+'_ca', type: 'textarea', value: eval('nvram.vpn_'+t+'_ca') },
				{ title: 'Client Certificate', name: 'vpn_'+t+'_crt', type: 'textarea', value: eval('nvram.vpn_'+t+'_crt') },
				{ title: 'Client Key', name: 'vpn_'+t+'_key', type: 'textarea', value: eval('nvram.vpn_'+t+'_key') },
			]);
			W('<\/div>');

			W('<div id="'+t+'-status">');
			W('<div id="'+t+'-no-status"><p>Client is not running or status could not be read.<\/p><\/div>');
			W('<div id="'+t+'-status-content" style="display:none" class="status-content">');
			W('<div id="'+t+'-status-header" class="vpn-status-header"><p>Data current as of <span id="'+t+'-status-time"><\/span><\/p><\/div>');
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
