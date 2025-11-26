<!DOCTYPE html>
<!--
	Tomato64 Port Labels Configuration
	Copyright (C) 2025 Lance Fredrickson
	lancethepants@gmail.com
-->

<html lang="en-GB">
<head>
<meta http-equiv="content-type" content="text/html;charset=utf-8">
<meta name="robots" content="noindex,nofollow">
<title>[<% ident(); %>] Advanced: Port Labels</title>
<link rel="stylesheet" type="text/css" href="tomato.css?rel=<% version(); %>">
<% css(); %>
<script src="isup.jsx?_http_id=<% nv(http_id); %>"></script>
<script src="tomato.js?rel=<% version(); %>"></script>

<script>
//	<% nvram("t_model_name,nics,port0_label,port1_label,port2_label,port3_label,port4_label,port5_label,port6_label,port7_label,port8_label"); %>

var cprefix = 'advanced_port_labels';
var MAX_PORT_LABEL_LENGTH = 20;

var ref = new TomatoRefresh('isup.jsx', '', 2);

ref.refresh = function(text) {
	try {
		eval(text);
	}
	catch (ex) {
	}
	updatePortStatus();
}

function updatePortStatus() {
	var nicCount = nvram.nics ? parseInt(nvram.nics) : 0;
	for (var p = 0; p < nicCount; p++) {
		var elem = E('port_status_'+p);
		if (!elem) continue;

		var port = etherstates['port'+p];
		var state = _ethstates(port);
		elem.innerHTML = '<img src="'+state[0]+'.gif" alt="" title="'+state[1]+'">';
	}
}

function verifyFields(focused, quiet) {
	var nicCount = nvram.nics ? parseInt(nvram.nics) : 0;
	for (var p = 0; p < nicCount; p++) {
		var elem = E('_port'+p+'_label');
		if (!elem) continue;

		var value = elem.value.trim();
		if (value.length > MAX_PORT_LABEL_LENGTH) {
			ferror.set(elem, 'Label must be '+MAX_PORT_LABEL_LENGTH+' characters or less', quiet);
			return 0;
		}
		ferror.clear(elem);
	}
	return 1;
}

function save() {
	if (!verifyFields(null, false)) return;

	var fom = E('t_fom');
	var nicCount = nvram.nics ? parseInt(nvram.nics) : 0;
	for (var p = 0; p < nicCount; p++) {
		var elem = E('_port'+p+'_label');
		if (elem) {
			fom['port'+p+'_label'].value = elem.value.trim();
		}
	}

	form.submit(fom); /* Submit to saved.asp which will redirect */
}

function init() {
	var c;
	if (((c = cookie.get(cprefix+'_notes_vis')) != null) && (c == '1'))
		toggleVisibility(cprefix, 'notes');

	updatePortStatus();
	ref.initPage(2000, 1);
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

<input type="hidden" name="_nextpage" value="status-overview.asp">
<input type="hidden" name="_nextwait" value="1">
<input type="hidden" name="_service" value="">
<input type="hidden" name="port0_label">
<input type="hidden" name="port1_label">
<input type="hidden" name="port2_label">
<input type="hidden" name="port3_label">
<input type="hidden" name="port4_label">
<input type="hidden" name="port5_label">
<input type="hidden" name="port6_label">
<input type="hidden" name="port7_label">
<input type="hidden" name="port8_label">

<div class="section-title">Ethernet Port Labels</div>
<div class="section">
	<table class="fields">
		<script>
		var nicCount = nvram.nics ? parseInt(nvram.nics) : 0;
		for (var p = 0; p < nicCount; p++) {
			var hwLabel = PortNames.getHardwareLabel(p);
			var customLabel = nvram['port'+p+'_label'] || '';

			W('<tr style="height:55px">');
			W('<td class="title indent2" style="width:120px;vertical-align:middle">');
			W('<span id="port_status_'+p+'" style="vertical-align:middle"><img src="eth_x.gif" alt="" style="vertical-align:middle"></span> ');
			W('<b style="vertical-align:middle">'+hwLabel+'</b>');
			W('</td>');
			W('<td class="content" style="vertical-align:middle">');
			W('<input type="text" maxlength="'+MAX_PORT_LABEL_LENGTH+'" size="25" id="_port'+p+'_label" value="'+escapeHTML(customLabel)+'" onchange="verifyFields(this, true)" style="vertical-align:middle"> ');
			W('<span class="text-info" style="vertical-align:middle">(optional custom label)</span>');
			W('</td>');
			W('</tr>');
		}
		</script>
	</table>
</div>

<!-- / / / -->

<div class="section-title">Notes <small><i><a href="javascript:toggleVisibility(cprefix,'notes');" id="toggleLink-notes"><span id="sesdiv_notes_showhide">(Show)</span></a></i></small></div>
<div class="section" id="sesdiv_notes" style="display:none">
	<ul>
		<li><b>Custom labels</b> - Optional custom labels for ethernet ports that appear on the Status Overview page above the hardware port name.</li>
		<li><b>Label length</b> - Labels are limited to <script>W(MAX_PORT_LABEL_LENGTH)</script> characters.</li>
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
