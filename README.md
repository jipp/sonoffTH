[![Build Status](https://travis-ci.org/jipp/sonoffTH.svg?branch=master)](https://travis-ci.org/jipp/sonoffTH)

# sonoffTH
Firmware for the sonoffTH (based on ESP8266).

## The following features are implemented
* start WiFiManager if wifi access is not possible
* sending data to MQTT broker
* LED blink shows progress
* manual switch by pressing bottom
* OTA update from webserver (requested by device)
* reset settings when buttom pressed during startup

## The following features are in progress
* react better when mqtt broker is not responding
* add last will for mqtt
* parameter the OTA
* CI for Project
* maybe start ondemand wifi instead of resetting all settings

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
-
