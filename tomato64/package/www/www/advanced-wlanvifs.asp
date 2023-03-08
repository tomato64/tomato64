<!DOCTYPE html>
<!--
	Tomato GUI
	Copyright (C) 2006-2007 Jonathan Zarate
	http://www.polarcloud.com/tomato/

	Virtual Wireless Interfaces web interface & extensions
	Copyright (C) 2012 Augusto Bott
	http://code.google.com/p/tomato-sdhc-vlan/
	Some portions Copyright (C) Jean-Yves Avenard
	mailto:jean-yves@avenard.org

	For use with Tomato Firmware only.
	No part of this file may be used without permission.
	LAN Access admin module by Augusto Bott
-->
<html lang="en-GB">
<head>
<meta http-equiv="content-type" content="text/html;charset=utf-8">
<meta name="robots" content="noindex,nofollow">
<title>[<% ident(); %>] Advanced: Virtual Wireless Interfaces</title>
<link rel="stylesheet" type="text/css" href="tomato.css">
<% css(); %>
<script src="tomato.js"></script>
<script src="md5.js"></script>
<script src="interfaces.js"></script>
<script src="wireless.js"></script>
<script>
var lastjiffiestotal = 0, lastjiffiesidle = 0, lastjiffiesusage = 100;
</script>
<script src="wireless.jsx?_http_id=<% nv(http_id); %>"></script>
<script src="status-data.jsx?_http_id=<% nv(http_id); %>"></script>

<script>

//	<% nvram("wl_auth,wl_auth_mode,wl_ap_isolate,wl_bss_enabled,wl_channel,wl_closed,wl_corerev,wl_crypto,wl_hwaddr,wl_ifname,wl_key,wl_key1,wl_key2,wl_key3,wl_key4,wl_lazywds,wl_mode,wl_nband,wl_nbw_cap,wl_nctrlsb,wl_net_mode,wl_passphrase,wl_phytype,wl_radio,wl_radius_ipaddr,wl_radius_key,wl_radius_port,wl_security_mode,wl_ssid,wl_vifs,wl_wds,wl_wds_enable,wl_wep_bit,wl_wpa_gtk_rekey,wl_wpa_psk,wl_bss_maxassoc,wl_wme,lan_ifname,lan_ifnames,lan1_ifname,lan1_ifnames,lan2_ifname,lan2_ifnames,lan3_ifname,lan3_ifnames,t_features,wl_macmode,wl_maclist");%>

var cprefix = 'advanced_wlanvifs';
var vifs_possible = [];
var vifs_defined = [];
var vifs_deleted = [];
var max_no_vifs = 0;

var wl_modes_available = [];

var wmo = {'ap':'Access Point','apwds':'Access Point + WDS','sta':'Wireless Client','wet':'Wireless Ethernet Bridge','wds':'WDS'
/* BCMWL6-BEGIN */
	   ,'psta':'Media Bridge'
/* BCMWL6-END */
	   };
var macmode = {'disabled':'Disabled','deny':'Block','allow':'Permit'};

var tabs = [['overview','Overview']];

var xob = null;
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

earlyInit();

var wlg = new TomatoGrid();

wlg.setup = function() {
	this.init('wlif-grid', '', max_no_vifs, [
		{ type: 'select', options: vifs_possible },
		{ type: 'checkbox', prefix: '<div class="centered">', suffix: '<\/div>' },
		{ type: 'text', maxlen: 32, size: 34, prefix: '<div class="centered">', suffix: '<\/div>' },
		{ type: 'select', options: wl_modes_available , prefix: '<div class="centered">', suffix: '<\/div>' },
		{ type: 'select', options: [[0,'LAN0 (br0)'],[1,'LAN1 (br1)'],[2,'LAN2 (br2)'],[3,'LAN3 (br3)'],[4,'none']] },
		{ type: 'select', options: [['disabled','Disabled'],['deny','Block'],['allow','Permit']] }
	]);

	this.headerSet(['Interface', 'Enabled', 'SSID', 'Mode', 'Bridge', 'WFilter']);

	wlg.populate();

	wlg.canDelete = false;
	wlg.showNewEditor();
	wlg.resetNewEditor();
	if (wlg.getAllData().length >= max_no_vifs)
		wlg.disableNewEditor(true);
}

wlg.populate = function() {
	if (wlg.tb != null) {
		wlg.removeAllData();
		for (var uidx in vifs_defined) {
			if (typeof(vifs_defined[uidx][0]) == 'undefined')
				continue;

			var wmode = (((vifs_defined[uidx][7]) == 'ap') && ((nvram['wl'+u+'_wds_enable']) == '1')) ? 'apwds': (vifs_defined[uidx][7]);
			this.insertData(-1, [
				vifs_defined[uidx][0],
				vifs_defined[uidx][4],
				vifs_defined[uidx][8],
				wmode,
				vifs_defined[uidx][11].toString(),
				vifs_defined[uidx][12].toString()
			]);
		}
	}
}

wlg.resetNewEditor = function() {
	var f = fields.getAll(this.newEditor);
	f[2].value = '';
	f[1].checked = 1;

	f[0].selectedIndex=0;
	var t = max_no_vifs;
	while ((this.countVIF(f[0].options[f[0].selectedIndex].value) > 0) && (t > 0)) {
		f[0].selectedIndex = (f[0].selectedIndex%(max_no_vifs-1))+1;
		t--;
	}

	for (var i = 0; i < f[0].options.length ; i++) {
		f[0].options[i].disabled = (this.countVIF(f[0].options[i].value) > 0);
	}

/* REMOVE-BEGIN
	f[3].options[1].disabled = 1; // AP + WDS
	f[3].options[2].disabled = 1; // Wireless Client
	f[3].options[4].disabled = 1; // WDS
REMOVE-END */
	f[3].selectedIndex = 0;
	for (var i = 0; i < f[3].options.length ; i++) {
/* REMOVE-BEGIN
		f[3].options[i].disabled = ((f[3].options[i].value != 'ap') && (f[3].options[i].value != 'wet'));
REMOVE-END */
		f[3].options[i].disabled = (f[3].options[i].value != 'ap');
	}

	if (nvram.lan_ifname.length < 1)
		f[4].options[0].disabled = 1;
	if (nvram.lan1_ifname.length < 1)
		f[4].options[1].disabled = 1;
	if (nvram.lan2_ifname.length < 1)
		f[4].options[2].disabled = 1;
	if (nvram.lan3_ifname.length < 1)
		f[4].options[3].disabled = 1;

	f[4].selectedIndex = 4;
	f[5].selectedIndex = 0;
	ferror.clearAll(fields.getAll(this.newEditor));
}

wlg.dataToView = function(data) {
	var ifname, uidx;
	uidx = wl_ifidxx(data[0]);
	if (uidx < 0)
		ifname = 'wl'+data[0];
	else
		ifname = wl_display_ifname(uidx);

	return ([ifname,(data[1] == 1) ? 'Yes' : 'No',
	                 data[2] || '<small><i>(unset)<\/i><\/small>',
	                 wmo[data[3]] || '<small><i>(unset)<\/i><\/small>',
	                 ['LAN0 (br0)','LAN1 (br1)','LAN2 (br2)','LAN3 (br3)','none' ][data[4]],
	                 macmode[data[5]] || macmode[nvram['wl'+data[0].toString()+'_macmode']]
	       ]);
}

wlg.dataToFieldValues = function (data) {
	return ([data[0],(data[1] == '1') ? 'checked' : '',data[2],data[3],data[4],data[5]]);
}

wlg.fieldValuesToData = function(row) {
	var f = fields.getAll(row);

	return ([f[0].value,f[1].checked ? '1' : '0',f[2].value,f[3].value,f[4].value,f[5].value]);
}

wlg.onDelete = function() {
	this.removeEditor();
	if (this.source._data[0].indexOf('.') > 0) {
		var vif = definedVIFidx(this.source._data[0]);
		vifs_defined.splice(vif,1);
		vifs_deleted.push(this.source._data[0]);
		elem.remove(this.source);
		this.source = null;
	}
	else
		this.showSource();

	this.disableNewEditor(false);
	this.resetNewEditor();
}

