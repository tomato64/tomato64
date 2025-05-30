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
<title>[<% ident(); %>] Advanced: Firewall</title>
<link rel="stylesheet" type="text/css" href="tomato.css?rel=<% version(); %>">
<% css(); %>
<script src="tomato.js?rel=<% version(); %>"></script>

<script>

/* TOMATO64-REMOVE-BEGIN */
//	<% nvram("block_wan,block_wan_limit,block_wan_limit_icmp,nf_loopback,ne_syncookies,DSCP_fix_enable,multicast_pass,multicast_lan,multicast_lan1,multicast_lan2,multicast_lan3,multicast_quickleave,multicast_custom,lan_ifname,lan1_ifname,lan2_ifname,lan3_ifname,udpxy_enable,udpxy_lan,udpxy_lan1,udpxy_lan2,udpxy_lan3,udpxy_stats,udpxy_clients,udpxy_port,udpxy_wanface,ne_snat,emf_enable,force_igmpv2,wan_dhcp_pass,fw_blackhole"); %>
/* TOMATO64-REMOVE-END */
/* TOMATO64-BEGIN */
//	<% nvram("block_wan,block_wan_limit,block_wan_limit_icmp,nf_loopback,ne_syncookies,DSCP_fix_enable,multicast_pass,multicast_lan,multicast_lan1,multicast_lan2,multicast_lan3,multicast_lan4,multicast_lan5,multicast_lan6,multicast_lan7,multicast_quickleave,multicast_custom,lan_ifname,lan1_ifname,lan2_ifname,lan3_ifname,lan4_ifname,lan5_ifname,lan6_ifname,lan7_ifname,udpxy_enable,udpxy_lan,udpxy_lan1,udpxy_lan2,udpxy_lan3,udpxy_lan4,udpxy_lan5,udpxy_lan6,udpxy_lan7,udpxy_stats,udpxy_clients,udpxy_port,udpxy_wanface,ne_snat,emf_enable,force_igmpv2,wan_dhcp_pass,fw_blackhole,flow_offloading,wed_offloading,packet_steering,steering_flows,steering_flows_custom"); %>
/* TOMATO64-END */

var cprefix = 'advanced_firewall';

