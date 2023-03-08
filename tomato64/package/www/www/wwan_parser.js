function createWWANTableItem(value, unit, bar) {
	var retVal = '<td class="content">';
	var calculatedMargin = 6; /* dBm */

	if (unit.length == 0) { /* None */
		calculatedMargin = 26;
	} else if (unit.length < 3 && unit.length > 0) { /* dB */
		calculatedMargin = 14;
	}
	retVal += '<span style="width:34px;display:inline-block">'+value+'</span><small style="margin-right:'+calculatedMargin+'px">'+unit+'</small>';
	if (bar) {
		var altText = getAltText(bar);
		retVal += '<img src="'+bar+'" alt="'+altText+'" title="'+altText+'"/>';
	}
	retVal += '</td>';
	return retVal;
}

function getAltText(bar) {
	var altTextMap = { 'bar6.gif':'6/6', 'bar5.gif':'5/6', 'bar4.gif':'4/6', 'bar3.gif':'3/6', 'bar2.gif':'2/6', 'bar1.gif':'1/6' };
	return altTextMap[bar];
}

function createWWANStatusSection(wannum, wwanstatus) {
	var wanNumStr = 'wan'+(wannum > 1 ? wannum : '');
	var code = '<table class="fields"><tbody>';
	code += '<tr><td class="title indent1">Modem type</td>';
	code += '<td class="content">'+nvram[wanNumStr+'_modem_type']+'</td></tr>';
	code += '<tr><td class="title indent1">Current Mode</td>';
	code += '<td class="content">'+wwan_getCurrentMode(wwanstatus)+'</td></tr>';

	var valMap = [];
	wwan_getSignalStrengthMap(wwanstatus, valMap);
	if (valMap['RSSI']) {
		code += '<tr><td class="title indent1">RSSI</td>';
		code += createWWANTableItem(valMap['RSSI'], 'dBm', wwan_getRSSIBar(valMap['RSSI']));
	}
	if (valMap['RSRP']) {
		code += '<tr><td class="title indent1">RSRP</td>';
		code += createWWANTableItem(valMap['RSRP'], 'dBm', wwan_getRSRPBar(valMap['RSRP']));
	}
	if (valMap['RSRQ']) {
		code += '<tr><td class="title indent1">RSRQ</td>';
		code += createWWANTableItem(valMap['RSRQ'], 'dBm', wwan_getRSRQBar(valMap['RSRQ']));
	}
	if (valMap['RSSP']) {
		code += '<tr><td class="title indent1">RSSP</td>';
		code += createWWANTableItem(valMap['RSSP'], 'dB', wwan_getRSSIBar(valMap['RSSP']));
	}
	if (valMap['RSCP']) {
		code += '<tr><td class="title indent1">RSCP</td>';
		code += createWWANTableItem(valMap['RSCP'], 'dBm', wwan_getRSCPBar(valMap['RSCP']));
	}
	if (valMap['SINR']) {
		code += '<tr><td class="title indent1">SINR</td>';
		code += createWWANTableItem(valMap['SINR'], 'dB', wwan_getRSRPBar(valMap['SINR']));
	}
	if (valMap['CQI1']) {
		code += '<tr><td class="title indent1">CQI1</td>';
		code += createWWANTableItem(valMap['CQI1'], '', wwan_getCQIBar(valMap['CQI1']));
	}
	if (valMap['CQI2']) {
		code += '<tr><td class="title indent1">CQI2</td>';
		code += createWWANTableItem(valMap['CQI2'], '', wwan_getCQIBar(valMap['CQI2']));
	}
	if (valMap['ECIO']) {
		code += '<tr><td class="title indent1">ECIO</td>';
		code += createWWANTableItem(valMap['ECIO'], 'dB');
	}
	valMap = [];
	wwan_getLocationMap(wwanstatus, valMap);
	if (valMap['MCC']) {
		code += '<tr><td class="title indent1">Location</td>';
		code += '<td class="content">';
		code += '<span class="wwan-parser">MCC:</span>'+valMap['MCC'];
		code += '<div><span class="wwan-parser">MNC:</span>'+valMap['MNC']+'</div>';
			if (valMap['LAC'])
				code += '<div><span class="wwan-parser">LAC:</span>'+valMap['LAC']['HEX']+' ('+valMap['LAC']['DEC']+')</div>';

			if (valMap['CID'])
				code += '<div><span class="wwan-parser">CID:</span>'+valMap['CID']['HEX']+' ('+valMap['CID']['DEC']+')</div>';

			if (valMap['Cell ID'])
				code += '<div><span class="wwan-parser">Cell ID:</span>'+valMap['Cell ID']['HEX']+' ('+valMap['Cell ID']['DEC']+')</div>';

			if (valMap['PCI'])
				code += '<div><span class="wwan-parser">PCI:</span>'+valMap['PCI']['HEX']+' ('+valMap['PCI']['DEC']+')</div>';

		code += '</td></tr>';
	}

	valMap = wwan_getOperator(wwanstatus);
	if (valMap) {
		if (valMap['OPERATOR']) {
			code += '<tr><td class="title indent1">Current Operator</td>';
			code += '<td class="content">'+valMap['OPERATOR']+'</td></tr>';
		}
	}

	valMap = wwan_getCarrierMap(wwanstatus);
	if (valMap) {
		if (valMap['BBAND']) {
			code += '<tr><td class="title indent1">Current Band</td>';
			code += '<td class="content">'+valMap['BBAND']+' ('+valMap['BBAND_FREQ']+' <small>MHz</small>)</td></tr>';
		}
		if (valMap['DOWN_FREQ']){
			code += '<tr><td class="title indent1">Downlink Frequency</td>';
			code += '<td class="content">'+valMap['DOWN_FREQ']+' <small>MHz</small></td></tr>';
		}
		if (valMap['UP_FREQ']) {
			code += '<tr><td class="title indent1">Uplink Frequency</td>';
			code += '<td class="content">'+valMap['UP_FREQ']+' <small>MHz</small></td></tr>';
		}
		if (valMap['DOWN_BW']) {
			code += '<tr><td class="title indent1">Downlink Bandwidth</td>';
			code += '<td class="content">'+valMap['DOWN_BW']+' <small>MHz</small></td></tr>';
		}
		if (valMap['UP_BW']) {
			code += '<tr><td class="title indent1">Uplink Bandwidth</td>';
			code += '<td class="content">'+valMap['UP_BW']+' <small>MHz</small></td></tr>';
		}
	}

	code += '</tbody>'
	var modemType = nvram[wanNumStr+'_modem_type'];
	var connType = nvram[wanNumStr+'_proto'];
	if (connType == 'ppp3g' || modemType == 'non-hilink' || modemType == 'huawei-non-hilink' || modemType == 'hw-ether') {
		code += '<tr><td class="title indent1"></td>';
		code += '<td class="content wwan-parser-view"><a href="javascript:showSMSForWWAN('+wannum+')">Click to view SMS</a></td></tr>';
	}
	code += '</table>';
	return code;
}

