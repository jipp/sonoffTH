#include <Arduino.h>


// sonoff TH16
// 1M (64k SPIFFS)
// gpio  0 -> button
// gpio 12 -> relay and red LED
// gpio 13 -> blue LED
// gpio 14 -> jack in


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
#include <DHT.h>


// defines
#define DEBUG
#define BUTTON  4
//#define BUTTON  0
#define RELAY   12
#define LED     13
#define JACK    14
#define DHTTYPE DHT22


// constants
const int address = 0;
const char file[]="/config.json";
const unsigned long int timerMeasureIntervall = 600000l;
const unsigned long int timerLastReconnect = 10000l;
const unsigned long int timerButtonPressed = 3000l;
const int mqtt_port = 1880;


// switch adc port to monitor vcc
ADC_MODE(ADC_VCC);


// global definitions
Ticker ticker;
PubSubClient pubSubClient;
DHT dht(JACK, DHTTYPE);


// callback functions
void tick();
void saveConfigCallback ();
void callback(char*, byte* , unsigned int);


// functions
void setupHardware();
void readSwitchStateEEPROM();
void writeSwitchStateEEPROM();
void setupPubSub();
void checkForConfigReset();
void resetConfig();
void publishSwitchState(char);


//
float vcc = 0.0;
bool wifiAvailable = false;
bool mqttAvailable = false;
bool shouldSaveConfig = false;
char id[13];
char mqtt_server[40];
char mqtt_username[16];
char mqtt_password[16];
String subscribeSwitchTopic = "/switch/command";
String publishSwitchTopic = "/switch/state";
String publishVccTopic = "/vcc/value";
String publishTemperatureTopic = "/temperature/value";
String publishHumidityTopic = "/humidity/value";
unsigned long int timerMeasureIntervallStart = 0l;
unsigned long int timerLastReconnectStart = 0l;
char switchState = '0';
bool switchTransmit = true;
bool currentState = HIGH;
bool recentState = HIGH;
float temperature = 0.0;
float humidity = 0.0;



