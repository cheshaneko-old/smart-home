#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <ESP8266WebServer.h>
#include <ESP8266mDNS.h>
#include <DNSServer.h>
#include <string>

class WiFiConnect {
private:
    String connections = "[]";
    const char *myHostname = "smart";
    unsigned long scanLastCallMs = 0;
    const byte DNS_PORT = 53;
    bool scanEnabled = true;
    DNSServer dnsServer;
    IPAddress apIP = IPAddress(8, 8, 8, 8);
    IPAddress netMsk = IPAddress(255, 255, 255, 0);
    ESP8266WebServer server;
public:
    void setup() {
        delay(1000);
        Serial.begin(115200);
        Serial.println();
        Serial.print("Configuring access point...");
        WiFi.softAPConfig(apIP, apIP, netMsk);
        WiFi.softAP("SMART");

        IPAddress myIP = WiFi.softAPIP();
        Serial.print("AP IP address: ");
        Serial.println(myIP);

        /* Setup the DNS server redirecting all the domains to the apIP */
        dnsServer.setErrorReplyCode(DNSReplyCode::NoError);
        dnsServer.start(DNS_PORT, "*", apIP);


        server.on("/", [this]() {this->handleRoot();});
        server.on("/connections", [this]() {this->handleConnections();});
        server.on("/connect", [this]() {this->handleConnect();});
        server.on("/connected", [this]() {this->handleConnected();});
        server.on("/generate_204", [this]() {this->handleRoot();});  //Android captive portal. Maybe not needed. Might be handled by notFound handler.
        server.on("/fwlink", [this]() {this->handleRoot();});  //Microsoft captive portal. Maybe not needed. Might be handled by notFound handler.
        server.onNotFound([this]() {this->handleNotFound();});


        scanAndSaveConnections();
        server.begin();
        Serial.println("HTTP server started");

        MDNS.begin(myHostname);
        MDNS.addService("http", "tcp", 80);
    }

    void loop() {
        dnsServer.processNextRequest();
        server.handleClient();
        //MDNS.update();
        scanAndSaveConnections();
    }

private:
/** Is this an IP? */
    boolean isIp(String str) {
        for (size_t i = 0; i < str.length(); i++) {
            int c = str.charAt(i);
            if (c != '.' && (c < '0' || c > '9')) {
                return false;
            }
        }
        return true;
    }

/** IP to String? */
    String toStringIp(IPAddress ip) {
        String res = "";
        for (int i = 0; i < 3; i++) {
            res += String((ip >> (8 * i)) & 0xFF) + ".";
        }
        res += String(((ip >> 8 * 3)) & 0xFF);
        return res;
    }

/** Redirect to captive portal if we got a request for another domain. Return true in that case so the page handler do not try to handle the request again. */
    boolean captivePortal() {
        if (!isIp(server.hostHeader()) && server.hostHeader() != (String(myHostname) + ".local")) {
            Serial.println("Request redirected to captive portal");
            server.sendHeader("Location", String("http://") + toStringIp(server.client().localIP()), true);
            server.send(302, "text/plain",
                        "");   // Empty content inhibits Content-length header so we have to close the socket ourselves.
            server.client().stop(); // Stop is needed because we sent no content length
            return true;
        }
        return false;
    }

    void handleRoot() {
        if (captivePortal()) { // If caprive portal redirect instead of displaying the page.
            return;
        }
        server.sendHeader("Cache-Control", "no-cache, no-store, must-revalidate");
        server.sendHeader("Pragma", "no-cache");
        server.sendHeader("Expires", "-1");
        server.sendHeader("Content-Encoding", "gzip");
        server.send(200, "text/html; charset=utf-8", (char *) htmlPage, (size_t)
        sizeof(htmlPage));
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
                connections += "{\"name\": \"" + WiFi.SSID(i) + "\"}";
                if (i < n - 1) {
                    connections += ",";
                }
            }
            connections += "]";
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

    void handleNotFound() {
        if (captivePortal()) { // If caprive portal redirect instead of displaying the error page.
            return;
        }
        String message = F("File Not Found\n\n");
        message += F("URI: ");
        message += server.uri();
        message += F("\nMethod: ");
        message += (server.method() == HTTP_GET) ? "GET" : "POST";
        message += F("\nArguments: ");
        message += server.args();
        message += F("\n");

        for (uint8_t i = 0; i < server.args(); i++) {
            message += String(F(" ")) + server.argName(i) + F(": ") + server.arg(i) + F("\n");
        }
        server.sendHeader("Cache-Control", "no-cache, no-store, must-revalidate");
        server.sendHeader("Pragma", "no-cache");
        server.sendHeader("Expires", "-1");
        server.send(404, "text/plain", message);
    }
};
