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
<title>[<% ident(); %>] Basic: IPv6</title>
<link rel="stylesheet" type="text/css" href="tomato.css?rel=<% version(); %>">
<% css(); %>
<script src="isup.jsx?_http_id=<% nv(http_id); %>"></script>
<script src="tomato.js?rel=<% version(); %>"></script>

<script>

//	<% nvram("ipv6_6rd_prefix_length,ipv6_prefix,ipv6_prefix_length,ipv6_radvd,ipv6_dhcpd,ipv6_accept_ra,ipv6_isp_opt,ipv6_pdonly,ipv6_pd_norelease,ipv6_rtr_addr,ipv6_service,ipv6_debug,ipv6_duid_type,ipv6_dns,ipv6_tun_addr,ipv6_tun_addrlen,ipv6_ifname,ipv6_tun_v4end,ipv6_relay,ipv6_tun_mtu,ipv6_tun_ttl,ipv6_6rd_ipv4masklen,ipv6_6rd_prefix,ipv6_6rd_borderrelay,lan_ifname,ipv6_vlan,ipv6_prefix_len_wan,ipv6_isp_gw,ipv6_wan_addr"); %>

function show() {
	elem.setInnerHTML('notice_container', '<div id="notice">'+isup.notice_ip6tables.replace(/\n/g, '<br>')+'<\/div><br style="clear:both">');
	elem.display('notice_container', isup.notice_ip6tables != '');
}

nvram.ipv6_accept_ra = fixInt(nvram.ipv6_accept_ra, 0, 3, 0);

