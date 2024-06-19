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
<title>[<% ident(); %>] Logout...</title>
<link rel="stylesheet" type="text/css" href="tomato.css?rel=<% version(); %>">
<% css(); %>
<style>
div.tomato-grid.container-div {
	height: 180px;
}
</style>
</head>

<body onload='setTimeout("go.submit()", 7200)'>
<div class="tomato-grid container-div">
	<div class="wrapper1">
		<div class="wrapper2">
			<div class="info-centered">
				<b>Logout</b>
				<br>
				<hr style="height:1px">
				To clear the credentials cached by the browser:<br>
				<br>
				<b>Firefox, Internet Explorer, Opera, Safari</b><br>
				- Leave the password field blank.<br>
				- Click OK/Login<br>
				<br>
				<b>Chrome</b><br>
				- Select Cancel.
				<form action="logout" name="go" method="post">
					<div>
						<input type="hidden" name="_http_id" value="<% nv(http_id); %>">
					</div>
				</form>
			</div>
		</div>
	</div>
</div>
</body>
</html>
