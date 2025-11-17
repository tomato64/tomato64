<!DOCTYPE html>
<!--
	Tomato GUI
	Copyright (C) 2006-2010 Jonathan Zarate
	http://www.polarcloud.com/tomato/

	WWAN SMS and Signal Strength by Michał Obrembski.

	For use with Tomato Firmware only.
	No part of this file may be used without permission.
-->
<html lang="en-GB">
<head>
<meta http-equiv="content-type" content="text/html;charset=utf-8">
<meta name="robots" content="noindex,nofollow">
<title>[<% ident(); %>] Status: WWAN SMS</title>
<link rel="stylesheet" type="text/css" href="tomato.css?rel=<% version(); %>">
<% css(); %>
<script src="tomato.js?rel=<% version(); %>"></script>

<script>

//	<% nvram(''); %>

var sms_remover = null;
var wwansms = '';
var wannum_selection = cookie.get('wwansms_selection') || 1;
var wwansms_error;

var ref = new TomatoRefresh('wwansms.cgi', 'mwan_num='+wannum_selection, 10, 'wwan_sms_refresh');

ref.refresh = function(text) {
	try {
		eval(text);
	}
	catch (ex) {
	}
	smsGrid.removeAllData();
	smsGrid.populate();
	smsGrid.resort();
}

var smsGrid = new TomatoGrid();

smsGrid.setup = function() {
	this.init('sms-grid',['sort','delete']);
	this.headerSet(['ID','State','Sender','Date','Message']);
}

smsGrid.populate = function() {
	var error_div, buf, i, parseRegex, match;

	/* Removing hasn't been done, wait until it finishes */
	if (sms_remover)
		return;

	error_div = E('notice');
	if (wwansms_error) {
		error_div.style.display = 'inline-block';
		error_div.innerHTML = '<b>Error occurred!<\/b><br><br>Error message: '+wwansms_error;
	}
	else {
		error_div.style.display = 'none';
		buf = wwansms.split('\n');

		for (i = 0; i < buf.length; ++i) {
			parseRegex = /^\+CMGL\:\s([0-9]+),"(.*)","(.*)",,"(.*)"$/g;

			match = parseRegex.exec(buf[i]);

			if (match && match.length == 5) {
				++i;
				if (buf[i].length > 160)
					match[5] = "chunked...";
				else
					match[5] = buf[i];
			}

			if (match && match.length == 6)
				this.insertData(-1, match.slice(1));
		}
	}

	wwansms = '';
}

smsGrid.sortCompare = function(a, b) {
	var col = this.sortColumn;
	var da = a.getRowData();
	var db = b.getRowData();
	var r;

	switch (col) {
		case 0:
			r = cmpInt(da[col], db[col]);
		break;
		default:
			r = cmpText(da[col], db[col]);
		break;
	}

	return this.sortAscending ? r : -r;
}


smsGrid.rpDel = function(e) {
	var smsToRemove = PR(e)._data[0];
	TomatoGrid.prototype.rpDel.call(this, e);
	removeSMS(smsToRemove);
}

function showWait(x) {
	E('wait').style.display = (x ? 'block' : 'none');
	E('spin').style.display = (x ? 'inline-block' : 'none');
}

function removeSMS(smsNum) {
	if (sms_remover)
		return;

	showWait(1);
	sms_remover = new XmlHttp();
	sms_remover.onCompleted = function(text, xml) {
		showWait(0);
		sms_remover = null;
	}
	sms_remover.onError = function(x) {
		alert('error: '+x);
		showWait(0);
		sms_remover = null;
	}

	sms_remover.post('wwansmsdelete.cgi', 'mwan_num='+wannum_selection+'&sms_num='+smsNum);
}

function verifyFields(focused, quiet) {
	return true;
}

function init() {
	E('sec-title').innerHTML = 'WWAN SMS list for modem '+wannum_selection;

	ref.initPage(250, 10);
	if (!ref.running) {
		ref.once = 1;
		ref.start();
	}
	smsGrid.sort(0);
}
</script>

</head>
<body onload="init()">
<form action="javascript:{}">
<table id="container">
<tr><td colspan="2" id="header">
	<div class="title"><a href="/">Tomato64</a></div>
	<div class="version">Version <% version(); %> on <% nv("t_model_name"); %><span class="blinking bl2"><script><% anonupdate(); %> anon_update()</script>&nbsp;</span></div>
</td></tr>
<tr id="body"><td id="navi"><script>navi()</script></td>
<td id="content">
<div id="ident"><% ident(); %> | <script>wikiLink();</script></div>

<!-- / / / -->

<div id='sec-title' class="section-title">WWAN SMS list for modem</div>

<div id="notice" style="display:none"></div>

<div class="tomato-grid" id="sms-grid"></div>

<div id="wait">Please wait...&nbsp; <img src="spin.svg" alt="" id="spin"></div><p>

<!-- / / / -->

<div id="footer">
	<script>genStdRefresh(1,10,'ref.toggle()');</script>
</div>

</td></tr>
</table>
</form>
<script>smsGrid.setup()</script>
</body>
</html>
