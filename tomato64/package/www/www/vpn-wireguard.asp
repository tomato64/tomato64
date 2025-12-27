<!DOCTYPE html>
<!--
	FreshTomato GUI
	Copyright (C) 2023 - 2025 pedro
	https://freshtomato.org/

	For use with FreshTomato Firmware only.
	No part of this file may be used without permission.
-->
<html lang="en-GB">
<head>
<meta http-equiv="content-type" content="text/html;charset=utf-8">
<meta name="robots" content="noindex,nofollow">
<title>[<% ident(); %>] Wireguard</title>
<link rel="stylesheet" type="text/css" href="tomato.css?rel=<% version(); %>">
<% css(); %>
<script src="isup.jsx?_http_id=<% nv(http_id); %>"></script>
<script src="tomato.js?rel=<% version(); %>"></script>
<script src="wireguard.js?rel=<% version(); %>"></script>
<script src="interfaces.js?rel=<% version(); %>"></script>
<script src="qrcode.js?rel=<% version(); %>"></script>
<script src="html5-qrcode.js?rel=<% version(); %>"></script>
<script>


//	<% nvram("wan_ipaddr,wan_hostname,wan_domain,lan_ifname,lan_ipaddr,lan_netmask,wg_adns,wg0_enable,wg0_poll,wg0_file,wg0_ip,wg0_fwmark,wg0_mtu,wg0_preup,wg0_postup,wg0_predown,wg0_postdown,wg0_aip,wg0_dns,wg0_peer_dns,wg0_ka,wg0_port,wg0_key,wg0_endpoint,wg0_com,wg0_lan,wg0_rgw,wg0_peers,wg0_route,wg0_firewall,wg0_nat,wg0_fw,wg0_rgwr,wg0_routing_val,wg0_prio,wg1_enable,wg1_poll,wg1_file,wg1_ip,wg1_fwmark,wg1_mtu,wg1_preup,wg1_postup,wg1_predown,wg1_postdown,wg1_aip,wg1_dns,wg1_peer_dns,wg1_ka,wg1_port,wg1_key,wg1_endpoint,wg1_com,wg1_lan,wg1_rgw,wg1_peers,wg1_route,wg1_firewall,wg1_nat,wg1_fw,wg1_rgwr,wg1_routing_val,wg1_prio,wg2_enable,wg2_poll,wg2_file,wg2_ip,wg2_fwmark,wg2_mtu,wg2_preup,wg2_postup,wg2_predown,wg2_postdown,wg2_aip,wg2_dns,wg2_peer_dns,wg2_ka,wg2_port,wg2_key,wg2_endpoint,wg2_com,wg2_lan,wg2_rgw,wg2_peers,wg2_route,wg2_firewall,wg2_nat,wg2_fw,wg2_rgwr,wg2_routing_val,wg2_prio"); %>


var cprefix = 'vpn_wireguard';
var changed = 0, i;
var serviceType = 'wireguard';
for (i = 0; i < WG_INTERFACE_COUNT; i++) serviceLastUp.push('0');

function RouteGrid() {return this;}
RouteGrid.prototype = new TomatoGrid;

var tabs = [];
for (i = 0; i < WG_INTERFACE_COUNT; ++i)
	tabs.push(['wg'+i,'<span id="'+serviceType+i+'_tabicon" style="font-size:9px">▽ <\/span><span class="tabname">wg'+i+'<\/span>']);
var sections = [['wg-config','Config'],['wg-peersp','Peers Params'],['wg-peers','Peers'],['wg-scripts','Scripts'],['wg-policy','Routing Policy'],['wg-status','Status']];

var routingTables = [];
for (i = 0; i < tabs.length; ++i) routingTables.push(new RouteGrid());

function update_nvram(fom) {
	for (var i = 0; i < fom.length; ++i) {
		if (fom[i].name in nvram)
			nvram[fom[i].name] = fom[i].value;
	}
}

function PeerGrid() {return this;}
PeerGrid.prototype = new TomatoGrid;

var peerTables = [];
for (i = 0; i < tabs.length; ++i) {
	peerTables.push(new PeerGrid());
	peerTables[i].interface_name = tabs[i][0];
	peerTables[i].unit = i;
}

function StatusRefresh() {return this;}
StatusRefresh.prototype = new TomatoRefresh;

var statRefreshes = [];
for (i = 0; i < tabs.length; ++i) {
	statRefreshes.push(new StatusRefresh());
	statRefreshes[i].interface_name = tabs[i][0];
	statRefreshes[i].unit = i;
}

function toggleRefresh(unit) {
	statRefreshes[unit].toggle();
}

ferror.show = function(e) {
	if ((e = E(e)) == null) return;
	if (!e._error_msg) return;
	elem.addClass(e, 'error-focused');
	var id = locateElement(e);
	var tab = id.slice(0, 3);
	var section = id.slice(4);
	if (section == 'table_routing')
		section = 'wg-policy'; /* special case */

	tabSelect(tab);
	sectSelect(tab.substr(2), section);
	e.focus();
	alert(e._error_msg);
	elem.removeClass(e, 'error-focused');
}

function locateElement(e) {
	do {
		e = e.parentElement;
	} while(e.id.indexOf('wg') < 0);

	return e.id;
}

function show() {
	countButton += 1;
	for (var i = 0; i < WG_INTERFACE_COUNT; ++i) {
		var e = E('_'+serviceType+i+'_button');
		var d = isup[serviceType+i];

		E('_'+serviceType+i+'_notice').innerHTML = (d ? '<span class="service_up"><span class="servup_image">▲ <\/span>Up<\/span>' : '<span class="service_down"><span class="servdn_image">▽ <\/span>Down<\/span>');
		if (E(serviceType+i+'_tabicon')) E(serviceType+i+'_tabicon').innerHTML = (d ? '<span class="icon_up"><span class="servup_image">▲ <\/span><\/span>' : '<span class="icon_down"><span class="servdn_image">▽ <\/span><\/span>');
		e.value = (d ? 'Stop' : 'Start')+' Now';
		e.setAttribute('onclick', 'javascript:toggle(\''+serviceType+''+i+'\','+d+');');
		if (serviceLastUp[i] != d || countButton > 6) {
			serviceLastUp[i] = d;
			countButton = 0;
			e.disabled = 0;
			E('spin'+i).style.display = 'none';
		}
		if (!statRefreshes[i].running)
			statRefreshes[i].updateUI('stop');
	}
}

function toggle(service, up) {
	if (changed && !confirm('There are unsaved changes. Continue anyway?'))
		return;

	/* check for active 'External - VPN Provider' + 'Redirect Internet traffic' set to 'All' */
	var externalall_mode_active = 0;
	for (var i = 0; i < WG_INTERFACE_COUNT; i++) {
		if (isup['wireguard'+i] && E('_wg'+i+'_com').value == 3 && E('_wg'+i+'_rgwr').value == 1) /* active */
			externalall_mode_active++;
	}
	if (externalall_mode_active && !up && E('_wg'+service.substr(9, 1)+'_com').value == 3 && E('_wg'+service.substr(9, 1)+'_rgwr').value == 1) {
		alert('Only one wireguard instance can be run in "External - VPN Provider" mode with "Redirect Internet traffic" set to "All"!');
		return;
	}

	serviceLastUp[id] = up;
	countButton = 0;

	var id = service.substr(service.length - 1);
	E('_'+service+'_button').disabled = 1;
	E('spin'+id).style.display = 'inline';

	var fom = E('t_fom');
	var bup = fom._service.value;
	fom._service.value = service+(up ? '-stop' : '-start');
	fom._nofootermsg.value = 1;

	form.submit(fom, 1, 'service.cgi');
	fom._service.value = bup;
}

function tabSelect(name) {
	tgHideIcons();

	tabHigh(name);

	for (var i = 0; i < tabs.length; ++i) {
		if (name == tabs[i][0]) {
			elem.display(tabs[i][0]+'-wg-tab', true);
			elem.display(tabs[i][0]+'-wg-status-button', true);
		}
		else {
			elem.display(tabs[i][0]+'-wg-tab', false);
			elem.display(tabs[i][0]+'-wg-status-button', false);
		}
	}

	cookie.set(cprefix+'_tab', name);
}

function sectSelect(tab, section) {
	tgHideIcons();

	for (var i = 0; i < sections.length; ++i) {
		if (section == sections[i][0]) {
			elem.addClass(tabs[tab][0]+'-'+sections[i][0]+'-wg-tab', 'active');
			elem.display(tabs[tab][0]+'-'+sections[i][0], true);
		}
		else {
			elem.removeClass(tabs[tab][0]+'-'+sections[i][0]+'-wg-tab', 'active');
			elem.display(tabs[tab][0]+'-'+sections[i][0], false);
		}
	}

	cookie.set(cprefix+tab+'_section', section);
}

function updateForm(num, fw) {
	var fom = E('t_fom');

	if (fw && fom._service.value.indexOf('firewall') < 0) {
		if (fom._service.value != '')
			fom._service.value += ',';

		fom._service.value += 'firewall-restart';
	}

	if (isup['wireguard'+num] && fom._service.value.indexOf('wireguard'+num) < 0) {
		if (fom._service.value != '')
			fom._service.value += ',';

		fom._service.value += 'wireguard'+num+'-restart';
	}
}

function loadConfig(unit) {
	if (isup['wireguard'+unit]) {
		alert('Before importing the configuration file, you must first stop this instance');
		return;
	}

	var [file] = E('wg'+unit+'_config_file').files;
	var index = file.name.lastIndexOf('.');

	if (file.name.slice(index).toLowerCase() != '.conf') {
		alert('Only files that end in ".conf" are accepted for import');
		return;
	}

	var reader = new FileReader();
	reader.unit = unit;
	addEvent(reader, 'load', mapConfigToFields);
	reader.readAsText(file);
}

