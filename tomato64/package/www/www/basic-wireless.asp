<!DOCTYPE html>
<!--
	Wireless Web GUI
	Copyright (C) 2024 Lance Fredrickson
	lancethepants@gmail.com
-->

<html lang="en-GB">
<head>
<meta http-equiv="content-type" content="text/html;charset=utf-8">
<meta name="robots" content="noindex,nofollow">
<title>[<% ident(); %>] Wifi</title>
<link rel="stylesheet" type="text/css" href="tomato.css?rel=<% version(); %>">
<% css(); %>
<script src="isup.jsx?_http_id=<% nv(http_id); %>"></script>
<script src="tomato.js?rel=<% version(); %>"></script>
<script src="wireless.js?rel=<% version(); %>"></script>

<script>

//	<% nvram("wifi_sta_list,wan_sta,wifi,lan_ifname"); %>
//	<% wireless(); %>

var cprefix = 'basic_wireless';

var devices = [];
var interfaces = [];
var mode_loaded = [];
var width_loaded = [];
var channel_loaded = [];
var power_loaded = [];
var country_loaded = [];
var macTables = [];

var phy_count = (typeof wireless !== 'undefined' && typeof wireless.phy_count !== 'undefined') ? wireless.phy_count : 0;

for (let i = 0; i < phy_count; i++) {
	devices.push(['phy'+i, 'phy'+i]);
	mode_loaded.push(0);
	width_loaded.push(0);
	channel_loaded.push(0);
	power_loaded.push(0);
	country_loaded.push(0);
	macTables.push([]);
}

var hasWirelessDevices = devices.length > 0;

var interfaceCount = [];

for (let i = 0; i < devices.length; i++) {
	interfaceCount.push([eval('nvram.wifi_phy'+i+'_ifaces')]);
}

var deviceSections = [
	['general', 'General Setup'],
	['advanced', 'Advanced Settings']
];

var interfaceSections = [
	['general', 'General Setup'],
	['security', 'Wireless Security'],
	['filter', 'MAC-Filter'],
	['advanced', 'Advanced Settings'],
	['roaming', 'WLAN Roaming']
];

var security_modes = [
	['psk2', 'WPA2-PSK (strong security)'],
	['sae', 'WPA3-SAE (strong security)'],
	['sae-mixed', 'WPA2/WPA3-SAE Mixed Mode (strong security)'],
	['psk-mixed', 'WPA-PSK/WPA2-PSK Mixed Mode (medium security)'],
	['psk', 'WPA-PSK (weak security)'],
	['owe', 'OWE (open network)'],
	['none', 'No Encryption (open network)']
];

var ciphers = [
	['auto', 'auto'],
	['ccmp', 'Force CCMP (AES)'],
	['ccmp256', 'Force CCMP-256 (AES)'],
	['gcmp', 'Force GCMP (AES)'],
	['gcmp256', 'Force GCMP-256 (AES)'],
	['tkip', 'Force TKIP'],
	['tkip+ccmp', 'Force TKIP and CCMP (AES)']
];

var networks = [
	['br0', 'LAN0 (br0)'],
	['br1', 'LAN1 (br1)'],
	['br2', 'LAN2 (br2)'],
	['br3', 'LAN3 (br3)'],
	['br4', 'LAN4 (br4)'],
	['br5', 'LAN5 (br5)'],
	['br6', 'LAN6 (br6)'],
	['br7', 'LAN7 (br7)'],
	['', 'none']
];

const regionNames = new Intl.DisplayNames(['en'], {
	type: 'region'
});

var cmdresult = '';
var cmd = null;
var cmdresult = '';
var changed = 0;

function show() {}

function displayCountry(device) {
	var country = '';
	var countrylist = [];
	var result = cmdresult.split('\n');

	countrylist.push(['', 'driver default']);

	for (var i = 0; i < result.length; i++) {
		if (result[i] !== "") {
			var result2 = result[i].split(/\s+/);
			if ((result2[0] == '*') || (result2[0] == ''))
				result2.shift();

			if (result2[0] !== '00')
				countrylist.push([result2[0], result2[0] + ' - ' + regionNames.of(result2[0])]);
		}
	}

	t = devices[device][0];
	e = E('_wifi_'+t+'_country');
	buf = '';
	val = (!country_loaded[device]) ? eval('nvram["wifi_'+t+'_country"]') : e.value;

	for (i = 0; i < countrylist.length; ++i)
		buf += '<option value="' + countrylist[i][0] + '"' + ((countrylist[i][0] == val) ? ' selected="selected"' : '') + '>' + countrylist[i][1] + '<\/option>';

	e = E('__wifi_'+t+'_country');
	buf = '<select name="wifi_'+t+'_country" onchange="verifyFields(this, 1)" id = "_wifi_'+t+'_country">' + buf + '<\/select>';
	elem.setInnerHTML(e, buf);
	country_loaded[device] = 1;
	cmdresult = '';
}

function refreshCountry(device) {
	var cmd = new XmlHttp();
	cmd.onCompleted = function(text, xml) {
		eval(text);
		displayCountry(device);
	}
	cmd.onError = function(x) {
		cmdresult = 'ERROR: ' + x;
		displayCountry(device);
	}
	var c = 'iwinfo phy'+device+' countrylist';
	cmd.post('shell.cgi', 'action=execute&command=' + escapeCGI(c.replace(/\r/g, '')));
}

function displayPower(device) {
	var powerDisplay = '';
	var powerlist = [];
	var result = cmdresult.split('\n');

	powerlist.push(['', 'driver default']);

	for (var i = 0; i < result.length; i++) {
		if (result[i] !== "") {
			var result2 = result[i].split(/\s+/);
			if ((result2[0] == '*') || (result2[0] == ''))
				result2.shift();

			if (result2.length >= 5) {
				powerDisplay = result2[0] + ' ' + result2[1] + ' ' + result2[2] + result2[3] + ' ' + result2[4];
			} else {
				powerDisplay = result2[0] + ' ' + result2[1] + ' ' + result2[2] + ' ' + result2[3];
			}
			powerlist.push([result2[0], powerDisplay]);
		}
	}

	t = devices[device][0];
	e = E('_wifi_'+t+'_power');
	buf = '';
	val = (!power_loaded[device]) ? eval('nvram["wifi_'+t+'_power"]') : e.value;

	for (i = 0; i < powerlist.length; ++i)
		buf += '<option value="' + powerlist[i][0] + '"' + ((powerlist[i][0] == val) ? ' selected="selected"' : '') + '>' + powerlist[i][1] + '<\/option>';

	e = E('__wifi_'+t+'_power');
	buf = '<select name="wifi_'+t+'_power" onchange="verifyFields(this, 1)" id = "_wifi_'+t+'_power">' + buf + '<\/select>';
	elem.setInnerHTML(e, buf);
	power_loaded[device] = 1;
	cmdresult = '';
}

function refreshPower(device) {
	var cmd = new XmlHttp();
	cmd.onCompleted = function(text, xml) {
		eval(text);
		displayPower(device);
	}
	cmd.onError = function(x) {
		cmdresult = 'ERROR: ' + x;
		displayPower(device);
	}
	var c = 'iwinfo phy'+device+' txpowerlist';
	cmd.post('shell.cgi', 'action=execute&command=' + escapeCGI(c.replace(/\r/g, '')));
}

