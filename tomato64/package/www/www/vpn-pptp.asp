<!DOCTYPE html>
<!--
	Tomato GUI
	Copyright (C) 2006-2009 Jonathan Zarate
	http://www.polarcloud.com/tomato/

	Portions Copyright (C) 2010-2011 Jean-Yves Avenard, jean-yves@avenard.org

	For use with Tomato Firmware only.
	No part of this file may be used without permission.
-->
<html lang="en-GB">
<head>
<meta http-equiv="content-type" content="text/html;charset=utf-8">
<meta name="robots" content="noindex,nofollow">
<title>[<% ident(); %>] PPTP: Client</title>
<link rel="stylesheet" type="text/css" href="tomato.css?rel=<% version(); %>">
<% css(); %>
<script src="isup.jsx?_http_id=<% nv(http_id); %>"></script>
<script src="isup.js?rel=<% version(); %>"></script>
<script src="tomato.js?rel=<% version(); %>"></script>

<script>

//	<% nvram("pptpc_eas,pptpc_usewan,pptpc_peerdns,pptpc_mtuenable,pptpc_mtu,pptpc_mruenable,pptpc_mru,pptpc_nat,pptpc_srvip,pptpc_srvsub,pptpc_srvsubmsk,pptpc_username,pptpc_passwd,pptpc_mppeopt,pptpc_crypt,pptpc_custom,pptpc_dfltroute,pptpc_stateless"); %>

var changed = 0;
var serviceType = 'pptpclient';

