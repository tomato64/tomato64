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
<title>[<% ident(); %>] Basic: DDNS Client</title>
<link rel="stylesheet" type="text/css" href="tomato.css?rel=<% version(); %>">
<% css(); %>
<script src="isup.jsz?rel=<% version(); %>"></script>
<script src="tomato.js?rel=<% version(); %>"></script>

<script>

/* DDNS2CLS-BEGIN */
//	<% nvram("ddnsx0,ddnsx1,ddnsx0_refresh,ddnsx1_refresh,ddnsx0_save,ddnsx1_save,ddnsx0_cktime,ddnsx1_cktime,ddnsx0_opendns,ddnsx1_opendns,ddnsx_custom_if,wan_proto,wan2_proto,wan3_proto,wan4_proto,wan_addget,wan2_addget,wan3_addget,wan4_addget,dnscrypt_proxy,stubby_proxy,dnscrypt_priority,stubby_priority"); %>

var clients_num = 2;
/* DDNS2CLS-END */
/* DDNS4CLS-BEGIN */
//	<% nvram("ddnsx0,ddnsx1,ddnsx2,ddnsx3,ddnsx0_refresh,ddnsx1_refresh,ddnsx2_refresh,ddnsx3_refresh,ddnsx0_save,ddnsx1_save,ddnsx2_save,ddnsx3_save,ddnsx0_cktime,ddnsx1_cktime,ddnsx2_cktime,ddnsx3_cktime,ddnsx0_opendns,ddnsx1_opendns,ddnsx2_opendns,ddnsx3_opendns,ddnsx_custom_if,wan_proto,wan2_proto,wan3_proto,wan4_proto,wan_addget,wan2_addget,wan3_addget,wan4_addget,dnscrypt_proxy,stubby_proxy,dnscrypt_priority,stubby_priority"); %>

var clients_num = 4;
/* DDNS4CLS-END */

/* REMOVE-BEGIN
t = hostname (top)
u = username/password
p = password
h = hostname
j = hostname (optional)
c = custom url
w = wildcard
m = MX
b = backup MX
o = use OpenDNS
n = token
s = save state checkbox
REMOVE-END */

var services = [
	['', 'None', '', ''],
	['changeip', 'ChangeIP (https)', 'https://www.changeip.com/', 'uh', 'Email Address'],
	['cloudflare', 'Cloudflare (https)', 'https://www.cloudflare.com/', 'phbnw', null, 'API token', null, 'Proxied', 'Create record if needed', 'Zone ID'],
	['dnsexit', 'DNS Exit (https)', 'https://www.dnsexit.com/', 'uh'],
	['dnshenet', 'dns.he.net (https)', 'https://dns.he.net/', 'u', 'Host name', 'DDNS key'],
	['dnsomatic', 'DNS-O-Matic (https)', 'https://www.dnsomatic.com/', 'uj', null, null, 'Domain <small>(optional)<\/small>'],
	['duckdns', 'Duck DNS (https)', 'https://www.duckdns.org/', 'tn', 'Domain'],
	['dyndns', 'DynDNS (https) - Dynamic', 'https://www.dyndns.com/', 'uhwmbs'],
	['dyndns-static', 'DynDNS (https) - Static', 'https://www.dyndns.com/', 'uhwmbs'],
	['dyndns-custom', 'DynDNS (https) - Custom', 'https://www.dyndns.com/', 'uhwmbs'],
	['easydns', 'easyDNS (https)', 'https://www.easydns.com/', 'uhwm'],
	['enom', 'eNom', 'https://www.enom.com/', 'ut', 'Domain'],
	['afraid', 'FreeDNS (https)', 'https://freedns.afraid.org/', 'n'],
	['heipv6tb', 'HE.net IPv6 Tunnel Broker (https)', 'https://www.tunnelbroker.net/', 'uh', 'Account Name', 'Update Key', 'Tunnel ID'],
	['namecheap', 'namecheap (https)', 'https://www.namecheap.com/', 'ut', 'Domain'],
	['noip', 'No-IP.com (https)', 'https://www.noip.com/', 'uh', 'Email Address', null, 'Hostname / Group'],
	['opendns', 'OpenDNS (https)', 'https://www.opendns.com/', 'uhoz', null, null, 'Network <small>(optional)<\/small>'],
	['ovh', 'OVH (https)', 'https://www.ovh.com/', 'uh'],
	['pairdomains', 'pairDOMAINS (https)', 'https://www.pairdomains.com/', 'uh'],
	['pubyun', 'PubYun [3322]', 'https://www.pubyun.com', 'uhwmb'],
	['pubyun-static', 'PubYun [3322] - Static', 'https://www.pubyun.com', 'uhwmb'],
	['zoneedit', 'ZoneEdit (https)', 'https://www.zoneedit.com/', 'uh'],
	['custom', 'Custom URL', '', 'c']];

