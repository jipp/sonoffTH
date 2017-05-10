#include "Arduino.h"

// defines
#define DS1822  0x22
#define DS18B20 0x28
#define DS18S20 0x10
#define LUX     0x6D6
#define SHT3X   0x11

#if (DHTSENSOR == DHT11) || (DHTSENSOR == DHT21) || (DHTSENSOR == DHT22)
#include <DHT.h>
#endif
#if (ONEWIRESENSOR == DS1822) || (ONEWIRESENSOR == DS18B20) || (ONEWIRESENSOR == DS18S20)
#include <OneWire.h>
#include <DallasTemperature.h>
#endif
#if (I2CSENSOR == LUX)
#include <Wire.h>
#include <BH1750.h>
#elif (I2CSENSOR == SHT3X)
#include <Wire.h>
#endif

#ifndef VERSION
#define VERSION "sonoffTH"
#endif
#ifndef BUTTON
#define BUTTON  0
#endif
#ifndef RELAY
#define RELAY   12
#endif
#ifndef LED
#define LED     13
#endif
#ifndef DHTJACK
#define DHTJACK    14
#endif
#ifndef ONEWIREJACK
#define ONEWIREJACK    14
#endif
#ifndef LEDOFF
#define LEDOFF  HIGH
#endif
#ifndef SERVER
#define SERVER  "lemonpi"
#endif
#ifndef PORT
#define PORT    80
#endif
#ifndef PATH
#define PATH    "/esp/update/arduino.php"
#endif


// libraries
#include <Streaming.h>
#include <Ticker.h>
#include <FS.h>
#include <ESP8266WiFi.h>
#include <DNSServer.h>
#include <ESP8266WebServer.h>
#include <WiFiManager.h>
#include <ArduinoJson.h>
#include <ESP8266httpUpdate.h>
#include <PubSubClient.h>
#include <EEPROM.h>
#include <ESP8266mDNS.h>
#include <WiFiUdp.h>
#include <ArduinoOTA.h>


// constants
const int address = 0;
const char file[]="/config.json";
const unsigned long timerMeasureIntervall = 60;
const unsigned long timerLastReconnect = 60;
const unsigned long timerButtonPressedReset = 3;
const int qos = 1;


// switch adc port to monitor vcc
ADC_MODE(ADC_VCC);


// global definitions
Ticker ticker;
PubSubClient pubSubClient;
#if (DHTSENSOR == DHT11) || (DHTSENSOR == DHT21) || (DHTSENSOR == DHT22)
DHT dht(DHTJACK, DHTSENSOR);
#endif
#if (ONEWIRESENSOR == DS1822) || (ONEWIRESENSOR == DS18B20) || (ONEWIRESENSOR == DS18S20)
OneWire oneWire(ONEWIREJACK);
DallasTemperature dallasTemperature(&oneWire);
#endif
#if (I2CSENSOR == LUX)
BH1750 lightMeter;
#endif
WiFiClient wifiClient;


// global variables
bool shouldSaveConfig = false;
char id[13];
unsigned long timerMeasureIntervallStart = 0;
unsigned long timerLastReconnectStart = 0;
unsigned long timerButtonPressedStart = 0;
bool calculate = false;
char mqtt_server[40] = "test.mosquitto.org";
char mqtt_username[16] = "";
char mqtt_password[16] = "";
char mqtt_port[6] = "1883";
String subscribeSwitchTopic = "/switch/command";
String publishSwitchTopic = "/switch/state";
String publishVccTopic = "/vcc/value";
String publishTemperatureTopic = "/temperature/value";
String publishHumidityTopic = "/humidity/value";
String publishLuxTopic = "/lux/value";


// callback functions
void tick();
void saveConfigCallback ();
void callback(char*, byte*, unsigned int);


// functions
void setupHardware();
void printSettings();
void readSwitchStateEEPROM();
void writeSwitchStateEEPROM();
void setupPubSub();
void checkForConfigReset();
void resetConfig();
void publishSwitchState();
void setupID();
void publishValues();
bool connect();
void updater();
void finishSetup();
void setupTopic();
void shutPubSub();
void saveConfig();
void setupOTA();
void goDeepSleep();
void setupWiFiManager(bool);
void readConfig();


