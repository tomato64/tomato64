<!DOCTYPE html>
<!--
	Tomato GUI
	Copyright (C) 2007-2022 FreshTomato
	ver="v2.72b - 04/23" # rs232
	https://www.freshtomato.org/
	For use with Tomato Firmware only.
	No part of this file may be used without permission.
-->
<html lang="en-GB">
<head>
<meta http-equiv="content-type" content="text/html;charset=utf-8">
<meta name="robots" content="noindex,nofollow">
<title>[<% ident(); %>] Advanced: Adblock (DNS filtering)</title>
<link rel="stylesheet" type="text/css" href="tomato.css">
<% css(); %>
<script src="tomato.js"></script>
<script>

//	<% nvram("adblock_enable,adblock_blacklist,adblock_blacklist_custom,adblock_whitelist,adblock_path,adblock_limit,adblock_logs"); %>

var cprefix = 'advanced_adblock';
var adblockg = new TomatoGrid();
var adblock_refresh = cookie.get(cprefix+'_refresh');

adblockg.exist = function(f, v) {
	var data = this.getAllData();
	for (var i = 0; i < data.length; ++i) {
		if (data[i][f] == v) return true;
	}

	return false;
}

adblockg.dataToView = function(data) {
	return [(data[0] != '0') ? '&#x2b50' : '',data[1],data[2]];
}

adblockg.fieldValuesToData = function(row) {
	var f = fields.getAll(row);

	return [f[0].checked ? 1 : 0,f[1].value,f[2].value];
}

adblockg.verifyFields = function(row, quiet) {
	var ok = 1;

	return ok;
}
function verifyFields(focused, quiet) {
	var ok = 1;
	cookie.set(cprefix+'_refresh', adblock_refresh);

	return ok;
}

var ref = new TomatoRefresh(' ', ' ', 3, 'advanced_adblock_refresh');
	ref.refresh = function(text) {
	try {
		eval(text);
	}
	catch (ex) {
	}
	adblockStatus();
}

adblockg.resetNewEditor = function() {
	var f;

	f = fields.getAll(this.newEditor);
	ferror.clearAll(f);
	f[0].checked = 1;
	f[1].value = '';
	f[2].value = '';
}

adblockg.setup = function() {
	this.init('adblock-grid', '', 50, [
		{ type: 'checkbox', prefix: '<div class="centered">', suffix: '<\/div>' },
		{ type: 'text', maxlen: 130 },
		{ type: 'text', maxlen: 40 }
	]);
	this.headerSet(['On','Blacklist URL','Description']);
	var s = nvram.adblock_blacklist.split('>');
	for (var i = 0; i < s.length; ++i) {
		var t = s[i].split('<');
		if (t.length == 3) this.insertData(-1, t);
	}
	this.showNewEditor();
	this.resetNewEditor();
}

function save() {
	var data = adblockg.getAllData();
	var blacklist = '';
	for (var i = 0; i < data.length; ++i) {
		blacklist += data[i].join('<')+'>';
	}

	var fom = E('t_fom');
	fom.adblock_enable.value = E('_f_adblock_enable').checked ? 1 : 0;
	fom.adblock_logs.value = fom.f_adblock_logs.value;
	fom.adblock_limit.value = fom.f_adblock_limit.value;
	fom.adblock_path.value = fom.f_adblock_path.value.replace(/\/+$/, '');
	fom.adblock_blacklist.value = blacklist;
	form.submit(fom, 1);
	setTimeout(function() { adblockStatus(); }, 2000);
}

function init() {
	if (((c = cookie.get(cprefix+'_notes_vis')) != null) && (c == '1'))
		toggleVisibility(cprefix, 'notes');

	if (((c = cookie.get(cprefix+'_advanced_vis')) != null) && (c == '1'))
		toggleVisibility(cprefix, 'advanced');

	adblockg.recolor();
	adblockStatus();
	ref.initPage();
	eventHandler();
}