function displayChannels(device) {
	var channel = '';
	var channels = [];
	var result = cmdresult.split('\n');
	var selectedBand = E('_wifi_'+devices[device][0]+'_band').value;

	channels.push(['auto', 'auto']);
	for (var i = 0; i < result.length; i++) {
		if (result[i] !== "") {
			var result2 = result[i].split(/\s+/);
			if ((result2[0] == '*') || (result2[0] == ''))
				result2.shift();

			/* filter channels to only show those matching the selected band */
			var freq = parseFloat(result2[0]);
			var channelBand;
			if (freq < 3)
				channelBand = '2g';
			else if (freq < 5.9)
				channelBand = '5g';
			else
				channelBand = '6g';

			if (channelBand !== selectedBand)
				continue;

			channel = result2[6].substring(0, result2[6].length - 1);
			channels.push([channel, channel + ' (' + result2[0] + ' ' + result2[4].substring(0, result2[4].length - 1) + ')']);
		}
	}

	t = devices[device][0];
	e = E('_wifi_'+t+'_channel');
	buf = '';
	val = (!channel_loaded[device] || (e.value + '' == '')) ? eval('nvram["wifi_'+t+'_channel"]') : e.value;

	for (i = 0; i < channels.length; ++i)
		buf += '<option value="' + channels[i][0] + '"' + ((channels[i][0] == val) ? ' selected="selected"' : '') + '>' + channels[i][1] + '<\/option>';

	e = E('__wifi_'+t+'_channel');
	buf = '<select name="wifi_'+t+'_channel" onchange="verifyFields(this, 1)" id = "_wifi_'+t+'_channel">' + buf + '<\/select>';
	elem.setInnerHTML(e, buf);
	channel_loaded[device] = 1;
	cmdresult = '';
}

function refreshChannels(device) {
	var cmd = new XmlHttp();
	cmd.onCompleted = function(text, xml) {
		eval(text);
		displayChannels(device);
	}
	cmd.onError = function(x) {
		cmdresult = 'ERROR: ' + x;
		displayChannels(device);
	}
	var c = 'iwinfo phy'+device+' freqlist';
	cmd.post('shell.cgi', 'action=execute&command=' + escapeCGI(c.replace(/\r/g, '')));
}

function refreshModes(device) {
	var m = [];
	t = devices[device][0];

	if (device >= devices.length)
		return;

	var bandselect = document.getElementById('_wifi_'+t+'_band');
	var selected = bandselect.value;

	//htmodes=[['legacy','Legacy'],['n','N'],['ac','AC'],['ax','AX']];
	if (selected == '2g') {
		m.push(['legacy', 'Legacy']);
		if (eval('wireless.'+t+'_2G_ht') == 1) m.push(['n', 'N']);
		if (eval('wireless.'+t+'_2G_he') == 1) m.push(['ax', 'AX']);
	}
	if (selected == '5g') {
		if (eval('wireless.'+t+'_5G_ht') == 1) m.push(['n', 'N']);
		if (eval('wireless.'+t+'_5G_vht') == 1) m.push(['ac', 'AC']);
		if (eval('wireless.'+t+'_5G_he') == 1) m.push(['ax', 'AX']);
		if (eval('wireless.'+t+'_5G_eht') == 1) m.push(['be', 'BE']);
	}
	if (selected == '6g') {
		if (eval('wireless.'+t+'_6G_ht') == 1) m.push(['n', 'N']);
		if (eval('wireless.'+t+'_6G_vht') == 1) m.push(['ac', 'AC']);
		if (eval('wireless.'+t+'_6G_he') == 1) m.push(['ax', 'AX']);
		if (eval('wireless.'+t+'_6G_eht') == 1) m.push(['be', 'BE']);
	}

	e = E('_wifi_'+t+'_mode');
	buf = '';
	val = (!mode_loaded[device] || (e.value + '' == '')) ? eval('nvram["wifi_'+t+'_mode"]') : e.value;

	for (i = 0; i < m.length; ++i)
		buf += '<option value="' + m[i][0] + '"' + ((m[i][0] == val) ? ' selected="selected"' : '') + '>' + m[i][1] + '<\/option>';

	e = E('__wifi_'+t+'_mode');
	buf = '<select name="wifi_'+t+'_mode" onchange="verifyFields(this, 1)" id = "_wifi_'+t+'_mode">' + buf + '<\/select>';
	elem.setInnerHTML(e, buf);
	mode_loaded[device] = 1;
}

function refreshWidths(device) {
	var w = [];
	var bandselect = document.getElementById('_wifi_'+t+'_band');
	var band = bandselect.value;
	var widthselect = document.getElementById('_wifi_'+t+'_mode');
	var mode = widthselect.value;
	t = devices[device][0]+'_'+band.toUpperCase();

	if (mode == 'legacy') {}
	if (mode == 'n') {
		if (eval('wireless.'+t+'_HT20') == 1) w.push(['20', '20 MHz']);
		if (eval('wireless.'+t+'_HT40') == 1) w.push(['40', '40 MHz']);
	}
	if (mode == 'ac') {
		if (eval('wireless.'+t+'_VHT20') == 1) w.push(['20', '20 MHz']);
		if (eval('wireless.'+t+'_VHT40') == 1) w.push(['40', '40 MHz']);
		if (eval('wireless.'+t+'_VHT80') == 1) w.push(['80', '80 MHz']);
		if (eval('wireless.'+t+'_VHT160') == 1) w.push(['160', '160 MHz']);
	}
	if (mode == 'ax') {
		if (eval('wireless.'+t+'_HE20') == 1) w.push(['20', '20 MHz']);
		if (eval('wireless.'+t+'_HE40') == 1) w.push(['40', '40 MHz']);
		if (eval('wireless.'+t+'_HE80') == 1) w.push(['80', '80 MHz']);
		if (eval('wireless.'+t+'_HE160') == 1) w.push(['160', '160 MHz']);
	}
	if (mode == 'be') {
		if (eval('wireless.'+t+'_EHT20') == 1) w.push(['20', '20 MHz']);
		if (eval('wireless.'+t+'_EHT40') == 1) w.push(['40', '40 MHz']);
		if (eval('wireless.'+t+'_EHT80') == 1) w.push(['80', '80 MHz']);
		if (eval('wireless.'+t+'_EHT160') == 1) w.push(['160', '160 MHz']);
		if (eval('wireless.'+t+'_EHT320') == 1) w.push(['320', '320 MHz']);
	}

	u = devices[device][0];
	e = E('_wifi_'+u+'_width');
	buf = '';
	val = (!width_loaded[device] || (e.value + '' == '')) ? eval('nvram["wifi_'+u+'_width"]') : e.value;

	for (i = 0; i < w.length; ++i)
		buf += '<option value="' + w[i][0] + '"' + ((w[i][0] == val) ? ' selected="selected"' : '') + '>' + w[i][1] + '<\/option>';

	e = E('__wifi_'+u+'_width');
	buf = '<select name="wifi_'+u+'_width" onchange="verifyFields(this, 1)" id = "_wifi_'+u+'_width">' + buf + '<\/select>';
	elem.setInnerHTML(e, buf);
	width_loaded[device] = 1;
}

