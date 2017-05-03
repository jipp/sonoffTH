[![Build Status](https://travis-ci.org/jipp/sonoffTH.svg?branch=master)](https://travis-ci.org/jipp/sonoffTH)

[![GitHub issues](https://img.shields.io/github/issues/jipp/sonoffTH.svg)](https://github.com/jipp/sonoffTH/issues)
[![GitHub forks](https://img.shields.io/github/forks/jipp/sonoffTH.svg)](https://github.com/jipp/sonoffTH/network)
[![GitHub stars](https://img.shields.io/github/stars/jipp/sonoffTH.svg)](https://github.com/jipp/sonoffTH/stargazers)
[![GitHub license](https://img.shields.io/badge/license-MIT-blue.svg)](https://raw.githubusercontent.com/jipp/sonoffTH/master/LICENSE)

# sonoffTH
Firmware for the sonoffTH (based on ESP8266) written with PlatformIO, to be used as well with other boards.

## IDE
* PlatformIO IDE: used for developement
* Arduino IDE: copy content of main.cpp into Arduino IDE

## The following features are implemented
* start WiFiManager if wifi access is not possible
* sending data to MQTT broker (username/password is required)
* LED blinking shows progress and status
* manual switch status by pressing bottom
* OTA update from webserver during startup
* OTA update triggered from IDE
* reset settings when buttom pressed for 3 sec
* connect sensor to jack for measurements
* publish switch status, temperature, humidity and vcc
* username/password for mqtt broker optional
* Deepsleep for longer battery live

## The following features are in progress
* react faster when mqtt broker is not responding (this seems to be a timeout when mqtt is reconnecting)
* add last will for mqtt
* maybe in case mqtt dies in operation, reconnect when switch changes
* tell when woke up after Deepsleep

## Additional files
For OTA update triggered by the esp a set of php files are needed. Below is a list of the php- and bin-files and how they are placed inside the file system (using lighttpd with php):

```
/var/www/html/esp/
/var/www/html/esp/update
/var/www/html/esp/update/esp.php
/var/www/html/esp/update/log.php
/var/www/html/esp/update/index.php
/var/www/html/esp/update/mac.php
/var/www/html/esp/update/arduino.php
/var/www/html/esp/update/arduino.db
/var/www/html/esp/update/sqlite3.php
/var/www/html/esp/update/bin
/var/www/html/esp/update/bin/sonoffTH-sonoffTH-v0.4.1.bin
/var/www/html/esp/update/bin/sonoffTH-esp12e-v0.4.0.bin
/var/www/html/esp/update/bin/sonoffTH-esp12e-v0.4.1.bin
/var/www/html/esp/update/bin/sonoffTH-witty-v0.4.1.bin
/var/www/html/esp/update/bin/sonoffTH-huzzah-v0.4.1.bin
/var/www/html/esp/update/bin/sonoffTH-sonoffTH-v0.4.0.bin
/var/www/html/esp/update/bin/sonoffTH-witty-v0.4.0.bin
/var/www/html/esp/update/bin/sonoffTH-wemosPro-v0.4.1.bin
/var/www/html/esp/update/bin/sonoffTH-wemosPro-v0.4.0.bin
/var/www/html/esp/update/bin/esp8266-v1.3.1.bin
/var/www/html/esp/update/bin/sonoffTH-huzzah-v0.4.0.bin
/var/www/html/esp/update/arp.php
```

The .db file is automatically created first time an esp tries to update via OTA and calls the arduino.php script.

## Needed Libraries
* [  1  ] OneWire
* [ 19  ] DHT sensor library
* [ 31  ] Adafruit Unified Sensor
* [ 54  ] DallasTemperature
* [ 64  ] ArduinoJson
* [ 89  ] PubSubClient
* [ 560 ] Streaming
* [ 567 ] WifiManager
* [ 962 ] mbed-drivers
* [1265 ] WiFiManager

## Board Settings
### sonoffTH - ESP8266: 1M (64k SPIFFS)
* gpio 0  -> button
* gpio 12 -> relay
* gpio 13 -> blue LED
* gpio 14 -> jack

### Witty Cloud Modul - ESP8266 12F
* ADC     -> LDR
* gpio 2  -> build-in blue LED
* gpio 4  -> button
* gpio 12 -> green LED
* gpio 13 -> blue LED
* gpio 15 -> red LED

### HUZZAH ESP8266 breakout

### esp12e
check out the wiring here:
http://www.esp8266.com/wiki/doku.php?id=getting-started-with-the-esp8266

### WeMos D1 mini pro
* D0 -> gpio 16
* D1 -> gpio 5
* D2 -> gpio 4
* D3 -> gpio 0
* D4 -> gpio 2
* D5 -> gpio 14
* D6 -> gpio 12
* D7 -> gpio 13
* D8 -> gpio 15
For Deepsleep connect D0 -> RST

## Info
### enable verbose output (default not defined)
* #define VERBOSE

### enable deep sleep (default not defined)
* #define DEEPSLEEP 900

### default gpio settings
* ADC -> voltage measurement
* #define BUTTON  0   -> gpio 0
* #define RELAY   12  -> gpio 12 (relay and red LED)
* #define LED     13  -> gpio 13 (blue LED)
* #define JACK    14  -> gpio 14

### default settings for OTA at startup via webserver
* #define SERVER  "lemonpi"
* #define PORT    80
* #define PATH    "/esp/update/arduino.php"

### level when LED is off
* #define LEDOFF  HIGH

## tested hardware
### boards
* esp12e
* sonoffTH
* HUZZAH ESP8266 breakout
* Witty Cloud Modul
* WeMos D1 Mini Pro

### sensors
* DHT11
* DHT21
* DHT22
* DS1822
* DS18B22
* DS18S22
