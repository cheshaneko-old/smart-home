#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <ESP8266WebServer.h>
#include <ESP8266mDNS.h>
#include "Page.h"
#include <string>


const char* ssid = "SMART";

ESP8266WebServer server(80);

void handleRoot() {
  server.sendHeader("Content-Encoding", "gzip");
  server.send(200, "text/html; charset=utf-8", (char*) htmlPage, (size_t) sizeof(htmlPage));
}

void handleConnections() {
  Serial.println("Start scanning Wi-Fi");
  int n = WiFi.scanNetworks();
  Serial.print("Found connections: ");
  Serial.println(n);
  std::string connections = "[";
  if (n > 0) {
    for (int i = 0; i < n; ++i) {
      Serial.println(WiFi.SSID(i));
      connections.append("{\"name\": \"");
      connections.append(WiFi.SSID(i).c_str());
      connections.append("\"}");
      if (i < n - 1) {
        connections.append(",");
      }
    }
  }
  connections.append("]");
  server.send(200, "appliation/json; charset=utf-8", connections.c_str());
}

void setup(void) {
  delay(1000);
  Serial.begin(115200);
  Serial.println();
  Serial.print("Configuring access point...");
  WiFi.softAP(ssid);

  IPAddress myIP = WiFi.softAPIP();
  Serial.print("AP IP address: ");
  Serial.println(myIP);
  server.on("/", handleRoot);
  server.on("/connections", handleConnections);
  server.begin();
  Serial.println("HTTP server started");
}

void loop(void) {
  server.handleClient();
  MDNS.update();
}
