#include <Arduino.h>

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
#include <DHT.h>
#include <ESP8266mDNS.h>
#include <WiFiUdp.h>
#include <ArduinoOTA.h>

// defines
// #define DEEPSLEEP
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
#ifndef DHTTYPE
#define DHTTYPE DHT22
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


// constants
const int address = 0;
const char file[]="/config.json";
const unsigned long int timerMeasureIntervall = 60l;
const unsigned long int timerLastReconnect = 60l;
const unsigned long int timerButtonPressed = 3l;
const unsigned long timerDeepSleep = 60l;


// switch adc port to monitor vcc
ADC_MODE(ADC_VCC);


// global definitions
Ticker ticker;
PubSubClient pubSubClient;
DHT dht(JACK, DHTTYPE);
WiFiClient wifiClient;


// global variables
bool shouldSaveConfig = false;
char id[13];
unsigned long int timerMeasureIntervallStart = 0l;
unsigned long int timerLastReconnectStart = 0l;
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



void setupWiFiManager() {
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
        if (digitalRead(BUTTON) == LOW) {
                delay(250);
                digitalWrite(RELAY, !digitalRead(RELAY));
                writeSwitchStateEEPROM();
                publishSwitchState();
                Serial << "Switch state: " << digitalRead(RELAY) << endl;
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
        Serial << endl << endl << "Version: " << VERSION << endl;
        ticker.attach(0.3, tick);
        dht.begin();
        pinMode(BUTTON, INPUT);
        pinMode(RELAY, OUTPUT);
        pinMode(LED, OUTPUT);
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
        unsigned long int timerButtonPressedStart =  millis();

        Serial << "waiting for reset" << endl;
        delay(1000);
        while (digitalRead(BUTTON) == LOW) {
                if (millis() - timerButtonPressedStart > timerButtonPressed * 1000) {
                        resetConfig();
                }
        }
        Serial << "finished waiting for reset" << endl;
}

void resetConfig() {
        WiFiManager wifiManager;

        Serial << "reset config" << endl;
        SPIFFS.format();
        wifiManager.resetSettings();
        delay(3000);
        ESP.reset();
        /*  wifiManager.setTimeout(180);
           if (!wifiManager.startConfigPortal("OnDemandAP")) {
           Serial.println("failed to connect and hit timeout");
           delay(3000);
           ESP.reset();
           delay(5000);
           }*/
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
        float temperature = dht.readTemperature();
        float humidity = dht.readHumidity();

        if (pubSubClient.connected()) {
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
                publishSwitchState();
                publishValues();
                pubSubClient.subscribe(subscribeSwitchTopic.c_str());
                ticker.detach();
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
        } else {
                ticker.attach(0.5, tick);
        }
        digitalWrite(LED, LEDOFF);
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

void goDeepSleep() {
        Serial << "going to sleep" << endl;

        shutPubSub();
        ESP.deepSleep(timerDeepSleep * 1000000);
}
