<!DOCTYPE html>
<!--
	Tomato GUI
	Copyright (C) 2006-2010 Jonathan Zarate
	http://www.polarcloud.com/tomato/

	IP Traffic enhancements
	Copyright (C) 2011 Augusto Bott
	http://code.google.com/p/tomato-sdhc-vlan/

	For use with Tomato Firmware only.
	No part of this file may be used without permission.
-->
<html lang="en-GB">
<head>
<meta http-equiv="content-type" content="text/html;charset=utf-8">
<meta name="robots" content="noindex,nofollow">
<title>[<% ident(); %>] IP Traffic: Monthly History</title>
<link rel="stylesheet" type="text/css" href="tomato.css?rel=<% version(); %>">
<% css(); %>
<script src="tomato.js?rel=<% version(); %>"></script>
<script src="bwm-hist.js?rel=<% version(); %>"></script>
<script src="interfaces.js?rel=<% version(); %>"></script>

<script>
//	<% jsdefaults(); %>
//	<% devlist(); %>
</script>

<script src="bwm-common.js?rel=<% version(); %>"></script>

<script>
var cprefix = 'ipt_monthly';
try {
//	<% bandwidth("monthly","ipt"); %>
}
catch (ex) {
	monthly_history = [];
}
cstats_busy = 0;
if (typeof(monthly_history) == 'undefined') {
	monthly_history = [];
	cstats_busy = 1;
}

var iptHistory = monthly_history;
var iptHistoryDaily = 0;
var iptFilterIp = '<% cgi_get("ipt_filterip"); %>';
</script>
<script src="ipt-hist.js?rel=<% version(); %>"></script>
</head>

<body onload="init()">
<form>
<table id="container">
<tr><td colspan="2" id="header">
	<div class="title"><a href="/">Tomato64</a></div>
	<div class="version">Version <% version(); %> on <% nv("t_model_name"); %><span class="blinking bl2"><script><% anonupdate(); %> anon_update()</script>&nbsp;</span></div>
</td></tr>
<tr id="body"><td id="navi"><script>navi()</script></td>
<td id="content">
<div id="ident"><% ident(); %> | <script>wikiLink();</script></div>

<!-- / / / -->

<div class="section-title">IP Traffic - Monthly History</div>

<div id="cstats">
	<div class="section">
		<div class="tomato-grid" id="bwm-grid"></div>
	</div>

	<script>writeToggleSectionTitle('Options', 'options');</script>
	<div class="section" id="sesdiv_options" style="display:none">
		<script>writeIptHistoryOptions();</script>
		<div id="bwm-ctrl">
			&raquo; <a href="javascript:genData()">Data</a>
			<br>
			&raquo; <a href="admin-iptraffic.asp">Configure</a>
		</div>
	</div>

</div>

<!-- / / / -->

<script>checkStats('cstats');</script>

<!-- / / / -->

<div id="footer">
	<input type="button" value="Refresh" id="refresh-button" onclick="reloadPage()">
</div>

</td></tr>
</table>
</form>
<script>insOvl()</script>
</body>
</html>