function verifyFields(focused, quiet) {
	var b;

	if (!hasWirelessDevices) {
		return 1;
	}

	/* --- visibility --- */

	for (var i = 0; i < devices.length; ++i) {
		refreshModes(i);
		refreshWidths(i);
		refreshChannels(i);
		refreshPower(i);
		refreshCountry(i);
		t = devices[i][0];

		// hide legacy b rates when not a 2G device
		b = E('_f_wifi_'+t+'_brates');
		if ((typeof eval('wireless.phy'+i+'_2G') === 'undefined') || E('_wifi_'+t+'_band').value != '2g') {
			b.disabled = 1;
			PR(b).style.display = 'none';
		} else {
			b.disabled = 0;
			PR(b).style.display = '';
		}

		// hide width when enabling legacy mode for 2G devices
		b = E('_wifi_'+t+'_width');
		if ((E('_wifi_'+t+'_band').value == '2g') && (E('_wifi_'+t+'_mode').value == 'legacy')) {
			b.disabled = 1;
			PR(b).style.display = 'none';
		} else {
			b.disabled = 0;
			PR(b).style.display = '';
		}

		for (var j = 0; j < interfaceCount[i]; ++j) {
			t = 'phy'+i+'iface'+j;

			var iface_mode = E('_wifi_'+t+'_mode').value;

			// ESSID row: relabel to "Mesh ID" in mesh mode, restore in other modes
			var essid_el = E('_wifi_'+t+'_essid');
			var essid_row = PR(essid_el);
			if (essid_row) {
				var title_cell = essid_row.querySelector('td.title') || essid_row.cells[0];
				if (title_cell) title_cell.textContent = (iface_mode == 'mesh') ? 'Mesh ID' : 'ESSID';
			}

			// General tab: show/hide fields per mode
			b = E('_wifi_'+t+'_bssid');
			c = E('_wifi_'+t+'_network');
			d = E('_f_wifi_'+t+'_hidden');
			e = E('_f_wifi_'+t+'_wmm');
			f = E('_f_wifi_'+t+'_uapsd');

			// BSSID: sta and bridge only
			var show_bssid = (iface_mode == 'sta' || iface_mode == 'bridge');
			b.disabled = show_bssid ? 0 : 1;
			PR(b).style.display = show_bssid ? '' : 'none';

			// Network/bridge: ap, bridge, mesh (not sta — sta uses routing)
			var show_network = (iface_mode == 'ap' || iface_mode == 'bridge' || iface_mode == 'mesh');
			c.disabled = show_network ? 0 : 1;
			PR(c).style.display = show_network ? '' : 'none';

			// Hide ESSID, WMM, U-APSD: AP only
			var ap_only = (iface_mode == 'ap');
			d.disabled = ap_only ? 0 : 1;
			PR(d).style.display = ap_only ? '' : 'none';
			e.disabled = ap_only ? 0 : 1;
			PR(e).style.display = ap_only ? '' : 'none';

			// U-APSD: visible only when AP mode and WMM is checked
			if (ap_only && e.checked) {
				f.disabled = 0;
				PR(f).style.display = '';
			} else {
				f.disabled = 1;
				PR(f).style.display = 'none';
			}

			// Advanced tab: isolate and br_isolate are AP-only; mesh fields are mesh-only
			var adv_isolate = E('_f_wifi_'+t+'_isolate');
			var adv_br_isolate = E('_f_wifi_'+t+'_br_isolate');
			var adv_mesh_fwding = E('_f_wifi_'+t+'_mesh_fwding');
			var adv_mesh_rssi = E('_wifi_'+t+'_mesh_rssi_threshold');

			adv_isolate.disabled = ap_only ? 0 : 1;
			PR(adv_isolate).style.display = ap_only ? '' : 'none';
			adv_br_isolate.disabled = ap_only ? 0 : 1;
			PR(adv_br_isolate).style.display = ap_only ? '' : 'none';

			var is_mesh = (iface_mode == 'mesh');
			adv_mesh_fwding.disabled = is_mesh ? 0 : 1;
			PR(adv_mesh_fwding).style.display = is_mesh ? '' : 'none';
			adv_mesh_rssi.disabled = is_mesh ? 0 : 1;
			PR(adv_mesh_rssi).style.display = is_mesh ? '' : 'none';

			// === Roaming tab (802.11r / 11k / 11v): AP-only, encryption-gated ===
			var enc_val = E('_wifi_'+t+'_encryption').value;
			var r_enc_allowed = (enc_val == 'wpa2' || enc_val == 'wpa3' || enc_val == 'wpa3-mixed' ||
			                     enc_val == 'wpa3-192' || enc_val == 'psk2' || enc_val == 'psk-mixed' ||
			                     enc_val == 'sae' || enc_val == 'sae-mixed');
			var r_psk = (enc_val == 'psk2' || enc_val == 'psk-mixed');
			var r_enabled = E('_f_wifi_'+t+'_ieee80211r').checked;
			var k_enabled = E('_f_wifi_'+t+'_ieee80211k').checked;

			function _roamShow(id, show) {
				var el = E(id);
				if (!el) return;
				el.disabled = show ? 0 : 1;
				PR(el).style.display = show ? '' : 'none';
			}
			// 11r flag itself
			_roamShow('_f_wifi_'+t+'_ieee80211r', ap_only && r_enc_allowed);
			var r_show = ap_only && r_enc_allowed && r_enabled;
			_roamShow('_wifi_'+t+'_nasid', r_show);
			_roamShow('_wifi_'+t+'_mobility_domain', r_show);
			_roamShow('_wifi_'+t+'_reassociation_deadline', r_show);
			_roamShow('_wifi_'+t+'_ft_over_ds', r_show);
			_roamShow('_f_wifi_'+t+'_ft_psk_generate_local', r_show && r_psk);
			_roamShow('_wifi_'+t+'_r0_key_lifetime', r_show);
			_roamShow('_wifi_'+t+'_r1_key_holder', r_show);
			_roamShow('_f_wifi_'+t+'_pmk_r1_push', r_show);
			_roamShow('_wifi_'+t+'_r0kh', r_show);
			_roamShow('_wifi_'+t+'_r1kh', r_show);
			// 11k
			_roamShow('_f_wifi_'+t+'_ieee80211k', ap_only);
			_roamShow('_f_wifi_'+t+'_rrm_neighbor_report', ap_only && k_enabled);
			_roamShow('_f_wifi_'+t+'_rrm_beacon_report', ap_only && k_enabled);
			// 11v
			_roamShow('_wifi_'+t+'_time_advertisement', ap_only);
			var tadv = E('_wifi_'+t+'_time_advertisement').value;
			_roamShow('_wifi_'+t+'_time_zone', ap_only && tadv == '2');
			_roamShow('_f_wifi_'+t+'_wnm_sleep_mode', ap_only);
			_roamShow('_f_wifi_'+t+'_wnm_sleep_mode_no_keys', ap_only);
			_roamShow('_f_wifi_'+t+'_bss_transition', ap_only);
			_roamShow('_f_wifi_'+t+'_proxy_arp', ap_only);

			// Rebuild encryption dropdown: mesh restricts to SAE or open only
			var enc_sel = E('_wifi_'+t+'_encryption');
			var current_enc = enc_sel.value;
			var enc_options = is_mesh
				? [['sae','WPA3-SAE (recommended)'],['none','No Encryption (open)']]
				: security_modes;
			var enc_buf = '';
			for (var m = 0; m < enc_options.length; ++m) {
				enc_buf += '<option value="' + enc_options[m][0] + '"' +
					((enc_options[m][0] == current_enc) ? ' selected' : '') +
					'>' + enc_options[m][1] + '<\/option>';
			}
			enc_sel.innerHTML = enc_buf;
			if (is_mesh && current_enc != 'sae' && current_enc != 'none') {
				enc_sel.value = 'none';
			}


			// show the cipher option only for relevant modes
			b = E('_wifi_'+t+'_cipher');
			if ((E('_wifi_'+t+'_encryption').value == 'psk2') || (E('_wifi_'+t+'_encryption').value == 'psk-mixed') || (E('_wifi_'+t+'_encryption').value == 'psk')) {
				b.disabled = 0;
				PR(b).style.display = '';
			} else {
				b.disabled = 1;
				PR(b).style.display = 'none';
			}

			// show the key (password) field option only for relevant modes
			b = E('_wifi_'+t+'_key');
			c = E('_f_wifi_'+t+'_psk_random1');

			if ((E('_wifi_'+t+'_encryption').value == 'psk2') || (E('_wifi_'+t+'_encryption').value == 'sae') || (E('_wifi_'+t+'_encryption').value == 'sae-mixed') ||
			    (E('_wifi_'+t+'_encryption').value == 'psk-mixed') || (E('_wifi_'+t+'_encryption').value == 'psk')) {
				b.disabled = 0;
				PR(b).style.display = '';
				c.disabled = 0;
				PR(c).style.display = '';
			} else {
				b.disabled = 1;
				PR(b).style.display = 'none';
				c.disabled = 1;
				PR(c).style.display = 'none';
			}

			// show mac filter table for relevant modes
			b = E('table_'+t+'_maclist');

			if ((E('_wifi_'+t+'_macfilter').value == 'allow') || (E('_wifi_'+t+'_macfilter').value == 'deny')) {
				b.disabled = 0;
				PR(b).style.display = '';
			} else {
				b.disabled = 1;
				PR(b).style.display = 'none';
			}

			for (var k = 0; k <= MAX_BRIDGE_ID; ++k) {
				n = (k == 0 ? '' : k);
				if (eval('nvram.lan'+n+'_ifname.length') < 1) {
					E('_wifi_'+t+'_network').options[k].disabled = 1;
				}
			}
		}
	}

	/* --- verify --- */

	var ok = 1
	for (var i = 0; i < devices.length; ++i) {
		for (var j = 0; j < interfaceCount[i]; ++j) {
			t = 'phy'+i+'iface'+j;

			if (E('_f_wifi_'+t+'_enable').checked) {

				if (E('_wifi_'+t+'_essid').value.length == 0 ) {
					ferror.set(E('_wifi_'+t+'_essid'), 'ESSID cannot be empty', quiet || !ok);
					ok = 0;
				} else {
					ferror.clear(E('_wifi_'+t+'_essid'));
				}

				if ((E('_wifi_'+t+'_bssid').value.length > 0 ) && (E('_wifi_'+t+'_mode').value == "sta" ) && (!v_mac(E('_wifi_'+t+'_bssid'),quiet))) {
					ok = 0;
				} else {
					ferror.clear(E('_wifi_'+t+'_bssid'));
				}

				if (((E('_wifi_'+t+'_encryption').value == 'psk2') || (E('_wifi_'+t+'_encryption').value == 'sae') || (E('_wifi_'+t+'_encryption').value == 'sae-mixed') ||
				     (E('_wifi_'+t+'_encryption').value == 'psk-mixed') || (E('_wifi_'+t+'_encryption').value == 'psk')) && (E('_wifi_'+t+'_key').value.length < 8)) {
					ferror.set(E('_wifi_'+t+'_key'), 'Key must be at least 8 characters', quiet || !ok);
					ok = 0;
				} else {
					ferror.clear(E('_wifi_'+t+'_key'));
				}
			} else {
				ferror.clear(E('_wifi_'+t+'_essid'));
				ferror.clear(E('_wifi_'+t+'_bssid'));
				ferror.clear(E('_wifi_'+t+'_key'));
			}
		}
	}
	changed = 1;
	return ok;
}

