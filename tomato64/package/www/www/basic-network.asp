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
<title>[<% ident(); %>] Basic: Network</title>
<link rel="stylesheet" type="text/css" href="tomato.css">
<% css(); %>
<script src="tomato.js"></script>
<script src="md5.js"></script>
<script src="wireless.jsx?_http_id=<% nv(http_id); %>"></script>
<script src="interfaces.js"></script>
<script src="wireless.js"></script>
<script>
//	<% nvram("dhcp_lease,dhcpd_startip,dhcpd_endip,lan_dhcp,lan_gateway,lan_ipaddr,lan_netmask,lan_proto,lan_state,lan_desc,lan_invert,wl_security_mode,wl_wds_enable,wl_channel,wl_closed,wl_crypto,wl_key,wl_key1,wl_key2,wl_key3,wl_key4,wl_lazywds,wl_mode,wl_net_mode,wl_passphrase,wl_radio,wl_radius_ipaddr,wl_radius_port,wl_ssid,wl_wds,wl_wep_bit,wl_wpa_gtk_rekey,wl_wpa_psk,wl_radius_key,wl_auth,wl_hwaddr,t_features,wl_nbw_cap,wl_nctrlsb,wl_nband,wl_phytype,lan_ifname,lan_stp,lan1_ifname,lan1_ipaddr,lan1_netmask,lan1_proto,lan1_stp,dhcp1_lease,dhcpd1_startip,dhcpd1_endip,lan2_ifname,lan2_ipaddr,lan2_netmask,lan2_proto,lan2_stp,dhcp2_lease,dhcpd2_startip,dhcpd2_endip,lan3_ifname,lan3_ipaddr,lan3_netmask,lan3_proto,lan3_stp,dhcp3_lease,dhcpd3_startip,dhcpd3_endip,cstats_enable,wan_proto,wan_weight,wan_modem_type,wan_modem_pin,wan_modem_dev,wan_modem_init,wan_modem_apn,wan_modem_speed,wan_modem_band,wan_modem_roam,wan_ppp_username,wan_ppp_passwd,wan_ppp_service,wan_l2tp_server_ip,wan_pptp_dhcp,wan_ipaddr,wan_netmask,wan_gateway,wan_pptp_server_ip,wan_ppp_custom,wan_ppp_demand,wan_ppp_idletime,wan_ppp_demand_dnsip,wan_ppp_redialperiod,wan_pppoe_lei,wan_pppoe_lef,wan_mtu_enable,wan_mtu,wan_ppp_mlppp,wan_modem_ipaddr,wan_sta,wan_dns,wan_dns_auto,wan_ifnameX,wan_ckmtd,wan_ck_pause,wan2_proto,wan2_weight,wan2_modem_type,wan2_modem_pin,wan2_modem_dev,wan2_modem_init,wan2_modem_apn,wan2_modem_speed,wan2_modem_band,wan2_modem_roam,wan2_ppp_username,wan2_ppp_passwd,wan2_ppp_service,wan2_l2tp_server_ip,wan2_pptp_dhcp,wan2_ipaddr,wan2_netmask,wan2_gateway,wan2_pptp_server_ip,wan2_ppp_custom,wan2_ppp_demand,wan2_ppp_idletime,wan2_ppp_demand_dnsip,wan2_ppp_redialperiod,wan2_pppoe_lei,wan2_pppoe_lef,wan2_mtu_enable,wan2_mtu,wan2_ppp_mlppp,wan2_modem_ipaddr,wan2_sta,wan2_dns,wan2_dns_auto,wan2_ifnameX,wan2_ckmtd,wan2_ck_pause,wan3_proto,wan3_weight,wan3_modem_type,wan3_modem_pin,wan3_modem_dev,wan3_modem_init,wan3_modem_apn,wan3_modem_speed,wan3_modem_band,wan3_modem_roam,wan3_ppp_username,wan3_ppp_passwd,wan3_ppp_service,wan3_l2tp_server_ip,wan3_pptp_dhcp,wan3_ipaddr,wan3_netmask,wan3_gateway,wan3_pptp_server_ip,wan3_ppp_custom,wan3_ppp_demand,wan3_ppp_idletime,wan3_ppp_demand_dnsip,wan3_ppp_redialperiod,wan3_pppoe_lei,wan3_pppoe_lef,wan3_mtu_enable,wan3_mtu,wan3_ppp_mlppp,wan3_modem_ipaddr,wan3_sta,wan3_dns,wan3_dns_auto,wan3_ifnameX,wan3_ckmtd,wan3_ck_pause,wan4_proto,wan4_weight,wan4_modem_type,wan4_modem_pin,wan4_modem_dev,wan4_modem_init,wan4_modem_apn,wan4_modem_speed,wan4_modem_band,wan4_modem_roam,wan4_ppp_username,wan4_ppp_passwd,wan4_ppp_service,wan4_l2tp_server_ip,wan4_pptp_dhcp,wan4_ipaddr,wan4_netmask,wan4_gateway,wan4_pptp_server_ip,wan4_ppp_custom,wan4_ppp_demand,wan4_ppp_idletime,wan4_ppp_demand_dnsip,wan4_ppp_redialperiod,wan4_pppoe_lei,wan4_pppoe_lef,wan4_mtu_enable,wan4_mtu,wan4_ppp_mlppp,wan4_modem_ipaddr,wan4_sta,wan4_dns,wan4_dns_auto,wan4_ifnameX,wan4_ckmtd,wan4_ck_pause,mwan_num,mwan_cktime,mwan_ckdst,mwan_tune_gc,wan_hilink_ip,wan2_hilink_ip,wan3_hilink_ip,wan4_hilink_ip,wan_status_script,wan2_status_script,wan3_status_script,wan4_status_script,smart_connect_x,dnscrypt_proxy,dnscrypt_priority,stubby_proxy,stubby_priority,dhcp_moveip"); %>

/* DUALWAN-BEGIN */
maxwan_num = 2;
/* DUALWAN-END */

/* MULTIWAN-BEGIN */
maxwan_num = 4;
/* MULTIWAN-END */

var sta_list = [];
function refresh_sta_list() {
	var u;
	for (var uidx = 0; uidx < wl_ifaces.length; ++uidx) {
		if (wl_sunit(uidx) < 0) {
			u = wl_unit(uidx);
			sta_list[u] = [];
			sta_list[u][0] = 'wl'+u;
			sta_list[u][1] = wl_display_ifname(uidx);
		}
	}
	sta_list[u + 1] = [];
	sta_list[u + 1][0] = '';
	sta_list[u + 1][1] = 'Disabled';
}

var lg = new TomatoGrid();
lg.setup = function() {
	this.init('lan-grid', '', 4, [
		{ type: 'select', options: [[0,'0'],[1,'1'],[2,'2'],[3,'3']], prefix: '<div class="centered">', suffix: '<\/div>' },
		{ type: 'checkbox', prefix: '<div class="centered">', suffix: '<\/div>' },
		{ type: 'text', maxlen: 15, size: 17 },
		{ type: 'text', maxlen: 15, size: 17 },
		{ type: 'checkbox', prefix: '<div class="centered">', suffix: '<\/div>' },
		{ multi: [ { type: 'text', maxlen: 15, size: 17}, { type: 'text', maxlen: 15, size: 17 } ] },
		{ type: 'text', maxlen: 6, size: 8 }] );
	this.headerSet(['Bridge','STP','IP Address','Netmask','DHCP','IP&nbsp;Range&nbsp;<i>(first/last)<\/i>','Lease&nbsp;Time&nbsp;<i>(mins)<\/i>']);

	var numBridges = 0;
	for (var i = 0 ; i <= MAX_BRIDGE_ID; i++) {
		var j = (i == 0) ? '' : i.toString();
		if (nvram['lan'+j+'_ifname'].length > 0) {
			if ((!fixIP(nvram['dhcpd'+j+'_startip'])) || (!fixIP(nvram['dhcpd'+j+'_endip']))) {
				if ((fixIP(nvram['lan'+j+'_ipaddr'])) && (fixIP(nvram['lan'+j+'_netmask']))) {
					var n = getNetworkAddress(nvram['lan'+j+'_ipaddr'], nvram['lan'+j+'_netmask']);
					/* defaults */
					nvram['dhcpd'+j+'_startip'] = getAddress('0.0.0.2', n);
					nvram['dhcpd'+j+'_endip'] = getAddress('0.0.0.50', n);
				}
			}
			lg.insertData(-1, [i.toString(), nvram['lan'+j+'_stp'], nvram['lan'+j+'_ipaddr'], nvram['lan'+j+'_netmask'], (nvram['lan'+j+'_proto'] == 'dhcp') ? 1 : 0, nvram['dhcpd'+j+'_startip'], 
			                   nvram['dhcpd'+j+'_endip'], (nvram['lan'+j+'_proto'] == 'dhcp') ? (((nvram['dhcp'+j+'_lease']) * 1 == 0) ? '1440' : (nvram['dhcp'+j+'_lease']).toString()) : '']) ;
			numBridges++;
		}
	}
	lg.canDelete = false;
	lg.sort(0);
	elem.removeClass(lg.header.cells[lg.sortColumn], 'sortasc', 'sortdes');
	lg.showNewEditor();
	lg.resetNewEditor();
}

lg.dataToView = function(data) {
	return ['br'+data[0],
		(data[1].toString() == '1') ? '<small><i>Enabled<\/i><\/small>' : '<small><i>Disabled<\/i><\/small>', data[2], data[3],
		(data[4].toString() == '1') ? '<small><i>Enabled<\/i><\/small>' : '<small><i>Disabled<\/i><\/small>',
		(data[5].toString()+((numberOfBitsOnNetMask(data[3]) >= 24) ? (' - '+data[6].split('.').splice(3, 1).toString()) : ('<br>'+data[6].toString()))),
		(((data[7] != null) && (data[7] != '')) ? data[7] : '')];
}

lg.dataToFieldValues = function (data) {
	return [data[0], (data[1] != 0) ? 'checked' : '', data[2].toString(), data[3].toString(), (data[4].toString() == '1') ? 'checked' : '', data[5].toString(), data[6].toString(), data[7].toString()];
}

lg.fieldValuesToData = function(row) {
	var f = fields.getAll(row);

	return [f[0].value, f[1].checked ? 1 : 0, f[2].value, f[3].value, f[4].checked ? 1 : 0, f[5].value, f[6].value, f[7].value];
}

lg.resetNewEditor = function() {
	var f = fields.getAll(this.newEditor);
	f[0].selectedIndex=0;

	var t = MAX_BRIDGE_ID;
	while ((this.countBridge(f[0].selectedIndex) > 0) && (t > 0)) {
		f[0].selectedIndex = (f[0].selectedIndex%(MAX_BRIDGE_ID)) + 1;
		t--;
	}

	for (var j = 0; j <= MAX_BRIDGE_ID ; j++) {
		f[0].options[j].disabled = (this.countBridge(j) > 0);
	}

	f[1].checked = 0;
	f[2].value = '';
	f[3].value = '';
	f[5].value = '';
	f[6].value = '';
	f[7].value = '';
	f[4].checked = 0;
	f[4].disabled = 1;
	f[5].disabled = 1;
	f[6].disabled = 1;
	f[7].disabled = 1;
	ferror.clearAll(fields.getAll(this.newEditor));
}

lg.onCancel = function() {
	this.removeEditor();
	this.showSource();
	this.disableNewEditor(false);

	this.resetNewEditor();
}

lg.onAdd = function() {
	var data;

	this.moving = null;
	this.rpHide();

	if (!this.verifyFields(this.newEditor, 0))
		return;

	data = this.fieldValuesToData(this.newEditor);
	this.insertData(-1, data);

	this.disableNewEditor(false);
	this.resetNewEditor();

	this.resort();
}

lg.onOK = function() {
	var i, data, view;

	if (!this.verifyFields(this.editor, 0))
		return;

	data = this.fieldValuesToData(this.editor);
	view = this.dataToView(data);

	this.source.setRowData(data);
	for (i = 0; i < this.source.cells.length; ++i) {
		this.source.cells[i].innerHTML = view[i];
	}

	this.removeEditor();
	this.showSource();
	this.disableNewEditor(false);

	this.resort();
	this.resetNewEditor();
}

lg.onDelete = function() {
	this.removeEditor();
	elem.remove(this.source);
	this.source = null;
	this.disableNewEditor(false);

	this.resetNewEditor();
}

lg.countElem = function(f, v) {
	var data = this.getAllData();

	var total = 0;
	for (var i = 0; i < data.length; ++i) {
		total += (data[i][f] == v) ? 1 : 0;
	}

	return total;
}

lg.countBridge = function (v) {
	return this.countElem(0, v);
}

lg.countOverlappingNetworks = function (ip) {
	var data = this.getAllData();
	var total = 0;
	for (var i = 0; i < data.length; ++i) {
		var net = getNetworkAddress(data[i][2], data[i][3]);
		var brd = getBroadcastAddress(net, data[i][3]);
		total += (net != '0.0.0.0' ? (((aton(ip) <= aton(brd)) && (aton(ip) >= aton(net))) ? 1 : 0) : 0);
	}

	return total;
}

