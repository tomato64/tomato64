<!DOCTYPE html>
<html lang="en-GB">
<head>
<meta http-equiv="content-type" content="text/html;charset=utf-8">
<meta name="robots" content="noindex,nofollow">
<title>[<% ident(); %>] Error</title>
<link rel="stylesheet" type="text/css" href="tomato.css?rel=<% version(); %>">
<% css(); %>
<style>
div.tomato-grid.container-div {
	height: 90px;
}
</style>
</head>

<body>
<div class="tomato-grid container-div">
	<div class="wrapper1">
		<div class="wrapper2">
			<div class="info-centered">
				<script>
//	<% resmsg('Unknown error'); %>
					document.write(resmsg);
				</script>&nbsp;
				<input type="button" value="Back" onclick="history.go(-1)">
			</div>
		</div>
	</div>
</div>
</body>
</html>
