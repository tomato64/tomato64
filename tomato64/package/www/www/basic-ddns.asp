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
<link rel="stylesheet" type="text/css" href="tomato.css">
<% css(); %>
<script src="isup.jsz"></script>
<script src="tomato.js"></script>

<script>

//	<% nvram("ddnsx0,ddnsx1,ddnsx_refresh,ddnsx_save,ddnsx_cktime"); %>

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
	['dyns', 'DyNS', 'http://www.dyns.cx/', 'uh'],
	['easydns', 'easyDNS (https)', 'https://www.easydns.com/', 'uhwm'],
	['enom', 'eNom', 'https://www.enom.com/', 'ut', 'Domain'],
	['afraid', 'FreeDNS (https)', 'https://freedns.afraid.org/', 'n'],
	['heipv6tb', 'HE.net IPv6 Tunnel Broker (https)', 'https://www.tunnelbroker.net/', 'uh', 'Account Name', 'Update Key', 'Tunnel ID'],
	['ieserver', 'ieServer.net (https)', 'http://www.ieserver.net/', 'uhz', 'Username / Hostname', null, 'Domain'],
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
var opendnsInUse = 0;
var clients_num = 2;

/* DUALWAN-BEGIN */
maxwan_num = 2;
/* DUALWAN-END */

/* MULTIWAN-BEGIN */
maxwan_num = 4;
/* MULTIWAN-END */


function show() {
	var i, j;

	for (i = 1; i <= maxwan_num; i++) {
		j = (i > 1 ? i : '');
		E('_f_ddnsx_ip').options[i - 1].text = 'Use WAN'+(i - 1)+' IP Address '+(eval('ddnsx'+j+'_ip') != '0.0.0.0' ? eval('ddnsx'+j+'_ip') : '');
	}

	for (i = 0; i < clients_num; ++i) {
		elem.setInnerHTML('str-update'+i, msgLoc(ddnsx_last[i]));
		elem.setInnerHTML('str-response'+i, msgLoc(ddnsx_msg[i]));
		elem.setInnerHTML('o_dns'+i, findDNS(wan_dns_nvram, wan_get_dns_nvram, dns_addget_nvram));
	}
}