lg.verifyFields = function(row, quiet) {
	var ok = 1;
	var f;

	f = fields.getAll(row);

	for (var j = 0; j <= MAX_BRIDGE_ID; j++) {
		f[0].options[j].disabled = (this.countBridge(j) > 0);
	}

	if (this.countBridge(f[0].selectedIndex) > 0) {
		ferror.set(f[0], 'Cannot add another entry for bridge br'+f[0].selectedIndex, quiet);
		ok = 0;
	}
	else
		ferror.clear(f[0]);
/* valid IP address? */
	if (!v_ip(f[2], quiet || !ok)) 
		ok = 0;
/* if we have a properly defined IP address - 0.0.0.0 is NOT a valid IP address for our intents/purposes! */
	if ((f[2].value != '') && (f[2].value != '0.0.0.0')) {
/* allow DHCP to be enabled */
		f[4].disabled = 0;
/* validate netmask */
		if (!v_netmask(f[3], quiet || !ok))
			return 0;
		else {
/* should be 22 bits or smaller network */
			if ((numberOfBitsOnNetMask(f[3].value) < 22) && (nvram.cstats_enable == '1' )) {
				if (!confirm("Netmask should have at least 22 bits (255.255.252.0). You may continue anyway but remember - you was warned")) return;
			}
			else
				ferror.clear(f[3]);
		}
		if (f[2].value == getNetworkAddress(f[2].value, f[3].value)) {
			var s = 'Invalid IP address or subnet mask (the address of the network cannot be used)';
			ferror.set(f[2], s, quiet);
			ferror.set(f[3], s, quiet);
			return 0;
		}
		else if (f[2].value == getBroadcastAddress(getNetworkAddress(f[2].value, f[3].value), f[3].value)) {
			var s = 'Invalid IP address or subnet mask (the broadcast address cannot be used)';
			ferror.set(f[2], s, quiet);
			ferror.set(f[3], s, quiet);
			return 0;
		}
		else if (this.countOverlappingNetworks(f[2].value) > 0) {
			var s = 'Invalid IP address or subnet mask (conflicts/overlaps with another LAN bridge)';
			ferror.set(f[2], s, quiet);
			ferror.set(f[3], s, quiet);
			return 0;
		}
		else {
			ferror.clear(f[2]);
			ferror.clear(f[3]);
		}
	}
	else {
		f[4].checked = 0;
		f[4].disabled = 1;
	}
/* dhcp enabled? */
	if ((f[4].checked) && (v_ip(f[2], 1)) && (v_netmask(f[3],1))) {
		f[5].disabled = 0;
		f[6].disabled = 0;
		f[7].disabled = 0;
/* first/last IP still unset? */
		if (f[5].value == '') {
			var l;
			var m = aton(f[2].value) & aton(f[3].value);
			var o = (m) ^ (~ aton(f[3].value))
			var n = o - m;
			do {
				if (--n < 0) {
					f[5].value = '';
					return;
				}
				m++;
			} while (((l = fixIP(ntoa(m), 1)) == null) || (l == f[2].value));

			f[5].value = l;
		}
		if (f[6].value == '') {
			var l;
			var m = aton(f[2].value) & aton(f[3].value);
			var o = (m) ^ (~ aton(f[3].value));
			var n = o - m;
			do {
				if (--n < 0) {
					f[6].value = '';
					return;
				}
				o--;
			} while (((l = fixIP(ntoa(o), 1)) == null) || (l == f[2].value));

			f[6].value = l;
		}
/* first IP valid? */
		if ((getNetworkAddress(f[5].value, f[3].value) != getNetworkAddress(f[2].value, f[3].value)) ||
		    (f[5].value == getBroadcastAddress(getNetworkAddress(f[2].value, f[3].value), f[3].value)) ||
		    (f[5].value == getNetworkAddress(f[2].value, f[3].value)) ||
		    (f[2].value == f[5].value)) {
			ferror.set(f[5], 'Invalid first IP address or subnet mask', quiet || !ok);
			return 0;
		}
		else
			ferror.clear(f[5]);
/* last IP valid? */
		if ((getNetworkAddress(f[6].value, f[3].value) != getNetworkAddress(f[2].value, f[3].value)) ||
		    (f[6].value == getBroadcastAddress(getNetworkAddress(f[2].value, f[3].value), f[3].value)) ||
		    (f[6].value == getNetworkAddress(f[2].value, f[3].value)) ||
		    (f[2].value == f[6].value)) {
			ferror.set(f[6], 'Invalid last IP address or subnet mask', quiet || !ok);
			return 0;
		}
		else
			ferror.clear(f[6]);
/* validate range, swap first/last IP if needed */
		if (aton(f[6].value) < aton(f[5].value)) {
			var t = f[5].value;
			f[5].value = f[6].value;
			f[6].value = t;
		}
/* lease time */
		if (parseInt(f[7].value*1) == 0)
			f[7].value = 1440; /* from nvram/defaults.c */
		if (!v_mins(f[7], quiet || !ok, 1, 10080)) 
			ok = 0;
	}
	else {
		f[5].disabled = 1;
		f[6].disabled = 1;
		f[7].disabled = 1;
		ferror.clear(f[5]);
		ferror.clear(f[6]);
		ferror.clear(f[7]);
	}

	return ok;
}

W('<style>\n');
for (var u = 0; u < wl_ifaces.length; ++u) {
	W('#spin'+wl_unit(u)+', \n');
}
W('#spin {\n');
W('display:none;\n');
W('vertical-align:middle;\n');
W('}\n');
W('<\/style>\n');

var xob = null;
var cmd = null;
var refresher = [];
var nphy = features('11n');
var acphy = features('11ac');

var ghz = [];
var bands = [];
var nm_loaded = [], ch_loaded = [], max_channel = [];

for (var uidx = 0; uidx < wl_ifaces.length; ++uidx) {
	if (wl_sunit(uidx) < 0) {
		var b;
		b = [];
		for (var i = 0; i < wl_bands[uidx].length; ++i) {
			b.push([wl_bands[uidx][i]+'', (wl_bands[uidx][i] == '1') ? '5 GHz' : '2.4 GHz']);
		}
		bands.push(b);

		b = [];
		ghz.push(b);

		nm_loaded.push(0);
		ch_loaded.push(0);
		max_channel.push(0);
		refresher.push(null);
	}
}

