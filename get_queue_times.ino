#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <ESP8266HTTPClient.h>
#include <WiFiClientSecure.h>
#include <ArduinoJson.h>
#include <WiFiManager.h>

ESP8266WebServer server(80);
StaticJsonDocument<8192> doc;

WiFiManager wm;
char parkNum[4] = "64";
char parkName[34] = "Islands Of Adventure";
WiFiManagerParameter parkNumParam("park_num", "Queue-Times Park Number", parkNum, 3);
WiFiManagerParameter parkNameParam("park_name", "Park Name", parkName, 32);

bool shouldSaveConfig = false;

const int maxDisplay = 4;
const int maxTotalRides = 32;
String rideNames[maxTotalRides];
String rideWaits[maxTotalRides];
int rideCount = 0;
int currentIndex = 0;
unsigned long lastRefreshTime = 0;
const unsigned long refreshInterval = 30000;
unsigned long lastCycleTime = 0;
const unsigned long cycleInterval = 7000;


void handleReset() {
  server.send(200, "text/plain", "Resetting configuration.");
  Serial.println("[RESET]");
  wm.resetSettings();
  if (SPIFFS.begin()) {
    SPIFFS.remove("/config.json");
  }
  ESP.restart();
}


void saveParametersFlag() {
  shouldSaveConfig = true;
}


void sendAPMessage(WiFiManager* wm) {
  Serial.println("[AP]");
}


void saveParameters() {
  Serial.println("Saving config parameters");
  DynamicJsonDocument json(1024);
  json["parkNum"] = parkNum;
  json["parkName"] = parkName;

  File configFile = SPIFFS.open("/config.json", "w");
  if (!configFile) {
    Serial.println("failed to open config file for writing");
  }

  serializeJson(json, Serial);
  Serial.println();
  serializeJson(json, configFile);
  configFile.close();
}


void setup() {
  Serial.begin(9600);

  wm.setSaveConfigCallback(saveParametersFlag);
  wm.setAPCallback(sendAPMessage);
  wm.addParameter(&parkNumParam);
  wm.addParameter(&parkNameParam);
  wm.setTitle("waittimes");

  if (SPIFFS.begin()) {
    Serial.println("mounted file system");
    if (SPIFFS.exists("/config.json")) {
      //file exists, reading and loading
      Serial.println("reading config file");
      File configFile = SPIFFS.open("/config.json", "r");
      if (configFile) {
        Serial.println("opened config file");
        size_t size = configFile.size();
        // Allocate a buffer to store contents of the file.
        std::unique_ptr<char[]> buf(new char[size]);

        configFile.readBytes(buf.get(), size);

        DynamicJsonDocument json(1024);
        auto deserializeError = deserializeJson(json, buf.get());
        serializeJson(json, Serial);
        if ( ! deserializeError ) {
          Serial.println("\nparsed json");
          strcpy(parkNum, json["parkNum"]);
          strcpy(parkName, json["parkName"]);
        } else {
          Serial.println("[ERROR] failed to load json config");
        }
        configFile.close();
      }
    }
  }

  if (!wm.autoConnect("waittimes", "waittimes")) {
    Serial.println("failed to connect and hit timeout");
    delay(3000);
    ESP.restart();
    delay(5000);
  }

  server.on("/reset", handleReset);
  server.begin();

  if (shouldSaveConfig) {
    strcpy(parkNum, parkNumParam.getValue());
    strcpy(parkName, parkNameParam.getValue());
    saveParameters();
  }

  Serial.println("parkNum: " + String(parkNum));
  Serial.println("parkName: " + String(parkName));

  Serial.println("[CONNECTED] " + WiFi.localIP().toString());
  
  refreshData();
  lastRefreshTime = millis();

  // wait for display to be ready
  bool ready = false;
  String incomingString;
  while (!ready) {
    if (Serial.available()) {
      incomingString = Serial.readStringUntil('\n');
      if (incomingString == "[READY]\r") {
        ready = true;
      }
    }
  }
  
  sendParkInfo();
  delay(1000);
}


void sendParkInfo() {
  String parkLine = "P:" + String(parkName);
  Serial.println(parkLine);
}


void sendRideInfo(int index) {
  String rideLine = "R:" + rideNames[index] + ";" + rideWaits[index];
  Serial.println(rideLine);
}


String sanitizeName(String name) {
  String cleanName = "";
  for (unsigned int i = 0; i < name.length(); i++) {
    char c = name[i];
    if (c >= 32 && c <= 126) {  // Printable ASCII
      cleanName += c;
    }
  }
  return name.length() <= 28 ? cleanName : (cleanName.substring(0,25) + String("..."));
}


void processRidesJson(JsonVariant ridesVariant) {
  if (!ridesVariant.is<JsonArray>()) return;
  for (JsonObject ride : ridesVariant.as<JsonArray>()) {
    String name = ride["name"].as<String>();
    String cleanName = "";
    cleanName = sanitizeName(name);

    bool open = ride["is_open"];
    int wait = ride["wait_time"];
    String rideStatus = open ? String(wait) + " min" : "CLOSED";

    if (rideCount < maxTotalRides && name.indexOf("Single") == -1) {
      rideNames[rideCount] = cleanName;
      rideWaits[rideCount] = rideStatus;
      rideCount++;
    }
  }
}


void refreshData() {
  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("Refreshing data...");
    
    WiFiClientSecure client;
    client.setInsecure();  // Disable SSL cert validation for simplicity

    HTTPClient http;
    if (http.begin(client, "https://queue-times.com/parks/" + String(parkNum) + "/queue_times.json")) {
      int httpCode = http.GET();
      if (httpCode != HTTP_CODE_OK) {
        Serial.print("[ERROR] HTTP GET failed, code: ");
        Serial.println(httpCode);
        return;
      }

      Stream* stream = http.getStreamPtr();
      DeserializationError error = deserializeJson(doc, *stream);
      if (error) {
        Serial.println("[ERROR] JSON parsing failed");
        return;
      }
      
      rideCount = 0;

      // Iterate over all rides contained within lands
      for (JsonObject land : doc["lands"].as<JsonArray>()) {
        processRidesJson(land["rides"]);
      }
      // If there's a top-level "rides" array, process that too
      processRidesJson(doc["rides"]);

      Serial.println("[DONE]");
      http.end();
    } else {
      Serial.println("[ERROR] Unable to connect to API");
    }
  } else {
    Serial.println("[ERROR] Not connected to Wi-Fi");
  }
}


void pushDataPage() {
  Serial.println("[UPDATE]");
  delay(250);
  for (int i = 0; i < maxDisplay; i++) {
    int idx = (currentIndex + i) % rideCount;
    sendRideInfo(idx);
    delay(250);
  }
}


void loop() {
  server.handleClient();

  if (millis() - lastRefreshTime > refreshInterval) {
    lastRefreshTime = millis();
    refreshData();
  }

  if (millis() - lastCycleTime > cycleInterval) {
    lastCycleTime = millis();
    currentIndex = (currentIndex + maxDisplay) % rideCount;
    pushDataPage();
  }
}
