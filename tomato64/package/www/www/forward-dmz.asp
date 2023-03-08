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
<title>[<% ident(); %>] Forwarding: DMZ</title>
<link rel="stylesheet" type="text/css" href="tomato.css">
<% css(); %>
<script src="tomato.js"></script>
<script src="interfaces.js"></script>

<script>

//	<% nvram("dmz_enable,dmz_ipaddr,dmz_sip,dmz_ra"); %>

//	<% lanip(1); %>

function verifyFields(focused, quiet) {
	var sip, dip, off;

	off = !E('_f_dmz_enable').checked;

	dip = E('_f_dmz_ipaddr')
	dip.disabled = off;

	sip = E('_f_dmz_sip');
	sip.disabled = off;

	sip = E('_f_dmz_ra');
	sip.disabled = off;

	if (off) {
		ferror.clearAll(dip, sip);
		return 1;
	}

	if (nvram.dmz_enable) {
		dip.value = dip.value.trim();

		if (!v_ip(dip, quiet))
			return 0;

		if (lanip.indexOf(dip.value.substr(0, dip.value.lastIndexOf('.'))) == -1) {
			ferror.set(dip, 'The specified IP address is outside the range of any enabled LAN', quiet);
			return 0;
		}
	}

	if ((sip.value.length) && (!v_iptaddr(sip, quiet, 15)))
		return 0;

	return 1;
}

function save() {
	if (!verifyFields(null, 0))
		return;

	var fom = E('t_fom');
	fom.dmz_enable.value = fom.f_dmz_enable.checked ? 1 : 0;
	nvram.dmz_enable = fom.dmz_enable.value;
	fom.dmz_ra.value = fom._f_dmz_ra.checked ? 1 : 0;
	fom. dmz_ipaddr.value = fom._f_dmz_ipaddr.value;
	fom.dmz_sip.value = fom.f_dmz_sip.value.split(/\s*,\s*/).join(',');

	form.submit(fom, 1);
}

function init() {
	verifyFields(null, 1);
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

<input type="hidden" name="_nextpage" value="forward-dmz.asp">
<input type="hidden" name="_service" value="firewall-restart">
<input type="hidden" name="dmz_enable">
<input type="hidden" name="dmz_ipaddr">
<input type="hidden" name="dmz_sip">
<input type="hidden" name="dmz_ra">

<!-- / / / -->

<div class="section-title">DMZ</div>
<div class="section">
	<script>
		createFieldTable('', [
			{ title: 'Enable DMZ', name: 'f_dmz_enable', type: 'checkbox', value: (nvram.dmz_enable == '1') },
			{ title: 'Destination Address', indent: 2, name: 'f_dmz_ipaddr', type: 'text', maxlen: 15, size: 17, value: nvram.dmz_ipaddr },
			{ title: 'Source Address<br>Restriction', indent: 2, name: 'f_dmz_sip', type: 'text', maxlen: 512, size: 64, value: nvram.dmz_sip, suffix: ' &nbsp;<br><small>optional; ex: "1.1.1.1", "1.1.1.0/24", "1.1.1.1 - 2.2.2.2" or "me.example.com"<\/small>' },
			null,
			{ title: 'Leave Remote Access', indent: 2, name: 'f_dmz_ra', type: 'checkbox', value: (nvram.dmz_ra == '1'), suffix: ' &nbsp;<small>Redirect remote access ports for SSH and HTTP(s) to router<\/small>' }
		]);
	</script>
</div>

<!-- / / / -->

<script>
	if (nvram.dmz_enable == 1)
		show_notice1('<% notice("iptables"); %>');
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
<script>verifyFields(null, 1);</script>
</body>
</html>