function tabSelect(name) {
	if (!hasWirelessDevices) {
		return;
	}
	tabHigh(name);
	for (var i = 0; i < devices.length; ++i) {
		elem.display(devices[i][0]+'-tab', (name == devices[i][0]));
		elem.display('phy'+i+'-interface-tab', (name == devices[i][0]));
	}
	cookie.set('wireless_device', name);
}

function deviceSectSelect(tab, deviceSection) {
	for (var i = 0; i < deviceSections.length; ++i) {
		if (deviceSection == deviceSections[i][0]) {
			elem.addClass(devices[tab][0]+'-'+deviceSections[i][0]+'-tab', 'active');
			elem.display(devices[tab][0]+'-'+deviceSections[i][0], true);
		} else {
			elem.removeClass(devices[tab][0]+'-'+deviceSections[i][0]+'-tab', 'active');
			elem.display(devices[tab][0]+'-'+deviceSections[i][0], false);
		}
	}
	cookie.set('wireless'+tab+'_deviceSection', deviceSection);
}

function interfaceSelect(tab, my_interface) {
	for (var i = 0; i < interfaces.length; ++i) {
		if (my_interface == interfaces[i][0]) {
			elem.addClass(my_interface+'-tab', 'active');
			elem.display(my_interface, true);
		} else {
			if (interfaces[i][0].charAt(3) == tab) {
				elem.removeClass(interfaces[i][0]+'-tab', 'active');
				elem.display(interfaces[i][0], false);
			}
		}
	}
	cookie.set('wireless'+tab+'_interface', my_interface);
}

function interfaceSectSelect(device, interface_num, category) {
	for (var i = 0; i < devices.length; ++i) {
		for (var j = 0; j < interfaceCount[i]; ++j) {
			for (var k = 0; k < interfaceSections.length; ++k) {
				if ((i == device) && (j == interface_num)) {
					if (interfaceSections[k][0] == interfaceSections[category][0]) {
						elem.addClass('phy'+device+'iface'+interface_num+'-'+interfaceSections[category][0]+'-tab', 'active');
						elem.display('phy'+device+'iface'+interface_num+'-'+interfaceSections[category][0], true);
					} else {
						elem.removeClass('phy'+device+'iface'+interface_num+'-'+interfaceSections[k][0]+'-tab', 'active');
						elem.display('phy'+device+'iface'+interface_num+'-'+interfaceSections[k][0], false);
					}
				}
			}
		}
	}
	cookie.set('wireless'+device+'_interface'+interface_num, category);
}

function addInterface(device) {
	if (changed == 0 || (changed == 1 && confirm("Any unsaved changes will be lost, continue?"))) {
		var fom = E('t_iface');
		for (var i = 0; i < devices.length; i++) {
			if (i == device) {
				fom['wifi_phy'+i+'_ifaces'].value = +eval('nvram["wifi_phy"+i+"_ifaces"]') + +1;
				cookie.set('wireless'+device+'_interface', 'phy'+device+'iface'+(Number(nvram["wifi_phy"+i+"_ifaces"])));
			} else {
				fom['wifi_phy'+i+'_ifaces'].value = eval('nvram["wifi_phy"+i+"_ifaces"]');
			}
		}
		form.submit(fom, 0);
	}
}

function removeInterface(device) {
	if (changed == 0 || (changed == 1 && confirm("Any unsaved changes will be lost, continue?"))) {
		var fom = E('t_iface');
		for (var i = 0; i < devices.length; i++) {
			if (i == device) {
				fom['wifi_phy'+i+'_ifaces'].value = +eval('nvram["wifi_phy"+i+"_ifaces"]') - +1;
				cookie.set('wireless'+device+'_interface', 'phy'+device+'iface'+(Number(nvram["wifi_phy"+i+"_ifaces"]) - 2));
			} else {
				fom['wifi_phy'+i+'_ifaces'].value = eval('nvram["wifi_phy"+i+"_ifaces"]');
			}
		}
		form.submit(fom, 0);
	}
}

function save_pre() {
	if (!verifyFields(null, 0))
		return 0;
	return 1;
}

