var sufGridMsg = '\n\nThis will not be permanent until you click the \'Save\' button at the bottom.';
function getNextpage() {
	var f, i, a;
	f = (window.fields && fields.getAll) ? fields.getAll(document.body) : [];
	for (i = 0; i < f.length; ++i)
		if (f[i].name === '_nextpage' && f[i].value)
			return f[i].value;
	a = asp();
	return a ? a.replace(/[\.\-]/g, '_') : '';
}

function getModelVersion() {
	var m, v;
	m = nvram.t_model_name || 'unknown';
	v = nvram.os_version;
	versionMatch = v.match(/\d{4}\.\d+/);
	v = versionMatch ? versionMatch[0] : v;
	return { model: m.replace(/[^a-zA-Z0-9_\-\.]+/g, '_'), version: v.replace(/[^a-zA-Z0-9_\-\.]+/g, '_') };
}

function findGridFromButton() {
	var button = event.target || event.srcElement;
	var section = button.parentNode;
	var gridContainer = section.querySelector('.tomato-grid');
	if (!gridContainer || !gridContainer.gridObj) { alert('Error: Grid not found in this section. Make sure gridObj is linked in earlyInit.'); return null; }
	return { id: gridContainer.id, grid: gridContainer.gridObj, table: gridContainer.gridObj.tb };
}

function backupGrid() {
	var np, gridData, id, g, rows, cols, j, b, d, meta, pg, ts, fn, a;
	np = getNextpage();
	if (!np) { alert('Unable to detect page name (_nextpage or filename)'); return; }
	gridData = findGridFromButton();
	if (!gridData) return;
	id = gridData.id;
	g = gridData.grid;
	rows = g.getAllData();
	cols = [];
	if (g.columns) {
		for (j = 0; j < g.columns.length; ++j) {
			if (!g.columns[j].readonly && g.columns[j].edit !== false) cols.push(j);
		}
	}
	b = { nextpage: np, grids: {} };
	b.grids[id] = rows.map(function(r) {
		if (!Array.isArray(r) || !cols.length) return r;
		for (d = [], k = 0; k < cols.length; ++k) d.push(r[cols[k]]);
		return d;
	});
	meta = getModelVersion();
	pg = np.replace(/[^a-zA-Z0-9_-]+/g, '_') || 'unknown';
	d = new Date();
	ts = d.getFullYear()+'-'+(('0'+(d.getMonth()+1)).slice(-2))+'-'+(('0'+d.getDate()).slice(-2))+'_'+(('0'+d.getHours()).slice(-2))+(('0'+d.getMinutes()).slice(-2));
	fn = meta.model+'-'+meta.version+'-'+pg+'-backup-'+ts+'.json';
	j = JSON.stringify(b, null, 2);
	a = document.createElement('a');
	a.href = 'data:application/json;charset=utf-8,'+encodeURIComponent(j);
	a.download = fn;
	a.style.display = 'none';
	document.body.appendChild(a);
	a.click();
	document.body.removeChild(a);
}

function restoreGrid() {
	var fi, button, f, r, tempEvent, b, currNext, gridData, id, g, tbl, warnings, enableBox, p, lvl, ebs, nrm, rm, msg;
	fi = document.createElement('input');
	fi.type = 'file';
	fi.accept = '.json,application/json';
	fi.style.display = 'none';
	button = event.target || event.srcElement;
	fi._sourceButton = button;
	document.body.appendChild(fi);
	fi.onchange = function(e) {
		f = e.target.files[0];
		if (!f) return;
		r = new FileReader();
		r.onerror = function() { alert('Failed to read backup file.'); };
		r.onload = function(x) {
			try {
				tempEvent = { target: fi._sourceButton, srcElement: fi._sourceButton };
				window.event = tempEvent;
				b = JSON.parse(x.target.result);
				currNext = getNextpage();
				if (!currNext) { alert('Cannot restore: missing _nextpage field or filename.'); return; }
				if (b.nextpage !== currNext) { alert('Restore denied!\nThis backup is for \''+(b.nextpage || 'unknown')+'\',\nbut this page is \''+currNext+'\'.'); return; }
				gridData = findGridFromButton();
				if (!gridData) return;
				id = gridData.id;
				g = gridData.grid;
				tbl = gridData.table;
				if (!b.grids || !Array.isArray(b.grids[id])) { alert('No backup data found for grid \''+id+'\''); return; }
				warnings = [];
				enableBox = null;
				p = tbl;
				for (lvl = 0; lvl < 2 && p; ++lvl) {
					ebs = p.querySelectorAll && p.querySelectorAll('input[type=checkbox][name*="enable"]');
					if (ebs && ebs.length) { enableBox = ebs[0]; break; }
					p = p.parentNode;
				}
				if (enableBox && enableBox.disabled) {
					enableBox.disabled = false;
					enableBox.checked = true;
					warnings.push('Enabled a section to allow grid restore (checkbox \''+enableBox.name+'\').');
				}
				if (g.removeAllData) g.removeAllData();
				b.grids[id].forEach(function(row) { g.insertData(-1, row); });
				if (typeof g.getAllData === 'function' && g.getAllData().length > b.grids[id].length) {
					nrm = g.getAllData().length - b.grids[id].length;
					for (rm = 0; rm < nrm; ++rm) { if (g.removeRow) g.removeRow(g.getAllData().length - 1); }
				}
				msg = 'Successfully restored table.'+sufGridMsg;
				if (warnings.length) msg += '\n\nWarnings:\n- '+warnings.join('\n- ');
				alert(msg);
			}
			catch(ex) { alert('Restore failed: '+(ex.message || ex)); }
		};
		r.readAsText(f);
		fi.value = '';
	};
	fi.click();
}

function clearGrid() {
	if (!confirm('Are you sure you want to remove ALL entries from this table?'+sufGridMsg)) return;
	var gridData = findGridFromButton();
	if (!gridData) return;
	var grid = gridData.grid;
	if (grid && grid.removeAllData) {
		grid.removeAllData();
		grid.recolor();
		if (grid.update) grid.update();
	}
	else
		alert('Error: Table object not found. Make sure gridObj is linked in earlyInit.');
}