void setup() {
  setupHardware();
  printSettings();
  readSwitchStateEEPROM();
  checkForConfigReset();
  setupWiFiManager(true);
  updater();
  setupID();
  setupPubSub();
  setupTopic();
  finishSetup();
  connect();
  #ifdef DEEPSLEEP
  goDeepSleep();
  #endif
  setupOTA();
}

void loop() {
  ArduinoOTA.handle();
  if (WiFi.status() == WL_CONNECTED) {
    if (!pubSubClient.connected()) {
      if (millis() - timerLastReconnectStart > timerLastReconnect * 1000) {
        timerLastReconnectStart = millis();
        if (connect()) {
          timerLastReconnectStart = 0;
        }
      }
    } else {
      pubSubClient.loop();
      if (millis() - timerMeasureIntervallStart > timerMeasureIntervall * 1000) {
        timerMeasureIntervallStart = millis();
        publishValues();
      }
    }
  }
  if (!calculate and (digitalRead(BUTTON) == LOW)) {
    timerButtonPressedStart = millis();
    calculate = true;
  }
  if (calculate and (digitalRead(BUTTON) == HIGH)) {
    if (millis() - timerButtonPressedStart > timerButtonPressedReset * 1000) {
      calculate = false;
      resetConfig();
    } else if (millis() - timerButtonPressedStart > 0) {
      calculate = false;
      digitalWrite(RELAY, !digitalRead(RELAY));
      writeSwitchStateEEPROM();
      publishSwitchState();
      Serial << "Switch state: " << digitalRead(RELAY) << endl;
    }
  }
}


void tick() {
  int state = digitalRead(LED);

  digitalWrite(LED, !state);
}

void saveConfigCallback () {
  shouldSaveConfig = true;
  Serial << "Should save config" << endl;
}

void callback(char* topic, byte* payload, unsigned int length) {
  Serial << " > " << topic << ": ";
  for (unsigned int i = 0; i < length; i++) {
    Serial << (char) payload[i];
  }
  Serial << endl;
  if ((payload[0] == '0' ? false : true) != digitalRead(RELAY)) {
    digitalWrite(RELAY, !digitalRead(RELAY));
    publishSwitchState();
    writeSwitchStateEEPROM();
  }
}

void setupHardware() {
  Serial.begin(115200);
  ticker.attach(0.3, tick);
  #if (DHTSENSOR == DHT11) || (DHTSENSOR == DHT21) || (DHTSENSOR == DHT22)
  dht.begin();
  #endif
  #if (ONEWIRESENSOR == DS1822) || (ONEWIRESENSOR == DS18B20) || (ONEWIRESENSOR == DS18S20)
  dallasTemperature.begin();
  #endif
  #if (I2CSENSOR == LUX)
  lightMeter.begin();
  #elif (I2CSENSOR == SHT3X)
  Wire.begin();
  #endif
  pinMode(BUTTON, INPUT_PULLUP);
  pinMode(RELAY, OUTPUT);
  pinMode(LED, OUTPUT);
}

void printSettings() {
  Serial << endl << endl << "RESETINFO: " << ESP.getResetInfo() << endl;
  Serial << endl << "VERSION: " << VERSION << endl;
  #ifdef DEEPSLEEP
  Serial << "DEEPSLEEP: " << DEEPSLEEP << "s" << endl;
  #endif
  Serial << "BUTTON: " << BUTTON << endl;
  Serial << "RELAY: " << RELAY << endl;
  Serial << "LED: " << LED << endl;
  Serial << "DHTJACK: " << DHTJACK << endl;
  Serial << "ONEWIREJACK: " << ONEWIREJACK << endl;
  #ifdef DHTSENSOR
  Serial << "DHT: " << DHTSENSOR << endl;
  #endif
  #ifdef ONEWIRESENSOR
  Serial << "ONEWIRESENSOR: " << ONEWIRESENSOR << endl;
  #endif
  #ifdef I2CSENSOR
  Serial << "I2CSENSOR: " << I2CSENSOR << endl;
  #endif
  Serial << "LEDOFF: " << LEDOFF << endl;
  Serial << "SERVER: " << SERVER << endl;
  Serial << "PORT: " << PORT << endl;
  Serial << "PATH: " << PATH << endl;
  Serial << endl;
}

void setupPubSub() {
  pubSubClient.setClient(wifiClient);
  pubSubClient.setServer(mqtt_server, String(mqtt_port).toInt());
  pubSubClient.setCallback(callback);
}