function save(nomsg) {
	if (!hasWirelessDevices) return;
	if (!save_pre()) return;
	if (!nomsg) show(); /* update '_service' field first */
	var fom = E('t_fom');
	var sta_list = "";
	for (var i = 0; i < devices.length; i++) {
		t = devices[i][0];
		E('wifi_'+t+'_brates').value = E('_f_wifi_'+t+'_brates').checked ? 1 : "";
		E('wifi_'+t+'_noscan').value = E('_f_wifi_'+t+'_noscan').checked ? 1 : "";
		for (var j = 0; j < interfaceCount[i]; j++) {
			t = 'phy'+i+'iface'+j;
			E('wifi_'+t+'_enable').value = E('_f_wifi_'+t+'_enable').checked ? 1 : 0;
			E('wifi_'+t+'_hidden').value = E('_f_wifi_'+t+'_hidden').checked ? 1 : "";
			E('wifi_'+t+'_wmm').value = E('_f_wifi_'+t+'_wmm').checked ? 1 : 0;
			E('wifi_'+t+'_uapsd').value = E('_f_wifi_'+t+'_uapsd').checked ? 1 : 0;
			E('wifi_'+t+'_isolate').value = E('_f_wifi_'+t+'_isolate').checked ? 1 : "";
			E('wifi_'+t+'_br_isolate').value = E('_f_wifi_'+t+'_br_isolate').checked ? 1 : "";
			E('wifi_'+t+'_mesh_fwding').value = E('_f_wifi_'+t+'_mesh_fwding').checked ? 1 : 0;
			E('wifi_'+t+'_ieee80211r').value = E('_f_wifi_'+t+'_ieee80211r').checked ? 1 : 0;
			E('wifi_'+t+'_ft_psk_generate_local').value = E('_f_wifi_'+t+'_ft_psk_generate_local').checked ? 1 : 0;
			E('wifi_'+t+'_pmk_r1_push').value = E('_f_wifi_'+t+'_pmk_r1_push').checked ? 1 : 0;
			E('wifi_'+t+'_ieee80211k').value = E('_f_wifi_'+t+'_ieee80211k').checked ? 1 : 0;
			E('wifi_'+t+'_rrm_neighbor_report').value = E('_f_wifi_'+t+'_rrm_neighbor_report').checked ? 1 : 0;
			E('wifi_'+t+'_rrm_beacon_report').value = E('_f_wifi_'+t+'_rrm_beacon_report').checked ? 1 : 0;
			E('wifi_'+t+'_wnm_sleep_mode').value = E('_f_wifi_'+t+'_wnm_sleep_mode').checked ? 1 : 0;
			E('wifi_'+t+'_wnm_sleep_mode_no_keys').value = E('_f_wifi_'+t+'_wnm_sleep_mode_no_keys').checked ? 1 : 0;
			E('wifi_'+t+'_bss_transition').value = E('_f_wifi_'+t+'_bss_transition').checked ? 1 : 0;
			E('wifi_'+t+'_proxy_arp').value = E('_f_wifi_'+t+'_proxy_arp').checked ? 1 : 0;

			/* r0kh/r1kh: convert newline-separated textarea back to '>'-separated */
			var r0kh_el = E('_wifi_'+t+'_r0kh');
			if (r0kh_el) r0kh_el.value = r0kh_el.value.split(/\r?\n/).filter(function(s){return s.length>0;}).join('>');
			var r1kh_el = E('_wifi_'+t+'_r1kh');
			if (r1kh_el) r1kh_el.value = r1kh_el.value.split(/\r?\n/).filter(function(s){return s.length>0;}).join('>');

			var macdata = macTables[i][j].getAllData();
			var macs = '';
			for (k = 0; k < macdata.length; k++) {
				macs += macdata[k].join('<')+'>';
			}
			E('wifi_'+t+'_maclist').value = macs;

			if (((E('_wifi_'+t+'_mode').value == "sta") || (E('_wifi_'+t+'_mode').value == "bridge")) && (E('_f_wifi_'+t+'_enable').checked)) {
				if (E('_wifi_'+t+'_ifname').value != "") {
					sta_list += ' ' + E('_wifi_'+t+'_ifname').value;
				} else {
					sta_list += ' phy'+i+'-sta'+j;
				}
				E('wifi_sta_list').value = sta_list.trim();
			}
		}
	}

	// We don't have a gui element for these so set the hidden element to it's nvram value
	// If we renamed/destroyed a station that was used by a wan we will modify its value below
	E('wan_sta').value  = nvram['wan_sta'];
	E('wan2_sta').value = nvram['wan2_sta'];
	E('wan3_sta').value = nvram['wan3_sta'];
	E('wan4_sta').value = nvram['wan4_sta'];

	if (E('wifi_sta_list').value.length > 0) {

		// check if we've renamed/destroyed a station that a wan was using
		// if so, set wanX_sta = ""
		const wan_list = ["wan_sta", "wan2_sta", "wan3_sta", "wan4_sta"];
		const station_list = E('wifi_sta_list').value.split(" ");

		for (var i = 0; i < wan_list.length; i++) {
			if (nvram[wan_list[i]].length > 0) {
				for (var j = 0; j < station_list.length; j++) {
					if (nvram[wan_list[i]] == station_list[j]) {
						break;
					}
					if (j == (station_list.length -1)) {
						E(wan_list[i]).value = "";
					}
				}
			}
		}

		fom._service.value = '*';
	} else {
		fom._service.value = 'wifi-reload';
	}
	form.submit(fom, 1);
	changed = 0;
}

function earlyInit() {
	if (!hasWirelessDevices) {
		return;
	}
	tabSelect(cookie.get('wireless_device') || devices[0][0]);
	for (var i = 0; i < devices.length; ++i) {
		deviceSectSelect(i, cookie.get('wireless'+i+'_deviceSection') || deviceSections[0][0]);
		interfaceSelect(i, cookie.get('wireless'+i+'_interface') || 'phy'+i+'iface0');
		for (var j = 0; j < interfaceCount[i]; ++j) {

			macTables[i].push(new MacGrid());

			macTables[i][j].init('table_phy'+i+'iface'+j+'_maclist','sort',0,[
				{ type: 'text', maxlen: 17 },
				{ type: 'text', maxlen: 48 }]);

			macTables[i][j].headerSet(['MAC Address','Description']);

			var nv = nvram['wifi_phy'+i+'iface'+j+'_maclist'].split('>');
			for (var m = 0; m < nv.length; m++) {
				var t = nv[m].split('<');
					if (t.length == 2) {
					macTables[i][j].insertData(-1, t);
				}
			}

			macTables[i][j].showNewEditor();
			macTables[i][j].resetNewEditor();

			for (var k = 0; k < interfaceSections.length; ++k) {
				interfaceSectSelect(i, j, cookie.get('wireless'+i+'_interface'+j, k) || 0);
			}
		}
	}
	//	show();
	verifyFields(null, 1);
	changed = 0;
	cookie.set('addmac', '', 0);
}

function init() {
        if (((c = cookie.get(cprefix + '_notes_vis')) != null) && (c == '1'))
                toggleVisibility(cprefix, "notes");

	up.initPage(250, 5);
}

function MacGrid() {return this;}
MacGrid.prototype = new TomatoGrid;

MacGrid.prototype.fieldValuesToData = function(row) {
	var f = fields.getAll(row);
	var r = [f[0].value, f[1].value];
	return r;
}

MacGrid.prototype.verifyFields = function(row, quiet) {
	var f;
	f = fields.getAll(row);

	return v_mac(f[0], quiet) && v_nodelim(f[1], quiet, 'Description', 1);
}

MacGrid.prototype.resetNewEditor = function() {
	var f, c, n;

	f = fields.getAll(this.newEditor);
	ferror.clearAll(f);

	if ((c = cookie.get('addmac')) != null) {
		c = c.split(',');
		if (c.length == 2) {
			f[0].value = c[0];
			f[1].value = c[1];
			return;
		}
	}

	f[0].value = mac_null;
	f[1].value = '';
}

</script>
</head>

<body onload="init()">
<form id="t_iface" method="post" action="tomato.cgi">
<input type="hidden" name="_nextpage" value="basic-wireless.asp">
<input type="hidden" name="_nextwait" value="1">
<script>
for (i = 0; i < devices.length; ++i) {
	W('<input type="hidden" id="wifi_phy'+i+'_ifaces" name="wifi_phy'+i+'_ifaces">');
}
</script>
</form>

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

<input type="hidden" name="_nextpage" value="basic-wireless.asp">
<input type="hidden" name="_service" value="">
<input type="hidden" name="_nofootermsg">
<input type="hidden" name="wifi_sta_list" id="wifi_sta_list">
<input type="hidden" name="wan_sta" id="wan_sta">
<input type="hidden" name="wan2_sta" id="wan2_sta">
<input type="hidden" name="wan3_sta" id="wan3_sta">
<input type="hidden" name="wan4_sta" id="wan4_sta">

<!-- / / / -->

<script>

