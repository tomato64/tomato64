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
<title>[<% ident(); %>] Advanced: Wireless</title>
<link rel="stylesheet" type="text/css" href="tomato.css?rel=<% version(); %>">
<% css(); %>
<script src="tomato.js?rel=<% version(); %>"></script>
<script src="wireless.jsx?_http_id=<% nv(http_id); %>"></script>
<script>

//	<% nvram("t_model_name,wl_security_mode,wl_auth,wl_bcn,wl_dtim,wl_frag,wl_frameburst,wl_gmode_protection,wl_plcphdr,wl_rate,wl_rateset,wl_rts,wl_wme,wl_wme_no_ack,wl_wme_apsd,wl_txpwr,wl_mrate,t_features,wl_distance,wl_psta_inact,wl_mode,wl_maxassoc,wl_bss_maxassoc,wlx_hpamp,wlx_hperx,wl_reg_mode,wl_country_code,0:ccode,1:ccode,2:ccode,pci/1/1/ccode,pci/2/1/ccode,pci/3/1/ccode,wl_country_rev,0:regrev,1:regrev,2:regrev,pci/1/1/regrev,pci/2/1/regrev,pci/3/1/regrev,wl_btc_mode,wl_mimo_preamble,wl_obss_coex,wl_mitigation,wl_mitigation_ac,wl_nband,wl_phytype,wl_corerev,wl_igs,wl_wmf_bss_enable,wl_wmf_ucigmp_query,wl_wmf_mdata_sendup,wl_wmf_ucast_upnp,wl_wmf_igmpq_filter,wl_optimizexbox,wl_atf,wl_turbo_qam,wl_txbf,wl_txbf_bfr_cap,wl_txbf_bfe_cap,wl_itxbf,wl_txbf_imp,wl_mumimo,wl_mu_features,wl_mfp,wl_user_rssi"); %>

//	<% wlcountries(); %>

hp = features('hpamp');
nphy = features('11n');
acwave2 = features('11acwave2'); /* for MU-MIMO (and NITRO QAM support 2,4 GHz & 5 GHz) SDK7.14 */
var cprefix = 'advanced_wireless';

function verifyFields(focused, quiet) {
	for (var uidx = 0; uidx < wl_ifaces.length; ++uidx) {
		if (wl_sunit(uidx) < 0) {
			var u = wl_unit(uidx);
			var sm = nvram['wl'+u+'_security_mode'];

			if (!v_range('_f_wl'+u+'_distance', quiet, 0, 99999)) return 0;
			if (!v_range('_wl'+u+'_maxassoc', quiet, 0, 255)) return 0;
			if (!v_range('_wl'+u+'_bcn', quiet, 1, 65535)) return 0;
			if (!v_range('_wl'+u+'_dtim', quiet, 1, 255)) return 0;
			if (!v_range('_wl'+u+'_frag', quiet, 256, 2346)) return 0;
			if (!v_range('_wl'+u+'_rts', quiet, 0, 2347)) return 0;
			if (!v_range('_wl'+u+'_country_rev', quiet, 0, 999)) return 0;
			if ((E('_wl'+u+'_txpwr').value != 0) && !v_range(E('_wl'+u+'_txpwr'), quiet, 5, hp ? 251 : 1000)) return 0;
			if ((E('_wl'+u+'_psta_inact').value != 0) && !v_range(E('_wl'+u+'_psta_inact'), quiet, 60, 3600)) return 0;
/* ROAM-BEGIN */
			if ((E('_wl'+u+'_user_rssi').value != 0) && !v_range(E('_wl'+u+'_user_rssi'), quiet, -90, -45)) return 0;
/* ROAM-END */

			/* for Protected Management Frames - only allow/possible for wpa2 */
			if ((E('_wl'+u+'_mfp').value != 0) && (sm != "wpa2_personal") && (sm != "wpa2_enterprise")) {
				ferror.set('_wl'+u+'_mfp', 'Protected Management Frames only possible for WPA2', quiet);
				return 0;
			}
			else
				ferror.clear('_wl'+u+'_mfp');

			var b = E('_wl'+u+'_wme').value == 'off';
			E('_wl'+u+'_wme_no_ack').disabled = b;
			E('_wl'+u+'_wme_apsd').disabled = b;
		}
	}

	return 1;
}

