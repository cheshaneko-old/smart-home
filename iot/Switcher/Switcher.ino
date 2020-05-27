#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <ESP8266WebServer.h>
#include <ESP8266mDNS.h>
#include "Page.h"
#include <string>

ESP8266WebServer server(80);

std::string connections = "[]";

unsigned long scanLastCallMs = 0;
bool scanEnabled = true;

void handleRoot() {
  server.sendHeader("Content-Encoding", "gzip");
  server.send(200, "text/html; charset=utf-8", (char*) htmlPage, (size_t) sizeof(htmlPage));
}

void scanAndSaveConnections() {
  if (((millis() - scanLastCallMs) < (1000 * 30)) || !scanEnabled) {
    return; 
  }
  
  Serial.println("Scan connections");
  int n = WiFi.scanNetworks();
  Serial.print("Found connections: ");
  Serial.println(n);

  if (n > 0) {
    connections = "[";
    for (int i = 0; i < n; ++i) {
      Serial.println(WiFi.SSID(i));
      connections.append("{\"name\": \"");
      connections.append(WiFi.SSID(i).c_str());
      connections.append("\"}");
      if (i < n - 1) {
        connections.append(",");
      }
    }
    connections.append("]");
  }
  scanLastCallMs = millis();
}

void handleConnections() {
  server.send(200, "appliation/json; charset=utf-8", connections.c_str());
}

void handleConnect() {
  if (server.method() != HTTP_POST) {
    server.send(405, "text/plain", "Method Not Allowed");
  } else {
    scanEnabled = false;
    String SSID = server.arg("SSID");
    String password = server.arg("password");
    Serial.print("SSID: ");
    Serial.print(SSID);
    Serial.print(" password: ");
    Serial.println(password);

    WiFi.begin(SSID, password);

    server.send(200, "text/plain", "OK");
  }
}

void handleConnected() {
  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("WiFi connected");
    Serial.println("IP address: ");
    Serial.println(WiFi.localIP());
    server.send(200, "text/plain", "{\"value\":\"OK\"}");
    scanEnabled = true;
  } else if (WiFi.status() == WL_IDLE_STATUS || WiFi.status() == WL_DISCONNECTED) {
    server.send(201, "text/plain", "{\"value\":\"WAIT\"}");
  } else {
    Serial.println("Failed to connect");
    server.send(500, "text/plain", "{\"value\":\"FAIL\"}");
    scanEnabled = true;

  }
}

void setup(void) {
  delay(1000);
  Serial.begin(115200);
  Serial.println();
  Serial.print("Configuring access point...");
  WiFi.softAP("SMART");

  IPAddress myIP = WiFi.softAPIP();
  Serial.print("AP IP address: ");
  Serial.println(myIP);
  server.on("/", handleRoot);
  server.on("/connections", handleConnections);
  server.on("/connect", handleConnect);
  server.on("/connected", handleConnected);

  scanAndSaveConnections();
  server.begin();
  Serial.println("HTTP server started");

}

void loop(void) {
  server.handleClient();
  MDNS.update();
  scanAndSaveConnections();
}