if (!hasWirelessDevices) {
	W('<div class="fields"><b>No compatible Wifi hardware detected.</b><\/div>');
} else {

W('<div class="section-title">Device Configuration</div>');
W('<div class="section">');

tabCreate.apply(this, devices);

for (i = 0; i < devices.length; ++i) {
	t = devices[i][0];
	W('<div id="'+t+'-tab">');
	W('<input type="hidden" id="wifi_'+t+'_brates" name="wifi_'+t+'_brates">');
	W('<input type="hidden" id="wifi_'+t+'_noscan" name="wifi_'+t+'_noscan">');

	W('<ul class="tabs">');
	for (j = 0; j < deviceSections.length; j++) {
		W('<li><a href="javascript:deviceSectSelect('+i+',\''+deviceSections[j][0]+'\')" id="'+t+'-'+deviceSections[j][0]+'-tab">'+deviceSections[j][1]+'<\/a><\/li>');
	}
	W('<\/ul><div class="tabs-bottom"><\/div>');

	W('<div id="'+t+'-general">');

	var bands = [];

	if (eval('wireless.phy'+i+'_2G') == 1) {
		bands.push(['2g', '2.4 GHz']);
	}
	if (eval('wireless.phy'+i+'_5G') == 1) {
		bands.push(['5g', '5 GHz']);
	}
	if (eval('wireless.phy'+i+'_6G') == 1) {
		bands.push(['6g', '6 GHz']);
	}

	createFieldTable('', [
		{ title: 'Band', name: 'wifi_'+t+'_band', type: 'select', options: bands, value: nvram['wifi_'+t+'_band'] },
		{ title: 'Mode', name: 'wifi_'+t+'_mode', type: 'select',
			value: nvram['wifi_'+t+'_mode'], options: [], prefix: '<span id="__wifi_'+t+'_mode">', suffix: '<\/span>' },
		{ title: 'Channel', name: 'wifi_'+t+'_channel', type: 'select',
			value: nvram['wifi_'+t+'_channel'], options: [], prefix: '<span id="__wifi_'+t+'_channel">', suffix: '<\/span>' },
		{ title: 'Width', name: 'wifi_'+t+'_width', type: 'select',
			value: nvram['wifi_'+t+'_width'], options: [], prefix: '<span id="__wifi_'+t+'_width">', suffix: '<\/span>'},
		{ title: 'Allow legacy 802.11b rates', name: 'f_wifi_'+t+'_brates', type: 'checkbox', value: nvram['wifi_'+t+'_brates'] == 1 },
		{ title: 'Maximum transmit power', name: 'wifi_'+t+'_power', type: 'select',
			value: nvram['wifi_'+t+'_power'], options: [], prefix: '<span id="__wifi_'+t+'_power">', suffix: '<\/span>' },
		{ title: 'Country code', name: 'wifi_'+t+'_country', type: 'select',
			value: nvram['wifi_'+t+'_country'], options: [], prefix: '<span id="__wifi_'+t+'_country">', suffix: '<\/span>' }
	]);
	W('<\/div>');

	W('<div id="'+t+'-advanced">');
	createFieldTable('', [
		{ title: 'Coverage cell density', name: 'wifi_'+t+'_cell_density', type: 'select',
			value: nvram['wifi_'+t+'_cell_density'], options: [['0','Disabled'],['1','Normal'],['2','High'],['3','Very High']] },
		{ title: 'Force 40MHz mode', name: 'f_wifi_'+t+'_noscan', type: 'checkbox', value: nvram['wifi_'+t+'_noscan'] == 1 },
		{ title: 'Custom Configuration', name: 'wifi_'+t+'_custom', type: 'textarea', value: nvram['wifi_'+t+'_custom'],
			attrib: 'style="width:40em"' }
	]);
	W('<\/div>');

	W('<\/div>');
}

} /* hasWirelessDevices */
</script>
</div>

<script>

