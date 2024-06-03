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
<title>[<% ident(); %>] Tools: Wireless Survey</title>
<link rel="stylesheet" type="text/css" href="tomato.css">
<% css(); %>
<script src="tomato.js"></script>

<script>

//	<% nvram("wl_radio,wl_nband,wl_mode,wl_closed,wl_ifname,web_css,web_adv_scripts"); %>


var cprefix = 'tools_wlsurvey';
var div24 = 21; /* 2.4GHz horizontal divisions */
var div5 = 39; /* 5GHz horizontal divisions */
var vdiv = 9; /* vertical divisions */
var vsize = 200; /* absolute canvas height */
var hsize = 756; /* absolute canvas width */
/* ADVTHEMES-BEGIN */
if (nvram.web_css.match(/at-/g)) {
	vsize = 450;
	hsize = 1200;
}
/* ADVTHEMES-END */
function internalWiFI(interface) {
	return new Promise(function(resolve, reject) {
		var cmd = new XmlHttp();
		cmd.onCompleted = function(text, xml) {
			var result = text.replace(/\x0a/g, ' ');
			resolve(result);
		};
		cmd.onError = function(x) {
			var error = 'ERROR: '+x.replace(/\x0a/g, ' ');
			reject(error);
		};
		var c = '/usr/sbin/wl -i '+interface+' status';
		cmd.post('shell.cgi', 'action=execute&command='+encodeURIComponent(c.replace(/\r/g, '')));
	});
}

var res0a, res1a, res2a;
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

var res0 = internalWiFI(wl0.ifname.value);
var res1 = internalWiFI(wl1.ifname.value);
var res2 = internalWiFI(wl2.ifname.value);
res0.then(function(res) {
	res0a = res.replace(/\\x([0-9A-Fa-f]{2})/g, (match, p1) => String.fromCharCode(parseInt(p1, 16)));
}).catch(function(error) {
	console.error('Error fetching result for eth1:', error);
});
res1.then(function(res) {
	res1a = res.replace(/\\x([0-9A-Fa-f]{2})/g, (match, p1) => String.fromCharCode(parseInt(p1, 16)));
}).catch(function(error) {
	console.error('Error fetching result for eth2:', error);
});
res2.then(function(res) {
	res2a = res.replace(/\\x([0-9A-Fa-f]{2})/g, (match, p1) => String.fromCharCode(parseInt(p1, 16)));
}).catch(function(error) {
	console.error('Error fetching result for eth3:', error);
});

if (parseInt(nvram.wl0_nband) === 1) {
	var temp = wl0;
	wl0 = wl1;
	wl1 = temp;
}
/* ADVTHEMES-BEGIN */
function resize_graph(id) {
	var targetNode = E('content');
	if (targetNode && targetNode.tagName === 'TD') {
		targetNode.classList.add('dynamic');
	}

	var graph = E('graph'+id);
	var dest = E('ellipses'+id);

	const observer = new ResizeObserver(entries => {
		hsize = graph.clientWidth - 10;
		vsize = parseInt(hsize * 0.35);

		dest.setAttribute("width", hsize);
		dest.setAttribute("height", vsize);
	});
	observer.observe(graph);
}
/* ADVTHEMES-END */
function hexToDecimal(hexColor) {
	hexColor = hexColor.replace('#', '');
	var red = parseInt(hexColor.substring(0, 2), 16);
	var green = parseInt(hexColor.substring(2, 4), 16);
	var blue = parseInt(hexColor.substring(4, 6), 16);
	return red+', '+green+', '+blue;
}

function redraw() {
	clearCanvas('ellipses2');
	clearCanvas('ellipses5');
	drawBoard('ellipses2');
	drawBoard('ellipses5');
}

function recolor() {
	colors = colors.sort(() => Math.random() - 0.5);
	redraw();
	doit();
}

