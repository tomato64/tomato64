<!DOCTYPE html>
<!--
	Tomato GUI
	Copyright (C) 2006-2010 Jonathan Zarate
	http://www.polarcloud.com/tomato/

	Tomato VLAN GUI
	Copyright (C) 2011 Augusto Bott
	http://code.google.com/p/tomato-sdhc-vlan/

	For use with Tomato Firmware only.
	No part of this file may be used without permission.
-->
<html lang="en-GB">
<head>
<meta http-equiv="content-type" content="text/html;charset=utf-8">
<meta name="robots" content="noindex,nofollow">
<title>[<% ident(); %>] Status: Overview</title>
<link rel="stylesheet" type="text/css" href="tomato.css">
<% css(); %>
<script src="tomato.js"></script>
<script src="interfaces.js"></script>
<!-- USB-BEGIN -->
<script src="wwan_parser.js"></script>
<!-- USB-END -->

<script>

var wmo = {'ap':'Access Point','sta':'Wireless Client','wet':'Wireless Ethernet Bridge','wds':'WDS'
/* BCMWL6-BEGIN */
	   ,'psta':'Media Bridge'
/* BCMWL6-END */
	   };
var auth = {'disabled':'-','wep':'WEP','wpa_personal':'WPA Personal (PSK)','wpa_enterprise':'WPA Enterprise','wpa2_personal':'WPA2 Personal (PSK)','wpa2_enterprise':'WPA2 Enterprise','wpaX_personal':'WPA / WPA2 Personal','wpaX_enterprise':'WPA / WPA2 Enterprise','radius':'Radius'};
var enc = {'tkip':'TKIP','aes':'AES','tkip+aes':'TKIP / AES'};
var bgmo = {'disabled':'-','mixed':'Auto','b-only':'B Only','g-only':'G Only','bg-mixed':'B/G Mixed','lrs':'LRS','n-only':'N Only'
/* BCMWL6-BEGIN */
	    ,'nac-mixed':'N/AC Mixed','ac-only':'AC Only'
/* BCMWL6-END */
};
var lastjiffiestotal = 0, lastjiffiesidle = 0, lastjiffiesusage = 100;
var updateWWANTimers = [], customStatusTimers = [], show_dhcpc = [], show_codi = [], show_radio = [];
</script>

<script src="wireless.jsx?_http_id=<% nv(http_id); %>"></script>
<script src="status-data.jsx?_http_id=<% nv(http_id); %>"></script>

<script>
var cprefix = 'status_overview';
var u;
nphy = features('11n');

var ref = new TomatoRefresh('status-data.jsx?_http_id=<% nv(http_id); %>', '', 5, cprefix+'_refresh');

ref.refresh = function(text) {
	stats = {};
	try {
		eval(text);
	}
	catch (ex) {
		stats = {};
	}
	show();
}

/* USB-BEGIN */
foreach_wwan(function(i) {
	updateWWANTimers[i - 1] = new TomatoRefresh('wwansignal.cgi', 'mwan_num='+i, 30, '', 1);
	updateWWANTimers[i - 1].refresh = function(text) {
		try {
			E('WWANStatus'+i).innerHTML = createWWANStatusSection(i, eval(text));
		}
		catch (ex) {
		}
	}
});
/* USB-END */

for (var uidx = 1; uidx <= nvram.mwan_num; uidx++) {
	u = (uidx > 1) ? uidx : '';

	if (nvram['wan'+u+'_status_script'] == 1) {
		customStatusTimers[uidx - 1] = new TomatoRefresh('/user/cgi-bin/wan'+u+'_status.sh', null, 15, '', 1);
		customStatusTimers[uidx - 1].refresh = (function(u) {
			return function(text) {
				try {
					document.querySelector('#WanCustomStatus'+u+' > td').innerHTML = text;
				}
				catch (ex) {
				}
			};
		})(u);
	}
}

function visibility() {
	for (var uidx = 1; uidx <= nvram.mwan_num; ++uidx) {
		var u, proto;
		u = (uidx > 1) ? uidx : '';
		proto = nvram['wan'+u+'_proto'];

		show_dhcpc[uidx - 1] = ((proto == 'dhcp')
/* USB-BEGIN */
		                        || (proto == 'lte')
/* USB-END */
		                        || (((proto == 'l2tp') || (proto == 'pptp')) && (nvram.pptp_dhcp == '1')));
		show_codi[uidx - 1] = ((proto == 'pppoe') || (proto == 'l2tp') || (proto == 'pptp')
/* USB-BEGIN */
		                        || (proto == 'lte') || (proto == 'ppp3g')
/* USB-END */
		                      );
	}

	show_radio = [];
	for (var uidx = 0; uidx < wl_ifaces.length; ++uidx) {
		if (wl_sunit(uidx) < 0)
			show_radio.push((nvram['wl'+wl_fface(uidx)+'_radio'] == '1'));
	}
}
visibility();

