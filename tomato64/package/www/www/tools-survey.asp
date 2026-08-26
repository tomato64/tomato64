<!DOCTYPE html>
<!--
	FreshTomato GUI
	Copyright (C) 2018 - 2026 pedro
	https://freshtomato.org/

	For use with FreshTomato Firmware only.
	No part of this file may be used without permission.
-->
<html lang="en-GB">
<head>
<meta http-equiv="content-type" content="text/html;charset=utf-8">
<meta name="robots" content="noindex,nofollow">
<title>[<% ident(); %>] Tools: Wireless Survey</title>
<link rel="stylesheet" type="text/css" href="tomato.css?rel=<% version(); %>">
<% css(); %>
<script src="tomato.js?rel=<% version(); %>"></script>

<script>

//	<% nvram("wl_radio,wl_nband,wl_mode,wl_closed,wl_ifname,web_css,web_adv_scripts"); %>
/* TOMATO64-BEGIN */
//	<% wlifaces(); %>
//	<% wlstats(); %>
/* TOMATO64-END */


var cprefix = 'tools_wlsurvey';
var div24 = 21; /* 2.4GHz horizontal divisions */
var div5 = 39; /* 5GHz horizontal divisions */
/* TOMATO64-BEGIN */
var div6 = 61; /* 6GHz horizontal divisions */
var min6 = -4; /* 6GHz leftmost channel number, channels run 1..233 spaced 4 apart */
var max6 = 240; /* 6GHz rightmost channel number */
/* TOMATO64-END */
var vdiv = 9; /* vertical divisions */
var vsize = 200; /* absolute canvas height */
var hsize = 756; /* absolute canvas width */
/* ADVTHEMES-BEGIN */
if (nvram.web_css.match(/at-/g)) {
	vsize = 450;
	hsize = 1200;
}
/* ADVTHEMES-END */
var SVG_NS = 'http://www.w3.org/2000/svg';
var observers = {};

/* TOMATO64-REMOVE-BEGIN */
function internalWiFI(interface) {
	return new Promise(function(resolve, reject) {
		var cmd = new XmlHttp();
		cmd.onCompleted = function(text, xml) {
			resolve(text);
		};
		cmd.onError = function(x) {
			var error = 'ERROR: '+x.replace(/\x0a/g, ' ');
			reject(error);
		};
		var c = '/usr/sbin/wl -i '+interface+' status';
		cmd.post('shell.cgi', 'action=execute&command='+encodeURIComponent(c.replace(/\r/g, '')));
	});
}

function runShell(command) {
	return new Promise(function(resolve, reject) {
		var cmd = new XmlHttp();
		cmd.onCompleted = function(text, xml) {
			resolve(text);
		};
		cmd.onError = function(x) {
			var error = 'ERROR: '+x.replace(/\x0a/g, ' ');
			reject(error);
		};
		cmd.post('shell.cgi', 'action=execute&command='+encodeURIComponent(String(command || '').replace(/\r/g, '')));
	});
}

function decodeWlStatus(text) {
	return String(text || '')
		.replace(/\r/g, '')
		.replace(/\x0a/g, ' ')
		.replace(/\\x([0-9A-Fa-f]{2})/g, function(match, p1) {
			return String.fromCharCode(parseInt(p1, 16));
		});
}