function doit() {
	fillstyle = E('fill-style').value;
	ssidshow = E('ssid-show').value;
	ssidlimit = E('ssid-limit').value;
	if (ssidlimit > 40)
		ssidlimit = 40;

	sg.removeAllData();
	sg.populate(fillstyle, ssidshow);
	sg.resort();
	drawNoise('ellipses2', fillstyle);
	drawNoise('ellipses5', fillstyle);
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
var entries = [];
var dayOfWeek = ['Sun','Mon','Tue','Wed','Thu','Fri','Sat'];
var cmd = null;
var ref = new TomatoRefresh('update.cgi', 'exec=wlscan', 0, 'tools_survey_refresh');

ref.refresh = function(text) {
	try {
		eval(text);
	}
	catch (ex) {
	}
	doit();
}
var sg = new TomatoGrid();

sg.setup = function() {
	this.init('survey-grid','sort');
	this.headerSet(['Last Seen','RGB','SSID','BSSID','RSSI<br>dBm','SNR<br>dB','Qual','Ctrl/Centr<br>Channel','Security','802.11']);
	this.populate();
	this.sort(4);
}

function drawNoise(board, style) {
	var noise1, noise2, noise;
	var canvas = E(board);
	var ctx = canvas.getContext('2d');
	if (wl0.radio.value == 1 && board == 'ellipses2') {
		if (res0a !== null) noise = ((-wl0.noise - 10) * (canvas.height / 100) * (10 / vdiv));
		var noiseV = Number(wl0.noise);
	}
	if (wl1.radio.value == 1 && board == 'ellipses5') {
		res1 = String(res1).replace(/\\x0a/g, ',').match(/\d+/g);
		if (res1a !== null) noise = ((-wl1.noise - 10) * (canvas.height / 100) * (10 / vdiv));
		var noiseV = Number(wl1.noise);
	}
	if (wl2.radio.value == 1 && board == 'ellipses5') {
		res2 = String(res2).replace(/\\x0a/g, ',').match(/\d+/g);
		if (res2a !== null) noise = ((-wl2.noise - 10) * (canvas.height / 100) * (10 / vdiv));
		var noiseV = Number(wl2.noise);
	}

	if (typeof noise1 !== 'undefined' && typeof noise2 !== 'undefined')
		noise = Math.max(noise1, noise2);

	ctx.beginPath();
	ctx.save();
	if (noise != 0) {
		ctx.lineWidth = 0.2;
		ctx.moveTo(0, noise + 1);
		ctx.lineTo(canvas.width, noise + 1);
		ctx.strokeStyle = 'rgba(80,0,0,0.5)';
		var density = (style / 100);
		ctx.fillStyle = 'rgba(189,158,0'+','+density / 3+')';
		ctx.stroke();
		ctx.fillRect(0, noise, canvas.width, noise);
		ctx.fillStyle = '#660000';
		ctx.textBaseline = 'center';
		ctx.textAlign = 'right';
		ctx.fillText('noise '+noiseV, (canvas.width - 14), noise);
		ctx.fill();
	}
	ctx.restore();
	ctx.closePath();
}

function drawEllipse(c = -100, m = 20, q, col, ssid, noise, style, sshow) {
	if (c < 35) {
		var canvas = E('ellipses2');
		var ctx = canvas.getContext('2d');
		if (m == 20)
			mf = (canvas.width / div24) * 2.20;
		else if (m == 40)
			mf = (canvas.width / div24) * 4.2;

		if (c == 1)
			cf = (canvas.width / div24) * 3;
		else if (c == 14)
			cf = (canvas.width / div24) * 18;
		else
			cf = ((canvas.width / div24) * 3) + ((canvas.width / div24) * (c - 1));

		rf = q * (canvas.height / 100) * (10 / vdiv);
	}
	else if (c > 35) {
		var canvas  = E('ellipses5');
		var ctx = canvas.getContext('2d');
		var cc = c;
		if (m == 20)
			mf = ((canvas.width / div5) * 0.5);
		else if (m == 40) {
			mf = ((canvas.width / div5) * 1);
			cc = c;
		}
		else if (m == 80) {
			mf = ((canvas.width / div5) * 2);
			cc = c;
		}
		cc = cc - 36 + 4;
		cf = (canvas.width / div5) + ((canvas.width / div5) * (cc / 4));
		rf = q * (canvas.height / 100) * (10 / vdiv); /* adapt calculation for -10 to -100 only */
	}
	ctx.beginPath();
	ctx.save();
	var decimalColor = hexToDecimal(col);
	ctx.ellipse(cf, vsize, mf, rf, 0, 0, 2 * Math.PI, false);
	ctx.strokeStyle = 'rgba('+decimalColor+', 1)';
	if (style == '0') {
		ctx.fillStyle = 'rgba('+decimalColor+', 0)';
		ctx.lineWidth = 1;
	}
	else {
		var density = (style / 100);
		ctx.fillStyle = 'rgba('+decimalColor+', '+density+')';
	}
	ctx.stroke();
	ctx.fill();
	ctx.restore();
	ctx.save();
	ctx.fillStyle = '#000000';
	ctx.font = sshow+'px Arial';
	ctx.textAlign = 'center';
	ctx.textBaseline = 'middle';
	var lines = ssid.split('¬');
	ctx.fillText(lines[0], cf, (vsize + 1 + sshow / 2) - rf);
	if (typeof lines[1] !== 'undefined' ) {
		ctx.fillText(lines[1], cf, (vsize + 1 + sshow * 1.5) - rf);
		ctx.fillText(lines[2], cf, (vsize + 1 + sshow * 2.5) - rf);
	}
	ctx.restore();
	ctx.closePath();
}

sg.populate = function(style, sshow) {
	var added = 0;
	var removed = 0;
	var i, j, k, t, e, s;

	redraw();

	var lim2 = 1;
	var lim5 = 1;
	var col2 = 0;
	var col5 = colors.length - 1;

	if ((wlscandata.length == 1) && (!wlscandata[0][0])) {
		setMsg('error: '+wlscandata[0][1]);
		return;
	}

	drawFT();
	wlscandata.sort((b, a) => a[2] - b[2]);

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
		if (s[1] === '')
			e.ssid = '🕶️';
		else
			e.ssid = s[1];

		e.control = s[3];
		e.channel = s[10];
		e.channel = e.channel+'<br><small>'+s[9]+' GHz<\/small><br><small>'+s[4]+' MHz<\/small>';
		e.rssi = s[2];
		if (s[9] == 2.4)
			e.snr = Number(e.rssi) + Math.abs(wl0.noise)
		else
			e.snr = Number(e.rssi) + Math.abs(wl1.noise)
		e.mhz = s[4];
		e.cap = s[7]+ '<br>'+s[8];
		e.rates =s[6].replace('11', '');;
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
		if (lim2 <= ssidlimit && chan[0] < 35 && col2 <= colors.length - 1) {
			if (e.bssid === wl0.bssid)
				colo = '#aaaaaa';
			else {
				colo = colors[col2];
				col2++;
			}
			lim2++;
			var decimalColor = hexToDecimal(colo);
			e.col = '<div style="margin:0 auto;display:block;padding:0px;width:20px;height:40px;background-color:rgba('+decimalColor+','+density+');border:1px solid black;"><\/div>';
			drawEllipse(chan[0], e.mhz, (100 + e.rssi), colo, e.ssid, 0, style, sshow);
		}
		else if (lim5 <= ssidlimit && chan[0] > 35) {
			if ((e.bssid === wl1.bssid) || (e.bssid === wl2.bssid))
				colo = '#aaaaaa';
			else {
				colo = colors[col5];
				col5--;
			}
			lim5++;
			var decimalColor = hexToDecimal(colo);
			e.col = '<div style="margin:0 auto;display:block;padding:0px;width:20px;height:40px;background-color:rgba('+decimalColor+','+density+');border:1px solid black;"><\/div>';
			drawEllipse(chan[0], e.mhz, (100 + e.rssi), colo, e.ssid, 0, style, sshow);
		}
		else {
			e.col = '<div><\/div>';
		}
		if (mac.match(/^(..):(..):(..)/))
			mac = '<div style="display:none" id="gW_'+i+'">&nbsp; <img src="spin.gif" alt="" style="vertical-align:middle"><\/div><a href="javascript:searchOUI(\''+RegExp.$1+'-'+RegExp.$2+'-'+RegExp.$3+'\','+i+')" title="OUI Search">'+mac+'<\/a>';

		sg.insert(-1, e, [ '<small>'+seen+'<\/small>', ''+e.col, ''+e.ssid, mac, (e.rssi < 0 ? e.rssi+'' : ''), ''+e.snr,
		          (e.qual < 0 ? '' : '<small>'+e.qual+'<\/small><br><img src="bar'+MIN(MAX(Math.floor(e.qual / 12), 1), 6)+'.gif" id="bar_'+i+'" alt="">'),
		          ''+e.control+'/'+e.channel, '<small>'+e.cap, '<\/small>'+e.rates], false);
	}
	var max2 = div24 - 2 ;
	drawCoordinates('ellipses2', -2, max2, 1, 13, div24, 1); /* min, max, display min, display max, total num, increment */
	drawCoordinates('ellipses5', 28, 184, 36, 180, div5, 4);
	s = '';
	if (useAjax())
		s = added+' added, '+removed+' removed, ';

	s += entries.length+' total.';
	s += '<br><br><small>Last updated: '+(new Date()).toWHMS()+'<\/small>';
	setMsg(s);

	wlscandata = [];
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
	var noiseMatch, ssidMatch, bssidMatch, rssiMatch, channelMatch, chanspecMatch;

	if (typeof res0a === 'undefined')
		return;

	noiseMatch = res0a.match(/noise: (-?\d+)/);
	wl0.noise = noiseMatch ? noiseMatch[1] : null;
	ssidMatch = res0a.match(/SSID: "([^"]+)"/);
	wl0.ssid = ssidMatch ? ssidMatch[1] : null;
	bssidMatch = res0a.match(/BSSID: (\S+)/);
	wl0.bssid = bssidMatch ? bssidMatch[1] : null;
	if (wl0.mode.value === 'ap')
		wl0.rssi = -10;
	else {
		rssiMatch = res0a.match(/RSSI:\s*(-?\d+)\s*dBm/);
		wl0.rssi = rssiMatch ? parseInt(rssiMatch[1], 10) : null;
	}
	channelMatch = res0a.match(/Primary channel: (\d+)/);
	wl0.controlchannel = channelMatch ? channelMatch[1] : null;
	chanspecMatch = res0a.match(/Chanspec: (\d+(?:\.\d+)?)GHz channel (\d+) (\d+)MHz/);
	wl0.centralchannel = chanspecMatch ? chanspecMatch[2] : null;
	wl0.width = chanspecMatch ? chanspecMatch[3] : null;
	noiseMatch = res1a.match(/noise: (-?\d+)/);
	wl1.noise = noiseMatch ? noiseMatch[1] : null;
	ssidMatch = res1a.match(/SSID: "([^"]+)"/);
	wl1.ssid = ssidMatch ? ssidMatch[1] : null;
	bssidMatch = res1a.match(/BSSID: (\S+)/);
	wl1.bssid = bssidMatch ? bssidMatch[1] : null;
	if (wl1.mode.value === 'ap')
		wl1.rssi = -10;
	else {
		rssiMatch = res1a.match(/RSSI:\s*(-?\d+)\s*dBm/);
		wl1.rssi = rssiMatch ? parseInt(rssiMatch[1], 10) : null;
	}
	channelMatch = res1a.match(/Primary channel: (\d+)/);
	wl1.controlchannel = channelMatch ? channelMatch[1] : null;
	chanspecMatch = res1a.match(/Chanspec: (\d+(?:\.\d+)?)GHz channel (\d+) (\d+)MHz/);
	wl1.centralchannel = chanspecMatch ? chanspecMatch[2] : null;
	wl1.width = chanspecMatch ? chanspecMatch[3] : null;
	noiseMatch = res2a.match(/noise: (-?\d+)/);
	wl2.noise = noiseMatch ? noiseMatch[1] : null;
	ssidMatch = res2a.match(/SSID: "([^"]+)"/);
	wl2.ssid = ssidMatch ? ssidMatch[1] : null;
	bssidMatch = res2a.match(/BSSID: (\S+)/);
	wl2.bssid = bssidMatch ? bssidMatch[1] : null;
	wl2.bssid = bssidMatch ? bssidMatch[1] : null;
	if (wl2.mode.value === 'ap')
		wl2.rssi = -10;
	else {
		rssiMatch = res2a.match(/RSSI:\s*(-?\d+)\s*dBm/);
		wl2.rssi = rssiMatch ? parseInt(rssiMatch[1], 10) : null;
	}
	channelMatch = res2a.match(/Primary channel: (\d+)/);
	wl2.controlchannel = channelMatch ? channelMatch[1] : null;
	chanspecMatch = res2a.match(/Chanspec: (\d+(?:\.\d+)?)GHz channel (\d+) (\d+)MHz/);
	wl2.centralchannel = chanspecMatch ? chanspecMatch[2] : null;
	wl2.width = chanspecMatch ? chanspecMatch[3] : null;
	if (Number(wl0.radio.value) === 1) {
		var internalWl0 = [ wl0.bssid, wl0.ssid, wl0.rssi, wl0.controlchannel, wl0.width, 100, '', '', '', '2.4', wl0.centralchannel, 0 ];
		if (wlscandata.find(obj => obj.bssid === wl0.bssid.value) == -1)
			wlscandata.push(internalWl0);
	}
	if (Number(wl1.radio.value) === 1) {
		var internalWl1 = [ wl1.bssid, wl1.ssid, wl1.rssi, wl1.controlchannel, wl1.width, 100, '', '', '', '5', wl1.centralchannel, 0 ];
		if (wlscandata.find(obj => obj.bssid === wl1.bssid.value) == -1)
			wlscandata.push(internalWl1);
	}
	if (Number(wl2.radio.value) === 1) {
		var internalWl2 = [ wl2.bssid, wl2.ssid, wl2.rssi, wl2.controlchannel, wl2.width, 100, '', '', '', '5', wl2.centralchannel, 0 ];
		if (wlscandata.find(obj => obj.bssid === wl2.bssid.value) == -1)
			wlscandata.push(internalWl2);
	}
}