function wlenable(uidx, n) {
	E('b_wl'+uidx+'_enable').disabled = 1;
	E('b_wl'+uidx+'_disable').disabled = 1;

	var fom = E('t_fom');
	fom.enable.value = ''+n;
	fom._wl_unit.value = wl_unit(uidx);
	form.submit(fom, 1, 'wlradio.cgi');
}

function dhcpc(what, unit) {
	E('b'+unit+'_renew').disabled = 1;
	E('b'+unit+'_release').disabled = 1;

	var fom = E('t_fom');
	fom.exec.value = what;
	fom.prefix.value = 'wan'+unit;
	form.submit(fom, 1, 'dhcpc.cgi');
}

function serv(service, uidx, sleep) {
	var u = (uidx > 1) ? uidx : '';

	E('b'+u+'_connect').disabled = 1;
	E('b'+u+'_disconnect').disabled = 1;

	var fom = E('t_fom');
	fom._service.value = service;
	fom._sleep.value = sleep;
	form.submit(fom, 1, 'service.cgi');
}

function wan_connect(uidx) {
	serv('wan'+uidx+'-restart', uidx, 5);
}

function wan_disconnect(uidx) {
	serv('wan'+uidx+'-stop', uidx, 2);
}

function watchdog_check(uidx) {
	var e = E('b'+uidx+'_check');
	e.disabled = 1;

	var c = '/usr/sbin/watchdog';
	var cmd = new XmlHttp();
	cmd.post('shell.cgi', 'action=execute&command='+escapeCGI(c.replace(/\r/g, '')));

	setTimeout(
		function() {
			e.disabled = 0;
		}, 15000);
}

function onRefToggle() {
	var u;
	ref.toggle();
	if (!ref.running) {
/* USB-BEGIN */
		for (var i = 0; i < updateWWANTimers.length; i++) {
			if (updateWWANTimers[i].running)
				updateWWANTimers[i].stop();
		}
/* USB-END */
		for (var uidx = 1; uidx <= nvram.mwan_num; uidx++) {
			u = (uidx > 1) ? uidx : '';
			if (nvram['wan'+u+'_status_script'] == 1) {
				if (customStatusTimers[uidx - 1].running)
					customStatusTimers[uidx - 1].stop();
			}
		}
	}
	else {
/* USB-BEGIN */
		for (var i = 0; i < updateWWANTimers.length; i++)
			updateWWANTimers[i].toggle();
/* USB-END */
		for (var uidx = 1; uidx <= nvram.mwan_num; uidx++) {
			u = (uidx > 1) ? uidx : '';
			if (nvram['wan'+u+'_status_script'] == 1)
				customStatusTimers[uidx - 1].toggle();
		}
	}
}

/* USB-BEGIN */
function foreach_wwan(functionToDo) {
	for (var uidx = 1; uidx <= nvram.mwan_num; uidx++) {
		var u = (uidx > 1) ? uidx : '';
		var wan_proto = nvram['wan'+u+'_proto'];
		var wan_hilink_ip = nvram['wan'+u+'_hilink_ip'];

		if (wan_proto == 'lte' || wan_proto == 'ppp3g' || (wan_hilink_ip && wan_hilink_ip != '0.0.0.0'))
			functionToDo(uidx);
	}
}
/* USB-END */

function c(id, htm) {
	E(id).cells[1].innerHTML = htm;
}

function ethstates() {
	var port = etherstates.port0;
	if (port == 'disabled')
		return 0;

	var state = [];
	var u, uidx, code = '', v = 0;
	var code ='<div class="section-title">Ethernet Ports State<\/div><div class="section"><table class="fields"><tr>';

	/* WANs */
	for (uidx = 1; uidx <= nvram.mwan_num; ++uidx) {
		u = (uidx > 1) ? uidx : '';

		if ((nvram['wan'+u+'_sta'] == '')
/* USB-BEGIN */
		    && (nvram['wan'+u+'_proto'] != 'lte') && (nvram['wan'+u+'_proto'] != 'ppp3g')
/* USB-END */
		) {
			code += '<td class="title indent2"><b>WAN'+(uidx - 1)+'<\/b><\/td>';
			++v;
		}
	}
	/* LANs */
	for (uidx = v; uidx <= MAX_PORT_ID; ++uidx)
		code += '<td class="title indent2"><b>LAN'+(v > 0 ? (uidx - 1) : uidx)+'<\/b><\/td>';

	code += '<td class="content"><\/td><\/tr><tr>';
	for (uidx = 0; uidx <= MAX_PORT_ID; ++uidx) {
		port = eval('etherstates.port'+uidx);

		state = _ethstates(port);

		code += '<td class="title indent2"><img id="'+state[0]+'_'+uidx+'" src="'+state[0]+'.gif" alt=""><br>'+(stats.lan_desc == '1' ? state[1] : '')+'<\/td>';
	}

	code += '<td class="content"><\/td><\/tr><tr><td class="title indent1" colspan="6" style="text-align:right">&raquo; <a href="basic-network.asp">Configure<\/a><\/td><\/tr><\/table><\/div>';
	E('ports').innerHTML = code;
}