function showSMSForWWAN(wwannum) {
	cookie.set('wwansms_selection', wwannum);
	document.location.href = 'wwan-sms.asp';
}

function wwan_getSignalStrengthMap(buffer, returnMap) {
	var index, itemsToFind = ['RSSI', 'RSRP', 'RSRQ', 'RSSP', 'RSCP', 'SINR', 'CQI1', 'CQI2', 'ECIO'];
	for (index = 0; index < itemsToFind.length; ++index) {
		var element = itemsToFind[index];
		returnMap[element] = extractStringItem(element, buffer);
	}
}

function wwan_getLocationMap(buffer, returnMap) {
	var index, itemsToFind = ['LAC', 'CID', 'PCI', 'Cell ID'];
	for (index = 0; index < itemsToFind.length; ++index) {
		var element = itemsToFind[index];
		returnMap[element] = extractLocationItem(element, buffer);
	}
	var extractedMCCMap = extractMCCMNC(buffer);
	if (extractedMCCMap) {
		returnMap['MCC'] = extractedMCCMap['MCC'];
		returnMap['MNC'] = extractedMCCMap['MNC'];
	}
}

function extractStringItem(tag, buffer) {
	var regExtract = new RegExp(tag+' (.*?)(?:(\\s|\\,|$))', 'gm');
	var matchedArrs = regExtract.exec(buffer);

	if (matchedArrs)
		return matchedArrs[1];

	return undefined;
}

