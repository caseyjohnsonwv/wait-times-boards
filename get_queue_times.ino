#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <ESP8266HTTPClient.h>
#include <WiFiClientSecure.h>
#include <ArduinoJson.h>
#include <WiFiManager.h>

ESP8266WebServer server(80);
StaticJsonDocument<8192> doc;

WiFiManager wm;



void sendAPMessage(WiFiManager* wm) {
  Serial.println("[AP]");
}



void setup() {
  Serial.begin(9600);

  wm.resetSettings();
  wm.setAPCallback(sendAPMessage);
  wm.setTitle("waittimes");

  if (!wm.autoConnect("waittimes", "waittimes")) {
    Serial.println("failed to connect and hit timeout");
    delay(3000);
    ESP.restart();
    delay(5000);
  }

  server.begin();
  Serial.println("\n[CONNECTED] " + WiFi.localIP().toString());
}


const char* includeList[] = {
  "Jurassic Park River Adventure",
  "Jurassic World VelociCoaster",
  "Pteranodon Flyers",
  "Skull Island: Reign of Kong",
  "Doctor Doom's Fearfall",
  "Storm Force Accelatron",
  "The Amazing Adventures of Spider-Man",
  "The Incredible Hulk Coaster",
  "Caro-Suess-el",
  "One Fish, Two Fish, Red Fish, Blue Fish",
  "The Cat in The Hat",
  "The High in the Sky Seuss Trolley Train Ride!",
  "Flight of the Hippogriff",
  "Hagrid's Magical Creatures Motorbike Adventure",
  "Harry Potter and the Forbidden Journey",
  "Hogwarts Express - Hogsmeade Station",
  "Dudley Do-Right's Ripsaw Falls",
  "Popeye & Bluto's Bilge-Rat Barges"
};


String sanitizeName(String name) {
  String cleanName = "";
  for (unsigned int i = 0; i < name.length(); i++) {
    char c = name[i];
    if (c >= 32 && c <= 126) {  // Printable ASCII
      cleanName += c;
    }
  }
  return cleanName;
}


bool isIncluded(String name) {
  for (int i = 0; i < sizeof(includeList) / sizeof(includeList[0]); i++) {
    if (name.equalsIgnoreCase(includeList[i])) {
      return true;
    }
  }
  return false;
}


void loop() {
  server.handleClient();

  if (WiFi.status() == WL_CONNECTED) {
    WiFiClientSecure client;
    client.setInsecure();  // Disable SSL cert validation for simplicity

    HTTPClient http;
    if (http.begin(client, "https://queue-times.com/parks/64/queue_times.json")) {
      int httpCode = http.GET();
      if (httpCode != HTTP_CODE_OK) {
        Serial.print("[ERROR] HTTP GET failed, code: ");
        Serial.println(httpCode);
        delay(10000);
        return;
      }

      Stream* stream = http.getStreamPtr();
      DeserializationError error = deserializeJson(doc, *stream);
      if (error) {
        Serial.println("[ERROR] JSON parsing failed");
        delay(10000);
        return;
      }

      // Clear display for new data
      Serial.println("[RESET]");

      // ✅ Iterate over all lands and rides
      for (JsonObject land : doc["lands"].as<JsonArray>()) {
        for (JsonObject ride : land["rides"].as<JsonArray>()) {
          String name = ride["name"].as<String>();

          // 🧼 Remove non-ASCII characters
          String cleanName = "";
          cleanName = sanitizeName(name);

          // ❌ Skip suppressed rides
          if (!isIncluded(cleanName)) {
            continue;
          }

          bool open = ride["is_open"];
          int wait = ride["wait_time"];

          String line = "R:" + cleanName + ";" + (open ? String(wait) + " min" : "CLOSED");
          Serial.println(line);
          delay(500);
        }
      }

      Serial.println("[DONE]");
      http.end();
    } else {
      Serial.println("[ERROR] Unable to connect to API");
    }
  } else {
    Serial.println("[ERROR] Not connected to Wi-Fi");
  }

  delay(30000);  // Wait 30 seconds before updating again
}
