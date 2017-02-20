<html><head><title>Test</title>
<link rel="stylesheet" type="text/css" href="style.css">
</head>
<body>
<div id="main">
<h1>The LED</h1>
<p>
RTL-00/01 module has a led connected to PA4, it's now <b>%ledstate%</b>. You can change that using the buttons below.
</p>
<form method="post" action="led.cgi">
<input type="submit" name="led" value="1">
<input type="submit" name="led" value="0">
</form>
</div>
</body></html>
