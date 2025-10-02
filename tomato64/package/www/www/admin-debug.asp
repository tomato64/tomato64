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
<title>[<% ident(); %>] Admin: Debugging</title>
<link rel="stylesheet" type="text/css" href="tomato.css?rel=<% version(); %>">
<% css(); %>
<script src="tomato.js?rel=<% version(); %>"></script>

<script>

//	<% nvram("t_model_name,debug_nocommit,debug_cprintf,debug_cprintf_file,console_loglevel,t_cafree,t_hidelr,debug_ddns,debug_norestart,debug_logsegfault,debug_wlx_shdown,http_nocache"); %>

/* BCMWL714-BEGIN */
var wlx_shdown_display = 'none';
switch(nvram['t_model_name']) {
	case 'Asus RT-AC5300':
		wlx_shdown_display = '';
}
/* BCMWL714-END */

var cprefix = 'admin_debug';

function nvramCommit() {
	fields.disableAll('t_fom', 1);
	form.submitHidden('nvcommit.cgi', { '_nextpage': myName() });
}

function clearCache() {
	localStorage.clear();
	alert('Done!');
}

function flushDNS() {
	var c = '/usr/bin/killall -HUP dnsmasq';
	var cmd = new XmlHttp();
	cmd.post('shell.cgi', 'action=execute&command='+escapeCGI(c.replace(/\r/g, '')));
	alert('Done!');
}

function verifyFields(focused, quiet) {
	return 1;
}

function save() {
	var fom = E('t_fom');
	fom.debug_nocommit.value = fom.f_debug_nocommit.checked ? 1 : 0;
	fom.debug_cprintf.value = fom.f_debug_cprintf.checked ? 1 : 0;
	fom.debug_cprintf_file.value = fom.f_debug_cprintf_file.checked ? 1 : 0;
	fom.t_cafree.value = fom.f_cafree.checked ? 1 : 0;
	fom.t_hidelr.value = fom.f_hidelr.checked ? 1 : 0;
	fom.debug_logsegfault.value = fom.f_debug_logsegfault.checked ? 1 : 0;
	fom.debug_ddns.value = fom.f_debug_ddns.checked ? 1 : 0;
	fom.http_nocache.value = fom.f_http_nocache.checked ? 1 : 0;
	fom.console_loglevel.value = fom._f_console_loglevel.value;

/* BCMWL714-BEGIN */
	fom.debug_wlx_shdown.value = 0; /* init with 0 and check */
		if (fom.f_wlx_shdown_eth1.checked)
			fom.debug_wlx_shdown.value = fom.debug_wlx_shdown.value  | 0x01; /* set bit 0, wl radio eth1 */

		if (fom.f_wlx_shdown_eth2.checked)
			fom.debug_wlx_shdown.value = fom.debug_wlx_shdown.value  | 0x02; /* set bit 1, wl radio eth2 */

		if (fom.f_wlx_shdown_eth3.checked)
			fom.debug_wlx_shdown.value = fom.debug_wlx_shdown.value  | 0x04; /* set bit 2, wl radio eth3 */
/* BCMWL714-END */

	var a = [];
	if (fom.f_nr_crond.checked) a.push('crond');
	if (fom.f_nr_dnsmasq.checked) a.push('dnsmasq');
	if (fom.f_nr_hotplug2.checked) a.push('hotplug2');
	if (fom.f_nr_igmprt.checked) a.push('igmprt');
	if (fom.f_nr_ntpd.checked) a.push('ntpd');
	fom.debug_norestart.value = a.join(',');

	fom._service.value = '';
	if (fom.debug_logsegfault.value != nvram.debug_logsegfault) {
		nvram.debug_logsegfault = fom.debug_logsegfault.value;
		fom._service.value = 'firewall-restart';
	}
	if (fom.http_nocache.value != nvram.http_nocache) {
		nvram.http_nocache = fom.http_nocache.value;
		if (fom._service.value != '')
			fom._service.value += ',';

		fom._service.value += 'httpd-restart';
	}

	if (nvram.console_loglevel != fom.console_loglevel.value && confirm('Router must be rebooted to apply changed kernel log level. Reboot now? (and commit changes to NVRAM)')) {
		fom._reboot.value = 1;
		form.submit(fom, 0);
	}
	else
		form.submit(fom, 1);
}

