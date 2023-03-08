<!DOCTYPE html>
<!--
	Tomato GUI
	Copyright (C) 2007-2011 Shibby
	http://openlinksys.info
	For use with Tomato Firmware only.
	No part of this file may be used without permission.
-->
<html lang="en-GB">
<head>
<meta http-equiv="content-type" content="text/html;charset=utf-8">
<meta name="robots" content="noindex,nofollow">
<title>[<% ident(); %>] Advanced: TOR Project</title>
<link rel="stylesheet" type="text/css" href="tomato.css">
<% css(); %>
<script src="isup.jsz"></script>
<script src="isup.js"></script>
<script src="tomato.js"></script>

<script>

//	<% nvram("tor_enable,tor_solve_only,tor_socksport,tor_transport,tor_dnsport,tor_datadir,tor_users,tor_ports,tor_ports_custom,tor_custom,tor_iface,lan_ifname,lan1_ifname,lan2_ifname,lan3_ifname"); %>

var changed = 0;
var serviceType = 'tor';

function verifyFields(focused, quiet) {
	if (focused && focused != E('_f_tor_enable')) /* except on/off */
		changed = 1;

	var ok = 1;

	var txt1 = ' option is not allowed here.';
	var txt2 = ' You can set it in FreshTomato GUI';

	var b = E('_f_tor_solve_only').checked;
	var o = (E('_tor_iface').value == 'custom');
	var p = (E('_tor_ports').value == 'custom');

	E('_tor_iface').disabled = b;
	E('_tor_ports').disabled = b;

	elem.display('_tor_users', o && !b);
	elem.display('_tor_ports_custom', p && !b);

	var bridge = E('_tor_iface');
	if (nvram.lan_ifname.length < 1)
		bridge.options[0].disabled = 1;
	if (nvram.lan1_ifname.length < 1)
		bridge.options[1].disabled = 1;
	if (nvram.lan2_ifname.length < 1)
		bridge.options[2].disabled = 1;
	if (nvram.lan3_ifname.length < 1)
		bridge.options[3].disabled = 1;

	if (!v_port(E('_tor_socksport'), quiet || !ok)) ok = 0;
	if (!v_port(E('_tor_transport'), quiet || !ok)) ok = 0;
	if (!v_port(E('_tor_dnsport'), quiet || !ok)) ok = 0;
	if (!v_nodelim(E('_tor_datadir'), quiet || !ok, 'Directory', 1) || !v_path(E('_tor_datadir'), quiet || !ok, 1)) ok = 0;

	var s = E('_tor_custom');

	if (s.value.indexOf('DNSPort') != -1) {
		ferror.set(s, 'The "DNSPort"'+txt1+txt2, quiet || !ok);
		ok = 0;
	}

	if (s.value.indexOf('DataDirectory') != -1) {
		ferror.set(s, 'The "DataDirectory"'+txt1+txt2, quiet || !ok);
		ok = 0;
	}

	if (s.value.indexOf('TransPort') != -1) {
		ferror.set(s, 'The "TransPort"'+txt1+txt2, quiet || !ok);
		ok = 0;
	}

	if (s.value.indexOf('SocksBindAddress') != -1) {
		ferror.set(s, 'The "SocksBindAddress"'+txt1, quiet || !ok);
		ok = 0;
	}

	if (s.value.indexOf('AllowUnverifiedNodes') != -1) {
		ferror.set(s, 'The "AllowUnverifiedNodes"'+txt1, quiet || !ok);
		ok = 0;
	}

	if (s.value.indexOf('TransListenAddress') != -1) {
		ferror.set(s, 'The "TransListenAddress"'+txt1, quiet || !ok);
		ok = 0;
	}

	if (s.value.indexOf('DNSListenAddress') != -1) {
		ferror.set(s, 'The "DNSListenAddress"'+txt1, quiet || !ok);
		ok = 0;
	}

	if (s.value.indexOf('User') != -1) {
		ferror.set(s, 'The "User"'+txt1, quiet || !ok);
		ok = 0;
	}

	if (s.value.indexOf('Log') != -1) {
		ferror.set(s, 'The "Log"'+txt1, quiet || !ok);
		ok = 0;
	}

	return ok;
}