function anon_update() {
	var code = '';

	if ((stats.anon_enable == '-1') || (stats.anon_answer == '0'))
		E('status-anonwarn').style.display = 'block';
	else
		E('status-anonwarn').style.display = 'none';

	var update = anonupdate.update;
	if (update == 'no' || update == '' || !update)
		return 0;

	var code = '<div class="section-title">!! Attention !!<\/div><div class="section-centered">New version of FreshTomato '+update+' is now available. <a class="new_window" href="https://freshtomato.org/">Click here to download<\/a>.<\/div>';
	E('status-nversion').style.display = 'block';
	E('status-nversion').innerHTML = code;
}

function show() {
	var uidx, u;

	visibility();
	ethstates();
	anon_update();

	c('cpu', stats.cpuload);
	c('cpupercent', stats.cpupercent);
	c('wlsense', stats.wlsense);
	c('temps', stats.cputemp + 'C / ' + Math.round(stats.cputemp.slice(0, -1) * 1.8 + 32) + '°F');
	c('uptime', stats.uptime);
	c('time', stats.time);
	c('memory', stats.memory);
	c('swap', stats.swap);
	elem.display('swap', stats.swap != '');
	c('nvram_stat', scaleSize(nvstat.size - nvstat.free)+' / '+scaleSize(nvstat.size)+' <small>('+((nvstat.size - nvstat.free) / nvstat.size * 100.0).toFixed(2)+'%)<\/small><div class="progress-wrapper"><div class="progress-container"><div class="progress-bar" style="background-color:'+setColor(((nvstat.size - nvstat.free) / nvstat.size * 100.0).toFixed(2))+';width:'+((nvstat.size - nvstat.free) / nvstat.size * 100.0).toFixed(2)+'%"><\/div><\/div><\/div>');
/* IPV6-BEGIN */
	c('ip6_duid', stats.ip6_duid);
	elem.display('ip6_duid', stats.ip6_duid != '');
	c('ip6_wan', stats.ip6_wan);
	elem.display('ip6_wan', stats.ip6_wan != '');
	c('ip6_wan_dns1', stats.ip6_wan_dns1);
	elem.display('ip6_wan_dns1', stats.ip6_wan_dns1 != '');
	c('ip6_wan_dns2', stats.ip6_wan_dns2);
	elem.display('ip6_wan_dns2', stats.ip6_wan_dns2 != '');
	c('ip6_lan', stats.ip6_lan);
	elem.display('ip6_lan', stats.ip6_lan != '');
	c('ip6_lan_ll', stats.ip6_lan_ll);
	elem.display('ip6_lan_ll', stats.ip6_lan_ll != '');
	c('ip6_lan1', stats.ip6_lan1);
	elem.display('ip6_lan1', stats.ip6_lan1 != '');
	c('ip6_lan1_ll', stats.ip6_lan1_ll);
	elem.display('ip6_lan1_ll', stats.ip6_lan1_ll != '');
	c('ip6_lan2', stats.ip6_lan2);
	elem.display('ip6_lan2', stats.ip6_lan2 != '');
	c('ip6_lan2_ll', stats.ip6_lan2_ll);
	elem.display('ip6_lan2_ll', stats.ip6_lan2_ll != '');
	c('ip6_lan3', stats.ip6_lan3);
	elem.display('ip6_lan3', stats.ip6_lan3 != '');
	c('ip6_lan3_ll', stats.ip6_lan3_ll);
	elem.display('ip6_lan3_ll', stats.ip6_lan3_ll != '');
/* IPV6-END */

	for (uidx = 1; uidx <= nvram.mwan_num; ++uidx) {
		u = (uidx > 1) ? uidx : '';

		if (nvram['wan'+u+'_proto'] == 'disabled') /* disabled? */
			elem.display('wan'+u+'-title', 'sesdiv_wan'+u, (nvram['wan'+u+'_proto'] != 'disabled'));

		c('wan'+u+'ip', stats.wanip[uidx - 1]);
		c('wan'+u+'netmask', stats.wannetmask[uidx - 1]);
		c('wan'+u+'gateway', stats.wangateway[uidx - 1]);
		c('wan'+u+'dns', stats.dns[uidx - 1]);
		c('wan'+u+'status', stats.wanstatus[uidx - 1]);
		c('wan'+u+'uptime', stats.wanuptime[uidx - 1]);

		elem.display('b'+u+'_dhcpc', show_dhcpc[uidx - 1]);
		E('b'+u+'_renew').disabled = 0;
		E('b'+u+'_release').disabled = 0;
		if (show_dhcpc[uidx - 1])
			c('wan'+u+'lease', stats.wanlease[uidx - 1]);

		elem.display('b'+u+'_codi', show_codi[uidx - 1]);
		if (show_codi[uidx - 1]) {
			E('b'+u+'_connect').disabled = (stats.wanup[uidx - 1] || (!stats.wanup[uidx - 1] && stats.wanstatus[uidx - 1].substring(3, 13) == 'Connecting'));
			E('b'+u+'_disconnect').disabled = (!stats.wanup[uidx - 1] && stats.wanstatus[uidx - 1].substring(3, 13) != 'Connecting');
		}

		elem.display('b'+u+'_multiwan', (nvram.mwan_num > 1 && nvram.mwan_cktime > 0 && stats.wanck_pause != 1));
		elem.display('wan'+u+'multiwan', (nvram.mwan_num > 1));
		c('wan'+u+'multiwan', '<div class="stats_mwan1">Weight '+stats.wanweight[uidx - 1]+'<\/div><div class="stats_mwan2">Watchdog '+(nvram.mwan_cktime > 0 && stats.wanck_pause != 1 ? 'enabled, check time: '+(nvram.mwan_cktime / 60)+' minute(s)' : '<b>disabled<\/b>')+'<\/div>');
	}

	for (var uidx = 0; uidx < wl_ifaces.length; ++uidx) {
		if (wl_sunit(uidx) < 0) {
			/* warn against unsecured wifi */
			if (nvram['wl'+wl_fface(uidx)+'_radio'] == '1' && wlstats[uidx].radio && nvram['wl'+wl_fface(uidx)+'_net_mode'] != 'disabled' && nvram['wl'+wl_fface(uidx)+'_security_mode'] == 'disabled')
				E('status-wifiwarn').style.display = 'block';
			else
				E('status-wifiwarn').style.display = 'none';

			c('radio'+uidx, (wlstats[uidx].radio ? 'Enabled' : '<b>Disabled<\/b>'));

			elem.display('b_wl'+uidx+'_enable', 'b_wl'+uidx+'_disable', show_radio[uidx]);
			if (show_radio[uidx]) {
				E('b_wl'+uidx+'_enable').disabled = wlstats[uidx].radio;
				E('b_wl'+uidx+'_disable').disabled = !wlstats[uidx].radio;
			}
			c('channel'+uidx, stats.channel[uidx]);
			if (nphy)
				c('nbw'+uidx, wlstats[uidx].nbw);

			c('rate'+uidx, wlstats[uidx].rate);
			elem.display('rate'+uidx, (show_radio[uidx] && wlstats[uidx].rate));

			c('interference'+uidx, stats.interference[uidx]);
			elem.display('interference'+uidx, (wlstats[uidx].radio && stats.interference[uidx] != ''));

			if (wlstats[uidx].client) {
				c('rssi'+uidx, wlstats[uidx].rssi || '');
				c('noise'+uidx, wlstats[uidx].noise || '');
				c('qual'+uidx, stats.qual[uidx] || '');
			}
		}
		else {
				/* do not display any virtual interface linked to the chip/frequency that is disabled */
				if (!wlstats[uidx].radio) /* disabled? */
					elem.display('wl'+wl_fface(uidx)+'-title', 'sesdiv_wl_'+wl_fface(uidx), wlstats[uidx].radio);
		}
		c('ifstatus'+uidx, wlstats[uidx].ifstatus || '');
	}
}