function drawCoordinates(a, b, c, d, e, f, g) {
	var canvas = E(a);
	var ctx = canvas.getContext('2d');
	var minX = b;
	var maxX = c;
	var minV = d;
	var maxV = e;
	var numDivisions = f;
	var incrementX = g;
	var fontSize = 10;
	var fontFamily = 'Arial Narrow';
	ctx.font = fontSize+'px '+fontFamily;
	/* draw x-axis */
	ctx.beginPath();
	ctx.moveTo(0, canvas.height);
	ctx.lineTo(canvas.width, canvas.height);
	ctx.fillStyle = 'black';
	ctx.textBaseline = 'bottom';
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
				var xPos = (canvas.width / (maxX - minX)) * (x - minX);
				ctx.textAlign = 'center';
				ctx.fillText(x, xPos, canvas.height + 2);
			}
		}
		ctx.textAlign = 'center';
		xPos = (canvas.width / (maxX - minX)) * 18
		ctx.fillText(14, xPos, canvas.height + 2);
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
				var xPos = (canvas.width / (maxX - minX)) * (x - minX);
				ctx.textAlign = 'center';
				ctx.fillText(x, xPos, canvas.height + 2);
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
				var xPos = (canvas.width / (184 - 33)) * (x - 33);
				ctx.textAlign = 'center';
				ctx.fillText(x, xPos, canvas.height + 2);
			}
		}
	}
	/* draw y-axis */
	for (var i = 0; i < 10; i++) {
		var y = -10 * i;

		if (y !== -10 && y >= -90) {
			var yPos = (canvas.height / (vdiv * 10)) * (-y - 10);

			ctx.textBaseline = 'middle';
			ctx.fillText(y.toString(), 7, yPos);
			ctx.fillText(y.toString(), hsize - 7, yPos);
		}
	}
}

