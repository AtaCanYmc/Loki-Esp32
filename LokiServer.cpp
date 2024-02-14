//
// Created by Ata Can Yaymacı on 15.01.2024.
//

#include "LokiServer.h"

LokiServer::LokiServer(AsyncWebServer &brainServer, Memory &memory) {
    this->brainServer = &brainServer;
    this->memory = &memory;
    this->voice = new Voice();
    this->dnsServer = new DNSServer();
}

void LokiServer::setFace(Face &face) {
    this->face = &face;
}

void LokiServer::setDeviceTime() {
    configTime(0, 0, "pool.ntp.org", "time.nist.gov");
    Serial.println("[LOKI-SERVER] Waiting for NTP time sync: ");
    time_t now = time(nullptr);
    while (now < 8 * 3600 * 2) {
        delay(500);
        Serial.print(".");
        now = time(nullptr);
    }
    Serial.println("");
    struct tm timeinfo;
    gmtime_r(&now, &timeinfo);
    Serial.print("[LOKI-SERVER] Current time: ");
    Serial.print(asctime(&timeinfo));
}

String LokiServer::getDateTime() {
    time_t now = time(nullptr);
    struct tm *timeinfo;
    timeinfo = localtime(&now);
    char buffer[80];
    strftime(buffer, 80, "%d-%m-%Y %H:%M:%S", timeinfo);
    return buffer;
}

String LokiServer::getDate() {
    time_t now = time(nullptr);
    struct tm *timeinfo;
    timeinfo = localtime(&now);
    char buffer[80];
    strftime(buffer, 80, "%d-%m-%Y", timeinfo);
    return buffer;
}

String LokiServer::getTime() {
    time_t now = time(nullptr);
    struct tm *timeinfo;
    timeinfo = localtime(&now);
    char buffer[80];
    strftime(buffer, 80, "%H:%M:%S", timeinfo);
    return buffer;
}

void LokiServer::setupServer() {
    if (this->brainServer == nullptr || this->memory == nullptr || this->voice == nullptr) {
        Serial.println("[LOKI-SERVER] Brain server, memory or voice is null");
        return;
    }

    if (WiFi.getMode() == WIFI_AP) {
        Serial.println("[LOKI-SERVER] Running access point");
        this->mode = "AP";
    } else if (WiFi.isConnected()) {
        Serial.println("[LOKI-SERVER] Connected to wifi");
        this->mode = "Wifi";
    } else {
        Serial.println("[LOKI-SERVER] No connection");
        this->mode = "Null";
    }

    if (this->isServerRunning) {
        this->brainServer->end();
        this->isServerRunning = false;
        Serial.println("[LOKI-SERVER] Brain server stopped");
    }

    Serial.println("[LOKI-SERVER] Brain server started");
    this->dnsServer->setErrorReplyCode(DNSReplyCode::NoError);
    this->dnsServer->setTTL(300);
    this->dnsServer->start(DNS_PORT, "*", WiFi.softAPIP());

    Serial.println("[LOKI-SERVER] Setup Server");
    this->brainServer->addHandler(new CaptivePortalHelper());
    this->restServerRouting();
}

