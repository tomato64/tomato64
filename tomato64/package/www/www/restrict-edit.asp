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
<title>[<% ident(); %>] Edit Access Restrictions</title>
<link rel="stylesheet" type="text/css" href="tomato.css?rel=<% version(); %>">
<% css(); %>
<script src="tomato.js?rel=<% version(); %>"></script>
<script src="protocols.js?rel=<% version(); %>"></script>

<script>

//	<% nvram(''); %>

/* TOMATO64-REMOVE-BEGIN */
//	<% layer7(); %>
/* TOMATO64-REMOVE-END */

/* TOMATO64-BEGIN */
//	<% ndpi(); %>
/* TOMATO64-END */

//	<% nvramseq("rrules", "rrule%d", 0, 99); %>

//	<% rrule(); %>

/* {enable}|{begin_mins}|{end_mins}|{dow}|{comp[<comp]}|{rules<rules[...]>}|{http[ ...]}|{http_file}|{desc} */

/* adding a new rule - find free slot */
if (rrule == '') {
	for (var i = 0; i < 100; ++i) {
		if ((rrules[i] == null) || (rrules[i] == '')) {
			rruleN = i;
			break;
		}
	}
}

if ((rule = rrule.match(/^(\d+)\|(-?\d+)\|(-?\d+)\|(\d+)\|(.*?)\|(.*?)\|([^|]*?)\|(\d+)\|(.*)$/m)) == null)
	rule = ['', 1, 1380, 240, 31, '', '', '', 0, 'New Rule '+(rruleN + 1)];

rule[2] *= 1;
rule[3] *= 1;
rule[4] *= 1;
rule[8] *= 1;

/* TOMATO64-REMOVE-BEGIN */
layer7.sort();
for (i = 0; i < layer7.length; ++i)
	layer7[i] = [layer7[i],layer7[i]];
layer7.unshift(['','Layer 7 (disabled)']);

var ipp2p = [[0,'IPP2P (disabled)'],[0xFFFF,'All IPP2P Filters'],[1,'AppleJuice'],[2,'Ares'],[4,'BitTorrent'],[8,'Direct Connect'],[16,'eDonkey'],[32,'Gnutella'],
             [64,'Kazaa'],[128,'Mute'],[4096,'PPLive/UUSee'],[256,'SoulSeek'],[512,'Waste'],[1024,'WinMX'],[2048,'XDCC'],[8192,'Xunlei/QQCyclone']];
/* TOMATO64-REMOVE-END */

/* TOMATO64-BEGIN */
for (i = 0; i < ndpi.length; ++i)
	ndpi[i] = [ndpi[i],ndpi[i]];
ndpi.unshift(['', 'nDPI (disabled)']);
/* TOMATO64-END */

var dowNames = ['Sun','Mon','Tue','Wed','Thu','Fri','Sat'];
var addrestrict = 0;

var cg = new TomatoGrid();

cg.setup = function() {
	var a, i, count, ex;

	this.init('res-comp-grid', 'sort', 140, [ { type: 'text', maxlen: 32 } ]);
	this.headerSet(['MAC / IP Address']);
	this.showNewEditor();
	this.resetNewEditor();

	/* wireless disable rule */
	if (rule[5] == '~')
		return;

	ex = 0;
	count = 0;
	a = rule[5].split('>');
	for (i = 0; i < a.length; ++i) {
		if (!a[i].length)
			continue;

		if (a[i] == '!')
			ex = 1;
		else {
			cg.insertData(-1, [a[i]]);
			++count;
		}
	}

	a = E('_f_comp_all')
	if (count || addrestrict == 1)
		a.value = ex ? 2 : 1;
	else
		a.value = 0;
}

cg.resetNewEditor = function() {
	var f, c;
	f = fields.getAll(this.newEditor);
	ferror.clearAll(f);

	if ((c = cookie.get('addrestrict')) != null) {
		cookie.set('addrestrict', '', 0);
		c = c.split(',');
		if (c.length >= 1 && c != '') {
			addrestrict = 1;
			E('_f_desc').value = (c[1] ? c[1] : c[0]);
			E('_f_sched_allday').checked = 1;
			E('_f_sched_everyday').checked = 1;
			E('_f_comp_all').options[1].selected = 1;
			f[0].value = c[0];
			return;
		}
	}

	f[0].value = '';
}