function mapConfigToFields(event) {
	var config = mapConfig(event.target.result);
	var unit = event.target.unit;

	if (!validateConfig(config))
		return;

	clearAllFields(unit);

	if (config.interface.privkey)
		E('_wg'+unit+'_key').value = config.interface.privkey;

	if (config.interface.port)
		if (config.interface.port == (51820 + unit))
			E('_wg'+unit+'_port').value = ''
		else
			E('_wg'+unit+'_port').value = config.interface.port;

	if (config.interface.fwmark)
		E('_wg'+unit+'_fwmark').value = config.interface.fwmark;

	if (config.interface.address)
		E('_wg'+unit+'_ip').value = config.interface.address;

	if (config.interface.dns)
		E('_wg'+unit+'_dns').value = config.interface.dns;

	if (config.interface.mtu)
		E('_wg'+unit+'_mtu').value = config.interface.mtu;

	if (config.interface.preup)
		E('_wg'+unit+'_preup').value = config.interface.preup;

	if (config.interface.postup)
		E('_wg'+unit+'_postup').value = config.interface.postup;

	if (config.interface.predown)
		E('_wg'+unit+'_predown').value = config.interface.predown;

	if (config.interface.postdown)
		E('_wg'+unit+'_postdown').value = config.interface.postdown;

	if (config.interface.keepalive)
		E('_wg'+unit+'_ka').value = 25; /* default for keepalive, user may change */
	else
		E('_wg'+unit+'_ka').value = 0;

	if (config.interface.endpoint) {
		E('_f_wg'+unit+'_custom_endpoint').value = config.interface.endpoint;
		E('_f_wg'+unit+'_endpoint').selectedIndex = 2;
	}

	for (var i = 0; i < config.peers.length; ++i) {
		var peer = config.peers[i];

		if (E('_wg'+unit+'_com').value == 3) { /* 'External - VPN Provider' */
			var ip = '';
			var allowed_ips = peer.allowed_ips;
		}
		else {
			var [ip, allowed_ips] = peer.allowed_ips.split(',', 2);
			ip = ip.trim().split('/')[0]+'/32';
		}

		var data = [
			peer.alias ? peer.alias : '',
			peer.endpoint ? peer.endpoint : '',
			peer.privkey ? peer.privkey : '',
			peer.pubkey,
			peer.psk ? peer.psk : '',
			ip ? ip.trim() : '',
			allowed_ips ? allowed_ips.trim() : '',
			peer.keepalive ? 25 : 0 /* default for keepalive, user may change */
		];

		peerTables[unit].insertData(-1, data)
	}

	verifyFields();

	alert('Wireguard configuration imported successfully');
}

function clearAllFields(unit) {
	E('_wg'+unit+'_file').value = '';
	E('_wg'+unit+'_port').value = '';
	E('_wg'+unit+'_prio').value = '';
	E('_wg'+unit+'_key').value = '';
	E('_wg'+unit+'_pubkey').value = '';
	E('_wg'+unit+'_ip').value = '';
	E('_wg'+unit+'_fwmark').value = '';
	E('_wg'+unit+'_mtu').value = '';
	E('_wg'+unit+'_aip').value = '';
	E('_wg'+unit+'_peer_dns').value = '';
	E('_wg'+unit+'_preup').value = '';
	E('_wg'+unit+'_postup').value = '';
	E('_wg'+unit+'_predown').value = '';
	E('_wg'+unit+'_postdown').value = '';
	E('_wg'+unit+'_ka').value = '';
	E('_f_wg'+unit+'_adns').checked = 0;
	E('_f_wg'+unit+'_endpoint').selectedIndex = 0;
	E('_f_wg'+unit+'_custom_endpoint').value = '';
	E('_f_wg'+unit+'_rgw').checked = 0;
	for (var i = 0; i <= MAX_BRIDGE_ID; i++)
		E('_f_wg'+unit+'_lan'+i).checked = 0;

	peerTables[unit].removeAllData();
}

function validateConfig(config) {
	if (!config.interface.privkey) {
		alert('The interface requires a PrivateKey');
		return false;
	}

	if (!config.interface.address) {
		alert('The interface requires an Address');
		return false;
	}

	for (var i = 0; i < config.peers.length; ++i) {
		var peer = config.peers[i];

		if (!peer.pubkey) {
			alert('Every peer requires a PublicKey');
			return false;
		}

		if (!peer.allowed_ips) {
			alert('Every peer requires AllowedIPs');
			return false;
		}
	}

	return true;
}

