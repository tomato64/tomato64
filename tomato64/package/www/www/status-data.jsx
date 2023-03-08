/*
	Tomato GUI
	Copyright (C) 2006-2010 Jonathan Zarate
	http://www.polarcloud.com/tomato/

	For use with Tomato Firmware only.
	No part of this file may be used without permission.
*/

//	<% nvram("router_name,wan_domain,wan_hostname,wan_dns,lan_hwaddr,lan_proto,lan_ipaddr,dhcpd_startip,dhcpd_endip,lan_netmask,wl_security_mode,wl_crypto,wl_mode,wl_wds_enable,wl_hwaddr,wl_net_mode,wl_radio,wl_channel,lan_gateway,wl_ssid,wl_closed,wl_nband,t_model_name,t_features,dhcpd1_startip,dhcpd1_endip,dhcpd2_startip,dhcpd2_endip,dhcpd3_startip,dhcpd3_endip,lan1_proto,lan1_ipaddr,lan1_netmask,lan2_proto,lan2_ipaddr,lan2_netmask,lan3_proto,lan3_ipaddr,lan3_netmask,lan_ifname,lan1_ifname,lan2_ifname,lan3_ifname,lan_ifnames,lan1_ifnames,lan2_ifnames,lan3_ifnames,wan_ifnames,wan2_ifnames,wan3_ifnames,wan4_ifnames,tomatoanon_enable,tomatoanon_answer,lan_desc,wan_ppp_get_ip,wan_pptp_dhcp,wan_pptp_server_ip,wan_ipaddr_buf,wan_gateway,wan_gateway_get,wan_get_domain,wan_hwaddr,wan_ipaddr,wan_netmask,wan_proto,wan_run_mtu,wan_sta,wan2_ppp_get_ip,wan2_pptp_dhcp,wan2_pptp_server_ip,wan2_ipaddr_buf,wan2_gateway,wan2_gateway_get,wan2_get_domain,wan2_hwaddr,wan2_ipaddr,wan2_netmask,wan2_proto,wan2_run_mtu,wan2_sta,wan3_ppp_get_ip,wan3_pptp_dhcp,wan3_pptp_server_ip,wan3_ipaddr_buf,wan3_gateway,wan3_gateway_get,wan3_get_domain,wan3_hwaddr,wan3_ipaddr,wan3_netmask,wan3_proto,wan3_run_mtu,wan3_sta,wan4_ppp_get_ip,wan4_pptp_dhcp,wan4_pptp_server_ip,wan4_ipaddr_buf,wan4_gateway,wan4_gateway_get,wan4_get_domain,wan4_hwaddr,wan4_ipaddr,wan4_netmask,wan4_proto,wan4_run_mtu,wan4_sta,mwan_num,wan_modem_type,wan2_modem_type,wan3_modem_type,wan4_modem_type,wan_hilink_ip,wan2_hilink_ip,wan3_hilink_ip,wan4_hilink_ip,wan_status_script,wan2_status_script,wan3_status_script,wan4_status_script,mwan_cktime,wan_weight,wan2_weight,wan3_weight,wan4_weight,wan_ck_pause,wan2_ck_pause,wan3_ck_pause,wan4_ck_pause,dnscrypt_proxy,dnscrypt_priority,stubby_proxy,stubby_priority"); %>
//	<% nvstat(); %>
//	<% etherstates(); %>
//	<% anonupdate(); %>
//	<% sysinfo(); %>
//	<% wlstats(1); %>

function setColor(n) {
	if (n >= 80)
		return '#ff6f59';
	else if (n >= 60)
		return '#f9c05c';
	else
		return '#6fbf3d';
}

stats = { };

var a, b, i;

stats.anon_enable = nvram.tomatoanon_enable;
stats.anon_answer = nvram.tomatoanon_answer;

stats.lan_desc = nvram.lan_desc;

if (typeof(last_wan_proto) == 'undefined')
	last_wan_proto = nvram.wan_proto;
else if (last_wan_proto != nvram.wan_proto)
	reloadPage();