var opendns = ['208.67.222.222', '208.67.220.220'];
var no_wan_mode;

function show() {
	var i, j, k;

	for (i = 0; i < clients_num; ++i) {
		for (k = 1; k <= MAXWAN_NUM; k++) {
			j = (k > 1 ? k : '');
			E('_f_ddnsx'+i+'_wanip').options[k - 1].text = 'Use WAN'+(k - 1)+' IP '+(eval('ddnsx'+j+'_wanip') != '0.0.0.0' ? eval('ddnsx'+j+'_wanip') : '');
		}
		elem.setInnerHTML('str-update'+i, msgLoc(ddnsx_last[i]));
		elem.setInnerHTML('str-response'+i, msgLoc(ddnsx_msg[i]));
	}
	elem.setInnerHTML('o_dns', findDNS());
}

function msgLoc(s) {
	var r;

	s = s.trim().replace(/\n+/g, ' ');
	if (r = s.match(/^(.*?): (.*)/)) {
		r[2] = r[2].replace(/#RETRY (\d+) (\d+)/,
			function(s, min, num) {
				return '<small>('+((num >= 1) ? (num+'/10: ') : '')+'Automatically retrying in '+min+' minutes)<\/small>';
			}
		);
		return (new Date(r[1])).toLocaleString()+': '+r[2];
	}
	else if (s.length == 0)
		return '-';

	return s;
}

function mop(s) {
	var op, i;

	op = {};
	for (i = s.length - 1; i >= 0; --i)
		op[s.charAt(i)] = 1;

	return op;
}

function findDNS() {
	var i, j, k, a;
	var dns = [];

	/* we already have stats.dns (isup.jsz) so mark opendns server (only the first one) */
	for (k = 1; k <= MAXWAN_NUM; k++) {
		for (i = 0; i < stats.dns[k - 1].length; ++i) {
			for (j = 0; j < opendns.length; ++j) {
				stats.dns[k - 1][i] = stats.dns[k - 1][i].split(':')[0];
				if (stats.dns[k - 1][i] == opendns[j]) {
					stats.dns[k - 1][i] = '<i>'+stats.dns[k - 1][i]+'<\/i>';
					break;
				}
			}
		}
		stats.dns[k - 1] = stats.dns[k - 1].join(', ');
	}

	/* check for stubby/dnscrypt_proxy */
	if ((nvram.dnscrypt_proxy == 1) || (nvram.stubby_proxy == 1)) {
		for (i = 0; i < stats.dns.length; ++i) {
			dns[i] = '<a href="advanced-dhcpdns.asp">'+(nvram.dnscrypt_proxy == 1 ? 'dnscrypt-proxy' : 'Stubby')+'<\/a>';
			if ((nvram.dnscrypt_proxy == 1 && nvram.dnscrypt_priority == 2) || (nvram.stubby_proxy == 1 && nvram.stubby_priority == 2)) /* no-resolv */
				stats.dns[i] = 'Only: '+dns[i];
			else if ((nvram.dnscrypt_proxy == 1 && nvram.dnscrypt_priority == 1) || (nvram.stubby_proxy == 1 && nvram.stubby_priority == 1)) /* strict-order */
				stats.dns[i] = 'In order of (failover) preference: '+dns[i]+(stats.dns[i] > '' ? ','+stats.dns[i] : '');
			else
				stats.dns[i] = 'Use all: '+dns[i]+(stats.dns[i] > '' ? ','+stats.dns[i] : '');
		}
	}

	/* do not show on disabled interface */
	for (i = 1; i <= MAXWAN_NUM; i++) {
		j = (i > 1 ? i : '');
		if (nvram['wan'+j+'_proto'] == 'disabled')
			stats.dns[i - 1] = '';
		else
			stats.dns[i - 1] = 'WAN'+(i - 1)+': &nbsp;'+stats.dns[i - 1]+'<br>';
	}
	stats.dns = stats.dns.join('');

	return stats.dns;
}