function drawBoard(can) {
	var p = 0;
	var canvas = E(can);
	var ctx = canvas.getContext('2d');
	ctx.beginPath();
	if (can == 'ellipses2') {
		for (var x = 0; x <= hsize; x += (canvas.width / div24)) {
			ctx.moveTo(0.5 + x + p, p);
			ctx.lineTo(0.5 + x + p, vsize + p);
		}
		for (var x = 0; x <= vsize; x += (canvas.height / vdiv)) {
			ctx.moveTo(p, 0.5 + x + p);
			ctx.lineTo(hsize + p, 0.5 + x + p);
		}
		ctx.strokeStyle = '#f2f2f2';
		ctx.stroke();
		ctx.rect(0, 0, canvas.width / div24 * 3, vsize);
		ctx.rect(canvas.width / div24 * 15, 0, canvas.width / div24 * 11, vsize);
		ctx.fillStyle = 'rgba(112,66,20,0.06)';
		ctx.fill();
		ctx.rect(canvas.width / div24 * 13, 0, canvas.width / div24 * 15, vsize);
		ctx.fillStyle = 'rgba(112,66,20,0.06)';
		ctx.fill();
	}
	else if (can == 'ellipses5') {
		for (var x = 0; x <= hsize; x += (canvas.width / div5)) {
			ctx.moveTo(0.5 + x + p, p);
			ctx.lineTo(0.5 + x + p, vsize + p);
		}
		for (var x = 0; x <= vsize; x += (canvas.height / vdiv)) {
			ctx.moveTo(p, 0.5 + x + p);
			ctx.lineTo(hsize + p, 0.5 + x + p);
		}
		ctx.strokeStyle = '#f4f4f4';
		ctx.stroke();
		ctx.rect(0, 0, (canvas.width / div5) * 2, vsize);
		ctx.rect(canvas.width / div5 * 9, 0, canvas.width / div5 * 9, vsize);
		ctx.rect(canvas.width / div5 * 37, 0, canvas.width, vsize);
		ctx.fillStyle = 'rgba(112,66,20,0.06)';
		ctx.fill();
		ctx.rect(canvas.width / div5 * 29.3, 0, canvas.width / div5 * 0.6, vsize);
		ctx.fillStyle = 'rgba(112,66,20,0.06)';
		ctx.fill();
	}
	ctx.closePath();
}