void readSwitchStateEEPROM() {
  EEPROM.begin(512);
  digitalWrite(RELAY, EEPROM.read(address));
}

void writeSwitchStateEEPROM() {
  EEPROM.write(address, digitalRead(RELAY));
  EEPROM.commit();
}

void publishSwitchState() {
  bool state = digitalRead(RELAY);

  if (pubSubClient.connected()) {
    if (pubSubClient.publish(publishSwitchTopic.c_str(), String(state).c_str())) {
      Serial << " < " << publishSwitchTopic << ": " << state << endl;
    } else {
      Serial << "!< " << publishSwitchTopic << ": " << state << endl;
    }
  }
}

void checkForConfigReset() {
  Serial << "waiting 1 sec for pressing button " << BUTTON << " to reset" << endl;
  delay(1000);
  timerButtonPressedStart =  millis();
  while (digitalRead(BUTTON) == LOW) {
    if (millis() - timerButtonPressedStart > 0) {
      Serial << "reset triggered" << endl;
      resetConfig();
    }
  }
  Serial << "done" << endl;
}

void resetConfig() {
  Serial << "reset config" << endl;
  setupWiFiManager(false);
  Serial << "reset done" << endl;
  yield();
}

void setupID() {
  byte mac[6];

  WiFi.macAddress(mac);
  sprintf(id, "%02x%02x%02x%02x%02x%02x", mac[0], mac[1], mac[2], mac[3],
  mac[4], mac[5]);
  Serial << "id: " << id << endl;
}

void publishValues() {
  unsigned int vcc = ESP.getVcc();
  #if (DHTSENSOR == DHT11) || (DHTSENSOR == DHT21) || (DHTSENSOR == DHT22)
  float temperature = dht.readTemperature();
  float humidity = dht.readHumidity();
  #elif (ONEWIRESENSOR == DS1822) || (ONEWIRESENSOR == DS18B20) || (ONEWIRESENSOR == DS18S20)
  float temperature;
  #endif
  #if (I2CSENSOR == LUX)
  unsigned short lux = lightMeter.readLightLevel();
  #elif (I2CSENSOR == SHT3X)
  unsigned int data[6];
  Wire.beginTransmission(0x45);
  Wire.write(0x2C);
  Wire.write(0x06);
  Wire.endTransmission();
  delay(500);
  Wire.requestFrom(0x45, 6);
  if (Wire.available() == 6) {
    data[0] = Wire.read();
    data[1] = Wire.read();
    data[2] = Wire.read();
    data[3] = Wire.read();
    data[4] = Wire.read();
    data[5] = Wire.read();
  }
  float temperature = ((((data[0] * 256.0) + data[1]) * 175) / 65535.0) - 45;
  float humidity = ((((data[3] * 256.0) + data[4]) * 100) / 65535.0);
  #endif

  if (pubSubClient.connected()) {
    if (pubSubClient.publish(publishVccTopic.c_str(), String(vcc).c_str())) {
      Serial << " < " << publishVccTopic << ": " << vcc << endl;
    } else {
      Serial << "!< " << publishVccTopic << ": " << vcc << endl;
    }
    #if (DHTSENSOR == DHT11) || (DHTSENSOR == DHT21) || (DHTSENSOR == DHT22) || (I2CSENSOR == SHT3X)
    if (!isnan(temperature) && pubSubClient.publish(publishTemperatureTopic.c_str(), String(temperature).c_str())) {
      Serial << " < " << publishTemperatureTopic << ": " << temperature << endl;
    } else {
      Serial << "!< " << publishTemperatureTopic << ": " << temperature << endl;
    }
    if (!isnan(humidity) && pubSubClient.publish(publishHumidityTopic.c_str(), String(humidity).c_str())) {
      Serial << " < " << publishHumidityTopic << ": " << humidity << endl;
    } else {
      Serial << "!< " << publishHumidityTopic << ": " << humidity << endl;
    }
    #elif (ONEWIRESENSOR == DS1822) || (ONEWIRESENSOR == DS18B20) || (ONEWIRESENSOR == DS18S20)
    dallasTemperature.requestTemperatures();
    temperature = dallasTemperature.getTempCByIndex(0);
    if (pubSubClient.publish(publishTemperatureTopic.c_str(), String(temperature).c_str())) {
      Serial << " < " << publishTemperatureTopic << ": " << temperature << endl;
    } else {
      Serial << "!< " << publishTemperatureTopic << ": " << temperature << endl;
    }
    #endif
    #if (I2CSENSOR == LUX)
    lux = lightMeter.readLightLevel();
    if (pubSubClient.publish(publishLuxTopic.c_str(), String(lux).c_str())) {
      Serial << " < " << publishLuxTopic << ": " << lux << endl;
    } else {
      Serial << "!< " << publishLuxTopic << ": " << lux << endl;
    }
    #endif
  }
}