function verifyFields(focused, quiet) {
	var i;
	var ok = 1;
	var a, b, c, d, e;
	var u, n, uidx, wan_uidx;
	var wmode, sm2, sta_wl;
	var curr_mwan_num = E('_mwan_num').value;
	var wanproto = [];

	n = E('_f_lan_state').checked;
	E('_f_lan_desc').disabled = !n;
	E('_f_lan_invert').disabled = !n;

	var mwan = E('_mwan_num');
	if (mwan.options[(mwan.selectedIndex)].disabled)
		mwan.selectedIndex = 0;

	var cktime = (E('_mwan_cktime').value == 0);
	elem.display(PR('_f_mwan_ckdst_1'), !cktime);
	elem.display(PR('_f_mwan_ckdst_2'), !cktime);
	E('_f_mwan_ckdst_1').disabled = cktime;
	E('_f_mwan_ckdst_2').disabled = cktime;

	if (!v_ip('_f_mwan_ckdst_1', true) && !v_domain('_f_mwan_ckdst_1', true)) {
		ferror.set(E('_f_mwan_ckdst_1'), "Target 1 is not a valid IP address or domain name.", quiet);
		ok = 0;
	}
	if (!v_ip('_f_mwan_ckdst_2', true) && !v_domain('_f_mwan_ckdst_2', true)) {
		ferror.set(E('_f_mwan_ckdst_2'), "Target 2 is not a valid IP address or domain name.", quiet);
		ok = 0;
	}

	for (uidx = 0; uidx < wl_ifaces.length; ++uidx) {
		if (wl_sunit(uidx)<0) {
			u = wl_unit(uidx);
			if (focused == E('_f_wl'+u+'_nband')) {
				refreshNetModes(uidx);
				refreshChannels(uidx);
				refreshBandWidth(uidx);
			}
			else if (focused == E('_f_wl'+u+'_nctrlsb') || focused == E('_wl'+u+'_nbw_cap'))
				refreshChannels(uidx);
		}
	}

	/* --- visibility --- */

	var vis = {
		_f_automatic_ip: 1,
		_f_dns_1: 1,
		_f_dns_2: 1,
		_lan_gateway: 1
	};

	for (uidx = 1; uidx <= maxwan_num; ++uidx) {
		u = (uidx > 1) ? uidx : '';
		if (uidx <= curr_mwan_num) {
			vis['_wan'+u+'_proto'] = 1;
			vis['_wan'+u+'_weight'] = 1;
			vis['_wan'+u+'_ppp_username'] = 1;
			vis['_wan'+u+'_ppp_passwd'] = 1;
			vis['_wan'+u+'_ppp_service'] = 1;
			vis['_wan'+u+'_ppp_custom'] = 1;
			vis['_wan'+u+'_l2tp_server_ip'] = 1;
			vis['_wan'+u+'_ipaddr'] = 1;
			vis['_wan'+u+'_netmask'] = 1;
			vis['_wan'+u+'_gateway'] = 1;
/* USB-BEGIN */
			vis['_wan'+u+'_hilink_ip'] = 1;
			vis['_f_wan'+u+'_status_script'] = 1;
/* USB-END */
			vis['_wan'+u+'_ckmtd'] = 1;
			vis['_f_wan'+u+'_ck_pause'] = 1;
			vis['_wan'+u+'_pptp_server_ip'] = 1;
			vis['_f_wan'+u+'_pptp_dhcp'] = 1;
			vis['_wan'+u+'_ppp_demand'] = 1;
			vis['_wan'+u+'_ppp_demand_dnsip'] = 1;
			vis['_wan'+u+'_ppp_idletime'] = 1;
			vis['_wan'+u+'_ppp_redialperiod'] = 1;
			vis['_wan'+u+'_pppoe_lei'] = 1;
			vis['_wan'+u+'_pppoe_lef'] = 1;
			vis['_wan'+u+'_mtu_enable'] = 1;
			vis['_f_wan'+u+'_mtu'] = 1;
			vis['_f_wan'+u+'_ppp_mlppp'] = 1;
			vis['_wan'+u+'_modem_ipaddr'] = 1;
/* USB-BEGIN */
			vis['_wan'+u+'_modem_pin'] = 1;
			vis['_wan'+u+'_modem_dev'] = 1;
			vis['_wan'+u+'_modem_init'] = 1;
			vis['_wan'+u+'_modem_apn'] = 1;
			vis['_wan'+u+'_modem_speed'] = 1;
			vis['_wan'+u+'_modem_band'] = 1;
			vis['_wan'+u+'_modem_roam'] = 1;
/* USB-END */
			vis['_wan'+u+'_sta'] = 1;
			vis['_f_wan'+u+'_dns_1'] = 1;
			vis['_f_wan'+u+'_dns_2'] = 1;
			vis['_wan'+u+'_dns_auto'] = 1;
			E('_wan'+u+'_proto').disabled = 0;
			E('_wan'+u+'_weight').disabled = 0;
			E('_wan'+u+'_ppp_username').disabled = 0;
			E('_wan'+u+'_ppp_passwd').disabled = 0;
			E('_wan'+u+'_ppp_service').disabled = 0;
			E('_wan'+u+'_ppp_custom').disabled = 0;
			E('_wan'+u+'_l2tp_server_ip').disabled = 0;
			E('_wan'+u+'_ipaddr').disabled = 0;
			E('_wan'+u+'_netmask').disabled = 0;
			E('_wan'+u+'_gateway').disabled = 0;
/* USB-BEGIN */
			E('_wan'+u+'_hilink_ip').disabled = 0;
			E('_f_wan'+u+'_status_script').disabled = 0;
/* USB-END */
			E('_wan'+u+'_ckmtd').disabled = 0;
			E('_f_wan'+u+'_ck_pause').disabled = 0;
			E('_wan'+u+'_pptp_server_ip').disabled = 0;
			E('_f_wan'+u+'_pptp_dhcp').disabled = 0;
			E('_wan'+u+'_ppp_demand').disabled = 0;
			E('_wan'+u+'_ppp_demand_dnsip').disabled = 0;
			E('_wan'+u+'_ppp_idletime').disabled = 0;
			E('_wan'+u+'_ppp_redialperiod').disabled = 0;
			E('_wan'+u+'_pppoe_lei').disabled = 0;
			E('_wan'+u+'_pppoe_lef').disabled = 0;
			E('_wan'+u+'_mtu_enable').disabled = 0;
			E('_f_wan'+u+'_mtu').disabled = 0;
			E('_f_wan'+u+'_ppp_mlppp').disabled = 0;
			E('_wan'+u+'_modem_ipaddr').disabled = 0;
/* USB-BEGIN */
			E('_wan'+u+'_modem_pin').disabled = 0;
			E('_wan'+u+'_modem_dev').disabled = 0;
			E('_wan'+u+'_modem_init').disabled = 0;
			E('_wan'+u+'_modem_apn').disabled = 0;
			E('_wan'+u+'_modem_speed').disabled = 0;
			E('_wan'+u+'_modem_band').disabled = 0;
			E('_wan'+u+'_modem_roam').disabled = 0;
/* USB-END */
			E('_wan'+u+'_sta').disabled = 0;
			E('_f_wan'+u+'_dns_1').disabled = 0;
			E('_f_wan'+u+'_dns_2').disabled = 0;
			E('_wan'+u+'_dns_auto').disabled = 0;
			wanproto[uidx - 1] = E('_wan'+u+'_proto').value;
		}
		else {
			vis['_wan'+u+'_proto'] = 0;
			vis['_wan'+u+'_weight'] = 0;
			vis['_wan'+u+'_ppp_username'] = 0;
			vis['_wan'+u+'_ppp_passwd'] = 0;
			vis['_wan'+u+'_ppp_service'] = 0;
			vis['_wan'+u+'_ppp_custom'] = 0;
			vis['_wan'+u+'_l2tp_server_ip'] = 0;
			vis['_wan'+u+'_ipaddr'] = 0;
			vis['_wan'+u+'_netmask'] = 0;
			vis['_wan'+u+'_gateway'] = 0;
			vis['_wan'+u+'_ckmtd'] = 0;
			vis['_f_wan'+u+'_ck_pause'] = 0;
			vis['_wan'+u+'_pptp_server_ip'] = 0;
			vis['_f_wan'+u+'_pptp_dhcp'] = 0;
			vis['_wan'+u+'_ppp_demand'] = 0;
			vis['_wan'+u+'_ppp_demand_dnsip'] = 0;
			vis['_wan'+u+'_ppp_idletime'] = 0;
			vis['_wan'+u+'_ppp_redialperiod'] = 0;
			vis['_wan'+u+'_pppoe_lei'] = 0;
			vis['_wan'+u+'_pppoe_lef'] = 0;
			vis['_wan'+u+'_mtu_enable'] = 0;
			vis['_f_wan'+u+'_mtu'] = 0;
			vis['_f_wan'+u+'_ppp_mlppp'] = 0;
			vis['_wan'+u+'_modem_ipaddr'] = 0;
/* USB-BEGIN */
			vis['_wan'+u+'_modem_pin'] = 0;
			vis['_wan'+u+'_modem_dev'] = 0;
			vis['_wan'+u+'_modem_init'] = 0;
			vis['_wan'+u+'_modem_apn'] = 0;
			vis['_wan'+u+'_modem_speed'] = 0;
			vis['_wan'+u+'_modem_band'] = 0;
			vis['_wan'+u+'_modem_roam'] = 0;
/* USB-END */
			vis['_wan'+u+'_sta'] = 0;
			vis['_f_wan'+u+'_dns_1'] = 0;
			vis['_f_wan'+u+'_dns_2'] = 0;
			vis['_wan'+u+'_dns_auto'] = 0;
			E('_wan'+u+'_proto').disabled = 0;
			E('_wan'+u+'_weight').disabled = 1;
			E('_wan'+u+'_ppp_username').disabled = 1;
			E('_wan'+u+'_ppp_passwd').disabled = 1;
			E('_wan'+u+'_ppp_service').disabled = 1;
			E('_wan'+u+'_ppp_custom').disabled = 1;
			E('_wan'+u+'_l2tp_server_ip').disabled = 1;
			E('_wan'+u+'_ipaddr').disabled = 1;
			E('_wan'+u+'_netmask').disabled = 1;
			E('_wan'+u+'_gateway').disabled = 1;
/* USB-BEGIN */
			E('_wan'+u+'_hilink_ip').disabled = 1;
			E('_f_wan'+u+'_status_script').disabled = 1;
/* USB-END */
			E('_wan'+u+'_ckmtd').disabled = 1;
			E('_f_wan'+u+'_ck_pause').disabled = 1;
			E('_wan'+u+'_pptp_server_ip').disabled = 1;
			E('_f_wan'+u+'_pptp_dhcp').disabled = 1;
			E('_wan'+u+'_ppp_demand').disabled = 1;
			E('_wan'+u+'_ppp_demand_dnsip').disabled = 1;
			E('_wan'+u+'_ppp_idletime').disabled = 1;
			E('_wan'+u+'_ppp_redialperiod').disabled = 1;
			E('_wan'+u+'_pppoe_lei').disabled = 1;
			E('_wan'+u+'_pppoe_lef').disabled = 1;
			E('_wan'+u+'_mtu_enable').disabled = 1;
			E('_f_wan'+u+'_mtu').disabled = 1;
			E('_f_wan'+u+'_ppp_mlppp').disabled = 1;
			E('_wan'+u+'_modem_ipaddr').disabled = 1;
/* USB-BEGIN */
			E('_wan'+u+'_modem_pin').disabled = 1;
			E('_wan'+u+'_modem_dev').disabled = 1;
			E('_wan'+u+'_modem_init').disabled = 1;
			E('_wan'+u+'_modem_apn').disabled = 1;
			E('_wan'+u+'_modem_speed').disabled = 1;
			E('_wan'+u+'_modem_band').disabled = 1;
			E('_wan'+u+'_modem_roam').disabled = 1;
/* USB-END */
			E('_f_wan'+u+'_dns_1').disabled = 1;
			E('_f_wan'+u+'_dns_2').disabled = 1;
			E('_wan'+u+'_dns_auto').disabled = 1;
		}
	}

	var wl_vis = [];
	for (uidx = 0; uidx < wl_ifaces.length; ++uidx) {
		if (wl_sunit(uidx)<0) {
			a = {
				_f_wl_radio: 1,
				_f_wl_mode: 1,
				_f_wl_nband: (bands[uidx].length > 1) ? 1 : 0,
				_wl_net_mode: 1,
				_wl_ssid: 1,
				_f_wl_bcast: 1,
				_wl_channel: 1,
				_wl_nbw_cap: nphy || acphy ? 1 : 0,
				_f_wl_nctrlsb: nphy || acphy ? 1 : 0,
				_f_wl_scan: 1,

				_wl_security_mode: 1,
				_wl_crypto: 1,
				_wl_wpa_psk: 1,
				_f_wl_psk_random1: 1,
				_f_wl_psk_random2: 1,
				_wl_wpa_gtk_rekey: 1,
				_wl_radius_key: 1,
				_wl_radius_ipaddr: 1,
				_wl_radius_port: 1,
				_wl_wep_bit: 1,
				_wl_passphrase: 1,
				_f_wl_wep_gen: 1,
				_f_wl_wep_random: 1,
				_wl_key1: 1,
				_wl_key2: 1,
				_wl_key3: 1,
				_wl_key4: 1,

				_f_wl_lazywds: 1,
				_f_wl_wds_0: 1
			};
			wl_vis.push(a);
		}
	}

	for (uidx = 0; uidx < wl_ifaces.length; ++uidx) {
		if (wl_sunit(uidx) < 0) {
			wmode = E('_f_wl'+wl_unit(uidx)+'_mode').value;

			if ((wmode == 'wet') ||
/* BCMWL6-BEGIN */
			    (wmode == 'psta') ||
/* BCMWL6-END */
			    0) {
				E('_mwan_num').value = 1;
				E('_mwan_cktime').value = 0;
				elem.display('mwan-title', 'mwan-section', 0);
				for (wan_uidx = 1; wan_uidx <= maxwan_num; ++wan_uidx) {
					u = (wan_uidx > 1) ? wan_uidx : '';
					vis['_wan'+u+'_proto'] = 0;
					E('_wan'+u+'_proto').value = 'disabled';
					wanproto[wan_uidx - 1] = 'disabled';
					elem.display('wan'+u+'-title', 'sesdiv_wan'+u, 0);
				}
				break; /* break the loop! one wlan module is using wireless ethernet bridge or media bridge mode --> hide wan options! */
			}
			else { /* not in wireless bridge mode - show wan options */
				elem.display('mwan-title', 'mwan-section', 1);
				for (wan_uidx = 1; wan_uidx <= curr_mwan_num; ++wan_uidx) {
					u = (wan_uidx > 1) ? wan_uidx : '';
					vis['_wan'+u+'_proto'] = 1;
					elem.display('wan'+u+'-title', 'sesdiv_wan'+u, 1);
				}
			}

		}
	}

	for (uidx = 1; uidx <= maxwan_num; ++uidx) {
		u = (uidx > 1) ? uidx : '';
		switch (wanproto[uidx - 1]) {
		case 'disabled':
			vis['_wan'+u+'_weight'] = 0;
			vis['_wan'+u+'_ppp_username'] = 0;
			vis['_wan'+u+'_ppp_service'] = 0;
			vis['_wan'+u+'_ppp_custom'] = 0;
			vis['_wan'+u+'_l2tp_server_ip'] = 0;
			vis['_wan'+u+'_ipaddr'] = 0;
			vis['_wan'+u+'_netmask'] = 0;
			vis['_wan'+u+'_gateway'] = 0;
/* USB-BEGIN */
			vis['_wan'+u+'_hilink_ip'] = 0;
			vis['_f_wan'+u+'_status_script'] = 0;
/* USB-END */
			vis['_wan'+u+'_ckmtd'] = 0;
			vis['_f_wan'+u+'_ck_pause'] = 0;
			vis['_wan'+u+'_pptp_server_ip'] = 0;
			vis['_f_wan'+u+'_pptp_dhcp'] = 0;
			vis['_wan'+u+'_ppp_demand'] = 0;
			vis['_wan'+u+'_mtu_enable'] = 0;
			vis['_f_wan'+u+'_mtu'] = 0;
			vis['_f_wan'+u+'_ppp_mlppp'] = 0;
			vis['_wan'+u+'_modem_ipaddr'] = 0;
/* USB-BEGIN */
			vis['_wan'+u+'_modem_pin'] = 0;
			vis['_wan'+u+'_modem_dev'] = 0;
			vis['_wan'+u+'_modem_init'] = 0;
			vis['_wan'+u+'_modem_apn'] = 0;
			vis['_wan'+u+'_modem_speed'] = 0;
			vis['_wan'+u+'_modem_band'] = 0;
			vis['_wan'+u+'_modem_roam'] = 0;
/* USB-END */
			vis['_wan'+u+'_pppoe_lei'] = 0;
			vis['_wan'+u+'_pppoe_lef'] = 0;
			vis['_f_wan'+u+'_dns_1'] = 0;
			vis['_f_wan'+u+'_dns_2'] = 0;
			vis['_wan'+u+'_dns_auto'] = 0;
			E('_wan'+u+'_sta').value = '';
			vis['_wan'+u+'_sta'] = 0;
		break;
		case 'dhcp':
			vis['_wan'+u+'_l2tp_server_ip'] = 0;
			vis['_wan'+u+'_pppoe_lei'] = 0;
			vis['_wan'+u+'_pppoe_lef'] = 0;
			vis['_wan'+u+'_ppp_demand'] = 0;
			vis['_wan'+u+'_ppp_service'] = 0;
			vis['_wan'+u+'_ppp_username'] = 0;
			vis['_wan'+u+'_ppp_custom'] = 0;
			vis['_wan'+u+'_pptp_server_ip'] = 0;
			vis['_f_wan'+u+'_pptp_dhcp'] = 0;
			vis['_wan'+u+'_gateway'] = 0;
			vis['_wan'+u+'_ipaddr'] = 0;
			vis['_wan'+u+'_netmask'] = 0;
			vis['_f_wan'+u+'_ppp_mlppp'] = 0;
/* USB-BEGIN */
			vis['_wan'+u+'_modem_pin'] = 0;
			vis['_wan'+u+'_modem_dev'] = 0;
			vis['_wan'+u+'_modem_init'] = 0;
			vis['_wan'+u+'_modem_apn'] = 0;
			vis['_wan'+u+'_modem_speed'] = 0;
			vis['_wan'+u+'_modem_band'] = 0;
			vis['_wan'+u+'_modem_roam'] = 0;
/* USB-END */
		break;
		case 'pppoe':
			vis['_wan'+u+'_l2tp_server_ip'] = 0;
			vis['_wan'+u+'_pptp_server_ip'] = 0;
			vis['_wan'+u+'_gateway'] = 0;
			vis['_wan'+u+'_ipaddr'] = 0;
			vis['_wan'+u+'_netmask'] = 0;
/* USB-BEGIN */
			vis['_wan'+u+'_modem_pin'] = 0;
			vis['_wan'+u+'_modem_dev'] = 0;
			vis['_wan'+u+'_modem_init'] = 0;
			vis['_wan'+u+'_modem_apn'] = 0;
			vis['_wan'+u+'_modem_speed'] = 0;
			vis['_wan'+u+'_modem_band'] = 0;
			vis['_wan'+u+'_modem_roam'] = 0;
/* USB-END */
		break;
/* USB-BEGIN */
		case 'ppp3g':
			vis['_wan'+u+'_ppp_service'] = 0;
			vis['_wan'+u+'_l2tp_server_ip'] = 0;
			vis['_wan'+u+'_pptp_server_ip'] = 0;
			vis['_f_wan'+u+'_pptp_dhcp'] = 0;
			vis['_wan'+u+'_gateway'] = 0;
			vis['_wan'+u+'_ipaddr'] = 0;
			vis['_wan'+u+'_netmask'] = 0;
			vis['_wan'+u+'_modem_ipaddr'] = 0;
			vis['_wan'+u+'_modem_speed'] = 0;
			vis['_wan'+u+'_modem_band'] = 0;
			vis['_wan'+u+'_modem_roam'] = 0;
			vis['_f_wan'+u+'_ppp_mlppp'] = 0;
			E('_wan'+u+'_sta').value = '';
			vis['_wan'+u+'_sta'] = 0;
		break;
		case 'lte':
			vis['_wan'+u+'_pppoe_lei'] = 0;
			vis['_wan'+u+'_pppoe_lef'] = 0;
			vis['_wan'+u+'_ppp_service'] = 0;
			vis['_wan'+u+'_ppp_demand'] = 0;
			vis['_wan'+u+'_ppp_username'] = 0;
			vis['_wan'+u+'_ppp_custom'] = 0;
			vis['_wan'+u+'_l2tp_server_ip'] = 0;
			vis['_wan'+u+'_pptp_server_ip'] = 0;
			vis['_f_wan'+u+'_pptp_dhcp'] = 0;
			vis['_wan'+u+'_gateway'] = 0;
			vis['_wan'+u+'_ipaddr'] = 0;
			vis['_wan'+u+'_netmask'] = 0;
			vis['_wan'+u+'_modem_ipaddr'] = 0;
			vis['_wan'+u+'_modem_dev'] = 0;
			vis['_wan'+u+'_modem_init'] = 0;
			vis['_f_wan'+u+'_ppp_mlppp'] = 0;
			E('_wan'+u+'_sta').value = '';
			vis['_wan'+u+'_sta'] = 0;
		break;
/* USB-END */
		case 'static':
			vis['_wan'+u+'_pppoe_lei'] = 0;
			vis['_wan'+u+'_pppoe_lef'] = 0;
			vis['_wan'+u+'_l2tp_server_ip'] = 0;
			vis['_wan'+u+'_ppp_demand'] = 0;
			vis['_wan'+u+'_ppp_service'] = 0;
			vis['_wan'+u+'_ppp_username'] = 0;
			vis['_wan'+u+'_ppp_custom'] = 0;
			vis['_wan'+u+'_pptp_server_ip'] = 0;
			vis['_f_wan'+u+'_pptp_dhcp'] = 0;
			vis['_f_wan'+u+'_ppp_mlppp'] = 0;
/* USB-BEGIN */
			vis['_wan'+u+'_modem_pin'] = 0;
			vis['_wan'+u+'_modem_dev'] = 0;
			vis['_wan'+u+'_modem_init'] = 0;
			vis['_wan'+u+'_modem_apn'] = 0;
			vis['_wan'+u+'_modem_speed'] = 0;
			vis['_wan'+u+'_modem_band'] = 0;
			vis['_wan'+u+'_modem_roam'] = 0;
/* USB-END */
		break;
		case 'pptp':
			vis['_wan'+u+'_l2tp_server_ip'] = 0;
			vis['_wan'+u+'_ppp_service'] = 0;
			vis['_wan'+u+'_gateway'] = (!E('_f_wan'+u+'_pptp_dhcp').checked);
			vis['_wan'+u+'_ipaddr'] = (!E('_f_wan'+u+'_pptp_dhcp').checked);
			vis['_wan'+u+'_netmask'] = (!E('_f_wan'+u+'_pptp_dhcp').checked);
			vis['_wan'+u+'_modem_ipaddr'] = 0;
/* USB-BEGIN */
			vis['_wan'+u+'_modem_pin'] = 0;
			vis['_wan'+u+'_modem_dev'] = 0;
			vis['_wan'+u+'_modem_init'] = 0;
			vis['_wan'+u+'_modem_apn'] = 0;
			vis['_wan'+u+'_modem_speed'] = 0;
			vis['_wan'+u+'_modem_band'] = 0;
			vis['_wan'+u+'_modem_roam'] = 0;
/* USB-END */
		break;
		case 'l2tp':
			vis['_wan'+u+'_pptp_server_ip'] = 0;
			vis['_wan'+u+'_ppp_service'] = 0;
			vis['_wan'+u+'_gateway'] = (!E('_f_wan'+u+'_pptp_dhcp').checked);
			vis['_wan'+u+'_ipaddr'] = (!E('_f_wan'+u+'_pptp_dhcp').checked);
			vis['_wan'+u+'_netmask'] = (!E('_f_wan'+u+'_pptp_dhcp').checked);
			vis['_wan'+u+'_modem_ipaddr'] = 0;
/* USB-BEGIN */
			vis['_wan'+u+'_modem_pin'] = 0;
			vis['_wan'+u+'_modem_dev'] = 0;
			vis['_wan'+u+'_modem_init'] = 0;
			vis['_wan'+u+'_modem_apn'] = 0;
			vis['_wan'+u+'_modem_speed'] = 0;
			vis['_wan'+u+'_modem_band'] = 0;
			vis['_wan'+u+'_modem_roam'] = 0;
/* USB-END */
		break;
		}
/* USB-BEGIN */
		vis['_wan'+u+'_modem_band'] = (E('_wan'+u+'_modem_speed').value == '03') && vis['_wan'+u+'_modem_speed'] && (eval('nvram.wan'+u+'_modem_type') != "qmi_wwan");
/* USB-END */
		if (E('_mwan_cktime').value == 0) {
			vis['_wan'+u+'_ckmtd'] = 0;
			vis['_f_wan'+u+'_ck_pause'] = 0;
			E('_wan'+u+'_ckmtd').disabled = 1;
			E('_f_wan'+u+'_ck_pause').disabled = 1;
		}
		if (E('_f_wan'+u+'_ck_pause').checked == 1) {
			vis['_wan'+u+'_ckmtd'] = 0;
			E('_wan'+u+'_ckmtd').disabled = 1;
		}

		if (wanproto[uidx - 1] != 'disabled' && uidx <= curr_mwan_num) {
			vis._f_automatic_ip = 0;
			vis._lan_gateway = 0;
			vis._f_dns_1 = 0;
			vis._f_dns_2 = 0;
		}
		else { /* hide gateway and dns if automatic ip is turned on! */
			if (E('_f_automatic_ip').checked) {
				vis._lan_gateway = 0;
				vis._f_dns_1 = 0;
				vis._f_dns_2 = 0;
			}
		}

		vis['_wan'+u+'_ppp_idletime'] = (E('_wan'+u+'_ppp_demand').value == 1) && vis['_wan'+u+'_ppp_demand'];
		vis['_wan'+u+'_ppp_demand_dnsip'] = vis['_wan'+u+'_ppp_idletime'];
		vis['_wan'+u+'_ppp_redialperiod'] = !vis['_wan'+u+'_ppp_idletime'] && vis['_wan'+u+'_ppp_demand'];
		vis['_wan'+u+'_ppp_passwd'] = vis['_wan'+u+'_ppp_username'];

		if (vis['_wan'+u+'_mtu_enable']) {
			if (E('_wan'+u+'_mtu_enable').value == 0) {
				vis['_f_wan'+u+'_mtu'] = 2;
				a = E('_f_wan'+u+'_mtu');
				switch (E('_wan'+u+'_proto').value) {
				case 'pppoe':
					a.value = 1492;
				break;
				case 'pptp':
				case 'l2tp':
					a.value = 1460;
				break;
				default:
					a.value = 1500;
				break;
				}
			}
		}
	}

	for (uidx = 0; uidx < wl_ifaces.length; ++uidx) {
		if (wl_sunit(uidx) < 0) {
			u = wl_unit(uidx);
			wmode = E('_f_wl'+u+'_mode').value;

			if (!E('_f_wl'+u+'_radio').checked) { /* WL is disabled */
				for (a in wl_vis[uidx]) {
					wl_vis[uidx][a] = 2;
				}
				wl_vis[uidx]._f_wl_radio = 1;
				wl_vis[uidx]._wl_nbw_cap = nphy || acphy ? 2 : 0;
				wl_vis[uidx]._f_wl_nband = (bands[uidx].length > 1) ? 2 : 0;

				for (c = 1; c <= curr_mwan_num; ++c) { /* on every WAN */
					d = (c > 1) ? c : '';
					sta_wl = E('_wan'+d+'_sta');

					if (sta_wl.value == 'wl'+u) /* 'sta' is set, so change to 'disabled' */
						sta_wl.value = '';

					for (e = 0; e < sta_wl.options.length; ++e) {
						if (sta_wl.options[e].value == 'wl'+u)
							sta_wl.options[e].disabled = 1; /* and disable 'sta' option */
					}
				}
			}
			else { /* WL is enabled */
				for (c = 1; c <= curr_mwan_num; ++c) {
					d = (c > 1) ? c : '';
					var sta_wl = E('_wan'+d+'_sta');

					for (e = 0; e < sta_wl.options.length; ++e) {
						if (sta_wl.options[e].value == 'wl'+u)
							sta_wl.options[e].disabled = 0; /* so enable 'sta' option */
					}
				}
			}

			switch (wmode) {
			case 'apwds':
			case 'wds':
				break;
			case 'wet':
/* BCMWL6-BEGIN */
			case 'psta':
/* BCMWL6-END */
			case 'sta':
				wl_vis[uidx]._f_wl_bcast = 0;
				/* wl_vis[uidx]._wl_channel = 0; */
				/* wl_vis[uidx]._wl_nbw_cap = 0; */
				vis['_wan_modem_ipaddr'] = 0;
			default:
				wl_vis[uidx]._f_wl_lazywds = 0;
				wl_vis[uidx]._f_wl_wds_0 = 0;
				break;
			}

			sm2 = E('_wl'+u+'_security_mode').value;
			switch (sm2) {
			case 'disabled':
				wl_vis[uidx]._wl_crypto = 0;
				wl_vis[uidx]._wl_wep_bit = 0;
				wl_vis[uidx]._wl_wpa_psk = 0;
				wl_vis[uidx]._wl_radius_key = 0;
				wl_vis[uidx]._wl_radius_ipaddr = 0;
				wl_vis[uidx]._wl_wpa_gtk_rekey = 0;
				break;
			case 'wep':
				wl_vis[uidx]._wl_crypto = 0;
				wl_vis[uidx]._wl_wpa_psk = 0;
				wl_vis[uidx]._wl_radius_key = 0;
				wl_vis[uidx]._wl_radius_ipaddr = 0;
				wl_vis[uidx]._wl_wpa_gtk_rekey = 0;
				break;
			case 'radius':
				wl_vis[uidx]._wl_crypto = 0;
				wl_vis[uidx]._wl_wpa_psk = 0;
				break;
			default:	/* wpaX */
				wl_vis[uidx]._wl_wep_bit = 0;
				if (sm2.indexOf('personal') != -1) {
					wl_vis[uidx]._wl_radius_key = 0;
					wl_vis[uidx]._wl_radius_ipaddr = 0;
				} else
					wl_vis[uidx]._wl_wpa_psk = 0;

				break;
			}

			if ((E('_f_wl'+u+'_lazywds').value == 1) && (wl_vis[uidx]._f_wl_wds_0 == 1))
				wl_vis[uidx]._f_wl_wds_0 = 2;

			if (wl_vis[uidx]._wl_nbw_cap != 0) {
				switch (E('_wl'+u+'_net_mode').value) {
				case 'b-only':
				case 'g-only':
				case 'a-only':
				case 'bg-mixed':
					wl_vis[uidx]._wl_nbw_cap = 2;
					if (E('_wl'+u+'_nbw_cap').value != '0') {
						E('_wl'+u+'_nbw_cap').value = 0;
						refreshChannels(uidx);
					}
					break;
				}
				/* avoid Enterprise-TKIP with 40MHz */
				if ((sm2 == 'wpa_enterprise') && (E('_wl'+u+'_crypto').value == 'tkip')) {
					wl_vis[uidx]._wl_nbw_cap = 2;
					if (E('_wl'+u+'_nbw_cap').value != '0') {
						E('_wl'+u+'_nbw_cap').value = 0;
						refreshChannels(uidx);
					}
				}
			}

			wl_vis[uidx]._f_wl_nctrlsb = (E('_wl'+u+'_nbw_cap').value == 0) ? 0 : wl_vis[uidx]._wl_nbw_cap;

/* REMOVE-BEGIN
	This is ugly...
	Special case - 2.4GHz band, currently running in B/G-only mode,
	with N/Auto and 40MHz selected in the GUI.
	Channel list is not filtered in this case by the wl driver,
	and includes all channels available with 20MHz channel width.
REMOVE-END */
			b = selectedBand(uidx);
			if (wl_vis[uidx]._wl_channel == 1 && wl_vis[uidx]._f_wl_nctrlsb != 0 && ((b == '2') || (wl_vis[uidx]._f_wl_nband == 0 && b == '0'))) {
				switch (eval('nvram.wl'+u+'_net_mode')) {
				case 'b-only':
				case 'g-only':
				case 'bg-mixed':
					i = E('_wl'+u+'_channel').value * 1;
					if (i > 0 && i < 5) {
						E('_f_wl'+u+'_nctrlsb').value = 'lower';
						wl_vis[uidx]._f_wl_nctrlsb = 2;
					}
					else if (i > max_channel[uidx] - 4) {
						E('_f_wl'+u+'_nctrlsb').value = 'upper';
						wl_vis[uidx]._f_wl_nctrlsb = 2;
					}
					break;
				}
			}

			wl_vis[uidx]._f_wl_scan = wl_vis[uidx]._wl_channel;
			wl_vis[uidx]._f_wl_psk_random1 = wl_vis[uidx]._wl_wpa_psk;
			wl_vis[uidx]._f_wl_psk_random2 = wl_vis[uidx]._wl_radius_key;
			wl_vis[uidx]._wl_radius_port = wl_vis[uidx]._wl_radius_ipaddr;
			wl_vis[uidx]._wl_key1 = wl_vis[uidx]._wl_key2 = wl_vis[uidx]._wl_key3 = wl_vis[uidx]._wl_key4 = wl_vis[uidx]._f_wl_wep_gen = wl_vis[uidx]._f_wl_wep_random = wl_vis[uidx]._wl_passphrase = wl_vis[uidx]._wl_wep_bit;

			for (i = 1; i < 10; ++i)
				wl_vis[uidx]['_f_wl_wds_'+i] = wl_vis[uidx]._f_wl_wds_0;
		}
	} /* for each wl_iface */

	/* apply visibility and disabled attribute */
	for (a in vis) {
		b = E(a);
		c = vis[a];
		b.disabled = (c != 1);
		PR(b).style.display = (c ? 'table-row' : 'none');
	}

	for (uidx = 1; uidx <= maxwan_num; ++uidx) {
		u = (uidx > 1) ? uidx : '';
		if (wanproto[uidx - 1] == 'static') {
			E('_wan'+u+'_dns_auto').value = '0';
			E('_wan'+u+'_dns_auto').options[0].disabled = 1;
			elem.display(E('dns'+u+'_faq'), 0);
		}
		else {
			E('_wan'+u+'_dns_auto').options[0].disabled = 0;
			/* disable DNS and set to Auto if dnscrypt/Stubby with No-Resolv is enabled (except for static proto) */
			if ((nvram.dnscrypt_proxy == '1' && nvram.dnscrypt_priority == '2') || (nvram.stubby_proxy == '1' && nvram.stubby_priority == '2')) {
				E('_wan'+u+'_dns_auto').value = '1';
				E('_wan'+u+'_dns_auto').disabled = 1;
				elem.display(E('dns'+u+'_faq'), 1);
			}
			else {
				E('_wan'+u+'_dns_auto').disabled = 0;
				elem.display(E('dns'+u+'_faq'), 0);
			}
		}
		/* hide dns1/dns2 when set to auto or invisible */
		if ((PR('_wan'+u+'_dns_auto').style.display == 'none') || (E('_wan'+u+'_dns_auto').value == '1')) {
			PR('_f_wan'+u+'_dns_1').style.display = 'none';
			PR('_f_wan'+u+'_dns_2').style.display = 'none';
		}
	}

	for (uidx = 0; uidx < wl_ifaces.length; ++uidx) {
		if (wl_ifaces[uidx][0].indexOf('.') < 0) {
			for (a in wl_vis[uidx]) {
				i = 3;
				if (a.substr(0, 6) == '_f_wl_')
					i = 5;

				b = E(a.substr(0, i) + wl_unit(uidx) + a.substr(i, a.length));
				c = wl_vis[uidx][a];
				b.disabled = (c != 1);
				PR(b).style.display = (c ? 'table-row' : 'none');
			}
		}
	}

	E(PR('_f_mwan_tune_gc')).style.display = (E('_mwan_num').value > 1 ? '' : 'none');

	/* --- verify --- */

	for (uidx = 1; uidx <= maxwan_num; ++uidx) {
		u = (uidx > 1) ? uidx : '';
		ferror.clear('_wan'+u+'_proto');
	}

	var wlclnt = 0;
	for (uidx = 0; uidx < wl_ifaces.length; ++uidx) {
		if (wl_sunit(uidx) < 0) {
			u = wl_unit(uidx);
			wmode = E('_f_wl'+u+'_mode').value;
			sm2 = E('_wl'+u+'_security_mode').value;

			/* --- N standard does not support WPA+TKIP --- */
			a = E('_wl'+u+'_crypto');
			switch (E('_wl'+u+'_net_mode').value) {
			case 'mixed':
			case 'n-only':
/* BCMWL6-BEGIN */
			case 'nac-mixed':
			case 'ac-only':
/* BCMWL6-END */
				if ((nphy || acphy) && (a.value == 'tkip') && (sm2.indexOf('wpa') != -1)) {
					ferror.set(a, 'TKIP encryption is not supported with WPA / WPA2 in N and/or AC mode.', quiet || !ok);
					ok = 0;
				}
				else
					ferror.clear(a);

				break;
			}

			a = E('_wl'+u+'_net_mode');
			ferror.clear(a);
			b = E('_f_wl'+u+'_mode');
			ferror.clear(b);
			if ((wmode == 'sta') || (wmode == 'wet') ||
/* BCMWL6-BEGIN */
			    (wmode == 'psta') ||
/* BCMWL6-END */
			    0) {
				++wlclnt;
				if (wlclnt > 1) {
					ferror.set(b, 'Only one wireless interface can be configured in client mode.', quiet || !ok);
					ok = 0;
				}
				else if (a.value == 'n-only') {
					ferror.set(a, 'N-only is not supported in wireless client modes, use Auto.', quiet || !ok);
					ok = 0;
				}
/* BCMWL6-BEGIN */
				else if (a.value == 'nac-mixed') {
					ferror.set(a, 'N/AC Mixed is not supported in wireless client modes, use Auto.', quiet || !ok);
					ok = 0;
				}
				else if (a.value == 'ac-only') {
					ferror.set(a, 'AC-only is not supported in wireless client modes, use Auto.', quiet || !ok);
					ok = 0;
				}
/* BCMWL6-END */
			}

			a = E('_wl'+u+'_wpa_psk');
			ferror.clear(a);
			if (wl_vis[uidx]._wl_wpa_psk == 1) {
				if ((a.value.length < 8) || ((a.value.length == 64) && (a.value.search(/[^0-9A-Fa-f]/) != -1))) {
					ferror.set('_wl'+u+'_wpa_psk', 'Invalid pre-shared key. Please enter at least 8 characters or 64 hexadecimal digits.', quiet || !ok);
					ok = 0;
				}
			}

			/* wl channel */
			if (((wmode == 'wds') || (wmode == 'apwds')) && (wl_vis[uidx]._wl_channel == 1) && (E('_wl'+u+'_channel').value == '0')) {
				ferror.set('_wl'+u+'_channel', 'Fixed wireless channel required in WDS mode.', quiet || !ok);
				ok = 0;
			}
			else
				ferror.clear('_wl'+u+'_channel');
		}
	}

	for (uidx = 1; uidx <= curr_mwan_num; ++uidx) {
		u = (uidx > 1) ? uidx : '';
		/* domain name or IP address */
		if ((vis['_wan'+u+'_l2tp_server_ip']) && ((!v_length('_wan'+u+'_l2tp_server_ip', 1, 1)) || ((!v_ip('_wan'+u+'_l2tp_server_ip', 1)) && (!v_domain('_wan'+u+'_l2tp_server_ip', 1))))) {
			ok = 0;
			if (!quiet) ferror.show('_wan'+u+'_l2tp_server_ip');
		}
		if ((vis['_wan'+u+'_pptp_server_ip']) && ((!v_length('_wan'+u+'_pptp_server_ip', 1, 1)) || ((!v_ip('_wan'+u+'_pptp_server_ip', 1)) && (!v_domain('_wan'+u+'_pptp_server_ip', 1))))) {
			ok = 0;
			if (!quiet) ferror.show('_wan'+u+'_pptp_server_ip');
		}
		/* WANx IP address */
		if ((vis['_wan'+u+'_ipaddr']) && (!v_ip('_wan'+u+'_ipaddr', quiet))) ok = 0;
		if ((vis['_wan'+u+'_gateway']) && (!v_ip('_wan'+u+'_gateway', quiet))) ok = 0;
		if ((vis['_wan'+u+'_modem_ipaddr']) && (!v_ip('_wan'+u+'_modem_ipaddr', quiet))) ok = 0;
/* USB-BEGIN */
		if ((vis['_wan'+u+'_hilink_ip']) && (!v_ip('_wan'+u+'_hilink_ip', quiet))) ok = 0;
/* USB-END */
		if ((vis['_wan'+u+'_ppp_demand_dnsip']) && (!v_ip('_wan'+u+'_ppp_demand_dnsip', quiet))) ok = 0;
		/* WANx netmask */
		if ((vis['_wan'+u+'_netmask']) && (!v_netmask('_wan'+u+'_netmask', quiet))) ok = 0;
		/* range */
		if ((vis['_wan'+u+'_ppp_idletime']) && (!v_range('_wan'+u+'_ppp_idletime', quiet, 3, 1440))) ok = 0;
		if ((vis['_wan'+u+'_ppp_redialperiod']) && (!v_range('_wan'+u+'_ppp_redialperiod', quiet, 1, 86400))) ok = 0;
		if ((vis['_f_wan'+u+'_mtu']) && (!v_range('_f_wan'+u+'_mtu', quiet, 576, 1500))) ok = 0;
		if ((vis['_wan'+u+'_pppoe_lei']) && (!v_range('_wan'+u+'_pppoe_lei', quiet, 1, 60))) ok = 0;
		if ((vis['_wan'+u+'_pppoe_lef']) && (!v_range('_wan'+u+'_pppoe_lef', quiet, 1, 10))) ok = 0;
	}

	/* IP address, blank -> 0.0.0.0 */
	a = ['_f_dns_1', '_f_dns_2', '_lan_gateway'];
	for (i = a.length - 1; i >= 0; --i) {
		if ((vis[a[i]]) && (!v_dns(a[i], quiet || !ok)))
			ok = 0;
	}

	for (uidx = 0; uidx < wl_ifaces.length; ++uidx) {
		if (wl_sunit(uidx) < 0) {
			u = wl_unit(uidx);

			/* IP address */
			a = ['_radius_ipaddr'];
			for (i = a.length - 1; i >= 0; --i) {
				if ((wl_vis[uidx]['_wl'+a[i]]) && (!v_ip('_wl'+u+a[i], quiet || !ok))) ok = 0;
			}

			/* range */
			a = [['_wpa_gtk_rekey', 60, 7200], ['_radius_port', 1, 65535]];
			for (i = a.length - 1; i >= 0; --i) {
				v = a[i];
				if ((wl_vis[uidx]['_wl'+v[0]]) && (!v_range('_wl'+u+v[0], quiet || !ok, v[1], v[2]))) ok = 0;
			}

			/* length */
			a = [['_ssid', 1], ['_radius_key', 1]];
			for (i = a.length - 1; i >= 0; --i) {
				v = a[i];
				if ((wl_vis[uidx]['_wl'+v[0]]) && (!v_length('_wl'+u+v[0], quiet || !ok, v[1], E('_wl'+u+v[0]).maxlength))) ok = 0;
			}

			if (wl_vis[uidx]._wl_key1) {
				a = (E('_wl'+u+'_wep_bit').value == 128) ? 26 : 10;
				for (i = 1; i <= 4; ++i) {
					b = E('_wl'+u+'_key'+i);
					b.maxLength = a;
					if ((b.value.length > 0) || (E('_f_wl'+u+'_wepidx_'+i).checked)) {
						if (!v_wep(b, quiet || !ok)) ok = 0;
					}
					else
						ferror.clear(b);
				}
			}

			ferror.clear('_f_wl'+u+'_wds_0');
			if (wl_vis[uidx]._f_wl_wds_0 == 1) {
				b = 0;
				for (i = 0; i < 10; ++i) {
					a = E('_f_wl'+u+'_wds_'+i);
					if (!v_macz(a, quiet || !ok))
						ok = 0;
					else if (!isMAC0(a.value))
						b = 1;
				}
				if (!b) {
					ferror.set('_f_wl'+u+'_wds_0', 'WDS MAC address required.', quiet || !ok);
					ok = 0;
				}
			}
		}
	}

	if (typeof(wl_mode_last) == 'undefined')
		wl_mode_last = [];

	for (uidx = 0; uidx < wl_ifaces.length; ++uidx) {
		if (wl_sunit(uidx) < 0) {
			u = wl_unit(uidx);
			if (typeof(wl_mode_last[u]) == 'undefined')
				wl_mode_last[u] = E('_f_wl'+u+'_mode').value;

			wmode = E('_f_wl'+u+'_mode');
			wmode.options[0].disabled = 0;
			wmode.options[1].disabled = 0;
			wmode.options[2].disabled = 1;
			wmode.options[3].disabled = 0;
			wmode.options[4].disabled = 0;
/* BCMWL6-BEGIN */
			wmode.options[5].disabled = 0;
/* BCMWL6-END */
		}
	}

	/* first reset all previous 'sta' modes on every WL */
	for (uidx = 0; uidx < wl_ifaces.length; ++uidx) {
		if (wl_sunit(uidx) < 0) {
			u = wl_unit(uidx);

			if (E('_f_wl'+u+'_mode').value == 'sta') {
				if (wl_mode_last[u] != 'sta') /* only if last mode (on loading the page) is not 'sta'! */
					E('_f_wl'+u+'_mode').value = wl_mode_last[u];
				else /* reset to the most popular 'ap' mode */
					E('_f_wl'+u+'_mode').value = 'ap';
			}
		}
	}

	/* then apply new ones (if any) */
	for (uidx = 1; uidx <= curr_mwan_num; ++uidx) {
		u = (uidx > 1) ? uidx : '';
		sta_wl = E('_wan'+u+'_sta').value;

		/* sta_wl: wl0, wl1, wl2 */
		if (sta_wl != '') {
			wmode = E('_f_'+sta_wl+'_mode');
			wmode.value = 'sta';
			wmode.options[0].disabled = 1;
			wmode.options[1].disabled = 1;
			wmode.options[2].disabled = 0;
			wmode.options[3].disabled = 1;
			wmode.options[4].disabled = 1;
/* BCMWL6-BEGIN */
			wmode.options[5].disabled = 1;
/* BCMWL6-END */
		}
	}

	if (curr_mwan_num == 1) {
		elem.display(PR('_wan_weight'), 0);
		E('_wan_weight').disabled = 1;
	}

/* USB-BEGIN */
	for (uidx = 1; uidx <= curr_mwan_num; ++uidx) {
		u = (uidx > 1) ? uidx : '';
		var lte3g = E('_wan'+u+'_proto').value;
		if (lte3g == 'lte' || lte3g == 'ppp3g') {
			for (i = uidx + 1; i <= curr_mwan_num; ++i) {
				if ((E('_wan'+i+'_proto').value == 'lte') || (E('_wan'+i+'_proto').value == 'ppp3g')) {
					ferror.set('_wan'+i+'_proto', '3G or LTE mode can be set only to one WAN port', quiet || !ok);
					ok = 0;
				}
			}
		}
	}
/* USB-END */

	return ok;
}