void LokiServer::restServerRouting() {
    Serial.println("[LOKI-SERVER] Rest Server Routing");

    brainServer->on("/root", HTTP_GET, [&](AsyncWebServerRequest *request) { // root
        handleRoot(request);
    });

    brainServer->on("/device", HTTP_GET, [&](AsyncWebServerRequest *request) { // device
        getDevice(request);
    });

    brainServer->on("/restart", HTTP_GET, [&](AsyncWebServerRequest *request) { // restart device
        restartDevice(request);
    });

    brainServer->on("/wifi/scan", HTTP_GET, [&](AsyncWebServerRequest *request) { // wifi scan
        getWifiScan(request);
    });

    brainServer->on("/wifi/connection/list", HTTP_GET,
                    [&](AsyncWebServerRequest *request) { // get saved wifi connections
                        getSavedWifiConnections(request);
                    });

    brainServer->on("/wifi/connection/current", HTTP_GET,
                    [&](AsyncWebServerRequest *request) { // get current wifi connection
                        getWifiConnection(request);
                    });

    brainServer->on("/wifi/connection/specific", HTTP_DELETE,
                    [&](AsyncWebServerRequest *request) { // delete wifi connection from list
                        deleteWifiConnection(request);
                    });

    brainServer->on("/wifi/connection/list", HTTP_DELETE,
                    [&](AsyncWebServerRequest *request) { // delete all wifi connections from list
                        deleteAllWifiConnections(request);
                    });

    brainServer->on("/wifi/connection/specific", HTTP_GET,
                    [&](AsyncWebServerRequest *request) { // get wifi connection from list
                        getConnectionInformation(request);
                    });

    brainServer->on("/voice", HTTP_GET, [&](AsyncWebServerRequest *request) { // get voice infos
        getVoice(request);
    });

    brainServer->on("/wifi/attempt/list", HTTP_GET, [&](AsyncWebServerRequest *request) { // get attempts
        getPasswordAttempts(request);
    });

    brainServer->on("/wifi/attempt/list", HTTP_DELETE, [&](AsyncWebServerRequest *request) { // delete attempts
        deletePasswordAttempts(request);
    });

    brainServer->on("/wifi/attempt", HTTP_GET, [&](AsyncWebServerRequest *request) { // add new attempt
        addNewAttemptWithGet(request);
    });

    brainServer->on("/wifi/access-point", HTTP_DELETE, [&](AsyncWebServerRequest *request) { // stop access point
        stopAccessPoint(request);
    });

    brainServer->onNotFound([&](AsyncWebServerRequest *request) { // not found
        handleNotFound(request);
        //return true;
    });

    brainServer->onRequestBody(
            [&](AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total) {
                Serial.println("[LOKI-SERVER] Request body");
                Serial.println((char *) data);

                if (request->url() == "/wifi/connection") {
                    setWifiConnection(request, data);
                } else if (request->url() == "/wifi/connection/list") {
                    addNewWifiConnection(request, data);
                } else if (request->url() == "/voice") {
                    setVoice(request, data);
                } else if (request->url() == "/voice/sound") {
                    makeSound(request, data);
                } else if (request->url() == "/wifi/access-point") {
                    runAccessPoint(request, data);
                } else if (request->url() == "/wifi/attempt") {
                    addNewPasswordAttempt(request, data);
                }
            });

    brainServer->on("/*", HTTP_OPTIONS, [&](AsyncWebServerRequest *request) {
        request->send(200);
    });
    DefaultHeaders::Instance().addHeader("Access-Control-Allow-Origin", "*");
    DefaultHeaders::Instance().addHeader("Access-Control-Allow-Methods", "GET, POST, PUT, DELETE");
}

void LokiServer::returnError(AsyncWebServerRequest *request, errorContainer error) {
    String errorJson;
    StaticJsonDocument<200> doc;
    doc["date"] = error.date;
    doc["error"] = error.error;
    doc["errorType"] = error.errorType;
    doc["errorCode"] = error.errorCode;
    serializeJson(doc, errorJson);
    request->send(error.errorCode, "application/json", errorJson);
}

void LokiServer::handleNotFound(AsyncWebServerRequest *request) {
    Serial.println("[LOKI-SERVER] Not Found");
    String messageHTML = this->memory->returnDefaultHtmlPage();

    if (WiFi.isConnected()) {
        messageHTML.replace("YOUR_IP", WiFi.localIP().toString().c_str());
        messageHTML.replace("YOUR_SSID_NAME", WiFi.SSID().c_str());
    } else if (WiFi.getMode() == WIFI_AP) {
        messageHTML.replace("YOUR_IP", WiFi.softAPIP().toString().c_str());
        messageHTML.replace("YOUR_SSID_NAME", WiFi.softAPSSID().c_str());
    }
    request->send(200, "text/html", messageHTML);
}