function extractMCCMNC(buffer) {
	var regExtract = new RegExp('MCCMNC (\\d*)(?:\\,?)', 'gm');
	var matchedArrs = regExtract.exec(buffer);
	if (matchedArrs) {
		var returnMap = [];
		var mccmncstring = matchedArrs[1];
		returnMap['MCC'] = mccmncstring.substr(0, mccmncstring.length - 2);
		returnMap['MNC'] = mccmncstring.substr(-2);
		return returnMap;
	}
	return undefined;
}

function extractLocationItem(tag, buffer) {
	var regExtract = new RegExp(tag+' ((.*?))\\((.*?)\\)', 'gm');
	var matchedArrs = regExtract.exec(buffer);
	if (matchedArrs) {
		var returnMap = [];
		returnMap['HEX'] = matchedArrs[1].trim();
		returnMap['DEC'] = matchedArrs[3];
		return returnMap;
	}
	return undefined;
}

function wwan_getCarrierMap(buffer) {
	var regExtract = new RegExp('MODEM Carrier: (.[0-9]) \\((.*) MHz\\)\\, Downlink FQ (.*) MHz, Uplink FQ (.*) MHz, Downlink BW (.*) MHz, Uplink BW (.*) MHz', 'gm');
	var matchedArrs = regExtract.exec(buffer);
	if (matchedArrs) {
		var returnMap = [];
		returnMap['BBAND'] = matchedArrs[1];
		returnMap['BBAND_FREQ'] = matchedArrs[2];
		returnMap['DOWN_FREQ'] = matchedArrs[3];
		returnMap['UP_FREQ'] = matchedArrs[4];
		returnMap['DOWN_BW'] = matchedArrs[5];
		returnMap['UP_BW'] = matchedArrs[6];
		return returnMap;
	}
	return undefined;
}

function wwan_getOperator(buffer) {
	var regExtract = new RegExp('MODEM Current Operator: (.*)', 'gm');

	var matchedArrs = regExtract.exec(buffer);
	if (matchedArrs) {
		var returnMap = [];
		returnMap['OPERATOR'] = matchedArrs[1];
		return returnMap;
	}
	return undefined;
}

function wwan_getRSSIBar(value) {
	if (value.substr(0, 5) == '&gt;=')
		value = parseInt(value.substr(5)); /* special case for hilink modems */

	if (value > -50)
		return 'bar6.gif';
	else if (value <= -50 && value >= -73)
		return 'bar5.gif';
	else if (value <= -75 && value >= -85)
		return 'bar4.gif';
	else if (value <= -87 && value >= -93)
		return 'bar3.gif';
	else
		return 'bar2.gif';
}

function wwan_getRSRPBar(value) {
	if (value > -79)
		return 'bar6.gif';
	else if (value <= -80 && value >= -90)
		return 'bar5.gif';
	else if (value <= -91 && value >= -100)
		return 'bar4.gif';
	else
		return 'bar2.gif';
}

function wwan_getRSRQBar(value) {
	if (value >= -9)
		return 'bar6.gif';
	else if (value <= -10 && value >= -15)
		return 'bar5.gif';
	else if (value <= -16 && value >= -20)
		return 'bar4.gif';
	else
		return 'bar2.gif';
}

function wwan_getSINRBar(value) {
	if (value >= 21)
		return 'bar6.gif';
	else if (value <= 20 && value >= 13)
		return 'bar5.gif';
	else if (value <= 12 && value >= 0)
		return 'bar4.gif';
	else
		return 'bar2.gif';
}

function wwan_getRSCPBar(value) {
	if (value > -65)
		return 'bar6.gif';
	else if (value <= -65 && value > -75)
		return 'bar5.gif';
	else if (value <= -75 && value > -85)
		return 'bar4.gif';
	else if (value <= -85 && value > -95)
		return 'bar3.gif';
	else if (value <= -95 && value > -105)
		return 'bar2.gif';
	else
		return 'bar1.gif';
}

function wwan_getCQIBar(value) {
	if (value >= 14)
		return 'bar6.gif';
	else if (value < 14 && value >= 11)
		return 'bar5.gif';
	else if (value < 11 && value >= 9)
		return 'bar4.gif';
	else if (value < 9 && value >= 7)
		return 'bar3.gif';
	else if (value < 7 && value >= 5)
		return 'bar3.gif';
	else if (value < 5 && value >= 3)
		return 'bar2.gif';
	else
		return 'bar1.gif';
}

function wwan_getCurrentMode(buffer) {
	return extractStringItem('MODEM Current Mode\\:', buffer);
}