function clearCanvas(canvas) {
	canvas = E(canvas);
	var ctx = canvas.getContext('2d');
	ctx.clearRect(0, 0, canvas.width, canvas.height);
	ctx.fillStyle = '#fdfdfd';
	ctx.fillRect(0, 0, canvas.width, canvas.height);
}

function earlyInit() {
	if (!useAjax())
		E('expire-time').style.display = 'none';

	sg.setup();

	addEvent(E('expire-time'), 'change', function() { cookie.set(cprefix+'_expire_time', E('expire-time').selectedIndex); } );
	addEvent(E('fill-style'), 'change', function() { cookie.set(cprefix+'_fill_style', E('fill-style').selectedIndex); } );
	addEvent(E('ssid-show'), 'change', function() { cookie.set(cprefix+'_ssid_show', E('ssid-show').selectedIndex); } );
	addEvent(E('ssid-limit'), 'change', function() { cookie.set(cprefix+'_ssid_limit', E('ssid-limit').value); } );
}

function init() {
	var c;
	if (((c = cookie.get(cprefix+'_notes_vis')) != null) && (c == '1'))
		toggleVisibility(cprefix, 'notes');

	E('expire-time').selectedIndex = cookie.get(cprefix+'_expire_time') || 9;
	E('ssid-limit').value = cookie.get(cprefix+'_ssid_limit') || 20;
	E('fill-style').selectedIndex = cookie.get(cprefix+'_fill_style') || 7;
	E('ssid-show').selectedIndex = cookie.get(cprefix+'_ssid_show') || 4;

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
/* ADVTHEMES-BEGIN */
	if (nvram.web_adv_scripts == 1) {
		resize_graph('2');
		resize_graph('5');
	}
/* ADVTHEMES-END */
	sg.recolor();

	ref.initPage(0, 1);
	if (!ref.running)
		ref.once = 1;

	ref.start();
}
</script>
</head>
<body onload="init()">
<form action="javascript:{}">
<table id="container">
<tr><td colspan="2" id="header">
	<div class="title">Tomato64</div>
	<div class="version">Version <% version(); %> on <% nv("t_model_name"); %></div>
