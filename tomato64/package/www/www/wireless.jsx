/*
	Tomato GUI
	http://www.polarcloud.com/tomato/

	For use with Tomato Firmware only.
	No part of this file may be used without permission.
*/

//	<% wlifaces(1); %>
//	<% wlbands(1); %>
//	<% nvram("wl_nband"); %>

function wl_fface(uidx) {
	return wl_ifaces[uidx][1];
}

function wl_unit(uidx) {
	return wl_ifaces[uidx][2];
}

function wl_sunit(uidx) {
	return wl_ifaces[uidx][3];
}

function wl_uidx(unit) {
	for (var u = 0; u < wl_ifaces.length; ++u) {
		if (wl_ifaces[u][2] == unit) return u;
	}
	return -1;
}

function wl_ifidx(ifname) {
	for (var u = 0; u < wl_ifaces.length; ++u) {
		if (wl_ifaces[u][0] == ifname) return u;
	}
	return -1;
}

function wl_ifidxx(ifname) {
	for (var u = 0; u < wl_ifaces.length; ++u) {
		if (wl_ifaces[u][1] == ifname) return u;
	}
	return -1;
}

/* TOMATO64-REMOVE-BEGIN */
function wl_display_ifname(uidx) {
	return wl_ifaces[uidx][0]+(wl_sunit(uidx) < 0 ?
	       ' (wl'+wl_fface(uidx)+')' : '')+((wl_bands[uidx].length == 1) ?
	       ((wl_bands[uidx][0] == '1') ? ' / 5 GHz' : ' / 2.4 GHz') : ((nvram['wl'+wl_unit(uidx)+'_nband'] == 1) ?
	       ' / 5 GHz' : ' / 2.4 GHz'));
}
/* TOMATO64-REMOVE-END */

/* TOMATO64-BEGIN */
function wl_display_ifname(uidx) {
	let result = wl_ifaces[uidx][0];

	if (wl_sunit(uidx) == 0) {
		result += ' (wl' + wl_fface(uidx) + ')';
	}

	if (wl_bands[uidx].length == 1) {
		if (wl_bands[uidx][0] == '1') {
			result += ' / 5 GHz';
		} else if (wl_bands[uidx][0] == '2') {
			result += ' / 2.4 GHz';
		} else if (wl_bands[uidx][0] == '3') {
			result += ' / 6 GHz';
		}
	} else {
		if (nvram['wl' + wl_unit(uidx) + '_nband'] == 1) {
			result += ' / 5 GHz';
		} else {
			result += ' / 2.4 GHz';
		}
	}

	return result;
}
/* TOMATO64-END */