bool connect() {
  bool connected = false;

  Serial << "Attempting MQTT connection (~5s) ..." << endl;
  if ((String(mqtt_username).length() == 0) || (String(mqtt_password).length() == 0)) {
    Serial << "no authentication" << endl;
    connected = pubSubClient.connect(id);
  } else {
    Serial << "authentication" << endl;
    connected = pubSubClient.connect(id, String(mqtt_username).c_str(), String(mqtt_password).c_str());
  }
  if (connected) {
    Serial << "connected, rc=" << pubSubClient.state() << endl;
    #ifndef DEEPSLEEP
    publishSwitchState();
    #endif
    publishValues();
    pubSubClient.subscribe(subscribeSwitchTopic.c_str(), qos);
    ticker.detach();
    digitalWrite(LED, LEDOFF);
  } else {
    Serial << "failed, rc=" << pubSubClient.state() << endl;
    ticker.attach(1.0, tick);
  }

  return pubSubClient.connected();
}

void updater() {
  if (WiFi.status() == WL_CONNECTED) {
    Serial << "try update" << endl;
    t_httpUpdate_return ret = ESPhttpUpdate.update(SERVER, PORT, PATH, VERSION);
    switch (ret) {
      case HTTP_UPDATE_FAILED:
      Serial << "HTTP_UPDATE_FAILD Error (" << ESPhttpUpdate.getLastError() << "): " << ESPhttpUpdate.getLastErrorString().c_str() << endl;
      break;
      case HTTP_UPDATE_NO_UPDATES:
      Serial << "HTTP_UPDATE_NO_UPDATES" << endl;
      break;
      case HTTP_UPDATE_OK:
      Serial << "HTTP_UPDATE_OK" << endl;
      break;
    }
  }
}

void finishSetup() {
  if (WiFi.status() == WL_CONNECTED) {
    ticker.detach();
    digitalWrite(LED, LEDOFF);
  } else {
    ticker.attach(0.5, tick);
  }
}

void setupTopic() {
  subscribeSwitchTopic = id + subscribeSwitchTopic;
  publishSwitchTopic = id + publishSwitchTopic;
  publishVccTopic = id + publishVccTopic;
  publishTemperatureTopic = id + publishTemperatureTopic;
  publishHumidityTopic = id + publishHumidityTopic;
  publishLuxTopic = id + publishLuxTopic;
}

void shutPubSub() {
  if (pubSubClient.connected()) {
    pubSubClient.unsubscribe(subscribeSwitchTopic.c_str());
    pubSubClient.disconnect();
  }
}

void saveConfig() {
  Serial << "saving config" << endl;
  DynamicJsonBuffer jsonBuffer;
  JsonObject& json = jsonBuffer.createObject();
  File configFile = SPIFFS.open(file, "w");

  if (configFile) {
    json["mqtt_server"] = mqtt_server;
    json["mqtt_port"] = mqtt_port;
    json["mqtt_username"] = mqtt_username;
    json["mqtt_password"] = mqtt_password;
    #ifdef VERBOSE
    json.prettyPrintTo(Serial);
    Serial << endl;
    #endif
    json.printTo(configFile);
    configFile.close();
  } else {
    Serial << "failed to open config file for writing" << endl;
  }
}

void setupOTA() {
  ArduinoOTA.onStart([]() {
    Serial << "Start" << endl;
  });
  ArduinoOTA.onEnd([]() {
    Serial << "End" << endl;
  });
  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
    Serial << "Progress: " << (progress / (total / 100)) << "%" << endl;
  });
  ArduinoOTA.onError([](ota_error_t error) {
    Serial << "Error[" << error << "]: ";
    if (error == OTA_AUTH_ERROR) Serial << "Auth Failed" << endl;
    else if (error == OTA_BEGIN_ERROR) Serial << "Begin Failed" << endl;
    else if (error == OTA_CONNECT_ERROR) Serial << "Connect Failed" << endl;
    else if (error == OTA_RECEIVE_ERROR) Serial << "Receive Failed" << endl;
    else if (error == OTA_END_ERROR) Serial << "End Failed" << endl;
  });
  ArduinoOTA.begin();
}