function save_pre() {
	if (!verifyFields(null, 0))
		return 0;
	return 1;
}

function save(nomsg) {
	save_pre();
	if (!nomsg) show(); /* update '_service' field first */

	var fom = E('t_fom');
	fom.tor_enable.value = E('_f_tor_enable').checked ? 1 : 0;
	fom.tor_solve_only.value = E('_f_tor_solve_only').checked ? 1 : 0;
	fom._nofootermsg.value = (nomsg ? 1 : 0);

	form.submit(fom, 1);

	changed = 0;
}

function earlyInit() {
	show();
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

<input type="hidden" name="_nextpage" value="advanced-tor.asp">
<input type="hidden" name="_service" value="">
<input type="hidden" name="_nofootermsg">
<input type="hidden" name="tor_enable">
<input type="hidden" name="tor_solve_only">

<!-- / / / -->

<div class="section-title">Status</div>
<div class="section">
	<div class="fields">
		<span id="_tor_notice"></span>
		<input type="button" id="_tor_button" value="">
		&nbsp; <img src="spin.gif" alt="" id="spin">
	</div>
</div>

<!-- / / / -->

<div class="section-title">TOR Settings</div>
<div class="section" id="config-section">
	<script>
		createFieldTable('', [
			{ title: 'Enable on Start', name: 'f_tor_enable', type: 'checkbox', value: nvram.tor_enable == 1 },
			null,
			{ title: 'Socks Port', name: 'tor_socksport', type: 'text', maxlen: 5, size: 7, value: fixPort(nvram.tor_socksport, 9050) },
			{ title: 'Trans Port', name: 'tor_transport', type: 'text', maxlen: 5, size: 7, value: fixPort(nvram.tor_transport, 9040) },
			{ title: 'DNS Port', name: 'tor_dnsport', type: 'text', maxlen: 5, size: 7, value: fixPort(nvram.tor_dnsport, 9053) },
			{ title: 'Data Directory', name: 'tor_datadir', type: 'text', maxlen: 24, size: 28, value: nvram.tor_datadir },
			null,
			{ title: 'Only solve .onion/.exit domains<br>(or use Tor as a proxy)', name: 'f_tor_solve_only', type: 'checkbox', value: nvram.tor_solve_only == 1 },
			{ title: 'Redirect all users from', multi: [
				{ name: 'tor_iface', type: 'select', options: [['br0','LAN0 (br0)'],['br1','LAN1 (br1)'],['br2','LAN2 (br2)'],['br3','LAN3 (br3)'],['custom','Selected IP`s']], value: nvram.tor_iface },
				{ name: 'tor_users', type: 'text', maxlen: 512, size: 64, value: nvram.tor_users } ] },
			{ title: 'Redirect TCP Ports', multi: [
				{ name: 'tor_ports', type: 'select', options: [['80','HTTP only (TCP 80)'],['80,443','HTTP/HTTPS (TCP 80,443)'],['custom','Selected Ports']], value: nvram.tor_ports },
				{ name: 'tor_ports_custom', type: 'text', maxlen: 512, size: 64, value: nvram.tor_ports_custom } ] },
			null,
			{ title: 'Custom Configuration', name: 'tor_custom', type: 'textarea', value: nvram.tor_custom }
		]);
	</script>
</div>

<!-- / / / -->

<div class="section-title">Notes</div>
<div class="section">
	<ul>
		<li><b>Enable on Start</b> - Be patient. Starting the Tor client can take from several seconds to several minutes.</li>
		<li><b>Selected IP`s</b> - ex: 1.2.3.4,1.1.0/24,1.2.3.1-1.2.3.4</li>
		<li><b>Selected Ports</b> - ex: one port (80), few ports (80,443,8888), range of ports (80:88), mix (80,8000:9000,9999)</li>
		<li><b>Only solve .onion/.exit domains</b> - Also used to set Tor as a proxy only (remember to add 'SocksPort ROUTER_IP:PORT' and 'SocksPolicy accept CLIENT_IP' to Custom Config. Disables <a href="advanced-dhcpdns.asp">'DNS Rebind protection'</a>)</li>
		<li><b style="text-decoration:underline">Caution!</b> - If your router has only 32MB of RAM, you'll have to use swap.</li>
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
