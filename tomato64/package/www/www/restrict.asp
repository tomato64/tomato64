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
<title>[<% ident(); %>] Access Restrictions</title>
<link rel="stylesheet" type="text/css" href="tomato.css">
<% css(); %>
<script src="tomato.js"></script>

<script>

//	<% nvram('bcmnat_disable'); %>

//	<% nvramseq("rrules", "rrule%d", 0, 99); %>

var dowNames = ['Sun', 'Mon', 'Tue', 'Wed', 'Thu', 'Fri', 'Sat'];

var og = new TomatoGrid();

og.setup = function() {
	this.init('res-over-grid', 'sort');
	this.headerSet(['Description', 'Schedule']);
	var r = this.footerSet(['<input type="button" value="Add" onclick="TGO(this).addEntry()" id="res-add-button">']);
	r.cells[0].colSpan = 2;
}

og.populate = function() {
	this.removeAllData();
	for (var i = 0; i < rrules.length; ++i) {
		var v;
		if ((v = rrules[i].match(/^(\d+)\|(-?\d+)\|(-?\d+)\|(\d+)\|(.*?)\|(.*?)\|([^|]*?)\|(\d+)\|(.*)$/m)) == null) {
			rrules[i] = '';
			continue;
		}
		v = v.slice(1);
		if (isNaN(v[1] *= 1))
			continue;
		if (isNaN(v[2] *= 1))
			continue;
		if (isNaN(v[3] *= 1))
			continue;

		var s = '';
		if (v[3] == 0x7F)
			s += 'Everyday';
		else {
			for (var j = 0; j < 7; ++j) {
				if (v[3] & (1 << j)) {
					if (s.length) s += ', ';
					s += dowNames[j];
				}
			}
		}

		if ((v[1] >= 0) && (v[2] >= 0)) {
			s += '<br>'+timeString(v[1])+' to '+timeString(v[2]);
			if (v[2] <= v[1])
				s += ' <small>(the following day)<\/small>';
		}
		if (v[0] != '1')
			s += '<br><i><b>Disabled<\/b><\/i>';

		this.insertData(-1, [i, v[8], s]);
	}
	og.sort(0);
}

og.dataToView = function(data) {
	return [escapeHTML(data[1]), data[2]];
}

og.onClick = function(cell) {
	E('t_rruleN').value = PR(cell).getRowData()[0];
	form.submit('t_fom');
}

og.addEntry = function() {
	E('t_rruleN').value = -1;
	form.submit('t_fom');
}

function init() {
/* BCMNAT-BEGIN */
	if (nvram.bcmnat_disable == 0)
		E('bcmnatnotice').style.display = 'block';
	else
		E('bcmnatnotice').style.display = 'none';
/* BCMNAT-END */

	og.populate();
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

<input type="hidden" name="_redirect" value="restrict-edit.asp">
<input type="hidden" name="_commit" value="0">
<input type="hidden" name="rruleN" id="t_rruleN" value="">

<!-- / / / -->

<div class="section-title">Access Restriction Overview</div>
<div class="section">
<!-- BCMNAT-BEGIN -->
	<div class="fields" id="bcmnatnotice" style="display:none"><div class="about"><b><a href="advanced-misc.asp">Broadcom FastNAT is enabled</a> but it won't work if you turn on even one rule here.</b></div></div>
<!-- BCMNAT-END -->
	<div class="tomato-grid" id="res-over-grid"></div>
</div>

<script>show_notice1('<% notice("iptables"); %>');</script>
<script>show_notice1('<% notice("ip6tables"); %>');</script>

<!-- / / / -->

<div id="footer">
	&nbsp;
</div>

</td></tr>
</table>
<script>og.setup();</script>
</form>
</body>
</html>