wlg.onCancel = function() {
	this.removeEditor();
	this.showSource();
	this.disableNewEditor(false);
	this.resetNewEditor();
}

wlg.onAdd = function() {
	var data, u, wmode;

	this.moving = null;
	this.rpHide();

	if (!this.verifyFields(this.newEditor, 0))
		return;

	data = this.fieldValuesToData(this.newEditor);
	this.insertData(-1, data);

	u = data[0].toString();
	E('_f_wl'+u+'_radio').checked = (data[1] == '1');
	E('_wl'+u+'_ssid').value = data[2];
	E('_f_wl'+u+'_mode').value = data[3];
	E('_f_wl'+u+'_macmode').value = data[5];

	vifs_defined.push([
		u.toString(),				/* fface == wl_ifaces[uidx][1] */
		(nvram['wl'+u+'_ifname']) || ('wl'+u),	/* ifname =~ wl_ifaces[uidx][0] */
		u.substr(0, u.indexOf('.')),		/* unit */
		u.substr(u.indexOf('.')+1) || '-1',	/* subunit */
		data[1] || '1',				/* radio */
		'0',					/* iface up? */
		data[1] || '1',				/* bss_enabled */
		data[3],				/* WL net mode */
		data[2],				/* nvram['wl'+u+'_ssid'] */
		(eval('nvram["wl'+u+'_hwaddr"]')) || mac_null, /* MAC addr */
		'0',					/* VIFs supported */
		data[4],
		data[5]					/* Wireless Filter */
	]);

	this.resort();
	this.disableNewEditor(false);
	this.resetNewEditor();

	/* if we had previously deleted this entry, remove it from deleted table */
	for (var i = 0; i < vifs_deleted.length; i++) {
		if (vifs_deleted[i] == u) {
			vifs_deleted.splice(i, 1);
			break;
		}
	}

	tabSelect(u);
	verifyFields(null, 1);

	var e = E('footer-msg');
	e.innerHTML = 'After configuring this VIF, review and save your settings on the Overview tab.';
	e.style.display = 'inline';
/* REMOVE-BEGIN
	setTimeout(
		function() {
			e.innerHTML = '';
			e.style.display = 'none';
		}, 5000);
REMOVE-END */
}

wlg.onOK = function() {
/* TODO: update info on VIF tab
//	var i, data, view, e, u;
*/
	var i, data, view, u;

	if (!this.verifyFields(this.editor, 0))
		return;

	data = this.fieldValuesToData(this.editor);
	view = this.dataToView(data);

	u = data[0].toString();

	E('_f_wl'+u+'_radio').checked = (data[1] == '1');
	E('_wl'+u+'_ssid').value = data[2];
	E('_f_wl'+u+'_mode').value = data[3];
	E('_f_wl'+u+'_macmode').value = data[5];

	var vif = definedVIFidx(u);

	vifs_defined[vif][4] = data[1]; /* radio */
/* REMOVE-BEGIN
	vifs_defined[vif][6] = data[2]; // bss_enabled
	vifs_defined[vif][8] = data[3]; // SSID
	vifs_defined[vif][7] = data[4]; // WL mode
REMOVE-END */
	vifs_defined[vif][8] = data[2]; /* SSID */
	vifs_defined[vif][7] = data[3]; /* WL mode */
	vifs_defined[vif][11] = data[4]; /* LAN bridge */
	vifs_defined[vif][12] = data[5]; /* Wireless Filter */
/* REMOVE-BEGIN
	alert(data.join('\n'));
REMOVE-END */

	this.source.setRowData(data);
	for (i = 0; i < this.source.cells.length; ++i) {
		this.source.cells[i].innerHTML = view[i];
	}

	this.removeEditor();
	this.showSource();
	this.disableNewEditor(false);
	this.resetNewEditor();

	verifyFields(null, 1);
}

wlg.countElem = function(f, v) {
	var data = this.getAllData();
	var total = 0;
	for (var i = 0; i < data.length; ++i) {
		total += (data[i][f] == v) ? 1 : 0;
	}

	return total;
}

wlg.countVIF = function (v) {
	return this.countElem(0, v);
}

wlg.verifyFields = function(row, quiet) {
	var ok = 1;
	var f = fields.getAll(row);

	if (nvram.lan_ifname.length < 1)
		f[4].options[0].disabled = 1;
	if (nvram.lan1_ifname.length < 1)
		f[4].options[1].disabled = 1;
	if (nvram.lan2_ifname.length < 1)
		f[4].options[2].disabled = 1;
	if (nvram.lan3_ifname.length < 1)
		f[4].options[3].disabled = 1;

	if (f[0].value.indexOf('.') < 0) {
/* REMOVE-BEGIN
		fields.disableAll(row, 1);
		return 1;
REMOVE-END */
	}
	else {
		for (var i = 0; i < f[3].options.length ; i++) {
/* REMOVE-BEGIN
			f[3].options[i].disabled = ((f[3].options[i].value != 'ap') && (f[3].options[i].value != 'wet'));
REMOVE-END */
			f[3].options[i].disabled = (f[3].options[i].value != 'ap');
		}

	}
/* REMOVE-BEGIN
//	AB: user can't change WLIF name on gridObj, only by deleting/adding WLIf (destroying/recreating)
REMOVE-END */
	if (this.isEditing())
		f[0].disabled = 1;

	for (var i=0; i < f[0].options.length ; i++) {
		f[0].options[i].disabled = (this.countVIF(f[0].options[i].value) > 0);
	}

	if (!v_length(f[2], quiet || !ok, 1, 32))
		ok = 0;

	return ok;
}

function definedVIFidx(vif) {
	for (var i = 0; i < vifs_defined.length; ++i) {
		if (vifs_defined[i][0] == vif)
			return i;
	}
	return -1;
}

function tabSelect(name) {
	if (wlg.isEditing())
		return;

	tgHideIcons();
	tabHigh(name);

	if (!E('save-button').disabled)
		E('footer-msg').style.display = 'none';

	if (name == 'overview')
		wlg.populate();

	elem.display('overview-tab', (name == 'overview'));
	E('save-button').value = (name != 'overview') ? 'Overview' : 'Save';

	for (var i = 1; i < tabs.length; ++i) {
		if (name == tabs[i][0]) {
			if (definedVIFidx(name) < 0) {
				elem.display(tabs[i][0]+'-tab-disabled', 1);
				elem.display(tabs[i][0]+'-tab-enabled', 0);
			}
			else {
				elem.display(tabs[i][0]+'-tab-disabled', 0);
				elem.display(tabs[i][0]+'-tab-enabled', 1);
			}
		}
		else {
			elem.display(tabs[i][0]+'-tab-enabled', 0);
			elem.display(tabs[i][0]+'-tab-disabled', 0);
		}
	}

	cookie.set(cprefix+'_tab', name);
}

function submit_complete() {
	reloadPage();
}

function cancel() {
	cookie.set(cprefix+'_tab', 'overview');
	reloadPage();
}