function earlyInit() {
	show();
}

function init() {
	var c, u;
	if (((c = cookie.get(cprefix+'_system_vis')) != null) && (c != '1'))
		toggleVisibility(cprefix, 'system');

	for (var uidx = 1; uidx <= nvram.mwan_num; ++uidx) {
		u = (uidx > 1) ? uidx : '';
		if (((c = cookie.get(cprefix+'_wan'+u+'_vis')) != null) && (c != '1'))
			toggleVisibility(cprefix, 'wan'+u);
	}

	if (((c = cookie.get(cprefix+'_lan_vis')) != null) && (c != '1'))
		toggleVisibility(cprefix, 'lan');

	for (var uidx = 0; uidx < wl_ifaces.length; ++uidx) {
		u = wl_fface(uidx);
		if (((c = cookie.get(cprefix+'_wl_'+u+'_vis')) != null) && (c != '1'))
			toggleVisibility(cprefix, 'wl_'+u);
	}
/* USB-BEGIN */
	foreach_wwan(function(i) {
		if (((c = cookie.get(cprefix+'_wwan'+i+'_vis')) != null) && (c != '1'))
			toggleVisibility(cprefix, 'wwan'+i);

		E('WWANStatus'+i+'_overall').style.display = 'block';
		updateWWANTimers[i - 1].initPage(3000, 30);
	});
/* USB-END */
	for (var uidx = 1; uidx <= nvram.mwan_num; uidx++) {
		if (!customStatusTimers[uidx - 1])
			continue;

		customStatusTimers[uidx - 1].initPage(1000, 15);
	}

	ref.initPage(1000, 5);

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

<input type="hidden" name="_nextpage" value="status-overview.asp">
<input type="hidden" name="_service" value="">
<input type="hidden" name="_nextwait" value="5">
<input type="hidden" name="_sleep" value="5">
<input type="hidden" name="exec" value="">
<input type="hidden" name="prefix" value="">
<input type="hidden" name="enable" value="">
<input type="hidden" name="_wl_unit" value="">

<!-- / / / -->

<div style="display:none" id="status-wifiwarn">
	<div class="section-title">!! Warning: Wifi Security Disabled !!</div>
	<div class="section-centered"> The Wifi Radios are <b>Enabled</b> without having a <b>Wifi Password</b> set.
	<br><b>Please make sure to <a href="basic-network.asp">Set a Wifi Password</a></b></div>
</div>

<div style="display:none" id="status-nversion"></div>

<div style="display:none" id="status-anonwarn">
	<div class="section-title">!! Attention !!</div>
	<div class="section-centered">You did not configure <b>TomatoAnon project</b> setting.
	<br>Please go to <a href="admin-tomatoanon.asp">TomatoAnon configuration page</a> and make a choice.</div>
</div>

<!-- / / / -->

<div class="section-title" id="sesdiv_system-title">System <small><i><a href="javascript:toggleVisibility(cprefix,'system');"><span id="sesdiv_system_showhide">(Hide)</span></a></i></small></div>
<div class="section" id="sesdiv_system">
<script>
	createFieldTable('', [
		{ title: 'Name', text: nvram.router_name },
		{ title: 'Model', text: nvram.t_model_name },
		{ title: 'Bootloader (CFE)', text: stats.cfeversion },
		{ title: 'Chipset', text: stats.systemtype },
		{ title: 'CPU Frequency', text: stats.cpumhz, suffix: ' <small>(dual-core)<\/small>' },
		{ title: 'Flash Size', text: stats.flashsize },
		null,
		{ title: 'Time', rid: 'time', text: stats.time },
		{ title: 'Uptime', rid: 'uptime', text: stats.uptime },
		{ title: 'CPU Load <small>(1 / 5 / 15 mins)<\/small>', rid: 'cpu', text: stats.cpuload },
		{ title: 'CPU Usage', rid: 'cpupercent', text: stats.cpupercent },
		{ title: 'Used / Total RAM', rid: 'memory', text: stats.memory },
		{ title: 'Used / Total Swap', rid: 'swap', text: stats.swap, hidden: (stats.swap == '') },
		{ title: 'Used / Total NVRAM', rid: 'nvram_stat', text: scaleSize(nvstat.size - nvstat.free)+' / '+scaleSize(nvstat.size)+' <small>('+((nvstat.size - nvstat.free) / nvstat.size * 100.0).toFixed(2)+'%)<\/small><div class="progress-wrapper"><div class="progress-container"><div class="progress-bar" style="background-color:'+setColor(((nvstat.size - nvstat.free) / nvstat.size * 100.0).toFixed(2))+';width:'+((nvstat.size - nvstat.free) / nvstat.size * 100.0).toFixed(2)+'%"><\/div><\/div><\/div>' },
		null,
		{ title: 'CPU Temperature', rid: 'temps', text: stats.cputemp + 'C / ' + Math.round(stats.cputemp.slice(0, -1) * 1.8 + 32) + '°F' },
		{ title: 'Wireless Temperature', rid: 'wlsense', text: stats.wlsense }
	]);
</script>
</div>

<!-- / / / -->

<div id="ports"></div>

<!-- / / / -->

<script>
/* USB-BEGIN */
	foreach_wwan(function(i) {
		W('<div id="WWANStatus'+i+'_overall" style="display:none;">');
		W('<div class="section-title" id="wwan'+i+'-title">WWAN'+(updateWWANTimers > 1 ? i : '')+' Modem Status <small><i><a href="javascript:toggleVisibility(cprefix,\'wwan'+i+'\');"><span id="sesdiv_wwan'+i+'_showhide">(Hide)<\/span><\/a><\/i><\/small><\/div>');
		W('<div class="section" id="sesdiv_wwan'+i+'">');
		W('<div id="WWANStatus'+i+'">');
		W('<div class="fields">Please wait... Initial refresh... &nbsp; <img src="spin.gif" alt="" style="vertical-align:middle"><\/div>');
		W('<\/div><\/div><\/div>');
	});
/* USB-END */
	for (var uidx = 1; uidx <= nvram.mwan_num; ++uidx) {
		u = (uidx > 1) ? uidx : '';
		W('<div class="section-title" id="wan'+u+'-title">WAN'+(uidx - 1)+' <small><i><a href="javascript:toggleVisibility(cprefix,\'wan'+u+'\');"><span id="sesdiv_wan'+u+'_showhide">(Hide)<\/span><\/a><\/i><\/small><\/div>');
		W('<div class="section" id="sesdiv_wan'+u+'">');
		createFieldTable('', [
			{ title: 'MAC Address', text: nvram['wan'+u+'_hwaddr'] },
			{ title: 'Connection Type', text: { 'dhcp':'DHCP','static':'Static IP','pppoe':'PPPoE','pptp':'PPTP','l2tp':'L2TP'
/* USB-BEGIN */
			                                    ,'ppp3g':'3G Modem','lte':'4G/LTE'
/* USB-END */
			} [nvram['wan'+u+'_proto']] || '-' },
			{ title: 'IP Address', rid: 'wan'+u+'ip', text: stats.wanip[uidx - 1] },
			{ title: 'Subnet Mask', rid: 'wan'+u+'netmask', text: stats.wannetmask[uidx - 1] },
			{ title: 'Gateway', rid: 'wan'+u+'gateway', text: stats.wangateway[uidx - 1] },
/* IPV6-BEGIN */
			{ title: 'IPv6 DUID', rid: 'ip6_duid', text: stats.ip6_duid, hidden: (stats.ip6_duid == '') },
			{ title: 'IPv6 Address', rid: 'ip6_wan', text: stats.ip6_wan, hidden: (stats.ip6_wan == '') },
			{ title: 'IPv6 DNS1', rid: 'ip6_wan_dns1', text: stats.ip6_wan_dns1, hidden: (stats.ip6_wan_dns1 == '') },
			{ title: 'IPv6 DNS2', rid: 'ip6_wan_dns2', text: stats.ip6_wan_dns2, hidden: (stats.ip6_wan_dns2 == '') },
/* IPV6-END */
			{ title: 'DNS', rid: 'wan'+u+'dns', text: stats.dns[uidx - 1] },
			{ title: 'MTU', text: nvram['wan'+u+'_run_mtu'] },
			null,
			{ title: 'Status', rid: 'wan'+u+'status', text: stats.wanstatus[uidx - 1] },
			{ title: 'MultiWAN Status', rid: 'wan'+u+'multiwan', text: stats.wanweight[uidx - 1] },
			{ title: 'Connection Uptime', rid: 'wan'+u+'uptime', text: stats.wanuptime[uidx - 1] },
			{ title: 'Remaining Lease Time', rid: 'wan'+u+'lease', text: stats.wanlease[uidx - 1], ignore: !show_dhcpc[uidx - 1] }
/* USB-BEGIN */
			, { text: 'Please wait... Initial refresh... &nbsp; <img src="spin.gif" alt="" style="vertical-align:middle">', rid: "WanCustomStatus"+u, ignore: !customStatusTimers[uidx - 1] }
/* USB-END */
		]);
		W('<span id="b'+u+'_dhcpc" style="display:none">');
		W('<input type="button" class="status-controls" onclick="dhcpc(\'renew\',\''+u+'\')" value="Renew" id="b'+u+'_renew"> &nbsp;');
		W('<input type="button" class="status-controls" onclick="dhcpc(\'release\',\''+u+'\')" value="Release" id="b'+u+'_release"> &nbsp;');
		W('<\/span>');
		W('<span id="b'+u+'_codi" style="display:none">');
		W('<input type="button" class="status-controls" onclick="wan_connect('+uidx+')" value="Connect" id="b'+u+'_connect"> &nbsp;');
		W('<input type="button" class="status-controls" onclick="wan_disconnect('+uidx+')" value="Disconnect" id="b'+u+'_disconnect"> &nbsp;');
		W('<\/span>');
		W('<span id="b'+u+'_multiwan" style="display:none">');
		W('<input type="button" class="status-controls" onclick="watchdog_check(\''+u+'\')" value="Force Check" id="b'+u+'_check" title="Force MultiWAN availability check">');
		W('<\/span>');
		W('<\/div>');
	}
</script>

<!-- / / / -->

<div class="section-title" id="sesdiv_lan-title">LAN <small><i><a href="javascript:toggleVisibility(cprefix,'lan');"><span id="sesdiv_lan_showhide">(Hide)</span></a></i></small></div>
<div class="section" id="sesdiv_lan">
<script>
	var s = '';
	var t = '';
	for (var i = 0 ; i <= MAX_BRIDGE_ID ; i++) {
		var j = (i == 0) ? '' : i.toString();
		if (nvram['lan'+j+'_ifname'].length > 0) {
			if (nvram['lan'+j+'_proto'] == 'dhcp') {
				if ((!fixIP(nvram.dhcpd_startip)) || (!fixIP(nvram.dhcpd_endip))) {
					var x = nvram['lan'+j+'_ipaddr'].split('.').splice(0, 3).join('.')+'.';
					nvram['dhcpd'+j+'_startip'] = x + 2;
					nvram['dhcpd'+j+'_endip'] = x + 50;
				}
				s += ((s.length > 0) && (s.charAt(s.length - 1) != ' ')) ? '<br>' : '';
				s += '<b>br'+i+'<\/b> (LAN'+i+') - '+nvram['dhcpd'+j+'_startip']+' - '+nvram['dhcpd'+j+'_endip'];
			}
			else {
				s += ((s.length > 0) && (s.charAt(s.length - 1) != ' ')) ? '<br>' : '';
				s += '<b>br'+i+'<\/b> (LAN'+i+') - Disabled';
			}
			t += ((t.length > 0) && (t.charAt(t.length - 1) != ' ')) ? '<br>' : '';
			t += '<b>br'+i+'<\/b> (LAN'+i+') - '+nvram['lan'+j+'_ipaddr']+'/'+numberOfBitsOnNetMask(nvram['lan'+j+'_netmask']);
		}
	}

	createFieldTable('', [
		{ title: 'Router MAC Address', text: nvram.lan_hwaddr },
		{ title: 'Router IP Addresses', text: t },
		{ title: 'Gateway', text: nvram.lan_gateway, ignore: nvram.wan_proto != 'disabled' },
/* IPV6-BEGIN */
		{ title: 'LAN (br0) IPv6 Address', rid: 'ip6_lan', text: stats.ip6_lan, hidden: (stats.ip6_lan == '') },
		{ title: 'LAN (br0) IPv6 LL Address', rid: 'ip6_lan_ll', text: stats.ip6_lan_ll, hidden: (stats.ip6_lan_ll == '') },
		{ title: 'LAN1 (br1) IPv6 Address', rid: 'ip6_lan1', text: stats.ip6_lan1, hidden: (stats.ip6_lan1 == '') },
		{ title: 'LAN1 (br1) IPv6 LL Address', rid: 'ip6_lan1_ll', text: stats.ip6_lan1_ll, hidden: (stats.ip6_lan1_ll == '') },
		{ title: 'LAN2 (br2) IPv6 Address', rid: 'ip6_lan2', text: stats.ip6_lan2, hidden: (stats.ip6_lan2 == '') },
		{ title: 'LAN2 (br2) IPv6 LL Address', rid: 'ip6_lan2_ll', text: stats.ip6_lan2_ll, hidden: (stats.ip6_lan2_ll == '') },
		{ title: 'LAN3 (br3) IPv6 Address', rid: 'ip6_lan3', text: stats.ip6_lan3, hidden: (stats.ip6_lan3 == '') },
		{ title: 'LAN3 (br3) IPv6 LL Address', rid: 'ip6_lan3_ll', text: stats.ip6_lan3_ll, hidden: (stats.ip6_lan3_ll == '') },
/* IPV6-END */
		{ title: 'DNS', rid: 'dns', text: nvram.wan_dns, ignore: nvram.wan_proto != 'disabled' },
		{ title: 'DHCP', text: s }
	]);
</script>
</div>

<!-- / / / -->

<script>
	for (var uidx = 0; uidx < wl_ifaces.length; ++uidx) {
		u = wl_fface(uidx);
		W('<div class="section-title" id="wl'+u+'-title">Wireless');
		if (wl_ifaces.length > 0)
			W(' '+wl_display_ifname(uidx));

		W(' <small><i><a href="javascript:toggleVisibility(cprefix,\'wl_'+u+'\');"><span id="sesdiv_wl_'+u+'_showhide">(Hide)<\/span><\/a><\/i><\/small>');
		W('<\/div>');
		W('<div class="section" id="sesdiv_wl_'+u+'">');
		var sec = auth[nvram['wl'+u+'_security_mode']]+'';
		if (sec.indexOf('WPA') != -1)
			sec += ' + '+enc[nvram['wl'+u+'_crypto']];

		var wmode = wmo[nvram['wl'+u+'_mode']]+'';
		if ((nvram['wl'+u+'_mode'] == 'ap') && (nvram['wl'+u+'_wds_enable'] * 1))
			wmode += ' + WDS';

		createFieldTable('', [
			{ title: 'MAC Address', text: nvram['wl'+u+'_hwaddr'] },
			{ title: 'Wireless Mode', text: wmode },
			{ title: 'Wireless Network Mode', text: bgmo[nvram['wl'+u+'_net_mode']], ignore: (wl_sunit(uidx) >= 0) },
			{ title: 'Interface Status', rid: 'ifstatus'+uidx, text: wlstats[uidx].ifstatus },
			{ title: 'Radio', rid: 'radio'+uidx, text: (wlstats[uidx].radio == 0) ? '<b>Disabled<\/b>' : 'Enabled', ignore: (wl_sunit(uidx) >= 0) },
			{ title: 'SSID', text: nvram['wl'+u+'_ssid'] },
			{ title: 'Broadcast', text: (nvram['wl'+u+'_closed'] == 0) ? 'Enabled' : '<b>Disabled<\/b>', ignore: (nvram['wl'+u+'_mode'] != 'ap') },
			{ title: 'Security', text: sec },
			{ title: 'Channel', rid: 'channel'+uidx, text: stats.channel[uidx], ignore: (wl_sunit(uidx) >= 0) },
			{ title: 'Channel Width', rid: 'nbw'+uidx, text: wlstats[uidx].nbw, ignore: ((!nphy) || (wl_sunit(uidx) >= 0)) },
			{ title: 'Interference Level', rid: 'interference'+uidx, text: stats.interference[uidx], ignore: (wl_sunit(uidx) >= 0) },
			{ title: 'Rate', rid: 'rate'+uidx, text: wlstats[uidx].rate, ignore: (wl_sunit(uidx) >= 0) },
/* QRCODE-BEGIN */
			{ title: ' ', rid: 'qr-code'+uidx, text: '<a href="tools-qr.asp?wl='+wl_unit(uidx)+(wl_sunit(uidx) >= 0 ? '.'+wl_sunit(uidx) : '')+'">Show QR code<\/a>' },
/* QRCODE-END */
			{ title: 'RSSI', rid: 'rssi'+uidx, text: wlstats[uidx].rssi || '', ignore: ((!wlstats[uidx].client) || (wl_sunit(uidx) >= 0)) },
			{ title: 'Noise', rid: 'noise'+uidx, text: wlstats[uidx].noise || '', ignore: ((!wlstats[uidx].client) || (wl_sunit(uidx) >= 0)) },
			{ title: 'Signal Quality', rid: 'qual'+uidx, text: stats.qual[uidx] || '', ignore: ((!wlstats[uidx].client) || (wl_sunit(uidx) >= 0)) }
		]);

		W('<input type="button" class="status-controls" onclick="wlenable('+uidx+', 1)" id="b_wl'+uidx+'_enable" title="Enable temporary this WL (up to the router\'s reboot)" value="Enable" style="display:none">');
		W('<input type="button" class="status-controls" onclick="wlenable('+uidx+', 0)" id="b_wl'+uidx+'_disable" title="Disable temporary this WL (up to the router\'s reboot)" value="Disable" style="display:none">');
		W('<\/div>');
	}
</script>

<!-- / / / -->

<div id="footer">
	<script>genStdRefresh(1,0,'onRefToggle()');</script>
</div>

</td></tr>
</table>
<script>earlyInit();</script>
</form>
</body>
</html>
