//
// Created by Ata Can Yaymacı on 29.01.2024.
//

#ifndef LOKI_V1_0_CAPTIVEPORTALHELPER_H
#define LOKI_V1_0_CAPTIVEPORTALHELPER_H

#include <WiFi.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <Arduino.h>
#include <DNSServer.h>
#include "defaultPageHtml.h"

class CaptivePortalHelper : public AsyncWebHandler {
public:
    CaptivePortalHelper() {}
    ~CaptivePortalHelper() {}

    bool canHandle(AsyncWebServerRequest *request) override {
        return request->url() == "/";
    }

    void handleRequest(AsyncWebServerRequest *request) override {
        Serial.println("Handle request called");

        if (request->method() == HTTP_GET && request->url() == "/") {
            Serial.println("Handling / endpoint");
            request->send(200, "text/html", defaultPageHtml);
        } else {
            request->send(200, "text/html", defaultPageHtml);
        }
    }
};


#endif //LOKI_V1_0_CAPTIVEPORTALHELPER_H
