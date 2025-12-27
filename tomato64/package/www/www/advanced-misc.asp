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
<title>[<% ident(); %>] Advanced: Miscellaneous</title>
<link rel="stylesheet" type="text/css" href="tomato.css?rel=<% version(); %>">
<% css(); %>
<script src="tomato.js?rel=<% version(); %>"></script>

<script>

/* TOMATO64-REMOVE-BEGIN */
//	<% nvram("t_features,wait_time,wan_speed,jumbo_frame_enable,jumbo_frame_size,ctf_disable,bcmnat_disable"); %>
/* TOMATO64-REMOVE-END */
/* TOMATO64-BEGIN */
//	<% nvram("t_features,wait_time,wan_speed,jumbo_frame_enable,jumbo_frame_size,ctf_disable,bcmnat_disable,zram_enable,zram_size,zram_priority,zram_comp_algo"); %>
/* TOMATO64-END */

et1000 = features('1000et');

function verifyFields(focused, quiet) {
	E('_jumbo_frame_size').disabled = !E('_f_jumbo_frame_enable').checked;
/* TOMATO64-BEGIN */
	var e = E('_f_zram_enable').checked;
	E('_zram_size').disabled = !e;
	E('_zram_priority').disabled = !e;
	E('_zram_comp_algo').disabled = !e;
/* TOMATO64-END */

	return 1;
}

function save() {
	var fom = E('t_fom');
	fom.jumbo_frame_enable.value = E('_f_jumbo_frame_enable').checked ? 1 : 0;
/* CTF-BEGIN */
	fom.ctf_disable.value = E('_f_ctf_disable').checked ? 0 : 1;
/* CTF-END */
/* BCMNAT-BEGIN */
	fom.bcmnat_disable.value = E('_f_bcmnat_disable').checked ? 0 : 1;
/* BCMNAT-END */
/* TOMATO64-BEGIN */
	fom.zram_enable.value = E('_f_zram_enable').checked ? 1 : 0;
/* TOMATO64-END */

	if ((fom.wan_speed.value != nvram.wan_speed) ||
/* CTF-BEGIN */
	    (fom.ctf_disable.value != nvram.ctf_disable) ||
/* CTF-END */
/* BCMNAT-BEGIN */
	    (fom.bcmnat_disable.value != nvram.bcmnat_disable) ||
/* BCMNAT-END */
	    (fom.jumbo_frame_enable.value != nvram.jumbo_frame_enable) ||
/* TOMATO64-REMOVE-BEGIN */
	    (fom.jumbo_frame_size.value != nvram.jumbo_frame_size)) {
/* TOMATO64-REMOVE-END */
/* TOMATO64-BEGIN */
	    (fom.jumbo_frame_size.value != nvram.jumbo_frame_size) ||
	    (fom.zram_enable.value != nvram.zram_enable) ||
	    (fom.zram_size.value != nvram.zram_size) ||
	    (fom.zram_priority.value != nvram.zram_priority) ||
	    (fom.zram_comp_algo.value != nvram.zram_comp_algo)
	    ) {
/* TOMATO64-END */
		if (confirm("Router must be rebooted to apply changed settings. Reboot now? (and commit changes to NVRAM)")) {
			fom._reboot.value = 1;
			form.submit(fom, 0);
		}
		else { /* countinue without reboot (user wants it that way) */
			form.submit(fom, 1);
		}
	}
	else { /* continue without reboot */
		form.submit(fom, 1);
	}
}
</script>
</head>

<body>
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

<input type="hidden" name="_nextpage" value="advanced-misc.asp">
<input type="hidden" name="_reboot" value="0">
<input type="hidden" name="jumbo_frame_enable">
<!-- CTF-BEGIN -->
<input type="hidden" name="ctf_disable">
<!-- CTF-END -->
<!-- BCMNAT-BEGIN -->
<input type="hidden" name="bcmnat_disable">
<!-- BCMNAT-END -->
<!-- TOMATO64-BEGIN -->
<input type="hidden" name="zram_enable">
<!-- TOMATO64-END -->

