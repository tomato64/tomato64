/*
	FreshTomato GUI
	Copyright (C) 2023 - 2026 pedro
	https://freshtomato.org/

	Filtering/Extensions

	For use with FreshTomato Firmware only.
	No part of this file may be used without permission.
*/

var filterip = [];
var filteripe = [];
var lanGateways = [];
var lanBroadcasts = [];

var queue = [];
var xob = null;
var cache = [];
var lock = 0;

var resolveCB = 0;
var bcastCB = 0;
var mcastCB = 0;

var numconntotal = 0;
var numconnshown = 0;

function initQoSConnectionNetworks() {
	for (var i = 0; i <= MAX_BRIDGE_ID; ++i) {
		var p = 'lan'+(i ? i : '');
		var ip = nvram[p+'_ipaddr'];
		var netmask = nvram[p+'_netmask'];

		if (ip)
			lanGateways.push(ip);
		if (ip && netmask)
			lanBroadcasts.push(getBroadcastAddress(getNetworkAddress(ip, netmask), netmask));
	}
}

function resolve() {
	if ((queue.length == 0) || (xob))
		return;

	xob = new XmlHttp();
	xob.onCompleted = function(text, xml) {
		eval(text);
		for (var i = 0; i < resolve_data.length; ++i) {
			var r = resolve_data[i];
			if (r[1] == '')
				r[1] = r[0];

			cache[r[0]] = r[1];
			if (lock == 0)
				grid.setName(r[0], r[1]);
		}
		if (queue.length == 0) {
			if ((lock == 0) && (resolveCB) && (grid.sortColumn == 4))
				grid.resort();
		}
		else
			setTimeout(resolve, 500);

		xob = null;
	}
	xob.onError = function(ex) {
		xob = null;
	}

	xob.post('resolve.cgi', 'ip='+queue.splice(0, 20).join(','));
}

function resolveChanged() {
	var b;

	b = E('_f_autoresolve').checked ? 1 : 0;
	if (b != resolveCB) {
		resolveCB = b;
		cookie.set(cprefix+'_resolve', b);
	}
	if (b)
		grid.resolveAll();
}

function setupQoSConnectionGrid(g) {
	g.onClick = function(cell) {
		var row = PR(cell);
		var ip = row.getRowData()[3];
		if (this.lastClicked != row) {
			this.lastClicked = row;
			if (ip.indexOf('<') == -1) {
				queue.push(ip);
				row.style.cursor = 'wait';
				resolve();
			}
		}
		else
			this.resolveAll();
	}

	g.resolveAll = function() {
		var i, ip, row, q, cols, j;

		q = [];
		cols = [1, 3];
		for (i = 1; i < this.tb.rows.length; ++i) {
			row = this.tb.rows[i];
			for (j = cols.length - 1; j >= 0; j--) {
				ip = row.getRowData()[cols[j]];
				if (ip.indexOf('<') == -1) {
					if (!q[ip]) {
						q[ip] = 1;
						queue.push(ip);
					}
					row.style.cursor = 'wait';
				}
			}
		}
		q = null;
		resolve();
	}

	g.setName = function(ip, name) {
		var i, row, data, cols, j;

		cols = [1, 3];
		for (i = this.tb.rows.length - 1; i > 0; --i) {
			row = this.tb.rows[i];
			data = row.getRowData();
			for (j = cols.length - 1; j >= 0; j--) {
				if (data[cols[j]].indexOf(ip) != -1) {
					data[cols[j]] = name+((ip.indexOf(':') != -1) ? '<br>' : ' ')+'<small>('+ip+')<\/small>';
					row.setRowData(data);
					if (E('_f_shortcuts').checked)
						data[cols[j]] = data[cols[j]]+' <small class="pics"><a href="javascript:addExcludeList(\''+ip+'\')" title="Filter out this IP">[hide]<\/a><\/small>';

					row.cells[cols[j]].innerHTML = data[cols[j]];
					row.style.cursor = 'default';
				}
			}
		}
	}
}