function save() {
	var fom;
	var n;
	var router_reboot = 0;

	if (!verifyFields(null, false)) return;

	fom = E('t_fom');

	for (var uidx = 0; uidx < wl_ifaces.length; ++uidx) {
		if (wl_sunit(uidx) < 0) {
			var u = wl_unit(uidx);
			var u_pci = (u+1);
			var c_code = E('_wl'+u+'_country_code').value;
			var c_rev = E('_wl'+u+'_country_rev').value;

			n = E('_f_wl'+u+'_distance').value * 1;
			E('_wl'+u+'_distance').value = n ? n : '';

			/* check if wireless country settings will be changed */
			if (nvram['wl'+u+'_country_code'] != c_code || nvram['wl'+u+'_country_rev'] != c_rev)
				router_reboot = 1;

			if (nvram[+u+':ccode'].length > 0) /* check short version */
 				E('_'+u+':ccode').value = c_code;

			if (nvram['pci/'+u_pci+'/1/ccode'].length > 0) /* check long version */
				E('_pci/'+u_pci+'/1/ccode').value = c_code;

			if (nvram[+u+':regrev'].length > 0)
 				E('_'+u+':regrev').value = c_rev;

			if (nvram['pci/'+u_pci+'/1/regrev'].length > 0)
				E('_pci/'+u_pci+'/1/regrev').value = c_rev;

			E('_wl'+u+'_nmode_protection').value = E('_wl'+u+'_gmode_protection').value;

			/* for Explicit TX Beamforming */
			E('_wl'+u+'_txbf_bfr_cap').value = E('_wl'+u+'_txbf').value; /* turn on (1)/off (0) with wl_txbf */
			E('_wl'+u+'_txbf_bfe_cap').value = E('_wl'+u+'_txbf').value;

/* BCMWL714-BEGIN */
			/* MU-MIMO SDK7.14 */
			if ((E('_wl'+u+'_txbf').value == 1) && (acwave2 == 1)) {
				if (E('_wl'+u+'_mumimo').value == 1) {
					E('_wl'+u+'_txbf_bfr_cap').value = 2; /* MU-MIMO (2) on */
					E('_wl'+u+'_txbf_bfe_cap').value = 2; /* MU-MIMO (2) on */
					E('_wl'+u+'_mu_features').value = '0x8000'; /* mu_feature on */
				}
				else { /* disabled */
					E('_wl'+u+'_txbf_bfr_cap').value = 1; /* MU-MIMO (1) off */
					E('_wl'+u+'_txbf_bfe_cap').value = 1; /* MU-MIMO (1) off */
					E('_wl'+u+'_mu_features').value = '0'; /* mu_feature off */
				}
			}
			else { /* default */
				E('_wl'+u+'_mu_features').value = '0'; /* mu_feature off */
			}
/* BCMWL714-END */

			/* for Implicit TX Beamforming */
			E('_wl'+u+'_txbf_imp').value = E('_wl'+u+'_itxbf').value; /* turn on (1)/off (0) with wl_itxbf */

			/* Set bss_maxassoc same as global */
			E('_wl'+u+'_bss_maxassoc').value = E('_wl'+u+'_maxassoc').value; /* keep it easy and sync value to global max clients */

/* EMF-BEGIN */
			/* for Wireless IGMP Snooping */
			var wb_enable = E('_wl'+u+'_wmf_bss_enable').value;
			E('_wl'+u+'_igs').value = wb_enable; /* turn on (1)/off (0) with wl_wmf_bss_enable */
			E('_wl'+u+'_wmf_ucigmp_query').value = wb_enable;
			E('_wl'+u+'_wmf_mdata_sendup').value = wb_enable;
			E('_wl'+u+'_wmf_ucast_upnp').value = wb_enable;
			E('_wl'+u+'_wmf_igmpq_filter').value = wb_enable;
/* EMF-END */
		}
	}

	if (hp) {
		if ((E('_wlx_hpamp').value != nvram.wlx_hpamp) || (E('_wlx_hperx').value != nvram.wlx_hperx)) {
			fom._service.disabled = 1;
			fom._reboot.value = 1;
			form.submit(fom, 0);
			return;
		}
	}
	else {
		E('_wlx_hpamp').disabled = 1;
		E('_wlx_hperx').disabled = 1;
	}

	/* check wireless country changed ? */
	if (router_reboot && confirm("Router must be rebooted to apply changed country settings. Reboot now? (and commit changes to NVRAM)")) {
		fom._service.disabled = 1;
		fom._reboot.value = 1;
		form.submit(fom, 0);
	}
 	else { /* countinue without reboot (if no country change or the user wants it that way) */
		form.submit(fom, 1);
	}
}