function verifyFields(focused, quiet) {
	var i;
	var ok = 1;
	var a, b, c;
	var prefixLen = parseInt(E('_f_ipv6_prefix_length').value) || 64;
	var availableNetworks = (prefixLen >= 64) ? 1 : Math.pow(2, 64 - prefixLen);

	/* --- visibility --- */

	var vis = {
		_ipv6_service: 1,
/* RTNPLUS-BEGIN */
		_f_ipv6_debug: 0,
/* RTNPLUS-END */
		_f_ipv6_duid_type: 0,
		_f_ipv6_prefix: 1,
		_f_ipv6_prefix_length: 1,
		_f_ipv6_wan_addr: 0,
		_f_ipv6_prefix_len_wan: 0,
		_f_ipv6_isp_gw: 0,
		_f_ipv6_rtr_addr_auto: 1,
		_f_ipv6_rtr_addr: 1,
		_f_ipv6_dns_1: 1,
		_f_ipv6_dns_2: 1,
		_f_ipv6_dns_3: 1,
		_f_ipv6_accept_ra_wan: 1,
		_f_ipv6_accept_ra_lan: 1,
		_f_ipv6_isp_opt: 1,
		_f_ipv6_pdonly: 1,
		_f_ipv6_pd_norelease: 0,
		_ipv6_tun_v4end: 1,
		_ipv6_relay: 1,
		_ipv6_ifname: 1,
		_ipv6_tun_addr: 1,
		_ipv6_tun_addrlen: 1,
		_ipv6_tun_ttl: 1,
		_ipv6_tun_mtu: 1,
		_ipv6_6rd_ipv4masklen: 1,
		_ipv6_6rd_prefix_length: 1,
		_ipv6_6rd_prefix: 1,
		_ipv6_6rd_borderrelay: 1
	};
	for (i = 1; i <= MAX_BRIDGE_ID; i++) {
		vis['_f_lan' + i + '_ipv6'] = 0;
	}

	c = E('_ipv6_service').value;
	switch(c) {
		case '':
			vis._ipv6_ifname = 0;
			vis._f_ipv6_rtr_addr_auto = 0;
			vis._f_ipv6_rtr_addr = 0;
			vis._f_ipv6_dns_1 = 0;
			vis._f_ipv6_dns_2 = 0;
			vis._f_ipv6_dns_3 = 0;
			vis._f_ipv6_accept_ra_wan = 0;
			vis._f_ipv6_accept_ra_lan = 0;
			vis._f_ipv6_isp_opt = 0;
			vis._f_ipv6_pdonly = 0;
			/* fall through */
		case 'other':
			vis._ipv6_6rd_ipv4masklen = 0;
			vis._ipv6_6rd_prefix_length = 0;
			vis._ipv6_6rd_prefix = 0;
			vis._ipv6_6rd_borderrelay = 0;
			vis._f_ipv6_prefix = 0;
			vis._f_ipv6_prefix_length = 0;
			vis._ipv6_tun_v4end = 0;
			vis._ipv6_relay = 0;
			vis._ipv6_tun_addr = 0;
			vis._ipv6_tun_addrlen = 0;
			vis._ipv6_tun_ttl = 0;
			vis._ipv6_tun_mtu = 0;
			vis._f_ipv6_isp_opt = 0;
			vis._f_ipv6_pdonly = 0;
			if (c == 'other') {
				E('_f_ipv6_rtr_addr_auto').value = 1;
				vis._f_ipv6_rtr_addr_auto = 2;
			}
		break;
		case '6rd':
			vis._f_ipv6_prefix = 0;
			vis._ipv6_tun_v4end = 0;
			vis._ipv6_relay = 0;
			vis._ipv6_tun_addr = 0;
			vis._ipv6_tun_addrlen = 0;
			vis._ipv6_tun_mtu = 0;
			vis._ipv6_ifname = 0;
			vis._ipv6_relay = 0;
			vis._f_ipv6_accept_ra_wan = 0;
			vis._f_ipv6_accept_ra_lan = 0;
			vis._f_ipv6_rtr_addr_auto = 0;
			vis._f_ipv6_rtr_addr = 0;
			vis._f_ipv6_prefix_length = 0;
			vis._f_ipv6_isp_opt = 0;
			vis._f_ipv6_pdonly = 0;
		break;
		case 'native-pd':
			t_fom.f_ipv6_accept_ra_wan.checked = true; /* must be enabled always for DHCPv6 with PD */
/* RTNPLUS-BEGIN */
			vis._f_ipv6_debug = 1;
/* RTNPLUS-END */
			vis._f_ipv6_duid_type = 1;
			vis._f_ipv6_pd_norelease = 1;
		case '6rd-pd':
			vis._f_ipv6_prefix = 0;
			vis._f_ipv6_rtr_addr_auto = 0;
			vis._f_ipv6_rtr_addr = 0;
			if (c == '6rd-pd') {
				vis._f_ipv6_prefix_length = 0;
				vis._f_ipv6_accept_ra_lan = 0;
				vis._f_ipv6_accept_ra_wan = 0;
				vis._f_ipv6_isp_opt = 0;
				vis._f_ipv6_pdonly = 0;
			}
			/* fall through */
		case 'native':
			vis._ipv6_ifname = 0;
			vis._ipv6_tun_v4end = 0;
			vis._ipv6_relay = 0;
			vis._ipv6_tun_addr = 0;
			vis._ipv6_tun_addrlen = 0;
			if (c == '6rd-pd')
				vis._ipv6_tun_ttl = 1;
			else
				vis._ipv6_tun_ttl = 0;
			vis._ipv6_tun_mtu = 0;
			vis._ipv6_6rd_ipv4masklen = 0;
			vis._ipv6_6rd_prefix_length = 0;
			vis._ipv6_6rd_prefix = 0;
			vis._ipv6_6rd_borderrelay = 0;
			if (c == 'native-pd') {
				for (i = 1; i <= MAX_BRIDGE_ID; i++) {
					if (nvram['lan' + i + '_ifname'] == 'br' + i && availableNetworks > i) {
						vis['_f_lan' + i + '_ipv6'] = 1;
					}
				}
			}
			if (c == 'native') {
				vis._f_ipv6_pdonly         = 0;
				vis._f_ipv6_wan_addr       = 1;
				vis._f_ipv6_prefix_len_wan = 1;
				vis._f_ipv6_isp_gw         = 1;
				vis._f_ipv6_accept_ra_wan  = 0;
				vis._f_ipv6_accept_ra_lan  = 0;
				vis._f_ipv6_isp_opt        = 0;
			}
		break;
		case '6to4':
			vis._ipv6_ifname = 0;
			vis._f_ipv6_prefix = 0;
			vis._f_ipv6_rtr_addr_auto = 0;
			vis._f_ipv6_rtr_addr = 0;
			vis._ipv6_tun_v4end = 0;
			vis._ipv6_tun_addr = 0;
			vis._ipv6_tun_addrlen = 0;
			vis._f_ipv6_accept_ra_wan = 0;
			vis._f_ipv6_accept_ra_lan = 0;
			vis._f_ipv6_isp_opt = 0;
			vis._f_ipv6_pdonly = 0;
			vis._ipv6_6rd_ipv4masklen = 0;
			vis._ipv6_6rd_prefix_length = 0;
			vis._ipv6_6rd_prefix = 0;
			vis._ipv6_6rd_borderrelay = 0;
		break;
		case 'sit':
			vis._ipv6_ifname = 0;
			vis._ipv6_relay = 0;
			vis._f_ipv6_accept_ra_wan = 0;
			vis._f_ipv6_accept_ra_lan = 0;
			vis._f_ipv6_isp_opt = 0;
			vis._f_ipv6_pdonly = 0;
			vis._ipv6_6rd_ipv4masklen = 0;
			vis._ipv6_6rd_prefix_length = 0;
			vis._ipv6_6rd_prefix = 0;
			vis._ipv6_6rd_borderrelay = 0;
		break;
	}

	if (vis._f_ipv6_rtr_addr_auto && E('_f_ipv6_rtr_addr_auto').value == 0) {
		vis._f_ipv6_rtr_addr = 2;
	}

	for (a in vis) {
		b = E(a);
		c = vis[a];
		b.disabled = (c != 1);
		PR(b).style.display = (c ? 'block' : 'none');
	}

	/* --- verify --- */

	for (i = 1; i <= MAX_BRIDGE_ID; i++) {
		var elem = E('_f_lan' + i + '_ipv6');
		if (availableNetworks <= i) {
			elem.checked = false;
			elem.disabled = true;
		}
		else {
			elem.disabled = false;
		}
	}

	/* check if ipv6_radvd or ipv6_dhcpd is enabled for RA (dnsmasq); If YES, then disable Accept RA from LAN option */
	if (nvram.ipv6_radvd == '1' || nvram.ipv6_dhcpd == '1') {
		E('_f_ipv6_accept_ra_lan').checked = false;
		E('_f_ipv6_accept_ra_lan').disabled = true;
	}

	if (vis._ipv6_ifname == 1) {
		if (E('_ipv6_service').value != 'other') {
			if (!v_length('_ipv6_ifname', quiet || !ok, 2)) ok = 0;
		}
		else ferror.clear('_ipv6_ifname');
	}

/* REMOVE-BEGIN
	// Length
	a = [['_ipv6_ifname', 2]];
	for (i = a.length - 1; i >= 0; --i) {
		v = a[i];
		if ((vis[v[0]]) && (!v_length(v[0], quiet || !ok, v[1]))) ok = 0;
	}
REMOVE-END */

	/* IP address */
	a = ['_ipv6_tun_v4end'];
	for (i = a.length - 1; i >= 0; --i)
		if ((vis[a[i]]) && (!v_ip(a[i], quiet || !ok))) ok = 0;

	a = ['_ipv6_6rd_borderrelay'];
	for (i = a.length - 1; i >= 0; --i)
		if ((vis[a[i]]) && (!v_ip(a[i], quiet || !ok))) ok = 0;

	/* range */
	a = [ ['_f_ipv6_prefix_length', 3, 127], ['_f_ipv6_prefix_len_wan', 3, 127], ['_ipv6_tun_addrlen', 3, 127], ['_ipv6_tun_ttl', 0, 255], ['_ipv6_relay', 1, 254]];
	for (i = a.length - 1; i >= 0; --i) {
		b = a[i];
		if ((vis[b[0]]) && (!v_range(b[0], quiet || !ok, b[1], b[2]))) ok = 0;
	}

	/* mtu */
	b = '_ipv6_tun_mtu';
	if (vis[b]) {
		if ((!v_range(b, 1, 0, 0)) && (!v_range(b, quiet || !ok, 1280, 1480)))
			ok = 0;
		else
			ferror.clear(E(b));
	}

	/* IPv6 prefix */
	b = '_f_ipv6_prefix';
	c = vis._f_ipv6_accept_ra_wan && (E('_f_ipv6_accept_ra_wan').checked || E('_f_ipv6_accept_ra_lan').checked);
	if (vis[b] && (E(b).value.length > 0 || (!c))) {
		if (!v_ipv6_addr(b, quiet || !ok)) ok = 0;
	}
	else ferror.clear(b);

	/* IPv6 address */
	a = ['_ipv6_tun_addr'];
	for (i = a.length - 1; i >= 0; --i)
		if ((vis[a[i]]) && (!v_ipv6_addr(a[i], quiet || !ok))) ok = 0;

	if (vis._f_ipv6_rtr_addr == 2) {
		b = E('_f_ipv6_prefix');
		ip = (b.value.length > 0) ? ZeroIPv6PrefixBits(b.value, E('_f_ipv6_prefix_length').value) : '';
		b.value = CompressIPv6Address(ip);
		E('_f_ipv6_rtr_addr').value = (ip.length > 0) ? CompressIPv6Address(ip + '1') : '';
	}

	/* optional IPv6 address */
	a = ['_f_ipv6_rtr_addr', '_f_ipv6_wan_addr', '_f_ipv6_isp_gw', '_f_ipv6_dns_1', '_f_ipv6_dns_2', '_f_ipv6_dns_3'];
	for (i = a.length - 1; i >= 0; --i)
		if ((vis[a[i]]==1) && (E(a[i]).value.length > 0) && (!v_ipv6_addr(a[i], quiet || !ok))) ok = 0;

	return ok;
}

