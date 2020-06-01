#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <ESP8266WebServer.h>
#include <ESP8266mDNS.h>
#include <DNSServer.h>
#include "Page.h"
#include "WiFiConnect.h"
#include <string>

WiFiConnect connect;

void setup(void) {
  connect.setup();
}

void loop(void) {
  connect.loop();
}