function adblockMe(command) {
	var c = '/usr/sbin/adblock '+command ;
	if (command == 'snapshot')
		alert('Result saved in /tmp/adblock.snapshot.$now');

	var cmd = new XmlHttp();
	cmd.post('shell.cgi', 'action=execute&command='+escapeCGI(c.replace(/\r/g, '')));
	setTimeout(function() { adblockStatus(); }, 500);
}

function displayStatus() {
	elem.setInnerHTML(E('status'), cmdresult);
	cmdresult = '';
}

function adblockStatus() {
	cmd = new XmlHttp();
	cmd.onCompleted = function(text, xml) {
		eval(text);
		displayStatus();
	}
	cmd.onError = function(x) {
		cmdresult = 'ERROR: '+x;
		displayStatus();
	}
	var commands = '/usr/sbin/adblock status-gui';
	cmd.post('shell.cgi', 'action=execute&command='+escapeCGI(commands.replace(/\r/g, '')));
}

function earlyInit() {
	adblockg.setup();
	verifyFields(null, true);
}

/* Determine Delimiter/Separator */
function determineDelimiter(inputString) {
	const lines = inputString.split(/\r?\n/);
	let isSpaceDelimited = false;
	var i = 0
	for (const line of lines) {
		const trimmedLine = line.trim();
		if (trimmedLine.startsWith('#') || trimmedLine === '') {
			continue;
		}
		const units = trimmedLine.split(' ');
		if (units.length > 1) {
			return ' ';
		}
		else if (i > 1) {
			return '\n';
		}
	i += 1;
	}
}