void setupWiFiManager(bool autoConnect) {
  ticker.attach(0.1, tick);
  readConfig();
  WiFiManagerParameter custom_mqtt_server("server", "mqtt server", mqtt_server, 40);
  WiFiManagerParameter custom_mqtt_port("port", "mqtt port", mqtt_port, 6);
  WiFiManagerParameter custom_mqtt_username("username", "mqtt username", mqtt_username, 16);
  WiFiManagerParameter custom_mqtt_password("password", "mqtt password", mqtt_password, 16);
  WiFiManager wifiManager;
  #ifdef VERBOSE
  wifiManager.setDebugOutput(true);
  #else
  wifiManager.setDebugOutput(false);
  #endif
  wifiManager.setSaveConfigCallback(saveConfigCallback);
  wifiManager.addParameter(&custom_mqtt_server);
  wifiManager.addParameter(&custom_mqtt_port);
  wifiManager.addParameter(&custom_mqtt_username);
  wifiManager.addParameter(&custom_mqtt_password);
  if (autoConnect) {
    wifiManager.setTimeout(180);
    if (!wifiManager.autoConnect("AutoConnectAP")) {
      Serial << "failed to connect and hit timeout" << endl;
    } else {
      Serial << "WiFi connected" << endl;
      strcpy(mqtt_server, custom_mqtt_server.getValue());
      strcpy(mqtt_port, custom_mqtt_port.getValue());
      strcpy(mqtt_username, custom_mqtt_username.getValue());
      strcpy(mqtt_password, custom_mqtt_password.getValue());
      if (shouldSaveConfig) {
        saveConfig();
      }
      Serial << "local ip: " << WiFi.localIP() << endl;
    }
  } else {
    wifiManager.setTimeout(180);
    if (!wifiManager.startConfigPortal("OnDemandAP")) {
      Serial << "failed to connect and hit timeout" << endl;
    } else {
      Serial << "WiFi connected" << endl;
      strcpy(mqtt_server, custom_mqtt_server.getValue());
      strcpy(mqtt_port, custom_mqtt_port.getValue());
      strcpy(mqtt_username, custom_mqtt_username.getValue());
      strcpy(mqtt_password, custom_mqtt_password.getValue());
      if (shouldSaveConfig) {
        saveConfig();
      }
      Serial << "local ip: " << WiFi.localIP() << endl;
    }
  }
}

void readConfig() {
  Serial << "mounting FS..." << endl;
  if (SPIFFS.begin()) {
    Serial << "mounted file system" << endl;
    if (SPIFFS.exists(file)) {
      Serial << "reading config file" << endl;
      File configFile = SPIFFS.open(file, "r");
      if (configFile) {
        Serial << "opened config file" << endl;;
        size_t size = configFile.size();
        std::unique_ptr<char[]> buf(new char[size]);
        configFile.readBytes(buf.get(), size);
        DynamicJsonBuffer jsonBuffer;
        JsonObject& json = jsonBuffer.parseObject(buf.get());
        #ifdef VERBOSE
        json.prettyPrintTo(Serial);
        Serial << endl;
        #endif
        if (json.success()) {
          Serial << "parsed json" << endl;
          if (json.containsKey("mqtt_server") && json.containsKey("mqtt_port") && json.containsKey("mqtt_username") && json.containsKey("mqtt_password")) {
            strcpy(mqtt_server, json["mqtt_server"]);
            strcpy(mqtt_port, json["mqtt_port"]);
            strcpy(mqtt_username, json["mqtt_username"]);
            strcpy(mqtt_password, json["mqtt_password"]);
          }
        } else {
          Serial << "failed to load json config" << endl;
        }
      }
    }
  } else {
    Serial << "failed to mount FS" << endl;
  }
}

#ifdef DEEPSLEEP
void goDeepSleep() {
  Serial << "going to sleep" << endl;

  shutPubSub();
  ESP.deepSleep(DEEPSLEEP * 1000000);
}
#endif