function init() {
	var c;
	if (((c = cookie.get(cprefix+'_notes_vis')) != null) && (c == '1'))
		toggleVisibility(cprefix, 'notes');
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

<input type="hidden" name="_nextpage" value="admin-misc.asp">
<input type="hidden" name="_service" value="">
<input type="hidden" name="_reboot" value="0">
<input type="hidden" name="debug_nocommit">
<input type="hidden" name="debug_cprintf">
<input type="hidden" name="debug_cprintf_file">
<input type="hidden" name="debug_ddns">
<input type="hidden" name="debug_norestart">
<input type="hidden" name="debug_logsegfault">
<!-- BCMWL714-BEGIN -->
<input type="hidden" name="debug_wlx_shdown">
<!-- BCMWL714-END -->
<input type="hidden" name="t_cafree">
<input type="hidden" name="t_hidelr">
<input type="hidden" name="http_nocache">
<input type="hidden" name="console_loglevel">

<!-- / / / -->

<div class="section-title">Debugging</div>
<div class="section" id="debug">
	<script>
		createFieldTable('', [
			{ title: 'Avoid performing an NVRAM commit', name: 'f_debug_nocommit', type: 'checkbox', value: nvram.debug_nocommit != '0' },
			{ title: 'Enable cprintf output to console', name: 'f_debug_cprintf', type: 'checkbox', value: nvram.debug_cprintf != '0' },
			{ title: 'Enable cprintf output to /tmp/cprintf', name: 'f_debug_cprintf_file', type: 'checkbox', value: nvram.debug_cprintf_file != '0' },
			{ title: 'Enable DDNS output to /tmp/mdu-*', name: 'f_debug_ddns', type: 'checkbox', value: nvram.debug_ddns != '0' },
			{ title: 'Enable segfault logging', name: 'f_debug_logsegfault', type: 'checkbox', value: nvram.debug_logsegfault != '0' },
			{ title: 'Count cache memory, buffers and reclaimable slab memory as free memory', name: 'f_cafree', type: 'checkbox', value: nvram.t_cafree == '1' },
			{ title: 'Avoid displaying LAN to router connections', name: 'f_hidelr', type: 'checkbox', value: nvram.t_hidelr == '1' },
			{ title: 'Kernel printk log level', name: 'f_console_loglevel', type: 'select', options: [[1,'Emergency'],[2,'Alert'],[3,'Critical'],[4,'Error'],[5,'Warning'],[6,'Notice'],[7,'Info'],[8,'Debug']], value: fixInt(nvram.console_loglevel, 1, 8, 1) },
			{ title: 'Do not restart the following process if they die', multi: [
				{ name: 'f_nr_crond', type: 'checkbox', suffix: ' crond<br>', value: (nvram.debug_norestart.indexOf('crond') != -1) },
				{ name: 'f_nr_dnsmasq', type: 'checkbox', suffix: ' dnsmasq<br>', value: (nvram.debug_norestart.indexOf('dnsmasq') != -1) },
				{ name: 'f_nr_hotplug2', type: 'checkbox', suffix: ' hotplug2<br>', value: (nvram.debug_norestart.indexOf('hotplug2') != -1) },
				{ name: 'f_nr_igmprt', type: 'checkbox', suffix: ' igmprt<br>', value: (nvram.debug_norestart.indexOf('igmprt') != -1) },
				{ name: 'f_nr_ntpd', type: 'checkbox', suffix: ' ntpd<br>', value: (nvram.debug_norestart.indexOf('ntpd') != -1) }
			] },
/* BCMWL714-BEGIN */
			{ title: 'Disable wireless interface(s)', multi: [
				{ name: 'f_wlx_shdown_eth1', type: 'checkbox', suffix: ' eth1 (2,4 GHz radio)<br>', value: (nvram.debug_wlx_shdown & 0x01) },
				{ name: 'f_wlx_shdown_eth2', type: 'checkbox', suffix: ' eth2 (first 5 GHz radio)<br>', value: (nvram.debug_wlx_shdown & 0x02) },
				{ name: 'f_wlx_shdown_eth3', type: 'checkbox', suffix: ' eth3 (second 5 GHz radio)<br>', value: (nvram.debug_wlx_shdown & 0x04) }
			], hidden: wlx_shdown_display },
/* BCMWL714-END */
			{ title: 'Set "no-cache" in httpd header', name: 'f_http_nocache', type: 'checkbox', value: nvram.http_nocache == '1' }
		]);
	</script>

	<br><br>

	&raquo; <a href="clearcookies.asp?_http_id=<% nv(http_id); %>">Clear browser cookies</a><br>
	&raquo; <a href="javascript:clearCache()">Clear browser cache</a><br>
	&raquo; <a href="javascript:flushDNS()">Flush DNS (dnsmasq) cache</a><br>
	&raquo; <a href="javascript:nvramCommit()">Commit to NVRAM now</a><br>
	&raquo; <a href="javascript:reboot()">Reboot</a><br>
	&raquo; <a href="javascript:halt()">Halt</a><br>

	<br><br>

/* TOMATO64-REMOVE-BEGIN */
	&raquo; <a href="/cfe/cfe.bin?_http_id=<% nv(http_id); %>">Download CFE</a><br>
/* TOMATO64-REMOVE-END */
	&raquo; <a href="/ipt/iptables.txt?_http_id=<% nv(http_id); %>">Download Iptables Dump</a><br>
<!-- IPV6-BEGIN -->
	&raquo; <a href="/ip6t/ip6tables.txt?_http_id=<% nv(http_id); %>">Download Ip6tables Dump</a><br>
<!-- IPV6-END -->
	&raquo; <a href="/logs/syslog.txt?_http_id=<% nv(http_id); %>">Download Logs</a><br>
	&raquo; <a href="/nvram/nvram.txt?_http_id=<% nv(http_id); %>">Download NVRAM Dump</a><br>

	<div class="note-spacer">
		<b>Warning</b>: The NVRAM Dump text file may contain information like wireless encryption keys and usernames/passwords for the router, ISP and DDNS. Please review &amp; edit this file before sharing it with anyone.<br>
	</div>
</div>

<div class="section-title">Notes <small><i><a href="javascript:toggleVisibility(cprefix,'notes');" id="toggleLink-notes"><span id="sesdiv_notes_showhide">(Show)</span></a></i></small></div>
<div class="section" id="sesdiv_notes" style="display:none">
	<ul>
		<li><b>Avoid performing an NVRAM commit</b> - To be used only for debugging. This prevents configuration changes from being saved permanently.</li>
		<li><b>Enable cprintf output to console</b> - Redirects cprintf output to the console, so it is visible in your browser.</li>
		<li><b>Enable cprintf output to /tmp/cprintf</b> - Redirects cprintf output to file: <i>/tmp/cprintf</i>  for viewing.</li>
		<li><b>Enable DDNS output to /tmp/mdu-*</b> - DDNS debug info will be written in a file in /tmp. The * is replaced with the name of the DDNS provider.</li>
		<li><b>Enable segfault logging</b> - When enabled, if a program crashes due to segfault, extensive messages are logged.</li>
		<li><b>Count cache memory and buffers as free memory</b> - Linux counts buffer and cache as used RAM. Enabling this will consider cache and buffer as unallocated RAM.</li>
		<li><b>Avoid displaying LAN to router connections</b> - Do not consider connections between LAN and router in CONNTRACK table.</li>
		<li><b>Set "no-cache" in httpd header</b> - Essentially, this will tell your browser not to cache Tomato64 web interface pages.</li>
		<li><b>Kernel printk log level</b> - This sets klogd (kernel logging) minimum logging level.</li>
	</ul>
<!-- BCMWL714-BEGIN -->
	<br>
	<ul>
		<li><b>Disable wireless interface(s)</b> - Allows you to shutdown wireless interfaces (for example if broken; Note: currently only for Asus RT-AC5300). Reboot required!</li>
	</ul>
<!-- BCMWL714-END -->
	<br>
	<ul>
/* TOMATO64-REMOVE-BEGIN */
		<li><b>Download CFE</b> - Allows you to extract the system CFE from your router in one click.</li>
/* TOMATO64-REMOVE-END */
		<li><b>Download NVRAM Dump</b> - Warning: An NVRAM dump will contain sensitive data, like usernames/passwords/keys/DDNS details.</li>
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
</body>
</html>