void LokiServer::restartDevice(AsyncWebServerRequest *request) {
    String messageJson;
    StaticJsonDocument<200> doc;
    doc["date"] = getDateTime();
    doc["message"] = "Device is restarting";
    serializeJson(doc, messageJson);
    request->send(200, "application/json", messageJson);

    this->face->idle(mode, "Restarting", getDate(), getTime());
    this->face->clearTime = millis() + (3 * 1000);
    this->voice->notification();

    delay(5000);
    ESP.restart();
}

void LokiServer::handleRoot(AsyncWebServerRequest *request) {
    String messageHTML = this->memory->returnDefaultHtmlPage();

    if (WiFi.isConnected()) {
        messageHTML.replace("YOUR_IP", WiFi.localIP().toString().c_str());
        messageHTML.replace("YOUR_SSID_NAME", WiFi.SSID().c_str());
    } else if (WiFi.getMode() == WIFI_AP) {
        messageHTML.replace("YOUR_IP", WiFi.softAPIP().toString().c_str());
        messageHTML.replace("YOUR_SSID_NAME", WiFi.softAPSSID().c_str());
    }

    request->send(200, "text/html", messageHTML);
}

void LokiServer::getDevice(AsyncWebServerRequest *request) {
    String messageJson;
    StaticJsonDocument<1024> doc;
    doc["project"]["name"] = "Loki";
    doc["project"]["version"] = "1.0";
    doc["project"]["author"] = "Secret";
    doc["project"]["date"] = getDateTime();

    doc["chip"]["id"] = ESP.getChipModel();
    doc["chip"]["sdk"] = ESP.getSdkVersion();
    doc["chip"]["cpu"] = ESP.getCpuFreqMHz();
    doc["chip"]["flash"] = ESP.getFlashChipSize();
    doc["chip"]["flashSpeed"] = ESP.getFlashChipSpeed();
    doc["chip"]["freeHeap"] = ESP.getFreeHeap();

    doc["wifi"]["ssid"] = WiFi.SSID();
    doc["wifi"]["pwd"] = WiFi.psk();
    doc["wifi"]["rssi"] = WiFi.RSSI();
    doc["wifi"]["ip"] = WiFi.localIP().toString();
    doc["wifi"]["isConnected"] = WiFi.isConnected();
    doc["wifi"]["host"] = WiFi.getHostname();
    doc["wifi"]["dns"] = WiFi.dnsIP().toString();
    doc["wifi"]["mac"] = WiFi.macAddress();
    doc["wifi"]["gateway"] = WiFi.gatewayIP().toString();
    doc["wifi"]["subnet"] = WiFi.subnetMask().toString();

    doc["device"]["mode"] = this->mode;
    doc["device"]["dnsPort"] = DNS_PORT;
    doc["device"]["host"] = this->host;
    doc["device"]["isAPRunning"] = WiFi.getMode() == WIFI_AP;
    doc["device"]["isWifiConnected"] = WiFi.isConnected();
    doc["device"]["isWifiRunning"] = WiFi.getMode() == WIFI_STA;

    doc["voice"]["isMute"] = voice->isMute;
    doc["voice"]["isLedOn"] = voice->isLedOn;
    doc["voice"]["isVibrationOn"] = voice->isVibrationOn;
    doc["voice"]["buzzerPin"] = BUZZER_PIN;
    doc["voice"]["buttonPin"] = ALARM_BUTTON_PIN;
    doc["voice"]["ledPin"] = LED_PIN;
    doc["voice"]["vibrationPin"] = VIBRATION_PIN;
    doc["voice"]["voiceTypes"][0] = "alarm";
    doc["voice"]["voiceTypes"][1] = "emergency";
    doc["voice"]["voiceTypes"][2] = "notification";
    doc["voice"]["voiceTypes"][3] = "blink-led";
    doc["voice"]["voiceTypes"][4] = "blink-vibration";

    serializeJson(doc, messageJson);
    request->send(200, "application/json", messageJson);
}

