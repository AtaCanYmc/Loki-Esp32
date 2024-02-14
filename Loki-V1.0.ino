
#include <WiFi.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <Arduino.h>
#include <ESPmDNS.h>
#include <LittleFS.h>
#include <FS.h>
#include "LokiServer.h"
#include "Memory.h"

// OLED
#define SCREEN_WIDTH 128 // OLED display width, in pixels
#define SCREEN_HEIGHT 64 // OLED display height, in pixels
// Declaration for an SSD1306 display connected to I2C (SDA, SCL pins)
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);

// Configure wifi setup
const char *con_ssid = "IoT_test";
const char *con_password = "nodemcuTest";
IPAddress ip;

// Configure access point
const char *ap_ssid = "Loki-V2.0";
const char *ap_password = "prince0fA$$gard"; // o as 0 and s as $
bool isPublic = true;

// Configure web server
AsyncWebServer server(80);
const char *host = "loki";

// define Memory
#define FORMAT_LITTLEFS_IF_FAILED true
Memory memory(LittleFS);

// define Face
Face face;

// define LokiServer
LokiServer lokiServer(server, memory);

void setup() {
    // start serial
    Serial.begin(115200);
    Serial.println("");
    Serial.println("[Loki] Loki V1.0");

    // start LittleFS
    startLittleFS();

    // start OLED
    startOLED();
    face.setDisplay(display);
    lokiServer.setFace(face);

    //startAP();
    connectWifi(con_ssid, con_password);
    setMDNS();

    // device time
    if (WiFi.status() == WL_CONNECTED) {
        setDeviceTime();
    }

    // start web server
    lokiServer.setupServer();
    Serial.println("[Loki] Web server started");
    server.begin();
    lokiServer.isServerRunning = true;
}

void loop() {
    lokiServer.dnsServer->processNextRequest();

    if (lokiServer.face->clearTime != 0 && millis() > lokiServer.face->clearTime) {
        if (lokiServer.mode == "AP") {
            lokiServer.face->idle(lokiServer.mode, WiFi.softAPIP().toString(), lokiServer.getDate(), lokiServer.getTime());
        } else if (lokiServer.mode == "Wifi") {
            lokiServer.face->idle(lokiServer.mode, WiFi.localIP().toString(), lokiServer.getDate(), lokiServer.getTime());
        } else {
            lokiServer.face->idle("Null", " - ", lokiServer.getDate(), lokiServer.getTime());
            if (lokiServer.voice->checkAlarmButton()){
                lokiServer.voice->notification();
                startAP();
                lokiServer.mode = "AP";
            }
        }
    }

    if (lokiServer.runAP) {
        Serial.println("[Loki] Main Menu Starting Access Point");
        lokiServer.startAccessPoint();
        lokiServer.runAP = false;
        lokiServer.setupServer();
        server.begin();
        lokiServer.isServerRunning = true;
    } else if (lokiServer.runWifi) {
        Serial.println("[Loki] Main Menu Connecting to Wifi");
        lokiServer.connectToWifi(lokiServer.ssidWifi, lokiServer.passwordWifi);
        lokiServer.runWifi = false;
        lokiServer.setupServer();
        server.begin();
        lokiServer.isServerRunning = true;
    }
}

// --------------------------------- Time

void setDeviceTime() {
    // Set device time from NTP servers for Turkey time zone
    configTime(3 * 3600, 0, "pool.ntp.org", "time.nist.gov");

    // Wait for time to be set
    time_t now = time(nullptr);
    if (now > 0) {
        Serial.println("[Time] Device time set successfully.");
    } else {
        Serial.println("[Time] Failed to set device time.");
    }
}

// --------------------------------- WIFI

void connectWifi(const char* ssid, const char* pwd){
    Serial.print("[Wifi] Connection found on: ");
    Serial.print(ssid);

    WiFi.mode(WIFI_STA);
    WiFi.begin(ssid, pwd);
    Serial.print("\n[Wifi] Connecting to WiFi ..");

    while (WiFi.status() != WL_CONNECTED) {
        Serial.print('.');
        delay(500);
    }
    Serial.println("\n[Wifi] Wifi Connected");

    Serial.print("[Wifi] RRSI: ");
    Serial.println(WiFi.RSSI());
    ip = WiFi.localIP();
    Serial.print("[Wifi] IP ADDRESS: ");
    Serial.println(ip);
}

void disconnectWifi(){
    WiFi.disconnect();
    Serial.println("[Wifi] Disconnected\n");
}

void startAP() {
    WiFi.mode(WIFI_AP);
    if (isPublic) {
        WiFi.softAP(ap_ssid);
    } else {
        WiFi.softAP(ap_ssid, ap_password);
    }
    Serial.println("[Wifi] ----------------------- Access Point");
    Serial.println("[Wifi] SSID: " + String(ap_ssid));
    Serial.println("[Wifi] Password: " + String(ap_password));
    Serial.println("[Wifi] IP: " + WiFi.softAPIP().toString());
}

void setMDNS(){
    int totalTry = 5;
    while (!MDNS.begin(host) && totalTry > 0) {
        Serial.println(".");
        delay(1000);
        totalTry--;
    }

    Serial.println("[Wifi] mDNS responder started");
    Serial.print("[Wifi] You can now connect to: http://");
    Serial.print(host);
    Serial.println(".local");
}

// --------------------------------- LittleFS

void startLittleFS() {
    if(!LittleFS.begin(FORMAT_LITTLEFS_IF_FAILED)) {
        Serial.println("[Memory] An Error has occurred while mounting LittleFS");
        ESP.restart();
    }
    Serial.println("[Memory] LittleFS mounted successfully");
}

// --------------------------------- OLED

void startOLED(){
    if(!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) { // Address 0x3D for 128x64
        Serial.println(F("[OLED] SSD1306 allocation failed"));
        for(;;);
        ESP.restart();
    }
    else {
        Serial.println("[OLED] SSD1306 allocation success");
    }
    delay(2000);
    display.clearDisplay();
}