stats.flashsize = sysinfo.flashsize+' MB';
stats.cpumhz = sysinfo.cpuclk+'-core)';
stats.cputemp = sysinfo.cputemp+'°';
stats.systemtype = sysinfo.systemtype;
stats.cfeversion = sysinfo.cfeversion;
stats.cpuload = ((sysinfo.loads[0] / 65536.0).toFixed(2)+'<small> / </small> '+(sysinfo.loads[1] / 65536.0).toFixed(2)+'<small> / </small>'+(sysinfo.loads[2] / 65536.0).toFixed(2));
stats.uptime = sysinfo.uptime_s;
stats.freqcpu = nvram.clkfreq;

var total_jiffies = 0;
var jiffylist = sysinfo.jiffies.split(' ');
for (i = 0; i < jiffylist.length; ++i)
	total_jiffies += parseInt(jiffylist[i]);

var diff_idle = jiffylist[3] - lastjiffiesidle;
var diff_total = total_jiffies - lastjiffiestotal;
lastjiffiesusage = (1000*(diff_total-diff_idle)/diff_total)/10;

lastjiffiestotal = total_jiffies;
lastjiffiesidle = jiffylist[3];

stats.cpupercent = lastjiffiesusage.toFixed(2)+'%<div class="progress-wrapper"><div class="progress-container"><div class="progress-bar" style="background-color:'+setColor(lastjiffiesusage.toFixed(2))+';width:'+lastjiffiesusage.toFixed(2)+'%"></div></div></div>';
stats.wlsense = sysinfo.wlsense;

a = sysinfo.totalram;
b = sysinfo.totalfreeram;
stats.memory = scaleSize(a - b)+' / '+scaleSize(a)+' <small>('+((a - b) / a * 100.0).toFixed(2)+'%)</small><div class="progress-wrapper"><div class="progress-container"><div class="progress-bar" style="background-color:'+setColor(((a - b) / a * 100.0).toFixed(2))+';width:'+((a - b) / a * 100.0).toFixed(2)+'%"></div></div></div>';
if (sysinfo.totalswap > 0) {
	a = sysinfo.totalswap;
	b = sysinfo.freeswap;
	stats.swap = scaleSize(a - b)+' / '+scaleSize(a)+' <small>('+((a - b) / a * 100.0).toFixed(2)+'%)</small><div class="progress-wrapper"><div class="progress-container"><div class="progress-bar" style="background-color:'+setColor(((a - b) / a * 100.0).toFixed(2))+';width:'+((a - b) / a * 100.0).toFixed(2)+'%"></div></div></div>';
} else
	stats.swap = '';

stats.time = '<% time(); %>';

/* DUALWAN-BEGIN */
stats.wanup = [<% wanup("wan"); %>,<% wanup("wan2"); %>];
stats.wanuptime = ['<% link_uptime("wan"); %>','<% link_uptime("wan2"); %>'];
stats.wanlease = ['<% dhcpc_time("wan"); %>','<% dhcpc_time("wan2"); %>'];
stats.dns = [<% dns("wan"); %>,<% dns("wan2"); %>];
/* DUALWAN-END */
/* MULTIWAN-BEGIN */
stats.wanup = [<% wanup("wan"); %>,<% wanup("wan2"); %>,<% wanup("wan3"); %>,<% wanup("wan4"); %>];
stats.wanuptime = ['<% link_uptime("wan"); %>','<% link_uptime("wan2"); %>','<% link_uptime("wan3"); %>','<% link_uptime("wan4"); %>'];
stats.wanlease = ['<% dhcpc_time("wan"); %>','<% dhcpc_time("wan2"); %>','<% dhcpc_time("wan3"); %>','<% dhcpc_time("wan4"); %>'];
stats.dns = [<% dns("wan"); %>,<% dns("wan2"); %>,<% dns("wan3"); %>,<% dns("wan4"); %>];
/* MULTIWAN-END */

var dns = [];
if (nvram.dnscrypt_proxy == 1 || nvram.stubby_proxy == 1) {
	for (i = 0; i < stats.dns.length; ++i) {
		dns[i] = '<a href="advanced-dhcpdns.asp">Using '+(nvram.dnscrypt_proxy == 1 ? 'dnscrypt-proxy' : 'Stubby')+' resolvers</a>';
		if ((nvram.dnscrypt_proxy == 1 && nvram.dnscrypt_priority != 2) || (nvram.stubby_proxy == 1 && nvram.stubby_priority != 2))
			stats.dns[i] = dns[i]+' and: '+stats.dns[i];
		else
			stats.dns[i] = dns[i];
	}
}

