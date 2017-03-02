#include <Arduino.h>

// defines
#define DS1822  0x22
#define DS18B20 0x28
#define DS18S20 0x10

#if (SENSOR == DHT11) || (SENSOR == DHT21) || (SENSOR == DHT22)
#include <DHT.h>
#elif (SENSOR == DS1822) || (SENSOR == DS18B20) || (SENSOR == DS18S20)
#include <OneWire.h>
#include <DallasTemperature.h>
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
#ifndef JACK
#define JACK    14
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


// libaries
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


// switch adc port to monitor vcc
ADC_MODE(ADC_VCC);


// global definitions
Ticker ticker;
PubSubClient pubSubClient;
#if (SENSOR == DHT11) || (SENSOR == DHT21) || (SENSOR == DHT22)
DHT dht(JACK, SENSOR);
#elif (SENSOR == DS1822) || (SENSOR == DS18B20) || (SENSOR == DS18S20)
OneWire oneWire(JACK);
DallasTemperature dallasTemperature(&oneWire);
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
  #if (SENSOR == DHT11) || (SENSOR == DHT21) || (SENSOR == DHT22)
  dht.begin();
  #elif (SENSOR == DS1822) || (SENSOR == DS18B20) || (SENSOR == DS18S20)
  dallasTemperature.begin();
  #endif
  pinMode(BUTTON, INPUT_PULLUP);
  pinMode(RELAY, OUTPUT);
  pinMode(LED, OUTPUT);
}

void printSettings() {
  Serial << endl << "VERSION: " << VERSION << endl;
  #ifdef VERBOSE
  #ifdef DEEPSLEEP
  Serial << "DEEPSLEEP: " << DEEPSLEEP << "s" << endl;
  #endif
  Serial << "BUTTON: " << BUTTON << endl;
  Serial << "RELAY: " << RELAY << endl;
  Serial << "LED: " << LED << endl;
  Serial << "JACK: " << JACK << endl;
  #ifdef SENSOR
  Serial << "SENSOR: " << SENSOR << endl;
  #endif
  Serial << "LEDOFF: " << LEDOFF << endl;
  Serial << "SERVER: " << SERVER << endl;
  Serial << "PORT: " << PORT << endl;
  Serial << "PATH: " << PATH << endl;
  Serial << endl;
  #endif
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
  #if (SENSOR == DHT11) || (SENSOR == DHT21) || (SENSOR == DHT22)
  float temperature = dht.readTemperature();
  float humidity = dht.readHumidity();
  #elif (SENSOR == DS1822) || (SENSOR == DS18B20) || (SENSOR == DS18S20)
  float temperature;
  #endif

  if (pubSubClient.connected()) {
    if (pubSubClient.publish(publishVccTopic.c_str(),
    String(vcc).c_str())) {
      Serial << " < " << publishVccTopic << ": " << vcc << endl;
    } else {
      Serial << "!< " << publishVccTopic << ": " << vcc << endl;
    }
    #if (SENSOR == DHT11) || (SENSOR == DHT21) || (SENSOR == DHT22)
    if (!isnan(temperature) && pubSubClient.publish(publishTemperatureTopic.c_str(),
    String(temperature).c_str())) {
      Serial << " < " << publishTemperatureTopic << ": " << temperature << endl;
    } else {
      Serial << "!< " << publishTemperatureTopic << ": " << temperature << endl;
    }
    if (!isnan(humidity) && pubSubClient.publish(publishHumidityTopic.c_str(),
    String(humidity).c_str())) {
      Serial << " < " << publishHumidityTopic << ": " << humidity << endl;
    } else {
      Serial << "!< " << publishHumidityTopic << ": " << humidity << endl;
    }
    #elif (SENSOR == DS1822) || (SENSOR == DS18B20) || (SENSOR == DS18S20)
    dallasTemperature.requestTemperatures();
    temperature = dallasTemperature.getTempCByIndex(0);
    if (pubSubClient.publish(publishTemperatureTopic.c_str(),
    String(temperature).c_str())) {
      Serial << " < " << publishTemperatureTopic << ": " << temperature << endl;
    } else {
      Serial << "!< " << publishTemperatureTopic << ": " << temperature << endl;
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
    pubSubClient.subscribe(subscribeSwitchTopic.c_str());
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
  //wifiManager.resetSettings();
  //SPIFFS.format();
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