function msgLoc(s) {
	var r;

	s = s.trim().replace(/\n+/g, ' ');
	if (r = s.match(/^(.*?): (.*)/)) {
		r[2] = r[2].replace(/#RETRY (\d+) (\d+)/,
			function(s, min, num) {
				return '<small>('+((num >= 1) ? (num+'/3: ') : '')+'Automatically retrying in '+min+' minutes)<\/small>';
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
	for (i = s.length - 1; i >= 0; --i) {
		op[s.charAt(i)] = 1;
	}

	return op;
}

function findDNS(wan_dns, wan_get_dns, dns_addget) {
	var i, j, a, dns;

	a = wan_dns.split(/\s+/);
	for (i = 0; i < a.length; ++i) {
		for (j = 0; j < opendns.length; ++j) {
			if (a[i] == opendns[j])
				++opendnsInUse;
		}
	}

	if (nvram.dns_addget == 1)
		dns = wan_dns+' '+wan_get_dns;
	else if (wan_dns != '')
		dns = wan_dns;
	else
		dns = wan_get_dns;

	dns = dns.split(/\s+/);
	for (i = 0; i < dns.length; ++i) {
		for (j = 0; j < opendns.length; ++j) {
			if (dns[i] == opendns[j]) {
				dns[i] = '<i>'+dns[i]+'<\/i>';
				break;
			}
		}
	}
	dns = dns.join(', ');

	return dns;
}

function verifyFields(focused, quiet) {
	var i;
	var data, b, e, r = 1;
	var op;
	var enabled;

	b = E('_f_ddnsx_ip').value == 'custom';
	e = E('_f_ddnsx_ip');
	elem.display(PR('_f_custom_ip'), b);
	if ((b) && (!v_ip('_f_custom_ip', quiet)))
		r = 0;
	else
		ferror.clear(e);

	if (!v_range('_ddnsx_refresh', quiet, 0, 90))
		r = 0;

	b = (E('_f_ddnsx_ip').value.indexOf('@') != -1);
	e = E('_f_ddnsx_cktime');
	e.disabled = !b;
	if ((b) && !v_range('_f_ddnsx_cktime', quiet, 5, 99999))
		r = 0;
	else
		ferror.clear(e);

	for (i = 0; i < clients_num; ++i) {
		data = services[E('_f_service'+i).selectedIndex] || services[0];
		enabled = (data[0] != '');

		op = mop(data[3]);

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
		elem.display(PR('_f_token'+i), op.n);
		elem.display(PR('_f_ddnsx_save'+i), op.s);

		elem.display(PR('_f_force'+i), 'last-response'+i, enabled);
		elem.display('last-update'+i, enabled);

		if (enabled) {
			PR('_f_user'+i).cells[0].innerHTML = data[4] || 'Username';
			PR('_f_pass'+i).cells[0].innerHTML = data[5] || 'Password';
			PR('_f_host'+i).cells[0].innerHTML = data[6] || 'Hostname';
			PR('_f_wild'+i).cells[0].innerHTML = data[7] || 'Wildcard';
			PR('_f_bmx'+i).cells[0].innerHTML = data[8] || 'Backup MX';
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
					r = 0;
				}
				else {
					if (!v_nodelim('_f_cust'+i, quiet, 'URL'))
						r = 0;
					else
						ferror.clear(e);
				}
			}
			else if (op.n) {
				e = E('_f_token'+i);
				e.value = e.value.trim();
				if (e.value.search(/^[A-Za-z0-9]+/) == -1) {
					ferror.set(e, 'Invalid token', quiet);
					r = 0;
				}
				else {
					if (!v_nodelim('_f_token'+i, quiet, 'Token'))
						r = 0;
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
					r = 0;
				}
			}
		}
	}

	/* shouldn't do this twice, but... */
	if (E('_f_opendns0') == focused)
		E('_f_opendns1').checked = E('_f_opendns0').checked;
	if (E('_f_opendns1') == focused)
		E('_f_opendns0').checked = E('_f_opendns1').checked;

	if (E('_f_ddnsx_save0') == focused)
		E('_f_ddnsx_save1').checked = E('_f_ddnsx_save0').checked;
	if (E('_f_ddnsx_save1') == focused)
		E('_f_ddnsx_save0').checked = E('_f_ddnsx_save1').checked;

	return r;
}

function save() {
	var fom, i, j, s, data, a, b, setopendns, op;

	if (!verifyFields(null, 0))
		return;

	fom = E('t_fom');
	fom.ddnsx_save.value = (nvram.ddnsx_save == 1 ? 1 : 0);
	fom.ddnsx_ip.value = (fom._f_ddnsx_ip.value == 'custom' ? fom._f_custom_ip.value : fom._f_ddnsx_ip.value);
	fom.ddnsx_cktime.value = fom._f_ddnsx_cktime.value;

	setopendns = -1;
	for (i = 0; i < clients_num; ++i) {
		s = [];
		data = services[fom['_f_service'+i].selectedIndex] || services[0];
		s.push(data[0]);
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
				fom.ddnsx_save.value = (fom['_f_ddnsx_save'+i].checked ? 1 : 0);

			if (data[0] == 'opendns')
				setopendns = fom['_f_opendns'+i].checked;
		}
		s = s.join('<');
		fom['ddnsx'+i].value = s;
		fom['ddnsx'+i+'_cache'].disabled = (!fom['_f_force'+i].checked) && (s == nvram['ddnsx'+i]); /* disable cache if force is not checked _and_ no changes in ddnsxX */

		fom['_f_force'+i].checked = 0; /* reset */
		nvram['ddnsx'+i] = s; /* update */
	}

	if (setopendns != -1) {
		if (setopendns) {
			if (opendnsInUse != opendns.length) {
				fom.wan_dns.value = opendns.join(' ');
				fom.wan_dns.disabled = 0;
				fom._service.value += ',dns-restart';
			}
		}
		else {
			/* not set if partial, do not remove if partial */
			if (opendnsInUse == opendns.length) {
				a = wan_dns_nvram.split(/\s+/);
				b = [];
				for (i = a.length - 1; i >= 0; --i) {
					for (j = opendns.length - 1; j >= 0; --j) {
						if (a[i] == opendns[j]) {
							a.splice(i, 1);
							break;
						}
					}
				}
				fom.wan_dns.value = a.join(' ');
				fom.wan_dns.disabled = 0;
				fom._service.value += ',dns-restart';
			}
		}
	}

	form.submit(fom, 1);
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
<input type="hidden" name="_nextwait" value="10">
<input type="hidden" name="ddnsx0" value="">
<input type="hidden" name="ddnsx1" value="">
<input type="hidden" name="ddnsx0_cache" value="" disabled="disabled">
<input type="hidden" name="ddnsx1_cache" value="" disabled="disabled">
<input type="hidden" name="wan_dns" value="" disabled="disabled">
<input type="hidden" name="ddnsx_ip" value="">
<input type="hidden" name="ddnsx_save" value="">
<input type="hidden" name="ddnsx_cktime" value="">

<!-- / / / -->

<div class="section-title">Dynamic DNS Client</div>
<div class="section">
	<script>
		var s = ddnsx_ip_nvram;
		var a = (s != '') && (s != 'wan') && (s != 'wan2') && (s != 'wan3') && (s != 'wan4') && (s.indexOf('@') != 0) && (s != '0.0.0.0') && (s != '10.1.1.1');

		createFieldTable('', [
			{ title: 'IP address', multi: [
				{ name: 'f_ddnsx_ip', type: 'select', options: [['wan','Use WAN0 IP Address'],['wan2','Use WAN1 IP Address' ],
/* MULTIWAN-BEGIN */
					['wan3','Use WAN2 IP Address' ],['wan4','Use WAN3 IP Address' ],
/* MULTIWAN-END */
					['@','External IP address checker - every:'],['0.0.0.0','Offline (0.0.0.0)'],['10.1.1.1','Offline (10.1.1.1)'],['custom','Custom IP Address...']], value: (a ? 'custom' : ddnsx_ip_nvram) },

				{ name: 'f_ddnsx_cktime', type: 'text', maxlen: 5, size: 6, suffix: '<small> minutes; range: 5 - 99999, default: 10<\/small>', value: nvram.ddnsx_cktime } ] },
				{ title: 'Custom IP address', indent: 2, name: 'f_custom_ip', type: 'text', maxlen: 15, size: 20, value: (a ? ddnsx_ip_nvram : ''), hidden: !a },
			{ title: 'Auto refresh every', name: 'ddnsx_refresh', type: 'text', maxlen: 8, size: 8, suffix: '<small> days (0 - disable)<\/small>', value: fixInt(nvram.ddnsx_refresh, 0, 90, 28) }
		]);
	</script>
</div>

<!-- / / / -->

<script>
	var i, v, u, h, dns;

	dns = findDNS(wan_dns_nvram, wan_get_dns_nvram, dns_addget_nvram);

	/* check if correct */
	for (i = 0; i < clients_num; ++i) {
		v = nvram['ddnsx'+i].split('<');
		if (v.length != 7)
			v = ['', '', '', 0, '', 0, ''];

		u = v[1].split(':');
		if (u.length != 2)
			u = ['', ''];

		h = (v[0] == '');

		W('<div class="section-title">Dynamic DNS '+(i + 1)+'<\/div><div class="section">');
		createFieldTable('', [
			{ title: 'Service', name: 'f_service'+i, type: 'select', options: services, value: v[0] },
				{ title: 'URL', indent: 2, text: '<a href="#" id="url'+i+'" target="tomato-ext-ddns"><\/a>', hidden: 1 },
			{ title: 'Hostname', name: 'f_hosttop'+i, type: 'text', maxlen: 96, size: 35, value: v[2], hidden: 1 },
			{ title: 'Username', name: 'f_user'+i, type: 'text', maxlen: 64, size: 35, value: u[0], hidden: 1 },
			{ title: 'Password', name: 'f_pass'+i, type: 'password', maxlen: 64, size: 35, value: u[1], hidden: 1 },
			{ title: 'Hostname', name: 'f_host'+i, type: 'text', maxlen: 255, size: 80, value: v[2], hidden: 1 },
			{ title: 'URL', name: 'f_cust'+i, type: 'text', maxlen: 255, size: 80, value: v[6], hidden: 1 },
			{ title: ' ', text: '(Use @IP for the current IP address)', rid: ('custmsg'+i), hidden: 1 },
				{ title: 'Wildcard', indent: 2, name: 'f_wild'+i, type: 'checkbox', value: v[3] != '0', hidden: 1 },
			{ title: 'MX', name: 'f_mx'+i, type: 'text', maxlen: 32, size: 35, value: v[4], hidden: 1 },
				{ title: 'Backup MX', indent: 2, name: 'f_bmx'+i, type: 'checkbox', value: v[5] != '0', hidden: 1 },
			{ title: 'Use as DNS', name: 'f_opendns'+i, type: 'checkbox', value: (opendnsInUse == opendns.length), suffix: '<br><small>(Current DNS: <span id="o_dns'+i+'">'+dns+'<\/span>)<\/small>', hidden: 1 },
			{ title: 'Token', name: 'f_token'+i, type: 'text', maxlen: 255, size: 80, value: v[6], hidden: 1 },
			{ title: 'Save state when IP changes (nvram commit)', name: 'f_ddnsx_save'+i, type: 'checkbox', value: nvram.ddnsx_save == 1, hidden: 1 },
			{ title: 'Force next update', name: 'f_force'+i, type: 'checkbox', value: 0, hidden: 1 },
			null,
			{ title: 'Last IP Address', custom: '<span id="str-update'+i+'"><\/span>', rid: 'last-update'+i, hidden: 1 },
			{ title: 'Last Result', custom: '<span id="str-response'+i+'"><\/span>', rid: 'last-response'+i, hidden: h }
		]);
		W('<\/div>');
	}
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