stats.wanip = [];
stats.wannetmask = [];
stats.wangateway = [];
stats.wanweight = [];
stats.wanck_pause = [];

for (var uidx = 1; uidx <= nvram.mwan_num; ++uidx) {
	var u = (uidx > 1) ? uidx : '';
	stats.wanip[uidx - 1] = nvram['wan'+u+'_ipaddr'];
	stats.wannetmask[uidx - 1] = nvram['wan'+u+'_netmask'];
	stats.wangateway[uidx - 1] = nvram['wan'+u+'_gateway_get'];
	if (stats.wangateway[uidx - 1] == '0.0.0.0' || stats.wangateway[uidx - 1] == '')
		stats.wangateway[uidx - 1] = nvram['wan'+u+'_gateway'];

	switch (nvram['wan'+u+'_proto']) {
	case 'pptp':
	case 'l2tp':
	case 'pppoe':
		if (stats.wanup[uidx - 1]) {
			stats.wanip[uidx - 1] = nvram['wan'+u+'_ppp_get_ip'];
			if (nvram['wan'+u+'_pptp_dhcp'] == '1') {
				if (nvram['wan'+u+'_ipaddr'] != '' && nvram['wan'+u+'_ipaddr'] != '0.0.0.0' && nvram['wan'+u+'_ipaddr'] != stats.wanip[uidx - 1])
					stats.wanip[uidx - 1] += '&nbsp;&nbsp;<small>(DHCP: '+nvram['wan'+u+'_ipaddr']+')</small>';
				if (nvram['wan'+u+'_gateway'] != '' && nvram['wan'+u+'_gateway']  != '0.0.0.0' && nvram['wan'+u+'_gateway']  != stats.wangateway[uidx - 1])
					stats.wangateway[uidx - 1] += '&nbsp;&nbsp;<small>(DHCP: '+nvram['wan'+u+'_gateway']+')</small>';
			}
			if (stats.wannetmask[uidx - 1] == '0.0.0.0')
				stats.wannetmask[uidx - 1] = '255.255.255.255';
		}
		else if (nvram['wan'+u+'_proto'] == 'pptp')
				stats.wangateway[uidx - 1] = nvram['wan'+u+'_pptp_server_ip'];
		break;
	default:
		if (!stats.wanup[uidx - 1]) {
			stats.wanip[uidx - 1] = '0.0.0.0';
			stats.wannetmask[uidx - 1] = '0.0.0.0';
			stats.wangateway[uidx - 1] = '0.0.0.0';
		}
	}
	stats.wanweight[uidx - 1] = nvram['wan'+u+'_weight'];
	stats.wanck_pause[uidx - 1] = nvram['wan'+u+'_ck_pause'];
}

/* IPV6-BEGIN */
stats.ip6_duid = ((typeof(sysinfo.ip6_duid) != 'undefined') ? sysinfo.ip6_duid : '')+'';
stats.ip6_wan = ((typeof(sysinfo.ip6_wan) != 'undefined') ? sysinfo.ip6_wan : '')+'';
stats.ip6_wan_dns1 = ((typeof(sysinfo.ip6_wan_dns1) != 'undefined') ? sysinfo.ip6_wan_dns1 : '')+'';
stats.ip6_wan_dns2 = ((typeof(sysinfo.ip6_wan_dns2) != 'undefined') ? sysinfo.ip6_wan_dns2 : '')+'';
stats.ip6_lan = ((typeof(sysinfo.ip6_lan) != 'undefined') ? sysinfo.ip6_lan : '')+'';
stats.ip6_lan_ll = ((typeof(sysinfo.ip6_lan_ll) != 'undefined') ? sysinfo.ip6_lan_ll : '')+'';
stats.ip6_lan1 = ((typeof(sysinfo.ip6_lan1) != 'undefined') ? sysinfo.ip6_lan1 : '')+'';
stats.ip6_lan1_ll = ((typeof(sysinfo.ip6_lan1_ll) != 'undefined') ? sysinfo.ip6_lan1_ll : '')+'';
stats.ip6_lan2 = ((typeof(sysinfo.ip6_lan2) != 'undefined') ? sysinfo.ip6_lan2 : '')+'';
stats.ip6_lan2_ll = ((typeof(sysinfo.ip6_lan2_ll) != 'undefined') ? sysinfo.ip6_lan2_ll : '')+'';
stats.ip6_lan3 = ((typeof(sysinfo.ip6_lan3) != 'undefined') ? sysinfo.ip6_lan3 : '')+'';
stats.ip6_lan3_ll = ((typeof(sysinfo.ip6_lan3_ll) != 'undefined') ? sysinfo.ip6_lan3_ll : '')+'';
/* IPV6-END */