function refreshQoSConnections(data, cfg) {
	var i, b, d, cols, j;

	numconntotal = 0;
	numconnshown = 0;

	grid.lastClicked = null;
	grid.removeAllData();

	var c = [];
	var q = [];
	var cursor;
	var ip;
	var fskip;

	cols = [cfg.src, cfg.dst];

	for (i = 0; i < data.length; ++i) {
		fskip = 0;
		numconntotal++;
		b = data[i];

		if (E('_f_excludegw').checked) {
			if ((lanGateways.indexOf(b[cfg.src]) != -1) || (lanGateways.indexOf(b[cfg.dst]) != -1) ||
			    (b[cfg.src] == '127.0.0.1') || (b[cfg.dst] == '127.0.0.1'))
				continue;
		}

		if (E('_f_excludebcast').checked) {
			if ((lanBroadcasts.indexOf(b[cfg.dst]) != -1) ||
			    (b[cfg.dst] == '255.255.255.255') || (b[cfg.dst] == '0.0.0.0'))
				continue;
		}

		if (E('_f_excludemcast').checked) {
			var mmin = 3758096384;	/* aton('224.0.0.0') */
			var mmax = 4026531839;	/* aton('239.255.255.255') */
			if (((aton(b[cfg.src]) >= mmin) && (aton(b[cfg.src]) <= mmax)) ||
			    ((aton(b[cfg.dst]) >= mmin) && (aton(b[cfg.dst]) <= mmax)))
				continue;
		}

		if (filteripe.length > 0) {
			fskip = 0;
			for (x = 0; x < filteripe.length; ++x) {
				if ((b[cfg.src] == filteripe[x]) || (b[cfg.dst] == filteripe[x])) {
					fskip = 1;
					break;
				}
			}
			if (fskip == 1)
				continue;
		}

		if (filterip.length > 0) {
			fskip = 1;
			for (x = 0; x < filterip.length; ++x) {
				if ((b[cfg.src] == filterip[x]) || (b[cfg.dst] == filterip[x])) {
					fskip = 0;
					break;
				}
			}
			if (fskip == 1)
				continue;
		}

		for (j = cols.length - 1; j >= 0; j--) {
			ip = b[cols[j]];
			if (cache[ip] != null) {
				c[ip] = cache[ip];
				b[cols[j]] = cache[ip]+((ip.indexOf(':') != -1) ? '<br>' : ' ')+'<small>('+ip+')<\/small>';
				cursor = 'default';
			}
			else {
				if (resolveCB) {
					if (!q[ip]) {
						q[ip] = 1;
						queue.push(ip);
					}
					cursor = 'wait';
				}
				else
					cursor = null;
			}

			if (E('_f_bold').checked) {
				if ((b[cfg.origin] == '0' && cols[j] == cfg.src) ||
				    (b[cfg.origin] == '1' && cols[j] == cfg.dst))
					b[cols[j]] = '<b>'+b[cols[j]]+'<\/b>';
			}

			if (E('_f_shortcuts').checked) {
				b[cols[j]] += ' <small class="pics">';
				if (cache[ip] == null)
					b[cols[j]] += '<a href="javascript:addToResolveQueue(\''+ip+'\')" title="Resolve the hostname of this address">[resolve]<\/a>';

				b[cols[j]] += ' <a href="javascript:addExcludeList(\''+ip+'\')" title="Filter out this IP">[hide]<\/a><\/small>';
			}
		}

		numconnshown++;

		var reverse = E('_f_originsource').checked && b[cfg.origin] == '1';
		var src = reverse ? cfg.dst : cfg.src;
		var dst = reverse ? cfg.src : cfg.dst;
		var sport = reverse ? cfg.dport : cfg.sport;
		var dport = reverse ? cfg.sport : cfg.dport;

		d = [protocols[b[0]] || b[0], b[src], b[sport], b[dst], b[dport]];

		for (j = 0; j < cfg.fixed.length; ++j)
			d.push(b[cfg.fixed[j]]);

		if (reverse) {
			d.push(b[cfg.traffic[1]]);
			d.push(b[cfg.traffic[0]]);
		}
		else {
			d.push(b[cfg.traffic[0]]);
			d.push(b[cfg.traffic[1]]);
		}

		var row = grid.insertData(-1, d);
		if (cursor)
			row.style.cursor = cursor;
	}

	cache = c;
	c = null;
	q = null;

	grid.resort();
	setTimeout(function() { E('loading').style.display = 'none'; }, 100);

	--lock;

	if (resolveCB)
		resolve();

	if (numconnshown != numconntotal)
		E('qos_numtotalconn').innerHTML = '(showing '+numconnshown+' out of '+numconntotal+' connections)';
	else
		E('qos_numtotalconn').innerHTML = '('+numconntotal+' connections)';
}