function do_pre_submit_form(fom) {

	var footermsg = E('footer-msg');
	footermsg.innerHTML = 'Saving...';
	footermsg.style.display = 'inline';

	E('save-button').disabled = 1;
	E('cancel-button').disabled = 1;

	var elem = fom.elements;
	var s = '';

	for (var vidx = 0; vidx < vifs_possible.length; ++vidx) {
		var u = vifs_possible[vidx][0].toString();  /* WL unit (primary) or unit.subunit (virtual) */
		if (u.indexOf('.') > 0) { /* only if virtual VIF */
			var vif = definedVIFidx(u);
			if (vif >= 0) {
				for (var i = 0; i < elem.length ; ++i) {
					if (elem[i].name.indexOf('wl'+u) == 0)
						s += 'nvram set '+elem[i].name+'="'+elem[i].value+'"\n';
				}
			}
/* REMOVE-BEGIN
			// unset HWADDR for any/all non-primary VIFs we have configured
			s += 'nvram unset wl'+u+'_hwaddr\n';
			// AB TODO: figure out what to do with pre-existing/set MAC addresses
			if (vif >= 0) {
				if ((vifs_defined[vif][9] == mac_null) || (vifs_defined[vif][9] == '')) {
					s += 'nvram unset wl'+u+'_hwaddr\n';
				}
			}
REMOVE-END */
		}
	}

	/* Clean-up deleted interfaces */
	var lan_ifnames = nvram['lan_ifnames'];
	var lan1_ifnames = nvram['lan1_ifnames'];
	var lan2_ifnames = nvram['lan2_ifnames'];
	var lan3_ifnames = nvram['lan3_ifnames'];
	var wl0_vifs = nvram['wl0_vifs'];
	var wl1_vifs = nvram['wl1_vifs'];
	var wl2_vifs = nvram['wl2_vifs'];

	for (var vidx = 0; vidx < vifs_deleted.length; ++vidx) {
		var u = vifs_deleted[vidx];
		for (var i = 0; i < elem.length ; ++i) {
			if (elem[i].name.indexOf('wl'+u) == 0)
				s += 'nvram unset '+elem[i].name+'\n';
		}
		lan_ifnames = lan_ifnames.replace('wl'+u, '');
		lan1_ifnames = lan1_ifnames.replace('wl'+u, '');
		lan2_ifnames = lan2_ifnames.replace('wl'+u, '');
		lan3_ifnames = lan3_ifnames.replace('wl'+u, '');

		if (typeof(wl0_vifs) != 'undefined')
			wl0_vifs = wl0_vifs.replace('wl'+u, '');
		if (typeof(wl1_vifs) != 'undefined')
			wl1_vifs = wl1_vifs.replace('wl'+u, '');
		if (typeof(wl2_vifs) != 'undefined')
			wl2_vifs = wl2_vifs.replace('wl'+u, '');

		s += 'nvram unset wl'+u+'_wme\n';
		s += 'nvram unset wl'+u+'_bss_maxassoc\n';
		s += 'nvram unset wl'+u+'_macmode\n';
		s += 'nvram unset wl'+u+'_maclist\n';
	}
	if (vifs_deleted.length > 0) {
		s += 'nvram set lan_ifnames="'+lan_ifnames+'"\n';
		s += 'nvram set lan1_ifnames="'+lan1_ifnames+'"\n';
		s += 'nvram set lan2_ifnames="'+lan2_ifnames+'"\n';
		s += 'nvram set lan3_ifnames="'+lan3_ifnames+'"\n';

		if (typeof(wl0_vifs) != 'undefined')
			s += 'nvram set wl0_vifs="'+wl0_vifs+'"\n';
		if (typeof(wl1_vifs) != 'undefined')
			s += 'nvram set wl1_vifs="'+wl1_vifs+'"\n';
		if (typeof(wl2_vifs) != 'undefined')
			s += 'nvram set wl2_vifs="'+wl2_vifs+'"\n';
	}
	post_pre_submit_form(s);
}

var cmdresult = '';
var cmd = null;

function error_pre_submit_form() {
	var footermsg = E('footer-msg');

	footermsg.innerHTML = '<tt>'+escapeText(cmdresult)+'<\/tt>';
	footermsg.style.display = 'inline';

	cmdresult = '';
}

function post_pre_submit_form(s) {
	if (cmd)
		return;

	cmd = new XmlHttp();
	cmd.onCompleted = function(text, xml) {
		form.submit(E('t_fom'), 1);
	}
	cmd.onError = function(x) {
		cmdresult = 'ERROR: '+x;
		error_pre_submit_form();
	}

	cmd.post('shell.cgi', 'action=execute&command='+escapeCGI(s.replace(/\r/g, '')));
}

