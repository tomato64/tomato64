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
<link rel="stylesheet" type="text/css" href="tomato.css?rel=<% version(); %>">
<% css(); %>
<script src="tomato.js?rel=<% version(); %>"></script>
<!-- TERMLIB-BEGIN -->
<script src="termlib_min.js?rel=<% version(); %>"></script>
<!-- TERMLIB-END -->

<script>

//	<% nvram(''); %>	// http_id

/* TERMLIB0-BEGIN */
var cmdresult = '';
var cmd = null;


var ref = new TomatoRefresh('update.cgi', '', 0, 'tools-shell_refresh');

ref.refresh = function(text) {
	execute();
}
/* TERMLIB0-END */
/* TERMLIB-BEGIN */
var term;
var working_dir = '/www';
/* TERMLIB-END */

function verifyFields(focused, quiet) {
	return 1;
}

/* TERMLIB0-BEGIN */
function escapeText(s) {
	function esc(c) {
		return '&#' + c.charCodeAt(0) + ';';
	}
	return s.replace(/[&"'<>]/g, esc).replace(/ /g, '&nbsp;'.replace(/\n/g, ' <br>'));
}

function showWait(state) {
	E('execb').disabled = state;
	E('_f_cmd').disabled = state;
	E('wait').style.display = (state ? 'block' : 'none');
	E('spin').style.display = (state ? 'inline-block' : 'none');
	if (!state) cmd = null;
}

function updateResult() {
	E('result').innerHTML = '<tt>' + escapeText(cmdresult) + '<\/tt>';
	cmdresult = '';
	showWait(0);
}

function execute() {
	/* Opera 8 sometimes sends 2 clicks */
	if (cmd) return;
	showWait(1);

	cmd = new XmlHttp();
	cmd.onCompleted = function(text, xml) {
		eval(text);
		updateResult();
	}
	cmd.onError = function(x) {
		cmdresult = 'ERROR: ' + x;
		updateResult();
	}

	var s = E('_f_cmd').value;
	cmd.post('shell.cgi', 'action=execute&command=' + escapeCGI(s.replace(/\r/g, '')));
	cookie.set('shellcmd', escape(s));
}

function init() {
	var s;
	if ((s = cookie.get('shellcmd')) != null)
		E('_f_cmd').value = unescape(s);
}
/* TERMLIB0-END */

/* TERMLIB-BEGIN */
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
	addEvent(window, 'paste', pasteEvent);
}

function pasteEvent(event) {
		if (term) {
			var clipboardData, pastedData;
			event.stopPropagation();
			event.preventDefault();
			clipboardData = event.clipboardData || window.clipboardData;
			pastedData = clipboardData.getData('Text');
			TermGlobals.importMultiLine(pastedData);
		}
}

function initHandler() {
	term.write('%+r Tomato64 Web Shell ready. %-r \n\n');
	runCommand('mymotd');
}

function termHandler() {
	this.newLine();

	this.lineBuffer = this.lineBuffer.replace(/^\s+/, '');
	if (this.lineBuffer.substring(0, 3) == 'cd ') {
		var tmp = this.lineBuffer.slice(3);

		if (tmp === '.')
			return; /* nothing to do here.. */
		else if (tmp === "..") {
			workingDirUp();
			term.write('%+r Switching to directory %+b'+working_dir+'%-b %-r \n');
			term.prompt();
		}
		else
			checkDirectoryExist(tmp);
	}
	else if (this.lineBuffer === 'clear') {
		term.clear();
		term.prompt();
	}
	else
		runCommand(this.lineBuffer);
}

function showWait(state) {
	E('wait').style.display = (state ? 'block' : 'none');
	E('spin').style.display = (state ? 'inline-block' : 'none');
}

function checkDirectoryExist(directoryToCheck) {
	var cmd = new XmlHttp();
	cmd.onCompleted = function(text, xml) {
		if (text.trim() === 'OK') {
			if (directoryToCheck.substring(0, 1) != '/')
				working_dir = working_dir+'/'+directoryToCheck;
			else
				working_dir = directoryToCheck;

			term.write('%+r Switching to directory %+b'+working_dir+'%-b %-r \n');
		}
		else
			term.write( '%+r%c(red) ERROR directory '+directoryToCheck+' does not exist %c(default)%-r');

		showWait(false);
		term.prompt();
	}
	cmd.onError = function(text) {
		term.write( '%c(red) ERROR during switching directory: '+text+' %c(default)' );
		showWait(false);
		term.prompt();
	}

	showWait(true);
	if (directoryToCheck.substring(0, 1) != '/')
		directoryToCheck = working_dir+'/'+directoryToCheck;

	cmd.post('shell.cgi', 'action=execute&nojs=1&command='+escapeCGI("[ -d \""+directoryToCheck+"\" ] && echo \"OK\""));
}

function workingDirUp() {
	var tmp = working_dir.split('/');
	if (tmp.length > 1) {
		tmp.pop();
		working_dir = tmp.join('/');
		if (working_dir === '') /* it was last dir; */
			working_dir = '/';
	}
}

function runCommand(command) {
	var cmd = new XmlHttp();
	cmd.onCompleted = function(text, xml) {
		term.write(text.split('\n'), true);
		showWait(false);
	}
	cmd.onError = function(text) {
		term.type( '> ERROR: '+text, 17 );
		showWait(false);
		term.prompt();
	}

	showWait(true);
	cmd.post('shell.cgi', 'action=execute&nojs=1&working_dir='+working_dir+'&command='+escapeCGI(command));
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
/* TERMLIB-END */
</script>
</head>

<!-- TERMLIB0-BEGIN -->
<body onload="init()">
<!-- TERMLIB0-END -->
<!-- TERMLIB-BEGIN -->
<body onload="termOpen()">
<!-- TERMLIB-END -->
<form action="javascript:{}">
<table id="container">
<tr><td colspan="2" id="header">
	<div class="title"><a href="/">Tomato64</a></div>
	<div class="version">Version <% version(); %> on <% nv("t_model_name"); %><span class="blinking bl2"><script><% anonupdate(); %> anon_update()</script>&nbsp;</span></div>
</td></tr>
<tr id="body"><td id="navi"><script>navi()</script></td>
<td id="content">
<div id="ident"><% ident(); %> | <script>wikiLink();</script></div>

<!-- / / / -->

<div class="section-title">Execute System Commands</div>
<div class="section">
<!-- TERMLIB0-BEGIN -->
	<script>
	createFieldTable('', [
		{ title: 'Command', name: 'f_cmd', type: 'textarea', wrap: 'pre', value: '' }
	]);
	</script>
	<div style="float:left"><input type="button" value="Execute" onclick="execute()" id="execb"></div>
	<script>genStdRefresh(1,5,'ref.toggle()');</script>
<!-- TERMLIB0-END -->
<!-- TERMLIB-BEGIN -->
	<div id="term-div"></div>

	<div class="note-spacer"><a id="term-helper-link" href="#" onclick="toggleHWKeyHelper();">No hardware keyboard</a></div>

	<div id="term-helper-div" style="display:none">
		<input type="button" class="term-helper-button" onclick="fakecommand()" value="Enter">
		<input type="button" class="term-helper-button" onclick="sendSpace()" value="Space"><br>
		<input type="text" id="term-helper-icommands" name="commands" spellcheck="false" style="text-transform:none">
	</div>
<!-- TERMLIB-END -->

</div>

<!-- / / / -->

<div id="wait">Please wait...&nbsp; <img src="spin.svg" alt="" id="spin"></div>
<!-- TERMLIB0-BEGIN -->
<pre id="result"></pre>
<!-- TERMLIB0-END -->

<!-- / / / -->

<div id="footer">
	&nbsp;
</div>

</td></tr>
</table>
</form>
<script>insOvl()</script>
</body>
</html>
