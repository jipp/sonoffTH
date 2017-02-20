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
    "HTTP_X_ESP8266_VERSION" => "1.3.0",
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

if ($sql->checkMac($_SERVER['HTTP_X_ESP8266_STA_MAC'])==1) {
  if ($sql->returnVersion($_SERVER['HTTP_X_ESP8266_STA_MAC']) != $_SERVER['HTTP_X_ESP8266_VERSION']) {
    if (file_exists("./bin/".$sql->returnVersion($_SERVER['HTTP_X_ESP8266_STA_MAC']).".bin")) {
      sendFile("./bin/".$sql->returnVersion($_SERVER['HTTP_X_ESP8266_STA_MAC']).".bin");
      file_put_contents('update.log', date('d.m.Y H:i:s', time())."\t".$_SERVER['HTTP_X_ESP8266_STA_MAC']."\t".$sql->returnVersion($_SERVER['HTTP_X_ESP8266_STA_MAC'])."\n", FILE_APPEND | LOCK_EX );
    } else {
      header($_SERVER["SERVER_PROTOCOL"].' 500 no file for ESP MAC', true, 500);
    }
  } else {
    header($_SERVER["SERVER_PROTOCOL"].' 304 Not Modified', true, 304);
  }
} else {
  if ($sql->checkMac($_SERVER['HTTP_X_ESP8266_STA_MAC'])==0) {
    $sql->insertVersion($_SERVER['HTTP_X_ESP8266_STA_MAC'], $_SERVER['HTTP_X_ESP8266_VERSION'], "automatic added");
  }
  header($_SERVER["SERVER_PROTOCOL"].' 500 no version for ESP MAC', true, 500);
}
