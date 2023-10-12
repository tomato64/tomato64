<!DOCTYPE html>
<!--
	Tomato GUI
	Copyright (C) 2006-2010 Jonathan Zarate
	http://www.polarcloud.com/tomato/

	Shell Web GUI
	Copyright (C) 2023 Lance Fredrickson
	lancethepants@gmail.com

	For use with Tomato Firmware only.
	No part of this file may be used without permission.
-->
<html lang="en-GB">
<head>
<meta http-equiv="content-type" content="text/html;charset=utf-8">
<meta name="robots" content="noindex,nofollow">
<title>[<% ident(); %>] Tools: System Commands</title>
<link rel="stylesheet" type="text/css" href="tomato.css">
<% css(); %>
<script src="tomato.js"></script>
</head>

<script>
	function loadIframe(){
		var x = location.hostname;
		document.getElementById("terminal").src=location.protocol+'//'+location.hostname+':7681';
    	}
</script>

<body onLoad="loadIframe()">
<table id="container">
<tr><td colspan="2" id="header">
	<div class="title">Tomato64</div>
	<div class="version">Version <% version(); %> on <% nv("t_model_name"); %></div>
</td></tr>
<tr id="body"><td id="navi"><script>navi()</script></td>
<td id="content">
<div id="ident"><% ident(); %> | <script>wikiLink();</script></div>

<div class="section-title">Execute System Commands</div>
<div class="section">
	<iframe id="terminal" height="500" width="760" title="Terminal"></iframe>
	<input type="button" value="Open in New Tab" class="new_window" onclick="window.open(location.protocol+'//'+location.hostname+':7681')"><br><br>
	<input type="button" value="Open Legacy System Commands" onclick="window.open(location.protocol+'//'+location.hostname+':'+location.port+'/tools-shell-legacy.asp','_self')"><br>
</div>

<div class="section-title">Notes</div>
<div class="section">
	<ul>
		<li>If you remote in over ssh, do a Local Port Forward of Port 7681 for shell access on this page.</li>
		<li>Or you can use the Legacy System Commands page.</li>
	</ul>
</div>

</td></tr>
</table>
</form>
</body>
</html>
