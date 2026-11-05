<!DOCTYPE html>
<!--

	Storage Health Page
	Copyright (C) 2026 Lance Fredrickson
	lancethepants@gmail.com

-->
<html lang="en-GB">
<head>
<meta http-equiv="content-type" content="text/html;charset=utf-8">
<meta name="robots" content="noindex,nofollow">
<title>[<% ident(); %>] Admin: Storage</title>
<link rel="stylesheet" type="text/css" href="tomato.css?rel=<% version(); %>">
<% css(); %>
<script src="tomato.js?rel=<% version(); %>"></script>

<style>
.st-ok { color: #2a9d2a; font-weight: bold; }
.st-warn { color: #d98a00; font-weight: bold; }
.st-bad { color: #d62020; font-weight: bold; }
.st-na { color: #888; }
table.st-grid { width: 100%; }
table.st-grid td { padding: 2px 6px; }
table.st-grid td.num { text-align: right; }
table.st-grid tr.total td { font-weight: bold; }
.st-tree { font-family: monospace; font-size: 1.2em; line-height: 1; padding: 0 0.4em 0 0.6em; opacity: 0.6; }
</style>

<script>

//	<% nvram("t_model_name"); %>

/* Filled in by httpd (storage.c) from sysfs and smartctl. Every value is a
   string; numbers are parsed here. */
<% storage(); %>

var cprefix = 'admin_storage';

var LVL_NA = 0, LVL_OK = 1, LVL_WARN = 2, LVL_BAD = 3;
var lvlClass = ['st-na', 'st-ok', 'st-warn', 'st-bad'];
var lvlText = ['Unknown', 'Good', 'Warning', 'Failing'];

function num(v) {
	var n = parseInt(v, 10);

	return isNaN(n) ? null : n;
}

function lvlSpan(lvl, text) {
	return '<span class="' + lvlClass[lvl] + '">' + (text || lvlText[lvl]) + '<\/span>';
}

/* Disks are sold in decimal units, flash chips in binary ones. */
function fmtBytes(n, bin) {
	var base = bin ? 1024 : 1000;
	var units = bin ? ['B', 'KiB', 'MiB', 'GiB', 'TiB', 'PiB'] : ['B', 'KB', 'MB', 'GB', 'TB', 'PB'];
	var i = 0;

	if (n == null || isNaN(n))
		return '&#8212;';

	while (n >= base && i < units.length - 1) {
		n /= base;
		++i;
	}

	return (i ? n.toFixed(n < 10 ? 2 : 1) : n) + ' ' + units[i];
}

function fmtHours(h) {
	if (h == null)
		return '&#8212;';

	return comma(h) + ' h (' + (h / 24 / 365).toFixed(1) + ' years)';
}

function fieldRows(rows) {
	var s = '<table class="fields">';

	for (var i = 0; i < rows.length; ++i) {
		if (rows[i] == null)
			continue;
		s += '<tr><td class="title indent1">' + rows[i][0] + '<\/td><td class="content">' + rows[i][1] + '<\/td><\/tr>';
	}

	return s + '<\/table>';
}

/* ---------------------------------------------------------------- eMMC -- */

/* JEDEC EXT_CSD DEVICE_LIFE_TIME_EST_TYP_A/B: 0x01-0x0A in 10% steps of
   rated life used, 0x0B past it, 0x00 not reported (pre-5.0 parts). */
function emmcLife(v) {
	var n = parseInt(v, 16);

	if (isNaN(n) || n == 0)
		return { lvl: LVL_NA, text: 'Not reported' };
	if (n >= 0x0b)
		return { lvl: LVL_BAD, text: 'Rated life exceeded' };

	return { lvl: (n >= 8) ? LVL_WARN : LVL_OK, text: ((n - 1) * 10) + '% - ' + (n * 10) + '% used' };
}

/* PRE_EOL_INFO: how much of the spare block pool is gone. */
function emmcEol(v) {
	switch (parseInt(v, 16)) {
	case 1:	return { lvl: LVL_OK, text: 'Normal' };
	case 2:	return { lvl: LVL_WARN, text: 'Warning - 80% of reserved blocks used' };
	case 3:	return { lvl: LVL_BAD, text: 'Urgent - reserved blocks nearly exhausted' };
	}

	return { lvl: LVL_NA, text: 'Not reported' };
}

/* CID manufacturer IDs. eMMC and SD number vendors separately, so the same
   byte means different makers on each. Taken from mmc-utils'
   multimediacard.ids and sdcard.ids (git.kernel.org, 2026-09), plus SK Hynix
   from the kernel's CID_MANFID_HYNIX, which its quirks table applies to eMMC,
   and BIWIN from the CID table in its eMMC 5.1 datasheet.
   The ".ids" files' "Unknown" rows are left out; anything not listed shows
   the raw ID only. */
var mmcVendors = {
	0x00: 'SanDisk', 0x02: 'Kingston/SanDisk', 0x03: 'Toshiba', 0x11: 'Toshiba',
	0x13: 'Micron', 0x15: 'Samsung/SanDisk/LG', 0x2c: 'Kingston', 0x37: 'KingMax',
	0x44: 'ATP', 0x45: 'SanDisk', 0x70: 'Kingston', 0x90: 'SK Hynix', 0xab: 'BIWIN',
	0xfe: 'Micron'
};

var sdVendors = {
	0x01: 'Panasonic', 0x02: 'Toshiba/Kingston/Viking', 0x03: 'SanDisk', 0x08: 'Silicon Power',
	0x09: 'ATP', 0x18: 'Infineon', 0x1b: 'Transcend/Samsung', 0x1c: 'Transcend',
	0x1d: 'Corsair/AData', 0x1e: 'Transcend', 0x1f: 'Kingston', 0x27: 'Delkin/Phison',
	0x28: 'Lexar', 0x30: 'SanDisk', 0x31: 'Silicon Power', 0x33: 'STMicroelectronics',
	0x41: 'Kingston', 0x6f: 'STMicroelectronics', 0x74: 'Transcend', 0x76: 'Patriot',
	0x82: 'Gobe/Sony'
};

/* sysfs prints manfid as 0x%06x; show it as the one byte it is */
function mmcVendor(d) {
	var id = parseInt(d.manfid, 16);
	var hex, name;

	if (isNaN(id))
		return '&#8212;';

	hex = '0x' + (id < 16 ? '0' : '') + id.toString(16);
	name = ((d.type == 'SD') ? sdVendors : mmcVendors)[id];

	return name ? escapeHTML(name) + ' <small>(' + hex + ')<\/small>' : hex + ' <small>(vendor unknown)<\/small>';
}

/* The CID year is 4 bits. From EXT_CSD rev 5 (eMMC 4.41) on, JEDEC counts
   it from 2013, so 0-15 is 2013-2028. The kernel decodes 1997 + n and only
   moves years before 2010 up by 16 (mmc.c, mmc_decode_ext_csd), so a part
   made in 2026-2028 reads back as 2010-2012. A chip that reports life-time
   estimates is eMMC 5.0 or later, where the 2013 base always applies. */
function emmcDate(d) {
	var m = /^(\d+)\/(\d+)$/.exec(d.date || '');
	var lt = (d.life || '').split(/\s+/);
	var y;

	if (!m)
		return d.date;

	y = parseInt(m[2], 10);
	if (y >= 2010 && y <= 2012 && (parseInt(lt[0], 16) > 0 || parseInt(lt[1], 16) > 0))
		y += 16;

	return m[1] + '/' + y;
}

function showMmc(d) {
	var lt = (d.life || '').split(/\s+/);
	var a = emmcLife(lt[0]), b = emmcLife(lt[1]), e = emmcEol(d.eol);
	var lvl = Math.max(a.lvl, b.lvl, e.lvl);
	var rows;

	if (d.type != 'MMC') {
		/* SD cards carry no health registers at all */
		return fieldRows([
			['Health', lvlSpan(LVL_NA, 'Not available - SD cards do not report wear')],
			['Card', escapeHTML(d.model)],
			['Manufacturer', mmcVendor(d)]
		]);
	}

	rows = [
		['Health', lvlSpan(lvl)],
		['Life used (SLC area, type A)', lvlSpan(a.lvl, a.text)],
		['Life used (MLC area, type B)', lvlSpan(b.lvl, b.text)],
		['Reserved blocks', lvlSpan(e.lvl, e.text)],
		['Product', escapeHTML(d.model)],
		['Manufacturer', mmcVendor(d)],
		['Manufactured', escapeHTML(emmcDate(d))],
		d.fwrev ? ['Firmware', escapeHTML(d.fwrev)] : null
	];

	return fieldRows(rows);
}

/* --------------------------------------------------------------- SMART -- */

function smartParse(d) {
	if (!d.smart)
		return null;

	try {
		return JSON.parse(d.smart);
	}
	catch (ex) {
		return null;
	}
}

function smartMessage(j) {
	var m = (j && j.smartctl && j.smartctl.messages) || [];
	var out = [];

	for (var i = 0; i < m.length; ++i)
		out.push(escapeHTML(m[i].string));

	return out.join('<br>');	/* already escaped */
}

function smartAttr(j, id) {
	var t = (j.ata_smart_attributes && j.ata_smart_attributes.table) || [];

	for (var i = 0; i < t.length; ++i)
		if (t[i].id == id)
			return t[i];

	return null;
}

function showNvme(j, rows) {
	var h = j.nvme_smart_health_information_log;
	var lvl = LVL_OK;

	if (!h) {
		rows.unshift(['Health', lvlSpan(LVL_NA, 'No health log returned')]);
		return;
	}

	if (h.critical_warning || (j.smart_status && j.smart_status.passed === false))
		lvl = LVL_BAD;
	else if (h.available_spare <= h.available_spare_threshold || h.percentage_used >= 100)
		lvl = LVL_BAD;
	else if (h.percentage_used >= 90 || h.media_errors > 0)
		lvl = LVL_WARN;

	rows.unshift(['Health', lvlSpan(lvl)]);
	rows.push(['Life used', lvlSpan(h.percentage_used >= 90 ? LVL_WARN : LVL_OK, h.percentage_used + '%')]);
	rows.push(['Available spare', h.available_spare + '% (threshold ' + h.available_spare_threshold + '%)']);
	if (h.critical_warning)
		rows.push(['Critical warning', lvlSpan(LVL_BAD, '0x' + h.critical_warning.toString(16))]);
	rows.push(['Media errors', h.media_errors ? lvlSpan(LVL_WARN, comma(h.media_errors)) : '0']);
	rows.push(['Temperature', h.temperature + ' &deg;C']);
	rows.push(['Data written', fmtBytes(h.data_units_written * 512000)]);
	rows.push(['Data read', fmtBytes(h.data_units_read * 512000)]);
	rows.push(['Power-on time', fmtHours(h.power_on_hours)]);
	rows.push(['Power cycles', comma(h.power_cycles)]);
	rows.push(['Unsafe shutdowns', comma(h.unsafe_shutdowns)]);
}

function showAta(j, rows) {
	var lvl = LVL_NA;
	var t = smartAttr(j, 5), p = smartAttr(j, 197), u = smartAttr(j, 198);
	var a, i, failed = [];

	if (j.smart_status)
		lvl = j.smart_status.passed ? LVL_OK : LVL_BAD;

	a = (j.ata_smart_attributes && j.ata_smart_attributes.table) || [];
	for (i = 0; i < a.length; ++i)
		if (a[i].when_failed)
			failed.push(escapeHTML(a[i].name) + ' (' + escapeHTML(a[i].when_failed) + ')');

	if (failed.length)
		lvl = LVL_BAD;
	else if (lvl == LVL_OK && ((t && t.raw.value > 0) || (p && p.raw.value > 0) || (u && u.raw.value > 0)))
		lvl = LVL_WARN;

	rows.unshift(['Health', lvlSpan(lvl, lvl == LVL_NA ? 'SMART not available' : null)]);
	if (failed.length)
		rows.push(['Failed attributes', lvlSpan(LVL_BAD, failed.join(', '))]);
	if (j.endurance_used)
		rows.push(['Life used', j.endurance_used.current_percent + '%']);
	if (t)
		rows.push(['Reallocated sectors', t.raw.value ? lvlSpan(LVL_WARN, comma(t.raw.value)) : '0']);
	if (p)
		rows.push(['Pending sectors', p.raw.value ? lvlSpan(LVL_WARN, comma(p.raw.value)) : '0']);
	if (u)
		rows.push(['Uncorrectable sectors', u.raw.value ? lvlSpan(LVL_WARN, comma(u.raw.value)) : '0']);
	if (j.temperature)
		rows.push(['Temperature', j.temperature.current + ' &deg;C']);
	if (j.power_on_time)
		rows.push(['Power-on time', fmtHours(j.power_on_time.hours)]);
	if (j.power_cycle_count != null)
		rows.push(['Power cycles', comma(j.power_cycle_count)]);
}

function showSmart(d) {
	var j = smartParse(d);
	var msg, rows = [];

	if (storage.smartctl != '1')
		return fieldRows([['Health', lvlSpan(LVL_NA, 'Not available - smartctl is not included on this device')]]);

	if (d.bus == 'virtio')
		return fieldRows([['Health', lvlSpan(LVL_NA, 'Not available - virtual disk')]]);

	if (!j)
		return fieldRows([['Health', lvlSpan(LVL_NA, 'No answer from smartctl')]]);

	msg = smartMessage(j);

	if (/standby/i.test(msg))
		return fieldRows([['Health', lvlSpan(LVL_NA, 'Drive is asleep - not woken to read it')]]);

	if (j.model_name)
		rows.push(['Model', escapeHTML(j.model_name)]);
	if (j.firmware_version)
		rows.push(['Firmware', escapeHTML(j.firmware_version)]);

	if (j.nvme_smart_health_information_log || d.bus == 'nvme')
		showNvme(j, rows);
	else
		showAta(j, rows);

	if (msg)
		rows.push(['smartctl', msg]);

	return fieldRows(rows);
}

/* ---------------------------------------------------------------- disks -- */

var busName = { mmc: 'eMMC / SD', nvme: 'NVMe', sata: 'SATA', usb: 'USB', virtio: 'Virtual', other: 'Disk' };

function showDisks() {
	var s = '', d, title, desc, i;

	for (i = 0; i < storage.disks.length; ++i) {
		d = storage.disks[i];

		title = (d.bus == 'mmc') ? (d.type == 'MMC' ? 'eMMC' : (d.type == 'SD' ? 'SD Card' : 'MMC')) : busName[d.bus];
		/* SCSI-layer vendor is just "ATA" for SATA disks; it only says
		   something useful for a USB enclosure */
		desc = ((d.bus == 'usb' && d.vendor) ? d.vendor + ' ' : '') + d.model;

		s += '<div class="section-title">' + title + ': ' + escapeHTML(d.name) +
		     ' <small>(' + fmtBytes(num(d.size) * 512) + (desc ? ', ' + escapeHTML(desc) : '') + ')<\/small><\/div>';
		s += '<div class="section">';
		s += (d.bus == 'mmc') ? showMmc(d) : showSmart(d);
		s += '<\/div>';
	}

	return s;
}

/* ------------------------------------------------------------------ MTD -- */

function isNand(t) {
	return /nand/i.test(t);
}

/* Tree lines in front of a sub-partition: a tee while its parent has more
   children after it, an elbow on the last one. */
function treeBranch(parts, i) {
	var m = parts[i], last = 1;

	if (m.parent == '')
		return '';

	for (var j = i + 1; j < parts.length; ++j) {
		if (parts[j].parent == m.parent) {
			last = 0;
			break;
		}
	}

	return '<span class="st-tree">' + (last ? '&#9492;&#9472;' : '&#9500;&#9472;') + '<\/span>';
}

function showMtd() {
	var chips = [], byPath = {}, parents = {};
	var s = '', c, m, i, k, row, tot;

	for (i = 0; i < storage.mtd.length; ++i) {
		m = storage.mtd[i];
		if (m.parent != '')
			parents[m.parent] = 1;
		if (!byPath[m.chippath]) {
			byPath[m.chippath] = { name: m.chip, parts: [] };
			chips.push(byPath[m.chippath]);
		}
		byPath[m.chippath].parts.push(m);
	}

	for (k = 0; k < chips.length; ++k) {
		c = chips[k];
		c.parts.sort(function(a, b) { return num(a.n) - num(b.n); });

		m = c.parts[0];
		s += '<div class="section-title">' + (isNand(m.type) ? 'NAND' : 'Flash') + ': ' + escapeHTML(c.name) +
		     ' <small>(' + escapeHTML(m.type) + ', erase block ' + fmtBytes(num(m.erasesize), 1) +
		     (num(m.writesize) > 1 ? ', page ' + fmtBytes(num(m.writesize), 1) : '') +
		     (num(m.strength) ? ', ECC ' + m.strength + ' bits' : '') + ')<\/small><\/div>';
		s += '<div class="section"><table class="tomato-grid st-grid">';
		s += '<tr class="header"><td>mtd<\/td><td>Partition<\/td><td class="num">Size<\/td>';
		if (isNand(m.type))
			s += '<td class="num">Bad blocks<\/td><td class="num">ECC corrected bits<\/td><td class="num">ECC failures<\/td>';
		s += '<\/tr>';

		tot = { size: 0, bad: 0, corrected: 0, failures: 0 };

		for (i = 0; i < c.parts.length; ++i) {
			m = c.parts[i];
			row = '<tr class="' + ((i & 1) ? 'even' : 'odd') + '">';
			row += '<td>' + m.n + '<\/td><td>' + treeBranch(c.parts, i) + escapeHTML(m.name) + '<\/td>';
			row += '<td class="num">' + fmtBytes(num(m.size), 1) + '<\/td>';
			if (isNand(m.type)) {
				row += '<td class="num">' + (num(m.bad) ? lvlSpan(LVL_WARN, m.bad) : m.bad) + '<\/td>';
				row += '<td class="num">' + comma(m.corrected) + '<\/td>';
				row += '<td class="num">' + (num(m.failures) ? lvlSpan(LVL_BAD, m.failures) : m.failures) + '<\/td>';
			}
			s += row + '<\/tr>';

			/* A partition that was split again (bcm53xx "firmware" into
			   "linux" + "ubi") covers the same blocks as its children, so
			   only leaves go into the total. */
			if (!parents[m.n]) {
				tot.size += num(m.size) || 0;
				tot.bad += num(m.bad) || 0;
				tot.corrected += num(m.corrected) || 0;
				tot.failures += num(m.failures) || 0;
			}
		}

		if (c.parts.length > 1) {
			s += '<tr class="total"><td><\/td><td>Total<\/td><td class="num">' + fmtBytes(tot.size, 1) + '<\/td>';
			if (isNand(c.parts[0].type))
				s += '<td class="num">' + tot.bad + '<\/td><td class="num">' + comma(tot.corrected) + '<\/td><td class="num">' + tot.failures + '<\/td>';
			s += '<\/tr>';
		}

		s += '<\/table><\/div>';
	}

	return s;
}

/* ------------------------------------------------------------------ UBI -- */

function mtdByNum(n) {
	for (var i = 0; i < storage.mtd.length; ++i)
		if (storage.mtd[i].n == n)
			return storage.mtd[i];

	return null;
}

function mtdName(n) {
	var m = mtdByNum(n);

	return m ? m.name : '';
}

/* UBI's eraseblock_size is the usable (logical) size: the physical block
   less its two header pages. The physical size comes from the mtd. */
function ubiBlocks(u) {
	var m = mtdByNum(u.mtd);
	var peb = m ? num(m.erasesize) : null;
	var leb = num(u.ebsize);
	var s = comma(u.pebs) + ' &times; ';

	if (peb && peb != leb)
		s += fmtBytes(peb, 1) + ' (' + fmtBytes(leb, 1) + ' usable each)';
	else
		s += fmtBytes(leb, 1);

	return s + ', ' + comma(u.avail) + ' unallocated';
}

function showUbi() {
	var s = '', u, i, k, v, vols, lvl, left, rows;

	for (i = 0; i < storage.ubi.length; ++i) {
		u = storage.ubi[i];
		left = num(u.reserved);

		/* UBI moves a block that goes bad into the reserve. Once the
		   reserve is gone, the next bad block costs usable space, and
		   once there is none left to give the volume goes read-only. */
		if (u.ro == '1')
			lvl = LVL_BAD;
		else if (left == 0 && num(u.bad) > 0)
			lvl = LVL_BAD;
		else if (num(u.bad) > 0 && left != null && left <= num(u.bad))
			lvl = LVL_WARN;
		else
			lvl = LVL_OK;

		vols = [];
		for (k = 0; k < u.vols.length; ++k) {
			v = u.vols[k];
			vols.push(escapeHTML(v.name) + ' <small>(' + fmtBytes(num(v.lebs) * num(u.ebsize), 1) + ', ' + escapeHTML(v.type) + ')<\/small>');
		}

		rows = [
			['Health', lvlSpan(lvl, u.ro == '1' ? 'Read-only - UBI stopped writing after an error' : null)],
			['On', 'mtd' + u.mtd + (mtdName(u.mtd) ? ' (' + escapeHTML(mtdName(u.mtd)) + ')' : '')],
			['Erase blocks', ubiBlocks(u)],
			['Bad blocks', num(u.bad) ? lvlSpan(LVL_WARN, u.bad) : '0'],
			['Reserve for bad blocks', comma(u.reserved) + ' blocks left'],
			['Highest erase count', comma(u.max_ec)],
			['Volumes', vols.length ? vols.join('<br>') : '&#8212;']
		];

		s += '<div class="section-title">UBI: ubi' + u.n + '<\/div>';
		s += '<div class="section">' + fieldRows(rows) + '<\/div>';
	}

	return s;
}

/* ----------------------------------------------------------------- page -- */

function show() {
	var s = showDisks() + showMtd() + showUbi();

	if (s == '')
		s = '<div class="section-title">Storage<\/div><div class="section">No storage devices were found.<\/div>';

	E('storage-content').innerHTML = s;
}

function init() {
	var c;

	if (((c = cookie.get(cprefix + '_notes_vis')) != null) && (c == '1'))
		toggleVisibility(cprefix, 'notes');

	show();
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

<div id="storage-content"></div>

<!-- / / / -->

<div class="section-title">Notes <small><i><a href="javascript:toggleVisibility(cprefix,'notes');" id="toggleLink-notes"><span id="sesdiv_notes_showhide">(Show)</span></a></i></small></div>
<div class="section" id="sesdiv_notes" style="display:none">
	<ul>
		<li><b>eMMC</b> - Life estimates are reported by the chip in 10% steps. Type A is the SLC area,
			type B the main area; many chips fill in only one. SD cards report neither.</li>
		<li><b>NAND</b> - Some bad blocks from the factory are normal. ECC counters reset at boot:
			<i>corrected bits</i> are routine, <i>ECC failures</i> mean lost data.</li>
		<li><b>UBI</b> - UBI spreads writes across its whole partition and keeps a reserve of blocks to replace
			ones that go bad. Once the reserve runs out, each further bad block takes space from
			the volumes. SLC NAND is usually rated for 60,000 - 100,000 erases per block.</li>
		<li><b>NVMe / SATA / USB</b> - Read with <tt>smartctl</tt>. Sleeping drives are not woken, and many
			USB enclosures do not pass SMART through.</li>
	</ul>
</div>

<!-- / / / -->

<div id="footer">
	<span id="footer-msg"></span>
	<input type="button" value="Refresh" id="refresh-button" onclick="reloadPage();">
</div>

</td></tr>
</table>
</form>
</body>
</html>
