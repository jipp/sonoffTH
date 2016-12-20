[![Build Status](https://travis-ci.org/jipp/sonoffTH.svg?branch=master)](https://travis-ci.org/jipp/sonoffTH)

# sonoffTH
Firmware for the sonoffTH (based on ESP8266).

## The following features are implemented
* start WiFiManager if wifi access is not possible
* sending data to MQTT broker (username/password is required)
* LED blinking shows progress and status
* manual switch status by pressing bottom
* OTA update from webserver during startup
* reset settings when buttom pressed during startup for 3 sec
* connect DHT22 sensor to jack for measurements
* publish switch status, temperature, humidity and vcc

## The following features are in progress
* react better when mqtt broker is not responding (this seems to be a timeout when mqtt is reconnecting)
* add last will for mqtt
* CI for Project
* maybe start on-demand wifi instead of resetting all settings
* in case mqtt dies in operation, reconnect when switch changes
* enhance build system to automatic versioning and nameing of .bin file
* make username/password for mqtt broker optional
* add OTA triggered from IDE

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

## Info
### settings for the sonoffTH
* ESP8266: 1M (64k SPIFFS)

### enable verbose output
* #define VERBOSE

## enable deep sleep
* #define DEEPSLEEP

### gpio settings used by default
* #define BUTTON  0  -> gpio 0
* #define RELAY 12  -> gpio 12 (relay and red LED)
* #define LED 13  -> gpio 13 (blue LED)
* #define JACK  14  -> gpio 14

### settings for OTA at startup
* #define SERVER  "lemonpi"
* #define PORT    80
* #define PATH    "/esp/update/arduino.php"

### DHT settings
* #define DHTTYPE DHT22

### LED off
* #define LEDOFF  HIGH
