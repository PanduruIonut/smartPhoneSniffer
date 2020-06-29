

#include <ESP8266WiFi.h>
#include "ESP8266WiFi.h"
#include <ArduinoJson.h>
#include <PubSubClient.h>
#include <set>
#include <string>
#include "./utility.h"
#include "./geolocation.h"

#define disable 0
#define enable  1
#define SENDTIME 30000
#define MAXDEVICES 60
#define JBUFFER 15+ (MAXDEVICES * 40)
#define PURGETIME 600000
#define MINRSSI -70

WiFiClient espClient;
PubSubClient client(espClient);

unsigned int channel = 1;
int known_clients_nr_old, aps_known_nr_old;
unsigned long sendEntry, deleteEntry;
char jsonString[JBUFFER];


String device[MAXDEVICES];
int nbrDevices = 0;
int usedChannels[15];

#ifndef CREDENTIALS
#define APName "DIGI-gsyt"
#define APPassword "MansardaM22"
#define APPasswordAddress "192.168.100.41"
#endif

StaticJsonBuffer<JBUFFER>  jsonBuffer;

void setup() {
  Serial.begin(115200);
  Serial.printf("\n\nSDK version:%s\n\r", system_get_sdk_version());

  wifi_set_opmode(STATION_MODE);
  wifi_set_channel(channel);
  wifi_promiscuous_enable(disable);
  wifi_set_promiscuous_rx_cb(promisc_cb);
  wifi_promiscuous_enable(enable);
}




void loop() {
  channel = 1;
  boolean sendMQTT = false;
  wifi_set_channel(channel);
  while (true) {
    no_newDevices++;
    if (no_newDevices > 200) {
      no_newDevices = 0;
      channel++;
      if (channel == 15) break;
      wifi_set_channel(channel);
    }
    delay(1);

    if (known_clients_nr > known_clients_nr_old) {
      known_clients_nr_old = known_clients_nr;
      sendMQTT = true;
    }
    if (aps_known_nr > aps_known_nr_old) {
      aps_known_nr_old = aps_known_nr;
      sendMQTT = true;
    }
    if (millis() - sendEntry > SENDTIME) {
      sendEntry = millis();
      sendMQTT = true;
    }
  }
  purgeDevice();
  if (sendMQTT) {
    showDevices();
    sendDevices();
  }
}


void connectToAP() {
  delay(10);
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(APName);

  WiFi.mode(WIFI_STA);
  WiFi.begin(APName, APPassword);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println(WiFi.localIP());
}

void purgeDevice() {
  for (int u = 0; u < known_clients_nr; u++) {
    if ((millis() - clients_known[u].lastDiscoveredTime) > PURGETIME) {
      for (int i = u; i < known_clients_nr; i++) memcpy(&clients_known[i], &clients_known[i + 1], sizeof(clients_known[i]));
      known_clients_nr--;
      break;
    }
  }
  for (int u = 0; u < aps_known_nr; u++) {
    if ((millis() - aps_known[u].lastDiscoveredTime) > PURGETIME) {
      for (int i = u; i < aps_known_nr; i++) memcpy(&aps_known[i], &aps_known[i + 1], sizeof(aps_known[i]));
      aps_known_nr--;
      break;
    }
  }
}


void showDevices() {
  Serial.println("");
  Serial.println("");
  Serial.println("*************Devices Found **************");
  Serial.printf("%4d Devices + Clients.\n",aps_known_nr + known_clients_nr);

  for (int u = 0; u < aps_known_nr; u++) {
    Serial.printf( "%4d ",u);
    Serial.print("Beacon nr ");
    Serial.print(forMac(aps_known[u].bssid));
    Serial.print(" RSSI ");
    Serial.print(aps_known[u].rssi);
    Serial.print(" channel ");
    Serial.println(aps_known[u].channel);
  }

  for (int u = 0; u < known_clients_nr; u++) {
    Serial.printf("%4d ",u);
    Serial.print("C ");
    Serial.print(forMac(clients_known[u].station));
    Serial.print(" RSSI ");
    Serial.print(clients_known[u].rssi);
    Serial.print(" channel ");
    Serial.println(clients_known[u].channel);
  }
}

void sendDevices() {
  String deviceMac;
  wifi_promiscuous_enable(disable);
  connectToAP();
  
  client.setServer(APPasswordAddress, 1883);
  while (!client.connected()) {
    Serial.println("Connecting to MQTT...");


    if (client.connect("ESP32Client", "panduru.ionut.alin@gmail.com", "78dcf88c" )) {
      Serial.println("connected");
    } else {
      Serial.print("failed with state ");
      Serial.println(client.state());
    }
    yield();
  }


  jsonBuffer.clear();
  JsonObject& root = jsonBuffer.createObject();
  JsonArray& mac = root.createNestedArray("MAC");

  for (int u = 0; u < aps_known_nr; u++) {
    deviceMac = forMac(aps_known[u].bssid);
    if (aps_known[u].rssi > MINRSSI) {
      mac.add(deviceMac);
    }
  }

  for (int u = 0; u < known_clients_nr; u++) {
    deviceMac = forMac(clients_known[u].station);
    if (clients_known[u].rssi > MINRSSI) {
      mac.add(deviceMac);
    }
  }


  Serial.println();
  Serial.printf("number of devices: %02d\n", mac.size());
  root.prettyPrintTo(Serial);
  root.printTo(jsonString);
  if (client.publish("Sniffer", jsonString) == 1) Serial.println("Successfully published");
  else {
    Serial.println();
    Serial.println(" Not published!");
    Serial.println();
  }
  client.loop();
  client.disconnect ();
  delay(100);
  wifi_promiscuous_enable(enable);
  sendEntry = millis();
}

void getLocation(){
  int countOFnetwork = getNumberOfnetwork();
  callRestAPI(setJsonString(countOFnetwork) , Host, thisPage, key);


  Serial.print("Lat " + String(latitude,5));
 
  Serial.print("Lon " + String(longitude,5));

}