function parseWlScanResults(text) {
	var out = [];
	var t = String(text || '').replace(/\r/g, '');
	var blocks = t.split(/(?=SSID:\s*")/);

	for (var b = 0; b < blocks.length; b++) {
		var block = blocks[b];
		if (!block.trim()) continue;

		var ssidM = block.match(/SSID:\s*"([^\"]*)"/);
		var bssidM = block.match(/BSSID:\s*([0-9A-Fa-f:]{17})/);
		if (!bssidM) continue;
		var bssid = bssidM[1];

		var rssiM = block.match(/RSSI:\s*(-?\d+)\s*dBm/);
		var priM = block.match(/Primary channel:\s*(\d+)/);
		var chanM = block.match(/\bChannel:\s*(\d+)\b/i);
		var csLine = block.match(/Chanspec:\s*([^\n]+)/i);
		var csStr = csLine ? csLine[1] : '';
		var csFreqM = csStr.match(/(\d+(?:\.\d+)?)GHz/i);
		var csChanM = csStr.match(/channel\s*(\d+)/i);
		var csWidthM = csStr.match(/(\d+)MHz/i);
		var modeM = block.match(/Mode:\s*([^\n]+)/i);
		var bwM = block.match(/Bandwidth:\s*(\d+)\s*MHz/i);

		var ssid = ssidM ? ssidM[1] : '';
		var rssi = rssiM ? parseInt(rssiM[1], 10) : -999;
		var control = priM ? priM[1] : (csChanM ? csChanM[1] : (chanM ? chanM[1] : ''));
		var central = csChanM ? csChanM[1] : control;
		var width = '20';
		if (csWidthM)
			width = csWidthM[1];
		else if (bwM)
			width = bwM[1];
		else if (modeM) {
			var mode = modeM[1];
			if (mode.match(/80/))
				width = '80';
			else if (mode.match(/40/))
				width = '40';
			else if (mode.match(/160/))
				width = '160';
		}
		else if (csChanM && priM) {
			var diff = Math.abs(parseInt(csChanM[1], 10) - parseInt(priM[1], 10));
			if (diff >= 14)
				width = '160';
			else if (diff >= 6)
				width = '80';
			else if (diff >= 2)
				width = '40';
		}
		var freq = csFreqM ? csFreqM[1] : '';
		if (freq) {
			if (freq.indexOf('2') === 0)
				freq = '2.4';
			else if (freq.indexOf('5') === 0)
				freq = '5';
		}

		if (!control || rssi == -999)
			continue;

		if (!freq)
			freq = (parseInt(control, 10) > 35) ? '5' : '2.4';

		out.push([ bssid, ssid, rssi, control, width, 100, '', '', '', freq, central, 0 ]);
	}
	return out;
}

function fetchShellScanResults() {
	var ifnames = [];
	if ((Number(wl0.radio.value) === 1) && wl0.ifname.value)
		ifnames.push(wl0.ifname.value);

	if ((Number(wl1.radio.value) === 1) && wl1.ifname.value)
		ifnames.push(wl1.ifname.value);

	if ((Number(wl2.radio.value) === 1) && wl2.ifname.value)
		ifnames.push(wl2.ifname.value);

	var cmds = [];
	for (var i = 0; i < ifnames.length; i++) {
		var ifn = ifnames[i];
		cmds.push(runShell('/usr/sbin/wl -i '+ifn+' scan >/dev/null 2>&1; sleep 2; /usr/sbin/wl -i '+ifn+' scanresults'));
	}

	if (!cmds.length)
		return Promise.resolve([]);

	return Promise.all(cmds).then(function(texts) {
		var merged = [];
		var seen = {};
		for (var i = 0; i < texts.length; i++) {
			var parsed = parseWlScanResults(decodeWlStatus(texts[i]));
			for (var j = 0; j < parsed.length; j++) {
				var row = parsed[j];
				if (!row || !row[0] || seen[row[0]])
					continue;

				seen[row[0]] = 1;
				merged.push(row);
			}
		}
		return merged;
	}, function(err) {
		console.error('wl scanresults fallback failed:', err);
		return [];
	});
}
/* TOMATO64-REMOVE-END */
/* TOMATO64-BEGIN */
/* No 'wl' binary and no shell.cgi fallback: update.cgi?exec=wlscan runs iwinfo. */
function fetchShellScanResults() {
	return Promise.resolve([]);
}
/* TOMATO64-END */

var res0a = '', res1a = '', res2a = '';
/* TOMATO64-REMOVE-BEGIN */
var wl0 = {
	ifname: { value: nvram.wl0_ifname },
	band:   { value: nvram.wl0_nband },
	radio:  { value: nvram.wl0_radio },
	mode:   { value: nvram.wl0_mode },
	closed: { value: nvram.wl0_closed } }
var wl1 = {
	ifname: { value: nvram.wl1_ifname },
	band:   { value: nvram.wl1_nband },
	radio:  { value: nvram.wl1_radio },
	mode:   { value: nvram.wl1_mode },
	closed: { value: nvram.wl1_closed } }
var wl2 = {
	ifname: { value: nvram.wl2_ifname },
	band:   { value: nvram.wl2_nband },
	radio:  { value: nvram.wl2_radio },
	mode:   { value: nvram.wl2_mode },
	closed: { value: nvram.wl2_closed } }

if ((parseInt(nvram.wl0_nband) === 1) && wl1.ifname.value) {
	var temp = wl0;
	wl0 = wl1;
	wl1 = temp;
}
/* TOMATO64-REMOVE-END */
/* TOMATO64-BEGIN */
/*
 * There is no wl0_* nvram on the OpenWrt stack, so build the three radio slots
 * the rest of this page expects out of wl_ifaces + wlstats. Both are produced
 * by the same enabled-AP-interface walk in httpd, so they share an index.
 *
 * Unlike FreshTomato, where slot 2 is a second 5 GHz radio, slot 2 is the
 * 6 GHz radio here.
 */
function wlBandOfMhz(mhz) {
	mhz = Number(mhz) || 0;
	if (mhz >= 2400 && mhz < 2500)
		return '2.4';
	if (mhz >= 5925)
		return '6';

	return '5';
}

/* True once this BSSID is already in wlscandata, ours or a neighbour's */
function ownBSSID(bssid) {
	for (var i = 0; i < wlscandata.length; ++i)
		if (wlscandata[i] && (wlscandata[i][0] === bssid))
			return true;

	return false;
}

/* True if the BSSID belongs to one of this router's own AP interfaces */
function isOurs(bssid) {
	if (typeof wl_ifaces === 'undefined')
		return false;

	for (var i = 0; i < wl_ifaces.length; ++i)
		if (wl_ifaces[i][9] === bssid)
			return true;

	return false;
}

function wlRadioSlot(band) {
	var r = {
		ifname: { value: '' },
		band:   { value: (band == '2.4') ? '0' : '1' },
		radio:  { value: '0' },
		mode:   { value: '' },
		closed: { value: '' },
		freq: band, noise: null, bssid: null, ssid: '', rssi: null,
		controlchannel: null, centralchannel: null, width: null
	};

	if (typeof wl_ifaces === 'undefined' || typeof wlstats === 'undefined')
		return r;

	for (var i = 0; i < wl_ifaces.length; ++i) {
		var st = wlstats[i];
		if (!st || (wlBandOfMhz(st.mhz) != band))
			continue;

		r.ifname.value = wl_ifaces[i][0];
		r.radio.value = '1';
		r.mode.value = 'ap';
		r.ssid = wl_ifaces[i][4];
		r.bssid = wl_ifaces[i][9];
		r.rssi = -10; /* our own AP, same convention FreshTomato uses */
		r.noise = (st.noise != null && st.noise > -99) ? st.noise : null;
		r.controlchannel = st.channel;
		r.centralchannel = st.center ? st.center : st.channel;
		r.width = st.nbw ? st.nbw : 20;
		break;
	}

	return r;
}

var wl0 = wlRadioSlot('2.4');
var wl1 = wlRadioSlot('5');
var wl2 = wlRadioSlot('6');
/* TOMATO64-END */

/* TOMATO64-BEGIN */
/* The radio slots are filled once, server side, by wlRadioSlot(). */
function refreshInternalWiFi() {
	return Promise.resolve();
}
/* TOMATO64-END */
/* TOMATO64-REMOVE-BEGIN */
function refreshInternalWiFi() {
	function fetch(ifname, enabled, assign, label) {
		if (!enabled || !ifname) {
			assign('');
			return Promise.resolve();
		}
		return internalWiFI(ifname).then(function(text) {
			assign(decodeWlStatus(text));
		}, function(err) {
			console.error('Error fetching wl status for '+label+':', err);
			assign('');
		});
	}

	return Promise.all([
		fetch(wl0.ifname.value, Number(wl0.radio.value) === 1, function(v) { res0a = v; }, 'wl0'),
		fetch(wl1.ifname.value, Number(wl1.radio.value) === 1, function(v) { res1a = v; }, 'wl1'),
		fetch(wl2.ifname.value, Number(wl2.radio.value) === 1, function(v) { res2a = v; }, 'wl2')
	]);
}
/* TOMATO64-REMOVE-END */
/* ADVTHEMES-BEGIN */
function resize_graph(id) {
	var targetNode = E('content');
	if (targetNode && targetNode.tagName === 'TD') {
		targetNode.classList.add('dynamic');
	}

	var graph = E('graph'+id);
	if (!graph) return;
	var dest = E('ellipses'+id);

	if (observers[id]) {
		observers[id].disconnect();
	}

	const observer = new ResizeObserver(entries => {
		const width = graph.clientWidth;
		if (!width || width <= 10) return;
		hsize = width - 10;
		vsize = Math.max(0, parseInt(hsize * 0.35));

		dest.setAttribute('width', hsize);
		dest.setAttribute('height', vsize);
		dest.setAttribute('viewBox', '0 0 '+hsize+' '+vsize);
		doit();
	});
	observer.observe(graph);
	observers[id] = observer;
}
/* ADVTHEMES-END */
function hexToDecimal(hexColor) {
	hexColor = hexColor.replace('#', '');
	var red = parseInt(hexColor.substring(0, 2), 16);
	var green = parseInt(hexColor.substring(2, 4), 16);
	var blue = parseInt(hexColor.substring(4, 6), 16);
	return red+', '+green+', '+blue;
}

function svgEl(tag, attrs) {
	var el = document.createElementNS(SVG_NS, tag);
	if (attrs) {
		for (var k in attrs)
			if (attrs.hasOwnProperty(k))
				el.setAttribute(k, attrs[k]);
	}
	return el;
}

function svgSize(svg) {
	return {
		w: Number(svg.getAttribute('width')) || svg.clientWidth || hsize,
		h: Number(svg.getAttribute('height')) || svg.clientHeight || vsize
	};
}

function redraw() {
	clearCanvas('ellipses2');
	clearCanvas('ellipses5');
	drawBoard('ellipses2');
	drawBoard('ellipses5');
/* TOMATO64-BEGIN */
	clearCanvas('ellipses6');
	drawBoard('ellipses6');
/* TOMATO64-END */
}

function drawBaseCoordinates() {
	var max2 = div24 - 2;
	drawCoordinates('ellipses2', -2, max2, 1, 13, div24, 1);
	drawCoordinates('ellipses5', 28, 184, 36, 180, div5, 4);
/* TOMATO64-BEGIN */
	drawCoordinates('ellipses6', min6, max6, 1, 233, div6, 4);
/* TOMATO64-END */
}

function setLoadingOverlay(id, show) {
	var ov = E('loading'+id);
	if (!ov)
		return;

	ov.style.display = show ? 'flex' : 'none';
}

function updateLoadingOverlays() {
	var ellipses2Div = E('tomato-chart2');
	var ellipses5Div = E('tomato-chart5');
	var show2 = ellipses2Div && (ellipses2Div.style.display !== 'none');
	var show5 = ellipses5Div && (ellipses5Div.style.display !== 'none');
/* TOMATO64-REMOVE-BEGIN */
	setLoadingOverlay('2', show2 && !scanReady);
	setLoadingOverlay('5', show5 && !scanReady);
/* TOMATO64-REMOVE-END */
/* TOMATO64-BEGIN */
	/*
	 * Keep spinning until either results land or the note goes up, so the
	 * settling window reads as "still trying" rather than as an empty chart
	 * with nothing said about it.
	 */
	var pending = !scanReady && !wlscanmsgShown;
	var ellipses6Div = E('tomato-chart6');

	setLoadingOverlay('2', show2 && pending);
	setLoadingOverlay('5', show5 && pending);
	setLoadingOverlay('6', ellipses6Div && (ellipses6Div.style.display !== 'none') && pending);
/* TOMATO64-END */
}

function updateFreqFilterOptions(setDefault) {
	var sel = E('freq-filter');
	if (!sel)
		return;

/* TOMATO64-REMOVE-BEGIN */
	var has24 = (Number(wl0.radio.value) === 1);
	var has5 = ((Number(wl1.radio.value) === 1) || (Number(wl2.radio.value) === 1));
	var filterDiv = sel.closest('td');

	if (has24 && has5) {
		if (filterDiv) filterDiv.style.display = '';
		if (setDefault) sel.value = '0';
	}
	else {
		if (filterDiv) filterDiv.style.display = 'none';
		if (setDefault) sel.value = has5 ? '5' : '2.4';
	}
/* TOMATO64-REMOVE-END */
/* TOMATO64-BEGIN */
	var bands = wlBandsPresent();
	var filterDiv = sel.closest('td');

	/* Offer only the bands this router actually has a radio for */
	for (var i = 0; i < sel.options.length; i++) {
		var v = sel.options[i].value;
		sel.options[i].style.display = ((v == '0') || (bands.indexOf(v) >= 0)) ? '' : 'none';
	}

	if (bands.length > 1) {
		if (filterDiv) filterDiv.style.display = '';
		if (setDefault) sel.value = '0';
	}
	else {
		if (filterDiv) filterDiv.style.display = 'none';
		if (setDefault) sel.value = bands.length ? bands[0] : '0';
	}
/* TOMATO64-END */
}

/* TOMATO64-BEGIN */
/* Bands this router has an enabled radio on, in chart order */
function wlBandsPresent() {
	var bands = [];
	if (Number(wl0.radio.value) === 1) bands.push('2.4');
	if (Number(wl1.radio.value) === 1) bands.push('5');
	if (Number(wl2.radio.value) === 1) bands.push('6');

	return bands;
}

/* Show the charts for the bands we have, honouring the Display filter */
function showChartsFor(filter) {
	var bands = wlBandsPresent();
	var charts = [ ['2.4', 'tomato-chart2'], ['5', 'tomato-chart5'], ['6', 'tomato-chart6'] ];

	for (var i = 0; i < charts.length; i++) {
		var div = E(charts[i][1]);
		if (!div)
			continue;

		var wanted = (filter == 0) || (filter == charts[i][0]);
		div.style.display = (wanted && (bands.indexOf(charts[i][0]) >= 0)) ? 'block' : 'none';
	}
}
/* TOMATO64-END */

function recolor() {
	colors = colors.sort(() => Math.random() - 0.5);
	redraw();
	doit();
}

function doit() {
	fillstyle = E('fill-style').value;
	ssidshow = E('ssid-show').value;
	ssidlimit = E('ssid-limit').value;
	var filter = E('freq-filter').value;
	updateFreqFilterOptions();
	if (ssidlimit > 40)
		ssidlimit = 40;

	sg.removeAllData();
	sg.populate(fillstyle, ssidshow, filter);
	sg.resort();
	drawNoise('ellipses2', fillstyle);
	drawNoise('ellipses5', fillstyle);
/* TOMATO64-BEGIN */
	drawNoise('ellipses6', fillstyle);
/* TOMATO64-END */

/* TOMATO64-REMOVE-BEGIN */
	var ellipses2Div = E('tomato-chart2');
	var ellipses5Div = E('tomato-chart5');
	var has24 = (Number(wl0.radio.value) === 1);
	var has5 = ((Number(wl1.radio.value) === 1) || (Number(wl2.radio.value) === 1));

	if (filter == 2.4) {
		ellipses2Div.style.display = has24 ? 'block' : 'none';
		ellipses5Div.style.display = 'none';
	}
	else if (filter == 5) {
		ellipses2Div.style.display = 'none';
		ellipses5Div.style.display = has5 ? 'block' : 'none';
	}
	else {
		ellipses2Div.style.display = has24 ? 'block' : 'none';
		ellipses5Div.style.display = has5 ? 'block' : 'none';
	}
/* TOMATO64-REMOVE-END */
/* TOMATO64-BEGIN */
	showChartsFor(filter);
/* TOMATO64-END */

	updateLoadingOverlays();
}

var colors = [
	'#FF0000', /* Red */
	'#00FF00', /* Lime */
	'#0000FF', /* Blue */
	'#FFA500', /* Orange */
	'#FF00FF', /* Magenta */
	'#FFFF00', /* Yellow */
	'#00FFFF', /* Cyan */
	'#800080', /* Purple */
	'#FF4500', /* OrangeRed */
	'#32CD32', /* LimeGreen */
	'#87CEEB', /* SkyBlue */
	'#FF69B4', /* HotPink */
	'#DC143C', /* Crimson */
	'#00CED1', /* DarkTurquoise */
	'#FF8C00', /* DarkOrange */
	'#4169E1', /* RoyalBlue */
	'#8A2BE2', /* BlueViolet */
	'#FFD700', /* Gold */
	'#ADFF2F', /* GreenYellow */
	'#9400D3', /* DarkViolet */
	'#7CFC00', /* LawnGreen */
	'#8B008B', /* DarkMagenta */
	'#20B2AA', /* LightSeaGreen */
	'#800000', /* Maroon */
	'#000080', /* Navy */
	'#008000', /* Green */
	'#008080', /* Teal */
	'#808000', /* Olive */
	'#800080', /* Purple */
	'#800000', /* Maroon */
	'#2E8B57', /* SeaGreen */
	'#4682B4', /* SteelBlue */
	'#808000', /* Olive */
	'#FF1493', /* DeepPink */
	'#00FFFF', /* Aqua */
	'#008B8B', /* DarkCyan */
	'#0000CD', /* MediumBlue */
	'#D2691E', /* Chocolate */
	'#800080', /* Purple */
	'#20B2AA', /* LightSeaGreen */
	'#556B2F'  /* DarkOliveGreen */
];
var wlscandata = [];
/* TOMATO64-BEGIN */
var wlscanmsg = ''; /* set by exec=wlscan when a radio could not be surveyed */

var wlscanmsgSeen = 0; /* consecutive responses carrying this same message */
var wlscanmsgSince = 0; /* when this message was first reported */
var wlscanmsgShown = ''; /* what the banner is currently displaying */
var wlscanNoteTimer = null;
var SCAN_NOTE_GRACE = 20000; /* ms to let the radios settle before explaining a failure */

/*
 * Track how long a refusal has persisted
 *
 * mac80211 answers -EBUSY both for a radio it may not take off channel (DFS)
 * and for a scan that is merely still in flight somewhere else on the wiphy,
 * and iwinfo reports the two identically. The second clears by itself, so a
 * refusal only becomes news once it repeats, or once it has stood unchanged
 * for the whole settling window.
 */
function noteScanResponse(prev) {
	if (wlscanmsg && (wlscanmsg === prev)) {
		wlscanmsgSeen++;
	}
	else {
		wlscanmsgSeen = wlscanmsg ? 1 : 0;
		wlscanmsgSince = (new Date()).getTime();
	}

	if (wlscanNoteTimer) {
		clearTimeout(wlscanNoteTimer);
		wlscanNoteTimer = null;
	}

	/*
	 * Look again when the window is up. With auto-refresh off there is only
	 * ever one response, so nothing else would revisit the decision.
	 */
	if (wlscanmsg)
		wlscanNoteTimer = setTimeout(showScanNote, SCAN_NOTE_GRACE + 100);
}

/*
 * Surface a radio that could not be surveyed above the charts. An empty band is
 * what the user actually notices, so the reason has to sit next to it - under a
 * grid that can run to fifty rows it just gets scrolled past.
 *
 * Held back until the refusal either repeats or survives the settling window,
 * so a scan that was only busy for a moment never flashes a note up and takes
 * it away again - including on the very first load, before any scan has had a
 * chance to return.
 */
function showScanNote() {
	var e = E('survey-note');
	if (!e)
		return;

	var age = wlscanmsg ? ((new Date()).getTime() - wlscanmsgSince) : 0;
	var msg = (wlscanmsg && ((wlscanmsgSeen > 1) || (age >= SCAN_NOTE_GRACE))) ? wlscanmsg : '';
	if (msg === wlscanmsgShown)
		return;

	wlscanmsgShown = msg;
	elem.setInnerHTML(e, msg ? '<b>Note:<\/b> '+msg : '');
	e.style.display = msg ? 'block' : 'none';
}
/* TOMATO64-END */
var scanReady = false;
var entries = [];
var dayOfWeek = ['Sun','Mon','Tue','Wed','Thu','Fri','Sat'];
var cmd = null;
var ref = new TomatoRefresh('update.cgi', 'exec=wlscan', 0, 'tools_wlsurvey_refresh');

function wlHasBand(freq) {
	if (!wlscandata || !wlscandata.length)
		return false;

	for (var i = 0; i < wlscandata.length; i++) {
		if (wlscandata[i] && (wlscandata[i][9] == freq))
			return true;
	}
	return false;
}

function renderNoData() {
	redraw();
	drawBaseCoordinates();
	updateLoadingOverlays();
}

ref.refresh = function(text) {
	var prevScan = wlscandata;
/* TOMATO64-BEGIN */
	var prevMsg = wlscanmsg;
/* TOMATO64-END */
	try {
		eval(text);
	}
	catch (ex) {
		console.error('wlscan refresh eval failed:', ex);
		wlscandata = prevScan;
	}
/* TOMATO64-BEGIN */
	/* Only a real response counts: the fast poll re-renders with an empty body */
	if (text)
		noteScanResponse(prevMsg);
/* TOMATO64-END */

	var isError = (wlscandata && (wlscandata.length == 1) && (!wlscandata[0][0]));
	var hasData = (wlscandata && wlscandata.length && !isError);

	if (isError) {
		if (!scanReady) {
			renderNoData();
			return;
		}
		wlscandata = prevScan;
	}
	else if (hasData) {
		scanReady = true;
	}
	else if (scanReady && wlscandata && (wlscandata.length === 0)) {
		wlscandata = prevScan;
	}

/* TOMATO64-REMOVE-BEGIN */
	var enabled24 = (Number(wl0.radio.value) === 1);
	var enabled5 = ((Number(wl1.radio.value) === 1) || (Number(wl2.radio.value) === 1));
	var hasBand24 = wlHasBand('2.4');
	var hasBand5 = wlHasBand('5');
	var needsFallback = (enabled24 && !hasBand24) || (enabled5 && !hasBand5);
/* TOMATO64-REMOVE-END */
/* TOMATO64-BEGIN */
	var needsFallback = false;
/* TOMATO64-END */

	var scanP = Promise.resolve();
	if (needsFallback || (!scanReady && (!wlscandata || (wlscandata.length === 0)))) {
		scanP = fetchShellScanResults().then(function(list) {
			if (list && list.length) {
				wlscandata = list;
				scanReady = true;
			}
		});
	}

	Promise.all([refreshInternalWiFi(), scanP]).then(function() {
		doit();
	}, function() {
		doit();
	});
}
var sg = new TomatoGrid();

sg.setup = function() {
	this.init('survey-grid','sort');
	this.headerSet(['Last Seen','RGB','SSID','BSSID','RSSI<br>dBm','SNR<br>dB','Qual','Ctrl/Centr<br>Channel','Security','802.11']);
	this.populate();
	this.sort(4);
}

function drawNoise(board, style) {
	var noise, noiseV;
	var svg = E(board);
	var sz = svgSize(svg);

/* TOMATO64-REMOVE-BEGIN */
	if ((Number(wl0.radio.value) === 1) && (board == 'ellipses2') && res0a && (wl0.noise != null)) {
		noise = ((-wl0.noise - 10) * (sz.h / 100) * (10 / vdiv));
		noiseV = Number(wl0.noise);
	}
	if ((Number(wl1.radio.value) === 1) && (board == 'ellipses5') && res1a && (wl1.noise != null)) {
		noise = ((-wl1.noise - 10) * (sz.h / 100) * (10 / vdiv));
		noiseV = Number(wl1.noise);
	}
	if ((Number(wl2.radio.value) === 1) && (board == 'ellipses5') && res2a && (wl2.noise != null)) {
		noise = ((-wl2.noise - 10) * (sz.h / 100) * (10 / vdiv));
		noiseV = Number(wl2.noise);
	}
/* TOMATO64-REMOVE-END */
/* TOMATO64-BEGIN */
	var wl = (board == 'ellipses2') ? wl0 : (board == 'ellipses5') ? wl1 : wl2;

	if ((Number(wl.radio.value) === 1) && (wl.noise != null)) {
		noise = ((-wl.noise - 10) * (sz.h / 100) * (10 / vdiv));
		noiseV = Number(wl.noise);
	}
/* TOMATO64-END */

	if (!noise)
		return;

	var density = (style / 100);
	svg.appendChild(svgEl('line', {
		x1: 0, y1: noise + 1,
		x2: sz.w, y2: noise + 1,
		stroke: 'rgba(80,0,0,0.5)',
		'stroke-width': 0.2
	}));
	svg.appendChild(svgEl('rect', {
		x: 0, y: noise,
		width: sz.w,
		height: noise,
		fill: 'rgba(189,158,0,'+(density / 3)+')'
	}));
	svg.appendChild(svgEl('text', {
		x: (sz.w - 22),
		y: noise,
		'class': 'svg-text',
		'pointer-events': 'none',
		'text-anchor': 'end',
		'dominant-baseline': 'middle'
	})).appendChild(document.createTextNode('noise '+noiseV));
}

/* TOMATO64-REMOVE-BEGIN */
function drawEllipse(c = -100, m = 20, q, col, ssid, noise, style, sshow) {
/* TOMATO64-REMOVE-END */
/* TOMATO64-BEGIN */
/*
 * 6 GHz channel numbers (1..233) overlap both the 2.4 and the 5 GHz ranges, so
 * the canvas has to be picked from the band rather than from the channel.
 */
function drawEllipse(c = -100, m = 20, q, col, ssid, noise, style, sshow, band) {
/* TOMATO64-END */
	var mf, cf, rf;
	var svg;
	var sz;
/* TOMATO64-REMOVE-BEGIN */
	if (c < 35) {
/* TOMATO64-REMOVE-END */
/* TOMATO64-BEGIN */
	if (band == '2.4') {
/* TOMATO64-END */
		svg = E('ellipses2');
		sz = svgSize(svg);
		if (m == 20)
			mf = (sz.w / div24) * 2.20;
		else if (m == 40)
			mf = (sz.w / div24) * 4.2;
		else
			mf = (sz.w / div24) * 2.20;

		if (c == 1)
			cf = (sz.w / div24) * 3;
		else if (c == 14)
			cf = (sz.w / div24) * 18;
		else
			cf = ((sz.w / div24) * 3) + ((sz.w / div24) * (c - 1));

		rf = q * (sz.h / 100) * (10 / vdiv);
	}
/* TOMATO64-REMOVE-BEGIN */
	else if (c > 35) {
/* TOMATO64-REMOVE-END */
/* TOMATO64-BEGIN */
	else if (band == '5') {
/* TOMATO64-END */
		svg  = E('ellipses5');
		sz = svgSize(svg);
		m = parseInt(m, 10);
		var cc = c;
		var xStep = (sz.w / (184 - 28));
		if (m == 20)
			mf = xStep * 2;
		else if (m == 40)
			mf = xStep * 4;
		else if (m == 80)
			mf = xStep * 8;
		else if (m == 160)
			mf = xStep * 16;
		else
			mf = xStep * 2;

		cc = cc - 36 + 4;
		cf = (sz.w / div5) + ((sz.w / div5) * (cc / 4));
		rf = q * (sz.h / 100) * (10 / vdiv); /* adapt calculation for -10 to -100 only */
	}
/* TOMATO64-BEGIN */
	else if (band == '6') {
		svg = E('ellipses6');
		sz = svgSize(svg);
		m = parseInt(m, 10);

		/*
		 * One channel number is 5 MHz, so an n MHz channel is n/5 channel
		 * numbers wide and the ellipse radius is half of that, n/10.
		 */
		var xStep6 = sz.w / (max6 - min6);
		mf = xStep6 * ((m > 0 ? m : 20) / 10);
		cf = xStep6 * (c - min6);
		rf = q * (sz.h / 100) * (10 / vdiv);
	}
/* TOMATO64-END */
	if (!svg)
		return;

	var decimalColor = hexToDecimal(col);
	var strokeWidth = 1;
	var fillAlpha = 0;
	if (style != '0')
		fillAlpha = (style / 100);

	svg.appendChild(svgEl('ellipse', {
		cx: cf,
		cy: sz.h,
		rx: mf,
		ry: rf,
		stroke: 'rgba('+decimalColor+', 1)',
		'stroke-width': strokeWidth,
		fill: 'rgba('+decimalColor+', '+fillAlpha+')'
	}));

	var lines = ssid.split('¬');
	if (sshow != '0') {
		var y0 = (sz.h + 1 + (sshow / 2)) - rf;
		var t = svgEl('text', {
			x: cf,
			y: y0,
			'class': 'svg-ssid',
			'pointer-events': 'none',
			'text-anchor': 'middle',
			'dominant-baseline': 'middle'
		});
		t.style.setProperty('font-size', sshow+'px', 'important');
		t.appendChild(document.createTextNode(lines[0] || ''));
		if (typeof lines[1] !== 'undefined') {
			t.appendChild(svgEl('tspan', { x: cf, dy: sshow }));
			t.lastChild.appendChild(document.createTextNode(lines[1] || ''));
			t.appendChild(svgEl('tspan', { x: cf, dy: sshow }));
			t.lastChild.appendChild(document.createTextNode(lines[2] || ''));
		}
		svg.appendChild(t);
	}
}

sg.populate = function(style, sshow, filter) {
	var added = 0;
	var removed = 0;
	var i, j, k, t, e, s;

	redraw();

	if (!scanReady && (!wlscandata || (wlscandata.length === 0))) {
		drawBaseCoordinates();
/* TOMATO64-REMOVE-BEGIN */
		setMsg('');
/* TOMATO64-REMOVE-END */
/* TOMATO64-BEGIN */
		/*
		 * Every radio was refused, so there is nothing to plot and nothing
		 * more coming. Say why, or the page just looks broken.
		 */
		setMsg('');
		showScanNote();
/* TOMATO64-END */
		updateLoadingOverlays();
		return;
	}

	var lim2 = 1;
	var lim5 = 1;
	var col2 = 0;
	var col5 = colors.length - 1;

	if ((wlscandata.length == 1) && (!wlscandata[0][0])) {
		drawBaseCoordinates();
		setMsg('');
		return;
	}

	drawFT();
	wlscandata.sort((b, a) => a[2] - b[2]);

	var currentBSSIDs = {};
	for (i = 0; i < wlscandata.length; ++i) {
		if (wlscandata[i][0]) currentBSSIDs[wlscandata[i][0]] = true;
	}

	for (i = 0; i < wlscandata.length; ++i) {
		s = wlscandata[i];
		e = null;
		for (j = 0; j < entries.length; ++j) {
			if (entries[j].bssid == s[0]) {
				e = entries[j];
				break;
			}
		}
		if (!e) {
			++added;
			e = {};
			e.firstSeen = new Date();
			entries.push(e);
		}
		e.lastSeen = new Date();
		e.bssid = s[0];
		if (!s[1])
			e.ssid = '🕶️';
		else
			e.ssid = s[1];

		e.control = s[3];
/* TOMATO64-BEGIN */
		e.band = s[9];
/* TOMATO64-END */
		e.channel = s[10];
		e.channel = e.channel+'<br><small>'+s[9]+' GHz<\/small><br><small>'+s[4]+' MHz<\/small>';
		e.rssi = s[2];
/* TOMATO64-REMOVE-BEGIN */
		if (s[9] == 2.4)
			e.snr = Number(e.rssi) + Math.abs(wl0.noise);
		else
			e.snr = Number(e.rssi) + Math.abs(wl1.noise);
/* TOMATO64-REMOVE-END */
/* TOMATO64-BEGIN */
		var wlb = (s[9] == '2.4') ? wl0 : (s[9] == '6') ? wl2 : wl1;
		e.snr = (wlb.noise != null) ? (Number(e.rssi) + Math.abs(wlb.noise)) : '';
/* TOMATO64-END */
		e.mhz = s[4];
		e.cap = s[7]+ '<br>'+s[8];
		e.rates =s[6].replace('11', '');
		if (e.rssi != -999) {
			if (e.rssi >= -50)
				e.qual = 100;
			else if (e.rssi >= -80) /* between -50 ~ -80dbm */
				e.qual = Math.round(24 + ((e.rssi + 80) * 26)/10);
			else if (e.rssi >= -90) /* between -80 ~ -90dbm */
				e.qual = Math.round(24 + ((e.rssi + 90) * 26)/10);
			else
				e.qual = 0;
		}
		else
			e.qual = -1;
	}

	t = E('expire-time').value;
	if (t > 0) {
		var cut = (new Date()).getTime() - (t * 1000);
		for (i = 0; i < entries.length; ) {
			if (entries[i].lastSeen.getTime() < cut) {
				entries.splice(i, 1);
				++removed;
			}
			else
				++i;
		}
	}

	if (typeof sshow === 'undefined' )
		var sshow = 'hide';

	for (i = 0; i < entries.length; ++i) {
		var seen, m, mac;
		e = entries[i];
		const startIndex = e.channel.indexOf("<small>") + 7; /* find the index of "<small>" and add 7 to skip it */
		const endIndex = e.channel.indexOf(" GHz"); /* find the index of " GHz" */
		var frequency = e.channel.substring(startIndex, endIndex);
		if (filter != frequency && filter != 0)
			continue
		seen = e.lastSeen.toWHMS();
		if (useAjax()) {
			m = Math.floor(((new Date()).getTime() - e.firstSeen.getTime()) / 60000);
			if (m <= 10)
				seen += '<br> <b><small>NEW ('+m+'m)<\/small><\/b>';
		}
		mac = e.bssid;
		var chan = e.channel.split('<');
		if (style == 0)
			var density = 100;
		else
			var density = (style / 100);

		var colo;
/* TOMATO64-REMOVE-BEGIN */
		if (lim2 <= ssidlimit && chan[0] < 35 && col2 <= colors.length - 1) {
/* TOMATO64-REMOVE-END */
/* TOMATO64-BEGIN */
		if (lim2 <= ssidlimit && e.band == '2.4' && col2 <= colors.length - 1) {
/* TOMATO64-END */
/* TOMATO64-REMOVE-BEGIN */
			if (e.bssid === wl0.bssid)
/* TOMATO64-REMOVE-END */
/* TOMATO64-BEGIN */
			if (isOurs(e.bssid))
/* TOMATO64-END */
				colo = '#aaaaaa';
			else {
				colo = colors[col2];
				col2++;
			}
			lim2++;
			var decimalColor = hexToDecimal(colo);
			e.col = '<div style="margin:0 auto;display:block;padding:0px;width:20px;height:40px;background-color:rgba('+decimalColor+','+density+');border:1px solid black;"><\/div>';
/* TOMATO64-REMOVE-BEGIN */
			drawEllipse(chan[0], e.mhz, (100 + e.rssi), colo, e.ssid, 0, style, sshow);
/* TOMATO64-REMOVE-END */
/* TOMATO64-BEGIN */
			drawEllipse(chan[0], e.mhz, (100 + e.rssi), colo, e.ssid, 0, style, sshow, e.band);
/* TOMATO64-END */
		}
/* TOMATO64-REMOVE-BEGIN */
		else if (lim5 <= ssidlimit && chan[0] > 35) {
/* TOMATO64-REMOVE-END */
/* TOMATO64-BEGIN */
		else if (lim5 <= ssidlimit && (e.band == '5' || e.band == '6')) {
/* TOMATO64-END */
/* TOMATO64-REMOVE-BEGIN */
			if ((e.bssid === wl1.bssid) || (e.bssid === wl2.bssid))
/* TOMATO64-REMOVE-END */
/* TOMATO64-BEGIN */
			if (isOurs(e.bssid))
/* TOMATO64-END */
				colo = '#aaaaaa';
			else {
				colo = colors[col5];
				col5--;
			}
			lim5++;
			var decimalColor = hexToDecimal(colo);
			e.col = '<div style="margin:0 auto;display:block;padding:0px;width:20px;height:40px;background-color:rgba('+decimalColor+','+density+');border:1px solid black;"><\/div>';
/* TOMATO64-REMOVE-BEGIN */
			drawEllipse(chan[0], e.mhz, (100 + e.rssi), colo, e.ssid, 0, style, sshow);
/* TOMATO64-REMOVE-END */
/* TOMATO64-BEGIN */
			drawEllipse(chan[0], e.mhz, (100 + e.rssi), colo, e.ssid, 0, style, sshow, e.band);
/* TOMATO64-END */
		}
		else {
			e.col = '<div><\/div>';
		}
/* OUI-BEGIN */
		if (mac.match(/^(..):(..):(..)/))
			mac = '<div style="display:none" id="gW_'+i+'">&nbsp; <img src="spin.svg" alt="" style="vertical-align:middle"><\/div><a href="javascript:searchOUI(\''+RegExp.$1+'-'+RegExp.$2+'-'+RegExp.$3+'\','+i+')" title="OUI Search">'+mac+'<\/a>';
/* OUI-END */

		sg.insert(-1, e, [ '<small>'+seen+'<\/small>', ''+e.col, ''+e.ssid, mac, (e.rssi < 0 ? e.rssi+'' : ''), ''+e.snr,
		          (e.qual < 0 ? '' : '<small>'+e.qual+'<\/small><br><img src="bar'+MIN(MAX(Math.floor(e.qual / 12), 1), 6)+'.gif" id="bar_'+i+'" alt="">'),
		          ''+e.control+'/'+e.channel, '<small>'+e.cap, '<\/small>'+e.rates], false);
	}
	drawBaseCoordinates();
	s = '';
	if (useAjax())
		s = added+' added, '+removed+' removed, ';

	s += entries.length+' total.';
	s += '<br><br><small>Last updated: '+(new Date()).toWHMS()+'<\/small>';
	setMsg(s);
/* TOMATO64-BEGIN */
	showScanNote();
/* TOMATO64-END */

	updateLoadingOverlays();
}

sg.sortCompare = function(a, b) {
	var col = this.sortColumn;
	var da = a.getRowData();
	var db = b.getRowData();
	var r;
	switch (col) {
		case 0:
			r = -cmpDate(da.lastSeen, db.lastSeen);
		break;
		case 2:
			r = -cmpText(da.ssid.toLowerCase(), db.ssid.toLowerCase());
		break;
		case 3:
			r = -cmpText(da.bssid.toString(), db.bssid.toString());
		break;
		case 4:
			r = cmpInt(da.rssi, db.rssi);
		break;
		case 5:
			r = cmpInt(da.snr, db.snr);
		break;
		case 6:
			r = cmpInt(da.qual, db.qual);
		break;
		case 7:
			r = cmpInt(da.channel, db.channel);
		break;
		default:
			r = cmpText(a.cells[col].innerHTML, b.cells[col].innerHTML);
	}

	if (r == 0)
		r = cmpText(da.bssid, db.bssid);

	return this.sortAscending ? -r : r;
}

Date.prototype.toWHMS = function() {
	return dayOfWeek[this.getDay()]+' '+this.getHours()+':'+this.getMinutes().pad(2)+ ':'+this.getSeconds().pad(2);
}

function setMsg(msg) {
	elem.setInnerHTML(E('survey-msg'), msg);
}

function drawFT(show) {
	function parseStatus(wl, status) {
		var m;
		if (!status)
			return;

		m = status.match(/noise: (-?\d+)/);
		wl.noise = m ? m[1] : null;
		m = status.match(/SSID: "([^"]*)"/);
		wl.ssid = m ? m[1] : '';
		m = status.match(/BSSID: (\S+)/);
		wl.bssid = m ? m[1] : null;

		if (wl.mode.value === 'ap')
			wl.rssi = -10;
		else {
			m = status.match(/RSSI:\s*(-?\d+)\s*dBm/);
			wl.rssi = m ? parseInt(m[1], 10) : null;
		}
		m = status.match(/Primary channel: (\d+)/);
		wl.controlchannel = m ? m[1] : null;
		m = status.match(/Chanspec: (\d+(?:\.\d+)?)GHz channel (\d+) (\d+)MHz/);
		wl.centralchannel = m ? m[2] : null;
		wl.width = m ? m[3] : null;
	}

	function pushInternal(wl) {
/* TOMATO64-REMOVE-BEGIN */
		var freq = (Number(wl.band.value) === 1) ? '5' : '2.4';
/* TOMATO64-REMOVE-END */
/* TOMATO64-BEGIN */
		/* nband cannot express 6 GHz, so take the band the slot was built for */
		var freq = wl.freq;
/* TOMATO64-END */
		if ((Number(wl.radio.value) !== 1) || !wl.bssid)
			return;

		var internal = [ wl.bssid, wl.ssid, wl.rssi, wl.controlchannel, wl.width, 100, '', '', '', freq, wl.centralchannel, 0 ];
		for (var i = 0; i < wlscandata.length; ++i) {
			if (wlscandata[i] && (wlscandata[i][0] === wl.bssid))
				return;
		}
		wlscandata.push(internal);
	}

/* TOMATO64-REMOVE-BEGIN */
	parseStatus(wl0, res0a);
	parseStatus(wl1, res1a);
	parseStatus(wl2, res2a);
/* TOMATO64-REMOVE-END */
/* TOMATO64-REMOVE-BEGIN */
	pushInternal(wl0);
	pushInternal(wl1);
	pushInternal(wl2);
/* TOMATO64-REMOVE-END */
/* TOMATO64-BEGIN */
	/*
	 * wl0/wl1/wl2 describe one radio each, but a radio can carry several
	 * BSSIDs and cannot hear any of them, so walk every enabled AP interface.
	 * Otherwise a guest SSID sharing a radio never appears anywhere.
	 */
	if (typeof wl_ifaces !== 'undefined' && typeof wlstats !== 'undefined') {
		for (var n = 0; n < wl_ifaces.length; ++n) {
			var st = wlstats[n];
			var bssid = wl_ifaces[n][9];
			if (!st || !bssid || ownBSSID(bssid))
				continue;

			/*
			 * httpd fills the security, cipher and 802.11 generation in from
			 * configuration, since a radio never hears its own BSSIDs, so
			 * these rows read like any scanned one rather than half empty.
			 */
			wlscandata.push([ bssid, wl_ifaces[n][4], -10, st.channel, (st.nbw ? st.nbw : 20),
			                  100, st.proto || '', st.security || '', st.cipher || '',
			                  wlBandOfMhz(st.mhz), (st.center ? st.center : st.channel), 0 ]);
		}
	}
/* TOMATO64-END */
}

function drawCoordinates(a, b, c, d, e, f, g) {
	var svg = E(a);
	var sz = svgSize(svg);
	var minX = b;
	var maxX = c;
	var minV = d;
	var maxV = e;
	var numDivisions = f;
	var incrementX = g;

	svg.appendChild(svgEl('line', {
		x1: 0, y1: sz.h - 0.05,
		x2: sz.w, y2: sz.h - 0.05,
		stroke: 'black',
		'stroke-width': 1
	}));

	if (a == 'ellipses2') {
		for (var i = 0; i <= numDivisions; i++) {
			var x;
			if (i === 0)
				x = minX + 2 * incrementX;
			else if (i === numDivisions)
				x = maxX - 2 * incrementX;
			else
				x = minX + i * incrementX;

			if (x >= 1 && x <= 13) {
				var xPos = (sz.w / (maxX - minX)) * (x - minX);
				svg.appendChild(svgEl('text', {
					x: xPos,
					y: sz.h,
					'class': 'svg-text',
					'pointer-events': 'none',
					'text-anchor': 'middle',
					'dy': '-0.2em'
				})).appendChild(document.createTextNode(x));
			}
		}
		xPos = (sz.w / (maxX - minX)) * 18;
		svg.appendChild(svgEl('text', {
			x: xPos,
			y: sz.h,
			'class': 'svg-text',
			'pointer-events': 'none',
			'text-anchor': 'middle',
			'dy': '-0.2em'
		})).appendChild(document.createTextNode(14));
	}
	else if (a == 'ellipses5') {
		for (var i = 0; i <= numDivisions; i++) {
			var x;
			if (i === 0)
				x = minX + 1 * incrementX;
			else if (i === numDivisions)
				x = maxX - 1 * incrementX;
			else
				x = minX + (i + 1) * incrementX;

			if ((x > 32 && x <= 64) || (x >= 100 && x <= 144)) {
				var xPos = (sz.w / (maxX - minX)) * (x - minX);
				svg.appendChild(svgEl('text', {
					x: xPos,
					y: sz.h,
					'class': 'svg-text',
					'pointer-events': 'none',
					'text-anchor': 'middle',
					'dy': '-0.2em'
				})).appendChild(document.createTextNode(x));
			}
		}
		for (var i = 0; i <= div5; i++) {
			var x;
			if (i === 0)
				x = 33 + 1 * 4;
			else if (i === div5)
				x = 184 - 1 * 4;
			else
				x = 33 + (i + 1) * 4;

			if (x >= 149 && x <= 177) {
				var xPos = (sz.w / (184 - 33)) * (x - 33);
				svg.appendChild(svgEl('text', {
					x: xPos,
					y: sz.h,
					'class': 'svg-text',
					'pointer-events': 'none',
					'text-anchor': 'middle',
					'dy': '-0.2em'
				})).appendChild(document.createTextNode(x));
			}
		}
	}
/* TOMATO64-BEGIN */
	else if (a == 'ellipses6') {
		/* 6 GHz channels are 1, 5, 9 ... 233; label every fourth one */
		for (var x = 1; x <= 233; x += 16) {
			var xPos = (sz.w / (maxX - minX)) * (x - minX);
			svg.appendChild(svgEl('text', {
				x: xPos,
				y: sz.h,
				'class': 'svg-text',
				'pointer-events': 'none',
				'text-anchor': 'middle',
				'dy': '-0.2em'
			})).appendChild(document.createTextNode(x));
		}
	}
/* TOMATO64-END */
	/* draw y-axis */
	for (var i = 0; i < 10; i++) {
		var y = -10 * i;

		if (y !== -10 && y >= -90) {
			var yPos = (sz.h / (vdiv * 10)) * (-y - 10);
			svg.appendChild(svgEl('text', {
				x: 1,
				y: yPos,
				'class': 'svg-text',
				'pointer-events': 'none',
				'text-anchor': 'start',
				'dominant-baseline': 'middle'
			})).appendChild(document.createTextNode(y.toString()));
			svg.appendChild(svgEl('text', {
				x: sz.w - 1,
				y: yPos,
				'class': 'svg-text',
				'pointer-events': 'none',
				'text-anchor': 'end',
				'dominant-baseline': 'middle'
			})).appendChild(document.createTextNode(y.toString()));
		}
	}
}

function drawBoard(can) {
	var p = 0;
	var svg = E(can);
	var sz = svgSize(svg);
	var frag = document.createDocumentFragment();
	var w = sz.w;
	var h = sz.h;
	var shade = 'rgba(112,66,20,0.06)';

	if (can == 'ellipses2') {
		for (var x = 0; x <= hsize; x += (w / div24)) {
			frag.appendChild(svgEl('line', {
				x1: 0.5 + x + p, y1: p,
				x2: 0.5 + x + p, y2: vsize + p,
				'class': 'svg-line a'
			}));
		}
		for (var x = 0; x <= vsize; x += (h / vdiv)) {
			frag.appendChild(svgEl('line', {
				x1: p, y1: 0.5 + x + p,
				x2: hsize + p, y2: 0.5 + x + p,
				'class': 'svg-line a'
			}));
		}
		frag.appendChild(svgEl('rect', { x: 0, y: 0, width: (w / div24) * 3, height: vsize, fill: shade }));
		frag.appendChild(svgEl('rect', { x: (w / div24) * 15, y: 0, width: (w / div24) * 11, height: vsize, fill: shade }));
		frag.appendChild(svgEl('rect', { x: (w / div24) * 13, y: 0, width: (w / div24) * 15, height: vsize, fill: shade }));
	}
	else if (can == 'ellipses5') {
		for (var x = 0; x <= hsize; x += (w / div5)) {
			frag.appendChild(svgEl('line', {
				x1: 0.5 + x + p, y1: p,
				x2: 0.5 + x + p, y2: vsize + p,
				'class': 'svg-line a'
			}));
		}
		for (var x = 0; x <= vsize; x += (h / vdiv)) {
			frag.appendChild(svgEl('line', {
				x1: p, y1: 0.5 + x + p,
				x2: hsize + p, y2: 0.5 + x + p,
				'class': 'svg-line a'
			}));
		}
		frag.appendChild(svgEl('rect', { x: 0, y: 0, width: (w / div5) * 2, height: vsize, fill: shade }));
		frag.appendChild(svgEl('rect', { x: (w / div5) * 9, y: 0, width: (w / div5) * 9, height: vsize, fill: shade }));
		frag.appendChild(svgEl('rect', { x: (w / div5) * 37, y: 0, width: w, height: vsize, fill: shade }));
		frag.appendChild(svgEl('rect', { x: (w / div5) * 29.3, y: 0, width: (w / div5) * 0.6, height: vsize, fill: shade }));
	}
/* TOMATO64-BEGIN */
	else if (can == 'ellipses6') {
		for (var x = 0; x <= hsize; x += (w / div6)) {
			frag.appendChild(svgEl('line', {
				x1: 0.5 + x + p, y1: p,
				x2: 0.5 + x + p, y2: vsize + p,
				'class': 'svg-line a'
			}));
		}
		for (var x = 0; x <= vsize; x += (h / vdiv)) {
			frag.appendChild(svgEl('line', {
				x1: p, y1: 0.5 + x + p,
				x2: hsize + p, y2: 0.5 + x + p,
				'class': 'svg-line a'
			}));
		}
	}
/* TOMATO64-END */

	svg.appendChild(frag);
}

function clearCanvas(canvas) {
	var svg = E(canvas);
	while (svg.firstChild)
		svg.removeChild(svg.firstChild);

	var sz = svgSize(svg);
	if (!svg.getAttribute('viewBox'))
		svg.setAttribute('viewBox', '0 0 '+sz.w+' '+sz.h);

	svg.appendChild(svgEl('rect', { x: 0, y: 0, width: sz.w, height: sz.h, 'class': 'svg-rect' }));
}

function earlyInit() {
	if (!useAjax())
		E('expire-time').style.display = 'none';

	sg.setup();

	addEvent(E('expire-time'), 'change', function() { cookie.set(cprefix+'_expire_time', E('expire-time').selectedIndex); } );
	addEvent(E('fill-style'), 'change', function() { cookie.set(cprefix+'_fill_style', E('fill-style').selectedIndex); } );
	addEvent(E('ssid-show'), 'change', function() { cookie.set(cprefix+'_ssid_show', E('ssid-show').selectedIndex); } );
	addEvent(E('ssid-limit'), 'change', function() { cookie.set(cprefix+'_ssid_limit', E('ssid-limit').value); } );
	insOvl();
}

function init() {
	restoreVisibility(cprefix, 'notes');

	E('expire-time').selectedIndex = cookie.get(cprefix+'_expire_time') || 5;
	E('ssid-limit').value = cookie.get(cprefix+'_ssid_limit') || 20;
	E('fill-style').selectedIndex = cookie.get(cprefix+'_fill_style') || 7;
	E('ssid-show').selectedIndex = cookie.get(cprefix+'_ssid_show') || 4;

	updateFreqFilterOptions(true);

/* TOMATO64-REMOVE-BEGIN */
	var ellipses2Div = E('tomato-chart2');
	var ellipses5Div = E('tomato-chart5');
	if (Number(wl0.radio.value) === 1 && wl0.mode.value !== '')
		ellipses2Div.style.display = 'block';
	else
		ellipses2Div.style.display = 'none';

	if ((Number(wl1.radio.value) === 1 && wl1.mode.value !== '') || (Number(wl2.radio.value) === 1 && wl2.mode.value !== ''))
		ellipses5Div.style.display = 'block';
	else
		ellipses5Div.style.display = 'none';
/* TOMATO64-REMOVE-END */
/* TOMATO64-BEGIN */
	showChartsFor(E('freq-filter').value);
/* TOMATO64-END */
/* ADVTHEMES-BEGIN */
	if (nvram.web_adv_scripts == 1) {
		resize_graph('2');
		resize_graph('5');
/* TOMATO64-BEGIN */
		resize_graph('6');
/* TOMATO64-END */
	}
/* ADVTHEMES-END */
	sg.recolor();

	renderNoData();

	ref.initPage(0, 15);
	if (!ref.running)
		ref.once = 1;

	ref.start();

	/* fast-poll every 1 second until we have results for all enabled bands */
	var pollCount = 0;
	var fastPoll = setInterval(function() {
		pollCount++;
/* TOMATO64-REMOVE-BEGIN */
		var has24 = (Number(wl0.radio.value) === 1);
		var has5 = ((Number(wl1.radio.value) === 1) || (Number(wl2.radio.value) === 1));
		var got24 = false, got5 = false;

		for (var i = 0; i < wlscandata.length; i++) {
			if (wlscandata[i][9] == '2.4') got24 = true;
			if (wlscandata[i][9] == '5') got5 = true;
		}

		var ready = (!has24 || got24) && (!has5 || got5);
/* TOMATO64-REMOVE-END */
/* TOMATO64-BEGIN */
		var wanted = wlBandsPresent();
		var ready = true;

		for (var b = 0; b < wanted.length; b++)
			if (!wlHasBand(wanted[b]))
				ready = false;
/* TOMATO64-END */
		if (ready || pollCount > 10)
			clearInterval(fastPoll);
		else
			ref.refresh('');
	}, 1000);
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

<div class="section-title">Charts</div>
<div class="section">
<!-- TOMATO64-BEGIN -->
	<div class="note-warning" id="survey-note"></div>
<!-- TOMATO64-END -->
	<div id="tomato-chart2">Channel Congestion: <b>2.4 GHz</b><br>
		<script>
			W('<div id="graph2" style="position:relative"><div id="loading2" style="position:absolute;left:0;top:0;right:0;bottom:0;display:flex;align-items:center;justify-content:center;pointer-events:none">Loading... <img src="spin.svg" alt="" style="transform:scale(3);filter:opacity(0.2);padding-left: 20px"><\/div><svg id="ellipses2" width="'+hsize+'" height="'+vsize+'" viewBox="0 0 '+hsize+' '+vsize+'" xmlns="'+SVG_NS+'" style="border:0px"><\/svg><\/div>');
		</script>
	</div>
	<br>
	<div id="tomato-chart5">Channel Congestion: <b>5 GHz</b><br>
		<script>
			W('<div id="graph5" style="position:relative"><div id="loading5" style="position:absolute;left:0;top:0;right:0;bottom:0;display:flex;align-items:center;justify-content:center;pointer-events:none">Loading... <img src="spin.svg" alt="" style="transform:scale(3);filter:opacity(0.2);padding-left: 20px;"><\/div><svg id="ellipses5" width="'+hsize+'" height="'+vsize+'" viewBox="0 0 '+hsize+' '+vsize+'" xmlns="'+SVG_NS+'" style="border:0px"><\/svg><\/div>');
		</script>
	</div>
	<br>
<!-- TOMATO64-BEGIN -->
	<div id="tomato-chart6">Channel Congestion: <b>6 GHz</b><br>
		<script>
			W('<div id="graph6" style="position:relative"><div id="loading6" style="position:absolute;left:0;top:0;right:0;bottom:0;display:flex;align-items:center;justify-content:center;pointer-events:none">Loading... <img src="spin.svg" alt="" style="transform:scale(3);filter:opacity(0.2);padding-left: 20px;"><\/div><svg id="ellipses6" width="'+hsize+'" height="'+vsize+'" viewBox="0 0 '+hsize+' '+vsize+'" xmlns="'+SVG_NS+'" style="border:0px"><\/svg><\/div>');
		</script>
	</div>
	<br>
<!-- TOMATO64-END -->
	<div id="wl-controls">
		<table style="border:none"><tr><td>
			<label for="freq-filter">Display: </label>
				<select id="freq-filter" onchange="doit();">
					<option value="2.4">2.4 GHz</option>
					<option value="5">5 GHz</option>
<!-- TOMATO64-BEGIN -->
					<option value="6">6 GHz</option>
<!-- TOMATO64-END -->
<!-- TOMATO64-REMOVE-BEGIN -->
					<option value="0" selected>2.4 & 5 GHz</option>
<!-- TOMATO64-REMOVE-END -->
<!-- TOMATO64-BEGIN -->
					<option value="0" selected>All bands</option>
<!-- TOMATO64-END -->
				</select>&nbsp;&nbsp;&nbsp;&nbsp;
		</td><td>
			<label for="fill-style">Style: </label>
				<select id="fill-style" onchange="doit();">
					<option value="100">100%</option>
					<option value="90">90%</option>
					<option value="80">80%</option>
					<option value="70">70%</option>
					<option value="60">60%</option>
					<option value="50">50%</option>
					<option value="40">40%</option>
					<option value="30">30%</option>
					<option value="20">20%</option>
					<option value="10">10%</option>
					<option value="5">5%</option>
					<option value="0">0%</option>
				</select>&nbsp;&nbsp;&nbsp;&nbsp;
		</td><td>
			<label for="ssid-show">SSID text: </label>
				<select id="ssid-show" onchange="doit();">
					<option value="20">20px</option>
					<option value="18">18px</option>
					<option value="16">16px</option>
					<option value="14">14px</option>
					<option value="12">12px</option>
					<option value="10">10px</option>
					<option value="8">8px</option>
					<option value="0">Hide</option>
				</select>&nbsp;&nbsp;&nbsp;&nbsp;
		</td><td>
			<label for="ssid-limit">Limit SSIDs: </label>
			<input type="number" id="ssid-limit" min="0" max="40" value="20" onchange="doit();">&nbsp;&nbsp;&nbsp;&nbsp;
		</td>
		</td><td>
			<input type=button id="shuffle" value="🔀 Shuffle" onclick='recolor()'>
		</td></tr>
		</table>
	</div>
</div>

<!-- / / / -->

<div class="section-title">Wireless Site Survey</div>
<div class="section">
	<div id="survey-grid" class="tomato-grid"></div>
	<div id="survey-msg"></div>
</div>

<!-- / / / -->

<script>writeToggleSectionTitle('Notes', 'notes');</script>
<div class="section" id="sesdiv_notes" style="display:none">
	<ul>
<!-- TOMATO64-REMOVE-BEGIN -->
		<script>
			if ('<% wlclient(); %>' == '0')
				W('<li><b>Warning:<\/b> Wireless connections to this router may be disrupted while using this tool.<br><\/li>');
		</script>
<!-- TOMATO64-REMOVE-END -->
<!-- TOMATO64-BEGIN -->
		<li><b>Warning:</b> Wireless connections to this router may be disrupted while using this tool.</li>
<!-- TOMATO64-END -->
<!-- TOMATO64-REMOVE-BEGIN -->
<!-- BCMARM-BEGIN -->
		<li><b>Wireless Survey:</b> will not show any results with WL filter turned on in 'permit only' mode. </li>
<!-- BCMARM-END -->
<!-- TOMATO64-REMOVE-END -->
		<li><b>Internal SSID:</b> a full page reload is needed if any of the internal SSID parameters (grey ellipse) ever change.</li>
<!-- TOMATO64-REMOVE-BEGIN -->
		<li><b>Protocols:</b> 802.11ac based FT routers may not accurately detect 160, 240, or 320 MHz channel widths used by 802.11ax (WiFi 6) and 802.11be (WiFi 7) routers. Conversely, 802.11ax-based FT routers may misinterpret 240 or 320 MHz widths in 802.11be routers as 160 MHz. Consider these limitations when assessing network configurations for compatibility and performance.</li>
<!-- TOMATO64-REMOVE-END -->
<!-- TOMATO64-BEGIN -->
		<li><b>Scan duration:</b> a survey takes several seconds per radio and briefly interrupts traffic on it. Results are cached for a few seconds, so refreshing faster than that re-shows the previous scan.</li>
		<li><b>6 GHz:</b> a 6 GHz radio can only find neighbours that advertise themselves on the preferred scanning channels or through a co-located 2.4/5 GHz AP, so this band normally reports far fewer networks than it can actually reach.</li>
<!-- TOMATO64-END -->
	</ul>
</div>

<!-- / / / -->

<div id="footer">
	<div id="survey-controls">
		Timeout:
		<img src="spin.svg" alt="" id="refresh-spinner">
		<script>
			genStdTimeList('expire-time', 'Never = ♾️', 10);
			genStdTimeList('refresh-time', 'One off', 10);
		</script>
		<input type="button" value="Refresh" onclick="ref.toggle()" id="refresh-button">
	</div>
</div>

</td></tr>
</table>
</form>
<script>earlyInit();</script>
</body>
</html>