function mapConfig(contents) {
	var lines = contents.split('\n');
	var unit = event.target.unit;
	var config = {
		'interface': {},
		'peers': []
	}

	var target;
	for (var i = 0; i < lines.length; ++i) {
		var line = lines[i].trim();

		var comment_index = line.indexOf('#');
		if (comment_index != -1) {
			if (line.match(/\#[a-zA-z]\s?.*\=\s?[a-zA-z0-9].*$/))
				line = line.substr(1);
			else
				line = line.slice(0, comment_index);
		}

		if (!line)
			continue;

		if (line.toLowerCase() == '[interface]') {
			target = config.interface;
			continue;
		}

		if (line.toLowerCase() == '[peer]') {
			target = {};
			config.peers.push(target);
			continue;
		}

		var index = line.indexOf('=');
		var key = line.slice(0, index).trim().toLowerCase();
		var value = line.slice(index + 1).trim();

		switch(key) {
			case 'alias':
				target.alias = value;
				break;
			case 'privatekey':
				target.privkey = value;
				break;
			case 'listenport':
				target.port = value;
				break;
			case 'fwmark':
				target.fwmark = value;
				break;
			case 'address':
				if (!target.address)
					target.address = value;
				else
					target.address = [target.address, value].join(',');
				break;
			case 'dns':
				if (!target.dns)
					target.dns = value;
				else
					target.dns = [target.dns, value].join(',');
				break;
			case 'mtu':
				target.mtu = value;
				break;
			case 'table':
				target.table = value;
				break;
			case 'preup':
				if (!target.preup)
					target.preup = value;
				else
					target.preup = [target.preup, value].join('\n');
				break;
			case 'postup':
				if (!target.postup)
					target.postup = value;
				else
					target.postup = [target.postup, value].join('\n');
				break;
			case 'predown':
				if (!target.predown)
					target.predown = value;
				else
					target.predown = [target.predown, value].join('\n');
				break;
			case 'postdown':
				if (!target.postdown)
					target.postdown = value;
				else
					target.postdown = [target.postdown, value].join('\n');
				break;
			case 'publickey':
				target.pubkey = value;
				break;
			case 'presharedkey':
				target.psk = value;
				break;
			case 'allowedips':
				if (!target.allowed_ips) {
					var tmp = value.split(',');
					for (var j = 0; j < tmp.length; ++j) {
						if (tmp[j].indexOf(':')) /* we're not IPv6 ready yet */
							tmp.splice(j, j);
					}
					target.allowed_ips = tmp.join(',');
				}
				else
					target.allowed_ips = [target.allowed_ips, value].join(',');
				break;
			case 'endpoint':
				if (E('_wg'+unit+'_com').value == 3) /* 'External - VPN Provider' */
					target.endpoint = value;
				else
					target.endpoint = value.split(':')[0];
				break;
			case 'persistentkeepalive':
				target.keepalive = value;
				break;
		} 
	}

	return config;
}

StatusRefresh.prototype.setup = function() {
	var e, v;

	this.actionURL = 'shell.cgi';
	this.postData = 'action=execute&command='+escapeCGI('/usr/sbin/wg show wg'+this.unit+' dump\n'.replace(/\r/g, ''));
	this.refreshTime = 5 * 1000;
	this.cookieTag = cprefix+this.unit+'_refresh';
	this.dontuseButton = 0;
	this.timer = new TomatoTimer(THIS(this, this.start));
}

StatusRefresh.prototype.start = function() {
	var e;

	if ((e = E('wg'+this.unit+'_status_refresh_time')) != null) {
		if (this.cookieTag)
			cookie.set(this.cookieTag, e.value);

		if (this.dontuseButton != 1)
			this.refreshTime = e.value * 1000;
	}

	this.updateUI('start');

	if ((e = E('wg'+this.unit+'_status_refresh_button')) != null) {
		if (e.value == 'Refresh')
			this.once = 1;
	}

	e = undefined;

	this.running = 1;
	if ((this.http = new XmlHttp()) == null) {
		reloadPage();
		return;
	}

	this.http.parent = this;

	this.http.onCompleted = function(text, xml) {
		var p = this.parent;

		if (p.cookieTag)
			cookie.unset(p.cookieTag+'-error');
		if (!p.running) {
			p.stop();
			return;
		}

		p.refresh(text);

		if ((p.refreshTime > 0) && (!p.once)) {
			p.updateUI('wait');
			p.timer.start(Math.round(p.refreshTime));
		}
		else
			p.stop();

		p.errors = 0;
	}

	this.http.onError = function(ex) {
		var p = this.parent;
		if ((!p) || (!p.running))
			return;

		p.timer.stop();

		if (++p.errors <= 3) {
			p.updateUI('wait');
			p.timer.start(3000);
			return;
		}

		if (p.cookieTag) {
			var e = cookie.get(p.cookieTag+'-error') * 1;
			if (isNaN(e))
				e = 0;
			else
				++e;

			cookie.unset(p.cookieTag);
			cookie.set(p.cookieTag+'-error', e, 1);
			if (e >= 3) {
				alert('XMLHTTP: '+ex);
				return;
			}
		}

		setTimeout(reloadPage, 2000);
	}

	this.errors = 0;
	this.http.post(this.actionURL, this.postData);
}

StatusRefresh.prototype.updateUI = function(mode) {
	var e, b;

	if (typeof(E) == 'undefined') /* for a bizzare bug... */
		return;

	if (this.dontuseButton != 1) {
		b = (mode != 'stop') && (this.refreshTime > 0);

		if ((e = E('wg'+this.unit+'_status_refresh_button')) != null) {
			e.value = b ? 'Stop' : 'Refresh';
			((mode == 'start') && (!b) ? e.setAttribute('disabled', 'disabled') : e.removeAttribute('disabled'));
		}

		if ((e = E('wg'+this.unit+'_status_refresh_time')) != null)
			((!b) ? e.removeAttribute('disabled') : e.setAttribute('disabled', 'disabled'));

		if ((e = E('wg'+this.unit+'_status_refresh_spinner')) != null)
			e.style.display = (b ? 'inline-block' : 'none');
			e.style.verticalAlign = (b ? 'middle' : '');
	}
}

StatusRefresh.prototype.initPage = function(delay, refresh) {
	var e, v;

	e = E('wg'+this.unit+'_status_refresh_time');
	if (((this.cookieTag) && (e != null)) && ((v = cookie.get(this.cookieTag)) != null) && (!isNaN(v *= 1))) {
		e.value = Math.abs(v);
		if (v > 0)
			v = v * 1000;
	}
	else if (refresh) {
		v = refresh * 1000;
		if ((e != null) && (this.dontuseButton != 1))
			e.value = refresh;
	}
	else
		v = 0;

	if (delay < 0) {
		v = -delay;
		this.once = 1;
	}

	if (v > 0) {
		this.running = 1;
		this.refreshTime = v;
		this.timer.start(delay);
		this.updateUI('wait');
	}
}

StatusRefresh.prototype.refresh = function(text) {
	var cmdresult;
	var output;
	eval(text);

	if ((cmdresult == 'Unable to access interface: No such device\n') || (cmdresult == 'Unable to access interface: Protocol not supported\n'))
		output = 'Wireguard device wg'+this.unit+' is down';
	else {
		var [iface, peers] = decodeDump(cmdresult, this.unit);
		output = encodeStatus(iface, peers);
	}
	displayStatus(this.unit, output);
}

PeerGrid.prototype.setup = function() {
	this.init(this.interface_name+'-peers-grid', '', 50, [
		{ type: 'text' },
		{ type: 'text' },
		{ type: 'text', maxlen: 32 },
		{ type: 'text', maxlen: 128 },
		{ type: 'password', maxlen: 44 },
		{ type: 'text', maxlen: 44 },
		{ type: 'password', maxlen: 44 },
		{ type: 'text', maxlen: 100 },
		{ type: 'text', maxlen: 128 },
		{ type: 'text', maxlen: 3 }
	]);
	this.headerSet(['QR', 'Cfg', 'Alias','Endpoint','Private Key','Public Key','Preshared Key','IP','Allowed IPs','KA']);
	this.disableNewEditor(true);

	var peers = decodePeers(this.unit);
	for (var i = 0; i < peers.length; ++i) {
		var peer = peers[i];
		var data = [
			peer.alias,
			peer.endpoint,
			peer.privkey,
			peer.pubkey,
			peer.psk,
			peer.ip,
			peer.allowed_ips,
			peer.keepalive
		];
		this.insertData(-1, data);
	}
}

PeerGrid.prototype.edit = function(cell) {
	var row = PR(cell);
	var data = row.getRowData();

	clearPeerFields(this.unit);

	var alias = E('_f_wg'+this.unit+'_peer_alias');
	var endpoint = E('_f_wg'+this.unit+'_peer_ep');
	var port = E('_f_wg'+this.unit+'_peer_port');
	var privkey = E('_f_wg'+this.unit+'_peer_privkey');
	var pubkey = E('_f_wg'+this.unit+'_peer_pubkey');
	var psk = E('_f_wg'+this.unit+'_peer_psk');
	var ip = E('_f_wg'+this.unit+'_peer_ip');
	var allowedips = E('_f_wg'+this.unit+'_peer_aip');
	var keepalive = E('_f_wg'+this.unit+'_peer_ka');
	var fwmark = E('_f_wg'+this.unit+'_peer_fwmark');

	var interface_port = nvram[this.interface_name+'_port'];
	if (interface_port == '')
		interface_port = (51820 + this.unit);

	E('_f_wg'+this.unit+'_peer_pubkey').disabled = 1;

	alias.value = data[0];
	endpoint.value = data[1];
	port.value = interface_port;
	privkey.value = data[2];
	pubkey.value = data[3];
	psk.value = data[4];
	ip.value = data[5];
	allowedips.value = data[6];
	/* compat */
	if (data[7] == 'true')
		keepalive.value = '25';
	else if (data[7] == 'false')
		keepalive.value = '0';
	else
		keepalive.value = data[7];

	fwmark.value = '';

	var button = E(this.interface_name+'_peer_add');
	button.value = 'Save to Peers';
	button.setAttribute('onclick', 'editPeer('+this.unit+', '+row.rowIndex+')');
}

PeerGrid.prototype.insertData = function(at, data) {
	if (at == -1)
		at = this.tb.rows.length;

	var view = this.dataToView(data);
	var cfg = '';
	var qr = '<span class="qriconsvg" title="Display QR Code" onclick="genPeerGridConfigQR(event,'+this.unit+','+at+')">&nbsp;<\/span>';
	if (data[2] != '') cfg = '<span class="cfgiconsvg" title="Download Config File" onclick="genPeerGridConfigFile(event,'+this.unit+','+at+',\'cfg\')">&nbsp;<\/span>';
	view.unshift(qr, cfg);
	view[5] = view[5].substring(0,8)+' ... '+view[5].slice(-8);

	return this.insert(at, data, view, false);
}

PeerGrid.prototype.rowDel = function(e) {
	changed = 1;
	TGO(e).moving = null;
	e.parentNode.removeChild(e);
	this.recolor();
	this.resort();
	this.rpHide();
}

PeerGrid.prototype.rpDel = function(e) {
	changed = 1;
	e = PR(e);
	var qrcode = E('wg'+this.unit+'_qrcode');
	if (qrcode.style.display != 'none') {
		var qr_row_id = qrcode.getAttribute('row_id');

		if (qr_row_id == e.rowIndex)
			elem.display('wg'+this.unit+'_qrcode', false);
		else {
			if (qr_row_id > e.rowIndex) {
				qr_row_id = qr_row_id - 1;
				qrcode.setAttribute('row_id', qr_row_id)
			}

			var content = genPeerGridConfig(this.unit, qr_row_id);
			displayQRCode(content, this.unit, qr_row_id);
		}
		
	}
	this.rowDel(e);
}

PeerGrid.prototype.getAllData = function() {
	var i, max, data, r, type;

	data = [];
	max = this.footer ? this.footer.rowIndex : this.tb.rows.length;
	for (i = this.header ? this.header.rowIndex + 1 : 0; i < max; ++i) {
		r = this.tb.rows[i];
		if ((r.style.display != 'none') && (r._data)) data.push(r._data);
	}

	/* reformat the data to include one key and a flag specifying which type */
	for (i = 0; i < data.length; ++i) {
		data[i] = encodePeers(data[i]);
	}

	return data;
}

function decodePeers(unit) {
	var peers = []
	var nv = nvram['wg'+unit+'_peers'].split('>');
	for (var i = 0; i < nv.length; ++i) {
		var t = nv[i].split('<');
		if (t.length == 8) {
			var data, pubkey, privkey;

			if (t[0] == 1) {
				privkey = t[3];
				pubkey = window.wireguard.generatePublicKey(privkey);
			}
			else {
				privkey = '';
				pubkey = t[3];
			}

			peers.push({
				'alias': t[1],
				'endpoint': t[2],
				'privkey': privkey,
				'pubkey': pubkey,
				'psk': t[4],
				'ip': t[5],
				'allowed_ips': t[6],
				'keepalive': t[7]
			});
		}
	}

	return peers;
}

function encodePeers(data) {
	var key, type;

	if(data[2]) {
		type = 1;
		key = data[2];
	}
	else {
		type = 0;
		key = data[3];
	}

	data = [
		type,
		data[0],
		data[1],
		key,
		data[4],
		data[5],
		data[6],
		data[7],
	]

	return data;
}

function verifyPeerFields(unit, require_privkey) {
	var ok = 1;

	var port = E('_f_wg'+unit+'_peer_port');
	var privkey = E('_f_wg'+unit+'_peer_privkey');
	var pubkey = E('_f_wg'+unit+'_peer_pubkey');
	var psk = E('_f_wg'+unit+'_peer_psk');
	var ip = E('_f_wg'+unit+'_peer_ip');
	var allowedips = E('_f_wg'+unit+'_peer_aip');
	var fwmark = E('_f_wg'+unit+'_peer_fwmark');

	if ((!port.value.match(/^ *[-\+]?\d+ *$/)) || (port.value < 1) || (port.value > 65535)) {
		ferror.set(port, 'A valid port must be provided', !ok);
		ok = 0;
	}
	else
		ferror.clear(port);

	if ((privkey.value || require_privkey) && !window.wireguard.validateBase64Key(privkey.value)) {
		ferror.set(privkey, 'A valid private key must be provided', !ok);
		ok = 0;
	}
	else
		ferror.clear(privkey);

	if (pubkey.value && !window.wireguard.validateBase64Key(pubkey.value)) {
		ferror.set(privkey, 'A valid public key must be provided', !ok);
		ok = 0;
	}
	else
		ferror.clear(pubkey);

	if (psk.value && !window.wireguard.validateBase64Key(psk.value)) {
		ferror.set(psk, 'A valid PresharedKey must be provided or left blank', !ok);
		ok = 0;
	}
	else
		ferror.clear(psk);

	if (E('_wg'+unit+'_com').value != 3) { /* !'External - VPN Provider' */
		if (!verifyCIDR(ip.value)) {
			ferror.set(ip, 'A valid CIDR (IP/MASK) must be provided to generate a configuration file', !ok);
			ok = 0;
		}
		else
			ferror.clear(ip);
	}
	else
		ferror.clear(ip);

	var ok = 1;
	if (allowedips.value != '') {
		var cidrs = allowedips.value.split(',')
		for(var i = 0; i < cidrs.length; i++) {
			var cidr = cidrs[i].trim();
			if (!verifyCIDR(cidr)) {
				ok = 0;
			}
		}
	}
	if (!ok) {
		ferror.set(allowedips, 'Allowed IPs must be in CIDR format separated by commas', !ok);
		ok = 0;
	}
	else
		ferror.clear(allowedips);

	/* verify peer keep alive */
	if (!v_range('_f_wg'+unit+'_peer_ka', !ok, 0, 99)) ok = 0;

	if (fwmark.value && !verifyFWMark(fwmark.value)) {
		ferror.set(fwmark, 'FWMark must be a hexadecimal number of 8 characters or 0', !ok);
		ok = 0;
	}
	else
		ferror.clear(fwmark);

	return ok;
}

function copyInterfacePubKey(unit) {
	const textArea = document.createElement('textarea');
	textArea.value = E('_wg'+unit+'_pubkey').value;

	/* move textarea out of the viewport so it's not visible */
	textArea.style.position = 'absolute';
	textArea.style.left = '-999999px';

	document.body.prepend(textArea);
	textArea.select();

	try {
		document.execCommand('copy');
	} catch (error) {
		console.error(error);
	} finally {
		textArea.remove();
	}
}

function generateInterfaceKey(unit) {
	var response = false;

	if (E('_wg'+unit+'_key').value == '')
		response = true;
	else
		response = confirm('Regenerating the interface Private Key will stop any\ndefined peers from communicating with this device!\n\nDo you want to continue?');

	if (response) {
		var keys = window.wireguard.generateKeypair();
		E('_wg'+unit+'_key').value = keys.privateKey;
		E('_wg'+unit+'_pubkey').value = keys.publicKey;
		updateForm(unit, 0);
		changed = 1;
	}
}

function peerFieldsToData(unit) {
	var alias = E('_f_wg'+unit+'_peer_alias').value;
	var endpoint = E('_f_wg'+unit+'_peer_ep').value;
	var privkey = E('_f_wg'+unit+'_peer_privkey').value;
	var pubkey = E('_f_wg'+unit+'_peer_pubkey').value;
	var psk = E('_f_wg'+unit+'_peer_psk').value;
	var ip = E('_f_wg'+unit+'_peer_ip').value;
	var allowedips = E('_f_wg'+unit+'_peer_aip').value;
	var keepalive = E('_f_wg'+unit+'_peer_ka').value;

	if (privkey != '')
		pubkey = window.wireguard.generatePublicKey(privkey);

	return [
		alias,
		endpoint,
		privkey,
		pubkey,
		psk,
		ip,
		allowedips,
		keepalive
	];
}

function clearPeerFields(unit) {
	var port = nvram['wg'+unit+'_port'];
	if (port == '')
		port = (51820 + unit);
	E('_f_wg'+unit+'_peer_alias').value = '';
	E('_f_wg'+unit+'_peer_ep').value = '';
	E('_f_wg'+unit+'_peer_port').value = port;
	E('_f_wg'+unit+'_peer_privkey').value = '';
	E('_f_wg'+unit+'_peer_privkey').disabled = 0;
	E('_f_wg'+unit+'_peer_pubkey').value = '';
	E('_f_wg'+unit+'_peer_pubkey').disabled = 0;
	E('_f_wg'+unit+'_peer_psk').value = '';
	E('_f_wg'+unit+'_peer_ip').value = '';
	E('_f_wg'+unit+'_peer_aip').value = '';
	E('_f_wg'+unit+'_peer_ka').value = '';
	E('_f_wg'+unit+'_peer_fwmark').value = '';

	var button = E('wg'+unit+'_peer_add');
	button.value = 'Add to Peers';
	button.setAttribute('onclick', 'addPeer('+unit+')');
}

function addPeer(unit, quiet) {
	if (!verifyPeerFields(unit))
		return;

	if (E('_wg'+unit+'_com').value == 3) { /* 'External - VPN Provider' - allow only one peer (us) */
		var rows = peerTables[unit].getAllData().length;
		if (rows > 0) {
			alert('In "External - VPN Provider" mode you can only add one peer (this router)')
			return;
		}
	}

	changed = 1;

	var data = peerFieldsToData(unit);
	peerTables[unit].insertData(-1, data);
	peerTables[unit].disableNewEditor(true);

	clearPeerFields(unit);
	updateForm(unit, 0);

	var qrcode = E('wg'+unit+'_qrcode');
	if (qrcode.style.display != 'none') {
		var row = qrcode.getAttribute('row_id');
		var content = genPeerGridConfig(unit, row);
		displayQRCode(content, unit, row);
	}
}

function editPeer(unit, rowIndex, quiet) {
	if (!verifyPeerFields(unit))
		return;

	changed = 1;

	var data = peerFieldsToData(unit);
	var row = peerTables[unit].tb.firstChild.rows[rowIndex];
	peerTables[unit].rowDel(row);
	peerTables[unit].insertData(rowIndex, data);
	peerTables[unit].disableNewEditor(true);

	clearPeerFields(unit);
	updateForm(unit, 0);

	var button = E('wg'+unit+'_peer_add');
	button.value = 'Add to Peers';
	button.setAttribute('onclick', 'addPeer('+unit+')');

	var qrcode = E('wg'+unit+'_qrcode');
	if (qrcode.style.display != 'none') {
		var row = qrcode.getAttribute('row_id');
		var content = genPeerGridConfig(unit, row);
		displayQRCode(content, unit, row);
	}
}

function verifyPeerGenFields(unit) {
	/* verify interface has a valid private key */
	if (!window.wireguard.validateBase64Key(nvram['wg'+unit+'_key'])) {
		alert('The interface must have a valid private key before peers can be generated')
		return false;
	}

	/* verify peer fwmark*/
	var fwmark = E('_f_wg'+unit+'_peer_fwmark').value;
	if (fwmark && !verifyFWMark(fwmark)) {
		alert('FWMark must be a hexadecimal number of 8 characters or 0')
		return false;
	}

	return true;
}

function generatePeer(unit) {
	/* verify peer gen fields have valid data */
	if (!verifyPeerGenFields(unit))
		return;

	/* generate keys */
	var keys = window.wireguard.generateKeypair();

	/* generate PSK (if checked) */
	var psk = '';
	if (E('_f_wg'+unit+'_peer_psk_gen').checked)
		psk = window.wireguard.generatePresharedKey();

	/* retrieve existing IPs of interface/peers to calculate new ip */
	var [interface_ip, interface_nm] = nvram['wg'+unit+'_ip'].split(',')[0].split('/', 2);
	var existing_ips = peerTables[unit].getAllData();
	existing_ips = existing_ips.map(x => x[5].split('/',1)[0]);
	existing_ips.push(interface_ip);

	/* calculate ip of new peer */
	var ip = '';
	var limit = 2 ** (32 - parseInt(interface_nm, 10));
	for (var i = 1; i < limit; i++) {
		var temp_ip = getAddress(ntoa(i), interface_ip);
		var end = temp_ip.split('.').slice(0, -1);

		if ((end >= '255') || (end == '0'))
			continue;

		if (existing_ips.includes(temp_ip))
			continue;

		ip = temp_ip;
		break;
	}

	/* return if we could not generate an IP */
	if (ip == '') {
		alert('Could not generate an IP for the peer');
		return;
	}

	if (E('_wg'+unit+'_com').value > 0) /* not 'Hub and Spoke' */
		var netmask = interface_nm;
	else
		var netmask = '32'; /* 'Hub and Spoke' */

	/* set fields with generated data */
	E('_f_wg'+unit+'_peer_privkey').value = keys.privateKey;
	E('_f_wg'+unit+'_peer_privkey').disabled = 0;
	E('_f_wg'+unit+'_peer_pubkey').value = keys.publicKey;
	E('_f_wg'+unit+'_peer_pubkey').disabled = 1;
	E('_f_wg'+unit+'_peer_psk').value = psk;
	E('_f_wg'+unit+'_peer_ip').value = ip+'/'+netmask;
	E('_f_wg'+unit+'_peer_ka').value = 0;

	var button = E('wg'+unit+'_peer_add');
	button.value = 'Add to Peers';
	button.setAttribute('onclick', 'addPeer('+unit+')');
}

function displayQRCode(content, unit) {
	var qrcode = E('wg'+unit+'_qrcode');
	var qrcode_content = content.join('');
	var image = showQRCode(qrcode_content);

	image.style.maxWidth = '700px';
	qrcode.replaceChild(image, qrcode.firstChild);

	elem.display('wg'+unit+'_qrcode', true);
}

function genPeerGridConfigQR(event, unit, row) {
	if (changed) {
		alert('Changes have been made. You need to save before continue!');
		return;
	}

	var qrcode = E('wg'+unit+'_qrcode');
	if (qrcode.getAttribute('row_id') == row && qrcode.style.display != 'none')
		elem.display('wg'+unit+'_qrcode', false);
	else {
		var content = genPeerGridConfig(unit, row);
		if (content != false) {
			displayQRCode(content, unit, row);
			qrcode.setAttribute('row_id', row);
		}
	}
	event.stopPropagation();
}

function genPeerGridConfigFile(event, unit, row, type) {
	if (changed) {
		alert('Changes have been made. You need to save before continue!');
		return;
	}

	var content = genPeerGridConfig(unit, row, type);
	if (content != false) {
		var filename = 'peer'+row+'.conf';
		var alias = peerTables[unit].tb.rows[row]._data[0];
		if (alias != '')
			filename = alias+'.conf';
		downloadConfig(content, filename);
	}
	event.stopPropagation();
}

function genPeerGridConfig(unit, row, type) {
	var port = E('_f_wg'+unit+'_peer_port');
	var fwmark = E('_f_wg'+unit+'_peer_fwmark');
	var row_data = peerTables[unit].tb.rows[row]._data;
	var ok = 1;

	clearPeerFields(unit);

	if (type == 'cfg' && !row_data[2]) {
		alert('The selected peer does not have a private key stored, which is require for configuration generation');
		ok = 0;
	}

	if ((!port.value.match(/^ *[-\+]?\d+ *$/)) || (port.value < 1) || (port.value > 65535)) {
		ferror.set(port, 'A valid port must be provided', !ok);
		ok = 0;
	}
	else
		ferror.clear(port);

	if (fwmark.value && !verifyFWMark(fwmark.value)) {
		ferror.set(fwmark, 'FWMark must be a hexadecimal number of 8 characters or 0', !ok);
		ok = 0;
	}
	else
		ferror.clear(fwmark);

	if (!ok)
		return 0;

	return generateWGConfig(unit, row_data[0], row_data[2], row_data[4], row_data[5].split('/')[0], port.value, fwmark.value, row_data[7], row_data[1]);
}

function generateWGConfig(unit, name, privkey, psk, ip, port, fwmark, keepalive, endpoint) {
	var [interface_ip, interface_nm] = nvram['wg'+unit+'_ip'].split(',', 1)[0].split('/', 2);
	var content = [];
	var dns = nvram['wg'+unit+'_peer_dns'];

	/* compat */
	if (keepalive == 'true')
		keepalive = '25';
	else if ((keepalive == 'false') || (keepalive == '0'))
		keepalive = false;

	/* build interface section */
	content.push('[Interface]\n');

	if (name != '')
		content.push('#Alias = '+name+'\n');

	content.push('Address = '+ip+'/'+interface_nm+'\n',
	             'ListenPort = '+port+'\n',
	             'PrivateKey = '+privkey+'\n');

	if (dns != '')
		content.push('DNS = '+dns+'\n')

	if (fwmark && fwmark != 0)
		content.push ('FwMark = 0x'+fwmark+'\n');

	/* build router peer */
	var publickey_interface = window.wireguard.generatePublicKey(nvram['wg'+unit+'_key']);
	var router_endpoint = nvram['wg'+unit+'_endpoint'];
	switch(router_endpoint[0]) {
	case '0':
		if (nvram.wan_domain) {
			router_endpoint = nvram.wan_domain;
			if (nvram.wan_hostname && nvram.wan_hostname != 'unknown')
				router_endpoint = nvram.wan_hostname+'.'+router_endpoint;
		}
		else
			router_endpoint = nvram.wan_ipaddr;
		break;
	case '1':
		router_endpoint = nvram.wan_ipaddr;
		break;
	case '2':
		router_endpoint = router_endpoint.split('|', 2)[1];
		break;
	}
	var port = nvram['wg'+unit+'_port'];
	if (port == '')
		port = (51820 + unit);
	router_endpoint += ':'+port;

	/* build allowed ips for router peer */
	var allowed_ips;
	if (nvram['wg'+unit+'_rgw'] == 1) /* forward all peer traffic? */
		allowed_ips = '0.0.0.0/0'
	else {
		if (nvram['wg'+unit+'_com'] > 0) /* not 'Hub and Spoke' */
			var netmask = interface_nm;
		else
			var netmask = '32'; /* 'Hub and Spoke' */

		allowed_ips = interface_ip+'/'+netmask;
		var other_ips_index = nvram['wg'+unit+'_ip'].indexOf(',');
		if (other_ips_index > -1) /* Interface IP has more IPs? */
			allowed_ips += nvram['wg'+unit+'_ip'].substring(other_ips_index);

		for (var i = 0; i <= MAX_BRIDGE_ID; ++i) {
			if ((E('_f_wg'+unit+'_lan'+i).checked) == 1) { /* push LANX to peer? */
				var t = (i == 0 ? '' : i);
				var nm = nvram['lan'+t+'_netmask'];
				var network_ip = getNetworkAddress(nvram['lan'+t+'_ipaddr'], nm);
				allowed_ips += ','+network_ip+'/'+netmaskToCIDR(nm);
			}
		}

		var interface_allowed_ips = nvram['wg'+unit+'_aip']; /* allowed IPs from Peer Parameters (Interface panel) */
		if (interface_allowed_ips != '')
			allowed_ips += ','+interface_allowed_ips;
	}

	/* populate router peer */
	var router_alias = 'Router';
	if (nvram.wan_hostname && nvram.wan_hostname != 'unknown')
		router_alias = nvram.wan_hostname;

	content.push('\n',
	             '[Peer]\n',
	             '#Alias = '+router_alias+'\n',
	             'PublicKey = '+publickey_interface+'\n');

	if (psk != '')
		content.push('PresharedKey = '+psk+'\n');

	content.push('AllowedIPs = '+allowed_ips+'\n',
	             'Endpoint = '+router_endpoint+'\n');

	if (keepalive)
		content.push('PersistentKeepalive = '+keepalive+'\n');

	/* add remaining peers to config */
	var pubkey = window.wireguard.generatePublicKey(privkey);
	if (nvram['wg'+unit+'_com'] > 0) { /* not 'Hub and Spoke' */
		var interface_peers = peerTables[unit].getAllData();

		for (var i = 0; i < interface_peers.length; ++i) {
			var peer = interface_peers[i];

			if (nvram['wg'+unit+'_com'] == 1 && endpoint == '' && peer[2] == '')
				continue;

			var peer_pubkey = peer[3];
			if (peer[0] == 1)
				peer_pubkey = window.wireguard.generatePublicKey(peer_pubkey);
			if (peer_pubkey == pubkey)
				continue;

			content.push('\n',
			             '[Peer]\n');

			if (peer[1].trim() != '')
				content.push('#Alias = '+peer[1]+'\n');

			content.push('PublicKey = '+peer_pubkey+'\n');

			if (peer[4].trim() != '')
				content.push('PresharedKey = '+peer[4]+'\n');

			content.push('AllowedIPs = '+peer[5]);

			if (peer[6].trim() != '')
				content.push(','+peer[6]);
			content.push('\n');

			if (keepalive)
				content.push('PersistentKeepalive = '+keepalive+'\n');

			if (peer[2].trim() != '') {
				/* FQDN or IPv4 */
				if (peer[2].indexOf('.') >= 0 && peer[2].indexOf(':') >= 0) /* has port */
					content.push('Endpoint = '+peer[2]+'\n');
				/* IPv6 */
				else if (peer[2].indexOf('[') >= 0 && peer[2].indexOf(']:') >= 0) /* has port */
					content.push('Endpoint = '+peer[2]+'\n');
				else
					content.push('Endpoint = '+peer[2]+':'+port+'\n');
			}
		}
	}

	return content;
}

function downloadConfig(content, name) {
	const link = document.createElement('a');
	const file = new Blob(content, { type: 'text/plain' });
	link.href = URL.createObjectURL(file);
	link.download = name;
	link.click();
	URL.revokeObjectURL(link.href);
}

function downloadAllConfigs(event, unit) {
	if (changed) {
		alert('Changes have been made. You need to save before continue!');
		return;
	}

	for (var i = 1; i < peerTables[unit].tb.rows.length; ++i) {
		var privkey = peerTables[unit].tb.rows[i].getRowData()[2];
		if (privkey != '')
			genPeerGridConfigFile(event, unit, i);
	}
}

function updateStatus(unit) {
	var result = E('wg'+unit+'_result');
	elem.setInnerHTML(result, '');
	spin(1, 'wg'+unit+'_status_wait');

	cmd = new XmlHttp();
	cmd.onCompleted = function(text, xml) {
		var cmdresult;
		var output;
		eval(text);
		if (cmdresult == 'Unable to access interface: No such device\n' || cmdresult == 'Unable to access interface: Protocol not supported\n') {
			output = 'ERROR: Wireguard device wg'+unit+' does not exist!';
		}
		else {
			var [iface, peers] = decodeDump(cmdresult, unit);
			output = encodeStatus(iface, peers);
		}
		displayStatus(unit, output);
	}
	cmd.onError = function(x) {
		var text = 'ERROR: '+x;
		displayStatus(unit, text);
	}

	var c = '/usr/sbin/wg show wg'+unit+' dump\n';

	cmd.post('shell.cgi', 'action=execute&command='+escapeCGI(c.replace(/\r/g, '')));
}

function decodeDump(dump, unit) {
	var iface;
	var peers = [];
	var lines = dump.split('\n');

	var sections = lines.shift().split('\t');
	iface = {
		'name': 'wg'+unit,
		'alias': 'This Router',
		'privkey': sections[0],
		'pubkey': sections[1],
		'port': sections[2],
		'fwmark': sections[3] == 'off' ? null : sections[3]
	};

	var nvram_peers = decodePeers(unit);

	for (var i = 0; i < lines.length; ++i) {
		var line = lines[i];
		if (line == '')
			continue;
		var line = lines[i].split('\t');
		var peer = {
			'alias': null,
			'pubkey': line[0],
			'psk': line[1] == '(none)' ? null : line[1],
			'endpoint': line[2] == '(none)' ? null : line[2],
			'allowed_ips': line[3],
			'handshake': line[4] == '0' ? null : line[4],
			'rx': line[5],
			'tx': line[6],
			'keepalive': line[7] == 'off' ? null : line[7]
		};
		for (var j = 0; j < nvram_peers.length; ++j) {
			var nvram_peer = nvram_peers[j];
			if (nvram_peer.pubkey == peer.pubkey && nvram_peer.alias != '') {
				peer.alias = nvram_peer.alias;
				break;
			}
		}
		peers.push(peer);
	}

	return [iface, peers];
}

function encodeStatus(iface, peers) {
	/* add interface status */
	var output = 'interface: '+iface.name+'\n';
	output += '  alias: '+iface.alias+'\n';
	output += '  public key: '+iface.pubkey+'\n';
	output += '  listening port: '+iface.port+'\n';
	if (iface.fwmark)
		output += '  fwmark: '+iface.fwmark+'\n';

	/* add peer statuses */
	for (var i = 0; i < peers.length; ++i) {
		var peer = peers[i];
		output +='\n';
		output += 'peer: '+peer.pubkey+'\n';
		if (peer.alias)
			output += '  alias: '+peer.alias+'\n';
		if (peer.psk)
			output += '  preshared key: (hidden)\n';
		if (peer.endpoint)
			output += '  endpoint: '+peer.endpoint+'\n';
		output += '  allowed ips: '+peer.allowed_ips+'\n';
		if (peer.handshake) {
			var seconds = Math.floor(Date.now()/1000 - peer.handshake);
			output += '  latest handshake: '+seconds+' seconds ago\n';
			output += '  transfer: '+formatBytes(peer.rx)+' received, '+formatBytes(peer.tx)+' sent\n';
		}
		if (peer.keepalive)
			output += '  persistent keepalive: every '+peer.keepalive+' seconds\n';
	}

	return output;
}

function formatBytes(bytes) {
	var output;

	if (bytes < 1024)
		output = bytes+' B';
	else if (bytes < 1024 * 1024)
		output = Math.floor(bytes/1024)+' KB';
	else if (bytes < 1024 * 1024 * 1024)
		output = Math.floor(bytes/(1024*1024))+' MB';
	else if (bytes < 1024 * 1024 * 1024 * 1024)
		output = Math.floor(bytes/(1024*1024*1024))+' GB';
	else
		output = Math.floor(bytes/(1024*1024*1024*1024))+' TB';

	return output;
}

function spin(x, which) {
	E(which).style.display = (x ? 'inline-block' : 'none');
}

function displayStatus(unit, text) {
	elem.setInnerHTML(E('wg'+unit+'_result'), escapeText(text));
	spin(0, 'wg'+unit+'_status_wait');
}

function netmaskToCIDR(mask) {
	var maskNodes = mask.match(/(\d+)/g);
	var cidr = 0;
	for (var i in maskNodes)
		cidr += (((maskNodes[i] >>> 0).toString(2)).match(/1/g) || []).length;

	return cidr;
}
/*
function CIDRToNetmask(bitCount) {
	var mask = [];
	for (var i = 0; i < 4; i++) {
		var n = Math.min(bitCount, 8);
		mask.push(256 - Math.pow(2, 8 - n));
		bitCount -= n;
	}

	return mask.join('.');
}

function isv6(ip) {
	return ip.indexOf(':') < 0;
}
*/
function mayClose(event) {
	if (changed) {
		var confirmationMessage = 'If you leave before saving, your changes will be lost';

		(event || window.event).returnValue = confirmationMessage;
		return confirmationMessage;
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

	for (var unit = 0; unit < tabs.length; ++unit) {
		if (routingTables[unit] == this)
			updateForm(unit, 1);
	}
}

RouteGrid.prototype.verifyFields = function(row, quiet) {
	changed = 1;
	var ok = 1;

	for (var unit = 0; unit < tabs.length; ++unit) {
		if (routingTables[unit] == this)
			updateForm(unit, 1);
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

function verifyCIDR(cidr) {
	return cidr.match(/(([1-9]{0,1}[0-9]{0,2}|2[0-4][0-9]|25[0-5])\.){3}([1-9]{0,1}[0-9]{0,2}|2[0-4][0-9]|25[0-5])\/[0-9]|([1-2][0-9]|3[0-2])/)
}

function verifyDNS(dns) {
	var ok = true;
	var ips = dns.split(',');
	for (var i = 0; i < ips.length; i++) {
		var ip = ips[i].trim();
		if (!ip.match(/(([1-9]{0,1}[0-9]{0,2}|2[0-4][0-9]|25[0-5])\.){3}([1-9]{0,1}[0-9]{0,2}|2[0-4][0-9]|25[0-5])/)) {
			ok = false;
			break;
		}
	}

	return ok;
}

function verifyFWMark(fwmark) {
	return fwmark == '0' || fwmark.match(/[0-9A-Fa-f]{8}/);
}

function verifyFields(focused, quiet) {
	var i, ok = 1;
	tgHideIcons();
	var externalall_mode_enabled = -1;
	var restart = 1;

	for (i = 0; i < WG_INTERFACE_COUNT; i++) {
		if (focused && focused == E('_f_wg'+i+'_enable')) /* except on/off */
			restart = 0;
	}

	/* When settings change, make sure we restart the right service */
	if (focused) {
		changed = 1;

		var fom = E('t_fom');
		var unitidx = focused.name.indexOf('wg');
		if (unitidx >= 0) {
			var num = focused.name.substring(unitidx + 2, unitidx + 3);

			/* check for active 'External - VPN Provider' + 'Redirect Internet traffic' set to 'All' + 'Enable On Start' in focused */
			if (E('_wg'+num+'_com').value == 3 && E('_wg'+num+'_rgwr').value == 1 && E('_f_wg'+num+'_enable').checked) /* enabled */
				externalall_mode_enabled = num;

			if (restart) { /* restart, except on/off */
				/* check if we need to restart firewall */
				if ((nvram['wg'+num+'_com'] < 3 && E('_wg'+num+'_com').value == 3) || (nvram['wg'+num+'_com'] == 3 && E('_wg'+num+'_com').value < 3) ||
				    ((nvram['wg'+num+'_rgwr'] == 1 && E('_wg'+num+'_rgwr').value > 1) || (nvram['wg'+num+'_rgwr'] > 1 && E('_wg'+num+'_rgwr').value == 1))
				)
					updateForm(num, 1);
				else
					updateForm(num, 0);

				if (focused.name.indexOf('_adns') >= 0 && fom._service.value.indexOf('dnsmasq') < 0) {
					if (fom._service.value != '')
						fom._service.value += ',';

					fom._service.value += 'dnsmasq-restart';
				}
			}
		}
	}

	for (i = 0; i < WG_INTERFACE_COUNT; i++) {
		/* check for active 'External - VPN Provider' + 'Redirect Internet traffic' set to 'All' + 'Enable On Start' */
		if (E('_wg'+i+'_com').value == 3 && E('_wg'+i+'_rgwr').value == 1 && E('_f_wg'+i+'_enable').checked) { /* enabled */
			if (externalall_mode_enabled != -1 && externalall_mode_enabled != i) {
				alert('Only one wireguard instance can be run on router start ("Enable on Start" option) in "External - VPN Provider" mode with "Redirect Internet traffic" set to "All"!');
				E('_f_wg'+externalall_mode_enabled+'_enable').checked = 0;
				return;
			}
		}

		if (!v_range('_wg'+i+'_poll', quiet || !ok, 0, 30))
			ok = 0;

		/* verify valid port */
		var port = E('_wg'+i+'_port');
		if (port.value != '' && !v_port('_wg'+i+'_port', quiet || !ok)) {
			ferror.set(port, 'The interface port must be a valid port', quiet || !ok);
			ok = 0;
		}
		else {
			ferror.clear(port);
			if (port.value == '')
				E('_f_wg'+i+'_peer_port').value = (51820 + i);
			else
				E('_f_wg'+i+'_peer_port').value = port.value;
		}

		/* verify priority */
		var priority = E('_wg'+i+'_prio');
		if (priority.value != '' && !v_range('_wg'+i+'_prio', quiet || !ok, 1, 32766)) {
			ferror.set(priority, 'The priority must be in the range 1 - 32766', quiet || !ok);
			ok = 0;
		}
		else
			ferror.clear(priority);

		/* disable lan checkbox if lan is not in use */
		for (var j = 0; j <= MAX_BRIDGE_ID; ++j) {
			t = (j == 0 ? '' : j);

			if (nvram['lan'+t+'_ifname.length'] < 1) {
				E('_f_wg'+i+'_lan'+j).checked = 0;
				E('_f_wg'+i+'_lan'+j).disabled = 1;
			}
		}

		/* verify interface private key */
		var privkey = E('_wg'+i+'_key');
		if (privkey.value != '' && !window.wireguard.validateBase64Key(privkey.value)) {
			ferror.set(privkey, 'A valid private key is required for the interface', quiet || !ok);
			ok = 0;
		}
		else
			ferror.clear(privkey);

		/* calculate interface pubkey */
		E('_wg'+i+'_pubkey').disabled = true;
		var pubkey = window.wireguard.generatePublicKey(privkey.value);
		if (pubkey == false)
			pubkey = '';

		E('_wg'+i+'_pubkey').value = pubkey;

		/* autopopulate IP if it's empty */
		var ip = E('_wg'+i+'_ip');
		if (ip.value == '') {
			ip.value = '10.'+(10 + i)+'.0.1/24';
			ferror.clear(ip);
		}
		/* otherwise verify interface CIDR address */
		else {
			var ip_valid = true;
			if (ip.value != '') {
				var cidrs = ip.value.split(',')
				for (var j = 0; j < cidrs.length; j++) {
					var cidr = cidrs[j].trim();
					if (!cidr.match(/^([0-9]{1,3}\.){3}[0-9]{1,3}(\/([0-9]|[1-2][0-9]|3[0-2]))?$/)) {
						ip_valid = false;
						break;
					}
				}
			}
			if (!ip_valid) {
				ferror.set(ip, 'The interface IPs must be a comma separated list of valid CIDRs', quiet || !ok);
				ok = 0;
			}
			else
				ferror.clear(ip);
		}

		var fw = E('_wg'+i+'_firewall').value;
		var nat = E('_f_wg'+i+'_nat').checked;
		var ext = E('_wg'+i+'_com').value == 3; /* 'External - VPN Provider' */
		var rgwr = (E('_wg'+i+'_rgwr').value == 2 || E('_wg'+i+'_rgwr').value == 3);
		if (ext) E('_f_wg'+i+'_peer_ip').value = '';
		if (ext) E('_f_wg'+i+'_route').value = '1';
		E('_f_wg'+i+'_peer_ip').disabled = ext;
		elem.display('wg'+i+'-peers-download', !ext);
		elem.display('wg'+i+'-peers-generate-title', !ext);
		elem.display('wg'+i+'-peers-generate', !ext);
		elem.display(PR('_wg'+i+'_firewall'), ext);
		elem.display(PR('_f_wg'+i+'_route'), !ext);
		elem.display(PR('_f_wg'+i+'_nat'), fw != 'custom' && ext);
		elem.display(PR('_f_wg'+i+'_fw'), fw != 'custom' && ext);
		elem.display(PR('_wg'+i+'_rgwr'), ext);
		elem.display(E('wg'+i+'_nat_warn_text'), ext && !nat);
		elem.display(PR('_wg'+i+'_prio'), ext && rgwr);

		/* Page Routing Policy */
		elem.display(E('_wg'+i+'_routing_div_help'), (!ext) || (ext && !rgwr));

		/* verify interface dns */
		var dns = E('_wg'+i+'_dns');
		if (dns.value != '' && !verifyDNS(dns.value)) {
			ferror.set(dns, 'DNS Servers must be a comma separated list of IPs');
			ok = 0;
		}
		else
			ferror.clear(dns);

		/* verify interface fwmark */
		var fwmark = E('_wg'+i+'_fwmark');
		if (fwmark.value && !verifyFWMark(fwmark.value)) {
			ferror.set(fwmark, 'The interface FWMark must be a hexadecimal number of 8 characters or 0', quiet || !ok);
			ok = 0;
		}
		else
			ferror.clear(fwmark);

		/* autopopulate mtu if it's empty */
		var mtu = E('_wg'+i+'_mtu');
		if (mtu.value == '') {
			mtu.value = '1420';
			ferror.clear(mtu);
		}
		/* otherwise verify interface mtu */
		else {
			if ((!mtu.value.match(/^ *[-\+]?\d+ *$/)) || (mtu.value < 0) || (mtu.value > 1500)) {
				ferror.set(mtu, 'The interface MTU must be a integer between 0 and 1500', quiet || !ok);
				ok = 0;
			}
			else
				ferror.clear(mtu);
		}

		/* hide/show custom endpoint based on option selected */
		var endpoint = E('_f_wg'+i+'_endpoint');
		var custom_endpoint = E('_f_wg'+i+'_custom_endpoint');
		elem.display(custom_endpoint, (endpoint.value == 2));

		/* hide/show custom table based on option selected */
		var route = E('_f_wg'+i+'_route');
		var custom_table = E('_f_wg'+i+'_custom_table');
		if (route.value == 2) {
			elem.display(custom_table, true);
			if (!custom_table.value.match(/^ *[-\+]?\d+ *$/))
				ferror.set(custom_table, 'The custom table must be an integer', quiet || !ok);
			else
				ferror.clear(custom_table);
		}
		else {
			elem.display(custom_table, false);
			ferror.clear(custom_table);
		}

		/* verify interface keep alive */
		if (!v_range('_wg'+i+'_ka', quiet || !ok, 0, 99)) ok = 0;

		/* verify peer dns */
		var peer_dns = E('_wg'+i+'_peer_dns');
		if (peer_dns.value != '' && !verifyDNS(peer_dns.value)) {
			ferror.set(peer_dns, 'DNS Servers must be a comma separated list of IPs');
			ok = 0;
		}
		else
			ferror.clear(peer_dns);

		/* verify interface allowed ips */
		var allowed_ips = E('_wg'+i+'_aip')
		var aip_valid = true;
		if (allowed_ips.value != '') {
			var cidrs = allowed_ips.value.split(',')
			for (var j = 0; j < cidrs.length; j++) {
				var cidr = cidrs[j].trim();
				if (!cidr.match(/^([0-9]{1,3}\.){3}[0-9]{1,3}(\/([0-9]|[1-2][0-9]|3[0-2]))?$/)) {
					aip_valid = false;
					break;
				}
			}
		}
		if (!aip_valid) {
			ferror.set(allowed_ips, 'The interface allowed ips must be a comma separated list of valid CIDRs', quiet || !ok);
			ok = 0;
		}
		else
			ferror.clear(allowed_ips);

		/*** peer key checking stuff ***/
		var peer_privkey = E('_f_wg'+i+'_peer_privkey');
		var peer_pubkey = E('_f_wg'+i+'_peer_pubkey');
		peer_privkey.disabled = false;
		peer_pubkey.disabled = false;

		/* if private key is populated (and valid), generate the public key and lock it */
		if (peer_privkey.value) {
			var pubkey_temp = window.wireguard.generatePublicKey(peer_privkey.value);
			if (pubkey_temp) {
				peer_pubkey.disabled = true;
				peer_pubkey.value = pubkey_temp;
			}
		}
	}

	return ok;
}

function save(nomsg) {
	if (!verifyFields(null, 0))
		return;

	if (!nomsg) show(); /* update '_service' field first */

	E('wg_adns').value = '';
	var fom = E('t_fom');
	for (var i = 0; i < WG_INTERFACE_COUNT; i++) {
		if (routingTables[i].isEditing())
			return;

		var privkey = E('_wg'+i+'_key').value;
		nvram['wg'+i+'_key'] = privkey;

		var data = peerTables[i].getAllData();
		var s = '';
		for (var j = 0; j < data.length; ++j)
			s += data[j].join('<')+'>';

		fom['wg'+i+'_peers'].value = s;
		nvram['wg'+i+'_peers'] = s;

		var routedata = routingTables[i].getAllData();
		s = '';
		for (j = 0; j < routedata.length; ++j)
			s += routedata[j].join('<')+'>';

		fom['wg'+i+'_routing_val'].value = s;
		fom['wg'+i+'_enable'].value = fom['_f_wg'+i+'_enable'].checked ? 1 : 0;
		fom['wg'+i+'_rgw'].value = fom['_f_wg'+i+'_rgw'].checked ? 1 : 0;
		fom['wg'+i+'_nat'].value = fom['_f_wg'+i+'_nat'].checked ? 1 : 0;
		fom['wg'+i+'_fw'].value = fom['_f_wg'+i+'_fw'].checked ? 1 : 0;

		/* copy values from the fields */
		nvram['wg'+i+'_rgwr'] = E('_wg'+i+'_rgwr').value;
		nvram['wg'+i+'_com'] = E('_wg'+i+'_com').value;

		/* set properly value of Push LANX to peers: bit 0 = LAN0, bit 1 = LAN1, bit 2 = LAN2, bit 3 = LAN3 */
		fom['wg'+i+'_lan'].value = 0; /* init with 0 and check */
		if (fom['_f_wg'+i+'_lan0'].checked)
			fom['wg'+i+'_lan'].value |= 1;
		if (fom['_f_wg'+i+'_lan1'].checked)
			fom['wg'+i+'_lan'].value |= 2;
		if (fom['_f_wg'+i+'_lan2'].checked)
			fom['wg'+i+'_lan'].value |= 4;
		if (fom['_f_wg'+i+'_lan3'].checked)
			fom['wg'+i+'_lan'].value |= 8;
/* TOMATO64-BEGIN */
		if (fom['_f_wg'+i+'_lan4'].checked)
			fom['wg'+i+'_lan'].value |= 16;
		if (fom['_f_wg'+i+'_lan5'].checked)
			fom['wg'+i+'_lan'].value |= 32;
		if (fom['_f_wg'+i+'_lan6'].checked)
			fom['wg'+i+'_lan'].value |= 64;
		if (fom['_f_wg'+i+'_lan7'].checked)
			fom['wg'+i+'_lan'].value |= 128;
/* TOMATO64-END */

		if (E('_f_wg'+i+'_adns').checked)
			E('wg_adns').value += i+',';

		var endpoint = E('_f_wg'+i+'_endpoint');
		var custom_endpoint = E('_f_wg'+i+'_custom_endpoint');
		var endpoint_output = endpoint.value+'';
		if (endpoint.value == 2)
			endpoint_output += '|'+custom_endpoint.value;

		fom['wg'+i+'_endpoint'].value = endpoint_output;
		nvram['wg'+i+'_endpoint'] = endpoint_output;

		var route = E('_f_wg'+i+'_route');
		var custom_table = E('_f_wg'+i+'_custom_table');
		var route_output = route.value+'';
		if (route.value == 2)
			route_output += '|'+custom_table.value;

		fom['wg'+i+'_route'].value = route_output;
		nvram['wg'+i+'_route'] = route_output;

		var qrcode = E('wg'+i+'_qrcode');
		if (qrcode.style.display != 'none') {
			var row = qrcode.getAttribute('row_id');
			var content = genPeerGridConfig(i, row);
			displayQRCode(content, i, row);
		}
	}
	fom._nofootermsg.value = 0;

	form.submit(fom, 1);

	changed = 0;
	fom._service.value = '';
}

function earlyInit() {
	show();
	var tab = cookie.get(cprefix+'_tab') || tabs[0][0];
	for (var i = 0; i < tabs.length; ++i) {
		sectSelect(i, cookie.get(cprefix+i+'_section') || sections[0][0]);

		var t = tabs[i][0];

		routingTables[i].init(t+'_table_routing','sort', 0,[ { type: 'checkbox', prefix: '<div class="centered">', suffix: '<\/div>' },
		                                                        { type: 'select', options: [[1,'From Source IP'],[2,'To Destination IP'],[3,'To Domain']] },
		                                                        { type: 'text', maxlen: 50 },
		                                                        { type: 'checkbox', prefix: '<div class="centered">', suffix: '<\/div>' }]);
		routingTables[i].headerSet(['Enable','Type','Value','Kill Switch']);
		var routingVal = nvram[t+'_routing_val'];
		if (routingVal.length) {
			var s = routingVal.split('>');
			for (var j = 0; j < s.length; ++j) {
				if (!s[j].length)
					continue;

				var row = s[j].split('<');
				routingTables[i].insertData(-1, row);
			}
		}
		routingTables[i].showNewEditor();
		routingTables[i].resetNewEditor();
	}
	tabSelect(tab);
	verifyFields(null, 1);
	insOvl();
}

function init() {
	eventHandler();
	addEvent(window, 'beforeunload', mayClose);

	up.initPage(250, 5);
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

<input type="hidden" name="_service" value="">
<input type="hidden" name="_nofootermsg">
<input type="hidden" name="wg_adns" id="wg_adns">

<!-- / / / -->

<div class="section-title">Status</div>
<div class="section">
	<div class="fields">
		<script>
			for (i = 0; i < tabs.length; ++i) {
				t = tabs[i][0];

				W('<div id="'+t+'-wg-status-button">');
				W('<span id="_wireguard'+i+'_notice"><\/span>');
				W('<input type="button" id="_wireguard'+i+'_button">&nbsp; <img src="spin.svg" alt="" id="spin'+i+'">');
				W('<\/div>');
			}
		</script>
	</div>
</div>

<!-- / / / -->

<div class="section-title"><span class="wireguardsvg">&nbsp;</span>Wireguard Configuration</div>
<div class="section">
	<script>
		tabCreate.apply(this, tabs);

		for (i = 0; i < tabs.length; ++i) {
			t = tabs[i][0];
			W('<div id="'+t+'-wg-tab">');
			W('<input type="hidden" name="'+t+'_enable">');
			W('<input type="hidden" name="'+t+'_lan">');
			W('<input type="hidden" name="'+t+'_rgw">');
			W('<input type="hidden" name="'+t+'_endpoint">');
			W('<input type="hidden" name="'+t+'_peers">');
			W('<input type="hidden" name="'+t+'_route">');
			W('<input type="hidden" name="'+t+'_nat">');
			W('<input type="hidden" name="'+t+'_fw">');
			W('<input type="hidden" name="'+t+'_routing_val">');

			W('<ul class="tabs">');
			for (j = 0; j < sections.length; j++) {
				W('<li><a href="javascript:sectSelect('+i+',\''+sections[j][0]+'\')" id="'+t+'-'+sections[j][0]+'-wg-tab">'+sections[j][1]+'<\/a><\/li>');
			}
			W('<\/ul><div class="tabs-bottom"><\/div>');

			/* config tab start */
			W('<div id="'+t+'-wg-config">');
			W('<div class="section-title">Interface<\/div>');
			createFieldTable('', [
				{ title: 'Enable on Start', name: 'f_'+t+'_enable', type: 'checkbox', value: nvram[t+'_enable'] == 1 },
				{ title: 'Poll Interval', name: t+'_poll', type: 'text', maxlen: 2, size: 5, value: nvram[t+'_poll'], suffix: ' <small>minutes; 0 to disable<\/small>' },
				{ title: 'Config file', name: t+'_file', type: 'text', placeholder: 'optional', maxlen: 64, size: 64, value: nvram[t+'_file'] },
				{ title: 'Port', name: t+'_port', type: 'text', maxlen: 5, size: 10, placeholder: (51820+i), value: nvram[t+'_port'] },
				{ title: 'Private Key', multi: [
					{ title: '', name: t+'_key', type: 'password', maxlen: 44, size: 48, value: nvram[t+'_key'], peekaboo: 1 },
					{ title: '', custom: '<input type="button" value="Generate" onclick="generateInterfaceKey('+i+')" id="'+t+'_keygen">' }
				] },
				{ title: 'Public Key', multi: [
					{ title: '', name: t+'_pubkey', type: 'text', maxlen: 10, size: 48, disabled: ""},
					{ title: '', custom: '<input type="button" value="Copy" onclick="copyInterfacePubKey('+i+')" id="'+t+'_pubkey_copy">' }
				] },
				{ title: 'VPN Interface IP', name: t+'_ip', type: 'text', maxlen: 32, size: 17, value: nvram[t+'_ip'], placeholder: 'CIDR format' },
				{ title: 'DNS Servers (out)', name: t+'_dns', type: 'text', maxlen: 128, size: 64, value: nvram[t+'_dns'], placeholder: 'comma separated' },
				{ title: 'FWMark', name: t+'_fwmark', type: 'text', maxlen: 8, size: 8, value: nvram[t+'_fwmark'] },
				{ title: 'MTU', name: t+'_mtu', type: 'text', maxlen: 4, size: 4, value: nvram[t+'_mtu'] },
				{ title: 'Respond to DNS', name: 'f_'+t+'_adns', type: 'checkbox', suffix: '&nbsp;<small>enables dnsmasq to resolve queries arriving on this interface<\/small>', value: nvram.wg_adns.indexOf(''+i) >= 0 },
				{ title: 'Routing Mode', name: 'f_'+t+'_route', type: 'select', options: [['0','Off'],['1','Auto'],['2','Custom Table']], value: nvram[t+'_route'][0] || 1, suffix: '&nbsp;<input type="text" name="f_'+t+'_custom_table" value="'+(nvram[t+'_route'].split('|', 2)[1] || '')+'" onchange="verifyFields(this, 1)" id="_f_'+t+'_custom_table" maxlength="32" size="32">' },
				{ title: 'Firewall', name: t+'_firewall', type: 'select', options: [['auto','Automatic'],['custom','Custom']], value: nvram[t+'_firewall'] },
				{ title: 'Create NAT on tunnel', indent: 2, name: 'f_'+t+'_nat', type: 'checkbox', value: nvram[t+'_nat'] != 0, suffix: ' <small id="'+t+'_nat_warn_text">routes must be configured manually<\/small>' },
				{ title: 'Inbound Firewall', indent: 2, name: 'f_'+t+'_fw', type: 'checkbox', value: nvram[t+'_fw'] != 0 },
				null,
				{ title: 'Type of VPN', name: t+'_com', type: 'select', options: [['0','Internal - Hub (this device) and Spoke (peers)'],['1','Internal - Full Mesh (defined Endpoint only)'],['2','Internal - Full Mesh'],['3','External - VPN Provider']], value: nvram[t+'_com'] || 0 },
				{ title: 'Redirect Internet traffic', name: t+'_rgwr', type: 'select', options: [[1,'All'],[2,'Routing Policy'],[3,'Routing Policy (strict)']], value: nvram[t+'_rgwr'] },
				{ title: 'Priority', indent: 2, name: t+'_prio', type: 'text', maxlen: 5, size: 5, placeholder: (100 + i), suffix: '&nbsp;<small>(1 - 32766) lower number = higher priority<\/small>', value: nvram[t+'_prio'] },
				{ title: 'Import Config from File', indent: 2, custom: '<input type="file" class="import-file" id="'+t+'_config_file" accept=".conf" name="Browse File"><input type="button" id="'+t+'_config_import" value="Import" onclick="loadConfig('+i+')">' },
				{ title: '', custom: '<div>Note: before importing the configuration, set the correct "Type of VPN" above.<\/div>' }
			]);
			W('<br><\/div>');
			/* config tab stop */

			/* peers params tab start */
			W('<div id="'+t+'-wg-peersp">');
			W('<div class="section-title">Peers Parameters <span style="font-size:0.7em">(used only to generate peer config files)<\/span><\/div>');
			createFieldTable('', [
				{ title: 'Router behind NAT', name: t+'_ka', type: 'text', maxlen: 2, size: 4, suffix: '&nbsp;<small>configures keepalive interval from this router towards the defined peers (0=disable/no NAT, 10-99s range, 25 is a common setting)<\/small>', value: nvram[t+'_ka'] },
				{ title: 'Endpoint', name: 'f_'+t+'_endpoint', type: 'select', options: [['0','FQDN'],['1','WAN IP'],['2','Custom Endpoint']], value: nvram[t+'_endpoint'][0] || 0, suffix: '&nbsp;<input type="text" name="f_'+t+'_custom_endpoint" value="'+(nvram[t+'_endpoint'].split('|', 2)[1] || '')+'" onchange="verifyFields(this, 1)" id="_f_'+t+'_custom_endpoint" maxlength="64" size="46">' },
				{ title: 'Allowed IPs', name: t+'_aip', type: 'text', placeholder: 'CIDR format / comma separated', maxlen: 128, size: 64, value: nvram[t+'_aip'] },
				{ title: 'DNS Servers for Peers', name: t+'_peer_dns', type: 'text', maxlen: 128, size: 64, placeholder: 'comma separated', value: nvram[t+'_peer_dns'] },
				{ title: 'Push LAN0 (br0) to peers', name: 'f_'+t+'_lan0', type: 'checkbox', value: (nvram[t+'_lan'] & 0x01) },
				{ title: 'Push LAN1 (br1) to peers', name: 'f_'+t+'_lan1', type: 'checkbox', value: (nvram[t+'_lan'] & 0x02) },
				{ title: 'Push LAN2 (br2) to peers', name: 'f_'+t+'_lan2', type: 'checkbox', value: (nvram[t+'_lan'] & 0x04) },
				{ title: 'Push LAN3 (br3) to peers', name: 'f_'+t+'_lan3', type: 'checkbox', value: (nvram[t+'_lan'] & 0x08) },
/* TOMATO64-BEGIN */
				{ title: 'Push LAN4 (br4) to peers', name: 'f_'+t+'_lan4', type: 'checkbox', value: (nvram[t+'_lan'] & 0x10) },
				{ title: 'Push LAN5 (br5) to peers', name: 'f_'+t+'_lan5', type: 'checkbox', value: (nvram[t+'_lan'] & 0x20) },
				{ title: 'Push LAN6 (br6) to peers', name: 'f_'+t+'_lan6', type: 'checkbox', value: (nvram[t+'_lan'] & 0x40) },
				{ title: 'Push LAN7 (br7) to peers', name: 'f_'+t+'_lan7', type: 'checkbox', value: (nvram[t+'_lan'] & 0x80) },
/* TOMATO64-END */
				{ title: 'Forward all peer traffic', name: 'f_'+t+'_rgw', type: 'checkbox', value: nvram[t+'_rgw'] == 1 }
			]);
			W('<br><\/div>');
			/* peers params tab stop */

			/* peers tab start */
			W('<div id="'+t+'-wg-peers">');
			W('<div class="section-title">Peers<\/div>');
			W('<div class="tomato-grid" id="'+t+'-peers-grid"><\/div>');
			peerTables[i].setup();
			W('<div id="'+t+'-peers-download">');
			W('<input type="button" value="Download All Configs" onclick="downloadAllConfigs(event,'+i+')" id="'+t+'_download_all">');
			W('<br>');
			W('<\/div>');
			W('<div id="'+t+'_qrcode" class="qrcode" style="display:none">');
			W('<img src="qr-icon.svg" alt="'+t+'_qrcode_img" style="max-width:100px">');
			W('<div id="'+t+'_qrcode_labels" class="qrcode-labels" title="Message">Point your mobile phone camera <br>here above to connect automatically<\/div>');
			W('<\/div>');

			W('<div id="'+t+'-dummy" style="display:none">');
			createFieldTable('', [
				{ title: 'Port', name: 'f_'+t+'_peer_port', type: 'text', maxlen: 5, size: 10, value: nvram[t+'_port'] == '' ? (51820 + i) : nvram[t+'_port'], hidden: 1 },
				{ title: 'FWMark', name: 'f_'+t+'_peer_fwmark', type: 'text', maxlen: 8, size: 8, value: '0', hidden: 1 }
			]);
			W('<\/div><br>');

			W('<div class="section-title" id="'+t+'-peers-generate-title">Peer Generation<\/div>');
			W('<div id="'+t+'-peers-generate">');
			createFieldTable('', [
				{ title: 'Generate PSK', name: 'f_'+t+'_peer_psk_gen', type: 'checkbox', value: true, suffix: '&nbsp;<small>strenghten encyption with PresharedKey<\/small>' },
				{ title: '', custom: '<input type="button" value="Generate Peer" onclick="generatePeer('+i+')" id="'+t+'_peer_gen">' }
			]);
			W('<br><\/div>');

			W('<div class="section-title">Peer\'s Parameters<\/div>');
			createFieldTable('', [
				{ title: 'Alias', name: 'f_'+t+'_peer_alias', type: 'text', maxlen: 32, size: 32, placeholder: 'optional' },
				{ title: 'Endpoint', name: 'f_'+t+'_peer_ep', type: 'text', maxlen: 64, size: 48, placeholder: 'optional' },
				{ title: 'Private Key', name: 'f_'+t+'_peer_privkey', type: 'text', maxlen: 44, size: 48 },
				{ title: 'Public Key', name: 'f_'+t+'_peer_pubkey', type: 'text', maxlen: 44, size: 48 },
				{ title: 'Preshared Key', name: 'f_'+t+'_peer_psk', type: 'text', maxlen: 44, size: 48 },
				{ title: 'VPN Interface IP', name: 'f_'+t+'_peer_ip', type: 'text', placeholder: 'CIDR format', maxlen: 64, size: 64 },
				{ title: 'Allowed IPs', name: 'f_'+t+'_peer_aip', type: 'text', placeholder: 'CIDR format / comma separated', maxlen: 128, size: 64 },
				{ title: 'Peer behind NAT', name: 'f_'+t+'_peer_ka', type: 'text', maxlen: 2, size: 4, value: '', suffix: '&nbsp;<small>configures keepalive interval for peer connections (0=disable/no NAT, 10-99s range, 25 is a common setting)<\/small>' },
				{ title: '', custom: '<input type="button" value="Add to Peers" onclick="addPeer('+i+')" id="'+t+'_peer_add"> <input type="button" value="Clean" onclick="clearPeerFields('+i+')" id="'+t+'_peer_clean">' }
			]);
			W('<\/div>');
			/* peers tab stop */

			/* scripts tab start */
			W('<div id="'+t+'-wg-scripts">');
			W('<div class="section-title">Custom Interface Scripts<\/div>');
			createFieldTable('', [
				{ title: 'Pre-Up Script', name: t+'_preup', type: 'textarea', value: nvram[t+'_preup'] },
				{ title: 'Post-Up Script', name: t+'_postup', type: 'textarea', value: nvram[t+'_postup'] },
				{ title: 'Pre-Down Script', name: t+'_predown', type: 'textarea', value: nvram[t+'_predown'] },
				{ title: 'Post-Down Script', name: t+'_postdown', type: 'textarea', value: nvram[t+'_postdown'] }
			]);
			W('<\/div>');
			/* scripts tab stop */

			/* routing policy tab start */
			W('<div id="'+t+'-wg-policy">');
			W('<div class="tomato-grid" id="'+t+'_table_routing"><\/div>');
			W('<div id="_'+t+'_routing_div_help"><div class="fields"><div class="about"><b>To use Routing Policy, you must select "External - VPN Provider" in the "Type of VPN" drop-down list and "Routing Policy"/"Routing Policy (strict)" in the "Redirect Internet Traffic".<\/b><\/div><\/div><\/div>');
			W('<div>');
			W('<ul>');
			W('<li><b>Type -> From Source IP<\/b> - Ex: "1.2.3.4", "1.2.3.4-2.3.4.5", "1.2.3.0/24".<\/li>');
			W('<li><b>Type -> To Destination IP<\/b> - Ex: "1.2.3.4" or "1.2.3.0/24".<\/li>');
			W('<li><b>Type -> To Domain<\/b> - Ex: "domain.com". Please enter one domain per line.<\/li>');
			W('<li><b>IMPORTANT!<\/b> - Kill Switch: iptables rules (if \'KS\' for given entry is enabled) are always applied even if instance is down but in PBR mode (so-called strict Kill Switch).<\/li>');
			W('<\/ul>');
			W('<\/div>');
			W('<\/div>');
			/* routing policy tab stop */

			/* status tab start */
			W('<div id="'+t+'-wg-status">');
			W('<pre id="'+t+'_result" class="status-result"><\/pre>');
			W('<div style="text-align:right">');
			W('<img src="spin.svg" id="'+t+'_status_refresh_spinner" alt=""> &nbsp;');
			genStdTimeList(t+'_status_refresh_time', 'One off', 0);
			W('<input type="button" value="Refresh" onclick="toggleRefresh('+i+')" id="'+t+'_status_refresh_button"><\/div>');
			W('<div style="display:none;padding-left:5px" id="'+t+'_status_wait"> Please wait... <img src="spin.svg" alt=""><\/div>');
			statRefreshes[i].setup();
			statRefreshes[i].initPage(3000, 0);
			W('<\/div>');
			/* status tab end */

			W('<\/div>');
		}
	</script>
</div>

<!-- / / / -->

<div class="section-title">Notes</div>
<div class="section" id="sesdiv_notes">
	<ul>
		<li><b>Do not change (and save)</b> the settings when wireguard <b>is running</b> - you may end up with a downed firewall or broken routing table!</li>
		<li>For help, click the link in the upper right corner to the dedicated wiki page.</li>
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
