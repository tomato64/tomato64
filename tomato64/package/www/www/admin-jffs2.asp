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
<title>[<% ident(); %>] Admin: JFFS</title>
<link rel="stylesheet" type="text/css" href="tomato.css?rel=<% version(); %>">
<% css(); %>
<script src="isup.jsx?_http_id=<% nv(http_id); %>"></script>
<script src="tomato.js?rel=<% version(); %>"></script>

<script>

//	<% nvram("jffs2_on,jffs2_exec"); %>

function show() {
/* JFFS2-BEGIN */
	statfs = jffs2;
/* JFFS2-END */
/* JFFS2NAND-BEGIN */
	statfs = brcmnand;
/* JFFS2NAND-END */
	elem.setInnerHTML('msg_container', (((statfs.mnt) || (statfs.size > 0)) ? scaleSize(statfs.size) : '')+((statfs.mnt) ? ' / '+scaleSize(statfs.free) : ' / (not mounted)'));

	if (isup.notice_jffs != '') {
		elem.setInnerHTML('notice_container', '<div id="notice">'+isup.notice_jffs.replace(/\n/g, '<br>')+'<\/div><br style="clear:both">');
		E('format').disabled = 0;
	}

	elem.display('notice_container', isup.notice_jffs != '');
}

function formatJffs() {
	if (!verifyFields(null, 0)) return;
	if (!confirm("Format the JFFS partition?")) return;
	save(1);
}

function verifyFields(focused, quiet) {
	var b = !E('_f_jffs2_on').checked;
	E('format').disabled = b;
	E('_jffs2_exec').disabled = b;

	return 1;
}

function save(format) {
	if (!verifyFields(null, 0)) return;

	E('format').disabled = 1;
	if (format) {
		elem.setInnerHTML('notice_container', '<div id="notice"><img src="spin.gif" alt="" id="spin" style="display:inline-block"> <span>Please wait ...<\/span><\/div><br style="clear:both">');
		elem.display('notice_container', 1);
	}

	var fom = E('t_fom');
	var on = E('_f_jffs2_on').checked ? 1 : 0;
	fom.jffs2_on.value = on;
	if (format) {
		fom.jffs2_format.value = 1;
		fom._commit.value = 0;
	}
	else {
		fom.jffs2_format.value = 0;
		fom._commit.value = 1;
	}

	form.submit(fom, 1);

	if (format) E('footer-msg').style.display = 'none';
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

<input type="hidden" name="_nextpage" value="admin-jffs2.asp">
<input type="hidden" name="_service" value="jffs2-restart">
<input type="hidden" name="_commit" value="1">
<input type="hidden" name="jffs2_on">
<input type="hidden" name="jffs2_format" value="0">

<!-- / / / -->

<div class="section-title">JFFS</div>
<div class="section">
	<script>
		createFieldTable('', [
			{ title: 'Enable', name: 'f_jffs2_on', type: 'checkbox', value: (nvram.jffs2_on == 1) },
			{ title: 'Execute when mounted', name: 'jffs2_exec', type: 'text', maxlen: 64, size: 34, value: nvram.jffs2_exec },
			null,
			{ title: 'Total / Free Size', text: '<span id="msg_container">&nbsp;<\/span>' },
			null,
			{ title: '', custom: '<input type="button" value="Format & Load" onclick="formatJffs()" id="format">' }
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