if (hasWirelessDevices) {

W('<div class="section-title">Interface Configuration</div>');
W('<div class="section">');

for (var i = 0; i < devices.length; i++) {
	W('<div id="phy'+i+'-interface-tab">');
	W('<ul id="tabs">');
	for (var j = 0; j < interfaceCount[i]; j++) {
		interfaces.push([devices[i][0]+'iface'+j, devices[i][0]+'iface'+j]);
		W('<li><a href="javascript:interfaceSelect('+i+',\''+devices[i][0]+'iface'+j+'\')" id="'+devices[i][0]+'iface'+j+'-tab">phy'+i+' interface '+j+'<\/a><\/li>');
	}

	if (eval('nvram["wifi_phy"+i+"_ifaces"]') < 16) {
		W('<li><a href="javascript:addInterface('+i+')">+<\/a><\/li>');
	}

	W('<\/ul><div id="tabs-bottom"></div>');
	for (var j = 0; j < interfaceCount[i]; j++) {
		W('<div id="phy'+i+'iface'+j+'">');

		t = 'phy'+i+'iface'+j;
		W('<input type="hidden" id="wifi_'+t+'_enable" name="wifi_'+t+'_enable">');
		W('<input type="hidden" id="wifi_'+t+'_hidden" name="wifi_'+t+'_hidden">');
		W('<input type="hidden" id="wifi_'+t+'_wmm" name="wifi_'+t+'_wmm">');
		W('<input type="hidden" id="wifi_'+t+'_uapsd" name="wifi_'+t+'_uapsd">');
		W('<input type="hidden" id="wifi_'+t+'_isolate" name="wifi_'+t+'_isolate">');
		W('<input type="hidden" id="wifi_'+t+'_br_isolate" name="wifi_'+t+'_br_isolate">');
		W('<input type="hidden" id="wifi_'+t+'_maclist" name="wifi_'+t+'_maclist">');
		W('<input type="hidden" id="wifi_'+t+'_mesh_fwding" name="wifi_'+t+'_mesh_fwding">');
		W('<input type="hidden" id="wifi_'+t+'_ieee80211r" name="wifi_'+t+'_ieee80211r">');
		W('<input type="hidden" id="wifi_'+t+'_ft_psk_generate_local" name="wifi_'+t+'_ft_psk_generate_local">');
		W('<input type="hidden" id="wifi_'+t+'_pmk_r1_push" name="wifi_'+t+'_pmk_r1_push">');
		W('<input type="hidden" id="wifi_'+t+'_ieee80211k" name="wifi_'+t+'_ieee80211k">');
		W('<input type="hidden" id="wifi_'+t+'_rrm_neighbor_report" name="wifi_'+t+'_rrm_neighbor_report">');
		W('<input type="hidden" id="wifi_'+t+'_rrm_beacon_report" name="wifi_'+t+'_rrm_beacon_report">');
		W('<input type="hidden" id="wifi_'+t+'_wnm_sleep_mode" name="wifi_'+t+'_wnm_sleep_mode">');
		W('<input type="hidden" id="wifi_'+t+'_wnm_sleep_mode_no_keys" name="wifi_'+t+'_wnm_sleep_mode_no_keys">');
		W('<input type="hidden" id="wifi_'+t+'_bss_transition" name="wifi_'+t+'_bss_transition">');
		W('<input type="hidden" id="wifi_'+t+'_proxy_arp" name="wifi_'+t+'_proxy_arp">');

		W('<ul class="tabs">');
		for (var k = 0; k < interfaceSections.length; k++) {
			var u = devices[i][0]+'iface'+j+'-'+interfaceSections[k][0];
			W('<li><a href="javascript:interfaceSectSelect('+i+','+j+','+k+')" id="'+u+'-tab">'+interfaceSections[k][1]+'<\/a><\/li>');
		}
		W('<\/ul><div class="tabs-bottom"><\/div>');
		W('<div id="'+t+'-general">');

		createFieldTable('', [
			{ title: 'Enable', name: 'f_wifi_'+t+'_enable', type: 'checkbox', value: nvram['wifi_'+t+'_enable'] == 1 },
			{ title: 'Mode', name: 'wifi_'+t+'_mode', type: 'select', options: [['ap', 'Access Point'],['sta','Client'],['bridge','Wireless Ethernet Bridge'],['mesh','802.11s Mesh Point']], value: nvram['wifi_'+t+'_mode'] },
			{ title: 'ESSID', name: 'wifi_'+t+'_essid', type: 'text', maxlen: 32, size: 34, value: nvram['wifi_'+t+'_essid'] },
			{ title: 'BSSID', name: 'wifi_'+t+'_bssid', type: 'text', maxlen: 17, size: 34, value: nvram['wifi_'+t+'_bssid'] },
			{ title: 'Network', name: 'wifi_'+t+'_network', type: 'select', options: networks, value: nvram['wifi_'+t+'_network'] },
			{ title: 'Hide ESSID', name: 'f_wifi_'+t+'_hidden', type: 'checkbox', value: nvram['wifi_'+t+'_hidden'] == 1 },
			{ title: 'WMM Mode', name: 'f_wifi_'+t+'_wmm', type: 'checkbox', value: nvram['wifi_'+t+'_wmm'] == 1 },
			{ title: 'U-APSD', indent: 2, name: 'f_wifi_'+t+'_uapsd', type: 'checkbox', value: nvram['wifi_'+t+'_uapsd'] == 1, suffix: '&nbsp; <small>Unscheduled Automatic Power Save Delivery<\/small><\/div>' }
		]);
		W('<\/div>');

		W('<div id="'+t+'-security">');
		createFieldTable('', [
			{ title: 'Encryption', name: 'wifi_'+t+'_encryption', type: 'select', options: security_modes, value: nvram['wifi_'+t+'_encryption'] },
			{ title: 'Cipher', name: 'wifi_'+t+'_cipher', type: 'select', options: ciphers, value: nvram['wifi_'+t+'_cipher'] },
			{ title: 'Key', name: 'wifi_'+t+'_key', type: 'password', maxlen: 64, size: 66, peekaboo: 1,
				suffix: ' <input type="button" id="_f_wifi_'+t+'_psk_random1" value="Random" onclick="random_psk(\'_wifi_'+t+'_key\')">', value: nvram['wifi_'+t+'_key'] }
		]);
		W('<\/div>');

		W('<div id="'+t+'-filter">');
		createFieldTable('', [
			{ title: 'MAC Address Filter', name: 'wifi_'+t+'_macfilter', type: 'select', options: [['', 'Disable'],['allow', 'Allow listed only'],['deny', 'Allow all except listed']], value: nvram['wifi_'+t+'_macfilter'] },
			{ title: 'MAC-List', suffix: '<div class="tomato-grid" id="table_'+t+'_maclist"><\/div>' },
		]);
		W('<\/div>');

		W('<div id="'+t+'-advanced">');
		createFieldTable('', [
			{ title: 'Isolate Clients', name: 'f_wifi_'+t+'_isolate', type: 'checkbox', value: nvram['wifi_'+t+'_isolate'] == 1 },
			{ title: 'Isolate Bridge Port', name: 'f_wifi_'+t+'_br_isolate', type: 'checkbox', value: nvram['wifi_'+t+'_br_isolate'] == 1 },
			{ title: 'Forward mesh peer traffic', name: 'f_wifi_'+t+'_mesh_fwding', type: 'checkbox', value: nvram['wifi_'+t+'_mesh_fwding'] == 1 },
			{ title: 'RSSI Threshold', name: 'wifi_'+t+'_mesh_rssi_threshold', type: 'text', maxlen: 4, size: 6, value: nvram['wifi_'+t+'_mesh_rssi_threshold'],
				suffix: '&nbsp;<small>dBm. 0=disabled, 1=driver default, -255 to -10 = signal floor<\/small><\/div>' },
			{ title: 'Interface name', name: 'wifi_'+t+'_ifname', type: 'text', maxlen: 15, size: 17, value: nvram['wifi_'+t+'_ifname'] },
			{ title: 'Custom Configuration', name: 'wifi_'+t+'_custom', type: 'textarea', value: nvram['wifi_'+t+'_custom'],
				attrib: 'style="width:40em"' }
		]);
		W('<\/div>');

		W('<div id="'+t+'-roaming">');
		createFieldTable('', [
			{ title: '802.11r Fast Transition', name: 'f_wifi_'+t+'_ieee80211r', type: 'checkbox', value: nvram['wifi_'+t+'_ieee80211r'] == 1,
				suffix: '<div class="notice"><small>Enables fast roaming among access points that belong to the same Mobility Domain<\/small><\/div>' },
			{ title: 'NAS ID', name: 'wifi_'+t+'_nasid', type: 'text', maxlen: 48, size: 34, value: nvram['wifi_'+t+'_nasid'],
				suffix: '<div class="notice"><small>Used for two different purposes: RADIUS NAS ID and 802.11r R0KH-ID<\/small><\/div>' },
			{ title: 'Mobility Domain', name: 'wifi_'+t+'_mobility_domain', type: 'text', maxlen: 4, size: 6, value: nvram['wifi_'+t+'_mobility_domain'],
				suffix: '<div class="notice"><small>4-character hexadecimal ID<\/small><\/div>' },
			{ title: 'Reassociation Deadline', name: 'wifi_'+t+'_reassociation_deadline', type: 'text', maxlen: 5, size: 7, value: nvram['wifi_'+t+'_reassociation_deadline'],
				suffix: '<div class="notice"><small>time units (TUs / 1.024 ms) [1000-65535], default 20000<\/small><\/div>' },
			{ title: 'FT protocol', name: 'wifi_'+t+'_ft_over_ds', type: 'select', options: [['0','FT over the Air'],['1','FT over DS']], value: nvram['wifi_'+t+'_ft_over_ds'] },
			{ title: 'Generate PMK locally', name: 'f_wifi_'+t+'_ft_psk_generate_local', type: 'checkbox', value: nvram['wifi_'+t+'_ft_psk_generate_local'] == 1,
				suffix: '<div class="notice"><small>When using a PSK, the PMK can be automatically generated. When enabled, the R0/R1 key options below are not applied.<\/small><\/div>' },
			{ title: 'R0 Key Lifetime', name: 'wifi_'+t+'_r0_key_lifetime', type: 'text', maxlen: 10, size: 12, value: nvram['wifi_'+t+'_r0_key_lifetime'],
				suffix: '<div class="notice"><small>minutes, default 10000<\/small><\/div>' },
			{ title: 'R1 Key Holder', name: 'wifi_'+t+'_r1_key_holder', type: 'text', maxlen: 12, size: 14, value: nvram['wifi_'+t+'_r1_key_holder'],
				suffix: '<div class="notice"><small>6-octet identifier as a hex string - no colons<\/small><\/div>' },
			{ title: 'PMK R1 Push', name: 'f_wifi_'+t+'_pmk_r1_push', type: 'checkbox', value: nvram['wifi_'+t+'_pmk_r1_push'] == 1 },
			{ title: 'External R0 Key Holder List', name: 'wifi_'+t+'_r0kh', type: 'textarea', value: nvram['wifi_'+t+'_r0kh'].replace(/>/g, '\n'),
				attrib: 'style="width:40em"',
				suffix: '<div class="notice"><small>One entry per line. Format: MAC-address,NAS-Identifier,256-bit key as hex string<\/small><\/div>' },
			{ title: 'External R1 Key Holder List', name: 'wifi_'+t+'_r1kh', type: 'textarea', value: nvram['wifi_'+t+'_r1kh'].replace(/>/g, '\n'),
				attrib: 'style="width:40em"',
				suffix: '<div class="notice"><small>One entry per line. Format: MAC-address,R1KH-ID as 6 octets with colons,256-bit key as hex string<\/small><\/div>' },
			{ title: '802.11k RRM', name: 'f_wifi_'+t+'_ieee80211k', type: 'checkbox', value: nvram['wifi_'+t+'_ieee80211k'] == 1,
				suffix: '<div class="notice"><small>Radio Resource Measurement - Sends beacons to assist roaming. Not all clients support this.<\/small><\/div>' },
			{ title: 'Neighbour Report', name: 'f_wifi_'+t+'_rrm_neighbor_report', type: 'checkbox', value: nvram['wifi_'+t+'_rrm_neighbor_report'] == 1,
				suffix: '<div class="notice"><small>802.11k: Enable neighbor report via radio measurements.<\/small><\/div>' },
			{ title: 'Beacon Report', name: 'f_wifi_'+t+'_rrm_beacon_report', type: 'checkbox', value: nvram['wifi_'+t+'_rrm_beacon_report'] == 1,
				suffix: '<div class="notice"><small>802.11k: Enable beacon report via radio measurements.<\/small><\/div>' },
			{ title: 'Time advertisement', name: 'wifi_'+t+'_time_advertisement', type: 'select', options: [['0','Disabled'],['2','Enabled']], value: nvram['wifi_'+t+'_time_advertisement'],
				suffix: '<div class="notice"><small>802.11v: Time Advertisement in management frames.<\/small><\/div>' },
			{ title: 'Time zone', name: 'wifi_'+t+'_time_zone', type: 'text', maxlen: 64, size: 34, value: nvram['wifi_'+t+'_time_zone'],
				suffix: '<div class="notice"><small>802.11v: Local Time Zone Advertisement in management frames.<\/small><\/div>' },
			{ title: 'WNM Sleep Mode', name: 'f_wifi_'+t+'_wnm_sleep_mode', type: 'checkbox', value: nvram['wifi_'+t+'_wnm_sleep_mode'] == 1,
				suffix: '<div class="notice"><small>802.11v: Wireless Network Management (WNM) Sleep Mode (extended sleep mode for stations).<\/small><\/div>' },
			{ title: 'WNM Sleep Mode Fixes', name: 'f_wifi_'+t+'_wnm_sleep_mode_no_keys', type: 'checkbox', value: nvram['wifi_'+t+'_wnm_sleep_mode_no_keys'] == 1,
				suffix: '<div class="notice"><small>802.11v: WNM Sleep Mode Fixes: Prevents reinstallation attacks.<\/small><\/div>' },
			{ title: 'BSS Transition', name: 'f_wifi_'+t+'_bss_transition', type: 'checkbox', value: nvram['wifi_'+t+'_bss_transition'] == 1,
				suffix: '<div class="notice"><small>802.11v: Basic Service Set (BSS) transition management.<\/small><\/div>' },
			{ title: 'ProxyARP', name: 'f_wifi_'+t+'_proxy_arp', type: 'checkbox', value: nvram['wifi_'+t+'_proxy_arp'] == 1,
				suffix: '<div class="notice"><small>802.11v: Proxy ARP enables non-AP STA to remain in power-save for longer.<\/small><\/div>' }
		]);
		W('<\/div>');

		if (j == interfaceCount[i] - 1) {
			W('<br>');
			W('<input type="button" style="float: right;margin-right: 4px" value="Remove Interface" onclick="removeInterface('+i+')">');
		}
		W('<\/div>');
	}
	W('<\/div>');
}

} /* hasWirelessDevices */
</script>
</div>

