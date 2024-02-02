//
// Created by Ata Can Yaymacı on 15.01.2024.
//

#ifndef LOKI_V1_0_LOKISERVER_H
#define LOKI_V1_0_LOKISERVER_H

#include <WiFi.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <Arduino.h>
#include <DNSServer.h>
#include <ESPmDNS.h>
#include "CaptivePortalHelper.h"
#include "Memory.h"
#include "Voice.h"
#include "Face.h"

struct errorContainer {
    String date;
    String error;
    String errorType;
    int errorCode;
};

#define DNS_PORT 53

class LokiServer {
public:
    AsyncWebServer *brainServer;
    Memory *memory;
    Voice *voice;
    Face *face;
    DNSServer *dnsServer;

    String mode = "AP";
    bool isServerRunning = false;
    bool runAP = false;
    bool runWifi = false;
    String ssidAP = "Loki-V2.0";
    String passwordAP = "prince0fA$$gard";
    bool isPublicAP = false;
    String ssidWifi = "";
    String passwordWifi = "";

    bool isAlarm = false;
    bool isNotification = false;
    bool isEmergency = false;

    LokiServer(AsyncWebServer &brainServer, Memory &memory);
    void setFace(Face &face);
    void restServerRouting();
    void setupServer();
    void setDeviceTime();
    String getDateTime();
    String getDate();
    String getTime();
    void startAccessPoint();
    bool connectToWifi(String ssid, String password);
private:
    String host = "loki";

    void restartDevice(AsyncWebServerRequest *request);
    void handleNotFound(AsyncWebServerRequest *request);
    void handleRoot(AsyncWebServerRequest *request);
    void getDevice(AsyncWebServerRequest *request);
    void returnError(AsyncWebServerRequest *request, errorContainer error);

    void getWifiScan(AsyncWebServerRequest *request);
    void setWifiConnection(AsyncWebServerRequest *request, uint8_t *data);
    void getWifiConnection(AsyncWebServerRequest *request);

    void addNewWifiConnection(AsyncWebServerRequest *request, uint8_t *data);
    void getSavedWifiConnections(AsyncWebServerRequest *request);
    void deleteWifiConnection(AsyncWebServerRequest *request);
    void deleteAllWifiConnections(AsyncWebServerRequest *request);
    void getConnectionInformation(AsyncWebServerRequest *request);

    void addNewPasswordAttempt(AsyncWebServerRequest *request, uint8_t *data);
    void addNewAttemptWithGet(AsyncWebServerRequest *request);
    void getPasswordAttempts(AsyncWebServerRequest *request);
    void deletePasswordAttempts(AsyncWebServerRequest *request);

    void getVoice(AsyncWebServerRequest *request);
    void setVoice(AsyncWebServerRequest *request, uint8_t *data);
    void makeSound(AsyncWebServerRequest *request, uint8_t *data);

    void runAccessPoint(AsyncWebServerRequest *request, uint8_t *data);
    void stopAccessPoint(AsyncWebServerRequest *request);
};


#endif //LOKI_V1_0_LOKISERVER_H
