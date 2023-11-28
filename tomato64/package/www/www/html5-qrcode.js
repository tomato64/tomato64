//---------------------------------------------------------------------
// JavaScript-HTML5 QRCode Generator
//
// Copyright (c) 2011 Amanuel Tewolde
//
// Licensed under the MIT license:
// http://www.opensource.org/licenses/mit-license.php
//
//---------------------------------------------------------------------

// Map for determing which QR version
// to use according to content length
VERSION_MAP = [
	17,
	32,
	53,
	78,
	106,
	134,
	154,
	192,
	230,
	271,
	321,
	367,
	425,
	458,
	520,
	586,
	644,
	718,
	792,
	858,
	929,
	1003,
	1091,
	1171,
	1273,
	1367,
	1465,
	1528,
	1628,
	1732,
	1840,
	1952,
	2068,
	2188,
	2303,
	2431,
	2563,
	2699,
	2809,
	2953
];


// Generates a QRCode of text provided.
// First QRCode is rendered to a canvas.
// The canvas is then turned to an image PNG
// before being returned as an <img> tag.
function showQRCode(text) {

	var dotsize = 5;  // size of box drawn on canvas
	var padding = 10; // (white area around your QRCode)
	var black = 'rgb(0,0,0)';
	var white = 'rgb(255,255,255)';
	var QRCodeVersion = 40;

	for (var i = 0; i < VERSION_MAP.length; ++i) {
		if (text.length < VERSION_MAP[i]) {
			QRCodeVersion = i + 1;
			break;
		}
	}

	var canvas = document.createElement('canvas');
	var qrCanvasContext = canvas.getContext('2d');
	try {
		// QR Code Error Correction Capability
		// Higher levels improves error correction capability while decreasing the amount of data QR Code size.
		// QRErrorCorrectLevel.L (5%) QRErrorCorrectLevel.M (15%) QRErrorCorrectLevel.Q (25%) QRErrorCorrectLevel.H (30%)
		// eg. L can survive approx 5% damage...etc.
		var qr = new QRCode(QRCodeVersion, QRErrorCorrectLevel.L);
		qr.addData(text);
		qr.make();
	}
	catch(err) {
		var errorChild = document.createElement('p');
		var errorMSG = document.createTextNode('QR Code FAIL! '+err);
		errorChild.appendChild(errorMSG);

		return errorChild;
	}

	var qrsize = qr.getModuleCount();
	canvas.setAttribute('height',(qrsize * dotsize) + padding);
	canvas.setAttribute('width',(qrsize * dotsize) + padding);
	var shiftForPadding = padding/2;

	if (canvas.getContext) {
		for (var r = 0; r < qrsize; r++) {
			for (var c = 0; c < qrsize; c++) {
				if (qr.isDark(r, c))
					qrCanvasContext.fillStyle = black;
				else
					qrCanvasContext.fillStyle = white;

				qrCanvasContext.fillRect ((c*dotsize) + shiftForPadding, (r*dotsize) + shiftForPadding, dotsize, dotsize); // x, y, w, h
			}
		}
	}

	var imgElement = document.createElement('img');
	imgElement.alt = 'qrcode';
	imgElement.style.background = 'white';
	imgElement.style.padding = '10px';
	imgElement.src = canvas.toDataURL('image/png');

	return imgElement;
}