/* DUALWAN-BEGIN */
stats.wanstatus = ['<% wanstatus("wan"); %>','<% wanstatus("wan2"); %>'];
/* DUALWAN-END */
/* MULTIWAN-BEGIN */
stats.wanstatus = ['<% wanstatus("wan"); %>','<% wanstatus("wan2"); %>','<% wanstatus("wan3"); %>','<% wanstatus("wan4"); %>'];
/* MULTIWAN-END */

for (var uidx = 1; uidx <= nvram.mwan_num; ++uidx)
	if (stats.wanstatus[uidx - 1] != 'Connected')
		stats.wanstatus[uidx - 1] = '<b>'+stats.wanstatus[uidx - 1]+'</b>';

stats.channel = [];
stats.interference = [];
stats.qual = [];

for (var uidx = 0; uidx < wl_ifaces.length; ++uidx) {
	a = i = wlstats[uidx].channel * 1;
	if (i < 0)
		i = -i;

	stats.channel.push('<a href="tools-survey.asp">'+((i) ? i+'' : 'Auto')+((wlstats[uidx].mhz) ? ' - '+(wlstats[uidx].mhz / 1000.0).toFixed(3)+' <small>GHz</small>' : '')+'</a>'+((a < 0) ? ' <small>(scanning...)</small>' : ''));
	stats.interference.push((wlstats[uidx].intf >= 0) ? ((wlstats[uidx].intf) ? 'Severe' : 'Acceptable') : '');

	a = wlstats[uidx].nbw * 1;
	wlstats[uidx].nbw = (a > 0) ? (a+' <small>MHz</small>') : 'Auto';

	if (wlstats[uidx].radio) {
		a = wlstats[uidx].rate * 1;
		if (a > 0)
			wlstats[uidx].rate = Math.floor(a / 2) + ((a & 1) ? '.5' : '')+' <small>Mbps</small>';
		else
			wlstats[uidx].rate = '-';

		if (wlstats[uidx].client) {
			/* use signal quality instead of SNR (the same like on Device List page */
			b = wlstats[uidx].rssi * 1;
			if (b > 0) {
				if (b >= -50)
					a = 100;
				else if (b >= -80) /* between -50 ~ -80dbm */
					a = Math.round(24 + ((b + 80) * 26)/10);
				else if (b >= -90) /* between -80 ~ -90dbm */
					a = Math.round(24 + ((b + 90) * 26)/10);
				else
					a = 0;
			}
			else
				a = -1;

			stats.qual.push(a < 0 ? '' : a+' <img src="bar'+MIN(MAX(Math.floor(a / 12), 1), 6)+'.gif" alt="">');
		}
		else
			stats.qual.push('');

		wlstats[uidx].noise += ' <small>dBm</small>';
		wlstats[uidx].rssi += ' <small>dBm</small>';
	}
	else {
		wlstats[uidx].rate = '';
		wlstats[uidx].noise = '';
		wlstats[uidx].rssi = '';
		stats.qual.push('');
	}

	if (wl_ifaces[uidx][6] != 1)
		wlstats[uidx].ifstatus = '<b>Down</b>';
	else {
		wlstats[uidx].ifstatus = 'Up';
		for (i = 0; i < xifs[0].length ; ++i) {
			if ((nvram[xifs[0][i]+'_ifnames']).indexOf(wl_ifaces[uidx][0]) >= 0) {
				wlstats[uidx].ifstatus = wlstats[uidx].ifstatus+' ('+xifs[1][i]+')';
				break;
			}
		}
	}
}