function verifyFields(focused, quiet) {
/* ICMP */
	E('_f_icmp_limit').disabled = !E('_f_icmp').checked;
	E('_f_icmp_limit_icmp').disabled = (!E('_f_icmp').checked || !E('_f_icmp_limit').checked);

/* PROXY-BEGIN */
	var enable_mcast = E('_f_multicast').checked;
	E('_f_multicast_lan').disabled = ((!enable_mcast) || (nvram.lan_ifname.length < 1));
	E('_f_multicast_lan1').disabled = ((!enable_mcast) || (nvram.lan1_ifname.length < 1));
	E('_f_multicast_lan2').disabled = ((!enable_mcast) || (nvram.lan2_ifname.length < 1));
	E('_f_multicast_lan3').disabled = ((!enable_mcast) || (nvram.lan3_ifname.length < 1));
/* TOMATO64-BEGIN */
	E('_f_multicast_lan4').disabled = ((!enable_mcast) || (nvram.lan4_ifname.length < 1));
	E('_f_multicast_lan5').disabled = ((!enable_mcast) || (nvram.lan5_ifname.length < 1));
	E('_f_multicast_lan6').disabled = ((!enable_mcast) || (nvram.lan6_ifname.length < 1));
	E('_f_multicast_lan7').disabled = ((!enable_mcast) || (nvram.lan7_ifname.length < 1));
/* TOMATO64-END */
	E('_f_multicast_quickleave').disabled = (!enable_mcast);

	if (nvram.lan_ifname.length < 1)
		E('_f_multicast_lan').checked = 0;
	if (nvram.lan1_ifname.length < 1)
		E('_f_multicast_lan1').checked = 0;
	if (nvram.lan2_ifname.length < 1)
		E('_f_multicast_lan2').checked = 0;
	if (nvram.lan3_ifname.length < 1)
		E('_f_multicast_lan3').checked = 0;
/* TOMATO64-BEGIN */
	if (nvram.lan4_ifname.length < 1)
		E('_f_multicast_lan4').checked = 0;
	if (nvram.lan5_ifname.length < 1)
		E('_f_multicast_lan5').checked = 0;
	if (nvram.lan6_ifname.length < 1)
		E('_f_multicast_lan6').checked = 0;
	if (nvram.lan7_ifname.length < 1)
		E('_f_multicast_lan7').checked = 0;
/* TOMATO64-END */

	var mcast_lan = E('_f_multicast_lan').checked;
	var mcast_lan1 = E('_f_multicast_lan1').checked;
	var mcast_lan2 = E('_f_multicast_lan2').checked;
	var mcast_lan3 = E('_f_multicast_lan3').checked;
/* TOMATO64-BEGIN */
	var mcast_lan4 = E('_f_multicast_lan4').checked;
	var mcast_lan5 = E('_f_multicast_lan5').checked;
	var mcast_lan6 = E('_f_multicast_lan6').checked;
	var mcast_lan7 = E('_f_multicast_lan7').checked;
/* TOMATO64-END */
	var mcast_custom_enable = 0;
/* disable multicast_custom textarea if lanX is checked / selected */
/* TOMATO64-REMOVE-BEGIN */
	E('_multicast_custom').disabled = ((!enable_mcast) || (mcast_lan) || (mcast_lan1) || (mcast_lan2) || (mcast_lan3));
/* TOMATO64-REMOVE-END */
/* TOMATO64-BEGIN */
	E('_multicast_custom').disabled = ((!enable_mcast) || (mcast_lan) || (mcast_lan1) || (mcast_lan2) || (mcast_lan3) || (mcast_lan4) || (mcast_lan5) || (mcast_lan6) || (mcast_lan7));
/* TOMATO64-END */
/* check if more than 50 charactars are in the textarea (no plausibility test) */
	if (!E('_multicast_custom').disabled) {
		if (v_length('_multicast_custom', 1, 50, 2048))
			mcast_custom_enable = 1;
		else
			mcast_custom_enable = 0;
	}
	else
		ferror.clear('_multicast_custom');
/* IGMP proxy enable checked but no lanX checked and no custom config */
/* TOMATO64-REMOVE-BEGIN */
	if ((enable_mcast) && (!mcast_lan) && (!mcast_lan1) && (!mcast_lan2) && (!mcast_lan3) && (!mcast_custom_enable)) {
/* TOMATO64-REMOVE-END */
/* TOMATO64-BEGIN */
	if ((enable_mcast) && (!mcast_lan) && (!mcast_lan1) && (!mcast_lan2) && (!mcast_lan3) && (!mcast_lan4) && (!mcast_lan5) && (!mcast_lan6) && (!mcast_lan7) && (!mcast_custom_enable)) {
/* TOMATO64-END */
		ferror.set('_f_multicast', 'IGMP proxy must be enabled in at least one LAN bridge OR you have to use custom configuration', quiet);
		return 0;
	}
/* IGMP proxy enable checked but custom config / textarea length not OK */
	else if ((enable_mcast) && (mcast_custom_enable) && !v_length('_multicast_custom', quiet, 0, 2048))
		return 0;
/* IGMP proxy clear */
	else
		ferror.clear('_f_multicast');

	var enable_udpxy = E('_f_udpxy_enable').checked;
	E('_f_udpxy_lan').disabled = ((!enable_udpxy) || (nvram.lan_ifname.length < 1));
	E('_f_udpxy_lan1').disabled = ((!enable_udpxy) || (nvram.lan1_ifname.length < 1));
	E('_f_udpxy_lan2').disabled = ((!enable_udpxy) || (nvram.lan2_ifname.length < 1));
	E('_f_udpxy_lan3').disabled = ((!enable_udpxy) || (nvram.lan3_ifname.length < 1));
/* TOMATO64-BEGIN */
	E('_f_udpxy_lan4').disabled = ((!enable_udpxy) || (nvram.lan4_ifname.length < 1));
	E('_f_udpxy_lan5').disabled = ((!enable_udpxy) || (nvram.lan5_ifname.length < 1));
	E('_f_udpxy_lan6').disabled = ((!enable_udpxy) || (nvram.lan6_ifname.length < 1));
	E('_f_udpxy_lan7').disabled = ((!enable_udpxy) || (nvram.lan7_ifname.length < 1));
/* TOMATO64-END */
	E('_f_udpxy_stats').disabled = !enable_udpxy;
	E('_f_udpxy_clients').disabled = !enable_udpxy;
	E('_f_udpxy_port').disabled = !enable_udpxy;
	E('_f_udpxy_wanface').disabled = !enable_udpxy;

/* TOMATO64-WIFI-BEGIN */
	E('_wed_offloading').readonly = !(E('_flow_offloading').value == 2);
	if (E('_flow_offloading').value != 2)
		E('_wed_offloading').value = 0;

	var p = E('_packet_steering').value != 0;
	var f = E('_steering_flows').value == 'custom';

	PR(E('_steering_flows')).style.display = p ? '' : 'none';

	elem.display('_steering_flows_custom', (p && f));
	E('_steering_flows_custom').disabled = (!p || !f);
	E('steering_note').style.display = p ? '' : 'none';
/* TOMATO64-WIFI-END */

	if (nvram.lan_ifname.length < 1)
		E('_f_udpxy_lan').checked = 0;
	if (nvram.lan1_ifname.length < 1)
		E('_f_udpxy_lan1').checked = 0;
	if (nvram.lan2_ifname.length < 1)
		E('_f_udpxy_lan2').checked = 0;
	if (nvram.lan3_ifname.length < 1)
		E('_f_udpxy_lan3').checked = 0;
/* TOMATO64-BEGIN */
	if (nvram.lan4_ifname.length < 1)
		E('_f_udpxy_lan4').checked = 0;
	if (nvram.lan5_ifname.length < 1)
		E('_f_udpxy_lan5').checked = 0;
	if (nvram.lan6_ifname.length < 1)
		E('_f_udpxy_lan6').checked = 0;
	if (nvram.lan7_ifname.length < 1)
		E('_f_udpxy_lan7').checked = 0;
/* TOMATO64-END */

	var udpxy_lan = E('_f_udpxy_lan').checked ? 1 : 0;
	var udpxy_lan1 = E('_f_udpxy_lan1').checked ? 1 : 0;
	var udpxy_lan2 = E('_f_udpxy_lan2').checked ? 1 : 0;
	var udpxy_lan3 = E('_f_udpxy_lan3').checked ? 1 : 0;
/* TOMATO64-BEGIN */
	var udpxy_lan4 = E('_f_udpxy_lan4').checked ? 1 : 0;
	var udpxy_lan5 = E('_f_udpxy_lan5').checked ? 1 : 0;
	var udpxy_lan6 = E('_f_udpxy_lan6').checked ? 1 : 0;
	var udpxy_lan7 = E('_f_udpxy_lan7').checked ? 1 : 0;
/* TOMATO64-END */
/* TOMATO64-REMOVE-BEGIN */
	var udpxy_lan_count = udpxy_lan + udpxy_lan1 + udpxy_lan2 + udpxy_lan3;
/* TOMATO64-REMOVE-END */
/* TOMATO64-BEGIN */
	var udpxy_lan_count = udpxy_lan + udpxy_lan1 + udpxy_lan2 + udpxy_lan3 + udpxy_lan4 + udpxy_lan5 + udpxy_lan6 + udpxy_lan7;
/* TOMATO64-END */

/* udpxy check: only one interface can be selected to listen on OR no interface (listen on default: 0.0.0.0) */
	if (enable_udpxy && (udpxy_lan_count > 1)) {
/* TOMATO64-REMOVE-BEGIN */
		ferror.set('_f_udpxy_enable', 'Udpxy: please select only one interface (LAN0, LAN1, LAN2 or LAN3) or none (see notes)', quiet);
/* TOMATO64-REMOVE-END */
/* TOMATO64-BEGIN */
		ferror.set('_f_udpxy_enable', 'Udpxy: please select only one interface (LAN0, LAN1, LAN2, LAN3, LAN4, LAN5, LAN6 or LAN7) or none (see notes)', quiet);
/* TOMATO64-END */
		return 0;
/* udpxy clear */
	}
	else
		ferror.clear('_f_udpxy_enable');
/* PROXY-END */

	return 1;
}

function save() {
	if (!verifyFields(null, 0))
		return;

	var fom = E('t_fom');
	fom.block_wan.value = fom._f_icmp.checked ? 0 : 1;
	fom.block_wan_limit.value = fom._f_icmp_limit.checked? 1 : 0;
	fom.block_wan_limit_icmp.value = fom._f_icmp_limit_icmp.value;

	fom.ne_syncookies.value = fom._f_syncookies.checked ? 1 : 0;
	fom.DSCP_fix_enable.value = fom._f_DSCP_fix_enable.checked ? 1 : 0;
/* PROXY-BEGIN */
	fom.multicast_pass.value = fom._f_multicast.checked ? 1 : 0;
	fom.multicast_lan.value = fom._f_multicast_lan.checked ? 1 : 0;
	fom.multicast_lan1.value = fom._f_multicast_lan1.checked ? 1 : 0;
	fom.multicast_lan2.value = fom._f_multicast_lan2.checked ? 1 : 0;
	fom.multicast_lan3.value = fom._f_multicast_lan3.checked ? 1 : 0;
/* TOMATO64-BEGIN */
	fom.multicast_lan4.value = fom._f_multicast_lan4.checked ? 1 : 0;
	fom.multicast_lan5.value = fom._f_multicast_lan5.checked ? 1 : 0;
	fom.multicast_lan6.value = fom._f_multicast_lan6.checked ? 1 : 0;
	fom.multicast_lan7.value = fom._f_multicast_lan7.checked ? 1 : 0;
/* TOMATO64-END */
	fom.multicast_quickleave.value = fom._f_multicast_quickleave.checked ? 1 : 0;
	fom.udpxy_enable.value = fom._f_udpxy_enable.checked ? 1 : 0;
	fom.udpxy_lan.value = fom._f_udpxy_lan.checked ? 1 : 0;
	fom.udpxy_lan1.value = fom._f_udpxy_lan1.checked ? 1 : 0;
	fom.udpxy_lan2.value = fom._f_udpxy_lan2.checked ? 1 : 0;
	fom.udpxy_lan3.value = fom._f_udpxy_lan3.checked ? 1 : 0;
/* TOMATO64-BEGIN */
	fom.udpxy_lan4.value = fom._f_udpxy_lan4.checked ? 1 : 0;
	fom.udpxy_lan5.value = fom._f_udpxy_lan5.checked ? 1 : 0;
	fom.udpxy_lan6.value = fom._f_udpxy_lan6.checked ? 1 : 0;
	fom.udpxy_lan7.value = fom._f_udpxy_lan7.checked ? 1 : 0;
/* TOMATO64-END */
	fom.udpxy_stats.value = fom._f_udpxy_stats.checked ? 1 : 0;
	fom.udpxy_clients.value = fom._f_udpxy_clients.value;
	fom.udpxy_port.value = fom._f_udpxy_port.value;
	fom.udpxy_wanface.value = fom._f_udpxy_wanface.value;
/* PROXY-END */
/* EMF-BEGIN */
	fom.emf_enable.value = fom._f_emf.checked ? 1 : 0;
	if (fom.emf_enable.value != nvram.emf_enable)
		fom._service.value = '*';
/* EMF-END */
	fom.force_igmpv2.value = fom._f_force_igmpv2.checked ? 1 : 0;
	fom.wan_dhcp_pass.value = fom._f_wan_dhcp_pass.checked ? 1 : 0;
	fom.fw_blackhole.value = fom._f_fw_blackhole.checked ? 1 : 0;

/* TOMATO64-WIFI-BEGIN */
	if (fom.wed_offloading.value != nvram.wed_offloading) {
		if (confirm("Router must be rebooted to apply changed settings. Reboot now? (and commit changes to NVRAM)")) {
			fom._reboot.value = 1;
			form.submit(fom, 0);
                }
		else { /* countinue without reboot (user wants it that way) */
			form.submit(fom, 1);
		}
	}
	else {
/* TOMATO64-WIFI-END */
	form.submit(fom, 1);
/* TOMATO64-WIFI-BEGIN */
	}
/* TOMATO64-WIFI-END */
}

function init() {
	if (((c = cookie.get(cprefix + '_notes_vis')) != null) && (c == '1'))
		toggleVisibility(cprefix, 'notes');

	eventHandler();
}
</script>
</head>

<body onload="init()">
<form id="t_fom" method="post" action="tomato.cgi">
<table id="container">
<tr><td colspan="2" id="header">
	<div class="title"><a href="/">Tomato64</a></div>
	<div class="version">Version <% version(); %> on <% nv("t_model_name"); %></div>
</td></tr>
<tr id="body"><td id="navi"><script>navi()</script></td>
<td id="content">
<div id="ident"><% ident(); %> | <script>wikiLink();</script></div>

<!-- / / / -->

<input type="hidden" name="_nextpage" value="advanced-firewall.asp">
<input type="hidden" name="_service" value="firewall-restart">
/* TOMATO64-WIFI-BEGIN */
<input type="hidden" name="_reboot" value="0">
/* TOMATO64-WIFI-END */
<input type="hidden" name="block_wan">
<input type="hidden" name="block_wan_limit">
<input type="hidden" name="block_wan_limit_icmp">
<input type="hidden" name="ne_syncookies">
<input type="hidden" name="DSCP_fix_enable">
<!-- PROXY-BEGIN -->
<input type="hidden" name="multicast_pass">
<input type="hidden" name="multicast_lan">
<input type="hidden" name="multicast_lan1">
<input type="hidden" name="multicast_lan2">
<input type="hidden" name="multicast_lan3">
/* TOMATO64-BEGIN */
<input type="hidden" name="multicast_lan4">
<input type="hidden" name="multicast_lan5">
<input type="hidden" name="multicast_lan6">
<input type="hidden" name="multicast_lan7">
/* TOMATO64-END */
<input type="hidden" name="multicast_quickleave">
<input type="hidden" name="udpxy_enable">
<input type="hidden" name="udpxy_lan">
<input type="hidden" name="udpxy_lan1">
<input type="hidden" name="udpxy_lan2">
<input type="hidden" name="udpxy_lan3">
/* TOMATO64-BEGIN */
<input type="hidden" name="udpxy_lan4">
<input type="hidden" name="udpxy_lan5">
<input type="hidden" name="udpxy_lan6">
<input type="hidden" name="udpxy_lan7">
/* TOMATO64-END */
<input type="hidden" name="udpxy_stats">
<input type="hidden" name="udpxy_clients">
<input type="hidden" name="udpxy_port">
<input type="hidden" name="udpxy_wanface">
<!-- PROXY-END -->
<!-- EMF-BEGIN -->
<input type="hidden" name="emf_enable">
<!-- EMF-END -->
<input type="hidden" name="force_igmpv2">
<input type="hidden" name="wan_dhcp_pass">
<input type="hidden" name="fw_blackhole">

<!-- / / / -->

<div class="section-title">Firewall</div>
<div class="section">
	<script>
		createFieldTable('', [
			{ title: 'WAN interfaces respond to Ping and Traceroute', name: 'f_icmp', type: 'checkbox', value: nvram.block_wan == '0' },
			{ title: 'Limit communication to', multi: [
				{ name: 'f_icmp_limit', type: 'checkbox', value: nvram.block_wan_limit != '0' },
				{ name: 'f_icmp_limit_icmp', type: 'text', maxlen: 3, size: 3, prefix: ' &nbsp;', suffix: ' &nbsp;<small>request per second<\/small>', value: fixInt(nvram.block_wan_limit_icmp || 1, 1, 300, 5) } ] },
			null,
			{ title: 'Enable TCP SYN cookies', name: 'f_syncookies', type: 'checkbox', value: nvram.ne_syncookies != '0' },
			{ title: 'Enable DSCP Fix', name: 'f_DSCP_fix_enable', type: 'checkbox', value: nvram.DSCP_fix_enable != '0', suffix: ' &nbsp;<small>fixes Comcast incorrect DSCP<\/small>' },
			null,
			{ title: 'Allow DHCP spoofing', name: 'f_wan_dhcp_pass', type: 'checkbox', value: nvram.wan_dhcp_pass == 1 },
			{ title: 'Smart MTU black hole detection', name: 'f_fw_blackhole', type: 'checkbox', value: nvram.fw_blackhole == 1 }
		]);
	</script>
</div>

/* TOMATO64-WIFI-BEGIN */
<div class="section-title">Routing/NAT Offloading</div>
<div class="section">
	<script>
		createFieldTable('', [
			{ title: 'Flow offloading type', name: 'flow_offloading', type: 'select', options: [[0,'None'],[1,'Software flow offloading'],[2,'Hardware flow offloading']], value: fixInt(nvram.flow_offloading, 0, 2, 0) },
			{ title: 'Wireless Ethernet Dispatch (WED)', name: 'wed_offloading', type: 'select', options: [[0,'Disabled'],[1,'Enabled']], value: fixInt(nvram.wed_offloading, 0, 1, 0), suffix: ' &nbsp;<small>requires Hardware flow offloading<\/small>' }
		]);
	</script>
</div>

<div class="section-title">Packet Steering</div>
<div class="section">
	<script>
		createFieldTable('', [
			{ title: 'Packet Steering', name: 'packet_steering', type: 'select', options: [[0,'Disabled'],[1,'Enabled'],[2,'Enabled (all CPUSs)']], value: nvram.packet_steering, suffix: ' &nbsp;<small>Enable packet steering across CPUs. May help or hinder network speed.<\/small>' },
			{ title: 'Steering Flows (RPS)*', multi: [
				{ name: 'steering_flows', type: 'select', options: [['0','none'],['128','128'],['256','256'],['custom','Custom']], value: nvram.steering_flows },
				{ name: 'steering_flows_custom', type: 'text', maxlen: 5, size: 5, value: nvram.steering_flows_custom },
				{ suffix: ' &nbsp;<small>Directs packet flows to specific CPUs where the local socket owner listens (the local service).<\/small>' } ] }
		]);
	</script>
	<div id="steering_note"><small><br>*&nbsp;Note: this setting is for local services on the device only (not for forwarding).</small></div>
</div>
/* TOMATO64-WIFI-END */

<!-- / / / -->

<div class="section-title">NAT</div>
<div class="section">
	<script>
		createFieldTable('', [
			{ title: 'NAT loopback', name: 'nf_loopback', type: 'select', options: [[0,'All'],[1,'Forwarded Only'],[2,'Disabled']], value: fixInt(nvram.nf_loopback, 0, 2, 1) },
			{ title: 'NAT target', name: 'ne_snat', type: 'select', options: [[0,'MASQUERADE'],[1,'SNAT']], value: nvram.ne_snat }
		]);
	</script>
</div>

<!-- / / / -->

<div class="section-title">Multicast</div>
<div class="section">
	<script>
		createFieldTable('', [
/* PROXY-BEGIN */
			{ title: 'Enable IGMP proxy', name: 'f_multicast', type: 'checkbox', value: nvram.multicast_pass == '1' },
			{ title: 'LAN0', indent: 2, name: 'f_multicast_lan', type: 'checkbox', value: (nvram.multicast_lan == '1') },
			{ title: 'LAN1', indent: 2, name: 'f_multicast_lan1', type: 'checkbox', value: (nvram.multicast_lan1 == '1') },
			{ title: 'LAN2', indent: 2, name: 'f_multicast_lan2', type: 'checkbox', value: (nvram.multicast_lan2 == '1') },
			{ title: 'LAN3', indent: 2, name: 'f_multicast_lan3', type: 'checkbox', value: (nvram.multicast_lan3 == '1') },
/* TOMATO64-BEGIN */
			{ title: 'LAN4', indent: 2, name: 'f_multicast_lan4', type: 'checkbox', value: (nvram.multicast_lan4 == '1') },
			{ title: 'LAN5', indent: 2, name: 'f_multicast_lan5', type: 'checkbox', value: (nvram.multicast_lan5 == '1') },
			{ title: 'LAN6', indent: 2, name: 'f_multicast_lan6', type: 'checkbox', value: (nvram.multicast_lan6 == '1') },
			{ title: 'LAN7', indent: 2, name: 'f_multicast_lan7', type: 'checkbox', value: (nvram.multicast_lan7 == '1') },
/* TOMATO64-END */
			{ title: 'Enable quickleave', indent: 2, name: 'f_multicast_quickleave', type: 'checkbox', value: (nvram.multicast_quickleave == '1') },
			{ title: '<a href="https://github.com/pali/igmpproxy" class="new_window">IGMP proxy<\/a><br>Custom configuration', name: 'multicast_custom', type: 'textarea', value: nvram.multicast_custom },
			null,
			{ title: 'Enable Udpxy', name: 'f_udpxy_enable', type: 'checkbox', value: (nvram.udpxy_enable == '1') },
			{ title: 'Upstream interface', indent: 2, name: 'f_udpxy_wanface', type: 'text', maxlen: 8, size: 8, value: nvram.udpxy_wanface, suffix: ' &nbsp;<small>leave empty for default<\/small>' },
			{ title: 'LAN0', indent: 2, name: 'f_udpxy_lan', type: 'checkbox', value: (nvram.udpxy_lan == '1') },
			{ title: 'LAN1', indent: 2, name: 'f_udpxy_lan1', type: 'checkbox', value: (nvram.udpxy_lan1 == '1') },
			{ title: 'LAN2', indent: 2, name: 'f_udpxy_lan2', type: 'checkbox', value: (nvram.udpxy_lan2 == '1') },
			{ title: 'LAN3', indent: 2, name: 'f_udpxy_lan3', type: 'checkbox', value: (nvram.udpxy_lan3 == '1') },
/* TOMATO64-BEGIN */
			{ title: 'LAN4', indent: 2, name: 'f_udpxy_lan4', type: 'checkbox', value: (nvram.udpxy_lan4 == '1') },
			{ title: 'LAN5', indent: 2, name: 'f_udpxy_lan5', type: 'checkbox', value: (nvram.udpxy_lan5 == '1') },
			{ title: 'LAN6', indent: 2, name: 'f_udpxy_lan6', type: 'checkbox', value: (nvram.udpxy_lan6 == '1') },
			{ title: 'LAN7', indent: 2, name: 'f_udpxy_lan7', type: 'checkbox', value: (nvram.udpxy_lan7 == '1') },
/* TOMATO64-END */
			{ title: 'Enable client statistics', indent: 2, name: 'f_udpxy_stats', type: 'checkbox', value: (nvram.udpxy_stats == '1') },
			{ title: 'Max clients', indent: 2, name: 'f_udpxy_clients', type: 'text', maxlen: 4, size: 6, value: fixInt(nvram.udpxy_clients || 3, 1, 5000, 3) },
			{ title: 'Udpxy port', indent: 2, name: 'f_udpxy_port', type: 'text', maxlen: 5, size: 7, value: fixPort(nvram.udpxy_port, 4022) },
			null,
/* PROXY-END */
/* EMF-BEGIN */
			{ title: 'Efficient Multicast Forwarding (IGMP Snooping)', name: 'f_emf', type: 'checkbox', value: nvram.emf_enable != '0', suffix: ' &nbsp;<small>Please refer to the wiki<\/small>' },
/* EMF-END */
			{ title: 'Force IGMPv2', name: 'f_force_igmpv2', type: 'checkbox', value: nvram.force_igmpv2 != '0' }
		]);
	</script>
</div>

<!-- / / / -->

<div class="section-title">Notes <small><i><a href="javascript:toggleVisibility(cprefix,'notes');" id="toggleLink-notes"><span id="sesdiv_notes_showhide">(Show)</span></a></i></small></div>
<div class="section" id="sesdiv_notes" style="display:none">
	<i>Firewall:</i><br>
	<ul>
		<li><b>Allow DHCP spoofing</b> - Accept/process packets from DHCP servers whose IP is different from the one advertised within the DHCP messages. This is often categorized as an attack (DHCP spoofing) but could be a genuine scenario in some rare cases. Enabling the option lowers the security level.</li>
		<li><b>Smart MTU black hole detection</b> - Read more <a href="https://blog.cloudflare.com/path-mtu-discovery-in-practice/" class="new_window">HERE</a>.</li>
	</ul>
<!-- PROXY-BEGIN -->
	<i>IGMP proxy:</i><br>
	<ul>
/* TOMATO64-REMOVE-BEGIN */
		<li><b>LAN0 / LAN1 / LAN2 / LAN3</b> - Add interface br0 / br1 / br2 / br3 to igmp.conf (Ex.: phyint br0 downstream ratelimit 0 threshold 1).</li>
/* TOMATO64-REMOVE-END */
/* TOMATO64-BEGIN */
		<li><b>LAN0 / LAN1 / LAN2 / LAN3 / LAN4 / LAN5 / LAN6 / LAN7</b> - Add interface br0 / br1 / br2 / br3 / br4 / br5 / br6 / br7 to igmp.conf (Ex.: phyint br0 downstream ratelimit 0 threshold 1).</li>
/* TOMATO64-END */
		<li><b>Enable quickleave</b> - Send a Leave IGMP message upstream as soon as it receives a Leave message for any downstream interface.</li>
		<li><b>Custom configuration</b> - Use custom config for IGMP proxy instead of tomato default config. You must define one (or more) upstream interface(s) and one or more downstream interfaces. Refer to the <a href="https://github.com/pali/igmpproxy/blob/master/igmpproxy.conf" class="new_window">IGMP proxy example configuration</a> and <a href="https://github.com/pali/igmpproxy/commit/b55e0125c79fc9dbc95c6d6ab1121570f0c6f80f" class="new_window">IGMP proxy commit b55e0125c79fc9d</a> for details.</li>
		<li><b>Note</b> - You can use your own custom config file (/etc/igmp.alt).</li>
		<li><b>Other hints</b> - For error messages please check the <a href="status-log.asp">log file</a>.</li>
		<li><b>Hidden settings (only optional)</b> - To define sources for multicasting and IGMP data you can use NVRAM variable <b>multicast_altnet_1</b>, <b>multicast_altnet_2</b> and <b>multicast_altnet_3</b> without using a (complete) custom configuration. Format: a.b.c.d/n (Example: 10.0.0.0/16).</li>
	</ul>
	<i>Udpxy:</i><br>
	<ul>
		<li><b>Upstream interface</b> - As one use case for it is to access IPTV which is often delivered via multicast by the ISP but outside of the (PPPoE,etc) session.</li>
/* TOMATO64-REMOVE-BEGIN */
		<li><b>LAN0 / LAN1 / LAN2 / LAN3</b> - Select one interface br0 / br1 / br2 / br3 to listen on.</li>
/* TOMATO64-REMOVE-END */
/* TOMATO64-BEGIN */
		<li><b>LAN0 / LAN1 / LAN2 / LAN3 / LAN4 / LAN5 / LAN6 / LAN7</b> - Select one interface br0 / br1 / br2 / br3 to listen on.</li>
/* TOMATO64-END */
		<li><b>Status</b> - To display udpxy status, please go to http://lanaddress:port/status/.</li>
		<li><b>Other hints</b> - If Udpxy is enabled and no interface is selected default address (0.0.0.0) will be used.</li>
	</ul>
<!-- PROXY-END -->
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
<script>verifyFields(null, true);</script>
</body>
</html>
