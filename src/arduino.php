<?php

header('Content-type: text/plain; charset=utf8', true);

require_once('sqlite3.php');

function test() {
  $_SERVER = array(
    "HTTP_USER_AGENT" => "ESP8266-http-Update",
    "HTTP_X_ESP8266_STA_MAC" => "18:FE:AA:AA:AA:AA",
    "HTTP_X_ESP8266_AP_MAC" => "1A:FE:AA:AA:AA:AA",
    "HTTP_X_ESP8266_FREE_SPACE" => "671744",
    "HTTP_X_ESP8266_SKETCH_SIZE" => "373940",
    "HTTP_X_ESP8266_CHIP_SIZE" => "524288",
    "HTTP_X_ESP8266_SDK_VERSION" => "DOOR-7-g14f53a19",
    "HTTP_X_ESP8266_VERSION" => "esp8266-v1.3.1",
    "SERVER_PROTOCOL" => "HTTP/1.0"
  );
}

function check_header($name, $value = false) {
  if (!isset($_SERVER[$name])) {
    return false;
  }
  if ($value && $_SERVER[$name] != $value) {
    return false;
  }
  return true;
}

function sendFile($path) {
  header($_SERVER["SERVER_PROTOCOL"].' 200 OK', true, 200);
  header('Content-Type: application/octet-stream', true);
  header('Content-Disposition: attachment; filename='.basename($path));
  header('Content-Length: '.filesize($path), true);
  header('x-MD5: '.md5_file($path), true);
  readfile($path);
}

//test();
$sql = new DB('arduino.db');
$sql->create('arduino.db');

if (!check_header('HTTP_USER_AGENT', 'ESP8266-http-Update')) {
  header($_SERVER["SERVER_PROTOCOL"].' 403 Forbidden', true, 403);
  echo "only for ESP8266 updater!\n";
  exit();
}

if (!check_header('HTTP_X_ESP8266_STA_MAC') ||
!check_header('HTTP_X_ESP8266_AP_MAC') ||
!check_header('HTTP_X_ESP8266_FREE_SPACE') ||
!check_header('HTTP_X_ESP8266_SKETCH_SIZE') ||
!check_header('HTTP_X_ESP8266_CHIP_SIZE') ||
!check_header('HTTP_X_ESP8266_SDK_VERSION') ||
!check_header('HTTP_X_ESP8266_VERSION')) {
  header($_SERVER ["SERVER_PROTOCOL"].' 403 Forbidden', true, 403);
  echo "only for ESP8266 updater! (header)\n";
  exit();
}

if ($sql->checkMac($_SERVER['HTTP_X_ESP8266_STA_MAC'])==0) {
  header($_SERVER["SERVER_PROTOCOL"].' 500 not in table', true, 500);
  $sql->insertLog($_SERVER['HTTP_X_ESP8266_STA_MAC'], "not in table");
  $sql->insertVersion($_SERVER['HTTP_X_ESP8266_STA_MAC'], $_SERVER['HTTP_X_ESP8266_VERSION'], "automatically added");
} else {
  if ($sql->returnVersion($_SERVER['HTTP_X_ESP8266_STA_MAC']) != $_SERVER['HTTP_X_ESP8266_VERSION']) {
    if (file_exists("./bin/".$sql->returnVersion($_SERVER['HTTP_X_ESP8266_STA_MAC']).".bin")) {
      sendFile("./bin/".$sql->returnVersion($_SERVER['HTTP_X_ESP8266_STA_MAC']).".bin");
      $sql->insertLog($_SERVER['HTTP_X_ESP8266_STA_MAC'], $_SERVER['HTTP_X_ESP8266_VERSION']." -> ".$sql->returnVersion($_SERVER['HTTP_X_ESP8266_STA_MAC'])." version modified");
    } else {
      header($_SERVER["SERVER_PROTOCOL"].' 500 no file available', true, 500);
      $sql->insertLog($_SERVER['HTTP_X_ESP8266_STA_MAC'], $sql->returnVersion($_SERVER['HTTP_X_ESP8266_STA_MAC']).".bin no file available");
    }
  } else {
    header($_SERVER["SERVER_PROTOCOL"].' 304 version not modified', true, 304);
    $sql->insertLog($_SERVER['HTTP_X_ESP8266_STA_MAC'], "version not modified");
  }
}

?>