function verifyFields(focused, quiet) {
	var i, j, k, l;
	var data, b, e, ok = 1;
	var op;
	var enabled;
	var used_wans = [0, 0, 0, 0];
	var count_enabled_wans = 0;
	var txt = 'not available when <a href="advanced-dhcpdns.asp">\'Use received DNS with user-entered DNS\'<\/a> is enabled';
	var set_custom_if = '';

	/* prepare used_wans[] array */
	for (i = 0; i < clients_num; ++i) {
		for (j = 1; j <= MAXWAN_NUM; j++) {
			k = (j > 1) ? j : '';
			if (E('_f_opendns'+i+'_wan'+k).checked && E('_f_opendns'+i).checked) /* add as used only if WANx _and_ 'Use as DNS' is checked */
				used_wans[j - 1]++;
			if (!E('_f_opendns'+i).checked) /* otherwise, uncheck WANx */
				E('_f_opendns'+i+'_wan'+k).checked = 0;
			if (nvram['wan'+k+'_proto'] != 'disabled')
				count_enabled_wans++;
		}
	}

	no_wan_mode = 0
	for (i = 0; i < clients_num; ++i) {
		data = services[E('_f_service'+i).selectedIndex] || services[0];
		enabled = (data[0] != '');
		op = mop(data[3]);

		elem.display(PR('_f_ddnsx'+i+'_wanip'), enabled);
		elem.display(PR('_ddnsx'+i+'_refresh'), enabled);
		elem.display('spacer1_'+i, enabled);
		elem.display('spacer2_'+i, enabled);

		b = E('_f_ddnsx'+i+'_wanip').value == 'custom';
		e = E('_f_ddnsx'+i+'_wanip');
		elem.display(PR('_f_custom_ip'+i), b);
		if ((b) && (!v_ip('_f_custom_ip'+i, quiet)))
			ok = 0;
		else
			ferror.clear(e);

		if (!v_range('_ddnsx'+i+'_refresh', quiet, 0, 90))
			ok = 0;

		b = (E('_f_ddnsx'+i+'_wanip').value.indexOf('@') != -1);
		elem.display('nsx_'+i+'_cktime', b);
		if ((enabled) && (b) && !v_range('_f_ddnsx'+i+'_cktime', quiet, 5, 99999))
			ok = 0;
		else
			ferror.clear(e);

		elem.display(PR('url'+i), (enabled && (data[0] != 'custom')));
		elem.display(PR('_f_hosttop'+i), op.t);
		elem.display(PR('_f_user'+i), op.u);
		elem.display(PR('_f_pass'+i), op.u || op.p);
		elem.display(PR('_f_host'+i), op.h || op.j);
		elem.display(PR('_f_cust'+i), 'custmsg'+i, op.c);
		elem.display(PR('_f_wild'+i), op.w);
		elem.display(PR('_f_mx'+i), op.m);
		elem.display(PR('_f_bmx'+i), op.b);
		elem.display(PR('_f_opendns'+i), op.o);

		b = E('_f_opendns'+i).checked;
		for (j = 1; j <= MAXWAN_NUM; j++) {
			k = (j > 1) ? j : '';
			elem.display(PR('_f_opendns'+i+'_wan'+k), op.o && b);
		}

		elem.display(PR('_f_token'+i), op.n);
		elem.display(PR('_f_ddnsx'+i+'_save'), op.s);
		elem.display(PR('_f_force'+i), 'last-response'+i, enabled);
		elem.display('last-update'+i, enabled);

		if (enabled) {
			PR('_f_user'+i).cells[0].innerHTML = data[4] || 'Username';
			PR('_f_pass'+i).cells[0].innerHTML = data[5] || 'Password';
			PR('_f_host'+i).cells[0].innerHTML = data[6] || 'Hostname';
			PR('_f_wild'+i).cells[0].innerHTML = data[7] || 'Wildcard';
			PR('_f_bmx'+i).cells[0].innerHTML  = data[8] || 'Backup MX';
			PR('_f_token'+i).cells[0].innerHTML = data[9] || 'Token';

			e = E('url'+i);
			e.href = data[2];
			e.innerHTML = data[2];

			if (op.c) {
				e = E('_f_cust'+i);
				e.value = e.value.trim();
				if (e.value == '') {
					e.value = 'http://';
				}
				if (e.value.search(/http(s?):\/\/./) != 0) {
					ferror.set(e, 'Expecting a URL -- http://... or https://...', quiet);
					ok = 0;
				}
				else {
					if (!v_nodelim('_f_cust'+i, quiet, 'URL'))
						ok = 0;
					else
						ferror.clear(e);
				}
			}
			else if (op.n) {
				e = E('_f_token'+i);
				e.value = e.value.trim();
				if (e.value.search(/^[A-Za-z0-9]+/) == -1) {
					ferror.set(e, 'Invalid token', quiet);
					ok = 0;
				}
				else {
					if (!v_nodelim('_f_token'+i, quiet, 'Token'))
						ok = 0;
					else
						ferror.clear(e);
				}
			}
			else {
				if (((op.u) && (!v_length('_f_user'+i, quiet, 1) || !v_nodelim('_f_user'+i, quiet, 'Username'))) ||
				    (!v_length('_f_pass'+i, quiet, 1) || !v_nodelim('_f_pass'+i, quiet, 'Password')) ||
				    ((op.m) && (!v_nodelim('_f_mx'+i, quiet, 'MX'))) ||
				    (((op.h) && (!op.o) && (!v_length('_f_host'+i, quiet, 1)) || !v_nodelim('_f_host'+i, quiet, 'Hostname'))) ||
				    ((op.t) && (!v_length('_f_hosttop'+i, quiet, 1) || !v_nodelim('_f_hosttop'+i, quiet, 'Hostname')))) {
					ok = 0;
				}
			}

			if (data[0] == 'opendns') {
				for (j = 1; j <= MAXWAN_NUM; j++) {
					k = (j > 1) ? j : '';

					/* disable 'Use as DNS' for opendns if stubby/dnscrypt-proxy is enabled; display info instead */
					if ((nvram.dnscrypt_proxy == 1) || (nvram.stubby_proxy == 1)) {
						E('_f_opendns'+i).disabled = 1;
						E('_f_opendns'+i).checked = 0;
						elem.display('opendns_info'+i, 1);
					}
					else {
						E('_f_opendns'+i).disabled = 0;
						elem.display('opendns_info'+i, 0);
					}

					/* disable WAN's checkboxes if proto is disabled OR 'Use as DNS' is not active OR wanX-addget is enabled (+display info) */
					/* OR if WANX is already used for opendns as a DNS in one client, disable it on the other(s) */
					if ((nvram['wan'+k+'_proto'] == 'disabled') || (!E('_f_opendns'+i).checked) || (nvram['wan'+k+'_addget'] == 1) || (!E('_f_opendns'+i+'_wan'+k).checked && used_wans[j - 1])) {
						E('_f_opendns'+i+'_wan'+k).disabled = 1;
						E('_f_opendns'+i+'_wan'+k).checked = 0;
						if (nvram['wan'+k+'_addget'] == 1)
							elem.setInnerHTML('opendns_info'+i+j, txt);
					}
					else {
						E('_f_opendns'+i+'_wan'+k).disabled = 0;
						elem.setInnerHTML('opendns_info'+i+j, '');
					}
				}
			}
		} /* -> if (enabled) */

		/* disable/unselect the dropdown item for which WAN is disabled */
		for (j = 1; j <= MAXWAN_NUM; j++) {
			k = (j > 1) ? j : '';
			l = j - 1;
			if (nvram['wan'+k+'_proto'] == 'disabled') {
				if (count_enabled_wans == 0 && l == 0) { /* device is configured with no WAN... */
					no_wan_mode = 1; /* ...so we must define our interface (br0 for example) */
					E('_f_ddnsx'+i+'_wanip')[MAXWAN_NUM].disabled = 0;
					E('_f_ddnsx'+i+'_wanip')[MAXWAN_NUM].text = 'External address checker';
				}
				else {
					E('_f_ddnsx'+i+'_wanip')[l].disabled = 1;
					E('_f_ddnsx'+i+'_wanip')[l].selected = 0;
					E('_f_ddnsx'+i+'_wanip')[l + MAXWAN_NUM].disabled = 1;
					E('_f_ddnsx'+i+'_wanip')[l + MAXWAN_NUM].selected = 0;
				}
			}
			else {
				E('_f_ddnsx'+i+'_wanip')[l].disabled = 0;
				E('_f_ddnsx'+i+'_wanip')[l + MAXWAN_NUM].disabled = 0;
			}
		}

		/* external checker + no WAN mode + enabled: handle custom interface */
		b = (E('_f_ddnsx'+i+'_wanip').value.indexOf('@') != -1) && no_wan_mode && enabled;
		elem.display(E('nsx_'+i+'_custom_if'), b);
		e = E('_f_ddnsx'+i+'_custom_if');
		if (b) {
			if (focused && focused.id.indexOf('_custom_if') > 0) /* to immediately clear errors on all clients */
				set_custom_if = focused.value.trim();
			else
				set_custom_if = e.value.trim();

			if (set_custom_if.search(/^([a-z0-9\.]{2,6})$/) == -1) {
				ferror.set(e, 'Invalid/No interface name. Only "a-z 0-9 ." are allowed (2 - 6 characters)', quiet);
				ok = 0;
			}
			else
				ferror.clear(e);
		}
		else
			ferror.clear(e);
	}

	/* external checker + no WAN mode: copy custom interface to all clients */
	if (set_custom_if.length > 0) {
		for (i = 0; i < clients_num; ++i)
			E('_f_ddnsx'+i+'_custom_if').value = set_custom_if;
	}

	return ok;
}

