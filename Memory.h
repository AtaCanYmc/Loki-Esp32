#ifndef CYCLOPS_V2_0_MEMORY_H
#define CYCLOPS_V2_0_MEMORY_H

#include <Arduino.h>
#include <LittleFS.h>
#include <FS.h>
#include "Base64.h"
#include <ArduinoJson.h>
#include "defaultPageHtml.h"

#define FORMAT_LITTLEFS_IF_FAILED true

class Memory {
    public:
        fs::FS *fileSystem;

        Memory(fs::FS &fileSystem);

        void createDirectory(String path);
        void removeDirectory(String path);
        void writeFile(String path, String message);
        void appendFile(String path, String message);
        String readFile(String path);
        void deleteFile(String path);
        void renameFile(String oldPath, String newPath);
        void listDirectory(String path);

        void addNewWifiConnection(String ssid, String password);
        String getWifiPassword(String ssid);
        DynamicJsonDocument getWifiConnections();
        void deleteWifiConnection(String ssid);
        void deleteAllWifiConnections();

        void addNewWifiConnectionAttempt(String datetime, String password);
        DynamicJsonDocument getWifiConnectionAttempts();
        void deleteWifiConnectionAttempts();

        String returnDefaultHtmlPage();
        String returnWifiLogoBase64();

    private:
        String wifiConnectionFile = "/wifiConnections.json";
        String wifiConnectionAttemptsFile = "/wifiConnectionAttempts.json";
        String rootPagesDirectory = "/rootPages";
        String defaultPage = "";

};

#endif //CYCLOPS_V2_0_MEMORY_H