function init() {
	if (((c = cookie.get(cprefix + '_notes_vis')) != null) && (c == '1')) {
		toggleVisibility(cprefix, 'notes');
	}
	eventHandler();
}
</script>
</head>

<body onload="init()">
<form id="t_fom" method="post" action="tomato.cgi">
<table id="container">
<tr><td colspan="2" id="header">
	<div class="title"><a href="/">Tomato64</a></div>
	<div class="version">Version <% version(); %> on <% nv("t_model_name"); %><span class="blinking bl2"><script><% anonupdate(); %> anon_update()</script>&nbsp;</span></div>
</td></tr>
<tr id="body"><td id="navi"><script>navi()</script></td>
<td id="content">
<div id="ident"><% ident(); %> | <script>wikiLink();</script></div>

<!-- / / / -->

<input type="hidden" name="_nextpage" value="advanced-wireless.asp">
<input type="hidden" name="_nextwait" value="10">
<input type="hidden" name="_service" value="*">
<input type="hidden" name="_reboot" value="0">

<!-- / / / -->

<script>
	for (var uidx = 0; uidx < wl_ifaces.length; ++uidx) {
		if (wl_sunit(uidx) < 0) {
			var u = wl_unit(uidx);
			var u_pci = (u+1);

			W('<input type="hidden" id="_wl'+u+'_distance" name="wl'+u+'_distance">');
			if (nvram[+u+':ccode'])
				W('<input type="hidden" id="_'+u+':ccode" name="'+u+':ccode">');
			if (nvram['pci/'+u_pci+'/1/ccode'])
				W('<input type="hidden" id="_pci/'+u_pci+'/1/ccode" name="pci/'+u_pci+'/1/ccode">');
			if (nvram[+u+':regrev'])
				W('<input type="hidden" id="_'+u+':regrev" name="'+u+':regrev">');
			if (nvram['pci/'+u_pci+'/1/regrev'])
				W('<input type="hidden" id="_pci/'+u_pci+'/1/regrev" name="pci/'+u_pci+'/1/regrev">');
			W('<input type="hidden" id="_wl'+u+'_nmode_protection" name="wl'+u+'_nmode_protection">');
			W('<input type="hidden" id="_wl'+u+'_txbf_bfr_cap" name="wl'+u+'_txbf_bfr_cap">');
			W('<input type="hidden" id="_wl'+u+'_txbf_bfe_cap" name="wl'+u+'_txbf_bfe_cap">');
			W('<input type="hidden" id="_wl'+u+'_txbf_imp" name="wl'+u+'_txbf_imp">');
/* BCMWL714-BEGIN */
			W('<input type="hidden" id="_wl'+u+'_mu_features" name="wl'+u+'_mu_features">');
/* BCMWL714-END */		    
			W('<input type="hidden" id="_wl'+u+'_bss_maxassoc" name="wl'+u+'_bss_maxassoc">');
/* EMF-BEGIN */
			W('<input type="hidden" id="_wl'+u+'_igs" name="wl'+u+'_igs">');
			W('<input type="hidden" id="_wl'+u+'_wmf_ucigmp_query" name="wl'+u+'_wmf_ucigmp_query">');
			W('<input type="hidden" id="_wl'+u+'_wmf_mdata_sendup" name="wl'+u+'_wmf_mdata_sendup">');
			W('<input type="hidden" id="_wl'+u+'_wmf_ucast_upnp" name="wl'+u+'_wmf_ucast_upnp">');
			W('<input type="hidden" id="_wl'+u+'_wmf_igmpq_filter" name="wl'+u+'_wmf_igmpq_filter">');
/* EMF-END */

/* / / / */

			W('<div class="section-title">Wireless settings for: '+wl_display_ifname(uidx)+'<\/div>');
			W('<div class="section">');

			at = ((nvram['wl'+u+'_security_mode'] != "wep") && (nvram['wl'+u+'_security_mode'] != "radius") && (nvram['wl'+u+'_security_mode'] != "disabled"));
			createFieldTable('', [
				{ title: 'Authentication Type', name: 'wl'+u+'_auth', type: 'select',
					options: [['0','Auto *'],['1','Shared Key']], attrib: at ? 'disabled' : '',
					value: at ? 0 : nvram['wl'+u+'_auth'] },
				{ title: 'Protected Management Frames', name: 'wl'+u+'_mfp', type: 'select', options: [['0','Disable *'],['1','Capable'],['2','Required']],
					value: nvram['wl'+u+'_mfp'] },
				{ title: 'Basic Rate', name: 'wl'+u+'_rateset', type: 'select', options: [['default','Default *'],['12','1-2 Mbps'],['all','All']],
					value: nvram['wl'+u+'_rateset'] },
				{ title: 'Beacon Interval', name: 'wl'+u+'_bcn', type: 'text', maxlen: 5, size: 7,
					suffix: ' <small>(range: 1 - 65535; default: 100)<\/small>', value: nvram['wl'+u+'_bcn'] },
				{ title: 'CTS Protection Mode', name: 'wl'+u+'_gmode_protection', type: 'select', options: [['off','Disable *'],['auto','Auto']],
					value: nvram['wl'+u+'_gmode_protection'] },
				{ title: 'Regulatory Mode', name: 'wl'+u+'_reg_mode', type: 'select',
					options: [['off', 'Off *'],['d', '802.11d'],['h', '802.11h']],
					value: nvram['wl'+u+'_reg_mode'] },
				{ title: 'Country / Region', name: 'wl'+u+'_country_code', type: 'select',
					options: wl_countries, value: nvram['wl'+u+'_country_code'] },
				{ title: 'Country Rev', name: 'wl'+u+'_country_rev', type: 'text', maxlen: 3, size: 7,
					suffix: ' <small>(range: 0 - 999)<\/small>', value: nvram['wl'+u+'_country_rev'] },
				{ title: 'Bluetooth Coexistence', name: 'wl'+u+'_btc_mode', type: 'select',
					options: [['0', 'Disable *'],['1', 'Enable'],['2', 'Preemption']],
					value: nvram['wl'+u+'_btc_mode'], hidden: (nvram['wl'+u+'_nband'] == 1) },
				{ title: 'Distance / ACK Timing', name: 'f_wl'+u+'_distance', type: 'text', maxlen: 5, size: 7,
					suffix: ' <small>meters<\/small>&nbsp;&nbsp;<small>(range: 0 - 99999; 0 = use default)<\/small>',
						value: (nvram['wl'+u+'_distance'] == '') ? '0' : nvram['wl'+u+'_distance'] },
				{ title: 'Inactivity Timer', name: 'wl'+u+'_psta_inact', type: 'text', maxlen: 4, size: 6,
					suffix: ' <small>(range: 60 - 3600 seconds; default: 0 (disabled))<\/small>', value: nvram['wl'+u+'_psta_inact'], hidden: (nvram['wl'+u+'_mode'] != 'psta') },
/* ROAM-BEGIN */
				{ title: 'Roaming Assistant', name: 'wl'+u+'_user_rssi', type: 'text', maxlen: 3, size: 5,
					suffix: ' <small>(range: -90 ~ -45 (RSSI-Value); default: 0 (disabled))<\/small>', value: nvram['wl'+u+'_user_rssi'] },
/* ROAM-END */
				{ title: 'DTIM Interval', name: 'wl'+u+'_dtim', type: 'text', maxlen: 3, size: 5,
					suffix: ' <small>(range: 1 - 255; default: 1)<\/small>', value: nvram['wl'+u+'_dtim'] },
				{ title: 'Fragmentation Threshold', name: 'wl'+u+'_frag', type: 'text', maxlen: 4, size: 6,
					suffix: ' <small>(range: 256 - 2346; default: 2346)<\/small>', value: nvram['wl'+u+'_frag'] },
				{ title: 'Frame Burst', name: 'wl'+u+'_frameburst', type: 'select', options: [['off','Disable *'],['on','Enable']],
					value: nvram['wl'+u+'_frameburst'] },
				{ title: 'HP', hidden: !hp || (uidx > 0) },
					{ title: 'Amplifier', indent: 2, name: 'wlx_hpamp' + (uidx > 0 ? uidx + '' : ''), type: 'select', options: [['0','Disable'],['1','Enable *']],
						value: nvram.wlx_hpamp != '0', hidden: !hp || (uidx > 0) },
					{ title: 'Enhanced RX Sensitivity', indent: 2, name: 'wlx_hperx' + (uidx > 0 ? uidx + '' : ''), type: 'select', options: [['0','Disable *'],['1','Enable']],
						value: nvram.wlx_hperx != '0', hidden: !hp || (uidx > 0) },
				{ title: 'Maximum Clients', name: 'wl'+u+'_maxassoc', type: 'text', maxlen: 3, size: 5,
					suffix: ' <small>(range: 1 - 255; default: 128)<\/small>', value: nvram['wl'+u+'_maxassoc'] },
				{ title: 'Multicast Rate', name: 'wl'+u+'_mrate', type: 'select',
					options: [['0','Auto *'],['1000000','1 Mbps'],['2000000','2 Mbps'],['5500000','5.5 Mbps'],['6000000','6 Mbps'],['9000000','9 Mbps'],['11000000','11 Mbps'],['12000000','12 Mbps'],['18000000','18 Mbps'],['24000000','24 Mbps'],['36000000','36 Mbps'],['48000000','48 Mbps'],['54000000','54 Mbps']],
					value: nvram['wl'+u+'_mrate'] },
				{ title: 'Preamble', name: 'wl'+u+'_plcphdr', type: 'select', options: [['long','Long *'],['short','Short']],
					value: nvram['wl'+u+'_plcphdr'] },
				{ title: '802.11n Preamble', name: 'wl'+u+'_mimo_preamble', type: 'select', options: [['auto','Auto'],['mm','Mixed Mode *'],['gf','Green Field'],['gfbcm','GF-BRCM']],
					value: nvram['wl'+u+'_mimo_preamble'], hidden: !nphy },
				{ title: 'Overlapping BSS Coexistence', name: 'wl'+u+'_obss_coex', type: 'select', options: [['0','Off *'],['1','On']],
					value: nvram['wl'+u+'_obss_coex'], hidden: !nphy },
				{ title: 'RTS Threshold', name: 'wl'+u+'_rts', type: 'text', maxlen: 4, size: 6,
					suffix: ' <small>(range: 0 - 2347; default: 2347)<\/small>', value: nvram['wl'+u+'_rts'] },
				{ title: 'Transmit Power', name: 'wl'+u+'_txpwr', type: 'text', maxlen: 4, size: 5,
					suffix: hp ?
						' <small>mW (before amplification)<\/small>&nbsp;&nbsp;<small>(range: 5 - 251; default: 10)<\/small>' :
						' <small>mW<\/small>&nbsp;&nbsp;<small>(range: 5 - 1000, override regulatory and other limitations; use 0 for country default)<\/small>',
						value: nvram['wl'+u+'_txpwr'] },
				{ title: 'Transmission Rate', name: 'wl'+u+'_rate', type: 'select',
					options: [['0','Auto *'],['1000000','1 Mbps'],['2000000','2 Mbps'],['5500000','5.5 Mbps'],['6000000','6 Mbps'],['9000000','9 Mbps'],['11000000','11 Mbps'],['12000000','12 Mbps'],['18000000','18 Mbps'],['24000000','24 Mbps'],['36000000','36 Mbps'],['48000000','48 Mbps'],['54000000','54 Mbps']],
					value: nvram['wl'+u+'_rate'] },
				{ title: 'Interference Mitigation', name: 'wl'+u+'_mitigation', type: 'select',
					options: [['0','None *'],['1','Non-WLAN'],['2','WLAN Manual'],['3','WLAN Auto'],['4','WLAN Auto with Noise Reduction']],
					value: nvram['wl'+u+'_mitigation'], hidden: (nvram['wl'+u+'_phytype'] == 'v') },
				{ title: 'AC-PHY Interference Mitigation', name: 'wl'+u+'_mitigation_ac', type: 'select',
					options: [['0','None *'],['1','desense based on glitch count (opt. 1)'],['2','limit pktgain based on hwaci (opt. 2)'],['4','limit pktgain based on w2/nb (opt. 3)'],['3','opt. 1 AND opt. 2'],['5','opt. 1 AND opt. 3'],['6','opt. 2 AND opt. 3'],['7','opt. 1 AND opt. 2 AND opt. 3']],
					value: nvram['wl'+u+'_mitigation_ac'], hidden: (nvram['wl'+u+'_phytype'] != 'v') },
				{ title: 'WMM', name: 'wl'+u+'_wme', type: 'select', options: [['auto','Auto'],['off','Disable'],['on','Enable *']], value: nvram['wl'+u+'_wme'] },
				{ title: 'No ACK', name: 'wl'+u+'_wme_no_ack', indent: 2, type: 'select', options: [['off','Disable *'],['on','Enable']],
					value: nvram['wl'+u+'_wme_no_ack'] },
				{ title: 'APSD Mode', name: 'wl'+u+'_wme_apsd', indent: 2, type: 'select', options: [['off','Disable'],['on','Enable *']],
					value: nvram['wl'+u+'_wme_apsd'] },
				{ title: 'Wireless Multicast Forwarding', name: 'wl'+u+'_wmf_bss_enable', type: 'select', options: [['0','Disable *'],['1','Enable']],
					value: nvram['wl'+u+'_wmf_bss_enable'] },
				{ title: 'Modulation Scheme (Requires Wireless Network Mode set to Auto)', name: 'wl'+u+'_turbo_qam', type: 'select', options: [['0','Up to MCS 7 (802.11n)'],['1','Up to MCS 9 (TurboQAM/256-QAM) *']
/* BCMWL714-BEGIN */
																			,['2','Up to MCS 11 (NitroQAM/1024-QAM)']
/* BCMWL714-END */
																		       ],
					value: nvram['wl'+u+'_turbo_qam'], hidden: (!acwave2 && ((nvram['wl'+u+'_phytype'] != 'v') || (nvram['wl'+u+'_nband'] == 1))) },
				{ title: 'Explicit beamforming', name: 'wl'+u+'_txbf', type: 'select', options: [['0','Disable'],['1','Enable *']],
					value: nvram['wl'+u+'_txbf'], hidden: (nvram['wl'+u+'_corerev'] < 40) },
				{ title: 'Universal/Implicit beamforming', name: 'wl'+u+'_itxbf', type: 'select', options: [['0','Disable'],['1','Enable *']],
					value: nvram['wl'+u+'_itxbf'], hidden: (nvram['wl'+u+'_corerev'] < 40) },
/* BCMWL714-BEGIN */
				{ title: 'MU-MIMO', name: 'wl'+u+'_mumimo', type: 'select', options: [['0','Disable *'],['1','Enable']],
					value: nvram['wl'+u+'_mumimo'] },
/* BCMWL714-END */
				{ title: 'Optimized for Xbox', name: 'wl'+u+'_optimizexbox', type: 'select', options: [['0','Disable *'],['1','Enable']],
					suffix: ' <small>(Default will enable LDPC cap.)<\/small>', value: nvram['wl'+u+'_optimizexbox'], hidden: (nvram['wl'+u+'_phytype'] != 'v') },
				{ title: 'Air Time Fairness', name: 'wl'+u+'_atf', type: 'select', options: [['0','Disable *'],['1','Enable']],
					value: nvram['wl'+u+'_atf'] }
			]);
			W('<\/div>');
		}
	}
