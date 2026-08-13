#!/usr/bin/env node
/*
 * Regression test for tomato64/package/www/package www/advanced-vlan.asp
 *
 * The VLAN page is the one Tomato64 web page that writes a large, interdependent
 * set of NVRAM variables (vlanNports / vlanNhwname / vlanNvid / lanN_ifnames /
 * lanN_ifnames_vlan / wanN_ifnameX / wanN_ifnameX_vlan ...).  A subtle change in
 * save() can silently produce a bad network config, so this test pins the exact
 * output of save() for a large set of deterministic grid configurations.
 *
 * It works by extracting the real code out of advanced-vlan.asp -- after applying
 * the same marker stripping the www Makefile applies for a Tomato64 build -- and
 * executing it against stub DOM objects.  Results are compared to a golden file.
 *
 * Usage (from anywhere -- paths are resolved relative to this script):
 *   node tomato64/scripts/tests/test-advanced-vlan.js
 *                                    run the suite (default 4000 cases/config)
 *   ... --samples 6                  print N worked examples
 *   ... --cases 20000                run more cases
 *   ... --verbose                    print every mismatching field
 *   ... --seed 1234                  dump one case in full (input + NVRAM written),
 *                                    for investigating a reported mismatch
 *   ... --compare-rev HEAD           also diff against that git revision of the
 *                                    page (no golden needed)
 *   ... --file <path>                test a different copy of the page
 *   ... --update                     rewrite the golden file
 *
 * IMPORTANT: only run --update after you have reviewed the reported differences
 * and decided the new behaviour is correct.  The golden file is the record of
 * "what the VLAN page is known to do".
 *
 * Exit status: 0 = pass, 1 = fail.
 */
'use strict';

const fs = require('fs');
const path = require('path');
const crypto = require('crypto');
const { execFileSync } = require('child_process');

/* ------------------------------------------------------------------ options */

const argv = process.argv.slice(2);
function opt(name, dflt) {
	const i = argv.indexOf('--' + name);
	if (i < 0) return dflt;
	const next = argv[i + 1];
	return (next === undefined || next.startsWith('--')) ? true : next;
}
const OPT = {
	file:       opt('file', path.join(__dirname, '..', '..', 'package', 'www', 'www', 'advanced-vlan.asp')),
	golden:     opt('golden', path.join(__dirname, 'test-advanced-vlan.golden.json')),
	cases:      parseInt(opt('cases', '4000'), 10),
	samples:    parseInt(opt('samples', '3'), 10),
	update:     opt('update', false) === true,
	verbose:    opt('verbose', false) === true,
	compareRev: opt('compare-rev', null),
	seed:       opt('seed', null)
};

let failures = [];
const fail = m => { failures.push(m); };

/* --------------------------------------------------- platform configurations
 * These mirror shared.h for a Tomato64 build:
 *   BRIDGE_COUNT 8   -> MAX_BRIDGE_ID 7
 *   TOMATO_VLANNUM 16-> MAX_VLAN_ID   15
 *   MAX_PORT_ID  8
 *   MWAN_MAX     TCONFIG_MWAN_MAX (default 4, range 1..8)
 * Two WAN-count variants are exercised.
 */
const CONFIGS = [
	{ name: 'multiwan', MAX_BRIDGE_ID: 7, MAX_VLAN_ID: 15, MAX_PORT_ID: 8, MAXWAN_NUM: 4 },
	{ name: 'singlewan', MAX_BRIDGE_ID: 7, MAX_VLAN_ID: 15, MAX_PORT_ID: 8, MAXWAN_NUM: 2 }
];

/* ------------------------------------------- www/Makefile marker replication */

/* Delete whole BEGIN..END ranges (inclusive), like sed '/A/,/B/d'. */
function dropRange(lines, begin, end) {
	const out = [];
	let skipping = false;
	for (const l of lines) {
		if (!skipping && l.includes(begin)) { skipping = true; continue; }
		if (skipping) { if (l.includes(end)) skipping = false; continue; }
		out.push(l);
	}
	return out;
}
/* Delete just the marker lines, keeping the content between them. */
function dropMarkers(lines, ...marks) {
	return lines.filter(l => !marks.some(m => l.includes(m)));
}

/*
 * Reproduce what package/www/www/Makefile leaves in advanced-vlan.asp for a
 * Tomato64 build (TCONFIG_TOMATO64=y implies the BCMARM / MIPSR2P / RTNPLUS
 * "not-NO" branches), plus the C-comment REMOVE blocks that remcoms2.sh strips.
 */