cg.verifyFields = function(row, quiet) {
	var f = fields.getAll(row)[0];
	if (v_mac(f, true))
		return true;
	if (_v_iptaddr(f, true, false, true, true))
		return true;

	ferror.set(f, 'Invalid MAC address or IP address/range', quiet);

	return false;
}

var bpg = new TomatoGrid();

bpg.setup = function() {
	var a, i, r, count, protos;

	protos = [[-2, 'Any Protocol'],[-1,'TCP/UDP'],[6,'TCP'],[17,'UDP']];
	for (i = 0; i < 256; ++i) {
		if ((i != 6) && (i != 17))
			protos.push([i, protocols[i] || i]);
	}

	this.init('res-bp-grid', 'sort', 140, [ { multi: [
		{ type: 'select', prefix: '<div class="box1">', suffix: '<\/div>', options: protos },
		{ type: 'select', prefix: '<div class="box2">', suffix: '<\/div>', options: [['a','Any Port'],['d','Dst Port'],['s','Src Port'],['x','Src or Dst']] },
		{ type: 'text', prefix: '<div class="box3">', suffix: '<\/div>', maxlen: 32 },
/* TOMATO64-REMOVE-BEGIN */
		{ type: 'select', prefix: '<div class="box4">', suffix: '<\/div>', options: ipp2p },
		{ type: 'select', prefix: '<div class="box5">', suffix: '<\/div>', options: layer7 },
/* TOMATO64-REMOVE-END */
/* TOMATO64-BEGIN */
		{ type: 'select', prefix: '<div class="box4">', suffix: '<\/div>', options: ndpi },
/* TOMATO64-END */
		{ type: 'select', prefix: '<div class="box6">', suffix: '<\/div>', options: [[0,'Any Address'],[1,'Dst IP'],[2,'Src IP']] },
		{ type: 'text', prefix: '<div class="box7">', suffix: '<\/div>', maxlen: 64 }
		] } ] );
	this.headerSet(['Rules']);
	this.showNewEditor();
	this.resetNewEditor();
	count = 0;

	/* proto<dir<port<ipp2p<layer7[<addr_type<addr] */

	a = rule[6].split('>');
	for (i = 0; i < a.length; ++i) {
		r = a[i].split('<');
/* TOMATO64-REMOVE-BEGIN */
		if (r.length == 7) {
/* TOMATO64-REMOVE-END */
/* TOMATO64-BEGIN */
		if (r.length == 6) {
/* TOMATO64-END */
			r[2] = r[2].replace(/:/g, '-');
			this.insertData(-1, r);
			++count;
		}
	}

	return count;
}

bpg.resetNewEditor = function() {
	var f = fields.getAll(this.newEditor);
	f[0].selectedIndex = 0;
	f[1].selectedIndex = 0;
	f[2].value = '';
	f[3].selectedIndex = 0;
/* TOMATO64-REMOVE-BEGIN */
	f[4].selectedIndex = 0;
	f[5].selectedIndex = 0;
	f[6].value = '';
/* TOMATO64-REMOVE-END */
/* TOMATO64-BEGIN */
	f[4].selectedIndex = 0;
	f[5].value = '';
/* TOMATO64-END */
	this.enDiFields(this.newEditor);
	ferror.clearAll(fields.getAll(this.newEditor));
}

bpg._createEditor = bpg.createEditor;
bpg.createEditor = function(which, rowIndex, source) {
	var row = this._createEditor(which, rowIndex, source);
	if (which == 'edit')
		this.enDiFields(row);

	return row;
}

bpg.dataToView = function(data) {
	var s, i;

	s = '';
/* TOMATO64-REMOVE-BEGIN */
	if (data[5] != 0)
		s = ((data[5] == 1) ? 'To ' : 'From ')+data[6]+', ';
/* TOMATO64-REMOVE-END */
/* TOMATO64-BEGIN */
	if (data[4] != 0)
		s = ((data[4] == 1) ? 'To ' : 'From ')+data[5]+', ';
/* TOMATO64-END */

	if (data[0] <= -2)
		s += (s.length ? 'a' : 'A')+'ny protocol';
	else if (data[0] == -1)
		s += 'TCP/UDP';
	else if (data[0] >= 0)
		s += protocols[data[0]] || data[0];

	if (data[0] >= -1) {
		if (data[1] == 'd')
			s += ', dst port ';
		else if (data[1] == 's')
			s += ', src port ';
		else if (data[1] == 'x')
			s += ', port ';
		else
			s += ', all ports';

		if (data[1] != 'a')
			s += data[2].replace(/:/g, '-');
	}

/* TOMATO64-REMOVE-BEGIN */
	if (data[3] != 0) {
		for (i = 0; i < ipp2p.length; ++i) {
			if (data[3] == ipp2p[i][0]) {
				s += ', IPP2P: '+ipp2p[i][1];
				break;
			}
		}
	}
	else if (data[4] != '')
		s += ', L7: '+data[4];
/* TOMATO64-REMOVE-END */
/* TOMATO64-BEGIN */
	if (data[3] != '')
		s += ', nDPI: '+data[3];
/* TOMATO64-END */

	return [s];
}

bpg.fieldValuesToData = function(row) {
	var f = fields.getAll(row);

/* TOMATO64-REMOVE-BEGIN */
	return [f[0].value, f[1].value, (f[1].selectedIndex == 0) ? '' : f[2].value, f[3].value, f[4].value, f[5].value, (f[5].selectedIndex == 0) ? '' : f[6].value];
/* TOMATO64-REMOVE-END */
/* TOMATO64-BEGIN */
	return [f[0].value, f[1].value, (f[1].selectedIndex == 0) ? '' : f[2].value, f[3].value, f[4].value, (f[4].selectedIndex == 0) ? '' : f[5].value];
/* TOMATO64-END */
}

bpg.enDiFields = function(row) {
	var x;
	var f = fields.getAll(row);

	x = f[0].value;
	x = ((x != -1) && (x != 6) && (x != 17));
	f[1].disabled = x;
	if (f[1].selectedIndex == 0)
		x = 1;

	f[2].disabled = x;
/* TOMATO64-REMOVE-BEGIN */
	f[3].disabled = (f[4].selectedIndex != 0);
	f[4].disabled = (f[3].selectedIndex != 0);
	f[6].disabled = (f[5].selectedIndex == 0);
/* TOMATO64-REMOVE-END */
/* TOMATO64-BEGIN */
	f[5].disabled = (f[4].selectedIndex == 0);
/* TOMATO64-END */
}

bpg.verifyFields = function(row, quiet) {
	var f = fields.getAll(row);
	ferror.clearAll(f);
	this.enDiFields(row);

/* TOMATO64-REMOVE-BEGIN */
	if ((f[5].selectedIndex != 0) && ((!v_length(f[6], quiet, 1)) || (!_v_iptaddr(f[6], quiet, false, true, true))))
/* TOMATO64-REMOVE-END */
/* TOMATO64-BEGIN */
	if ((f[4].selectedIndex != 0) && ((!v_length(f[5], quiet, 1)) || (!_v_iptaddr(f[5], quiet, false, true, true))))
/* TOMATO64-END */
		return 0;
	if ((f[1].selectedIndex != 0) && (!v_iptport(f[2], quiet)))
		return 0;

/* TOMATO64-REMOVE-BEGIN */
	if ((f[1].selectedIndex == 0) && (f[3].selectedIndex == 0) && (f[4].selectedIndex == 0) && (f[5].selectedIndex == 0)) {
		var m = 'Please enter a specific address or port, or select an application match';
		ferror.set(f[3], m, 1);
		ferror.set(f[4], m, 1);
		ferror.set(f[5], m, 1);
		ferror.set(f[1], m, quiet);
		return 0;
	}
/* TOMATO64-REMOVE-END */
/* TOMATO64-BEGIN */
	if ((f[1].selectedIndex == 0) && (f[3].selectedIndex == 0) && (f[4].selectedIndex == 0)) {
		var m = 'Please enter a specific address or port, or select an application match';
		ferror.set(f[3], m, 1);
		ferror.set(f[4], m, 1);
		ferror.set(f[1], m, quiet);
		return 0;
	}
/* TOMATO64-END */

	ferror.clear(f[1]);
	ferror.clear(f[3]);
	ferror.clear(f[4]);
	ferror.clear(f[5]);
/* TOMATO64-REMOVE-BEGIN */
	ferror.clear(f[6]);
/* TOMATO64-REMOVE-END */

	return 1;
}

function cancel() {
	document.location = 'restrict.asp';
}

function remove() {
	if (!confirm('Delete this rule?'))
		return;

	E('delete-button').disabled = 1;

	var e = E('t_rrule');
	e.name = 'rrule'+rruleN;
	e.value = '';
	form.submit('t_fom');
}

function verifyFields(focused, quiet) {
	var b, e;

	tgHideIcons();

	elem.display(PR('_f_sched_begin'), !E('_f_sched_allday').checked);
	elem.display(PR('_f_sched_sun'), !E('_f_sched_everyday').checked);

	b = E('rt_norm').checked;
	elem.display(PR('_f_comp_all'), PR('_f_block_all'), b);

	elem.display(PR('res-comp-grid'), b && E('_f_comp_all').value != 0);
	elem.display(PR('res-bp-grid'), PR('_f_block_http'), PR('_f_activex'), b && !E('_f_block_all').checked);

	ferror.clear('_f_comp_all');

	e = E('_f_block_http');
	e.value = e.value.replace(/[|"']/g, ' ');
	if (!v_length(e, quiet, 0, 2048 - 16))
		return 0;

	e = E('_f_desc');
	e.value = e.value.replace(/\|/g, '_');
	if (!v_length(e, quiet, 1))
		return 0;

	return 1;
}

function save() {
	if (!verifyFields(null, 0))
		return;
	if ((cg.isEditing()) || (bpg.isEditing()))
		return;

	var fom = E('t_fom');
	var a, b, e, s, n, data;

	data = [];
	data.push(fom._f_enabled.checked ? '1' : '0');
	if (fom._f_sched_allday.checked)
		data.push(-1, -1);
	else
		data.push(fom._f_sched_begin.value, fom._f_sched_end.value);

	if (fom._f_sched_everyday.checked)
		n = 0x7F;
	else {
		n = 0;
		for (i = 0; i < 7; ++i) {
			if (E('_f_sched_'+dowNames[i].toLowerCase()).checked)
				n |= (1 << i);
		}
		if (n == 0)
			n = 0x7F;
	}
	data.push(n);

	if (fom.rt_norm.checked) {
		e = fom._f_comp_all;
		if (e.value != 0) {
			a = cg.getAllData();
			if (a.length == 0) {
				ferror.set(e, 'No MAC or IP address was specified', 0);
				return;
			}
			if (e.value == 2)
				a.unshift('!');

			data.push(a.join('>'));
		}
		else
			data.push('');

		if (fom._f_block_all.checked)
			data.push('', '', '0');
		else {
			var check = 0;
			a = bpg.getAllData();
			check += a.length;
			b = [];
			for (i = 0; i < a.length; ++i) {
				a[i][2] = a[i][2].replace(/-/g, ':');
				b.push(a[i].join('<'));
			}
			data.push(b.join('>'));

			a = fom._f_block_http.value.replace(/\r+/g, ' ').replace(/\n+/g, '\n').replace(/ +/g, ' ').replace(/^\s+|\s+$/g, '');
			check += a.length;
			data.push(a);

			n = 0;
			if (fom._f_activex.checked)
				n = 1;
			if (fom._f_flash.checked)
				n |= 2;
			if (fom._f_java.checked)
				n |= 4;

			data.push(n);
			
			if (((check + n) == 0) && (data[0] == 1)) {
				alert('Please specify what items should be blocked');
				return;
			}
		}
	}
	else {
		data.push('~');
		data.push('', '', '', '0');
	}

	data.push(fom._f_desc.value);
	data = data.join('|');

	if (data.length >= 2048) {
		alert('This rule is too big. Please reduce by '+(data.length - 2048)+' characters');
		return;
	}

	e = fom.t_rrule;
	e.name = 'rrule'+rruleN;
	e.value = data;

	E('delete-button').disabled = 1;

	form.submit(fom);
}

function earlyInit() {
	if (rrule == '')
		E('delete-button').style.display = 'none';

	cg.setup();
	var count = bpg.setup();
	E('_f_block_all').checked = (count == 0) && (rule[7].search(/[^\s\r\n]/) == -1) && (rule[8] == 0);

	verifyFields(null, 1);
}

function init() {
	cg.recolor();
	bpg.recolor();
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

<input type="hidden" name="_redirect" value="restrict.asp">
<input type="hidden" name="_service" value="restrict-restart">
<input type="hidden" name="rruleNN" id="t_rrule" value="">

<!-- / / / -->

<div class="section-title">Access Restriction</div>
<div class="section">
	<script>
		W('<div><small>'+ 'ID: '+rruleN.pad(2)+'<\/small>&nbsp;<\/div><br>');
		var tm = [];
		for (i = 0; i < 1440; i += 15)
			tm.push([i, timeString(i)]);

		createFieldTable('', [
			{ title: 'Enabled', name: 'f_enabled', type: 'checkbox', value: rule[1] == '1' },
			{ title: 'Description', name: 'f_desc', type: 'text', maxlen: 32, size: 35, value: rule[9] },
			{ title: 'Schedule', multi: [
				{ name: 'f_sched_allday', type: 'checkbox', suffix: ' All Day &nbsp;&nbsp;', value: (rule[2] < 0) || (rule[3] < 0) },
				{ name: 'f_sched_everyday', type: 'checkbox', suffix: ' Everyday', value: (rule[4] & 0x7F) == 0x7F } ] },
				{ title: 'Time', indent: 2, multi: [
					{ name: 'f_sched_begin', type: 'select', options: tm, value: (rule[2] < 0) ? 0 : rule[2], suffix: ' - ' },
					{ name: 'f_sched_end', type: 'select', options: tm, value: (rule[3] < 0) ? 0 : rule[3] } ] },
				{ title: 'Days', indent: 2, multi: [
					{ name: 'f_sched_sun', type: 'checkbox', suffix: ' Sun &nbsp; ', value: (rule[4] & 1) },
					{ name: 'f_sched_mon', type: 'checkbox', suffix: ' Mon &nbsp; ', value: (rule[4] & (1 << 1)) },
					{ name: 'f_sched_tue', type: 'checkbox', suffix: ' Tue &nbsp; ', value: (rule[4] & (1 << 2)) },
					{ name: 'f_sched_wed', type: 'checkbox', suffix: ' Wed &nbsp; ', value: (rule[4] & (1 << 3)) },
					{ name: 'f_sched_thu', type: 'checkbox', suffix: ' Thu &nbsp; ', value: (rule[4] & (1 << 4)) },
					{ name: 'f_sched_fri', type: 'checkbox', suffix: ' Fri &nbsp; ', value: (rule[4] & (1 << 5)) },
					{ name: 'f_sched_sat', type: 'checkbox', suffix: ' Sat', value: (rule[4] & (1 << 6)) } ] },
			{ title: 'Type', name: 'f_type', id: 'rt_norm', type: 'radio', suffix: ' Normal Access Restriction', value: (rule[5] != '~') },
/* TOMATO64-REMOVE-BEGIN */
			{ title: '', name: 'f_type', id: 'rt_wl', type: 'radio', suffix: ' Disable Wireless', value: (rule[5] == '~') },
/* TOMATO64-REMOVE-END */
			{ title: 'Applies To', name: 'f_comp_all', type: 'select', options: [[0,'All Computers / Devices'],[1,'The Following...'],[2,'All Except...']], value: 0 },
			{ title: '&nbsp;', text: '<div class="tomato-grid" id="res-comp-grid"><\/div>' },
			{ title: 'Blocked Resources', name: 'f_block_all', type: 'checkbox', suffix: ' Block All Internet Access', value: 0 },
				{ title: 'Port / Application', indent: 2, text: '<div class="tomato-grid" id="res-bp-grid"><\/div>' },
				{ title: 'HTTP Request', indent: 2, name: 'f_block_http', type: 'textarea', value: rule[7] },
				{ title: 'HTTP Requested Files', indent: 2, multi: [
					{ name: 'f_activex', type: 'checkbox', suffix: ' ActiveX (ocx, cab) &nbsp;&nbsp;', value: (rule[8] & 1) },
					{ name: 'f_flash', type: 'checkbox', suffix: ' Flash (swf) &nbsp;&nbsp;', value: (rule[8] & 2) },
					{ name: 'f_java', type: 'checkbox', suffix: ' Java (class, jar) &nbsp;&nbsp;', value: (rule[8] & 4) } ] }
		]);
	</script>
</div>

<!-- / / / -->

<div id="footer">
	<span id="footer-msg"></span>
	<input type="button" value="Delete" id="delete-button" onclick="remove()">
	<input type="button" value="Save" id="save-button" onclick="save()">
	<input type="button" value="Cancel" id="cancel-button" onclick="cancel()">
</div>

</td></tr>
</table>
<script>earlyInit();</script>
</form>
</body>
</html>
