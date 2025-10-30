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
<title>[<% ident(); %>] Admin: SNMP</title>
<link rel="stylesheet" type="text/css" href="tomato.css?rel=<% version(); %>">
<% css(); %>
<script src="tomato.js?rel=<% version(); %>"></script>

<script>

//	<% nvram("snmp_enable,snmp_port,snmp_remote,snmp_remote_sip,snmp_location,snmp_contact,snmp_ro,snmp_name,snmp_descr"); %>

var xob = null;

function _snmpNvramAdd() {
	form.submitHidden('service.cgi', { _service: 'snmp_nvram-start', _sleep: 1 });
}

function snmpNvramAdd() {
	var sb, cb, msg;

	/* short check for snmp nvram var. If OK - nothing to do! */
	if ((nvram.snmp_port.length > 0) &&
	    (nvram.snmp_remote.length > 0))
		return;

	/* check already enabled? - nothing to do! */
	if (nvram.snmp_enable > 0)
		return;

	E('_f_snmp_enable').disabled = 1;
	if ((sb = E('save-button')) != null) sb.disabled = 1;
	if ((cb = E('cancel-button')) != null) cb.disabled = 1;

	if (!confirm("Add SNMP to nvram?"))
		return;

	if (xob)
		return;

	if ((xob = new XmlHttp()) == null) {
		_snmpNvramAdd();
		return;
	}

	if ((msg = E('footer-msg')) != null) {
		msg.innerHTML = 'adding nvram values...';
		msg.style.display = 'inline';
	}

	xob.onCompleted = function(text, xml) {
		if (msg) {
			msg.innerHTML = 'nvram ready';
		}
		setTimeout(
			function() {
				E('_f_snmp_enable').disabled = 0;
				if (sb) sb.disabled = 0;
				if (cb) cb.disabled = 0;
				if (msg) msg.style.display = 'none';
				setTimeout(reloadPage, 1000);
		}, 5000);
		xob = null;
	}
	xob.onError = function() {
		_snmpNvramAdd();
	}

	xob.post('service.cgi', '_service=snmp_nvram-start'+'&'+'_sleep=1'+'&'+'_ajax=1');
}

function verifyFields(focused, quiet) {
	var a, b, ok = 1;

	a = E('_f_snmp_enable').checked;
	E('_snmp_port').disabled = !a;
	E('_f_snmp_remote').disabled = !a;
	E('_f_snmp_remote_sip').disabled = !a;
	E('_snmp_location').disabled = !a;
	E('_snmp_contact').disabled = !a;
	E('_snmp_ro').disabled = !a;
	E('_snmp_name').disabled = !a;
	E('_snmp_descr').disabled = !a;
	E('_f_snmp_remote_sip').disabled = (!a || !E('_f_snmp_remote').checked);

	b = E('_f_snmp_remote_sip');
	if ((b.value.length) && (!_v_iptaddr(b, quiet || !ok, 15, 1, 1)))
		ok = 0;
	else
		ferror.clear(b);

	b = E('_f_snmp_remote_sip');
	if (b.value.length > 512) {
		ferror.set(b, 'Value cannot be longer than 512 characters', quiet || !ok);
		ok = 0;
	}
	else
		ferror.clear(b);

	b = E('_snmp_name');
	if (b.value.length > 40) {
		ferror.set(b, 'Value cannot be longer than 40 characters', quiet || !ok);
		ok = 0;
	}
	else
		ferror.clear(b);

	b = E('_snmp_descr');
	if (b.value.length > 40) {
		ferror.set(b, 'Value cannot be longer than 40 characters', quiet || !ok);
		ok = 0;
	}
	else
		ferror.clear(b);

	b = E('_snmp_location');
	if (b.value.length > 40) {
		ferror.set(b, 'Value cannot be longer than 40 characters', quiet || !ok);
		ok = 0;
	}
	else
		ferror.clear(b);

	b = E('_snmp_contact');
	if (b.value.length > 40) {
		ferror.set(b, 'Value cannot be longer than 40 characters', quiet || !ok);
		ok = 0;
	}
	else
		ferror.clear(b);

	b = E('_snmp_ro');
	if (b.value.length > 40) {
		ferror.set(b, 'Value cannot be longer than 40 characters', quiet || !ok);
		ok = 0;
	}
	else
		ferror.clear(b);

	return ok;
}

function save() {
	if (verifyFields(null, 0) == 0) return;

	var fom = E('t_fom');
	fom.snmp_enable.value = E('_f_snmp_enable').checked ? 1 : 0;
	fom.snmp_remote.value = E('_f_snmp_remote').checked ? 1 : 0;

	fom.snmp_remote_sip.value = fom.f_snmp_remote_sip.value.split(/\s*,\s*/).join(',');

	if (fom.snmp_enable.value == 0)
		fom._service.value = 'snmp-stop';
	else
		fom._service.value = 'snmp-restart,firewall-restart'; 

	form.submit(fom, 1);
}

function init() {
	snmpNvramAdd();
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

<input type="hidden" name="_nextpage" value="admin-snmp.asp">
<input type="hidden" name="_service" value="snmp-restart,firewall-restart">
<input type="hidden" name="snmp_enable">
<input type="hidden" name="snmp_remote">
<input type="hidden" name="snmp_remote_sip">

<!-- / / / -->

<div class="section-title">SNMP Settings</div>
<div class="section" id="config-section">
	<script>
		createFieldTable('', [
			{ title: 'Enable SNMP', name: 'f_snmp_enable', type: 'checkbox', value: nvram.snmp_enable == 1 },
			null,
			{ title: 'Port', name: 'snmp_port', type: 'text', maxlen: 5, size: 7, value: fixPort(nvram.snmp_port, 161) },
				{ title: 'Remote access', indent: 2, name: 'f_snmp_remote', type: 'checkbox', value: nvram.snmp_remote == 1 },
				{ title: 'Allowed Remote<br>IP Address', indent: 2, name: 'f_snmp_remote_sip', type: 'text', maxlen: 512, size: 64, value: nvram.snmp_remote_sip,
					suffix: '<br><small>eg: 1.2.3.4, 1.2.3.4/24, 1.2.3.4-1.2.3.255, me.example.com - comma separated<\/small>' },
				null,
				{ title: 'Location', indent: 2, name: 'snmp_location', type: 'text', placeholder: 'router', maxlen: 40, size: 64, value: nvram.snmp_location },
				{ title: 'Contact', indent: 2, name: 'snmp_contact', type: 'text', placeholder: 'admin@freshtomato', maxlen: 40, size: 64, value: nvram.snmp_contact },
				{ title: 'RO Community', indent: 2, name: 'snmp_ro', type: 'text', placeholder: 'rocommunity', maxlen: 40, size: 64, value: nvram.snmp_ro },
				{ title: 'Name', indent: 2, name: 'snmp_name', type: 'text', placeholder: 'FreshTomato', maxlen: 40, size: 64, value: nvram.snmp_name },
				{ title: 'Description', indent: 2, name: 'snmp_descr', type: 'text', placeholder: 'router1', maxlen: 40, size: 64, value: nvram.snmp_descr }
		]);
	</script>
</div>

<div class="section-title">Notes</div>
<div class="section">
	<ul>
		<li><b>NVRAM</b> - If SNMP has been enabled, NVRAM values will be added. These NVRAM values will be removed after a reboot if the service is disabled.</li>
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
<script>verifyFields(null, true);</script>
</body>
</html>