function tomato64BuildView(src) {
	let lines = src.split('\n');
	lines = dropRange(lines, 'TOMATO64-SKIP-BEGIN', 'TOMATO64-SKIP-END');
	lines = dropRange(lines, 'RTNPLUS-NO-BEGIN', 'RTNPLUS-NO-END');
	lines = dropRange(lines, 'MIPSR2P-NO-BEGIN', 'MIPSR2P-NO-END');
	lines = dropRange(lines, 'BCMARM-NO-BEGIN', 'BCMARM-NO-END');
	lines = dropRange(lines, 'REMOVE-BEGIN', 'REMOVE-END');
	lines = dropMarkers(lines,
		'TOMATO64-BEGIN', 'TOMATO64-END',
		'RTNPLUS-BEGIN', 'RTNPLUS-END',
		'MIPSR2P-BEGIN', 'MIPSR2P-END',
		'BCMARM-BEGIN', 'BCMARM-END');
	return lines.join('\n');
}

/* ------------------------------------------------------------- code extraction
 * Anchored on stable landmarks.  If the page is restructured these throw, which
 * is deliberate: a silent "0 tests ran" would be worse than a loud failure.
 */

function extract(view, what, startRe, endPred) {
	const lines = view.split('\n');
	const start = lines.findIndex(l => startRe.test(l));
	if (start < 0) throw new Error('could not locate ' + what + ' (looked for ' + startRe + ')');
	for (let i = start; i < lines.length; ++i) {
		if (i > start && endPred(lines[i])) return lines.slice(start, i + 1).join('\n');
	}
	throw new Error('could not find the end of ' + what);
}