function earlyInit() {
	verifyFields(null, 1);
	insOvl();
}

function joinIPv6Addr(a) {
	var r, i, s;

	r = [];
	for (i = 0; i < a.length; ++i) {
		s = CompressIPv6Address(a[i]);
		if ((s) && (s != '')) r.push(s);
	}

	return r.join(' ');
}

function save() {
	var a, b, c;
	var i;

	if (!verifyFields(null, false)) return;

	var fom = E('t_fom');

	fom.ipv6_dns.value = joinIPv6Addr([fom.f_ipv6_dns_1.value, fom.f_ipv6_dns_2.value, fom.f_ipv6_dns_3.value]);
	fom.ipv6_isp_opt.value = fom.f_ipv6_isp_opt.checked ? 1 : 0;
	fom.ipv6_pdonly.value = fom.f_ipv6_pdonly.checked ? 1 : 0;
	fom.ipv6_accept_ra.value = 0;
	if (fom.f_ipv6_accept_ra_wan.checked && !fom.f_ipv6_accept_ra_wan.disabled) {
		fom.ipv6_accept_ra.value = fom.ipv6_accept_ra.value | 0x01; /* set bit 0, accept_ra enabled for WAN */
	}
	if (fom.f_ipv6_accept_ra_lan.checked && !fom.f_ipv6_accept_ra_lan.disabled) {
		fom.ipv6_accept_ra.value = fom.ipv6_accept_ra.value | 0x02; /* set bit 1, accept_ra enabled for LAN (br0...brX if available) */
	}

	fom.ipv6_prefix_length.value  = fom.f_ipv6_prefix_length.value;
	fom.ipv6_prefix.value         = fom.f_ipv6_prefix.value;
	fom.ipv6_vlan.value           = 0;
	fom.ipv6_wan_addr.value       = fom.f_ipv6_wan_addr.value;
	fom.ipv6_prefix_len_wan.value = fom.f_ipv6_prefix_len_wan.value;
	fom.ipv6_isp_gw.value         = fom.f_ipv6_isp_gw.value;
/* RTNPLUS-BEGIN */
	fom.ipv6_debug.value = fom.f_ipv6_debug.checked ? 1 : 0;
/* RTNPLUS-END */
	fom.ipv6_duid_type.value = fom.f_ipv6_duid_type.value;
	fom.ipv6_pd_norelease.value = fom.f_ipv6_pd_norelease.checked ? 1 : 0;

	switch(E('_ipv6_service').value) {
		case 'other':
			fom.ipv6_prefix_length.value = 64;
			fom.ipv6_prefix.value = '';
			fom.ipv6_rtr_addr.value = fom.f_ipv6_rtr_addr.value;
		break;
		case '6rd':
		break; /* KDB todo */
		case '6to4':
			fom.ipv6_prefix.value = '';
			fom.ipv6_rtr_addr.value = '';
		break;
		case 'native-pd':
			fom.ipv6_prefix.value = '';
			fom.ipv6_rtr_addr.value = '';
			for (i = 1; i <= MAX_BRIDGE_ID; i++) {
				var elem = fom['f_lan' + i + '_ipv6'];
				if (elem.checked) {
					fom.ipv6_vlan.value = fom.ipv6_vlan.value | (1 << (i - 1));
				}
			}
		break;
		case 'native':
			fom.ipv6_wan_addr.value       = fom.f_ipv6_wan_addr.value;
			fom.ipv6_prefix_len_wan.value = fom.f_ipv6_prefix_len_wan.value;
			fom.ipv6_isp_gw.value         = fom.f_ipv6_isp_gw.value;
			fom.ipv6_rtr_addr.value       = fom.f_ipv6_rtr_addr.value;
		break;
		default:
			fom.ipv6_rtr_addr.disabled = fom.f_ipv6_rtr_addr_auto.disabled;
			if (fom.f_ipv6_rtr_addr_auto.value == 1)
				fom.ipv6_rtr_addr.value = fom.f_ipv6_rtr_addr.value;
			else
				fom.ipv6_rtr_addr.value = '';
		break;
	}

	form.submit(fom, 1);
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
	<div class="title"><a href="/">Tomato64</a></div>
	<div class="version">Version <% version(); %> on <% nv("t_model_name"); %><span class="blinking bl2"><script><% anonupdate(); %> anon_update()</script>&nbsp;</span></div>
</td></tr>
<tr id="body"><td id="navi"><script>navi()</script></td>
<td id="content">
<div id="ident"><% ident(); %> | <script>wikiLink();</script></div>

<!-- / / / -->

<input type="hidden" name="_nextpage" value="basic-ipv6.asp">
<input type="hidden" name="_nextwait" value="10">
<input type="hidden" name="_service" value="*">
<!-- RTNPLUS-BEGIN -->
<input type="hidden" name="ipv6_debug">
<!-- RTNPLUS-END -->
<input type="hidden" name="ipv6_duid_type">
<input type="hidden" name="ipv6_dns">
<input type="hidden" name="ipv6_prefix">
<input type="hidden" name="ipv6_prefix_length">
<input type="hidden" name="ipv6_rtr_addr">
<input type="hidden" name="ipv6_accept_ra">
<input type="hidden" name="ipv6_vlan">
<input type="hidden" name="ipv6_pdonly">
<input type="hidden" name="ipv6_pd_norelease">
<input type="hidden" name="ipv6_isp_opt">
<input type="hidden" name="ipv6_wan_addr">
<input type="hidden" name="ipv6_prefix_len_wan">
<input type="hidden" name="ipv6_isp_gw">

<!-- / / / -->

<div class="section-title">IPv6 Configuration</div>
<div class="section">
	<script>
		var f, i;
		dns = nvram.ipv6_dns.split(/\s+/);

		f = [
			{ title: 'IPv6 Service Type', name: 'ipv6_service', type: 'select',
				options: [['', 'Disabled'],['native-pd','DHCPv6 with Prefix Delegation'],['native','Static IPv6'],['6to4','6to4 Anycast Relay'],['sit','6in4 Static Tunnel'],['6rd','6rd Relay'],['6rd-pd','6rd from DHCPv4 (Option 212)'],['other','Other (Manual Configuration)']],
				value: nvram.ipv6_service },
/* RTNPLUS-BEGIN */
			{ title: 'Debug', name: 'f_ipv6_debug', type: 'checkbox', value: (nvram.ipv6_debug == '1'), suffix: ' <small>(see Notes)<\/small>' },
/* RTNPLUS-END */
			{ title: 'IPv6 DUID Type', name: 'f_ipv6_duid_type', type: 'select',
				options: [['1','DUID-LLT'],['3', 'DUID-LL (default)']],
				value: nvram.ipv6_duid_type },
			{ title: 'IPv6 WAN Interface', name: 'ipv6_ifname', type: 'text', maxlen: 8, size: 10, value: nvram.ipv6_ifname },
			null,
			{ title: 'IPv6 WAN Address', name: 'f_ipv6_wan_addr', type: 'text', maxlen: 40, size: 42, value: nvram.ipv6_wan_addr, suffix: ' <small>(ex. 2001:0db8:1234::2)<\/small>' },
			{ title: 'IPv6 WAN Prefix Length', name: 'f_ipv6_prefix_len_wan', type: 'text', maxlen: 3, size: 5, value: nvram.ipv6_prefix_len_wan, suffix: ' <small>(Usually  64)<\/small>' },
			{ title: 'IPv6 WAN Gateway', name: 'f_ipv6_isp_gw', type: 'text', maxlen: 40, size: 42, value: nvram.ipv6_isp_gw, suffix: ' <small>(ex. 2001:0db8:1234::1)<\/small>' },
			{ title: 'Assigned / Routed Prefix', name: 'f_ipv6_prefix', type: 'text', maxlen: 40, size: 42, value: nvram.ipv6_prefix, suffix: ' <small>(ex. 2001:0db8:1234:0001::)<\/small>' },
			{ title: '6rd Routed Prefix', name: 'ipv6_6rd_prefix', type: 'text', maxlen: 40, size: 42, value: nvram.ipv6_6rd_prefix },
			{ title: '6rd Prefix Length', name: 'ipv6_6rd_prefix_length', type: 'text', maxlen: 3, size: 5, value: nvram.ipv6_6rd_prefix_length, suffix: ' <small>(Usually 32)<\/small>' },
			{ title: 'Prefix Length', name: 'f_ipv6_prefix_length', type: 'text', maxlen: 3, size: 5, value: nvram.ipv6_prefix_length },
			{ title: 'Request PD Only', name: 'f_ipv6_pdonly', type: 'checkbox', value: (nvram.ipv6_pdonly != '0'), suffix: ' <small>(Usually PPPoE connections)<\/small>' },
			{ title: 'Do not allow PD/Address release', name: 'f_ipv6_pd_norelease', type: 'checkbox', value: (nvram.ipv6_pd_norelease == '1'), suffix: ' <small>(see Notes)<\/small>' },
			{ title: 'Add default route ::/0', name: 'f_ipv6_isp_opt', type: 'checkbox', value: (nvram.ipv6_isp_opt != '0'), suffix: ' <small>(see Notes)<\/small>' },
			{ title: 'IPv6 Router LAN Address', multi: [
				{ name: 'f_ipv6_rtr_addr_auto', type: 'select', options: [['0', 'Default'],['1','Manual']], value: (nvram.ipv6_rtr_addr == '' ? '0' : '1') },
				{ name: 'f_ipv6_rtr_addr', type: 'text', maxlen: 46, size: 48, value: nvram.ipv6_rtr_addr }
			] },
			{ title: 'Static DNS', name: 'f_ipv6_dns_1', type: 'text', maxlen: 40, size: 42, value: dns[0] || '' },
			{ title: '',           name: 'f_ipv6_dns_2', type: 'text', maxlen: 40, size: 42, value: dns[1] || '' },
			{ title: '',           name: 'f_ipv6_dns_3', type: 'text', maxlen: 40, size: 42, value: dns[2] || '' },
			{ title: 'Accept RA from', multi: [
				{ suffix: '&nbsp; WAN &nbsp;&nbsp;&nbsp;', name: 'f_ipv6_accept_ra_wan', type: 'checkbox', value: (nvram.ipv6_accept_ra & 0x01) },
				{ suffix: '&nbsp; LAN &nbsp;',	name: 'f_ipv6_accept_ra_lan', type: 'checkbox', value: (nvram.ipv6_accept_ra & 0x02) }
			] },
			null,
			{ title: 'Tunnel Remote Endpoint (IPv4 Address)', name: 'ipv6_tun_v4end', type: 'text', maxlen: 15, size: 17, value: nvram.ipv6_tun_v4end },
			{ title: '6RD Tunnel Border Relay (IPv4 Address)', name: 'ipv6_6rd_borderrelay', type: 'text', maxlen: 15, size: 17, value: nvram.ipv6_6rd_borderrelay },
			{ title: '6RD IPv4 Mask Length', name: 'ipv6_6rd_ipv4masklen', type: 'text', maxlen: 3, size: 5, value: nvram.ipv6_6rd_ipv4masklen, suffix: ' <small>(usually 0)<\/small>' },
			{ title: 'Relay Anycast Address', name: 'ipv6_relay', type: 'text', maxlen: 3, size: 5, prefix: '192.88.99.&nbsp&nbsp', value: nvram.ipv6_relay },
			{ title: 'Tunnel Client IPv6 Address', multi: [
				{ name: 'ipv6_tun_addr', type: 'text', maxlen: 46, size: 48, value: nvram.ipv6_tun_addr, suffix: ' / ' },
				{ name: 'ipv6_tun_addrlen', type: 'text', maxlen: 3, size: 5, value: nvram.ipv6_tun_addrlen }
			] },
			{ title: 'Tunnel MTU', name: 'ipv6_tun_mtu', type: 'text', maxlen: 4, size: 8, value: nvram.ipv6_tun_mtu, suffix: ' <small>(0 for default)<\/small>' },
			{ title: 'Tunnel TTL', name: 'ipv6_tun_ttl', type: 'text', maxlen: 3, size: 8, value: nvram.ipv6_tun_ttl },
			null
		];

		for (i = 1; i <= MAX_BRIDGE_ID; ++i) {
			f.push({
				title: (i == 1) ? 'Enable IPv6 subnet for' : '',
				name: 'f_lan' + i + '_ipv6',
				type: 'checkbox',
				value: (nvram.ipv6_vlan & (1 << (i - 1))),
				suffix: '&nbsp; LAN' + i + '(br' + i + ') &nbsp;&nbsp;&nbsp;'
			});
		}

		createFieldTable('', f);
	</script>
</div>

<!-- / / / -->

<div id="notice_container" style="display:none">&nbsp;</div>

<!-- / / / -->

<div class="section-title">Notes</div>
<div class="section">
	<ul>
		<li><b>Request PD Only</b> - Check for ISP's that require only a Prefix Delegation (usually PPPoE (xDSL, Fiber) connections).</li>
		<li><b>Do not allow PD/Address release</b> - Prevent DHCP6 client to send a release message to the ISP on exit. With this option set, the client is more likely to receive the same allocation with subsequent requests.</li>
		<li><b>Add default route ::/0</b> - Some ISP's may need the default route (workaround).</li>
		<li><b>Accept RA from LAN</b> - Please disable Announce IPv6 on LAN (SLAAC) and Announce IPv6 on LAN (DHCP) at <a href="advanced-dhcpdns.asp">DHCP/DNS</a> to enable that option.</li>
<!-- RTNPLUS-BEGIN -->
		<li><b>Debug</b> - Start DHCP6 client in debug mode.</li>
<!-- RTNPLUS-END -->
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
<script>earlyInit()</script>
</body>
</html>