<!-- / / / -->

<div class="section-title">Miscellaneous</div>
<div class="section">
	<script>
		a = [];
		for (i = 3; i <= 20; ++i) a.push([i, i + ' seconds']);
		createFieldTable('', [
			{ title: 'Boot Wait Time *', name: 'wait_time', type: 'select', options: a, value: fixInt(nvram.wait_time, 3, 20, 3) },
			{ title: 'WAN Port Speed *', name: 'wan_speed', type: 'select', options: [[0,'10Mbps Full'],[1,'10Mbps Half'],[2,'100Mbps Full'],[3,'100Mbps Half'],[4,'Autonegotiation']], value: nvram.wan_speed },
			null,
/* CTF-BEGIN */
			{ title: 'CTF (Cut-Through Forwarding)<br>and HW acceleration', name: 'f_ctf_disable', type: 'checkbox', value: nvram.ctf_disable != '1', suffix: ' <small>disables QoS and BW Limiter!<\/small>' },
			null,
/* CTF-END */
/* BCMNAT-BEGIN */
			{ title: 'Broadcom FastNAT (bcm_nat)', name: 'f_bcmnat_disable', type: 'checkbox', value: nvram.bcmnat_disable != '1', suffix: ' <small>disables BW Limiter; using QoS or Access Restriction disables this module!<\/small>' },
/* BCMNAT-END */
			{ title: 'Enable Jumbo Frames *', name: 'f_jumbo_frame_enable', type: 'checkbox', value: nvram.jumbo_frame_enable != '0', hidden: !et1000 },
			{ title: 'Jumbo Frame Size *', name: 'jumbo_frame_size', type: 'text', maxlen: 4, size: 6, value: fixInt(nvram.jumbo_frame_size, 1, 9720, 2000),
				suffix: ' <small>Bytes (range: 1 - 9720; default: 2000)<\/small>', hidden: !et1000 }
		]);
	</script>

	<div class="note-spacer"><small>* Some router models might not support this option.</small></div>

</div>

<!-- / / / -->

<!-- TOMATO64-BEGIN -->
<div class="section-title">ZRAM Compressed Swap</div>
<div class="section">
	<script>
		createFieldTable('', [
			{ title: 'Enable ZRAM', name: 'f_zram_enable', type: 'checkbox', value: nvram.zram_enable == '1' },
			{ title: 'ZRAM Size', name: 'zram_size', type: 'text', maxlen: 5, size: 7, value: fixInt(nvram.zram_size, 0, 32768, 0),
				suffix: ' <small>MB (0 = auto: 50% of RAM; range: 0 - 32768)<\/small>' },
			{ title: 'Swap Priority', name: 'zram_priority', type: 'text', maxlen: 5, size: 7, value: fixInt(nvram.zram_priority, -1, 32767, 100),
				suffix: ' <small>Higher priority is used first (range: -1 to 32767; default: 100)<\/small>' },
			{ title: 'Compression Algorithm', name: 'zram_comp_algo', type: 'select',
				options: [['lz4','LZ4 (fastest)'],['lzo','LZO (fast)'],['lzo-rle','LZO-RLE (balanced)'],['lz4hc','LZ4HC (better compression)'],['zstd','ZSTD (best compression)']],
				value: nvram.zram_comp_algo || 'lz4' }
		]);
	</script>

	<div class="note-spacer">
		<small>
			ZRAM creates a compressed block device in RAM for swap space, effectively increasing available memory.<br>
			Useful for memory-intensive operations like adblock with large lists.<br>
			Typical compression ratios: 2-3x (LZ4/LZO) or 3-4x (ZSTD).
		</small>
	</div>

</div>
<!-- TOMATO64-END -->

<!-- / / / -->

<div id="footer">
	<span id="footer-msg"></span>
	<input type="button" value="Save" id="save-button" onclick="save()">
	<input type="button" value="Cancel" id="cancel-button" onclick="reloadPage();">
</div>

</td></tr>
</table>
</form>
<script>insOvl();verifyFields(null, true);</script>
</body>
</html>
