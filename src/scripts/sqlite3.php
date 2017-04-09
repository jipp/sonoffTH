<?php

class DB {
  private $sqlite3;
  private $mode;

  function __construct($filename, $mode=SQLITE3_ASSOC) {
    $this->mode = $mode;
    $this->sqlite3 = new SQLite3($filename);
  }

  function __destruct() {
    @$this->sqlite->close();
  }

  function create() {
	$create = "CREATE TABLE IF NOT EXISTS esp (id INTEGER PRIMARY KEY AUTOINCREMENT, mac TEXT NOT NULL, version TEXT NOT NULL, comment TEXT)";
    $this->sqlite3->exec($create);
	$create = "CREATE TABLE IF NOT EXISTS log (id INTEGER PRIMARY KEY AUTOINCREMENT, time TEXT NOT NULL DEFAULT (strftime('%Y-%m-%d %H:%M:%f','now', 'localtime')), mac TEXT NOT NULL, comment TEXT)";
//	$create = "CREATE TABLE IF NOT EXISTS log (id INTEGER PRIMARY KEY AUTOINCREMENT, time TEXT NOT NULL, mac TEXT NOT NULL, comment TEXT)";
    $this->sqlite3->exec($create);
  }

  function checkMac($mac) {
    $count = $this->sqlite3->querySingle("SELECT COUNT(*) FROM esp WHERE mac = '$mac'");
    return $count;
  }

  function returnVersion($mac) {
    $version = $this->sqlite3->querySingle("SELECT version FROM esp WHERE mac = '$mac'");
    return $version;
  }

  function selectAll($table) {
	$count = $this->sqlite3->querySingle("SELECT COUNT(*) FROM '$table'");
	echo "Number of entries: ".$count."<br>\n";
    echo "<table border=1>\n";
    echo "<tr> ";
    $pragma = $this->sqlite3->query("PRAGMA table_info('$table')");
    while($name = $pragma->fetchArray(SQLITE3_ASSOC)) {
      echo "<th>". $name['name']."</th> ";
    }
    echo "</tr>\n";
    $results = $this->sqlite3->query("SELECT * FROM '$table'");
    while($row = $results->fetchArray(SQLITE3_ASSOC)) {
      echo "<tr> ";
      foreach($row as $value) {
        echo "<td>".$value."</td> ";
      }
      echo "</tr>\n";
    }
    echo "</table>\n";
  }

  function insertLog($mac, $comment) {
	$insert = "INSERT INTO log (mac, comment) VALUES ('$mac', '$comment')";
//    $insert = "INSERT INTO log (time, mac, comment) VALUES (strftime('%Y-%m-%d %H:%M:%f','now', 'localtime'), '$mac', '$comment')";
    $this->sqlite3->exec($insert);
  }

  function insertVersion($mac, $version, $comment) {
    $insert = "INSERT INTO esp (mac, version, comment) VALUES ('$mac', '$version', '$comment')";
    $this->sqlite3->exec($insert);
  }

  function updateVersion($mac, $version) {
    $update = "UPDATE esp SET version = '$version' where mac = '$mac'";
    $this->sqlite3->exec($update);
  }
}

?>