void LokiServer::getWifiScan(AsyncWebServerRequest *request) {
    int n = WiFi.scanNetworks();
    String wifiScanJson;
    DynamicJsonDocument doc(1024 * 3);
    doc["date"] = getDateTime();
    doc["wifiCount"] = n;
    for (int i = 0; i < n; ++i) {
        doc["wifi"][i]["ssid"] = WiFi.SSID(i);
        doc["wifi"][i]["rssi"] = WiFi.RSSI(i);
        doc["wifi"][i]["encryption"] = WiFi.encryptionType(i);
        doc["wifi"][i]["bssid"] = WiFi.BSSIDstr(i);
        doc["wifi"][i]["channel"] = WiFi.channel(i);
    }
    serializeJson(doc, wifiScanJson);
    request->send(200, "application/json", wifiScanJson);
}

void LokiServer::getWifiConnection(AsyncWebServerRequest *request) {
    String messageJson;
    StaticJsonDocument<512> doc;
    doc["date"] = getDateTime();
    doc["ssid"] = WiFi.SSID();
    doc["pwd"] = WiFi.psk();
    doc["rssi"] = WiFi.RSSI();
    doc["ip"] = WiFi.localIP().toString();
    doc["isConnected"] = WiFi.isConnected();
    doc["host"] = WiFi.getHostname();
    doc["dns"] = WiFi.dnsIP().toString();
    doc["mac"] = WiFi.macAddress();
    doc["gateway"] = WiFi.gatewayIP().toString();
    doc["subnet"] = WiFi.subnetMask().toString();
    doc["deviceMode"] = mode;
    serializeJson(doc, messageJson);
    request->send(200, "application/json", messageJson);
}

void LokiServer::setWifiConnection(AsyncWebServerRequest *request, uint8_t *data) {
    String messageJson;
    StaticJsonDocument<512> doc;
    StaticJsonDocument<512> postBodyDoc;
    errorContainer errorCont;
    DeserializationError error = deserializeJson(postBodyDoc, data);

    if (error) {
        Serial.println("[LOKI-SERVER] Invalid JSON in POST body");
        errorCont.date = getDateTime();
        errorCont.error = "Invalid JSON";
        errorCont.errorType = "InvalidJSON";
        errorCont.errorCode = 400;
        returnError(request, errorCont);
        return;
    }

    String ssid = postBodyDoc["ssid"];
    String password = postBodyDoc["password"];

    delay(5000);

    this->runWifi = true;

    this->voice->notification();
    this->face->clearTime = millis() + (3 * 1000);
    this->setDeviceTime();

    doc["date"] = getDateTime();
    doc["message"] = "Wifi connection gonna start!";
    doc["ssid"] = ssid;
    doc["password"] = password;
    serializeJson(doc, messageJson);
    request->send(200, "application/json", messageJson);

}

bool LokiServer::connectToWifi(String ssid, String password) {
    if (WiFi.isConnected()) {
        Serial.println("[Wifi] Already connected to a wifi");
        WiFi.disconnect();
        Serial.println("[Wifi] Disconnected from wifi");
    } else if (WiFi.getMode() == WIFI_AP) {
        Serial.println("[Wifi] Already running access point");
        // stop access point
        WiFi.softAPdisconnect(true);
        Serial.println("[Wifi] Access point stopped");
    }

    Serial.print("[Wifi] Connecting to wifi: ");
    Serial.println(ssid);

    WiFi.begin(ssid, password);

    while (WiFi.status() != WL_CONNECTED) {
        Serial.print('.');
        delay(500);
    }

    Serial.println("\n[Wifi] Wifi Connected");
    Serial.print("[Wifi] RRSI: ");
    Serial.println(WiFi.RSSI());
    Serial.print("[Wifi] IP ADDRESS: ");
    Serial.println(WiFi.localIP());

    if (WiFi.isConnected()) {
        this->mode = "Wifi";
    } else {
        this->mode = "Null";
    }

    return WiFi.isConnected();
}