<!-- / / / -->

<div id="notes-wrapper">
<div class="section-title">Notes <small><i><a href="javascript:toggleVisibility(cprefix,'notes');" id="toggleLink-notes"><span id="sesdiv_notes_showhide">(Show)</span></a></i></small></div>
<div class="section" id="sesdiv_notes" style="display:none">
	<i>Device Configuration:</i><br>
	<ul>
		<li><b>General Setup</b></li>
		<ul>
			<li><b>Band</b> - Frequency this radio will operate on.</li>
			<li><b>Mode</b> - Wifi standard to use.</li>
			<ul>
				<li><b>AX</b> - Wifi 6 (802.11ax)</li>
				<li><b>AC</b> - Wifi 5 (802.11ac)</li>
				<li><b>N</b> - Wifi 4 (802.11n)</li>
				<li><b>Legacy</b> - 802.11b/g</li>
			</ul>
			<li><b>Channel</b> - Specifies the wireless channel. “auto” defaults to the lowest available channel.</li>
			<li><b>Width</b> - The width is the range of frequencies i.e. how broad the signal is for transferring data.</li>
			<li><b>Allow legacy 802.11b rates</b> - Enable compatibility for very old devices running up to 11 Mbps. Can reduce network efficiency and lower throughput for all devices.</li>
			<li><b>Maximum transmit power</b> - The highest radio signal strength the Wi-Fi interface can use to transmit.</li>
			<li><b>Country code</b> - Specifies the Country Code/Regulatory Domain (e.g. US, EU) for the Wi-Fi radio. Affects available channels and transmit powers to comply with local regulations.</li>
		</ul>
		<li><b>Advanced Settings</b></li>
		<ul>
			<li><b>Force 40MHz mode </b> - Disables scanning and forces use of the specified width. Applicable to 80 & 160 widths as well.</li>
			<li><b>Custom Configuration</b> - Custom device config options, one per line. Format: <tt>key=value</tt>.<br>To pass directly to hostapd: <tt>hostapd:key=value</tt>.</li>
		</ul>
	</ul>
	<br>
	<i>Interface Configuration:</i><br>
	<ul>
		<li><b>General Setup</b></li>
		<ul>
			<li><b>Enable</b> - Enables this interface.</li>
			<li><b>Mode</b> - Interface operating mode.</li>
			<ul>
				<li><b>Access Point</b> - Creates a wireless network for devices to join.</li>
				<li><b>Client</b> - Connects to another wireless network. This connection can then be shared wired or wirelessly.</li>
				<li><b>Wireless Ethernet Bridge</b> - Bridges an upstream wireless network to the LAN.</li>
				<li><b>802.11s Mesh Point</b> - Creates a self-forming, self-healing mesh network using the 802.11s standard.</li>
			</ul>
			<li><b>ESSID / Mesh ID</b> - The network name. In 802.11s Mesh mode this field is relabeled "Mesh ID" and sets the mesh network identifier — all nodes in the same mesh must share it. In Client mode this is the SSID to connect to.</li>
			<li><b>BSSID</b> - Available in Client and Bridge mode and is optional. This specifies the MAC address of the Wireless SSID network you want to connect to. This is useful for situations where there may be multiple APs with the same SSID and you want to connect to a specific one.</li>
			<li><b>Network</b> - Which network/bridge this interface will join.</li>
			<li><b>Hide ESSID</b> - Disables the broadcasting of beacon frames if checked and hides the ESSID.</li>
			<li><b>WMM Mode</b> - Enables WMM. Where Wi-Fi Multimedia (WMM) Mode QoS is disabled, clients may be limited to 802.11a/802.11g rates. Required for 802.11n/802.11ac/802.11ax.</li>
			<li><b>U-APSD</b> - Allows wireless devices to save battery by sleeping longer and waking only when data is ready to transmit.</li>
		</ul>
		<li><b>Wireless Security</b></li>
		<ul>
			<li><b>Encryption</b> - Wireless encryption method.</li>
			<li><b>Cipher</b> - </li>
			<li><b>Key</b> - The Wifi Password. Must be 8-63 characters long.</li>
		</ul>
		<li><b>MAC-Filter</b></li>
		<ul>
			<li><b>Allow listed only</b> - Create a whitelist of allowed devices.</li>
			<li><b>Allow all except listed</b> - Create a blacklist of disallowed devices</li>
		</ul>
		<li><b>Advanced Settings</b></li>
		<ul>
			<li><b>Isolate Clients</b> - Isolates wireless clients from each other, </li>
			<li><b>Isolate Bridge Port</b> - Isolates wireless clients from each other on the AP's bridge. (i.e. 2.4ghz and 5ghz radios on the same AP.)</li>
			<li><b>Interface name</b> - Specifies a custom name for the Wi-Fi interface, which is otherwise automatically named.</li>
			<li><b>Forward mesh peer traffic</b> - Enables 802.11s layer-2 forwarding between mesh peers. Recommended on for most deployments.</li>
			<li><b>RSSI Threshold</b> - Minimum signal strength (dBm) required to establish a mesh peer link. 0 = disabled, 1 = use driver default, negative values (e.g. -70) set an explicit dBm floor.</li>
			<li><b>Custom Configuration</b> - Custom interface config options, one per line. Format: <tt>key=value</tt>.<br>To pass directly to hostapd (AP): <tt>hostapd:key=value</tt>.<br>For wpa_supplicant (STA/bridge/mesh): <tt>wpa:key=value</tt>.</li>
		</ul>
	</ul>
	<br>
	<i>Other relevant notes/hints:</i><br>
	<ul>
		<li>The Channel list and TX Power are dependent on your Country code. They are only updated accordingly once you</li>
		<ul>
			<li>make your country selection</li>
			<li>enable at least one interface</li>
			<li>click Save</li>
			Afterwards you may see a new selection of option to choose from.
		</ul>
		<li><b>5GHz Radio:</b> Before attempting to setup the 5Ghz radio, please follow the steps in the previous point. It can sometimes be difficult to find the right settings to bring the 5Ghz radio up, particularly if you want to use 160Mhz width. To see your country's regulations, run the following over ssh or in Tools -> System Commands</li>
			<ul>
				<li><b>iwinfo phy1-ap0 freqlist</b></li>
			</ul>
			Avoid channels with the "NO_IR" code, and channels with "NO_160MHZ" if wanting to use 160MHz widths. Also keep an eye in the logs for other issues/errors when starting the 5GHz radio.
	</ul>
</div>
</div>
<script>if (!hasWirelessDevices) E('notes-wrapper').style.display = 'none';</script>

<!-- / / / -->

<div id="footer">
	<span id="footer-msg"></span>
	<script>
	if (hasWirelessDevices) {
		W('<input type="button" value="Save" id="save-button" onclick="save()">');
		W('<input type="button" value="Cancel" id="cancel-button" onclick="reloadPage();">');
	} else {
		E('footer').style.display = 'none';
	}
	</script>
</div>

</td></tr>
</table>
</form>
<script>earlyInit();</script>
</body>
</html>