function writeQoSConnectionFilters(includeThreshold) {
	var f = [];

	f.push({ title: 'Show only these IPs', name: 'f_filter_ip', size: 50, maxlen: 255, type: 'text', suffix: ' <small>(Comma separated list)<\/small>' });
	f.push({ title: 'Exclude these IPs', name: 'f_filter_ipe', size: 50, maxlen: 255, type: 'text', suffix: ' <small>(Comma separated list)<\/small>' });
	f.push({ title: 'Exclude gateway traffic', name: 'f_excludegw', type: 'checkbox', value: ((nvram.t_hidelr) == '1' ? 1 : 0) });
	f.push({ title: 'Exclude IPv4 broadcast', name: 'f_excludebcast', type: 'checkbox' });
	f.push({ title: 'Exclude IPv4 multicast', name: 'f_excludemcast', type: 'checkbox' });

	if (includeThreshold)
		f.push({ title: 'Ignore inactive connections', name: 'f_excludebythreshold', type: 'checkbox' });

	f.push({ title: 'Auto resolve addresses', name: 'f_autoresolve', type: 'checkbox' });
	f.push({ title: 'Show shortcuts', name: 'f_shortcuts', type: 'checkbox' });
	f.push({ title: 'Bold connection originator', name: 'f_bold', type: 'checkbox' });
	f.push({ title: 'Show connection originator always as Source', name: 'f_originsource', type: 'checkbox' });

	createFieldTable('', f);
}

function addExcludeList(address) {
	if (E('_f_filter_ipe').value.length < 6)
		E('_f_filter_ipe').value = address;
	else if (E('_f_filter_ipe').value.indexOf(address) < 0)
		E('_f_filter_ipe').value += ','+address;

	dofilter();
}

function addToResolveQueue(ip) {
	queue.push(ip);
	resolve();
}

function updateQoSConnectionFilters(setOnce) {
	if (E('_f_filter_ip').value.length > 6)
		filterip = E('_f_filter_ip').value.split(',');
	else
		filterip = [];

	if (E('_f_filter_ipe').value.length > 6)
		filteripe = E('_f_filter_ipe').value.split(',');
	else
		filteripe = [];

	if (!ref.running && setOnce)
		ref.once = 1;

	if (setOnce || !ref.running)
		ref.start();
}

function saveQoSConnectionFilterState() {
	var b;

	b = E('_f_excludebcast').checked ? 1 : 0;
	if (b != bcastCB) {
		bcastCB = b;
		cookie.set(cprefix+'_bcast', b);
	}

	b = E('_f_excludemcast').checked ? 1 : 0;
	if (b != mcastCB) {
		mcastCB = b;
		cookie.set(cprefix+'_mcast', b);
	}

	cookie.set(cprefix+'_shortcuts', E('_f_shortcuts').checked ? '1' : '0', 1);
	cookie.set(cprefix+'_bold', E('_f_bold').checked ? '1' : '0', 1);
	cookie.set(cprefix+'_originsource', E('_f_originsource').checked ? '1' : '0', 1);
}

function restoreQoSConnectionFilterState() {
	var c;

	if ((c = cookie.get(cprefix+'_filterip')) != null) {
		cookie.set(cprefix+'_filterip', '', 0);
		if (c.length > 6) {
			E('_f_filter_ip').value = c;
			filterip = c.split(',');
		}
	}

	if (((c = cookie.get(cprefix+'_resolve')) != null) && (c == '1'))
		E('_f_autoresolve').checked = resolveCB = 1;

	if (((c = cookie.get(cprefix+'_bcast')) != null) && (c == '1'))
		E('_f_excludebcast').checked = bcastCB = 1;

	if (((c = cookie.get(cprefix+'_mcast')) != null) && (c == '1'))
		E('_f_excludemcast').checked = mcastCB = 1;

	restoreVisibility(cprefix, 'filters');

	E('_f_shortcuts').checked = (((c = cookie.get(cprefix+'_shortcuts')) != null) && (c == '1'));
	E('_f_bold').checked = (((c = cookie.get(cprefix+'_bold')) != null) && (c == '1'));
	E('_f_originsource').checked = (((c = cookie.get(cprefix+'_originsource')) != null) && (c == '1'));
}
