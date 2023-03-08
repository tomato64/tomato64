function show() {
	var up = eval('isup.'+serviceType);
	var e = E('_'+serviceType+'_button');

	if (E('_'+serviceType+'_notice')) E('_'+serviceType+'_notice').innerHTML = serviceType+' is currently '+(up ? 'running ' : 'stopped')+'&nbsp;';
	if (E('_'+serviceType+'_interface')) E('_'+serviceType+'_interface').disabled = (up ? 0 : 1);
	if (E('_'+serviceType+'_status')) E('_'+serviceType+'_status').disabled = (up ? 0 : 1);

	e.value = (up ? 'Stop' : 'Start')+' Now';
	e.setAttribute('onclick', 'javascript:toggle(\''+serviceType+'\','+up+');');
	countButton += 1;

	if (serviceLastUp[0] != up || countButton > 6) {
		serviceLastUp[0] = up;
		countButton = 0;
		e.disabled = 0;
		E('spin').style.display = 'none';
	}

	var fom = E('t_fom');
	if (up && changed) /* up and config changed? force restart on save */
		fom._service.value = (serviceType == 'pptpd' ? 'firewall-restart,'+serviceType+'-restart,dnsmasq-restart' :
		                      serviceType == 'ftpd' ? 'firewall-restart,'+serviceType+'-restart' :
		                      serviceType+'-restart');
	else
		fom._service.value = '';
}

function toggle(service, isup) {
	if (typeof save_pre === 'function') { if (!save_pre()) return; }
	if (typeof reinit === 'undefined') reinit = 0;
	if (changed && !reinit) alert('Configuration changes detected - will be saved');
	else if (service == 'ftpd' && !isup && E('_ftp_enable').value == 0) alert('Ftpd will be started on LAN only');

	E('_'+service+'_button').disabled = 1;
	if (E('_'+service+'_interface')) E('_'+service+'_interface').disabled = 1;
	if (E('_'+service+'_status')) E('_'+service+'_status').disabled = 1;
	E('spin').style.display = 'inline';

	serviceLastUp[0] = isup;
	countButton = 0;

	var fom = E('t_fom');
	fom._service.value = (service == 'pptpd' ? 'firewall-restart,'+service+(isup ? '-stop' : '-start')+',dnsmasq-restart' :
	                      service == 'ftpd' ? 'firewall-restart,'+service+(isup ? '-stop' : '-start') :
	                      service+(isup ? '-stop' : '-start'));

	save(1);
}
