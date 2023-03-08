<!DOCTYPE html>
<!--
	Tomato GUI

	For use with Tomato Firmware only.
	No part of this file may be used without permission.
-->
<html lang="en-GB">
<head>
<meta http-equiv="content-type" content="text/html;charset=utf-8">
<meta name="robots" content="noindex,nofollow">
<title>[<% ident(); %>] Tools: System Commands</title>
<link rel="stylesheet" type="text/css" href="tomato.css">
<% css(); %>
<script src="tomato.js"></script>
<script src="termlib_min.js"></script>

<script>

//	<% nvram(''); %>	// http_id

var term;
var working_dir = '/www';

function verifyFields(focused, quiet) {
	return 1;
}

function termOpen() {
	if ((!term) || (term.closed)) {
		term = new Terminal(
			{
				cols: 94,
				rows: 35,
				mapANSI: true,
				crsrBlinkMode: true,
				closeOnESC: false,
				termDiv: 'term-div',
				blinkDelay: 300,
				handler: termHandler,
				initHandler: initHandler,
				wrapping: true
			}
		);
		term.open();
	}
	window.addEventListener("paste", function(thePasteEvent) {
		if (term) {
			var clipboardData, pastedData;
			thePasteEvent.stopPropagation();
			thePasteEvent.preventDefault();
			clipboardData = thePasteEvent.clipboardData || window.clipboardData;
			pastedData = clipboardData.getData('Text');
			TermGlobals.importMultiLine(pastedData);
		}
	}, false);
}

function initHandler() {
	term.write('%+r FreshTomato Web Shell ready. %-r \n\n');
	runCommand('mymotd');
}

function termHandler() {
	this.newLine();

	this.lineBuffer = this.lineBuffer.replace(/^\s+/, '');
	if (this.lineBuffer.substring(0, 3) == 'cd ') {
		var tmp = this.lineBuffer.slice(3);

		if (tmp === ".") {
			return; /* nothing to do here.. */
		}
		else if (tmp === "..") {
			workingDirUp();
			term.write('%+r Switching to directory %+b' + working_dir + '%-b %-r \n');
			term.prompt();
		}
		else {
			checkDirectoryExist(tmp);
		}
	}
	else if (this.lineBuffer === "clear") {
		term.clear();
		term.prompt();
	}
	else {
		runCommand(this.lineBuffer);
	}
}

function showWait(state) {
	E('wait').style.display = (state ? 'block' : 'none');
	E('spin').style.display = (state ? 'inline-block' : 'none');
}

function checkDirectoryExist(directoryToCheck) {
	var cmd = new XmlHttp();
	cmd.onCompleted = function(text, xml) {
		if (text.trim() === "OK") {
			if (directoryToCheck.substring(0, 1) != '/') {
				working_dir = working_dir + "/" + directoryToCheck;
			}
			else {
				working_dir = directoryToCheck;
			}
			term.write('%+r Switching to directory %+b' + working_dir + '%-b %-r \n');
		}
		else {
			term.write( '%+r%c(red) ERROR directory ' + directoryToCheck + ' does not exist %c(default)%-r' );
		}
		showWait(false);
		term.prompt();
	}
	cmd.onError = function(text) {
		term.write( '%c(red) ERROR during switching directory: ' + text + ' %c(default)' );
		showWait(false);
		term.prompt();
	}

	showWait(true);
	if (directoryToCheck.substring(0, 1) != '/') {
		directoryToCheck = working_dir + "/" + directoryToCheck;
	}
	cmd.post('shell.cgi', 'action=execute&nojs=1&command=' + escapeCGI("[ -d \"" + directoryToCheck + "\" ] && echo \"OK\""));
}

function workingDirUp() {
	var tmp = working_dir.split("/");
	if (tmp.length > 1) {
		tmp.pop();
		working_dir = tmp.join("/");
		if (working_dir === "") { /* it was last dir; */
			working_dir = "/";
		}
	}
}

function runCommand(command) {
	var cmd = new XmlHttp();
	cmd.onCompleted = function(text, xml) {
		term.write(text.split('\n'), true);
		showWait(false);
	}
	cmd.onError = function(text) {
		term.type( '> ERROR: ' + text, 17 );
		showWait(false);
		term.prompt();
	}

	showWait(true);
	cmd.post('shell.cgi', 'action=execute&nojs=1&working_dir=' + working_dir + '&command=' + escapeCGI(command));
}

function fakecommand() {
	var command = E('term-helper-icommands').value;
	for (var i=0; i<command.length; i++) {
		Terminal.prototype.globals.keyHandler({which: command.charCodeAt(i), _remapped:true});
	}
	sendCR();
	E('term-helper-icommands').value = '';
	E('term-helper-icommands').focus();
}

function sendSpace() {
	Terminal.prototype.globals.keyHandler({which: 32, _remapped:false});
	sendCR();
}

function sendCR() {
	Terminal.prototype.globals.keyHandler({which: 0x0D, _remapped:false});
}

function toggleHWKeyHelper() {
	E('term-helper-div').style.display = 'inline-block';
	E('term-helper-link').style.display = 'none';
}
</script>
</head>

<body onload="termOpen()">
<form action="javascript:{}">
<table id="container">
<tr><td colspan="2" id="header">
	<div class="title">FreshTomato</div>
	<div class="version">Version <% version(); %> on <% nv("t_model_name"); %></div>
</td></tr>
<tr id="body"><td id="navi"><script>navi()</script></td>
<td id="content">
<div id="ident"><% ident(); %> | <script>wikiLink();</script></div>

<!-- / / / -->

<div class="section-title">Execute System Commands</div>
<div class="section">
	<div id="term-div"></div>

	<div class="note-spacer"><a id="term-helper-link" href="#" onclick="toggleHWKeyHelper();">No hardware keyboard</a></div>

	<div id="term-helper-div" style="display:none">
		<input type="button" class="term-helper-button" onclick="fakecommand()" value="Enter">
		<input type="button" class="term-helper-button" onclick="sendSpace()" value="Space"><br>
		<input type="text" id="term-helper-icommands" name="commands" spellcheck="false" style="text-transform:none">
	</div>

</div>

<!-- / / / -->

<div id="wait">Please wait...&nbsp; <img src="spin.gif" alt="" id="spin"></div>

<!-- / / / -->

<div id="footer">
	&nbsp;
</div>

</td></tr>
</table>
</form>
</body>
</html>
