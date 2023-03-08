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
<title>[<% ident(); %>] Admin: Scripts</title>
<link rel="stylesheet" type="text/css" href="tomato.css">
<% css(); %>
<script src="tomato.js"></script>

<script>

//	<% nvram("script_init,script_shut,script_fire,script_wanup,script_mwanup"); %>

tabs = [['as-init', 'Init'],['as-shut', 'Halt'],['as-fire','Firewall'],['as-wanup', 'WAN Up (main)'],['as-mwanup', 'MultiWAN Up']];

function tabSelect(name) {
	tabHigh(name);
	for (var i = 0; i < tabs.length; ++i) {
		var on = (name == tabs[i][0]);
		elem.display(tabs[i][0] + '-text', on);
	}
	E(name + '-text').focus();
	cookie.set('scripts_tab', name);
}

function wordWrap() {
	for (var i = 0; i < tabs.length; ++i) {
		var e = E(tabs[i][0] + '-text');
		var v = e.value;
		var s = e.style.display;
		var c = e.cloneNode(false);
		wrap = (E('as-wordwrap').checked ? 1 : 0);
		s = s ? s : "block";
		c.setAttribute('style', wrap ? 'display:' + s + ';white-space:pre-wrap' : 'display:' + s + ';white-space:pre;overflow-wrap:normal;overflow-x:scroll');
		e.parentNode.replaceChild(c, e);
		c.value = v;
		cookie.set('scripts_wrap', wrap);
	}
}

function save() {
	var i, t, n, x;

	for (i = 0; i < tabs.length; ++i) {
		t = tabs[i];
		n = E(t[0] + '-text').value.length;
		x = (t[0] == 'as-fire') ? 8192 : 4096;
		if (n > x) {
			tabSelect(t[0]);
			alert(t[1] + ' script is too long. Maximum allowed is ' + x + ' bytes.');
			return;
		}
	}
	form.submit('t_fom', 1);
}

function earlyInit() {
	for (var i = 0; i < tabs.length; ++i) {
		var t = tabs[i][0];
		E(t + '-text').value = nvram['script_' + t.replace('as-', '')];
	}
	tabSelect(cookie.get('scripts_tab') || 'as-init');
}
</script>
</head>

<body>
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

<input type="hidden" name="_nextpage" value="admin-scripts.asp">

<!-- / / / -->

<div class="section-title">Scripts</div>
<div class="section">
	<script>
		tabCreate.apply(this, tabs);

		wrap = cookie.get('scripts_wrap') || 0;
		s = 'display:none;' + (wrap == 1 ? 'white-space:pre-wrap' : 'white-space:pre;overflow-wrap:normal;overflow-x:scroll');
		for (i = 0; i < tabs.length; ++i) {
			t = tabs[i][0];
			W('<textarea class="as-script" name="script_' + t.replace('as-', '') + '" id="' + t + '-text" style="' + s + '"><\/textarea>');
		}
		W('<div style="margin-top:20px"><input type="checkbox" id="as-wordwrap" onclick="wordWrap()" onchange="wordWrap()" ' + (wrap == 1 ? 'checked' : '') + '>&nbsp; Word Wrap<\/div>');
	</script>
</div>

<!-- / / / -->

<div class="section-title">Notes</div>
<div class="section">
	<ul>
		<li>For <b>MultiWAN Up</b>, the active WAN number is passed as <b>$1</b>.</li>
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