void LokiServer::addNewWifiConnection(AsyncWebServerRequest *request, uint8_t *data) {
    String messageJson;
    StaticJsonDocument<512> doc;
    StaticJsonDocument<512> postBodyDoc;
    errorContainer errorCont;
    DeserializationError error = deserializeJson(postBodyDoc, data);

    if (error) {
        Serial.println("[LOKI-SERVER] Invalid JSON in POST body");
        errorCont.date = getDateTime();
        errorCont.error = "Invalid JSON";
        errorCont.errorType = "InvalidJSON";
        errorCont.errorCode = 400;
        returnError(request, errorCont);
        return;
    }

    String ssid = postBodyDoc["ssid"];
    String password = postBodyDoc["password"];

    memory->addNewWifiConnection(ssid, password);

    doc["date"] = getDateTime();
    doc["message"] = "Wifi connection added to memory";
    doc["ssid"] = ssid;
    doc["password"] = password;
    serializeJson(doc, messageJson);
    request->send(200, "application/json", messageJson);
}

void LokiServer::getSavedWifiConnections(AsyncWebServerRequest *request) {
    String messageJson;
    DynamicJsonDocument doc(1024);
    DynamicJsonDocument wifiConnections = memory->getWifiConnections();
    doc["date"] = getDateTime();
    doc["wifiConnections"] = wifiConnections;
    serializeJson(doc, messageJson);
    request->send(200, "application/json", messageJson);
}

void LokiServer::deleteWifiConnection(AsyncWebServerRequest *request) {
    String messageJson;
    StaticJsonDocument<512> doc;
    String ssid = request->arg("ssid");
    memory->deleteWifiConnection(ssid);

    doc["date"] = getDateTime();
    doc["message"] = "Wifi connection deleted from memory";
    doc["ssid"] = ssid;
    serializeJson(doc, messageJson);
    request->send(200, "application/json", messageJson);
}

void LokiServer::deleteAllWifiConnections(AsyncWebServerRequest *request) {
    String messageJson;
    StaticJsonDocument<512> doc;
    doc["date"] = getDateTime();
    doc["message"] = "Wifi connections deleted from memory";
    serializeJson(doc, messageJson);
    request->send(200, "application/json", messageJson);
    memory->deleteAllWifiConnections();
}

void LokiServer::getConnectionInformation(AsyncWebServerRequest *request) {
    String ssid = request->arg("ssid");
    String messageJson;
    StaticJsonDocument<200> doc;

    doc["date"] = getDateTime();
    doc["ssid"] = ssid;
    doc["password"] = memory->getWifiPassword(ssid);
    serializeJson(doc, messageJson);
    request->send(200, "application/json", messageJson);
}

void LokiServer::getVoice(AsyncWebServerRequest *request) {
    String messageJson;
    StaticJsonDocument<512> doc;
    doc["date"] = getDateTime();
    doc["voice"]["isMute"] = voice->isMute;
    doc["voice"]["isLedOn"] = voice->isLedOn;
    doc["voice"]["isVibrationOn"] = voice->isVibrationOn;
    doc["voice"]["buzzerPin"] = BUZZER_PIN;
    doc["voice"]["buttonPin"] = ALARM_BUTTON_PIN;
    doc["voice"]["ledPin"] = LED_PIN;
    doc["voice"]["vibrationPin"] = VIBRATION_PIN;
    doc["voice"]["voiceTypes"][0] = "alarm";
    doc["voice"]["voiceTypes"][1] = "emergency";
    doc["voice"]["voiceTypes"][2] = "notification";
    doc["voice"]["voiceTypes"][3] = "blink-led";
    doc["voice"]["voiceTypes"][4] = "blink-vibration";
    serializeJson(doc, messageJson);
    request->send(200, "application/json", messageJson);
}

