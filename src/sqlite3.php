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
    $create = "CREATE TABLE IF NOT EXISTS esp (mac TEXT PRIMARY KEY, version TEXT, comment TEXT)";
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

  function selectAll() {
    $results = $this->sqlite3->query('SELECT * FROM esp');
    echo "<table border=1> <tr> <th>mac</th> <th>version</th> <th>comment</th> </tr>";
    while ($row = $results->fetchArray()) {
      echo "<tr> <td>".$row["mac"]."</td><td>".$row["version"]."</td><td>".$row["comment"]."</td></tr>";
    }
    echo "</table>";
  }

  function insertVersion($mac, $version, $comment) {
    $insert = "INSERT INTO esp VALUES ('$mac', '$version', '$comment')";
    $this->sqlite3->exec($insert);
  }

  function updateVersion($mac, $version) {
    $update = "UPDATE esp SET version = '$version' where mac = '$mac'";
    $this->sqlite3->exec($update);
  }
}

?>