function pre_submit(s) {
	if (cmd)
		return;

	cmd = new XmlHttp();
	cmd.onCompleted = function(text, xml) {
		eval(text);
		cmd = null;
	}
	cmd.onError = function(x) {
		cmd = null;
	}

	cmd.post('shell.cgi', 'action=execute&command='+escapeCGI(s.replace(/\r/g, '')));
}

function save() {
	if (lg.isEditing())
		return;
	lg.resetNewEditor();

	var a, b, c, d, e;
	var i, j;
	var u, uidx, wan_uidx, wmode, sm2, wradio;
	var curr_mwan_num = E('_mwan_num').value;
	var s = '';

	if (!verifyFields(null, 0))
		return;

	var fom = E('t_fom');

	for (uidx = 0; uidx < wl_ifaces.length; ++uidx) {
		if (wl_sunit(uidx) < 0) {
			u = wl_unit(uidx);
			wmode = E('_f_wl'+u+'_mode').value;
			sm2 = E('_wl'+u+'_security_mode').value;
			wradio = E('_f_wl'+u+'_radio').checked;

			E('_wl'+u+'_nband').value = selectedBand(uidx);

			if (wmode == 'apwds')
				E('_wl'+u+'_mode').value = 'ap';
			else
				E('_wl'+u+'_mode').value = wmode;

			if ((wmode == 'wet') ||
/* BCMWL6-BEGIN */
			    (wmode == 'psta') ||
/* BCMWL6-END */
			    0) {
				for (wan_uidx = 1; wan_uidx <= maxwan_num; ++wan_uidx) {
					d = (wan_uidx > 1) ? wan_uidx : '';
					E('_wan'+d+'_proto').disabled = 0;
					E('_wan'+d+'_proto').value = 'disabled';
				}
			}

			a = [];
			for (i = 0; i < 10; ++i) a.push(E('_f_wl'+u+'_wds_'+i).value);
			E('_wl'+u+'_wds').value = joinAddr(a);

			if (wmode.indexOf('wds') != -1) {
				E('_wl'+u+'_wds_enable').value = 1;
				E('_wl'+u+'_lazywds').value = E('_f_wl'+u+'_lazywds').value;
				if (E('_wl'+u+'_lazywds').value == 1)
					E('_wl'+u+'_wds').value = '';
			}
			else {
				E('_wl'+u+'_wds_enable').value = 0;
				E('_wl'+u+'_wds').value = '';
				E('_wl'+u+'_lazywds').value = 0;
			}

			E('_wl'+u+'_radio').value = wradio ? 1 : 0;
			E('_wl'+u+'_auth').value = eval('nvram.wl'+u+'_auth');

			e = E('_wl'+u+'_akm');
			switch (sm2) {
			case 'disabled':
			case 'radius':
			case 'wep':
				e.value = '';
				break;
			default:
				c = [];

				if (sm2.indexOf('personal') != -1) {
					if (sm2.indexOf('wpa2_') == -1) c.push('psk');
					if (sm2.indexOf('wpa_') == -1) c.push('psk2');
				}
				else {
					if (sm2.indexOf('wpa2_') == -1) c.push('wpa');
					if (sm2.indexOf('wpa_') == -1) c.push('wpa2');
				}
				c = c.join(' ');
				e.value = c;
				break;
			}
			E('_wl'+u+'_auth_mode').value = (sm2 == 'radius') ? 'radius' : 'none';
			E('_wl'+u+'_wep').value = ((sm2 == 'radius') || (sm2 == 'wep')) ? 'enabled': 'disabled';

			if (sm2.indexOf('wpa') != -1)
				E('_wl'+u+'_auth').value = 0;

			E('_wl'+u+'_nreqd').value = 0;
			E('_wl'+u+'_gmode').value = 1;
			E('_wl'+u+'_nmode').value = 0;
			E('_wl'+u+'_nmcsidx').value = -2; /* Legacy Rate */
			E('_wl'+u+'_nbw').value = 0;
/* BCMWL6-BEGIN */
			E('_wl'+u+'_bss_opmode_cap_reqd').value = 0; /* no requirements for joining clients */
/* BCMWL6-END */
			switch (E('_wl'+u+'_net_mode').value) {
			case 'b-only':
				E('_wl'+u+'_gmode').value = 0;
				break;
			case 'g-only':
				E('_wl'+u+'_gmode').value = 2;
/* BCMWL6-BEGIN */
				E('_wl'+u+'_bss_opmode_cap_reqd').value = 1; /* client must advertise ERP / 11g cap. to be able to join */
/* BCMWL6-END */
				break;
			case 'bg-mixed':
				break;
			case 'a-only':
				E('_wl'+u+'_nmcsidx').value = -1; /* Auto */
				break;
			case 'n-only':
				if (selectedBand(uidx) == '1') { /* 5 GHz */
					E('_wl'+u+'_nmode').value = -1;
					E('_wl'+u+'_nmcsidx').value = -1;
				}
				else { /* 2.4 GHz */
					E('_wl'+u+'_nmode').value = 1;
					E('_wl'+u+'_nmcsidx').value = 32;
				}
				E('_wl'+u+'_nreqd').value = 1; /* require 11n support (SDK5) */
/* BCMWL6-BEGIN */
				E('_wl'+u+'_bss_opmode_cap_reqd').value = 2; /* client must advertise HT / 11n cap. to be able to join */
/* BCMWL6-END */
				break;
/* BCMWL6-BEGIN */
			case 'nac-mixed': /* only 5 GHz */
				E('_wl'+u+'_nmode').value = -1; /* Auto */
				E('_wl'+u+'_nmcsidx').value = -1; /* Auto */
				E('_wl'+u+'_bss_opmode_cap_reqd').value = 2; /* client must advertise HT / 11n cap. to be able to join */
				break;
			case 'ac-only': /* only 5 GHz */
				E('_wl'+u+'_nmode').value = -1; /* Auto */
				E('_wl'+u+'_nmcsidx').value = -1; /* Auto */
				E('_wl'+u+'_bss_opmode_cap_reqd').value = 3; /* client must advertise VHT / 11ac cap. to be able to join */
				break;
/* BCMWL6-END */
			default: /* Auto */
				E('_wl'+u+'_nmode').value = -1;
				E('_wl'+u+'_nmcsidx').value = -1;
				break;
			}

			E('_wl'+u+'_nctrlsb').value = eval('nvram.wl'+u+'_nctrlsb');
			if (E('_wl'+u+'_nmode').value != 0) {
				E('_wl'+u+'_nctrlsb').value = E('_f_wl'+u+'_nctrlsb').value;
				E('_wl'+u+'_nbw').value = (E('_wl'+u+'_nbw_cap').value == 0) ? 20 : ((E('_wl'+u+'_nbw_cap').value== 3) ? 80:40);
			}

			E('_wl'+u+'_closed').value = E('_f_wl'+u+'_bcast').checked ? 0 : 1;

			a = fields.radio.selected(eval('fom.f_wl'+u+'_wepidx'));
			if (a)
				E('_wl'+u+'_key').value = a.value;
		}
	}

	fom.lan_dhcp.value = fom.f_automatic_ip.checked ? 1 : 0;

	fom.lan_state.value = fom.f_lan_state.checked ? 1 : 0;
	fom.lan_desc.value = fom.f_lan_desc.checked ? 1 : 0;
	fom.lan_invert.value = fom.f_lan_invert.checked ? 1 : 0;
/* BSD-BEGIN */
	fom.smart_connect_x.value = fom.f_smart_connect_x.checked ? 1 : 0;
/* BSD-END */

/* initialize/wipe out relevant fields */
	for (i = 0 ; i <= MAX_BRIDGE_ID ; i++) {
		j = ((i == 0) ? '' : i.toString());
		fom['lan'+j+'_ifname'].value = '';
		fom['lan'+j+'_ipaddr'].value = '';
		fom['lan'+j+'_netmask'].value = '';
		fom['lan'+j+'_proto'].value = '';
		fom['lan'+j+'_stp'].value = '';
		fom['dhcp'+j+'_lease'].value = '';
		fom['dhcpd'+j+'_startip'].value = '';
		fom['dhcpd'+j+'_endip'].value = '';
	}

	d = lg.getAllData();
	for (i = 0; i < d.length; ++i) {
		if (lg.countOverlappingNetworks(d[i][2]) > 1) {
			c = 'Cannot proceed: two or more LAN bridges have conflicting IP addresses or overlapping subnets';
			alert(c);
			e = E('footer-msg');
			e.innerHTML = c;
			e.style.display = 'inline-block';
			setTimeout(
				function() {
					e.innerHTML = '';
					e.style.display = 'none';
				}, 5000);
			return;
		}

		j = (parseInt(d[i][0]) == 0) ? '' : d[i][0].toString();
		fom['lan'+j+'_ifname'].value = 'br'+d[i][0];
		fom['lan'+j+'_stp'].value = d[i][1];
		fom['lan'+j+'_ipaddr'].value = d[i][2];
		fom['lan'+j+'_netmask'].value = d[i][3];
		fom['lan'+j+'_proto'].value = (d[i][4] != '0') ? 'dhcp' : 'static';
		fom['dhcp'+j+'_lease'].value = (d[i][4] != '0') ? d[i][7] : '';
		fom['dhcpd'+j+'_startip'].value = (d[i][4] != '0') ? d[i][5] : '';
		fom['dhcpd'+j+'_endip'].value = (d[i][4] != '0') ? d[i][6] : '';

/* REMOVE-BEGIN
alert('lan'+j+'_ifname=' + fom['lan'+j+'_ifname'].value + '\n' +
	'lan'+j+'_stp=' + fom['lan'+j+'_stp'].value + '\n' +
	'lan'+j+'_ipaddr=' + fom['lan'+j+'_ipaddr'].value + '\n' +
	'lan'+j+'_netmask=' + fom['lan'+j+'_netmask'].value + '\n' +
	'lan'+j+'_proto=' + fom['lan'+j+'_proto'].value + '\n' +
	'dhcp'+j+'_lease=' + fom['dhcp'+j+'_lease'].value + '\n' +
	'dhcpd'+j+'_startip=' + fom['dhcpd'+j+'_startip'].value + '\n' +
	'dhcpd'+j+'_endip=' + fom['dhcpd'+j+'_endip'].value);
REMOVE-END */
	}

	e = E('footer-msg');
	d = fixIP(fom['lan_ipaddr'].value);
	if ((fom['lan_ifname'].value != 'br0') || (!d)) {
		e.innerHTML = 'Bridge br0 must be always defined';
		e.style.display = 'inline-block';
		setTimeout(
			function() {
				e.innerHTML = '';
				e.style.display = 'none';
				}, 5000);
		return;
	}

	for (uidx = 1; uidx <= maxwan_num; ++uidx) {
		u = (uidx > 1) ? uidx : '';
		fom['wan'+u+'_mtu'].value = fom['f_wan'+u+'_mtu'].value;
		fom['wan'+u+'_mtu'].disabled = fom['f_wan'+u+'_mtu'].disabled;
		fom['wan'+u+'_pptp_dhcp'].value = fom['f_wan'+u+'_pptp_dhcp'].checked ? 1 : 0;
		fom['wan'+u+'_ppp_mlppp'].value = fom['f_wan'+u+'_ppp_mlppp'].checked ? 1 : 0;
/* USB-BEGIN */
		fom['wan'+u+'_status_script'].value = fom['f_wan'+u+'_status_script'].checked ? 1 : 0;
/* USB-END */
		fom['wan'+u+'_ck_pause'].value = fom['f_wan'+u+'_ck_pause'].checked ? 1 : 0;

		if (fom['wan'+u+'_proto'].value == 'disabled') /* if wanX is disabled (for ex. wireless bridge / AP only) */
			fom['wan'+u+'_dns_auto'].value = '0'; /* make sure to use static dns! */

		if ((fom['wan'+u+'_proto'].value == 'disabled') || (uidx > curr_mwan_num)) { /* wipe out relevant fields (needed when switching, i.e. from 2 WANs to 1) */
			fom['wan'+u+'_iface'].value = '';
			fom['wan'+u+'_ifname'].value = '';
			fom['wan'+u+'_hwaddr'].value = '';

			if (uidx > curr_mwan_num)
				s += 'nvram set wan'+u+'_proto="disabled"\n';
		}

		if (fom['wan'+u+'_dns_auto'].value == '1')
			fom['wan'+u+'_dns'].value = '';
		else
			fom['wan'+u+'_dns'].value = joinAddr([fom['f_wan'+u+'_dns_1'].value, fom['f_wan'+u+'_dns_2'].value]);
	}

	/* set 'sta' to off when drop down list is disabled or hidden */
	e = E('sta_reset');
	e.innerHTML = ''; /* first wipe all possible contents */
	c = '';
	for (uidx = 1; uidx <= maxwan_num; ++uidx) {
		u = (uidx > 1) ? uidx : '';
		if ((E('_wan'+u+'_sta').disabled == 1) || (E(PR('_wan'+u+'_sta')).style.display == 'none'))
			c += '<input type="hidden" name="wan'+u+'_sta" value="">';
	}
	e.innerHTML = c;

	fom.wan_dns.value = joinAddr([fom.f_dns_1.value, fom.f_dns_2.value]); /* get static dns */
	for (uidx = 1; uidx <= curr_mwan_num; ++uidx) {
		u = (uidx > 1) ? uidx : '';
		if (fom['wan'+u+'_proto'].value != 'disabled') {
			fom.wan_dns.value = joinAddr([fom.f_wan_dns_1.value, fom.f_wan_dns_2.value]);
			break;
		}
	}

	if (E('_mwan_cktime').value)
		fom.mwan_ckdst.value = fom.f_mwan_ckdst_1.value+','+fom.f_mwan_ckdst_2.value;
	else
		fom.mwan_ckdst.value = '';

	fom.mwan_tune_gc.value = ((fom['_f_mwan_tune_gc'].checked && curr_mwan_num > 1) ? 1 : 0);

	if (s != '') /* workaround to set wanX_proto to disabled on every inactive WAN */
		pre_submit(s);

	/* lan IP will be changed via DHCP Client soon (after restart) */
	if (((fom.lan_dhcp.value == 1) || (nvram.lan_dhcp != fom.lan_dhcp.value)) && (fom.wan_proto.value == 'disabled')) {
		if (fom.lan_dhcp.value == 1) { /* Case: turn On (new) OR already turned On */
			fom.dhcp_moveip.value = 2; /* default lan IP 192.168.1.1 (waiting for DHCP Server Infos) - change IP to a.b.c.d */
			form.submit(fom);
		}
		else { /* Case: turn Off */
			fom.dhcp_moveip.value = 1; /* back to default lan IP 192.168.1.1 (nvram default) */
			form.submit(fom);
		}
	}
	/* lan IP changed (static case) */
	else if (nvram.lan_ipaddr != fom.lan_ipaddr.value) {
		fom.dhcp_moveip.value = 0;
		fom._moveip.value = 1;
		form.submit(fom);
	}
	else {
		fom.dhcp_moveip.value = 0;
		form.submit(fom, 1);
	}
}

