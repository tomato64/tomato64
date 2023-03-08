var vWidth = 310;
var vHeight = 310;
var tipInfo = [];
var tipTimer = null;
var tipLastN = null;
var ready = 0;

function init(evt) {
	var e, i, l, s;

	if (typeof(svgDocument) == 'undefined')
		svgDocument = evt.target.ownerDocument;

	e = document.getElementsByTagName('path');
	for (i = 0; i < count; ++i) {
		l = document.getElementById('lg'+i);
		l.setAttribute('x1', '0%');
		l.setAttribute('x2', '100%');
		l.setAttribute('y1', '0%');
		l.setAttribute('y2', '100%');

		s = l.getElementsByTagName('stop');
		s.item(0).setAttribute('offset', '0');
		s.item(0).setAttribute('stop-opacity', '0.7');
		s.item(0).setAttribute('stop-color', top.colors[i]);
		s.item(1).setAttribute('offset', '100%');
		s.item(1).setAttribute('stop-opacity', '1');
		s.item(1).setAttribute('stop-color', top.colors[i]);

		e.item(i).setAttribute('fill', 'url(#lg'+i+')');
	}

	tipGroup = document.getElementById('tip-group');
	tipMask = document.getElementById('tip-mask');
	tipText = document.getElementById('tip-text');

	ready = 1;
}

function updateSVG(data, abc) {
	var path, i, e, r, x, y, cx, cy, nx, ny, t, p, total;

	mOut();

	total = 0;
	for (i = 0; i < count; ++i)
		total += data[i] || 0;
	if (total == 0)
		return;

	r = 150;
	cx = cy = r + 5;
	t = 0;
	nx = cx;
	ny = cy + r;
	path = document.getElementsByTagName('path');

	for (i = 0; i < count; ++i) {
		e = path.item(i);

		p = (data[i] || 0) / total;
		t += p;

		x = nx;
		y = ny;
		nx = cx + (r * Math.sin(t * Math.PI * 2));
		ny = cy + (r * Math.cos(t * Math.PI * 2));

		if (data[i] == 0)
			e.setAttribute('d', '');
		else if (p == 1)
			e.setAttribute('d', 'M5,'+cy+'A'+r+','+r+' 0 1,1 5,'+(cy + 1)+'z');
		else
			e.setAttribute('d', 'M'+cx+','+cy+'L'+x+','+y+'A'+r+','+r+' 0 '+((p > 0.5) ? '1' : '0')+',0 '+nx+','+ny+'Z');

		if (count == 11)
			tipInfo[i] = abc[i]+' '+(p * 100).toFixed(2)+'%'+((data[i] == parseInt(data[i])) ? ' ('+(data[i] || 0)+')' : '');
		else
			tipInfo[i] = abc[i]+' '+(p * 100).toFixed(2)+'% ('+(data[i] / 1000 || 0).toFixed(2)+')';

		e.setAttribute('onclick', 'mClick('+i+')');
		e.setAttribute('onmousemove', 'mMove(evt,'+i+')');
		e.setAttribute('onmouseout', 'mOut()');
	}
}

function mClick(n) {
	top.mClick(n);
}

function setText(e, text) {
	if (e.firstChild)
		e.removeChild(e.firstChild);

	e.appendChild(document.createTextNode(text));
}

function showTip() {
	setText(tipText, tipInfo[tipT]);

	tipGroup.setAttribute('display', 'block');

	if (tipY > (vHeight - 35))
		tipY -= 20;
	else
		tipY += 20;

	if (tipX > (vWidth - 165))
		tipX = vWidth - 170;

	tipGroup.setAttribute('transform', 'translate('+tipX+','+tipY+')');
	tipMask.setAttribute('width', tipText.getComputedTextLength() + 10);

	tipLastN = tipT;

	clearTimeout(tipTimer);
	tipTimer = null;
}

function mMove(evt, n) {
	if (n == tipLastN)
		return;

	tipT = n;
	tipX = evt.clientX;
	tipY = evt.clientY;
	setText(tipText, tipT+' '+tipX+'x'+tipY);

	if (tipTimer)
		clearTimeout(tipTimer);

	tipTimer = setTimeout(showTip, 250); /* doesn't work properly under Adobe/IE */
}

function mOut() {
	if (tipLastN != -1) {
		tipLastN = -1;
		tipGroup.setAttribute('display', 'none');
	}
	if (tipTimer) {
		clearTimeout(tipTimer);
		tipTimer = null;
	}
}
