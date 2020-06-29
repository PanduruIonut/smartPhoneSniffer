#include "ESP8266WiFi.h"
#include <ArduinoJson.h>

#define WIFI_SSID     "DIGI-gsyt"
#define WIFI_PASSWORD "MansardaM22"


const char* Host = "www.googleapis.com";
String thisPage = "/geolocation/v1/geolocate?key=";
String key = "AIzaSyD57ceMGJenDk9u-yWCoD68xFi2Qt8uQF0";
double latitude, longitude, accuracy;


int getNumberOfnetwork() {

  Serial.print("\nscan start");
  Serial.print("\nwaiting for scan");
  int numbers = WiFi.scanNetworks();
  Serial.print("\nscan done");

  if (numbers == 0) {
    Serial.print("\n* no networks *");
  } else {
    Serial.print("\nnetworks : " + String(numbers));
    Serial.print("\nwaiting location");
    return numbers;
  }
}

String setJsonString(int counts) {
  String jsonS = "{\n";
  jsonS += "\"wifiAccessPoints\": [\n";
  for (int j = 0; j < counts; ++j) {
    jsonS += "{\n";
    jsonS += "\"macAddress\" : \"";
    jsonS += (WiFi.BSSIDstr(j));
    jsonS += "\",\n";
    jsonS += "\"signalStrength\": ";
    jsonS += WiFi.RSSI(j);
    jsonS += "\n";
    if (j < counts - 1) {
      jsonS += "},\n";
    } else {
      jsonS += "}\n";
    }
  }
  jsonS += ("]\n");
  jsonS += ("}\n");
  return jsonS;
}

void callRestAPI(String jsonString, String host, String url, String key) {
  WiFiClientSecure client;
  client.setInsecure();
  DynamicJsonBuffer jsonBuffer;

    Serial.print("\nHost:" +host);
  Serial.print("\nthisPage:" +url);
  Serial.print("\nkey:" +key);
  
  if (client.connect(Host, 443)) {   
    client.println("POST " + url + key + " HTTP/1.1");
    client.println("Host: " + (String)Host);
    client.println("Connection: close");
    client.println("Content-Type: application/json");
    client.println("User-Agent: Arduino/1.0");
    client.print("Content-Length: ");
    client.println(jsonString.length());
    client.println();
    client.print(jsonString);
    delay(500);
  }else {
    Serial.print("\nerror callRestAPI");
  }

  while (client.available()) {
    String line = client.readStringUntil('\r');
    Serial.print(line);
    JsonObject& root = jsonBuffer.parseObject(line);
    if (root.success()) {
      latitude    = root["location"]["lat"];
      longitude   = root["location"]["lng"];
      accuracy   = root["accuracy"];
    }
  }
  client.stop();
}