/* Sort Domains */
function sortDomains(element) {
	var textarea = document.getElementById(element);
	var delimiter = determineDelimiter(textarea.value.trim());
	var splitDomains = textarea.value.split(delimiter).map((domain) => domain.trim().split(".").reverse());
	const regex = /[%!#+\s]/g
	splitDomains.sort((a, b) => {
		const aList = a.map(item => item.replace(regex, ''));
		const bList = b.map(item => item.replace(regex, ''));
		var aSeg = aList[1], bSeg = bList[1];

		if ( aSeg === undefined || bSeg === undefined) { return 0; }
		if (a.length > 2 && aList[0].length === 2 && aSeg.length <= 3)
			aSeg = aList[2];

		if (b.length > 2 && bList[0].length === 2 && bSeg.length <= 3)
			bSeg = bList[2];

		var domainCompare = aSeg.toLowerCase().localeCompare(bSeg.toLowerCase());
		if (domainCompare !== 0) return domainCompare;

		var tldCompare = aList[0].toLowerCase().localeCompare(bList[0].toLowerCase());
		if (tldCompare !== 0) return tldCompare;

		var i = 1;
		while ( true ) {
			var aSeg = aList[i], bSeg = bList[i];

			if (aSeg === undefined && bSeg === undefined) {
				return 0;
			}
			else if (aSeg === undefined) {
				return -1;
			}
			else if (bSeg === undefined) {
				return 1;
			}

			var subCompare = aSeg.toLowerCase().localeCompare(bSeg.toLowerCase());
			if (subCompare !== 0) return subCompare;

			i += 1;
		}
	});
	var sortedDomains = splitDomains.map((segments) => segments.reverse().join("."));
	textarea.value = sortedDomains.join(delimiter).trim();
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

<input type="hidden" name="_nextpage" value="advanced-adblock.asp">
<input type="hidden" name="_service" value="adblock-restart">
<input type="hidden" name="adblock_enable">
<input type="hidden" name="adblock_logs">
<input type="hidden" name="adblock_path">
<input type="hidden" name="adblock_limit">
<input type="hidden" name="adblock_blacklist">

<!-- / / / -->

<div class="section-title">Adblock (DNS filtering) - Settings</div>
<div class="section">
<div class="section">
	<script>
		createFieldTable('', [
			{ title: 'Enable', name: 'f_adblock_enable', type: 'checkbox', value: nvram.adblock_enable != '0' },
			{ title: 'Max Log Level', indent: 2, name: 'f_adblock_logs', type: 'select', options: [[0,'Only Basic'],[3,'3 Error (default)'],[4,'4 Warning'],[5,'5 Notification'],[6,'6 Info'],[7,'7 Debug + trace mode']], value: nvram.adblock_logs },
			{ title: 'Blockfile size limit', indent: 2, name: 'f_adblock_limit', type: 'text', placeholder: 'empty = reset', maxlen: 32, size: 15, suffix: '&nbsp;<small>Bytes<\/small>', value: nvram.adblock_limit },
			{ title: 'Custom path (optional)', indent: 2, name: 'f_adblock_path', type: 'text', placeholder: 'empty = /tmp', maxlen: 64, size: 15, suffix: '<small>/adblock/<\/small>', value: nvram.adblock_path }
		]);
	</script>
</div>

<!-- / / / -->

<div class="section-title">Domain blacklist URLs & Group-of-lists</div>
<div class="section">
	<div class="tomato-grid" id="adblock-grid"></div>
</div>

<!-- / / / -->

<div class="section-title">Domain blacklist custom</div><input type="button" value="Sort domains backward a-z ↓" onclick="sortDomains('domain-blacklist')" id="sort-button-blacklist" style="float: right;">
<div class="section">
	<script>
		createFieldTable('', [
			{ title: 'Individual domains and/or path to external file/s.<br>Domains defined with a prepending <b>+<\/b> will have any found subdomain pruned from the blockfile.<br>Prepend <b>#<\/b> to comment.', name: 'adblock_blacklist_custom', type: 'textarea', placeholder: 'baddomain.com&#10;/mnt/usb/list-of-bad-domains.list&#10;/mnt/usb/list-of-blacklisted-urls.list&#10;+prune-subdomains.com', value: nvram.adblock_blacklist_custom, id: 'domain-blacklist' }
		]);
	</script>
</div>


<!-- / / / -->

<div class="section-title">Domain whitelist</div>
<input type="button" value="Sort domains backward a-z ↓" onclick="sortDomains('domain-whitelist')" id="sort-button-whitelist" style="float: right;">
<div class="section">
	<script>
		createFieldTable('', [
			{ title: 'Individual domains and/or path to external file/s.<br>Domains defined with a prepending <b>%<\/b> will not have the own subdomains blocked.<br>Prepend <b>#<\/b> to comment.', name: 'adblock_whitelist', type: 'textarea', placeholder: 'gooddomain.com\&#10;/mnt/usb/list-of-good-domains.list&#10;/mnt/usb/file-cointaining-list-of-urls.list&#10;%onlythis-nosubdomains.com', value: nvram.adblock_whitelist, id: 'domain-whitelist' }
		]);
	</script>
</div>

<!-- / / / -->

<div class="section-title">Advanced</div>
<div class="section">
	<table cellspacing="1" cellpadding="2" border="0">
	<tr><td>Controls -</td><td>Status -</td></tr>
	<tr><td>&nbsp;</td>
	<th rowspan="10" valign="top" style="text-align: left;padding-left:30px;padding-top:5px;font-size:9px;width:100%;border:1px solid #aaaaaa"><div id="status"><wbr></div></th></tr>
	<tr><td><input type="button" style="width:130px" value="▶️ Load " id="adblock-start" onclick="adblockMe('start');"></td></tr>
	<tr><td><input type="button" style="width:130px" value="⏏️ Unload" id="adblock-stop" onclick="adblockMe('stop');"></td></tr>
	<tr><td><input type="button" style="width:130px" value="🔄 Update" id="adblock-update" onclick="adblockMe('update');"></td></tr>
	<tr><td><input type="button" style="width:130px" value="♻️ Reset limit" id="adblock-reset" onclick="adblockMe('reset');"></td></tr>
	<tr><td><input type="button" style="width:130px" value="🧹 Clear all files" id="adblock-clear" onclick="adblockMe('clear');"></td></tr>
	<tr><td><input type="button" style="width:130px" value="📷 Snapshot" id="adblock-snapshot" onclick="adblockMe('snapshot');"></td></tr>
	<tr><td><input type="button" style="width:130px" value="☑️ Enable only" id="adblock-enable" onclick="adblockMe('enable');"></td></tr>
	<tr><td><input type="button" style="width:130px" value="⬜ Disable only" id="adblock-disable" onclick="adblockMe('disable');"></td></tr>
	<tr><td>&nbsp;</td>
	<tr><th colspan="2">
	<div id="survey-controls">
		<img src="spin.gif" alt="" id="refresh-spinner">
		<small>Status - </small><script>genStdTimeList('refresh-time', 'One off', 5);</script>
		<input type="button" value="Refresh" onclick="ref.toggle()" id="refresh-button">
	</div>
	</div></th></tr>
	</table></div>
</div>

<!-- / / / -->

<div class="section-title">Notes <small><i><a href='javascript:toggleVisibility(cprefix,"notes");'><span id="sesdiv_notes_showhide">(Show)</span></a></i></small></div>
<div class="section" id="sesdiv_notes" style="display:none">
	<ul>
		<li><b>Updated information on tested adblock lists can be found at <a href="https://wiki.freshtomato.org/doku.php/adblock_dns_filtering" class="new_window">this page</a></b></li>
		<li><b>Enable</b> - Used to activate/deactivate the adblock function. When enable is set the script runs after a save, a manual Load/Update, it autostart at boot and set autoupdate to run daily at a random time between 3am and 6am (excluding mins 59,00,01).</li>
		<li><b>Blockfile size limit</b> - Defined in Bytes, it's an automatically calculated hard limit for the dnsmasq.adblock file. This limit can be overwritten manually. Removing the number and saving will trigger an internal calculation performed at the next run.</li>
		<li><b>Custom path</b> - Optional, allows to save the potentially large adblock files on permanent storage like USB/CIFS/etc. This indirectly also means lower RAM usage and additional list control to avoid downloads/processing when not necessary.</li>
		<li><b>Blacklist URL & Group-of-lists</b> - Supported blacklist can come in multiple format. as long as they are text and with maximum one domain reference per line. Empty lines and lines starting with "#" or "!" are always ignored. A particular note on the Group-of-lists format where the content of the defined list contains references to external URLs e.g.<br><code>[https://provider.com/badaddresses.txt] --> containing a list of URLs</code>.</li>
		<li><b>Blacklist Custom</b> - Optional, newline separated: domain1.com domain2.com domain3.com. It also accepts external files as a source e.g. <code>/mnt/usb/blacklist</code>, with one domain per line. Prepending a '+' to the domain will force a removal of all the child domains from the blocklist file keeping only the custom defined one (blocking all its subdomains).</li>
		<li><b>Whitelist</b> - Optional, newline separated: domain1.com domain2.com domain3.com. It also accepts external files as a source e.g. <code>/mnt/usb/whitelist</code>, with one domain per line. Please note by default given a domain, any of its subdomains will be whitelisted. To have a domain strictly whitelisted (subdomains blocked) prepend a <B>%</B> to the domain.</li>
		<li><b>Files</b> - Do not defined your custom files within the adblock folder as this is periodically cleaned up
		<li><b>Caution</b> - Configuring large blocklists in adblock is not ideal. Add one list at the time and monitor the RAM usage. There are multiple protections in place but the most important is to trim down your final blocklist if too many resources are needed, this is reflected in the <code>Blockfile size limit</code> field</li>
		<li><b>Hold-time</b> - There's a 30 min hold-time between consecutive updates to avoid false positive calls. This can be manually overridden performing an unload + update</li>
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