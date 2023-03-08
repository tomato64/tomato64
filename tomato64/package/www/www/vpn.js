/* List of available ciphers */
var vpnciphers = [];
/* AES-BEGIN */
vpnciphers = vpnciphers.concat(['CHACHA20-POLY1305','AES-128-CBC','AES-128-CFB','AES-128-OFB','AES-192-CBC','AES-192-CFB','AES-192-OFB','AES-256-CBC','AES-256-CFB','AES-256-OFB','AES-128-GCM','AES-192-GCM','AES-256-GCM']);
/* AES-END */
vpnciphers = vpnciphers.concat(['BF-CBC','BF-CFB','BF-OFB','CAST5-CBC','CAST5-CFB','CAST5-OFB','DES-CBC','DES-CFB','DES-EDE3-CBC','DES-EDE3-CFB','DES-EDE3-OFB','DES-EDE-CBC','DES-EDE-CFB','DES-EDE-OFB','DES-OFB','DESX-CBC','IDEA-CBC','IDEA-CFB','IDEA-OFB']);

var vpndigests = ['MD5','RIPEMD160','RSA-MD4','SHA1','SHA224','SHA256','SHA384','SHA512'];

var helpURL = new Array();
helpURL['howto'] = 'https://openvpn.net/community-resources/1xhowto/';
helpURL['staticKeys'] = 'https://openvpn.net/community-resources/static-key-mini-howto/';
helpURL['TLSKeys'] = 'https://openvpn.net/community-resources/how-to/';

/* Handles parsing status files and displaying the information */
function StatusUpdater(clients, routes, stats, time, cDiv, ncDiv, eDiv) {
	this.init(clients, routes, stats, time, cDiv, ncDiv, eDiv);
	return this;
}
StatusUpdater.prototype = {
	init: function(clients, routes, stats, time, cDiv, ncDiv, eDiv) {
		this.clientTable = clients? new TomatoGrid(clients,'sort',0,null): null;
		this.routeTable = routes? new TomatoGrid(routes,'sort',0,null): null;
		this.statTable = stats? new TomatoGrid(stats,'sort',0,null): null;
		this.statusTime = time? E(time): null;
		this.content = cDiv? E(cDiv): null;
		this.noContent = ncDiv? E(ncDiv): null;
		this.errors = eDiv? E(eDiv): null;
	},
	update: function(text) {
		if (this.errors) this.errors.innerHTML = '';
		if (this.noContent) this.noContent.style.display = (text == '' ? '' : 'none');
		if (this.content) this.content.style.display = (text == '' ? 'none' : '');

		if (this.clientTable) this.clientTable.tb.parentNode.style.display = 'none';
		if (this.clientTable) this.clientTable.removeAllData();
		if (this.routeTable) this.routeTable.tb.parentNode.style.display = 'none';
		if (this.routeTable) this.routeTable.removeAllData();
		if (this.statTable) this.statTable.tb.parentNode.style.display = 'none';
		if (this.statTable) this.statTable.removeAllData();

		if (this.statTable) this.statTable.headerSet(['Name','Value']);

		var lines = text.split('\n');
		var staticStats = false;
		for (i = 0; text != '' && i < lines.length; ++i) {
			var done = false;

			var fields = lines[i].split(',');
			if (fields.length == 0) continue;
			switch (fields[0]) {
				case "TITLE":
				break;
				case "TIME":
					if (this.statusTime) this.statusTime.innerHTML = fields[1];
				break;
				case "HEADER":
					switch (fields[1]) {
						case "CLIENT_LIST":
							if (this.clientTable) this.clientTable.headerSet(fields.slice(2,fields.length-1));
						break;
						case "ROUTING_TABLE":
							if (this.routeTable) this.routeTable.headerSet(fields.slice(2,fields.length-1));
						break;
						default:
							if (this.errors) this.errors.innerHTML += 'Unknown header: '+lines[i]+'<br>';
						break;
					}
				break;
				case "CLIENT_LIST":
					if (this.clientTable) this.clientTable.tb.parentNode.style.display = '';
					if (this.clientTable) this.clientTable.insertData(-1, fields.slice(1,fields.length-1));
				break;
				case "ROUTING_TABLE":
					if (this.routeTable) this.routeTable.tb.parentNode.style.display = '';
					if (this.routeTable) this.routeTable.insertData(-1, fields.slice(1,fields.length-1));
				break;
				case "GLOBAL_STATS":
					if (this.statTable) this.statTable.tb.parentNode.style.display = '';
					if (this.statTable) this.statTable.insertData(-1, fields.slice(1));
				break;
				case "OpenVPN STATISTICS":
					staticStats = true;
				break;
				case "Updated":
					if (staticStats)
						if (this.statusTime) this.statusTime.innerHTML = fields[1];
				break;
				case "END":
					done = true;
				break;
				default:
					if (staticStats) {
						if (this.statTable) this.statTable.tb.parentNode.style.display = '';
						if (this.statTable) this.statTable.insertData(-1, fields);
					}
					else if (this.errors) this.errors.innerHTML += 'Unknown: '+lines[i]+'<br>';
				break;
			}
		if (done) break;
		}
	}
}

function show() {
	countButton += 1;
	for (var i = 1; i <= unitCount; ++i) {
		var e = E('_'+serviceType+i+'_button');
		var d = eval('isup.'+serviceType+i);

		e.value = (d ? 'Stop' : 'Start')+' Now';
		e.setAttribute('onclick', 'javascript:toggle(\''+serviceType+''+i+'\','+d+');');
		if (serviceLastUp[i - 1] != d || countButton > 6) {
			serviceLastUp[i - 1] = d;
			countButton = 0;
			e.disabled = 0;
			E('spin'+i).style.display = 'none';
		}

		if (d) updateStatus(i - 1);
	}
}

function toggle(service, isup) {
	if (changed && !confirm('There are unsaved changes. Continue anyway?'))
		return;

	serviceLastUp[id - 1] = isup;
	countButton = 0;

	var id = service.substr(service.length - 1);
	E('_'+service+'_button').disabled = 1;
	E('spin'+id).style.display = 'inline';

	var fom = E('t_fom');
	var bup = fom._service.value;
	fom._service.value = service+(isup ? '-stop' : '-start');
	fom._nofootermsg.value = 1;

	form.submit(fom, 1, 'service.cgi');
	fom._service.value = bup;
}