/* constants + helper functions (VLAN_COUNT .. wlanBridgeOptions) */
function extractHelpers(view) {
	return extract(view, 'the VLAN constants/helpers block',
		/^var VLAN_COUNT\s*=/, l => /^wlanBridgeOptions\.push\(\[WLAN_BRIDGE_NONE/.test(l));
}
/* function save() { ... } -- ends at the first closing brace in column 0 */
function extractSave(view) {
	return extract(view, 'function save()', /^function save\(\)\s*\{/, l => l === '}');
}
/* the <script> that emits the hidden inputs */
function extractHiddenInputs(view) {
	const lines = view.split('\n');
	const anchor = lines.findIndex(l => l.includes("W('<input type=\"hidden\" name=\"vlan'+i+'ports\">')"));
	if (anchor < 0) throw new Error('could not locate the hidden-input generator script');
	let start = anchor; while (start > 0 && !/^<script>/.test(lines[start])) start--;
	let end = anchor; while (end < lines.length && !/^<\/script>/.test(lines[end])) end++;
	return lines.slice(start + 1, end).join('\n');
}

/* --------------------------------------------------------------- the sandbox */

function makeEnv(cfg, helpersSrc) {
	const env = {
		MAX_BRIDGE_ID: cfg.MAX_BRIDGE_ID,
		MAX_VLAN_ID:   cfg.MAX_VLAN_ID,
		MAX_PORT_ID:   cfg.MAX_PORT_ID,
		MAXWAN_NUM:    cfg.MAXWAN_NUM
	};
	/* evaluate the page's own constants/helpers so the test tracks the file */
	const names = ['MAX_BRIDGE_ID', 'MAX_VLAN_ID', 'MAX_PORT_ID', 'MAXWAN_NUM'];
	const exported = ['VLAN_COUNT', 'VLAN_OFFSET_MAX', 'VLAN_BRIDGE_NONE', 'VLAN_BRIDGE_WAN0',
	                  'VLAN_BRIDGE_LAN0', 'VLAN_BRIDGE_WAN1', 'WLAN_BRIDGE_NONE',
	                  'lanPrefix', 'wanPrefix', 'vlanBridgeLan', 'vlanBridgeWan',
	                  'vlanBridgeOptions', 'vlanBridgeNames', 'wlanBridgeOptions'];
	const fn = new Function(...names,
		helpersSrc + '\nreturn {' + exported.map(n => n + ':' + n).join(',') + '};');
	Object.assign(env, fn(...names.map(n => env[n])));

	env.COL_VID = 0;
	env.COL_MAP = 1;
	env.COL_P0  = 2;
	env.COL_VID_DEF = env.COL_P0 + cfg.MAX_PORT_ID + 1;
	env.COL_BRI = env.COL_VID_DEF + 1;
	return env;
}

/* Run the page's real save() against one grid dataset. */
function runSave(saveSrc, env, data, counts) {
	const fields = {};
	const fom = new Proxy({}, {
		get: (t, k) => (fields[k] = fields[k] || { value: '', disabled: 0 }),
		set: () => true
	});
	let submitted = false;
	const vlg = {
		isEditing: () => false,
		getAllData: () => data.map(r => r.slice()),
		countWan: w => (counts.wan[w] !== undefined ? counts.wan[w] : 1),
		countLan: l => (counts.lan[l] !== undefined ? counts.lan[l] : 1)
	};
	const E  = id => (id === 't_fom') ? fom : { value: '', innerHTML: '', style: {}, checked: false };
	const form = { submit: () => { submitted = true; } };

	const args = {
		vlg, E, form,
		setTimeout: () => {}, confirm: () => true, alert: () => {}, reloadPage: () => {},
		portCol:        i => env.COL_P0 + i,
		portColName:    i => i.toString(),
		trailingSpace:  s => (s.length && s.slice(-1) !== ' ') ? ' ' : '',
		cmpInt:         (a, b) => parseInt(a, 10) - parseInt(b, 10),
		trunk_vlan_supported: 1,
		PORT_VLAN_SUPPORT_OVERRIDE: 0,
		SWITCH_INTERNAL_PORT: String(env.MAX_PORT_ID),
		wl_ifaces: [],                      /* Tomato64 sets wl_ifaces=[] on this page */
		nvram: { t_model_name: 'x86_64' }
	};
	Object.assign(args, env);

	const keys = Object.keys(args);
	new Function(...keys, saveSrc + '\nsave();')(...keys.map(k => args[k]));

	const out = { _submitted: submitted };
	for (const k of Object.keys(fields).sort())
		out[k] = fields[k].value + (fields[k].disabled ? ' [disabled]' : '');
	return out;
}

/* Run the hidden-input generator and collect the field names it emits. */
function runHiddenInputs(src, env) {
	const emitted = [];
	const args = Object.assign({ W: s => emitted.push(s) }, env);
	const keys = Object.keys(args);
	new Function(...keys, src)(...keys.map(k => args[k]));
	return emitted.map(s => (s.match(/name="([^"]+)"/) || [])[1]).filter(Boolean);
}

/* ------------------------------------------------------ scenario generation
 *
 * Grid states are built directly, so some of them are ones vlg.verifyFields()
 * would refuse to let a user create (e.g. two VLANs bridged to the same LAN).
 * That is deliberate: pinning save() for out-of-range input as well as valid
 * input makes the test stricter, and such states can still reach populate()
 * from NVRAM written outside this page.  Do not read a generated case as
 * "a config the UI allows".
 */

/* Deterministic LCG so every run and every machine produces the same cases. */
function rng(seed) {
	let s = (seed * 2654435761) >>> 0;
	return () => { s = (s * 1103515245 + 12345) & 0x7fffffff; return s / 0x7fffffff; };
}

function makeCase(cfg, env, seed) {
	const r = rng(seed);
	const pick = n => Math.floor(r() * n);
	const rows = [];
	const usedVid = new Set();
	const nrows = 1 + pick(5);
	for (let n = 0; n < nrows; ++n) {
		let vid = pick(cfg.MAX_VLAN_ID + 1);
		let guard = 0;
		while (usedVid.has(vid) && guard++ < 64) vid = pick(cfg.MAX_VLAN_ID + 1);
		if (usedVid.has(vid)) continue;
		usedVid.add(vid);
		const row = new Array(env.COL_BRI + 1).fill(0);
		row[env.COL_VID] = vid;
		row[env.COL_MAP] = ['', '0', String(100 + vid), String(4094)][pick(4)];
		for (let p = 0; p <= cfg.MAX_PORT_ID; ++p) row[env.COL_P0 + p] = pick(3);
		row[env.COL_VID_DEF] = (r() < 0.3) ? 1 : 0;
		/* every selectable bridge value, as both number and string */
		const bri = env.vlanBridgeOptions[pick(env.vlanBridgeOptions.length)][0];
		row[env.COL_BRI] = (r() < 0.5) ? bri : String(bri);
		rows.push(row);
	}
	/* also vary the WAN/LAN uniqueness counters that gate submission */
	const counts = { wan: {}, lan: {} };
	if (r() < 0.15) counts.wan[0] = pick(3);
	if (r() < 0.15) counts.lan[0] = pick(3);
	return { rows, counts };
}

function describeCase(c, env) {
	const lines = [];
	for (const row of c.rows) {
		const ports = [];
		for (let p = 0; p <= env.MAX_PORT_ID; ++p) {
			const v = row[env.COL_P0 + p];
			if (v == 1) ports.push(p + ':on');
			else if (v == 2) ports.push(p + ':tag');
		}
		const bri = parseInt(row[env.COL_BRI], 10);
		lines.push('    VLAN ' + row[env.COL_VID] +
			'  vid=' + (row[env.COL_MAP] === '' ? '(auto)' : row[env.COL_MAP]) +
			'  bridge=' + (env.vlanBridgeNames[bri] || '?') + '(' + bri + ')' +
			'  native=' + row[env.COL_VID_DEF] +
			'  ports=[' + (ports.join(' ') || 'none') + ']');
	}
	return lines.join('\n');
}

/* ------------------------------------------------------- structural checks */

function structuralChecks(env, cfg, tag) {
	const o = env.vlanBridgeOptions;
	if (o[0][0] !== env.VLAN_BRIDGE_NONE || o[0][1] !== 'none')
		fail(tag + ': vlanBridgeOptions[0] must be the "none" entry');
	if (o[1][0] !== env.VLAN_BRIDGE_WAN0 || o[1][1] !== 'WAN0')
		fail(tag + ': vlanBridgeOptions[1] must be WAN0');
	/* verifyFields()/resetNewEditor() index LAN entries as options[i + 2] */
	for (let i = 0; i <= cfg.MAX_BRIDGE_ID; ++i) {
		if (!o[i + 2] || o[i + 2][0] !== env.vlanBridgeLan(i))
			fail(tag + ': vlanBridgeOptions[' + (i + 2) + '] must be LAN' + i +
			     ' (the options[i+2] indexing in verifyFields/resetNewEditor depends on it)');
		if (env.vlanBridgeNames[env.vlanBridgeLan(i)] !== 'LAN' + i + ' (br' + i + ')')
			fail(tag + ': vlanBridgeNames for LAN' + i + ' is wrong');
	}
	for (let w = 1; w < cfg.MAXWAN_NUM; ++w)
		if (env.vlanBridgeNames[env.vlanBridgeWan(w)] !== 'WAN' + w)
			fail(tag + ': vlanBridgeNames for WAN' + w + ' is wrong');
	if (o.length !== 2 + (cfg.MAX_BRIDGE_ID + 1) + (cfg.MAXWAN_NUM - 1))
		fail(tag + ': vlanBridgeOptions has an unexpected length (' + o.length + ')');
	/* wireless selector: value i must map to bridge i, "none" last */
	for (let i = 0; i <= cfg.MAX_BRIDGE_ID; ++i)
		if (env.wlanBridgeOptions[i][0] !== i)
			fail(tag + ': wlanBridgeOptions[' + i + '] must have value ' + i);
	const last = env.wlanBridgeOptions[env.wlanBridgeOptions.length - 1];
	if (last[0] !== env.WLAN_BRIDGE_NONE || last[1] !== 'none')
		fail(tag + ': wlanBridgeOptions must end with the "none" entry');
}

function markerChecks(src) {
	const count = s => (src.match(new RegExp(s, 'g')) || []).length;
	const pairs = [['TOMATO64-SKIP-BEGIN', 'TOMATO64-SKIP-END'],
	               ['TOMATO64-BEGIN', 'TOMATO64-END']];
	for (const [b, e] of pairs) {
		/* TOMATO64-BEGIN must not be counted inside TOMATO64-SKIP-BEGIN */
		const nb = (src.match(new RegExp('(?<!SKIP-)(?<!REMOVE-)' + b, 'g')) || []).length;
		const ne = (src.match(new RegExp('(?<!SKIP-)(?<!REMOVE-)' + e, 'g')) || []).length;
		if (b.includes('SKIP')) {
			if (count(b) !== count(e)) fail('unbalanced markers: ' + count(b) + ' ' + b + ' vs ' + count(e) + ' ' + e);
		} else if (nb !== ne) {
			fail('unbalanced markers: ' + nb + ' ' + b + ' vs ' + ne + ' ' + e);
		}
	}
	if (/TOMATO64-REMOVE-BEGIN/.test(src))
		fail('advanced-vlan.asp uses TOMATO64-REMOVE, but the www Makefile only applies ' +
		     'TOMATO64-SKIP to this file: the content would ship on Tomato64. Use TOMATO64-SKIP.');
}

/* ------------------------------------------------------------------- runner */

function analyse(src, label) {
	const view = tomato64BuildView(src);
	const helpers = extractHelpers(view);
	const saveSrc = extractSave(view);
	const hiddenSrc = extractHiddenInputs(view);
	const perConfig = {};
	for (const cfg of CONFIGS) {
		const env = makeEnv(cfg, helpers);
		structuralChecks(env, cfg, label + '/' + cfg.name);
		const hidden = runHiddenInputs(hiddenSrc, env).sort();
		const hashes = [];
		const cases = [];
		for (let seed = 1; seed <= OPT.cases; ++seed) {
			const c = makeCase(cfg, env, seed);
			const out = runSave(saveSrc, env, c.rows, c.counts);
			const json = JSON.stringify(out);
			hashes.push(crypto.createHash('sha256').update(json).digest('hex').slice(0, 8));
			if (seed <= Math.max(OPT.samples, 8)) cases.push({ seed, input: c, output: out });
		}
		perConfig[cfg.name] = { env, hidden, hashes, cases };
	}
	return perConfig;
}

function loadRevision(rev, file) {
	const repo = execFileSync('git', ['rev-parse', '--show-toplevel'],
		{ cwd: path.dirname(file), encoding: 'utf8' }).trim();
	const rel = path.relative(repo, path.resolve(file));
	return execFileSync('git', ['show', rev + ':' + rel], { cwd: repo, encoding: 'utf8' });
}

/* ---------------------------------------------------------------------- run */

console.log('advanced-vlan.asp regression test');
console.log('  page:   ' + OPT.file);
console.log('  cases:  ' + OPT.cases + ' per configuration x ' + CONFIGS.length + ' configurations');

const src = fs.readFileSync(OPT.file, 'utf8');

/* --seed N: dump one case in full, for investigating a reported mismatch */
if (OPT.seed !== null) {
	const seed = parseInt(OPT.seed, 10);
	const view = tomato64BuildView(src);
	const helpers = extractHelpers(view), saveSrc = extractSave(view);
	for (const cfg of CONFIGS) {
		const env = makeEnv(cfg, helpers);
		const c = makeCase(cfg, env, seed);
		const out = runSave(saveSrc, env, c.rows, c.counts);
		console.log('\n=== seed ' + seed + ', configuration ' + cfg.name + ' ===');
		console.log('  grid input:');
		console.log(describeCase(c, env));
		console.log('  NVRAM written by save():');
		for (const k of Object.keys(out)) {
			if (k === '_submitted' || out[k] === '') continue;
			console.log('    ' + k.padEnd(26) + ' = ' + out[k]);
		}
		console.log('    ' + '(form submitted)'.padEnd(26) + ' = ' + out._submitted);
	}
	process.exit(0);
}

markerChecks(src);

let current;
try {
	current = analyse(src, 'current');
} catch (e) {
	console.error('\nFATAL: ' + e.message);
	console.error('The page structure changed in a way this test cannot follow. Update the');
	console.error('extraction anchors in ' + path.basename(__filename) + '.');
	process.exit(1);
}

/* --- worked examples ------------------------------------------------------ */
const sampleCfg = current[CONFIGS[0].name];
console.log('\n' + '='.repeat(78));
console.log('WORKED EXAMPLES (configuration: ' + CONFIGS[0].name + ')');
console.log('='.repeat(78));
for (const c of sampleCfg.cases.slice(0, OPT.samples)) {
	console.log('\n--- case seed ' + c.seed + ' -------------------------------------------');
	console.log('  grid input:');
	console.log(describeCase(c.input, sampleCfg.env));
	console.log('  NVRAM written by save():');
	for (const k of Object.keys(c.output)) {
		if (k === '_submitted') continue;
		if (c.output[k] === '') continue;
		console.log('    ' + k.padEnd(26) + ' = ' + c.output[k]);
	}
	console.log('    ' + '(form submitted)'.padEnd(26) + ' = ' + c.output._submitted);
}

/* --- comparison ----------------------------------------------------------- */
console.log('\n' + '='.repeat(78));
console.log('RESULTS');
console.log('='.repeat(78));

function compare(refName, ref) {
	let diffs = 0;
	for (const cfg of CONFIGS) {
		const a = ref[cfg.name], b = current[cfg.name];
		if (!a) { fail(refName + ': missing configuration ' + cfg.name); continue; }
		if (a.hidden.join(',') !== b.hidden.join(',')) {
			const miss = a.hidden.filter(x => !b.hidden.includes(x));
			const extra = b.hidden.filter(x => !a.hidden.includes(x));
			fail(cfg.name + ': hidden input set changed' +
			     (miss.length ? ' (missing: ' + miss.join(', ') + ')' : '') +
			     (extra.length ? ' (added: ' + extra.join(', ') + ')' : ''));
		}
		const n = Math.min(a.hashes.length, b.hashes.length);
		const bad = [];
		for (let i = 0; i < n; ++i) if (a.hashes[i] !== b.hashes[i]) bad.push(i + 1);
		if (a.hashes.length !== b.hashes.length)
			console.log('  note: ' + refName + ' has ' + a.hashes.length + ' cases, this run has ' +
			            b.hashes.length + '; comparing the first ' + n);
		diffs += bad.length;
		if (bad.length) {
			fail(cfg.name + ': save() output changed for ' + bad.length + ' of ' + n +
			     ' cases (first at seed ' + bad[0] + ')');
			if (OPT.verbose) {
				const shown = bad.slice(0, 3);
				for (const seed of shown) {
					const ac = (a.cases || []).find(c => c.seed === seed);
					const bc = b.cases.find(c => c.seed === seed);
					if (!ac || !bc) continue;
					console.log('\n  seed ' + seed + ' field differences:');
					const keys = new Set([...Object.keys(ac.output), ...Object.keys(bc.output)]);
					for (const k of keys)
						if (ac.output[k] !== bc.output[k])
							console.log('    ' + k + ':\n      ' + refName + ': ' +
							            JSON.stringify(ac.output[k]) + '\n      current:  ' +
							            JSON.stringify(bc.output[k]));
				}
			}
		}
		console.log('  ' + cfg.name.padEnd(11) + ' ' + n + ' cases compared vs ' + refName +
		            ', ' + bad.length + ' behaviour change(s)');
	}
	return diffs;
}

if (OPT.compareRev) {
	console.log('\ncomparing against git revision ' + OPT.compareRev + ':');
	try {
		compare('rev ' + OPT.compareRev, analyse(loadRevision(OPT.compareRev, OPT.file), 'rev'));
	} catch (e) {
		fail('could not analyse revision ' + OPT.compareRev + ': ' + e.message);
	}
}

const serialise = () => {
	const o = { generated: new Date().toISOString().slice(0, 10), cases: OPT.cases, configs: {} };
	for (const cfg of CONFIGS)
		o.configs[cfg.name] = {
			hidden: current[cfg.name].hidden,
			hashes: current[cfg.name].hashes.join(''),
			cases: current[cfg.name].cases.map(c => ({ seed: c.seed, output: c.output }))
		};
	return o;
};

if (OPT.update) {
	fs.writeFileSync(OPT.golden, JSON.stringify(serialise(), null, 1) + '\n');
	console.log('\ngolden file rewritten: ' + OPT.golden);
	console.log('review the diff before committing it.');
} else if (fs.existsSync(OPT.golden)) {
	const raw = JSON.parse(fs.readFileSync(OPT.golden, 'utf8'));
	const ref = {};
	for (const name of Object.keys(raw.configs)) {
		const g = raw.configs[name];
		ref[name] = { hidden: g.hidden, hashes: g.hashes.match(/.{8}/g) || [], cases: g.cases };
	}
	console.log('\ncomparing against golden (' + OPT.golden + ', recorded ' + raw.generated + '):');
	compare('golden', ref);
} else {
	fail('no golden file at ' + OPT.golden + ' -- run once with --update to create it');
}

/* --------------------------------------------------------------- conclusion */

console.log('\n' + '='.repeat(78));
if (failures.length === 0) {
	console.log('PASS - advanced-vlan.asp behaves exactly as recorded.');
	console.log('='.repeat(78));
	process.exit(0);
} else {
	console.log('FAIL - ' + failures.length + ' problem(s):');
	for (const f of failures) console.log('  * ' + f);
	console.log('');
	console.log('If these changes are intentional, re-run with --verbose to inspect them,');
	console.log('then with --update to accept them into the golden file.');
	console.log('='.repeat(78));
	process.exit(1);
}
