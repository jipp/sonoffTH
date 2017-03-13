<html>
  <head>
    <title>log table</title>
  </head>
  <body>
    <?php
      require_once('sqlite3.php');
      $sql = new DB('arduino.db');
      $sql->create('arduino.db');
      $sql->selectAll('log');
    ?> 
  </body>
</html>