void LokiServer::setVoice(AsyncWebServerRequest *request, uint8_t *data) {
    StaticJsonDocument<512> postBodyDoc;
    DeserializationError error = deserializeJson(postBodyDoc, data);

    if (error) {
        Serial.println("[LOKI-SERVER] Invalid JSON in POST body");
        errorContainer errorCont;
        errorCont.date = getDateTime();
        errorCont.error = "Invalid JSON";
        errorCont.errorType = "InvalidJSON";
        errorCont.errorCode = 400;
        returnError(request, errorCont);
        return;
    }

    voice->isMute = postBodyDoc["isMute"];
    voice->isLedOn = postBodyDoc["isLedOn"];
    voice->isVibrationOn = postBodyDoc["isVibrationOn"];

    StaticJsonDocument<512> responseDoc;
    responseDoc["date"] = getDateTime();
    responseDoc["message"] = "Voice updated";
    responseDoc["voice"]["isMute"] = voice->isMute;
    responseDoc["voice"]["isLedOn"] = voice->isLedOn;
    responseDoc["voice"]["isVibrationOn"] = voice->isVibrationOn;
    String responseJson;
    serializeJson(responseDoc, responseJson);
    request->send(200, "application/json", responseJson);
}

void LokiServer::makeSound(AsyncWebServerRequest *request, uint8_t *data) {
    StaticJsonDocument<512> postBodyDoc;
    DeserializationError error = deserializeJson(postBodyDoc, data);

    if (error) {
        Serial.println("[LOKI-SERVER] Invalid JSON in POST body");
        errorContainer errorCont;
        errorCont.date = getDateTime();
        errorCont.error = "Invalid JSON";
        errorCont.errorType = "InvalidJSON";
        errorCont.errorCode = 400;
        returnError(request, errorCont);
        return;
    }

    String voiceType = postBodyDoc["voiceType"];

    if (voiceType == "alarm") {
        voice->alarm();
    } else if (voiceType == "emergency") {
        voice->emergency();
    } else if (voiceType == "notification") {
        voice->notification();
    } else if (voiceType == "blink-led") {
        int count = postBodyDoc["count"];
        voice->blink(count);
    } else if (voiceType == "blink-vibration") {
        int count = postBodyDoc["count"];
        voice->blinkVibration(count);
    } else {
        errorContainer errorCont;
        errorCont.date = getDateTime();
        errorCont.error = "Invalid voice type";
        errorCont.errorType = "Voice";
        errorCont.errorCode = 400;
        returnError(request, errorCont);
        return;
    }

    StaticJsonDocument<512> responseDoc;
    responseDoc["date"] = getDateTime();
    responseDoc["message"] = "Sound played";
    responseDoc["voice"]["isMute"] = voice->isMute;
    responseDoc["voice"]["buzzerPin"] = BUZZER_PIN;
    String responseJson;
    serializeJson(responseDoc, responseJson);
    request->send(200, "application/json", responseJson);
}

void LokiServer::runAccessPoint(AsyncWebServerRequest *request, uint8_t *data) {
    Serial.println("[LOKI-SERVER] Starting access point");
    if (request != nullptr) {
        String messageJson;
        StaticJsonDocument<512> doc;
        StaticJsonDocument<512> postBodyDoc;
        DeserializationError error = deserializeJson(postBodyDoc, data);

        if (error) {
            Serial.println("[LOKI-SERVER] Invalid JSON in POST body");
            errorContainer errorCont;
            errorCont.date = getDateTime();
            errorCont.error = "Invalid JSON";
            errorCont.errorType = "InvalidJSON";
            errorCont.errorCode = 400;
            returnError(request, errorCont);
            return;
        }

        this->ssidAP = postBodyDoc["ssid"].as<String>();
        this->isPublicAP = postBodyDoc["isPublic"].as<bool>();
        this->passwordAP = postBodyDoc["password"].as<String>();

        doc["date"] = getDateTime();
        doc["message"] = "Access point gonna start!";
        doc["ssid"] = this->ssidAP;
        doc["isPublic"] = this->isPublicAP;
        doc["password"] = this->passwordAP;
        serializeJson(doc, messageJson);
        request->send(200, "application/json", messageJson);
    } else {
        this->ssidAP = "Loki-V2.0";
        this->isPublicAP = false;
        this->passwordAP = "prince0fA$$gard";
        Serial.println("[LOKI-SERVER] Access point started");
    }

    this->runAP = true;
}

