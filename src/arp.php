<html>
  <head>
    <title>OTA ESP</title>
  </head>
  <body>
    <?php
#echo exec("arp -an | grep -i \"5C:CF:7F:24:2E:1E\" | awk '{print $1}'");
echo exec("cat /proc/net/arp | grep \"ac:bc:32:d5:9f:7d\" | awk '{print $1}'");
#echo exec("arp -an | grep -i \"ac:bc:32:d5:9f:7d\" | awk '{print $1}'");
    ?> 
  </body>
</html>