void updater() {
  if (wifiAvailable) {
    t_httpUpdate_return ret = ESPhttpUpdate.update("lemonpi", 80, "/esp/update/arduino.php", VERSION);
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
        #ifdef DEBUG
        json.prettyPrintTo(Serial);
        Serial << endl;
        #endif
        if (json.success()) {
          Serial << "parsed json" << endl;
          if (json.containsKey("mqtt_server") && json.containsKey("mqtt_username") && json.containsKey("mqtt_password")) {
            strcpy(mqtt_server, json["mqtt_server"]);
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

void saveConfig() {
  Serial << "saving config" << endl;
  DynamicJsonBuffer jsonBuffer;
  JsonObject& json = jsonBuffer.createObject();
  json["mqtt_server"] = mqtt_server;
  json["mqtt_username"] = mqtt_username;
  json["mqtt_password"] = mqtt_password;
  File configFile = SPIFFS.open(file, "w");
  if (!configFile) {
    Serial << "failed to open config file for writing" << endl;
  }
  json.printTo(Serial);
  json.printTo(configFile);
  configFile.close();
}

void setupWiFiManager() {
  ticker.attach(0.1, tick);
  readConfig();
  WiFiManagerParameter custom_mqtt_server("server", "mqtt server", mqtt_server, 40);
  WiFiManagerParameter custom_mqtt_username("username", "mqtt username", mqtt_username, 16);
  WiFiManagerParameter custom_mqtt_password("password", "mqtt password", mqtt_password, 16);
  WiFiManager wifiManager;
  #ifdef DEBUG
  wifiManager.setDebugOutput(true);
  #else
  wifiManager.setDebugOutput(false);
  #endif
  wifiManager.setSaveConfigCallback(saveConfigCallback);
  wifiManager.addParameter(&custom_mqtt_server);
  wifiManager.addParameter(&custom_mqtt_username);
  wifiManager.addParameter(&custom_mqtt_password);
  //wifiManager.resetSettings();
  //SPIFFS.format();
  wifiManager.setTimeout(180);
  if (!wifiManager.autoConnect("AutoConnectAP")) {
    Serial << "failed to connect and hit timeout" << endl;
    wifiAvailable = false;
  } else {
    wifiAvailable = true;
    Serial << "connected...yeey :)" << endl;
    strcpy(mqtt_server, custom_mqtt_server.getValue());
    strcpy(mqtt_username, custom_mqtt_username.getValue());
    strcpy(mqtt_password, custom_mqtt_password.getValue());
    if (shouldSaveConfig) {
      saveConfig();
    }
    Serial << "local ip: " << WiFi.localIP() << endl;
  }
}

void measureValues() {
  vcc = ESP.getVcc();
  temperature = dht.readTemperature();
  humidity = dht.readHumidity();
}

void finishSetup() {
  if (wifiAvailable) {
    ticker.detach();
  } else {
    ticker.attach(0.5, tick);
  }
  digitalWrite(LED, LOW);
}

void setupID() {
  byte mac[6];

  WiFi.macAddress(mac);
  Serial << "Mac: " << _HEX(mac[0]) << ":" << _HEX(mac[1]) << ":"
  << _HEX(mac[2]) << ":" << _HEX(mac[3]) << ":" << _HEX(mac[4]) << ":"
  << _HEX(mac[5]) << endl;
  sprintf(id, "%02x%02x%02x%02x%02x%02x", mac[0], mac[1], mac[2], mac[3],
  mac[4], mac[5]);
  Serial << "id: " << id << endl;
}

void publishValues() {
  if (pubSubClient.publish(publishVccTopic.c_str(),
  String(vcc).c_str())) {
    Serial << " < " << publishVccTopic << ": " << vcc << endl;
  } else {
    Serial << "!< " << publishVccTopic << ": " << vcc << endl;
  }
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
}

void setupTopic() {
  subscribeSwitchTopic = id + subscribeSwitchTopic;
  publishSwitchTopic = id + publishSwitchTopic;
  publishVccTopic = id + publishVccTopic;
  publishTemperatureTopic = id + publishTemperatureTopic;
  publishHumidityTopic = id + publishHumidityTopic;
}

int reconnect() {
  Serial << "Attempting MQTT connection..." << endl;
  if (pubSubClient.connect(id, String(mqtt_username).c_str(), String(mqtt_password).c_str())) {
    Serial << "connected" << endl;
    publishSwitchState(switchState);
    pubSubClient.subscribe(subscribeSwitchTopic.c_str());
    Serial << " > " << subscribeSwitchTopic << endl;
  } else {
    Serial << "failed, rc=" << pubSubClient.state() << endl;
  }
  return pubSubClient.state();
}



void setup() {
  setupHardware();
  readSwitchStateEEPROM();
  checkForConfigReset();
  setupWiFiManager();
  updater();
  setupID();
  setupPubSub();
  setupTopic();
  finishSetup();
}

void loop() {
  currentState = digitalRead(BUTTON);
  if ((currentState == LOW) and (recentState == HIGH)) {
    delay(250);
    if (switchState == '1') {
      digitalWrite(RELAY, LOW);
      switchState = '0';
      switchTransmit = true;
      writeSwitchStateEEPROM();
    } else {
      digitalWrite(RELAY, HIGH);
      switchState = '1';
      switchTransmit = true;
      writeSwitchStateEEPROM();
    }
    Serial << "Switch state: " << switchState << endl;
  }
  recentState = currentState;
  if (wifiAvailable) {
    if (!pubSubClient.connected()) {        // crash as mqtt_... not defined
      Serial << "hello" << endl;
      if (millis() - timerLastReconnectStart > timerLastReconnect) {
        if (reconnect() != 0) {
          timerLastReconnectStart = 0;
        } else {
          timerLastReconnectStart = millis();
        }
      }
    } else {
      pubSubClient.loop();
      if (switchTransmit) {
        publishSwitchState(switchState);
      }
      if (millis() - timerMeasureIntervallStart > timerMeasureIntervall) {
        timerMeasureIntervallStart = millis();
        measureValues();
        publishValues();
      }
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
  Serial << "> " << topic << ": ";
  for (unsigned int i = 0; i < length; i++) {
    Serial << (char) payload[i];
  }
  Serial << endl;
  switch (payload[0]) {
    case '0':
    if (switchState != '0') {
      digitalWrite(RELAY, LOW);
      switchState = '0';
      switchTransmit = true;
      writeSwitchStateEEPROM();
    }
    break;
    case '1':
    if (switchState != '1') {
      digitalWrite(RELAY, HIGH);
      switchState = '1';
      switchTransmit = true;
      writeSwitchStateEEPROM();
    }
    break;
    case '2':
    updater();
    break;
    case '3':
    resetConfig();
    break;
  }
}

void setupHardware() {
  ticker.attach(0.3, tick);
  Serial.begin(115200);
  dht.begin();
  pinMode(BUTTON, INPUT);
  pinMode(RELAY, OUTPUT);
  pinMode(LED, OUTPUT);
}

void setupPubSub() {
  WiFiClient client;

  pubSubClient.setClient(client);
  pubSubClient.setServer(mqtt_server, mqtt_port);
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

void publishSwitchState(char switchState) {
  if (pubSubClient.publish(publishSwitchTopic.c_str(), String(switchState).c_str())) {
    Serial << " < " << publishSwitchTopic << ": " << switchState << endl;
  } else {
    Serial << "!< " << publishSwitchTopic << ": " << switchState << endl;
  }
}

void checkForConfigReset() {
  unsigned long int timerButtonPressedStart =  millis();

  Serial << "waiting for reset" << endl;
  delay(1000);
  while (digitalRead(BUTTON) == LOW) {
    if (millis() - timerButtonPressedStart > timerButtonPressed) {
      resetConfig();
    }
  }
  Serial << "finished waiting" << endl;
}

void resetConfig() {
  WiFiManager wifiManager;

  Serial << "reset" << endl;
  SPIFFS.format();
  wifiManager.resetSettings();
  delay(3000);
  ESP.reset();
}