void LokiServer::stopAccessPoint(AsyncWebServerRequest *request) {
    if (request != nullptr) {
        StaticJsonDocument<512> doc;
        doc["date"] = getDateTime();
        doc["message"] = "Access point stopped";
        String messageJson;
        serializeJson(doc, messageJson);
        request->send(200, "application/json", messageJson);
    } else {
        Serial.println("[LOKI-SERVER] Access point stopped");
    }

    delay(5000);
    WiFi.softAPdisconnect(true);
    this->mode = "Null";
    this->face->idle(mode, " - ", getDate(), getTime());
    this->face->clearTime = millis() + (3 * 1000);
    this->voice->notification();
}

void LokiServer::addNewPasswordAttempt(AsyncWebServerRequest *request, uint8_t *data) {
    String messageJson;
    StaticJsonDocument<512> doc;
    StaticJsonDocument<512> postBodyDoc;
    errorContainer errorCont;
    DeserializationError error = deserializeJson(postBodyDoc, data);

    if (error) {
        Serial.println("[LOKI-SERVER] Invalid JSON in POST body");
        errorCont.date = getDateTime();
        errorCont.error = "Invalid JSON";
        errorCont.errorType = "InvalidJSON";
        errorCont.errorCode = 400;
        returnError(request, errorCont);
        return;
    }

    String password = postBodyDoc["password"];

    memory->addNewWifiConnectionAttempt(getDateTime(), password);

    doc["date"] = getDateTime();
    doc["message"] = "Password attempt added to memory";
    doc["password"] = password;
    serializeJson(doc, messageJson);
    request->send(200, "application/json", messageJson);
}

void LokiServer::addNewAttemptWithGet(AsyncWebServerRequest *request) {
    String password = request->arg("password");
    memory->addNewWifiConnectionAttempt(getDateTime(), password);

    String messageJson;
    StaticJsonDocument<512> doc;
    doc["date"] = getDateTime();
    doc["message"] = "Password attempt added to memory";
    doc["password"] = password;
    serializeJson(doc, messageJson);
    request->send(200, "application/json", messageJson);

    voice->notification();
    face->idle("Attempt", password, getDate(), getTime());
    face->clearTime = millis() + (3 * 1000);
}

void LokiServer::getPasswordAttempts(AsyncWebServerRequest *request) {
    String messageJson;
    DynamicJsonDocument doc(1024);
    DynamicJsonDocument passwordAttempts = memory->getWifiConnectionAttempts();
    doc["date"] = getDateTime();
    doc["passwordAttempts"] = passwordAttempts;
    serializeJson(doc, messageJson);
    request->send(200, "application/json", messageJson);
}

void LokiServer::deletePasswordAttempts(AsyncWebServerRequest *request) {
    String messageJson;
    StaticJsonDocument<512> doc;
    doc["date"] = getDateTime();
    doc["message"] = "Password attempts deleted from memory";
    serializeJson(doc, messageJson);
    request->send(200, "application/json", messageJson);
    memory->deleteWifiConnectionAttempts();
}

void LokiServer::startAccessPoint() {
    // check already exist
    if (WiFi.getMode() == WIFI_AP) {
        Serial.println("[Wifi] Already running access point");
        // stop access point
        stopAccessPoint(nullptr);
        Serial.println("[Wifi] Access point stopped");
    } else if (WiFi.isConnected()) {
        // close wifi connection
        WiFi.disconnect();
        Serial.println("[Wifi] Disconnected from wifi");
    }

    Serial.println("[Wifi] Running access point");
    WiFi.mode(WIFI_AP);

    if (this->isPublicAP) {
        WiFi.softAP(this->ssidAP);
    } else {
        WiFi.softAP(this->ssidAP, this->passwordAP);
    }

    Serial.print("[Wifi] IP ADDRESS: ");
    Serial.println(WiFi.softAPIP());

    this->face->idle(mode, "Creating A.P.", getDate(), getTime());
    this->face->clearTime = millis() + (3 * 1000);
    this->voice->notification();
    this->mode = "AP";
}
