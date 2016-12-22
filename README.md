[![Build Status](https://travis-ci.org/jipp/sonoffTH.svg?branch=master)](https://travis-ci.org/jipp/sonoffTH)
[![GitHub issues](https://img.shields.io/github/issues/jipp/sonoffTH.svg)](https://github.com/jipp/sonoffTH/issues)
[![GitHub license](https://img.shields.io/badge/license-MIT-blue.svg)](https://raw.githubusercontent.com/jipp/sonoffTH/master/LICENSE)

# sonoffTH
Firmware for the sonoffTH (based on ESP8266).

## The following features are implemented
* start WiFiManager if wifi access is not possible
* sending data to MQTT broker (username/password is required)
* LED blinking shows progress and status
* manual switch status by pressing bottom
* OTA update from webserver during startup
* OTA update triggered from IDE
* reset settings when buttom pressed during startup for 3 sec
* connect DHT22 sensor to jack for measurements
* publish switch status, temperature, humidity and vcc
* username/password for mqtt broker optional

## The following features are in progress
* react better when mqtt broker is not responding (this seems to be a timeout when mqtt is reconnecting)
* add last will for mqtt
* CI for Project
* maybe start on-demand wifi-setup instead of resetting all settings
* maybe in case mqtt dies in operation, reconnect when switch changes

## Additional files
-

## Needed Libraries
* [ 18  ] Adafruit DHT Unified
* [ 31  ] Adafruit Unified Sensor
* [ 64  ] ArduinoJson
* [ 19  ] DHT sensor library
* [ 89  ] PubSubClient
* [ 560 ] Streaming
* [1265 ] WiFiManager

## Board Settings
### sonoffTH - ESP8266: 1M (64k SPIFFS)
* gpio 0  -> button
* gpio 12 -> relay
* gpio 13 -> blue LED
* gpio 14 -> jack

### Witty Cloud Modul - ESP8266 12F
* ADC     -> LDR
* gpio 2  -> buildin blue LED
* gpio 4  -> button
* gpio 12 -> green LED
* gpio 13 -> blue LED
* gpio 15 -> red LED

### HUZZAH ESP8266 breakout
add an additional pull-up-resistor for the gpio button, otherwise the button will not work as expected.

## Info
### enable verbose output (default not defined)
* #define VERBOSE

### enable deep sleep (default not defined)
* #define DEEPSLEEP

### default gpio settings
* ADC -> free
* #define BUTTON  0   -> gpio 0
* #define RELAY   12  -> gpio 12 (relay and red LED)
* #define LED     13  -> gpio 13 (blue LED)
* #define JACK    14  -> gpio 14

### default settings for OTA at startup via webserver
* #define SERVER  "lemonpi"
* #define PORT    80
* #define PATH    "/esp/update/arduino.php"

### default DHT settings (allowed values: DHT11, DHT21, DHT22)
* #define DHTTYPE DHT22

### level when LED is off
* #define LEDOFF  HIGH

## tested environments
### boards
* sonoffTH
* huzzah
* witty cloud

## sensors
* DHT22