function earlyInit() {
	var mwan = E('_mwan_num');
	if (nvram.wan_ifnameX.length < 1)
		mwan.options[0].disabled = 1;
	if (nvram.wan2_ifnameX.length < 1)
		mwan.options[1].disabled = 1;
/* MULTIWAN-BEGIN */
	if (nvram.wan3_ifnameX.length < 1)
		mwan.options[2].disabled = 1;
	if (nvram.wan4_ifnameX.length < 1)
		mwan.options[3].disabled = 1;
/* MULTIWAN-END */

	verifyFields(null, 1);
}

function init() {
	for (var uidx = 0; uidx < wl_ifaces.length; ++uidx) {
		if (wl_sunit(uidx) < 0) {
			refreshNetModes(uidx);
			refreshChannels(uidx);
			refreshBandWidth(uidx);
		}
	}
	refreshWanSection();
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

<input type="hidden" name="_nextpage" value="basic-network.asp">
<input type="hidden" name="_nextwait" value="10">
<input type="hidden" name="_reboot" value="0">
<input type="hidden" name="_service" value="*">
<input type="hidden" name="_moveip" value="0">
<input type="hidden" name="lan_dhcp">
<input type="hidden" name="lan_state">
<input type="hidden" name="lan_desc">
<input type="hidden" name="lan_invert">
<!-- BSD-BEGIN -->
<input type="hidden" name="smart_connect_x">
<!-- BSD-END -->
<input type="hidden" name="mwan_tune_gc" value="0">
<input type="hidden" name="dhcp_moveip" value="0">
<div id="sta_reset" style="display:none"></div>

<script>
	for (var i = 0 ; i <= MAX_BRIDGE_ID ; i++) {
		var j = (i == 0) ? '' : i.toString();
		W('<input type="hidden" id="lan'+j+'_ifname" name="lan'+j+'_ifname">');
		W('<input type="hidden" id="lan'+j+'_ipaddr" name="lan'+j+'_ipaddr">');
		W('<input type="hidden" id="lan'+j+'_netmask" name="lan'+j+'_netmask">');
		W('<input type="hidden" id="lan'+j+'_proto" name="lan'+j+'_proto">');
		W('<input type="hidden" id="lan'+j+'_stp" name="lan'+j+'_stp">');
		W('<input type="hidden" id="dhcp'+j+'_lease" name="dhcp'+j+'_lease">');
		W('<input type="hidden" id="dhcpd'+j+'_startip" name="dhcpd'+j+'_startip">');
		W('<input type="hidden" id="dhcpd'+j+'_endip" name="dhcpd'+j+'_endip">');
	}
</script>

<!-- / / / -->

<div class="section-title" id="mwan-title">MultiWAN</div>
<div class="section" id="mwan-section">
<input type="hidden" name="mwan_ckdst">
<script>
	function refreshWanSection() {
		var curr_mwan_num = E('_mwan_num').value;
		for (uidx = maxwan_num; uidx > 1; --uidx) {
			var u = (uidx > 1) ? uidx : '';
			elem.display('wan'+u+'-title', 'sesdiv_wan'+u, curr_mwan_num >= uidx);
		}
	}
	ckdst = nvram.mwan_ckdst.split(',');
	createFieldTable('', [
		{ title: 'Number of logical WANs', name: 'mwan_num', type: 'select', options: [['1','1 WAN'],['2','2 WAN']
/* MULTIWAN-BEGIN */
											   ,['3','3 WAN'],['4','4 WAN']
/* MULTIWAN-END */
			], value: nvram.mwan_num, suffix: '&nbsp; <small>Please configure <a href="advanced-vlan.asp">VLAN<\/a> first<\/small>' },
		{ title: 'Tune route cache', name: 'f_mwan_tune_gc', type: 'checkbox', suffix: '&nbsp; <small>for multiwan in load balancing mode<\/small>', value: (nvram['mwan_tune_gc'] == 1) },
		{ title: 'Check connections every', name: 'mwan_cktime', type: 'select', options: [
			['0','Disabled'],['30','30 seconds'],['60','1 minute*'],['120','2 minutes'],['180','3 minutes'],
			['300','5 minutes'],['600','10 minutes'],['900','15 minutes'],['1800','30 minutes']],
			suffix: '&nbsp; <small>*recommended<\/small>',
			value: nvram.mwan_cktime },
		{ title: 'Target 1', indent: 2, name: 'f_mwan_ckdst_1', type: 'text', maxlen: 30, size: 30, value: ckdst[0] || '', suffix: '&nbsp; <small>IP/domain<\/small>'},
		{ title: 'Target 2', indent: 2, name: 'f_mwan_ckdst_2', type: 'text', maxlen: 30, size: 30, value: ckdst[1] || ''}
	]);
	E('_mwan_num').onchange = function () {
		refreshWanSection();
		verifyFields(null, 1);
	}
</script>
</div>

<!-- / / / -->

<script>
	refresh_sta_list();
	for (var uidx = 1; uidx <= maxwan_num; ++uidx) {
		var u = (uidx > 1) ? uidx : '';
		dns = nvram['wan'+u+'_dns'].split(/\s+/);
		W('<input type="hidden" name="wan'+u+'_mtu">');
		W('<input type="hidden" name="wan'+u+'_pptp_dhcp">');
		W('<input type="hidden" name="wan'+u+'_ppp_mlppp">');
		W('<input type="hidden" name="wan'+u+'_dns">');
/* USB-BEGIN */
		W('<input type="hidden" name="wan'+u+'_status_script">');
/* USB-END */
		W('<input type="hidden" name="wan'+u+'_ck_pause">');
		W('<input type="hidden" name="wan'+u+'_iface">');
		W('<input type="hidden" name="wan'+u+'_ifname">');
		W('<input type="hidden" name="wan'+u+'_hwaddr">');

		W('<div class="section-title" id="wan'+u+'-title">WAN'+(uidx - 1)+' Settings<\/div>');
		W('<div class="section" id="sesdiv_wan'+u+'">');
		createFieldTable('', [
			{ title: 'Type', name: 'wan'+u+'_proto', type: 'select', options: [['dhcp','DHCP'],['pppoe','PPPoE'],['static','Static'],['pptp','PPTP'],['l2tp','L2TP'],
/* USB-BEGIN */
				['ppp3g','3G Modem'],
				['lte','4G/LTE'],
/* USB-END */
				['disabled','Disabled']],
				suffix: '&nbsp; <small id="_f_wan'+u+'_islan" style="display:none"><a href="advanced-vlan.asp">Bridge WAN ?<\/a><\/small>', value: nvram['wan'+u+'_proto'] },
			{ title: 'Wireless Client Mode', name: 'wan'+u+'_sta', type: 'select', options: sta_list, value: nvram['wan'+u+'_sta'] },
/* USB-BEGIN */
			{ title: 'Modem device', name: 'wan'+u+'_modem_dev', type: 'select', options: [['/dev/ttyUSB0','/dev/ttyUSB0'],['/dev/ttyUSB1','/dev/ttyUSB1'],['/dev/ttyUSB2','/dev/ttyUSB2'],['/dev/ttyUSB3','/dev/ttyUSB3'],['/dev/ttyUSB4','/dev/ttyUSB4'],['/dev/ttyUSB5','/dev/ttyUSB5'],['/dev/ttyUSB6','/dev/ttyUSB6'],['/dev/ttyACM0','/dev/ttyACM0']], value: nvram['wan'+u+'_modem_dev'] },
/* USB-END */
			{ title: 'Load Balance Weight', name: 'wan'+u+'_weight', type: 'text', maxlen: 3, size: 8, value: nvram['wan'+u+'_weight'], suffix: '&nbsp; <small>Failover: 0; Load balancing: 1 - 256<\/small>' },
/* USB-BEGIN */
			{ title: 'PIN Code', name: 'wan'+u+'_modem_pin', type: 'text', maxlen: 6, size: 8, value: nvram['wan'+u+'_modem_pin'], suffix: '&nbsp; <small>advised to turn off PIN Code<\/small>' },
			{ title: 'Modem init string', name: 'wan'+u+'_modem_init', type: 'text', maxlen: 25, size: 32, value: nvram['wan'+u+'_modem_init'] },
			{ title: 'APN', name: 'wan'+u+'_modem_apn', type: 'text', maxlen: 25, size: 32, suffix: '&nbsp; <small>if empty, AT+CGDCONT will not be sent<\/small>', value: nvram['wan'+u+'_modem_apn'] },
			{ title: 'Network Type', name: 'wan'+u+'_modem_speed', type: 'select', options: [['00','Auto'],['030201','4G/3G/2G'],['0302','4G/3G only'],['03','4G only'],['02','3G only']], value: nvram['wan'+u+'_modem_speed'], suffix: '&nbsp; <small>works only with non-Hilink modems<\/small>' },
			{ title: 'Roaming', name: 'wan'+u+'_modem_roam', type: 'select', options: [['2','No change*'],['1','Supported'],['0','Disabled'],['3','Roam only']], value: nvram['wan'+u+'_modem_roam'], suffix: '&nbsp; <small>*default; works only with non-Hilink modems<\/small>' },
			{ title: 'LTE Band', name: 'wan'+u+'_modem_band', type: 'select', options: [['7FFFFFFFFFFFFFFF','All supported*'],['80000','B20 (800 MHz)'],['80','B8 (900 MHz)'],['4','B3 (1800 MHz)'],['1','B1 (2100 MHz)'],['40','B7 (2600 MHz)']], value: nvram['wan'+u+'_modem_band'], suffix: '&nbsp; <small>*default; tested only on non-Hilink Huawei modems<\/small>' },
/* USB-END */
			{ title: 'Username', name: 'wan'+u+'_ppp_username', type: 'text', maxlen: 60, size: 64, value: nvram['wan'+u+'_ppp_username'] },
			{ title: 'Password', name: 'wan'+u+'_ppp_passwd', type: 'password', maxlen: 60, size: 64, peekaboo: 1, value: nvram['wan'+u+'_ppp_passwd'] },
			{ title: 'Service Name', name: 'wan'+u+'_ppp_service', type: 'text', maxlen: 50, size: 64, value: nvram['wan'+u+'_ppp_service'] },
			{ title: 'L2TP Server', name: 'wan'+u+'_l2tp_server_ip', type: 'text', maxlen: 128, size: 64, value: nvram['wan'+u+'_l2tp_server_ip'] },
			{ title: 'IP Address', name: 'wan'+u+'_ipaddr', type: 'text', maxlen: 15, size: 17, value: nvram['wan'+u+'_ipaddr'] },
			{ title: 'Subnet Mask', name: 'wan'+u+'_netmask', type: 'text', maxlen: 15, size: 17, value: nvram['wan'+u+'_netmask'] },
			{ title: 'Gateway', name: 'wan'+u+'_gateway', type: 'text', maxlen: 15, size: 17, value: nvram['wan'+u+'_gateway'] },
			{ title: 'PPTP Gateway', name: 'wan'+u+'_pptp_server_ip', type: 'text', maxlen: 128, size: 64, value: nvram['wan'+u+'_pptp_server_ip'] },
			{ title: 'Options', name: 'wan'+u+'_ppp_custom', type: 'text', maxlen: 256, size: 64, value: nvram['wan'+u+'_ppp_custom'] },
			{ title: 'DNS Server', name: 'wan'+u+'_dns_auto', type: 'select', options: [['1','Auto'],['0','Manual']], suffix: '&nbsp; <small id="dns'+u+'_faq">inactive when using dncrypt-proxy/Stubby with No-Resolv option<\/small>', value: nvram['wan'+u+'_dns_auto']},
			{ title: 'DNS 1', indent: 2, name: 'f_wan'+u+'_dns_1', type: 'text', maxlen: 21, size: 17, value: dns[0] || '0.0.0.0' },
			{ title: 'DNS 2', indent: 2, name: 'f_wan'+u+'_dns_2', type: 'text', maxlen: 21, size: 17, value: dns[1] || '0.0.0.0' },
			{ title: 'Connect Mode', name: 'wan'+u+'_ppp_demand', type: 'select', options: [['1','Connect On Demand'],['0','Keep Alive']],
				value: nvram['wan'+u+'_ppp_demand'] },
			{ title: 'IP to trigger Connect', indent: 2, name: 'wan'+u+'_ppp_demand_dnsip', type: 'text', maxlen: 15, size: 17, suffix: '&nbsp; <small>default: 198.51.100.1<\/small>',
				value: nvram['wan'+u+'_ppp_demand_dnsip'] },
			{ title: 'Max Idle Time', indent: 2, name: 'wan'+u+'_ppp_idletime', type: 'text', maxlen: 5, size: 7, suffix: '&nbsp; <small>minutes<\/small>',
				value: nvram['wan'+u+'_ppp_idletime'] },
			{ title: 'Redial Interval', indent: 2, name: 'wan'+u+'_ppp_redialperiod', type: 'text', maxlen: 5, size: 7, suffix: '&nbsp; <small>seconds<\/small>',
				value: nvram['wan'+u+'_ppp_redialperiod'] },
			{ title: 'LCP Echo Interval', indent: 2, name: 'wan'+u+'_pppoe_lei', type: 'text', maxlen: 5, size: 7, suffix: '&nbsp; <small>seconds; range: 1 - 60, default: 10<\/small>',
				value: nvram['wan'+u+'_pppoe_lei'] },
			{ title: 'LCP Echo Link fail limit', indent: 2, name: 'wan'+u+'_pppoe_lef', type: 'text', maxlen: 5, size: 7, suffix: '&nbsp; <small>range: 1 - 10, default: 5<\/small>',
				value: nvram['wan'+u+'_pppoe_lef'] },
			{ title: 'MTU', multi: [
				{ name: 'wan'+u+'_mtu_enable', type: 'select', options: [['0', 'Default'],['1','Manual']], value: nvram['wan'+u+'_mtu_enable'] },
				{ name: 'f_wan'+u+'_mtu', type: 'text', maxlen: 4, size: 6, value: nvram['wan'+u+'_mtu'] } ] },
			{ title: 'Use DHCP', name: 'f_wan'+u+'_pptp_dhcp', type: 'checkbox', value: (nvram['wan'+u+'_pptp_dhcp'] == 1) },
			{ title: 'Single Line MLPPP', name: 'f_wan'+u+'_ppp_mlppp', type: 'checkbox', value: (nvram['wan'+u+'_ppp_mlppp'] == 1) },

			{ title: 'Route Modem IP', name: 'wan'+u+'_modem_ipaddr', type: 'text', maxlen: 15, size: 17, suffix: '&nbsp; <small>must be in different subnet to router, 0.0.0.0 to disable<\/small>', value: nvram['wan'+u+'_modem_ipaddr'] },
/* USB-BEGIN */
			{ title: 'Query HiLink Modem IP', name: 'wan'+u+'_hilink_ip', type: 'text', maxlen: 15, size: 17, suffix: '&nbsp; <small>show status of reachable hilink modem, 0.0.0.0 to disable<\/small>', value: nvram['wan'+u+'_hilink_ip'] },
			{ title: 'Call Custom Status Script', name: 'f_wan'+u+'_status_script', type: 'checkbox', suffix: '&nbsp; <small>Call /www/user/cgi-bin/wan'+u+'_status.sh in the home page. Must output HTML<\/small>', value: (nvram['wan'+u+'_status_script'] == 1) },
/* USB-END */

			{ title: 'Disable Watchdog', name: 'f_wan'+u+'_ck_pause', type: 'checkbox', suffix: '&nbsp; <small>only for this WAN<\/small>', value: (nvram['wan'+u+'_ck_pause'] == 1) },
			{ title: 'Watchdog Mode', name: 'wan'+u+'_ckmtd', type: 'select', options: [['1','Ping'],['2','Traceroute*']
/* BBT-BEGIN */
				,['3','Curl']
/* BBT-END */
				], value: nvram['wan'+u+'_ckmtd'], suffix: '&nbsp; <small>*default; use the other method only when Traceroute is not working correctly<\/small>' }
		]);
		W('<\/div>');
	}
</script>

<!-- / / / -->

<div class="section-title" id="section-lan">LAN</div>
<div class="section">
<div class="tomato-grid" id="lan-grid"></div>
<script>
	lg.setup();

	dns = nvram.wan_dns.split(/\s+/);

	createFieldTable('', [
		{ title: 'Automatic IP', name: 'f_automatic_ip', type: 'checkbox', value: (nvram.lan_dhcp == 1), suffix: ' <small>(obtain IP/Gateway/DNS via DHCP)<\/small>' },
		{ title: 'Default Gateway', name: 'lan_gateway', type: 'text', maxlen: 15, size: 17, value: nvram.lan_gateway },
		{ title: 'Static DNS', suffix: '&nbsp; <small>IP:port<\/small>', name: 'f_dns_1', type: 'text', maxlen: 21, size: 25, value: dns[0] || '0.0.0.0' },
		{ title: '', name: 'f_dns_2', type: 'text', maxlen: 21, size: 25, value: dns[1] || '0.0.0.0' }
	]);
</script>
</div>

<!-- / / / -->

<div class="section-title" id="section-eth">Ethernet Ports State - Configuration</div>
<div class="section">
<script>
	createFieldTable('', [
		{ title: 'Enable Ports State', name: 'f_lan_state', type: 'checkbox', value: (nvram.lan_state == 1) },
		{ title: 'Show Speed Info', indent: 2, name: 'f_lan_desc', type: 'checkbox', value: (nvram.lan_desc == 1) },
		{ title: 'Invert Ports Order', indent: 2, name: 'f_lan_invert', type: 'checkbox', value: (nvram.lan_invert == 1) }
	]);
</script>
</div>

<!-- BSD-BEGIN -->

<div class="section-title" id="wl-bsd">Wireless Band Steering</div>
<div class="section">
<script>
	createFieldTable('', [
		{ title: 'Enable', name: 'f_smart_connect_x', type: 'checkbox', value: (nvram.smart_connect_x == 1), suffix: ' <small>(Wireless settings will be synced to module one after saving to nvram!)<\/small>' }
	]);
</script>
</div>

<!-- BSD-END -->

<script>
	for (var uidx = 0; uidx < wl_ifaces.length; ++uidx) {
		if (wl_sunit(uidx) < 0) {
			var u = wl_unit(uidx);

			W('<input type="hidden" id="_wl'+u+'_mode" name="wl'+u+'_mode">');
			W('<input type="hidden" id="_wl'+u+'_nband" name="wl'+u+'_nband">');
			W('<input type="hidden" id="_wl'+u+'_wds_enable" name="wl'+u+'_wds_enable">');
			W('<input type="hidden" id="_wl'+u+'_wds" name="wl'+u+'_wds">');
			W('<input type="hidden" id="_wl'+u+'_radio" name="wl'+u+'_radio">');
			W('<input type="hidden" id="_wl'+u+'_closed" name="wl'+u+'_closed">');
			W('<input type="hidden" id="_wl'+u+'_key" name="wl'+u+'_key">');
			W('<input type="hidden" id="_wl'+u+'_gmode" name="wl'+u+'_gmode">');
			W('<input type="hidden" id="_wl'+u+'_akm" name="wl'+u+'_akm">');
			W('<input type="hidden" id="_wl'+u+'_auth" name="wl'+u+'_auth">');
			W('<input type="hidden" id="_wl'+u+'_auth_mode" name="wl'+u+'_auth_mode">');
			W('<input type="hidden" id="_wl'+u+'_wep" name="wl'+u+'_wep">');
			W('<input type="hidden" id="_wl'+u+'_lazywds" name="wl'+u+'_lazywds">');
			W('<input type="hidden" id="_wl'+u+'_nmode" name="wl'+u+'_nmode">');
			W('<input type="hidden" id="_wl'+u+'_nmcsidx" name="wl'+u+'_nmcsidx">');
			W('<input type="hidden" id="_wl'+u+'_nreqd" name="wl'+u+'_nreqd">');
			W('<input type="hidden" id="_wl'+u+'_nctrlsb" name="wl'+u+'_nctrlsb">');
			W('<input type="hidden" id="_wl'+u+'_nbw" name="wl'+u+'_nbw">');
/* BCMWL6-BEGIN */
			W('<input type="hidden" id="_wl'+u+'_bss_opmode_cap_reqd" name="wl'+u+'_bss_opmode_cap_reqd">');
/* BCMWL6-END */
			W('<div class="section-title" id="wl'+u+'">Wireless');
			W(' '+wl_display_ifname(uidx));
			W('<\/div>');
			W('<div class="section">');

			var f = [
				{ title: 'Enable Wireless', name: 'f_wl'+u+'_radio', type: 'checkbox',
					value: (eval('nvram.wl'+u+'_radio') == '1') && (eval('nvram.wl'+u+'_net_mode') != 'disabled') },
				{ title: 'MAC Address', text: '<a href="advanced-mac.asp">'+eval('nvram.wl'+u+'_hwaddr')+'<\/a>' },
				{ title: 'Wireless Mode', name: 'f_wl'+u+'_mode', type: 'select',
					options: [['ap','Access Point'],['apwds','Access Point + WDS'],['sta','Wireless Client'],['wet','Wireless Ethernet Bridge'],['wds','WDS']
/* BCMWL6-BEGIN */
						  ,['psta','Media Bridge']
/* BCMWL6-END */
						 ],
					value: ((eval('nvram.wl'+u+'_mode') == 'ap') && (eval('nvram.wl'+u+'_wds_enable') == '1')) ? 'apwds' : eval('nvram.wl'+u+'_mode') },
				{ title: 'Radio Band', name: 'f_wl'+u+'_nband', type: 'select', options: bands[uidx],
					value: eval('nvram.wl'+u+'_nband') || '0' == '0' ? bands[uidx][0][0] : eval('nvram.wl'+u+'_nband') },
				{ title: 'Wireless Network Mode', name: 'wl'+u+'_net_mode', type: 'select',
					value: (eval('nvram.wl'+u+'_net_mode') == 'disabled') ? 'mixed' : eval('nvram.wl'+u+'_net_mode'),
					options: [], prefix: '<span id="__wl'+u+'_net_mode">', suffix: '<\/span>' },
				{ title: 'SSID', name: 'wl'+u+'_ssid', type: 'text', maxlen: 32, size: 34, value: eval('nvram.wl'+u+'_ssid') },
				{ title: 'Broadcast', indent: 2, name: 'f_wl'+u+'_bcast', type: 'checkbox', value: (eval('nvram.wl'+u+'_closed') == '0') },
				{ title: 'Channel', name: 'wl'+u+'_channel', type: 'select', options: ghz[uidx], prefix: '<span id="__wl'+u+'_channel">', suffix: '<\/span> <input type="button" id="_f_wl'+u+'_scan" value="Scan" onclick="scanButton('+u+')"> <img src="spin.gif" alt="" id="spin'+u+'">',
					value: eval('nvram.wl'+u+'_channel') },
				{ title: 'Channel Width', name: 'wl'+u+'_nbw_cap', type: 'select', options: [],
					value: eval('nvram.wl'+u+'_nbw_cap'), prefix: '<span id="__wl'+u+'_nbw_cap">', suffix: '<\/span>' },
				{ title: 'Control Sideband', name: 'f_wl'+u+'_nctrlsb', type: 'select', options: [['lower','Lower'],['upper','Upper']],
					value: eval('nvram.wl'+u+'_nctrlsb') == 'none' ? 'lower' : eval('nvram.wl'+u+'_nctrlsb') },
				null,
				{ title: 'Security', name: 'wl'+u+'_security_mode', type: 'select',
					options: [['disabled','Disabled'],['wep','WEP'],['wpa_personal','WPA Personal'],['wpa_enterprise','WPA Enterprise'],['wpa2_personal','WPA2 Personal'],['wpa2_enterprise','WPA2 Enterprise'],['wpaX_personal','WPA / WPA2 Personal'],['wpaX_enterprise','WPA / WPA2 Enterprise'],['radius','Radius']],
					value: eval('nvram.wl'+u+'_security_mode') },
				{ title: 'Encryption', indent: 2, name: 'wl'+u+'_crypto', type: 'select',
					options: [['tkip','TKIP'],['aes','AES'],['tkip+aes','TKIP / AES']], value: eval('nvram.wl'+u+'_crypto') },
				{ title: 'Shared Key', indent: 2, name: 'wl'+u+'_wpa_psk', type: 'password', maxlen: 64, size: 66, peekaboo: 1,
					suffix: ' <input type="button" id="_f_wl'+u+'_psk_random1" value="Random" onclick="random_psk(\'_wl'+u+'_wpa_psk\')">',
					value: eval('nvram.wl'+u+'_wpa_psk') },
				{ title: 'Shared Key', indent: 2, name: 'wl'+u+'_radius_key', type: 'password', maxlen: 80, size: 32, peekaboo: 1,
					suffix: ' <input type="button" id="_f_wl'+u+'_psk_random2" value="Random" onclick="random_psk(\'_wl'+u+'_radius_key\')">',
					value: eval('nvram.wl'+u+'_radius_key') },
				{ title: 'Group Key Renewal', indent: 2, name: 'wl'+u+'_wpa_gtk_rekey', type: 'text', maxlen: 4, size: 6, suffix: '&nbsp; <small>seconds<\/small>',
					value: eval('nvram.wl'+u+'_wpa_gtk_rekey') },
				{ title: 'Radius Server', indent: 2, multi: [
					{ name: 'wl'+u+'_radius_ipaddr', type: 'text', maxlen: 15, size: 17, value: eval('nvram.wl'+u+'_radius_ipaddr') },
					{ name: 'wl'+u+'_radius_port', type: 'text', maxlen: 5, size: 7, prefix: ' : ', value: eval('nvram.wl'+u+'_radius_port') } ] },
				{ title: 'Encryption', indent: 2, name: 'wl'+u+'_wep_bit', type: 'select', options: [['128','128-bits'],['64','64-bits']],
					value: eval('nvram.wl'+u+'_wep_bit') },
				{ title: 'Passphrase', indent: 2, name: 'wl'+u+'_passphrase', type: 'text', maxlen: 16, size: 20,
					suffix: ' <input type="button" id="_f_wl'+u+'_wep_gen" value="Generate" onclick="generate_wep('+u+')"> <input type="button" id="_f_wl'+u+'_wep_random" value="Random" onclick="random_wep('+u+')">',
					value: eval('nvram.wl'+u+'_passphrase') }
			];

			for (var i = 1; i <= 4; ++i) {
				f.push(
					{ title: ('Key '+i), indent: 2, name: ('wl'+u+'_key'+i), type: 'text', maxlen: 26, size: 34,
						suffix: '<input type="radio" onchange="verifyFields(this,1)" onclick="verifyFields(this,1)" name="f_wl'+u+'_wepidx" id="_f_wl'+u+'_wepidx_'+i+'" value="'+i+'"'+((eval('nvram.wl'+u+'_key') == i) ? ' checked="checked">' : '>'),
						value: nvram['wl'+u+'_key'+i] });
			}

			f.push(null, { title: 'WDS', name: 'f_wl'+u+'_lazywds', type: 'select', options: [['0','Link With...'],['1','Automatic']], value: nvram['wl'+u+'_lazywds'] } );

			var wds = eval('nvram.wl'+u+'_wds').split(/\s+/);
			for (i = 0; i < 10; i += 2) {
				f.push(
					{ title: (i ? '' : 'MAC Address'), indent: 2, multi: [
						{ name: 'f_wl'+u+'_wds_'+i, type: 'text', maxlen: 17, size: 20, value: wds[i] || mac_null },
						{ name: 'f_wl'+u+'_wds_'+(i + 1), type: 'text', maxlen: 17, size: 20, value: wds[i + 1] || mac_null } ] } );
			}
			createFieldTable('', f);
			W('<\/div>');
		}
	}
/* for each wlif */
</script>

<!-- / / / -->

<div id="footer">
	<span id="footer-msg"></span>
	<input type="button" value="Save" id="save-button" onclick="save()">
	<input type="button" value="Cancel" id="cancel-button" onclick="reloadPage();">
</div>

</td></tr>
</table>
</form>
<script>earlyInit()</script>
</body>
</html>