function save() {
	var fom, i, j, k, l, m, s, data, a, b, op;
	var setopendns = [0, 0];
	var bits = [1, 2, 4, 8];

	if (!verifyFields(null, 0))
		return;

	fom = E('t_fom');

	fom.ddnsx_custom_if.value = no_wan_mode ? fom._f_ddnsx0_custom_if.value : ''; /* they are the same, just use the first one */

	for (k = 1; k <= MAXWAN_NUM; k++) {
		l = (k > 1 ? k : '');
		fom['wan'+l+'_dns'].disabled = 1; /* reset */
		fom['wan'+l+'_dns_auto'].disabled = 1; /* reset */
		fom['wan'+l+'_dns'].value = ''; /* reset */
		fom['wan'+l+'_dns_auto'].value = ''; /* reset */
	}

	for (i = 0; i < clients_num; ++i) {
		fom['ddnsx'+i+'_save'].value = (nvram['ddnsx'+i+'_save'] == 1 ? 1 : 0);
		fom['ddnsx'+i+'_ip'].value = (fom['_f_ddnsx'+i+'_wanip'].value == 'custom' ? fom['_f_custom_ip'+i].value : fom['_f_ddnsx'+i+'_wanip'].value);
		fom['ddnsx'+i+'_cktime'].disabled = 0; /* reset */
		fom['ddnsx'+i+'_cktime'].value = fom['_f_ddnsx'+i+'_cktime'].value > 0 ? fom['_f_ddnsx'+i+'_cktime'].value : fom['ddnsx'+i+'_cktime'].disabled = 1;
		fom['ddnsx'+i+'_opendns'].value = 0; /* init with 0 */

		s = [];
		data = (services[fom['_f_service'+i].selectedIndex] || services[0]);
		s.push(data[0]);

		/* one of the services is enabled */
		if (data[0] != '') {

/* REMOVE-BEGIN
t = hostname (top)
u = username/password
p = password
h = hostname
c = custom url
w = wildcard
m = MX
b = backup MX
o = use OpenDNS
n = token
s = save state checkbox
username:password<hostname<wildcard<mx<backup mx<custom url/token<
REMOVE-END */

			op = mop(data[3]);

			if ((op.u) || (op.p))
				s.push(fom['_f_user'+i].value+':'+fom['_f_pass'+i].value);
			else if (services[E('_f_service'+i).selectedIndex][0] == 'custom') { /* for custom, add (if any) username and pass from the url to auth it with wget */
				a = E('_f_cust'+i);
				b = a.value.trim();

				if (b.indexOf('@') != -1) {
					a = b.split('//');
					b = a[1].split('@');
					a = b[0].split(':');

					if (a[0] && a[1])
						s.push(a[0]+':'+a[1]);
					else
						s.push('');
				}
				else
					s.push('');
			}
			else
				s.push('');

			if (op.t)
				s.push(fom['_f_hosttop'+i].value);
			else if ((op.h) || (op.j))
				s.push(fom['_f_host'+i].value);
			else
				s.push('');

			if (op.w)
				s.push(fom['_f_wild'+i].checked ? 1 : 0);
			else
				s.push('');

			if (op.m)
				s.push(fom['_f_mx'+i].value)
			else
				s.push('');

			if (op.b)
				s.push(fom['_f_bmx'+i].checked ? 1 : 0);
			else
				s.push('');

			if (op.c)
				s.push(fom['_f_cust'+i].value);
			else if (op.n)
				s.push(fom['_f_token'+i].value);
			else
				s.push('');

			if (op.s)
				fom['ddnsx'+i+'_save'].value = (fom['_f_ddnsx'+i+'_save'].checked ? 1 : 0);

			if (data[0] == 'opendns') {
				setopendns[i] = fom['_f_opendns'+i].checked;
				/* set the ddnsxX_opendns value correctly: bit 0 = WAN0, bit 1 = WAN1, bit 2 = WAN2, bit 3 = WAN3 */
				if (setopendns[i]) { /* only if 'Use as DNS' is checked */
					l = 0;
					for (j = 1; j <= MAXWAN_NUM; j++) {
						k = (j > 1) ? j : '';
						if (fom['_f_opendns'+i+'_wan'+k].checked) {
							l++;
							fom['ddnsx'+i+'_opendns'].value |= bits[j - 1];
						}
					}
					if (l == 0) {
						alert('You must select at least one WAN');
						return;
					}
				}
			}
		} /* -> if (data[0] != '') */
		s = s.join('<');
		fom['ddnsx'+i].value = s;
		fom['ddnsx'+i+'_cache'].disabled = (!fom['_f_force'+i].checked) && (s == nvram['ddnsx'+i]); /* disable cache if force is not checked _and_ no changes in ddnsxX */

		fom['_f_force'+i].checked = 0; /* reset */
		nvram['ddnsx'+i] = s; /* update */

		if (setopendns[i]) { /* 'Use as DNS' for opendns is enabled (0 -> 1 or off -> 1 or just 1 -> 1 but with different set of WANs) */
			if (nvram['ddnsx'+i+'_opendns'] == fom['ddnsx'+i+'_opendns'].value) /* no change */
				continue;

			for (k = 1; k <= MAXWAN_NUM; k++) {
				l = (k > 1 ? k : '');
				if (((nvram['ddnsx'+i+'_opendns'] & bits[k - 1]) && fom['_f_opendns'+i+'_wan'+l].checked) ||
				    ((!(nvram['ddnsx'+i+'_opendns'] & bits[k - 1])) && !fom['_f_opendns'+i+'_wan'+l].checked)) { /* no change - continue */
					continue;
				}
				else if ((!(nvram['ddnsx'+i+'_opendns'] & bits[k - 1])) && fom['_f_opendns'+i+'_wan'+l].checked) { /* 0 -> 1 (WAN l checked on client i) */
					fom['wan'+l+'_dns'].disabled = 0;
					fom['wan'+l+'_dns'].value = opendns.join(' '); /* add opendns DNS IPs */
					fom['wan'+l+'_dns_auto'].disabled = 0;
					fom['wan'+l+'_dns_auto'].value = 0; /* it has to be set to manual */
					if (fom._service.value.indexOf('dnsmasq') < 0)
						fom._service.value += ',dnsmasq-restart';
				}
				else if ((nvram['ddnsx'+i+'_opendns'] & bits[k - 1]) && !fom['_f_opendns'+i+'_wan'+l].checked) { /* 1 -> 0 (WAN l unchecked on client i) */
					fom['wan'+l+'_dns'].disabled = 0; /* reset to null */
					fom['wan'+l+'_dns_auto'].disabled = 0;
					fom['wan'+l+'_dns_auto'].value = 1; /* back to auto */
					if (fom._service.value.indexOf('dnsmasq') < 0)
						fom._service.value += ',dnsmasq-restart';

					//if (fom._service.value.indexOf('wan'+k) < 0) /* needs testing if it's required - we may end up with wrong DNS servers in wanX_get_dns */
					//	fom._service.value += ',wan'+k+'-restart';
				}
			}
			nvram['ddnsx'+i+'_opendns'] = fom['ddnsx'+i+'_opendns'].value; /* update */
		}
		else { /* 'Use as DNS' for opendns is disabled (0 -> 0 or 1 -> 0 or 1 -> off or not opendns) - disable it */
			for (k = 1; k <= MAXWAN_NUM; k++) {
				l = (k > 1 ? k : '');
				if (nvram['ddnsx'+i+'_opendns'] & bits[k - 1]) {
					fom['wan'+l+'_dns'].disabled = 0; /* reset to null */
					fom['wan'+l+'_dns_auto'].disabled = 0;
					fom['wan'+l+'_dns_auto'].value = 1; /* back to auto */
					if (fom._service.value.indexOf('dnsmasq') < 0)
						fom._service.value += ',dnsmasq-restart';

					//if (fom._service.value.indexOf('wan'+k) < 0) /* needs testing if it's required - we may end up with wrong DNS servers in wanX_get_dns */
					//	fom._service.value += ',wan'+k+'-restart';
				}
			}
			nvram['ddnsx'+i+'_opendns'] = 0; /* reset */
			E('_f_opendns'+i).checked = 0; /* reset */
		}
	}

	form.submit(fom, 1);
	fom._service.value = 'ddns-restart'; /*  reset */
}