</script>

<!-- / / / -->

<div class="section"><small>The default settings are indicated with an asterisk <b style="font-size:1.5em">*</b> symbol.</small></div>

<!-- / / / -->

<div class="section-title">Notes <small><i><a href="javascript:toggleVisibility(cprefix,'notes');" id="toggleLink-notes"><span id="sesdiv_notes_showhide">(Show)</span></a></i></small></div>
<div class="section" id="sesdiv_notes" style="display:none">
	<i>Country / Region and Country Rev EXAMPLES:</i><br>
	<ul>
		<li><b>EU / 13</b> - Country: EU (Europe) AND Country Rev: 13 (Asus default setup for Germany and SDK6 router RT-N18U, RT-AC56U, RT-AC68U C1)</li>
		<li><b>EU / 33</b> - Country: EU (Europe) AND Country Rev: 33 (Asus default setup for Germany and SDK6 router RT-AC66U_B1)</li>
		<li><b>US / 0</b> - Country: US (USA) AND Country Rev: 0 (Asus default setup for USA and SDK6 router RT-AC68U A1/A2)</li>
		<li><b>Q2 / 96</b> - Country: Q2 (USA) AND Country Rev: 96 (Asus default setup for USA and SDK7 router RT-AC3200)</li>
		<li><b>Q2 / 33</b> - Country: Q2 (USA) AND Country Rev: 33 (Asus default setup for USA and SDK6 router RT-AC68U C1)</li>
		<li><b>Q2 / 40</b> - Country: Q2 (USA) AND Country Rev: 40 (Asus default setup for USA and SDK6 router RT-AC68U B1/B2)</li>
		<li><b>Q2 / 61</b> - Country: Q2 (USA) AND Country Rev: 61 (Asus default setup for USA and SDK6 router RT-AC66U_B1 / RT-AC1750_B1)</li>
		<li><b>SG / 12</b> - Country: SG (Singapore) AND Country Rev: 12 (default *)</li>
	</ul>

	<i>Further Notes:</i><br>
	<ul>
		<li>Please select the same country code and rev for all wireless interfaces and have a look at the <a href="https://wiki.freshtomato.org/doku.php/advanced-wireless#country_region" class="new_window">FT Wiki</a></li>
	  	<li>Country code AND rev define the possible channel list, power and other regulations</li>
		<li>Leave default values if you are not sure what you are doing!</li>
		<li>Info: wireless driver supports ~2000 combinations</li>
		<li>Inactivity Timer: Media Bridge Mode will create a virtual interface wlX.Y for every client device connected to the router. Remove virtual interface(s) after inactivity again.</li>
<!-- ROAM-BEGIN -->
		<li>Roaming Assistant: Do not enable wireless bandsteering (BSD) at the same time!</li>
<!-- ROAM-END -->
	</ul>
</div>

<!-- / / / -->

<div id="footer">
	<span id="footer-msg"></span>
	<input type="button" value="Save" id="save-button" onclick="save()">
	<input type="button" value="Cancel" id="cancel-button" onclick="reloadPage();">
</div>

</td></tr>
</table>
</form>
<script>verifyFields(null, true);</script>
</body>
</html>
