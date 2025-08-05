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
<title>[<% ident(); %>] Admin: CIFS Client</title>
<link rel="stylesheet" type="text/css" href="tomato.css?rel=<% version(); %>">
<% css(); %>
<script src="isup.jsz?_http_id=<% nv(http_id); %>"></script>
<script src="tomato.js?rel=<% version(); %>"></script>

<script>

//	<% nvram("cifs1,cifs2"); %>

//	<% statfs("/cifs1", "cifs1"); %>

//	<% statfs("/cifs2", "cifs2"); %>

function c(id, htm) {
	E(id).cells[1].innerHTML = htm;
}

function show() {
	elem.setInnerHTML('notice_container', '<div id="notice">'+isup.notice_cifs.replace(/\n/g, '<br>')+'<\/div><br style="clear:both">');
	elem.display('notice_container', isup.notice_cifs != '');
	c('cifs1_size', cifs1.mnt ? (scaleSize(cifs1.size)+' / '+scaleSize(cifs1.free)) : '(not mounted)');
	c('cifs2_size', cifs2.mnt ? (scaleSize(cifs2.size)+' / '+scaleSize(cifs2.free)) : '(not mounted)');
}

function verifyFields(focused, quiet) {
	var i, p, b;
	var unc, user, pass, dom, exec, servern, sec;

	for (i = 1; i <= 2; ++i) {
		p = '_f_cifs'+i;
		unc = E(p+'_unc');
		user = E(p+'_user');
		pass = E(p+'_pass');
		dom = E(p+'_dom');
		exec = E(p+'_exec');
		servern = E(p+'_servern');
		sec = E(p+'_sec');

		b = !E(p+'_enable').checked;
		unc.disabled = b;
		user.disabled = b;
		pass.disabled = b;
		exec.disabled = b;
		dom.disabled = b;
		servern.disabled = b;
		sec.disabled = b;
		if (!b) {
			if ((!v_nodelim(unc, quiet, 'UNC')) || (!v_nodelim(user, quiet, 'username')) || (!v_nodelim(pass, quiet, 'password')) ||
				 (!v_nodelim(servern, quiet, 'Netbios name')) ||
				 (!v_nodelim(dom, quiet, 'domain')) || (!v_nodelim(exec, quiet, 'exec path'))) return 0;

			if ((!v_length(user, quiet, 1)) || (!v_length(pass, quiet, 1))) return 0;

			unc.value = unc.value.replace(/\//g, '\\');
			if (!unc.value.match(/^\\\\.+\\/)) {
				ferror.set(unc, 'Invalid UNC', quiet);
				return 0;
			}
		}
		else
			ferror.clear(unc, user, pass, dom, exec, servern, sec);
	}

	return 1;
}

function save() {
	var i, p;

	if (!verifyFields(null, 0)) return;

	for (i = 1; i <= 2; ++i) {
		p = '_f_cifs'+i;
		E('cifs'+i).value = (E(p+'_enable').checked ? '1' : '0')+'<'+E(p+'_unc').value+'<'+E(p+'_user').value+'<'+E(p+'_pass').value+'<'+
				     E(p+'_dom').value+'<'+E(p+'_exec').value+'<'+E(p+'_servern').value+'<'+E(p+'_sec').value;
	}

	form.submit('t_fom', 1);
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
	<div class="version">Version <% version(); %> on <% nv("t_model_name"); %></div>
</td></tr>
<tr id="body"><td id="navi"><script>navi()</script></td>
<td id="content">
<div id="ident"><% ident(); %> | <script>wikiLink();</script></div>

<!-- / / / -->

<input type="hidden" name="_nextpage" value="admin-cifs.asp">
<input type="hidden" name="_service" value="cifs-restart">
<input type="hidden" name="cifs1" id="cifs1">
<input type="hidden" name="cifs2" id="cifs2">

<!-- / / / -->

<div class="section-title">CIFS Client</div>
<div class="section">
	<script>
		a = b = [0, '\\\\192.168.1.5\\shared_example', '', '', '', '', '', ''];

		if (r = nvram.cifs1.match(/^(0|1)<(\\\\.+)<(.*)<(.*)<(.*)<(.*)<(.*)<(.*)$/)) a = r.splice(1, 8);
		if (r = nvram.cifs2.match(/^(0|1)<(\\\\.+)<(.*)<(.*)<(.*)<(.*)<(.*)<(.*)$/)) b = r.splice(1, 8);

		createFieldTable('', [
			{ title: '/cifs1' },
			{ title: 'Enable', indent: 2, name: 'f_cifs1_enable', type: 'checkbox', value: a[0]*1 },
			{ title: 'UNC', indent: 2, name: 'f_cifs1_unc', type: 'text', maxlen: 128, size: 64, value: a[1] },
			{ title: 'Netbios Name', indent: 2, name: 'f_cifs1_servern', type: 'text', maxlen: 128, size: 64, value: a[6] },
			{ title: 'Username', indent: 2, name: 'f_cifs1_user', type: 'text', maxlen: 32, size: 34, value: a[2] },
			{ title: 'Password', indent: 2, name: 'f_cifs1_pass', type: 'password', maxlen: 32, size: 34, peekaboo: 1, value: a[3] },
			{ title: 'Domain', indent: 2, name: 'f_cifs1_dom', type: 'text', maxlen: 32, size: 34, value: a[4] },
			{ title: 'Execute When Mounted', indent: 2, name: 'f_cifs1_exec', type: 'text', maxlen: 64, size: 34, value: a[5] },
			{ title: 'Security', indent: 2, name: 'f_cifs1_sec', type: 'select',
				options: [['','Default (NTLM)'],['ntlmi','NTLM and packet signing'],['ntlmv2','NTLMv2'],['ntlmv2i','NTLMv2 and packet signing'],['nontlm','No NTLM'],['lanman','LANMAN'],['none','None']],
				value: a[7] },
			{ title: 'Total / Free Size', indent: 2, rid: 'cifs1_size', text: cifs1.mnt ? (scaleSize(cifs1.size)+' / '+scaleSize(cifs1.free)) : '(not mounted)' },
			null,
			{ title: '/cifs2' },
			{ title: 'Enable', indent: 2, name: 'f_cifs2_enable', type: 'checkbox', value: b[0]*1 },
			{ title: 'UNC', indent: 2, name: 'f_cifs2_unc', type: 'text', maxlen: 128, size: 64, value: b[1] },
			{ title: 'Netbios Name', indent: 2, name: 'f_cifs2_servern', type: 'text', maxlen: 128, size: 64, value: b[6] },
			{ title: 'Username', indent: 2, name: 'f_cifs2_user', type: 'text', maxlen: 32, size: 34, value: b[2] },
			{ title: 'Password', indent: 2, name: 'f_cifs2_pass', type: 'password', maxlen: 32, size: 34, peekaboo: 1, value: b[3] },
			{ title: 'Domain', indent: 2, name: 'f_cifs2_dom', type: 'text', maxlen: 32, size: 34, value: b[4] },
			{ title: 'Execute When Mounted', indent: 2, name: 'f_cifs2_exec', type: 'text', maxlen: 64, size: 34, value: b[5] },
			{ title: 'Security', indent: 2, name: 'f_cifs2_sec', type: 'select',
				options: [['','Default (NTLM)'],['ntlmi','NTLM and packet signing'],['ntlmv2','NTLMv2'],['ntlmv2i','NTLMv2 and packet signing'],['nontlm','No NTLM'],['lanman','LANMAN'],['none','None']],
				value: b[7] },
			{ title: 'Total / Free Size', indent: 2, rid: 'cifs2_size', text: cifs2.mnt ? (scaleSize(cifs2.size)+' / '+scaleSize(cifs2.free)) : '(not mounted)' }
		]);
	</script>
</div>

<!-- / / / -->

<div id="notice_container" style="display:none">&nbsp;</div>

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