</td></tr>
<tr id="body"><td id="navi"><script>navi()</script></td>
<td id="content">
<div id="ident"><% ident(); %> | <script>wikiLink();</script></div>

<!-- / / / -->

<div class="section-title">Charts</div>
<div class="section">
	<div id="tomato-chart2">Channel Congestion: <b>2.4 GHz</b><br>
		<script>
			W('<div id="graph2"><canvas id="ellipses2" width="'+hsize+'" height="'+vsize+'" style="border:0px"><\/canvas><\/div>');
		</script>
	</div>
	<br>
	<div id="tomato-chart5">Channel Congestion: <b>5 GHz</b><br>
		<script>
			W('<div id="graph5"><canvas id="ellipses5" width="'+hsize+'" height="'+vsize+'" style="border:0px"><\/canvas><\/div>');
		</script>
	</div>
	<br>
	<div id="wl-controls">
		<table border="0"><tr><td>
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

<div class="section-title">Notes <small><i><a href="javascript:toggleVisibility(cprefix,'notes');"><span id="sesdiv_notes_showhide">(Show)</span></a></i></small></div>
<div class="section" id="sesdiv_notes" style="display:none">
	<ul>
		<script>
			if ('<% wlclient(); %>' == '0')
				document.write('<li><b>Warning:<\/b> Wireless connections to this router may be disrupted while using this tool.<br><\/li>');
		</script>
		<li><b>Wireless Survey:</b> will not show any results with WL filter turned on in 'permit only' mode. </li>
		<li><b>Internal SSID:</b> a full page reload is needed if any of the internal SSID parameters (grey ellipse) ever change.</li>
		<li><b>Protocols:</b> 802.11ac based FT routers may not accurately detect 160, 240, or 320 MHz channel widths used by 802.11ax (WiFi 6) and 802.11be (WiFi 7) routers. Conversely, 802.11ax-based FT routers may misinterpret 240 or 320 MHz widths in 802.11be routers as 160 MHz. Consider these limitations when assessing network configurations for compatibility and performance.</li>
	</ul>
</div>

<!-- / / / -->

<div id="footer">
	<div id="survey-controls">
		Timeout:
		<img src="spin.gif" alt="" id="refresh-spinner">
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
