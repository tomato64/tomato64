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
<title>[<% ident(); %>] Clear Cookies</title>
<link rel="stylesheet" type="text/css" href="tomato.css">
<% css(); %>
<script src="tomato.js"></script>
</head>

<body>
<table id="container">
<tr><td colspan="2" id="header">
	<div class="title">FreshTomato</div>
	<div class="version">Version <% version(); %> on <% nv("t_model_name"); %></div>
</td></tr>
<tr id="body">
<td id="navi"><script>navi()</script></td>
<td id="content">
<div id="ident"><% ident(); %> | <script>wikiLink();</script></div>

<!-- / / / -->

<b>Cookies Cleared</b>

<script>
	b = [];
	c = document.cookie.split(';');
	for (i = 0; i < c.length; ++i) {
		if (c[i].match(/^\s*tomato_(.+?)=/)) {
			b.push('<li>' + c[i]);
			cookie.unset(RegExp.$1);
		}
	}
	if (b.length == 0) b.push('<li>no cookie found<\/li>');
		W('<ul>' + b.join('') + '<\/ul>');
</script>

<!-- / / / -->

<div id="footer">
	&nbsp;
</div>

</td></tr>
</table>
</body>
</html>
