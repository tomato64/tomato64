;(function () {
	var templateId = 'ethernet-svg-template';
	var templateContent = `<svg width="46" height="35" viewBox="0 0 46 35" preserveAspectRatio="xMidYMid meet" style="shape-rendering:geometricPrecision;text-rendering:geometricPrecision" version="1.1" xmlns="http://www.w3.org/2000/svg" data-duplex="HD" data-speed="0">
		<defs>
			<linearGradient id="housing-grad" x1="30%" y1="0%" x2="50%" y2="100%"><stop stop-color="#ffffff" offset="0%"/><stop stop-color="#ffffff" offset="100%"/></linearGradient>
			<linearGradient id="inner-dark" x1="0%" y1="0%" x2="0%" y2="100%"><stop stop-color="var(--inner-top, var(--duplex-end, #6a6d70ff))" offset="0%"/><stop stop-color="var(--inner-bottom, var(--duplex-start, rgb(47, 49, 50)))" offset="100%"/></linearGradient>
			<linearGradient id="led-base" x1="25%" y1="5%" x2="100%" y2="100%"><stop stop-color="var(--led-primary)" offset="0%"/><stop stop-color="var(--led-secondary)" offset="100%"/></linearGradient>
			<linearGradient id="led-on" x1="25%" y1="5%" x2="100%" y2="100%"><stop stop-color="var(--led-primary)" offset="0%"/><stop stop-color="var(--led-secondary)" offset="100%"/></linearGradient>
			<linearGradient id="led-dim" x1="0%" y1="0%" x2="0%" y2="100%">	<stop stop-color="var(--led-off-primary, var(--led-primary))" offset="0%"/><stop stop-color="var(--led-off-secondary, var(--led-secondary))" offset="100%"/></linearGradient>
			<linearGradient id="led-shade" x1="0%" y1="0%" x2="0%" y2="100%"><stop offset="0%" stop-color="transparent"/><stop offset="65%" stop-color="transparent"/><stop offset="100%" stop-color="var(--led-shade)" stop-opacity="0.25"/></linearGradient>
			<radialGradient id="led-bottom" cx="85%" cy="85%" r="60%" fx="85%" fy="85%"><stop offset="0%" stop-color="var(--led-secondary)" stop-opacity="0.28"/><stop offset="60%" stop-color="var(--led-secondary)" stop-opacity="0.06"/><stop offset="100%" stop-color="transparent" stop-opacity="0"/></radialGradient>

			<style><![CDATA[
				svg{--led-off-primary: rgb(245, 248, 250);--led-off-secondary: #D9DFE3ff;--led-off-dim-start: #777777ff;--led-off-dim-end: #666666ff;--led-off-shade: #777777ff}
				svg[data-duplex="FD"]{--duplex-start: rgb(123, 127, 131);--duplex-end: rgb(39, 41, 44);--pins-fill: #606060ff;--caption-fill: #ffffff;--inner-top:var(--duplex-end);--inner-bottom:var(--duplex-start)}
				svg[data-duplex="HD"]{--duplex-start: #fbfbfcff;--duplex-end: rgb(210, 210, 208);--pins-fill: #C9C9C9ff;--caption-fill: #000000;--inner-top:var(--duplex-start);--inner-bottom:var(--duplex-end)}
				svg[data-speed="0"]{--led-primary: #C8CDC0ff;--led-secondary: #D9DFE3ff;--led-shade: #777777ff;--led-duration:0s}
				svg[data-speed="10"]{--led-primary: #ffb32bff;--led-secondary: rgb(196, 128, 11);--led-shade: #8a4f24ff;--led-duration:3s}
				svg[data-speed="100"]{--led-primary: rgb(32, 177, 25);--led-secondary: rgb(8, 129, 3);--led-shade: #243f18ff;--led-duration:2.5s}
				svg[data-speed="1000"]{--led-primary: rgb(53, 125, 233);--led-secondary: rgb(27, 86, 182);--led-shade: #193b58ff;--led-duration:2s}
				svg[data-speed="2500"]{--led-primary: rgb(232, 63, 51);--led-secondary: rgb(184, 22, 3);--led-shade: #5a1b2eff;--led-duration:1.2s}
				svg[data-speed="5000"]{--led-primary: rgb(177, 18, 194);--led-secondary: rgb(105, 1, 170);--led-shade: #1f0420ff;--led-duration:0.9s}
				svg[data-speed="10000"]{--led-primary: rgb(108, 230, 246);--led-secondary: rgb(61, 184, 255);--led-dim-start: #595959ff;--led-dim-end: #494949ff;--led-shade: #595959ff;--led-duration:0.6s}
				@-webkit-keyframes blink-activity{ 0%,100%{opacity:0}6%{opacity:1}9%{opacity:0}17%{opacity:1}28%{opacity:1}35%{opacity:0}52%{opacity:1}66%{opacity:0}72%{opacity:1}83%{opacity:0}91%{opacity:1}}
				@keyframes blink-activity{0%,100%{opacity:0}6%{opacity:1}9%{opacity:0}17%{opacity:1}28%{opacity:1}35%{opacity:0}52%{opacity:1}66%{opacity:0}72%{opacity:1}83%{opacity:0}91%{opacity:1}}
				.act-led-blink .led-dim{opacity:0;-webkit-animation-name:blink-activity;animation-name:blink-activity;-webkit-animation-duration:var(--led-duration, 3s);animation-duration:var(--led-duration, 3s);-webkit-animation-iteration-count:infinite;animation-iteration-count:infinite;-webkit-animation-timing-function:step-end;animation-timing-function:step-end;}
				svg[data-speed="0"] .act-led-blink .led-dim{-webkit-animation:none;animation:none;}
			]]></style>
		</defs>

		<clipPath id="outer-clip"><rect width="46" height="35" rx="4" ry="4"/></clipPath>

		<g clip-path="url(#outer-clip)" transform="translate(0,35) scale(1,-1)">
			<rect x="0.75" y="0.75" width="44.5" height="33.5" rx="3.25" ry="3.25" fill="url(#housing-grad)" stroke="#888888" stroke-width="1"/>
			<path d="M6 6 Q6 5 7 5 L39 5 Q40 5 40 6 L40 22 Q40 23 39 23 L32 23 L32 24 Q32 25 31 25 L29 25 L29 27 Q29 28 28 28 L18 28 Q17 28 17 27 L17 25 L15 25 Q14 25 14 24 L14 23 L7 23 Q6 23 6 22 Z" fill="url(#inner-dark)" stroke="#323639ff" stroke-width="0.45"/>
			<g fill="var(--pins-fill)"><rect x="12" y="5.4" width="2.8" height="6"/><rect x="18.6" y="5.4" width="2.8" height="6"/><rect x="25.2" y="5.4" width="2.8" height="6"/><rect x="31.8" y="5.4" width="2.8" height="6"/></g>
			<rect x="3" y="25.5" width="9" height="6" rx="1.5" ry="1.5" fill="url(#led-on)"/>
			<rect x="3" y="25.5" width="9" height="6" rx="1.5" ry="1.5" fill="url(#led-bottom)" pointer-events="none" opacity="0.22"/>
			<rect x="3" y="25.5" width="9" height="6" rx="1.5" ry="1.5" fill="url(#led-shade)" pointer-events="none" opacity="0.15"/>
			<g class="act-led-blink"><rect class="led-on" x="33.5" y="25.5" width="9" height="6" rx="1.5" ry="1.5" fill="url(#led-on)"/><rect class="led-bottom" x="33.5" y="25.5" width="9" height="6" rx="1.5" ry="1.5" fill="url(#led-bottom)" pointer-events="none" opacity="0.22"/><rect class="led-dim" x="33.5" y="25.5" width="9" height="6" rx="1.5" ry="1.5" fill="url(#led-dim)" opacity="0.45"/><rect x="33.5" y="25.5" width="9" height="6" rx="1.5" ry="1.5" fill="url(#led-shade)" pointer-events="none" opacity="0.25"/></g>
			<rect x="3" y="25.5" width="9" height="6" rx="1.5" ry="1.5" fill="none" stroke="#1e2021ff" stroke-width="0.45"/>
			<rect x="33.5" y="25.5" width="9" height="6" rx="1.5" ry="1.5" fill="none" stroke="#1e2021ff" stroke-width="0.45"/>
		</g>

		<script><![CDATA[
			(function(){
				try{
					var root = document.documentElement;
					var search = (typeof location !== 'undefined' && location.search) ? location.search:'';
					if (!search&&document.baseURI) { var qi = document.baseURI.indexOf('?'); if (qi !== -1) search = document.baseURI.substring(qi); }
					var params = new URLSearchParams(search);
					var du = params.get('duplex') || root.getAttribute('data-duplex') || 'HD';
					var sp = params.get('speed') || root.getAttribute('data-speed') || '0';
					var caption = params.get('caption') || root.getAttribute('data-caption') || '';
					root.setAttribute('data-duplex', du);
					root.setAttribute('data-speed', sp);
					if (caption) root.setAttribute('data-caption', caption);
					var txt = root.querySelector('#port-caption');
					if (txt) txt.textContent = caption;
					var explicit = params.get('led');
					if (explicit){ root.style.setProperty('--led-primary', explicit); root.style.setProperty('--led-secondary', explicit); root.style.setProperty('--led-dim-start', '#000000'); root.style.setProperty('--led-dim-end', '#000000'); root.style.setProperty('--led-shade', '#000000'); }
					var flash = params.get('flash');
					if (flash !== null) { if (flash === 'none') root.style.setProperty('--led-duration', '0s'); else { if (/^[0-9.]+$/.test(flash)) flash += 's'; root.style.setProperty('--led-duration', flash); }
					}
				} catch(e) {}
			})();
		]]></script>
	</svg>`;

	function ensureTemplate() {
		var tpl = document.getElementById(templateId);
		if (tpl) return tpl;
		tpl = document.createElement('template');
		tpl.id = templateId;
		tpl.innerHTML = templateContent;
		(document.body || document.documentElement).appendChild(tpl);
		return tpl;
	}

	function ensureEthSvg(host) {
		if (!host) return null;
		var svg = host.querySelector('svg');
		if (svg) return svg;
		var tpl = ensureTemplate();
		if (!tpl) return null;
		if (tpl.content && tpl.content.firstElementChild) {
			svg = tpl.content.firstElementChild.cloneNode(true);
			host.appendChild(svg);
			return svg;
		}
		host.innerHTML = tpl.innerHTML;
		return host.querySelector('svg');
	}

	function updateEthSvg(host, speed, duplex, caption) {
		var svg = ensureEthSvg(host);
		if (!svg) return;
		var w = host.getAttribute('data-w');
		var h = host.getAttribute('data-h');
		if (w && h) {
			svg.setAttribute('width', w);
			svg.setAttribute('height', h);
		}
		if (speed !== undefined) svg.setAttribute('data-speed', speed);
		if (duplex !== undefined) svg.setAttribute('data-duplex', duplex);
		if (caption !== undefined) {
			if (caption !== '')
				svg.setAttribute('data-caption', caption);
			else
				svg.removeAttribute('data-caption');
			var txt = svg.querySelector('#port-caption');
			if (txt) txt.textContent = caption;
			var pins = svg.querySelectorAll("rect[x='12'], rect[x='18.6'], rect[x='25.2'], rect[x='31.8']");
			pins.forEach(function (pin) {
				pin.style.display = caption ? 'none' : '';
			});
		}
	}

	window.ensureEthSvg = ensureEthSvg;
	window.updateEthSvg = updateEthSvg;

	function ensureEthCaption(host) {
		if (!host) return null;
		var svg = ensureEthSvg(host);
		if (!svg) return null;
		var txt = svg.querySelector('#port-caption');
		if (txt) return txt;
		txt = document.createElementNS('http://www.w3.org/2000/svg', 'text');
		txt.setAttribute('id', 'port-caption');
		txt.setAttribute('x', '23');
		txt.setAttribute('y', '22');
		txt.setAttribute('text-anchor', 'middle');
		txt.setAttribute('dominant-baseline', 'middle');
		txt.setAttribute('font-size', '10');
		txt.setAttribute('font-family', 'Arial, Helvetica, sans-serif');
		txt.setAttribute('fill', 'var(--caption-fill)');
		svg.appendChild(txt);
		return txt;
	}

	function renderEthIcon(host, speed, duplex, caption) {
		if (!host) return null;
		var elem = host;
		var target = elem;
		try {
			if (elem && !elem._ethShadowHost && typeof elem.attachShadow === 'function') {
				var shadow = elem.attachShadow({mode: 'open'});
				var inner = document.createElement('div');
				shadow.appendChild(inner);
				elem._ethShadowHost = inner;
			}
			if (elem && elem._ethShadowHost)
				target = elem._ethShadowHost;
		} catch (e) {}
		ensureEthCaption(target);
		return updateEthSvg(target, speed, duplex, caption);
	}

	window.ensureEthCaption = ensureEthCaption;
	window.renderEthIcon = renderEthIcon;
})();