function init() {
	up.initPage(250, 5);
}
</script>
</head>

<body onload="init()">
<form id="t_fom" method="post" action="tomato.cgi">
<table id="container">
<tr><td colspan="2" id="header">
	<div class="title">Tomato64</div>
	<div class="version">Version <% version(); %> on <% nv("t_model_name"); %></div>
</td></tr>
<tr id="body"><td id="navi"><script>navi()</script></td>
<td id="content">
<div id="ident"><% ident(); %> | <script>wikiLink();</script></div>

<!-- / / / -->

<input type="hidden" name="_nextpage" value="basic-ddns.asp">
<input type="hidden" name="_service" value="ddns-restart">
<input type="hidden" name="wan_dns" value="" disabled="disabled">
<input type="hidden" name="wan2_dns" value="" disabled="disabled">
<input type="hidden" name="wan_dns_auto" value="" disabled="disabled">
<input type="hidden" name="wan2_dns_auto" value="" disabled="disabled">
<input type="hidden" name="ddnsx_custom_if">
<!-- MULTIWAN-BEGIN -->
<input type="hidden" name="wan3_dns" value="" disabled="disabled">
<input type="hidden" name="wan4_dns" value="" disabled="disabled">
<input type="hidden" name="wan3_dns_auto" value="" disabled="disabled">
<input type="hidden" name="wan4_dns_auto" value="" disabled="disabled">
<!-- MULTIWAN-END -->
<input type="hidden" name="ddnsx0" value="">
<input type="hidden" name="ddnsx1" value="">
<input type="hidden" name="ddnsx0_cache" value="" disabled="disabled">
<input type="hidden" name="ddnsx1_cache" value="" disabled="disabled">
<input type="hidden" name="ddnsx0_ip" value="">
<input type="hidden" name="ddnsx1_ip" value="">
<input type="hidden" name="ddnsx0_save" value="">
<input type="hidden" name="ddnsx1_save" value="">
<input type="hidden" name="ddnsx0_cktime" value="">
<input type="hidden" name="ddnsx1_cktime" value="">
<input type="hidden" name="ddnsx0_opendns">
<input type="hidden" name="ddnsx1_opendns">
<!-- DDNS4CLS-BEGIN -->
<input type="hidden" name="ddnsx2" value="">
<input type="hidden" name="ddnsx3" value="">
<input type="hidden" name="ddnsx2_cache" value="" disabled="disabled">
<input type="hidden" name="ddnsx3_cache" value="" disabled="disabled">
<input type="hidden" name="ddnsx2_ip" value="">
<input type="hidden" name="ddnsx3_ip" value="">
<input type="hidden" name="ddnsx2_save" value="">
<input type="hidden" name="ddnsx3_save" value="">
<input type="hidden" name="ddnsx2_cktime" value="">
<input type="hidden" name="ddnsx3_cktime" value="">
<input type="hidden" name="ddnsx2_opendns">
<input type="hidden" name="ddnsx3_opendns">
<!-- DDNS4CLS-END -->

