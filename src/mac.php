<html>
  <head>
    <title>mac table</title>
  </head>
  <body>
    <form method="post" action="<?php echo htmlspecialchars($_SERVER["PHP_SELF"]);?>">
    <?php
      $mac = "";

      function test_input($data) {
        $data = trim($data);
        $data = stripslashes($data);
        $data = htmlspecialchars($data);
        return $data;
      }

      if ($_SERVER["REQUEST_METHOD"] == "POST") {
        $mac = test_input($_POST["mac"]);
      }
    ?> 

    <form method="post" action="<?php echo htmlspecialchars($_SERVER["PHP_SELF"]);?>">
      mac: <input type="text" name="mac">
    </form>

    <table border=1>
      <tr><th>mac</th><th>ip</th></tr>

    <?php
      exec("arp -n | grep -i $mac | awk '{ print \"<tr><td>\" $3 \"</td><td>\" $1 \"</td></tr>\" }'", $var);
      foreach($var as $key => $value) {
        echo $value;
      }
    ?>

    </table>
  </body>
</html>
