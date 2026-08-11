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
<link rel="stylesheet" type="text/css" href="tomato.css?rel=<% version(); %>">
<% css(); %>
<script src="isup.jsx?_http_id=<% nv(http_id); %>"></script>
<script src="tomato.js?rel=<% version(); %>"></script>
<script src="interfaces.js?rel=<% version(); %>"></script>

<script>

//	<% nvram("dmz_enable,dmz_ipaddr,dmz_sip,dmz_ra"); %>

//	<% lanip(1); %>

function show() {
	elem.setInnerHTML('notice_container', '<div id="notice">'+isup.notice_iptables.replace(/\n/g, '<br>')+'<\/div><br style="clear:both">');
	elem.display('notice_container', isup.notice_iptables != '');
}

function verifyFields(focused, quiet) {
	var sip, dip, ra, off;

	off = !E('_f_dmz_enable').checked;

	dip = E('_f_dmz_ipaddr');
	dip.disabled = off;

	sip = E('_f_dmz_sip');
	sip.disabled = off;

	ra = E('_f_dmz_ra');
	ra.disabled = off;

	if (off) {
		ferror.clearAll(dip, sip);
		return 1;
	}

	dip.value = dip.value.trim();

	if (!v_ip(dip, quiet))
		return 0;

	if (lanip.indexOf(dip.value.substr(0, dip.value.lastIndexOf('.'))) == -1) {
		ferror.set(dip, 'The specified IP address is outside the range of any enabled LAN', quiet);
		return 0;
	}

	sip.value = sip.value.trim();
	if (sip.value.length && !v_iptaddr(sip, quiet, 15))
		return 0;

	return 1;
}

function save() {
	var fom;

	if (!verifyFields(null, 0))
		return;

	fom = E('t_fom');
	fom.dmz_enable.value = E('_f_dmz_enable').checked ? 1 : 0;
	nvram.dmz_enable = fom.dmz_enable.value;
	fom.dmz_ra.value = E('_f_dmz_ra').checked ? 1 : 0;
	fom.dmz_ipaddr.value = E('_f_dmz_ipaddr').value;
	fom.dmz_sip.value = E('_f_dmz_sip').value.split(/\s*,\s*/).join(',');

	form.submit(fom, 1);
}

function init() {
	verifyFields(null, 1);
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
				{ title: 'Source Address<br>Restriction', indent: 2, name: 'f_dmz_sip', type: 'text', maxlen: 512, size: 64, value: nvram.dmz_sip, suffix: ' &nbsp;<br><small>optional; ex: "1.1.1.1", "1.1.1.0/24", "1.1.1.1-2.2.2.2" or "me.example.com"<\/small>' },
				null,
				{ title: 'Keep remote-access on the router', indent: 2, name: 'f_dmz_ra', type: 'checkbox', value: (nvram.dmz_ra == '1'), suffix: ' &nbsp;<small>Remote access port like SSH and WEB as per admin-access page config will not be forwarded to the DMZ target<\/small>' }
		]);
	</script>
</div>

<!-- / / / -->

<div id="notice_container" style="display:none">&nbsp;</div>

<!-- / / / -->

<script>writeFooter();</script>

</td></tr>
</table>
</form>
<script>insOvl();verifyFields(null, 1);</script>
</body>
</html>