<!-- / / / -->

<script>
	var i, v, u, h, s, a;

	for (i = 0; i < clients_num; ++i) {
		W('<div class="section-title">Dynamic DNS Client '+(i + 1)+'<\/div><div class="section">');

		/* check if correct */
		v = nvram['ddnsx'+i].split('<');
		if (v.length != 7)
			v = ['', '', '', 0, '', 0, ''];

		u = v[1].split(':');
		if (u.length != 2)
			u = ['', ''];

		h = (v[0] == '');

		s = eval('ddnsx'+i+'_ip_get');
		a = (s != '') && (s != 'wan') && (s != 'wan2')
/* MULTIWAN-BEGIN */
		    && (s != 'wan3') && (s != 'wan4')
/* MULTIWAN-END */
		    && (s.indexOf('@') != 0) && (s != '0.0.0.0') && (s != '10.1.1.1');

		createFieldTable('', [
			{ title: 'Service', name: 'f_service'+i, type: 'select', options: services, value: v[0] },
				{ title: 'URL', indent: 2, text: '<a href="#" id="url'+i+'" target="tomato-ext-ddns"><\/a>', hidden: 1 },
			{ title: 'Hostname', name: 'f_hosttop'+i, type: 'text', maxlen: 96, size: 35, value: v[2], hidden: 1 },
			{ title: 'Username', name: 'f_user'+i, type: 'text', maxlen: 64, size: 35, value: u[0], hidden: 1 },
			{ title: 'Password', name: 'f_pass'+i, type: 'password', maxlen: 64, size: 35, value: u[1], hidden: 1 },
			{ title: 'Hostname', name: 'f_host'+i, type: 'text', maxlen: 255, size: 80, value: v[2], hidden: 1 },
			{ title: 'URL', name: 'f_cust'+i, type: 'text', maxlen: 255, size: 80, value: v[6], hidden: 1 },
			{ title: ' ', text: '(Use @IP <b>OR<\/b> @IP6 for the current IP(v6) address)', rid: ('custmsg'+i), hidden: 1 },
				{ title: 'Wildcard', indent: 2, name: 'f_wild'+i, type: 'checkbox', value: v[3] != '0', hidden: 1 },
			{ title: 'MX', name: 'f_mx'+i, type: 'text', maxlen: 32, size: 35, value: v[4], hidden: 1 },
				{ title: 'Backup MX', indent: 2, name: 'f_bmx'+i, type: 'checkbox', value: v[5] != '0', hidden: 1 },
			{ title: 'Use as DNS', name: 'f_opendns'+i, type: 'checkbox', value: (nvram['ddnsx'+i+'_opendns'] > 0), suffix: '<span id="opendns_info'+i+'">not available when using <a href="advanced-dhcpdns.asp">Stubby/dnscrypt-proxy<\/a><\/span>', hidden: 1 },
				{ title: 'WAN0', indent: 2, name: 'f_opendns'+i+'_wan', type: 'checkbox', value: (nvram['ddnsx'+i+'_opendns'] & 0x01), suffix: '<span id="opendns_info'+i+'1">&nbsp;<\/span>', hidden: 1 },
				{ title: 'WAN1', indent: 2, name: 'f_opendns'+i+'_wan2', type: 'checkbox', value: (nvram['ddnsx'+i+'_opendns'] & 0x02), suffix: '<span id="opendns_info'+i+'2">&nbsp;<\/span>', hidden: 1 },
/* MULTIWAN-BEGIN */
				{ title: 'WAN2', indent: 2, name: 'f_opendns'+i+'_wan3', type: 'checkbox', value: (nvram['ddnsx'+i+'_opendns'] & 0x04), suffix: '<span id="opendns_info'+i+'3">&nbsp;<\/span>', hidden: 1 },
				{ title: 'WAN3', indent: 2, name: 'f_opendns'+i+'_wan4', type: 'checkbox', value: (nvram['ddnsx'+i+'_opendns'] & 0x08), suffix: '<span id="opendns_info'+i+'4">&nbsp;<\/span>', hidden: 1 },
/* MULTIWAN-END */
			{ title: 'Token', name: 'f_token'+i, type: 'text', maxlen: 255, size: 80, value: v[6], hidden: 1 },
			{ title: 'Save state when IP changes (nvram commit)', name: 'f_ddnsx'+i+'_save', type: 'checkbox', value: nvram['ddnsx'+i+'_save'] == 1, hidden: 1 },
			{ title: 'Force next update', name: 'f_force'+i, type: 'checkbox', value: 0, hidden: 1 },
			{ title: '', rid: 'spacer1_'+i },
			{ title: 'IP address', multi: [
				{ name: 'f_ddnsx'+i+'_wanip', type: 'select', options: [['wan','Use WAN0 IP'],['wan2','Use WAN1 IP' ],
/* MULTIWAN-BEGIN */
					['wan3','Use WAN2 IP' ],['wan4','Use WAN3 IP' ],
/* MULTIWAN-END */
					['@1','External WAN0 IP checker'],['@2','External WAN1 IP checker'],
/* MULTIWAN-BEGIN */
					['@3','External WAN2 IP checker'],['@4','External WAN3 IP checker'],
/* MULTIWAN-END */
					['0.0.0.0','Offline (0.0.0.0)'],['10.1.1.1','Offline (10.1.1.1)'],['custom','Custom IP Address...']], value: (a ? 'custom' : s) },
				{ name: 'f_ddnsx'+i+'_custom_if', type: 'text', maxlen: 6, size: 6, prefix: '<span id="nsx_'+i+'_custom_if"><span id="note_cktime1_'+i+'a">interface:<\/span>', suffix: '<\/span>', value: nvram['ddnsx_custom_if'] },
				{ name: 'f_ddnsx'+i+'_cktime', type: 'text', maxlen: 5, size: 6, prefix: '<span id="nsx_'+i+'_cktime"><span id="note_cktime1_'+i+'b">every:<\/span>', suffix: '<span id="note_cktime2_'+i+'">mins (5 - 99999, default: 10)<\/span><\/span>', value: nvram['ddnsx'+i+'_cktime'] } ] },
				{ title: 'Custom IP address', indent: 2, name: 'f_custom_ip'+i, type: 'text', maxlen: 15, size: 20, value: (a ? s : ''), hidden: !a },
			{ title: 'Auto refresh every', name: 'ddnsx'+i+'_refresh', type: 'text', maxlen: 8, size: 8, suffix: '<small> days (0 - disable)<\/small>', value: fixInt(nvram['ddnsx'+i+'_refresh'], 0, 90, 28) },
			{ title: '', rid: 'spacer2_'+i },
			{ title: 'Last IP Address', custom: '<span id="str-update'+i+'"><\/span>', rid: 'last-update'+i, hidden: 1 },
			{ title: 'Last Result', custom: '<span id="str-response'+i+'"><\/span>', rid: 'last-response'+i, hidden: h }
		]);

		W('<\/div>');
	}

/* / / / */

	W('<div class="section-title"><\/div><div class="section">');
	createFieldTable('', [ { title: 'Current DNS:', text: '<span id="o_dns">&nbsp;<\/span>' } ]);
	W('<\/div>');
</script>

<!-- / / / -->

<div id="footer">
	<span id="footer-msg"></span>
	<input type="button" value="Save" id="save-button" onclick="save()">
	<input type="button" value="Cancel" id="cancel-button" onclick="reloadPage();">
</div>

</td></tr>
</table>
</form>
<script>verifyFields(null, 1);</script>
</body>
</html>
