<html>
<head>
	<title>OTA ESP</title>
</head>
<body>
	<?php
	require_once('sqlite3.php');
	$sql = new DB('arduino.db');
	$sql->create('arduino.db');
	echo "<h1>esp table:</h1>\n";
	$sql->selectAll('esp');
	echo "<h1>log table:</h1>\n";
	$sql->selectAll('log');
	?>
</body>
</html>