function escapeText(s) {
	function esc(c) {
		return '&#'+c.charCodeAt(0)+';';
	}

	return s.replace(/[&"'<>]/g, esc).replace(/\n/g, ' <br>').replace(/ /g, '&nbsp;');
}

function verifyFields(focused, quiet) {
	var i;
	var ok = 1;
	var a, b, c, d, e;
	var u, uidx;
	var wmode, sm2;

	for (uidx = 0; uidx < wl_ifaces.length; ++uidx) {
		u = wl_fface(uidx);
		if (u)
			E('wl'+u+'_hwaddr_msg').style.display = (((wl_ifaces[uidx][8] == 'ap') && (wl_ifaces[uidx][5] != wl_ifaces[uidx][9])) ? 'inline' : 'none');

		if (wl_sunit(uidx) < 0) {
			if (focused == E('_f_wl'+u+'_nband')) {
				refreshNetModes(uidx);
				refreshChannels(uidx);
				refreshBandWidth(uidx);
			}
			else if (focused == E('_f_wl'+u+'_nctrlsb') || focused == E('_wl'+u+'_nbw_cap'))
				refreshChannels(uidx);
		}
	}

	var wl_vis = [];
	for (var vidx = 0; vidx < vifs_possible.length; ++vidx) {
		var u = vifs_possible[vidx][0];
		if (definedVIFidx(u) < 0)
			continue;

		if (u.toString().indexOf('.') < 0) {
			var uidx = wl_ifidxx(u);
			a = {
			_f_wl_radio: 1,
			_f_wl_ap_isolate: 1,
			_f_wl_mode: 1,
/* AB disabled for VIFs?! */
			_f_wl_nband: (bands[uidx].length > 1) ? 1 : 0,
/* AB disabled for VIFs?! */
			_wl_net_mode: 1,
			_wl_ssid: 1,
			_f_wl_bcast: 1,
/* AB disabled for VIFs?! */
			_wl_channel: 1,
/* AB disabled for VIFs?! */
			_wl_nbw_cap: nphy || acphy ? 1 : 0,
/* AB disabled for VIFs?! */
			_f_wl_nctrlsb: nphy || acphy ? 1 : 0,
/* AB disabled for VIFs?! */
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
			_f_wl_wds_0: 1,
			_f_wl_macmode: 1
			};
		}
		else {
			a = {
			_f_wl_radio: 1,
			_f_wl_ap_isolate: 1,
			_f_wl_mode: 1,
/* REMOVE-BEGIN
// AB disabled for VIFs?!
			_f_wl_nband: (bands[uidx].length > 1) ? 1 : 0,
// AB disabled for VIFs?!
			_wl_net_mode: 1,
REMOVE-END */
			_wl_ssid: 1,
			_f_wl_bcast: 1,
/* REMOVE-BEGIN
// AB disabled for VIFs?!
			_wl_channel: 1,
			_wl_nbw_cap: nphy ? 1 : 0,
			_f_wl_nctrlsb: nphy ? 1 : 0,
			_f_wl_scan: 1,
REMOVE-END */

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
			_f_wl_wds_0: 1,
			_f_wl_macmode: 1
			};
		}
		wl_vis[vidx] = a;
	}

	for (var vidx = 0; vidx < vifs_possible.length; ++vidx) {
		var u = vifs_possible[vidx][0];
		if (definedVIFidx(u) < 0)
			continue;

		wmode = E('_f_wl'+u+'_mode').value;

		if (!E('_f_wl'+u+'_radio').checked) {
			for (a in wl_vis[vidx]) {
				wl_vis[vidx][a] = 2;
			}
			wl_vis[vidx]._f_wl_radio = 1;
			if (u.toString().indexOf('.') < 0) {
				var uidx = wl_ifidxx(u);
				wl_vis[vidx]._wl_nbw_cap = nphy || acphy ? 2 : 0;
				wl_vis[vidx]._f_wl_nband = (bands[uidx].length > 1) ? 2 : 0;
			}
		}

		E('wl'+u+'_mode_msg').style.display = (((wmode == 'sta') || (wmode == 'wet') ||
/* BCMWL6-BEGIN */
							(wmode == 'psta') ||
/* BCMWL6-END */
							0) ? 'inline' : 'none');

		switch (wmode) {
			case 'apwds':
			case 'wds':
			break;
			case 'wet':
/* BCMWL6-BEGIN */
			case 'psta':
/* BCMWL6-END */
			case 'sta':
				wl_vis[vidx]._f_wl_bcast = 0;
				if (u.toString().indexOf('.') < 0) {
					wl_vis[vidx]._wl_channel = 0;
					wl_vis[vidx]._wl_nbw_cap = 0;
				}
			default:
				wl_vis[vidx]._f_wl_lazywds = 0;
				wl_vis[vidx]._f_wl_wds_0 = 0;
			break;
		}

		sm2 = E('_wl'+u+'_security_mode').value;
		switch (sm2) {
			case 'disabled':
				wl_vis[vidx]._wl_crypto = 0;
				wl_vis[vidx]._wl_wep_bit = 0;
				wl_vis[vidx]._wl_wpa_psk = 0;
				wl_vis[vidx]._wl_radius_key = 0;
				wl_vis[vidx]._wl_radius_ipaddr = 0;
				wl_vis[vidx]._wl_wpa_gtk_rekey = 0;
			break;
			case 'wep':
				wl_vis[vidx]._wl_crypto = 0;
				wl_vis[vidx]._wl_wpa_psk = 0;
				wl_vis[vidx]._wl_radius_key = 0;
				wl_vis[vidx]._wl_radius_ipaddr = 0;
				wl_vis[vidx]._wl_wpa_gtk_rekey = 0;
			break;
			case 'radius':
				wl_vis[vidx]._wl_crypto = 0;
				wl_vis[vidx]._wl_wpa_psk = 0;
			break;
			default: /* wpaX */
				wl_vis[vidx]._wl_wep_bit = 0;
				if (sm2.indexOf('personal') != -1) {
					wl_vis[vidx]._wl_radius_key = 0;
					wl_vis[vidx]._wl_radius_ipaddr = 0;
				}
				else
					wl_vis[vidx]._wl_wpa_psk = 0;
			break;
		}

		if ((E('_f_wl'+u+'_lazywds').value == 1) && (wl_vis[vidx]._f_wl_wds_0 == 1))
			wl_vis[vidx]._f_wl_wds_0 = 2;

		if (u.toString().indexOf('.') < 0) {
			if (wl_vis[vidx]._wl_nbw_cap != 0) {
				switch (E('_wl'+u+'_net_mode').value) {
					case 'b-only':
					case 'g-only':
					case 'a-only':
					case 'bg-mixed':
						wl_vis[vidx]._wl_nbw_cap = 2;
						if (E('_wl'+u+'_nbw_cap').value != '0') {
							E('_wl'+u+'_nbw_cap').value = 0;
							refreshChannels(wl_ifidxx(u));
						}
					break;
				}
				/* avoid Enterprise-TKIP with 40MHz */
				if ((sm2 == 'wpa_enterprise') && (E('_wl'+u+'_crypto').value == 'tkip')) {
					wl_vis[vidx]._wl_nbw_cap = 2;
					if (E('_wl'+u+'_nbw_cap').value != '0') {
						E('_wl'+u+'_nbw_cap').value = 0;
						refreshChannels(wl_ifidxx(u));
					}
				}
			}

			wl_vis[vidx]._f_wl_nctrlsb = (E('_wl'+u+'_nbw_cap').value == 0) ? 0 : wl_vis[vidx]._wl_nbw_cap;

/* REMOVE-BEGIN
	This is ugly...
	Special case - 2.4GHz band, currently running in B/G-only mode,
	with N/Auto and 40MHz selected in the GUI.
	Channel list is not filtered in this case by the wl driver,
	and includes all channels available with 20MHz channel width.
REMOVE-END */

			b = selectedBand(wl_ifidxx(u));

			if (wl_vis[vidx]._wl_channel == 1 && wl_vis[vidx]._f_wl_nctrlsb != 0 && ((b == '2') || (wl_vis[vidx]._f_wl_nband == 0 && b == '0'))) {
				switch (eval('nvram["wl'+u+'_net_mode"]')) {
					case 'b-only':
					case 'g-only':
					case 'bg-mixed':
						i = E('_wl'+u+'_channel').value * 1;
						if (i > 0 && i < 5) {
							E('_f_wl'+u+'_nctrlsb').value = 'lower';
							wl_vis[vidx]._f_wl_nctrlsb = 2;
						}
						else if (i > max_channel[wl_ifidxx(u)] - 4) {
							E('_f_wl'+u+'_nctrlsb').value = 'upper';
							wl_vis[vidx]._f_wl_nctrlsb = 2;
						}
					break;
				}
			}
			wl_vis[vidx]._f_wl_scan = wl_vis[vidx]._wl_channel;
		}
		else {
			e = E('_f_wl'+u+'_mode');
			for (var i = 0; i < e.options.length ; i++) {
				e.options[i].disabled = (e.options[i].value != 'ap');
			}
		}

		wl_vis[vidx]._f_wl_psk_random1 = wl_vis[vidx]._wl_wpa_psk;
		wl_vis[vidx]._f_wl_psk_random2 = wl_vis[vidx]._wl_radius_key;
		wl_vis[vidx]._wl_radius_port = wl_vis[vidx]._wl_radius_ipaddr;
		wl_vis[vidx]._wl_key1 = wl_vis[vidx]._wl_key2 = wl_vis[vidx]._wl_key3 = wl_vis[vidx]._wl_key4 = wl_vis[vidx]._f_wl_wep_gen = wl_vis[vidx]._f_wl_wep_random = wl_vis[vidx]._wl_passphrase = wl_vis[vidx]._wl_wep_bit;

		for (i = 1; i < 10; ++i) {
			wl_vis[vidx]['_f_wl_wds_'+i] = wl_vis[vidx]._f_wl_wds_0;
		}

	} /* for each wl_iface */

	for (var vidx = 0; vidx < vifs_possible.length; ++vidx) {
		var u = vifs_possible[vidx][0];
		if (definedVIFidx(u) < 0)
			continue;

		for (a in wl_vis[vidx]) {
			i = 3;
			if (a.substr(0, 6) == '_f_wl_')
				i = 5;

			b = E(a.substr(0, i)+u+a.substr(i, a.length));
			c = wl_vis[vidx][a];
			b.disabled = (c != 1);
			PR(b).style.display = (c ? 'table-row' : 'none');
		}
	}

/* --- verify --- */

	var wlclnt = 0;
	for (var vidx = 0; vidx < vifs_possible.length; ++vidx) {
		var u = vifs_possible[vidx][0];
		if (definedVIFidx(u) < 0)
			continue;

		wmode = E('_f_wl'+u+'_mode').value;
		sm2 = E('_wl'+u+'_security_mode').value;

		/* --- N standard does not support WPA+TKIP --- */
		if (u.toString().indexOf('.') < 0) {
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
		}

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
		if (wl_vis[vidx]._f_wl_wpa_psk == 1) {
			if ((a.value.length < 8) || ((a.value.length == 64) && (a.value.search(/[^0-9A-Fa-f]/) != -1))) {
				ferror.set('_wl'+u+'_wpa_psk', 'Invalid pre-shared key. Please enter at least 8 characters or 64 hexadecimal digits.', quiet || !ok);
				ok = 0;
			}
		}

		if (u.toString().indexOf('.') < 0) {
			/* wl channel */
			if (((wmode == 'wds') || (wmode == 'apwds')) && (wl_vis[vidx]._wl_channel == 1) && (E('_wl'+u+'_channel').value == '0')) {
				ferror.set('_wl'+u+'_channel', 'Fixed wireless channel required in WDS mode.', quiet || !ok);
				ok = 0;
			}
			else
				ferror.clear('_wl'+u+'_channel');
		}
/* REMOVE-BEGIN
			if (E('_f_wl'+u+'_mode').value == 'sta') {
				if ((wan == 'disabled') && (E('_f_wl'+u+'_radio').checked)) {
					ferror.set('_wan_proto', 'Wireless Client mode requires a valid WAN setting (usually DHCP).', quiet || !ok);
					ok = 0;
				}
			}
REMOVE-END */
	}

	for (var vidx = 0; vidx < vifs_possible.length; ++vidx) {
		var v;
		var u = vifs_possible[vidx][0];
		if (definedVIFidx(u) < 0)
			continue;

		/* IP address */
		a = ['_radius_ipaddr'];
		for (i = a.length - 1; i >= 0; --i) {
			if ((wl_vis[vidx]['_wl'+a[i]]) && (!v_ip('_wl'+u+a[i], quiet || !ok)))
				ok = 0;
		}

		/* range */
		a = [['_wpa_gtk_rekey', 60, 7200], ['_radius_port', 1, 65535]];
		for (i = a.length - 1; i >= 0; --i) {
			v = a[i];
			if ((wl_vis[vidx]['_wl'+v[0]]) && (!v_range('_wl'+u+v[0], quiet || !ok, v[1], v[2])))
				ok = 0;
		}

		/* length */
		a = [['_ssid', 1], ['_radius_key', 1]];
		for (i = a.length - 1; i >= 0; --i) {
			v = a[i];
			if ((wl_vis[vidx]['_wl'+v[0]]) && (!v_length('_wl'+u+v[0], quiet || !ok, v[1], E('_wl'+u+v[0]).maxlength)))
				ok = 0;
		}

		if (wl_vis[vidx]._wl_key1) {
			a = (E('_wl'+u+'_wep_bit').value == 128) ? 26 : 10;
			for (i = 1; i <= 4; ++i) {
				b = E('_wl'+u+'_key'+i);
				b.maxLength = a;
				if ((b.value.length > 0) || (E('_f_wl'+u+'_wepidx_'+i).checked)) {
					if (!v_wep(b, quiet || !ok))
						ok = 0;
				}
				else
					ferror.clear(b);
			}
		}

		ferror.clear('_f_wl'+u+'_wds_0');
		if (wl_vis[vidx]._f_wl_wds_0 == 1) {
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

		ferror.clear('_f_wl'+u+'_macmode');
		a = E('_f_wl'+u+'_macmode').value;
		if (a != 'disabled' && a != 'deny' && a != 'allow') {
			ferror.set('_f_wl'+u+'_macmode', 'Wrong Wireless Filter settings.', quiet || !ok);
			ok = 0;
		}

		if ((ok) && (focused)) {
			var w = definedVIFidx(u);
			if (focused.id == '_wl'+u+'_ssid')
				vifs_defined[w][8] = focused.value;
			if (focused.id == '_f_wl'+u+'_mode')
				vifs_defined[w][7] = focused.value;
			if (focused.id == '_f_wl'+u+'_radio') {
				vifs_defined[w][4] = (focused.checked) ? '1' : '0';
				vifs_defined[w][6] = (focused.checked) ? '1' : '0';
			}
			if (focused.id == '_f_wl'+u+'_macmode')
				vifs_defined[w][12] = focused.value;
		}
	}

	return ok;
}

function save() {
	if (E('save-button').value != 'Save') {
		tabSelect('overview');
		return;
	}

	if (wlg.isEditing())
		return;

	wlg.resetNewEditor();

	if (!verifyFields(null, 0))
		return;

	var a, b, c;
	var w, uidx, wmode, sm2, wradio;

	var i, u, vidx, vif;
/* BCMWL6-BEGIN */
	var router_restart = 0;
/* BCMWL6-END */

	var fom = E('t_fom');

	for (var i = 0 ; i <= MAX_BRIDGE_ID ; i++) {
		var j = (i == 0) ? '' : i.toString();
		fom['lan'+j+'_ifnames'].value = '';
		var l = nvram['lan'+j+'_ifnames'].split(' ');
		for (var k = 0 ; k < l.length; ++k) {
			if (l[k].indexOf('vlan') != -1)
				fom['lan'+j+'_ifnames'].value += l[k]+' ';
		}
		fom['lan'+j+'_ifnames'].value = fom['lan'+j+'_ifnames'].value.trim();
	}

	for (vidx = 0; vidx < vifs_possible.length; ++vidx) {
		u = vifs_possible[vidx][0].toString(); /* WL unit (primary) or unit.subunit (virtual) */
		vif = definedVIFidx(u);

		if (u == 0)
			fom.wl_macmode.value = E('_f_wl'+u+'_macmode').value; /* rewrite for backward compatibility */

		if (vif < 0)
			E('_wl'+u+'_macmode').name = '_f_wl'+u+'_macmode_'; /* use fake input name to delete */
		else {
			E('_wl'+u+'_macmode').value = E('_f_wl'+u+'_macmode').value;
			E('_wl'+u+'_maclist').value = nvram['wl_maclist'].toString(); /* copy base maclist to 'u' interface */
		}
/* REMOVE-BEGIN
// AB TODO: try to play this safer - save some vital info on primary BSS (just in case?)
// AB TODO: with the UNSET part later on - is this really needed?
REMOVE-END */
		if (vif < 0) {
			a = [ ['radio', '0'], ['bss_enabled', '0'], ['ifname', ('wl'+u)] ];
			b = 'wl'+u+'_';
			for (i = 0; i < a.length; ++i) {
				c = ''+b+a[i][0];
				if (typeof(nvram[c]) != 'undefined')
					E('_'+c).value = a[i][1];
			}
			continue;
		}

		if (vifs_defined[vif][11]*1 != 4) {
			var x = (vifs_defined[vif][11] == '0') ? '' : vifs_defined[vif][11].toString();
			fom['lan'+x+'_ifnames'].value += ' '+vifs_defined[vif][1];
			fom['lan'+x+'_ifnames'].value = fom['lan'+x+'_ifnames'].value.trim();
		}

/* REMOVE-BEGIN
// AB TODO: cleanup in advance or just bail out later?
REMOVE-END */
		if (u.indexOf('.') < 0) {
			a = [];
			for (i = 0; i < vifs_defined.length; ++i) {
				if (vifs_defined[i][2].toString() != u)
					continue;
				if ((vifs_defined[i][3] * 1) < 1)
					continue;
				a.push('wl'+vifs_defined[i][0]);
			}
			E('_wl'+u+'_vifs').value = a.join(' ');
			E('_wl'+u+'_ifname').value = nvram['wl'+u+'_ifname'] || vifs_defined[vif][1];
		}
		else
			E('_wl'+u+'_ifname').value = nvram['wl'+u+'_ifname'] || 'wl'+u;

		wmode = E('_f_wl'+u+'_mode').value;
		sm2 = E('_wl'+u+'_security_mode').value;
		wradio = E('_f_wl'+u+'_radio').checked;

		if (wmode == 'apwds')
			E('_wl'+u+'_mode').value = 'ap';
		else
			E('_wl'+u+'_mode').value = wmode;

/* primary VIF */
		if (u.indexOf('.') < 0) {
			E('_wl'+u+'_nband').value = selectedBand(wl_ifidxx(u));

			a = [];
			for (i = 0; i < 10; ++i) {
				a.push(E('_f_wl'+u+'_wds_'+i).value);
			}
			E('_wl'+u+'_wds').value = joinAddr(a);

			if (wmode.indexOf('wds') != -1) {
				E('_wl'+u+'_wds_enable').value = 1;
				E('_wl'+u+'_lazywds').value = E('_f_wl'+u+'_lazywds').value;
				if (E('_wl'+u+'_lazywds').value == 1) E('_wl'+u+'_wds').value = '';
			}
			else {
				E('_wl'+u+'_wds_enable').value = 0;
				E('_wl'+u+'_wds').value = '';
				E('_wl'+u+'_lazywds').value = 0;
			}
		}

		E('_wl'+u+'_radio').value = wradio ? 1 : 0;
		E('_wl'+u+'_bss_enabled').value = wradio ? 1 : 0;
		E('_wl'+u+'_ap_isolate').value = E('_f_wl'+u+'_ap_isolate').checked ? 1 : 0;

		E('_wl'+u+'_auth').value = eval('nvram["wl'+u+'_auth"]') || '0';

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
			E('_wl'+u+'_auth').value = '0';

/* primary VIF */
		if (u.indexOf('.') < 0) {
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
					if (selectedBand(wl_ifidxx(u)) == '1') { /* 5 GHz */
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
				E('_wl'+u+'_nbw').value = (E('_wl'+u+'_nbw_cap').value == 0) ? 20 : ((E('_wl'+u+'_nbw_cap').value == 3) ? 80 : 40);
			}
		}

		E('_wl'+u+'_closed').value = E('_f_wl'+u+'_bcast').checked ? 0 : 1;

		a = fields.radio.selected(eval('fom["f_wl'+u+'_wepidx"]'));
/* REMOVE-BEGIN
		if (a) E('_wl'+u+'_key').value = a.value;
REMOVE-END */
		E('_wl'+u+'_key').value = (a) ? a.value : '1';
	}

/* BCMWL6-BEGIN */
	for (vidx = 0; vidx < vifs_possible.length; ++vidx) {
		u = vifs_possible[vidx][0].toString();
		if (definedVIFidx(u) < 0)
			continue;

		var check_psta = fom['wl'+u+'_mode'].value;

		if ((check_psta == 'psta') && (nvram['wl'+u+'_mode'] != check_psta))
			router_restart = 1;
	}

	if (router_restart) {
		fom._service.value = '*'; /* special case for Media Bridge mode: restart all */
	}
	else {
		fom._service.value = 'wlgui-restart'; /* always restart wireless */
	}
/* BCMWL6-END */

	do_pre_submit_form(fom);

/* REMOVE-BEGIN
	form.submit(fom, 1);
REMOVE-END */
}

function earlyInit() {

	for (var mode in wmo) {
		wl_modes_available.push([mode, wmo[mode]]);
	}

	for (var uidx = 0; uidx < wl_ifaces.length; ++uidx) {
		var u = wl_fface(uidx).toString();
		var bridged = 4;
		if (u) {
			var wmode = (((nvram['wl'+u+'_mode']) == 'ap') && ((nvram['wl'+u+'_wds_enable']) == '1')) ? 'apwds': (nvram['wl'+u+'_mode']);

			for (var i = 0 ; i <= MAX_BRIDGE_ID ; i++) {
				var j = (i == 0) ? '' : i.toString();
				var l = nvram['lan'+j+'_ifnames'].split(' ');
				for (var k = 0 ; k < l.length; k++) {
					if(l[k].indexOf(wl_ifaces[uidx][0]) != -1)
						bridged = i;
				}
			}

			var wlvifs = ((wl_ifaces[uidx][7] > 4) ? '4' : wl_ifaces[uidx][7].toString());
			vifs_defined.push([
				u.toString(),			/* fface == wl_ifaces[uidx][1] */
				wl_ifaces[uidx][0],
				wl_ifaces[uidx][2] || '0',	/* unit */
				wl_ifaces[uidx][3] || '0',	/* subunit */
				nvram['wl'+u+'_radio'] || '0',	/* radio */
				wl_ifaces[uidx][6] || '0',	/* iface up/operational status */
				nvram['wl'+u+'_bss_enabled'] || '1', /* bss_enabled */
				wmode || 'disabled',		/* WL net mode */
				wl_ifaces[uidx][4] || '',	/* nvram['wl'+u+'_ssid'] */
				nvram['wl'+u+'_hwaddr'],	/* MAC addr */
				wlvifs,				/* VIFs supported */
				bridged,
				nvram['wl'+u+'_macmode'] || 'disabled' /* Wireless Filter */
			]);
			/* max_no_vifs = max_no_vifs+((wl_ifaces[uidx][7] > 4) ? 4 : wl_ifaces[uidx][7]); */
			max_no_vifs = max_no_vifs + parseInt(wlvifs);
		}
	}

	W('<style>\n');
	for (var uidx in vifs_defined) {
		if (typeof(vifs_defined[uidx][0]) == 'undefined')
			continue;

		var total = vifs_defined[uidx][10] * 1;
		if (isNaN(total))
			continue;
		if (total >= 4)
			total = 4;

		W('#spin'+vifs_defined[uidx][2]+', \n');

		for (var i = 0; i < total; ++i) {
			var u = vifs_defined[uidx][2].toString();
			var s = (i == 0) ? '' : '.'+i.toString();
			var t = u+s;
			var v = wl_ifidxx(t);
			var w = (v < 0 ? 'wl'+t : wl_display_ifname(v));
			vifs_possible.push([t, w]);
			tabs.push([t, w]);
		}
	}

	W('#spin {\ndisplay:none;\nvertical-align:middle;\n}\n<\/style>\n');
}

function init() {
	var uninit = wl_ifaces.length - 1;
	while (uninit > 0) {
		if (((nvram['wl'+wl_unit(uninit)+'_corerev']) * 1) >= 9)
			break;

		uninit--;
	}

	E('sesdiv_vifs').style.display = 'block';
	if (uninit < 0) {
		E('sesdiv_vifs').innerHTML = '<div id="notice">This feature is not supported on this router.<\/div>';
		return;
	}

	tabSelect(cookie.get(cprefix+'_tab') || tabs[0][0]);

	var c;

	if (((c = cookie.get(cprefix+'_notes_vis')) != null) && (c == '1'))
		toggleVisibility(cprefix, 'notes');

	if (((c = cookie.get(cprefix+'_details_vis')) != null) && (c == '1'))
		toggleVisibility(cprefix, 'details');

	wlg.setup();
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

<input type="hidden" name="_nextpage" value="advanced-wlanvifs.asp">
<input type="hidden" name="_nextwait" value="10">
<input type="hidden" name="_service" value="wlgui-restart">
<input type="hidden" name="_force_commit" value="1">
<input type="hidden" name="lan_ifnames" value="">
<input type="hidden" name="lan1_ifnames" value="">
<input type="hidden" name="lan2_ifnames" value="">
<input type="hidden" name="lan3_ifnames" value="">
<input type="hidden" name="wl_macmode" value="">

<!-- / / / -->

<div class="section-title">Virtual Wireless Interfaces</div>

<div id="sesdiv_vifs" style="display:none">

	<div class="section">
		<script>tabCreate.apply(this, tabs);</script>
	</div>

<!-- / / / -->

	<div id="overview-tab">
		<div class="tomato-grid" id="wlif-grid"></div>

<!-- / / / -->

		<div class="section-title">Wireless Interfaces Details <small><i><a href="javascript:toggleVisibility(cprefix,'details');"><span id="sesdiv_details_showhide">(Show)</span></a></i></small></div>
		<div class="section" id="sesdiv_details" style="display:none">
			<script>
				for (var uidx = 0; uidx < wl_ifaces.length; ++uidx) {
					if (wl_sunit(uidx) < 0) {
						var c = [];
						c.push({ title: 'Interface'+(wlstats[uidx].radio ? '' : '&nbsp; &nbsp;<span style="color:red">Disabled<\/span>'), text: wl_display_ifname(uidx) });
						c.push({ title: 'Virtual Interfaces', indent: 2, rid: 'wl'+wl_fface(uidx)+'_vifs', text: 'wl'+wl_fface(uidx)+' '+nvram['wl'+wl_fface(uidx)+'_vifs']+' <small>(max '+wl_ifaces[uidx][7]+')<\/small>' });
						createFieldTable('',c);
					}
				}
			</script>
		</div>

<!-- / / / -->

		<div class="section-title">Notes <small><i><a href="javascript:toggleVisibility(cprefix,'notes');"><span id="sesdiv_notes_showhide">(Show)</span></a></i></small></div>
		<div class="section" id="sesdiv_notes" style="display:none">
			<ul>
				<li><b>Warning!</b> Avoid using 'virtual interfaces' on interface in Wireless Ethernet Bridge or Media Bridge mode due to possible problems.</li>
			</ul>
			<br>
			<ul>
				<li><b>Interface</b> - Wireless VIF name.</li>
				<li><b>Enabled</b> - If this VIF should be active and brought online.</li>
				<li><b>SSID</b> - Wireless Service Set Identifier.</li>
				<li><b>Mode</b> - Interface mode: Access Point, WDS, Wireless Client, etc...</li>
				<li><b>Bridge</b> - Which LAN bridge this VIF should be assigned.</li>
				<li><b>WFilter</b> - <a href="basic-wfilter.asp" class="new_window">Wireless Filter</a>:
					<ul>
						<li><i>Disabled</i> - Disable filter on that interface.</li>
						<li><i>Block</i> - Block clients from the <a href="basic-wfilter.asp" class="new_window">list</a> on that interface.</li>
						<li><i>Permit</i> - Permit only clients from the <a href="basic-wfilter.asp" class="new_window">list</a> on that interface.</li>
					</ul>
				</li>
			</ul>
			<br>
			<ul>
				<li><b>Other relevant notes/hints:</b>
					<ul>
						<li>When creating/defining a new wireless VIF, its MAC address will be shown (incorrectly) as '00:00:00:00:00:00', as it's unknown at that moment (until network is restarted and this page is reloaded).</li>
						<li>When saving changes, the MAC addresses of all defined non-primary wireless VIFs could sometimes be (already) <i>set</i> but might be <i>recreated</i> by the WL driver (so that previously defined/saved settings might need to be updated/changed accordingly on <a href="advanced-mac.asp">Advanced/MAC Address</a> after saving settings and rebooting your router).</li>
						<li>This web interface allows configuring a maximum of 4 VIFs for each physical wireless interface available - up to 3 extra VIFs can be defined in addition to the primary VIF (<i>on devices with multiple VIF capabilities</i>).</li>
						<li>By definition, configuration settings for the <i>primary VIF</i> of any physical wireless interfaces shouldn't be touched here (use the <a href="basic-network.asp">Basic/Network</a> page instead).</li>
						<li>Wireless clients will not be able to communicate with each other if setting <b>AP Isolation</b> is enabled.</li>
					</ul>
				</li>
			</ul>
		</div>

<!-- overview-tab -->
	</div>

<!-- / / / -->

	<div class="section" id="wlif-section">
		<script>
			for (var i = 1; i < tabs.length; ++i) {
				var t = tabs[i][0];
				var uidx = wl_ifidxx(t);
				var u = t;

				W('<div id="'+t+'-tab-disabled">VIF '+tabs[i][1]+' is not defined.<\/div>');

				W('<div id="'+t+'-tab-enabled">');

/* common to all VIFs */
				W('<input type="hidden" id="_wl'+u+'_radio" name="wl'+u+'_radio">');
				W('<input type="hidden" id="_wl'+u+'_ap_isolate" name="wl'+u+'_ap_isolate">');
				W('<input type="hidden" id="_wl'+u+'_mode" name="wl'+u+'_mode">');
				W('<input type="hidden" id="_wl'+u+'_closed" name="wl'+u+'_closed">');
				W('<input type="hidden" id="_wl'+u+'_key" name="wl'+u+'_key">');
				W('<input type="hidden" id="_wl'+u+'_akm" name="wl'+u+'_akm">');
				W('<input type="hidden" id="_wl'+u+'_auth_mode" name="wl'+u+'_auth_mode">');
				W('<input type="hidden" id="_wl'+u+'_wep" name="wl'+u+'_wep">');
				W('<input type="hidden" id="_wl'+u+'_auth" name="wl'+u+'_auth">');
				W('<input type="hidden" id="_wl'+u+'_bss_enabled" name="wl'+u+'_bss_enabled">');
				W('<input type="hidden" id="_wl'+u+'_ifname" name="wl'+u+'_ifname">');
				W('<input type="hidden" id="_wl'+u+'_macmode" name="wl'+u+'_macmode">');
				W('<input type="hidden" id="_wl'+u+'_maclist" name="wl'+u+'_maclist">');

/* only if primary VIF */
				if (u.toString().indexOf('.') < 0) {
					W('<input type="hidden" id="_wl'+u+'_nband" name="wl'+u+'_nband">');
					W('<input type="hidden" id="_wl'+u+'_wds_enable" name="wl'+u+'_wds_enable">');
					W('<input type="hidden" id="_wl'+u+'_wds" name="wl'+u+'_wds">');
					W('<input type="hidden" id="_wl'+u+'_lazywds" name="wl'+u+'_lazywds">');
					W('<input type="hidden" id="_wl'+u+'_gmode" name="wl'+u+'_gmode">');

					W('<input type="hidden" id="_wl'+u+'_nmode" name="wl'+u+'_nmode">');
					W('<input type="hidden" id="_wl'+u+'_nmcsidx" name="wl'+u+'_nmcsidx">');
					W('<input type="hidden" id="_wl'+u+'_nreqd" name="wl'+u+'_nreqd">');
					W('<input type="hidden" id="_wl'+u+'_nctrlsb" name="wl'+u+'_nctrlsb">');
					W('<input type="hidden" id="_wl'+u+'_nbw" name="wl'+u+'_nbw">');
/* BCMWL6-BEGIN */
					W('<input type="hidden" id="_wl'+u+'_bss_opmode_cap_reqd" name="wl'+u+'_bss_opmode_cap_reqd">');
/* BCMWL6-END */
					W('<input type="hidden" id="_wl'+u+'_vifs" name="wl'+u+'_vifs">');
				}
/* common to all VIFs */
				var f = [];
				f.push (
					{ title: 'Enable Interface', name: 'f_wl'+u+'_radio', type: 'checkbox', value: (eval('nvram["wl'+u+'_radio"]') == '1') && (eval('nvram["wl'+u+'_net_mode"]') != 'disabled') },
					{ title: 'AP Isolation', name: 'f_wl'+u+'_ap_isolate', type: 'checkbox', value: (eval('nvram["wl'+u+'_ap_isolate"]') == '1') },
					{ title: 'MAC Address', text: '<a href="advanced-mac.asp">'+(eval('nvram["wl'+u+'_hwaddr"]') || mac_null)+'<\/a>'+' &nbsp; <b id="wl'+u+'_hwaddr_msg" style="display:none"><small>(warning: WL driver reports BSSID <a href="advanced-mac.asp">'+((typeof(wl_ifaces[wl_ifidxx(u)]) != 'undefined') ? wl_ifaces[wl_ifidxx(u)][9] : '')+'<\/a>)<\/small><\/b>' },
					{ title: 'Wireless Mode', name: 'f_wl'+u+'_mode', type: 'select', options: wl_modes_available, value: ((eval('nvram["wl'+u+'_mode"]') == 'ap') && (eval('nvram["wl'+u+'_wds_enable"]') == '1')) ? 'apwds' : eval('nvram["wl'+u+'_mode"]'), suffix: ' &nbsp; <b id="wl'+u+'_mode_msg" style="display:none"><small>(note: you might wish to cross-check settings later on <a href="basic-network.asp">Basic/Network<\/a>)<\/small><\/b>' }
				);

/* only if primary VIF */
				if (u.toString().indexOf('.') < 0) {
					f.push (
						{ title: 'Radio Band', name: 'f_wl'+u+'_nband', type: 'select', options: bands[uidx], value: eval('nvram["wl'+u+'_nband"]') || '0' == '0' ? bands[uidx][0][0] : eval('nvram["wl'+u+'_nband"]') },
						{ title: 'Wireless Network Mode', name: 'wl'+u+'_net_mode', type: 'select', value: (eval('nvram["wl'+u+'_net_mode"]') == 'disabled') ? 'mixed' : eval('nvram["wl'+u+'_net_mode"]'), options: [], prefix: '<span id="__wl'+u+'_net_mode">', suffix: '<\/span>' }
					);
				}

				if (typeof(eval('nvram["wl'+u+'_closed"]')) == 'undefined')
					nvram['wl'+u+'_closed'] = '0';

				f.push (
					{ title: 'SSID', name: 'wl'+u+'_ssid', type: 'text', maxlen: 32, size: 34, value: eval('nvram["wl'+u+'_ssid"]') },
					{ title: 'Broadcast', indent: 2, name: 'f_wl'+u+'_bcast', type: 'checkbox', value: (eval('nvram["wl'+u+'_closed"]') == '0') }
				);

/* only if primary VIF */
				if (u.toString().indexOf('.') < 0) {
					f.push (
						{ title: 'Channel', name: 'wl'+u+'_channel', type: 'select', options: ghz[uidx], prefix: '<span id="__wl'+u+'_channel">', suffix: '<\/span> <input type="button" id="_f_wl'+u+'_scan" value="Scan" onclick="scanButton('+u+')"> <img src="spin.gif" alt="" id="spin'+u+'">', value: eval('nvram["wl'+u+'_channel"]') },
						{ title: 'Channel Width', name: 'wl'+u+'_nbw_cap', type: 'select', options: [], value: eval('nvram["wl'+u+'_nbw_cap"]'), prefix: '<span id="__wl'+u+'_nbw_cap">', suffix: '<\/span>' },
						{ title: 'Control Sideband', name: 'f_wl'+u+'_nctrlsb', type: 'select', options: [['lower','Lower'],['upper','Upper']], value: eval('nvram["wl'+u+'_nctrlsb"]') == 'none' ? 'lower' : eval('nvram["wl'+u+'_nctrlsb"]') }
					);
				}

				if (typeof(eval('nvram["wl'+u+'_crypto"]')) == 'undefined')
					nvram['wl'+u+'_crypto'] = 'aes';

				f.push (
					null,
					{ title: '<a href="basic-wfilter.asp" class="new_window">Wireless Filter<\/a>', name: 'f_wl'+u+'_macmode', type: 'select', options: [['disabled','Disable filter on that interface'],['deny','Block clients from the list on that interface'],['allow','Permit only clients from the list on that interface']], value: nvram['wl'+u+'_macmode'] },
					null,
					{ title: 'Security', name: 'wl'+u+'_security_mode', type: 'select', options: [['disabled','Disabled'],['wep','WEP'],['wpa_personal','WPA Personal'],['wpa_enterprise','WPA Enterprise'],['wpa2_personal','WPA2 Personal'],['wpa2_enterprise','WPA2 Enterprise'],['wpaX_personal','WPA / WPA2 Personal'],['wpaX_enterprise','WPA / WPA2 Enterprise'],['radius','Radius']], value: eval('nvram["wl'+u+'_security_mode"]') },
					{ title: 'Encryption', indent: 2, name: 'wl'+u+'_crypto', type: 'select', options: [['tkip','TKIP'],['aes','AES'],['tkip+aes','TKIP / AES']], value: eval('nvram["wl'+u+'_crypto"]') },
					{ title: 'Shared Key', indent: 2, name: 'wl'+u+'_wpa_psk', type: 'password', maxlen: 64, size: 66, peekaboo: 1,
						suffix: ' <input type="button" id="_f_wl'+u+'_psk_random1" value="Random" onclick="random_psk(\'_wl'+u+'_wpa_psk\')">', value: eval('nvram["wl'+u+'_wpa_psk"]') },
					{ title: 'Shared Key', indent: 2, name: 'wl'+u+'_radius_key', type: 'password', maxlen: 80, size: 32, peekaboo: 1, suffix: ' <input type="button" id="_f_wl'+u+'_psk_random2" value="Random" onclick="random_psk(\'_wl'+u+'_radius_key\')">', value: eval('nvram["wl'+u+'_radius_key"]') },
					{ title: 'Group Key Renewal', indent: 2, name: 'wl'+u+'_wpa_gtk_rekey', type: 'text', maxlen: 4, size: 6, suffix: '&nbsp; <small>seconds<\/small>', value: eval('nvram["wl'+u+'_wpa_gtk_rekey"]') || '3600' },
					{ title: 'Radius Server', indent: 2, multi: [
						{ name: 'wl'+u+'_radius_ipaddr', type: 'text', maxlen: 15, size: 17, value: eval('nvram["wl'+u+'_radius_ipaddr"]') },
						{ name: 'wl'+u+'_radius_port', type: 'text', maxlen: 5, size: 7, prefix: ' : ', value: eval('nvram["wl'+u+'_radius_port"]') || '1812' } ] },
					{ title: 'Encryption', indent: 2, name: 'wl'+u+'_wep_bit', type: 'select', options: [['128','128-bits'],['64','64-bits']], value: eval('nvram["wl'+u+'_wep_bit"]') },
					{ title: 'Passphrase', indent: 2, name: 'wl'+u+'_passphrase', type: 'text', maxlen: 16, size: 20, suffix: ' <input type="button" id="_f_wl'+u+'_wep_gen" value="Generate" onclick="generate_wep('+u+')"> <input type="button" id="_f_wl'+u+'_wep_random" value="Random" onclick="random_wep('+u+')">', value: eval('nvram["wl'+u+'_passphrase"]') }
				);

				if (typeof(eval('nvram["wl'+u+'_key"]')) == 'undefined')
					nvram['wl'+u+'_key'] = '1';
/* REMOVE-BEGIN
					eval('nvram["wl'+u+'_key"] = 1');
REMOVE-END */

				for (var j = 1; j <= 4; ++j) {
					f.push(
						{ title: ('Key '+j), indent: 2, name: ('wl'+u+'_key'+j), type: 'text', maxlen: 26, size: 34, suffix: '<input type="radio" onchange="verifyFields(this,1)" onclick="verifyFields(this,1)" name="f_wl'+u+'_wepidx" id="_f_wl'+u+'_wepidx_'+j+'" value="'+j+'"'+((eval('nvram["wl'+u+'_key"]') == j) ? ' checked>' : '>'), value: nvram['wl'+u+'_key'+j] });
				}

				f.push(null, { title: 'WDS', name: 'f_wl'+u+'_lazywds', type: 'select', options: [['0','Link With...'],['1','Automatic']], value: nvram['wl'+u+'_lazywds'] } );
/* REMOVE-BEGIN
				alert('nvram["wl'+u+'_wds"]='+eval('nvram["wl'+u+'_wds"]'));
REMOVE-END */
				var wds = eval('nvram["wl'+u+'_wds"]');
				if (typeof(wds) == 'undefined')
					nvram['wl'+u+'_wds'] = '';

				wds = eval('nvram["wl'+u+'_wds"]').split(/\s+/);
/* REMOVE-BEGIN
				wds = (nvram['wl'+u+'_wds']).split(/\s+/);
REMOVE-END */
				for (var k = 0; k < 10; k += 2)	{
					f.push({ title: (k ? '' : 'MAC Address'), indent: 2, multi: [
						{ name: 'f_wl'+u+'_wds_'+k, type: 'text', maxlen: 17, size: 20, value: wds[k] || mac_null },
						{ name: 'f_wl'+u+'_wds_'+(k + 1), type: 'text', maxlen: 17, size: 20, value: wds[k + 1] || mac_null } ] } );
				}

				createFieldTable('', f);
				W('<\/div>');
			}
		</script>

<!-- section -->
	</div>

<!-- sesdiv_vifs -->
</div>

<!-- / / / -->

<div id="footer">
	<span id="footer-msg"></span>
	<input type="button" value="Save" id="save-button" onclick="save()">
	<input type="button" value="Cancel" id="cancel-button" onclick="cancel()">
</div>

</td></tr>
</table>
</form>
<script>
	for (var uidx = 0; uidx < wl_ifaces.length; ++uidx) {
		if (wl_sunit(uidx) < 0) {
			refreshNetModes(uidx);
			refreshChannels(uidx);
			refreshBandWidth(uidx);
		}
	}
	verifyFields(null, 1);
</script>
</body>
</html>
