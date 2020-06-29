extern "C" {
#include "user_interface.h"
  typedef void (*freedom_outside_cb_t)(uint8 status);
  int  wifi_register_send_pkt_freedom_cb(freedom_outside_cb_t cb);
  void wifi_unregister_send_pkt_freedom_cb(void);
  int  wifi_send_pkt_freedom(uint8 *buf, int len, bool sys_seq);
}

#include <ESP8266WiFi.h>
#include "./data_models.h"

#define MAX_APS_TRACKED 100
#define MAX_CLIENTS_TRACKED 200

beaconinfo aps_known[MAX_APS_TRACKED];
int aps_known_nr = 0;
int no_newDevices = 0;
clientinfo clients_known[MAX_CLIENTS_TRACKED];
int known_clients_nr = 0;




String forMac(uint8_t mac[ETH_MAC_LEN]) {
  String hi = "";
  for (int i = 0; i < ETH_MAC_LEN; i++) {
    if (mac[i] < 16) hi = hi + "0" + String(mac[i], HEX);
    else hi = hi + String(mac[i], HEX);
    if (i < 5) hi = hi + ":";
  }
  return hi;
}

int register_beacon(beaconinfo beacon)
{
  int known = 0;
  for (int u = 0; u < aps_known_nr; u++)
  {
    if (! memcmp(aps_known[u].bssid, beacon.bssid, ETH_MAC_LEN)) {
      aps_known[u].lastDiscoveredTime = millis();
      aps_known[u].rssi = beacon.rssi;
      known = 1;
      break;
    }
  }
  if (! known && (beacon.err == 0))
  {
    beacon.lastDiscoveredTime = millis();
    memcpy(&aps_known[aps_known_nr], &beacon, sizeof(beacon));

    aps_known_nr++;

    if ((unsigned int) aps_known_nr >=
        sizeof (aps_known) / sizeof (aps_known[0]) ) {
      Serial.printf("exceeded max aps_known\n");
      aps_known_nr = 0;
    }
  }
  return known;
}

int register_device(clientinfo &ci) {
  int known = 0;
  for (int u = 0; u < known_clients_nr; u++)
  {
    if (! memcmp(clients_known[u].station, ci.station, ETH_MAC_LEN)) {
      clients_known[u].lastDiscoveredTime = millis();
      clients_known[u].rssi = ci.rssi;
      known = 1;
      break;
    }
  }
  
  if (! known) {
    ci.lastDiscoveredTime = millis();
    for (int u = 0; u < aps_known_nr; u++) {
      if (! memcmp(aps_known[u].bssid, ci.bssid, ETH_MAC_LEN)) {
        ci.channel = aps_known[u].channel;
        break;
      }
    }
    if (ci.channel != 0) {
      memcpy(&clients_known[known_clients_nr], &ci, sizeof(ci));
      known_clients_nr++;
    }

    if ((unsigned int) known_clients_nr >=
        sizeof (clients_known) / sizeof (clients_known[0]) ) {
      Serial.printf("exceeded max clients_known\n");
      known_clients_nr = 0;
    }
  }
  return known;
}


String display_beacon(beaconinfo beacon)
{
  String hi = "";
  if (beacon.err != 0) {
    Serial.printf("BEACON ERR: (%d)  ", beacon.err);
  } else {
    Serial.printf(" BEACON: <=============== [%32s]  ", beacon.ssid);
    Serial.print(forMac(beacon.bssid));
    Serial.printf("   %2d", beacon.channel);
    Serial.printf("   %4d\r\n", beacon.rssi);
  }
  return hi;
}

String display_client(clientinfo ci)
{
  String hi = "";
  int u = 0;
  int known = 0;   // Clear known flag
  if (ci.err != 0) {
  } else {
    Serial.printf("CLIENT: ");
    Serial.print(forMac(ci.station));  //Mac of device
    Serial.printf(" ==> ");
      Serial.print(forMac(ci.ap));   // Mac of connected AP
      Serial.printf("  % 3d", ci.channel);  //used channel
      Serial.printf("   % 4d\r\n", ci.rssi);
  }
  return hi;
}

void promisc_cb(uint8_t *buf, uint16_t len)
{
  int i = 0;
  uint16_t seq_n_new = 0;
  if (len == 12) {
    struct RxControl *sniffer = (struct RxControl*) buf;
  } else if (len == 128) {
    struct sniffer_buf2 *sniffer = (struct sniffer_buf2*) buf;
    if ((sniffer->buf[0] == 0x80)) {
      struct beaconinfo beacon = parse_beacon(sniffer->buf, 112, sniffer->rx_ctrl.rssi);
      if (register_beacon(beacon) == 0) {
        display_beacon(beacon);
        no_newDevices = 0;
      }
    } else if ((sniffer->buf[0] == 0x40)) {
      struct clientinfo ci = parse_probe(sniffer->buf, 36, sniffer->rx_ctrl.rssi);
        if (register_device(ci) == 0) {
          display_client(ci);
          no_newDevices = 0;
        }
    }
  } else {
    struct sniffer_buf *sniffer = (struct sniffer_buf*) buf;
    if ((sniffer->buf[0] == 0x08) || (sniffer->buf[0] == 0x88)) {
      struct clientinfo ci = parse_data(sniffer->buf, 36, sniffer->rx_ctrl.rssi, sniffer->rx_ctrl.channel);
      if (memcmp(ci.bssid, ci.station, ETH_MAC_LEN)) {
        if (register_device(ci) == 0) {
          display_client(ci);
          no_newDevices = 0;
        }
      }
    }
  }
}