function verifyFields(focused, quiet) {
	if (focused && focused != E('_f_pptpc_eas')) /* except on/off */
		changed = 1;

	var ok = 1;

	elem.display(PR('_pptpc_srvsub'), PR('_pptpc_srvsubmsk'), !E('_f_pptpc_dfltroute').checked);

	var f = E('_pptpc_mtuenable').value == '0';
	if (f) E('_pptpc_mtu').value = '1400';
	E('_pptpc_mtu').disabled = f;

	f = E('_pptpc_mruenable').value == '0';
	if (f) E('_pptpc_mru').value = '1400';
	E('_pptpc_mru').disabled = f;

	if (!v_range('_pptpc_mtu', quiet || !ok, 576, 1500)) ok = 0;
	if (!v_range('_pptpc_mru', quiet || !ok, 576, 1500)) ok = 0;

	if (!v_ip('_pptpc_srvip', true) && !v_domain('_pptpc_srvip', true)) {
		ferror.set(E('_pptpc_srvip'), "Invalid server address", quiet || !ok);
		ok = 0;
	}
	else
		ferror.clear(E('_pptpc_srvip'));

	if (!E('_f_pptpc_dfltroute').checked && !v_ip('_pptpc_srvsub', true)) {
		ferror.set(E('_pptpc_srvsub'), "Invalid subnet address", quiet || !ok);
		ok = 0;
	}
	else
		ferror.clear(E('_pptpc_srvsub'));

	if (!E('_f_pptpc_dfltroute').checked && !v_ip('_pptpc_srvsubmsk', true)) {
		ferror.set(E('_pptpc_srvsubmsk'), "Invalid netmask address", quiet || !ok);
		ok = 0;
	}
	else
		ferror.clear(E('_pptpc_srvsubmsk'));

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
	fom.pptpc_eas.value = fom._f_pptpc_eas.checked ? 1 : 0;
	fom.pptpc_nat.value = fom._f_pptpc_nat.checked ? 1 : 0;
	fom.pptpc_dfltroute.value = fom._f_pptpc_dfltroute.checked ? 1 : 0;
	fom.pptpc_stateless.value = fom._f_pptpc_stateless.checked ? 1 : 0;
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
<form id="t_fom" method="post" action="/tomato.cgi">
<table id="container">
<tr><td colspan="2" id="header">
	<div class="title"><a href="/">Tomato64</a></div>
	<div class="version">Version <% version(); %> on <% nv("t_model_name"); %></div>
</td></tr>
<tr id="body"><td id="navi"><script>navi()</script></td>
<td id="content">
<div id="ident"><% ident(); %> | <script>wikiLink();</script></div>

<!-- / / / -->

<input type="hidden" name="_nextpage" value="vpn-pptp.asp">
<input type="hidden" name="_service" value="">
<input type="hidden" name="_nofootermsg">
<input type="hidden" name="pptpc_eas">
<input type="hidden" name="pptpc_nat">
<input type="hidden" name="pptpc_dfltroute">
<input type="hidden" name="pptpc_stateless">

<!-- / / / -->

<div class="section-title">Status</div>
<div class="section">
	<div class="fields">
		<span id="_pptpclient_notice"></span><input type="button" id="_pptpclient_button">&nbsp; <img src="spin.svg" alt="" id="spin">
	</div>
</div>

<!-- / / / -->

<div class="section-title"><span class="pptpsvg">&nbsp;</span>PPTP Client Configuration</div>
<div class="section">
	<script>
		createFieldTable('', [
			{ title: 'Enable on Start', name: 'f_pptpc_eas', type: 'checkbox', value: nvram.pptpc_eas != 0 },
			{ title: 'Bind to', name: 'pptpc_usewan', type: 'select',
				options: [['wan','WAN0'],['wan2','WAN1'],
/* MULTIWAN-BEGIN */
					['wan3','WAN2'],['wan4','WAN3'],
/* MULTIWAN-END */
					['none','none']], value: nvram.pptpc_usewan },
			{ title: 'Server Address', name: 'pptpc_srvip', type: 'text', maxlen: 50, size: 27, value: nvram.pptpc_srvip },
			{ title: 'Username ', name: 'pptpc_username', type: 'text', maxlen: 50, size: 54, value: nvram.pptpc_username },
			{ title: 'Password ', name: 'pptpc_passwd', type: 'password', maxlen: 50, size: 54, peekaboo: 1, value: nvram.pptpc_passwd },
			{ title: 'Encryption', name: 'pptpc_crypt', type: 'select', options: [['0','Auto'],['1','None'],['2','Maximum (128 bit only)'],['3','Required (128 or 40 bit)']], value: nvram.pptpc_crypt },
			{ title: 'Stateless MPPE connection', name: 'f_pptpc_stateless', type: 'checkbox', value: nvram.pptpc_stateless != 0 },
			{ title: 'Accept DNS configuration', name: 'pptpc_peerdns', type: 'select', options: [[0,'Disabled'],[1,'Yes'],[2,'Exclusive']], value: nvram.pptpc_peerdns },
			{ title: 'Redirect Internet traffic', name: 'f_pptpc_dfltroute', type: 'checkbox', value: nvram.pptpc_dfltroute != 0 },
			{ title: 'Remote subnet / netmask', multi: [
				{ name: 'pptpc_srvsub', type: 'text', maxlen: 15, size: 17, value: nvram.pptpc_srvsub },
				{ name: 'pptpc_srvsubmsk', type: 'text', maxlen: 15, size: 17, prefix: ' /&nbsp', value: nvram.pptpc_srvsubmsk } ] },
			{ title: 'Create NAT on tunnel', name: 'f_pptpc_nat', type: 'checkbox', value: nvram.pptpc_nat != 0 },
			{ title: 'MTU', multi: [
				{ name: 'pptpc_mtuenable', type: 'select', options: [['0','Default'],['1','Manual']], value: nvram.pptpc_mtuenable },
				{ name: 'pptpc_mtu', type: 'text', maxlen: 4, size: 6, value: nvram.pptpc_mtu } ] },
			{ title: 'MRU', multi: [
				{ name: 'pptpc_mruenable', type: 'select', options: [['0','Default'],['1','Manual']], value: nvram.pptpc_mruenable },
				{ name: 'pptpc_mru', type: 'text', maxlen: 4, size: 6, value: nvram.pptpc_mru } ] },
			{ title: 'Custom Configuration', name: 'pptpc_custom', type: 'textarea', value: nvram.pptpc_custom }
		]);
	</script>
</div>

<!-- / / / -->

<div class="section-title">Notes</div>
<div class="section">
	<ul>
		<li><b>Do not change (and save)</b> the settings when client <b>is running</b> - you may end up with a downed firewall or broken routing table!</li>
		<li>In case of connection problems, reduce the MTU and/or MRU values.</li>
		<li>To boost connection performance, you can try to increase MTU/MRU values.</li>
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
