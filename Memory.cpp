#include "Memory.h"

Memory::Memory(fs::FS &fileSystem) : fileSystem(&fileSystem)
{
    if(!fileSystem.exists(wifiConnectionFile.c_str())){
        File file = fileSystem.open(wifiConnectionFile.c_str(), "w");
        Serial.println("[Memory] Wifi config file created");
        file.close();
    }

    if(!fileSystem.exists(wifiConnectionAttemptsFile.c_str())){
        File file = fileSystem.open(wifiConnectionAttemptsFile.c_str(), "w");
        Serial.println("[Memory] Wifi connection attempts file created");
        file.close();
    }

    if(!fileSystem.exists(rootPagesDirectory.c_str())){
        fileSystem.mkdir(rootPagesDirectory.c_str());
        Serial.println("[Memory] Root pages folder created");
    }
}

// --------------------------------- FILE SYSTEM

void Memory::createDirectory(String path)
{
    Serial.printf("[Memory] Creating Directory: %s\n", path);
    if(fileSystem->mkdir(path)){
        Serial.println("[Memory] Directory created");
    } else {
        Serial.println("[Memory] mkdir function failed");
    }
}

void Memory::removeDirectory(String path) {
    Serial.printf("[Memory] Removing Dir: %s\n", path);
    if(fileSystem->rmdir(path)){
        Serial.println("[Memory] Directory removed");
    } else {
        Serial.println("[Memory] rmdir failed");
    }
}

void Memory::writeFile(String path, String message) {
    Serial.printf("[Memory] Writing file: %s\n", path);
    File file = fileSystem->open(path, "w");
    if(!file){
        Serial.println("[Memory] Failed to open file for writing");
        return;
    }
    if(file.print(message)){
        Serial.println("[Memory] File written");
    } else {
        Serial.println("[Memory] Write failed");
    }
    file.close();
}

void Memory::appendFile(String path, String message) {
    Serial.printf("[Memory] Appending to file: %s\r\n", path);

    File file = fileSystem->open(path, "a");
    if(!file){
        Serial.println("[Memory] - failed to open file for appending");
        return;
    }
    if(file.print(message)){
        Serial.println("[Memory] - message appended");
    } else {
        Serial.println("[Memory] - append failed");
    }
    file.close();
}

String Memory::readFile(String path) {
    Serial.printf("[Memory] Reading file: %s\n", path);
    String fileContent;

    File file = fileSystem->open(path, "r");
    if(!file || file.isDirectory()){
        Serial.println("[Memory] - failed to open file for reading");
        return "";
    }

    Serial.println("[Memory] - read from file:");
    while(file.available()){
        fileContent += String((char)file.read());
    }
    file.close();

    return fileContent;
}

void Memory::deleteFile(String path) {
    Serial.printf("[Memory] Deleting file: %s\n", path);
    if(fileSystem->remove(path)){
        Serial.println("[Memory] File deleted");
    } else {
        Serial.println("[Memory] Delete failed");
    }
}

void Memory::renameFile(String oldPath, String newPath) {
    Serial.printf("[Memory] Renaming file %s to %s\n", oldPath, newPath);
    if (fileSystem->rename(oldPath, newPath)) {
        Serial.println("[Memory] File renamed");
    } else {
        Serial.println("[Memory] Rename failed");
    }
}

void Memory::listDirectory(String path) {
    Serial.printf("[Memory] Listing directory: %s\n", path);

    File root = fileSystem->open(path, "r");
    if(!root){
        Serial.println("[Memory] - failed to open directory");
        return;
    }
    if(!root.isDirectory()){
        Serial.println("[Memory] - not a directory");
        return;
    }

    File file = root.openNextFile();
    while(file){
        if(file.isDirectory()){
            Serial.print("[Memory]   DIR : ");
            Serial.println(file.name());
        } else {
            Serial.print("[Memory]   FILE: ");
            Serial.print(file.name());
            Serial.print("  SIZE: ");
            Serial.println(file.size());
        }
        file = root.openNextFile();
    }
}

// --------------------------------- WIFI

void Memory::addNewWifiConnection(String ssid, String password) {
    String wifiConfig = "\"" + ssid + "\":\"" + password + "\"";
    File file = this->fileSystem->open(this->wifiConnectionFile.c_str(), "r");
    String history = file.readString();
    file.close();

    wifiConfig = history.length() > 0 ? history + "," + wifiConfig : wifiConfig;

    file = this->fileSystem->open(this->wifiConnectionFile.c_str(), "w");
    if (file) {
        file.print(wifiConfig);
        file.close();
        Serial.printf("[Memory] The wifi config has been saved in %s\n", this->wifiConnectionFile.c_str());
    } else {
        Serial.println("[Memory] File open failed");
    }
}

String Memory::getWifiPassword(String ssid)
{
    File file = this->fileSystem->open(this->wifiConnectionFile.c_str(), "r");
    if (!file)
    {
        Serial.println("[Memory] File open failed");
        return "";
    }

    String fileContent = "{" + file.readString() + "}";


    StaticJsonDocument<200> doc;
    DeserializationError error = deserializeJson(doc, fileContent);
    if (error)
    {
        Serial.println("[Memory] Failed to read file, using default configuration");
        return "";
    }

    file.close();

    if(doc.containsKey(ssid)){
        return doc[ssid];
    }
    else{
        return "";
    }
}

DynamicJsonDocument Memory::getWifiConnections()
{
    File file = this->fileSystem->open(this->wifiConnectionFile.c_str(), "r");
    if (!file)
    {
        Serial.println("[Memory] File open failed");
        return JsonObject();
    }

    String fileContent = "{" + this->readFile(this->wifiConnectionFile) + "}";
    file.close();

    DynamicJsonDocument doc(200);
    DeserializationError error = deserializeJson(doc, fileContent);
    if (error)
    {
        Serial.println("[Memory] Failed to read file, using default configuration");
    }

    return doc;
}

void Memory::deleteWifiConnection(String ssid)
{
    String fileContent = "";
    StaticJsonDocument<200> doc;
    File file = this->fileSystem->open(this->wifiConnectionFile.c_str(), "r");
    if (!file){
        Serial.println("[Memory] File open failed");
        return;
    }

    fileContent = "{" + file.readString() + "}";
    DeserializationError error = deserializeJson(doc, fileContent);
    if (error){
        Serial.println("[Memory] Failed to read file, using default configuration");
        return;
    }
    file.close();

    doc.remove(ssid);

    file = this->fileSystem->open(this->wifiConnectionFile.c_str(), "w");
    serializeJson(doc, fileContent);
    if (!file)
    {
        file.print(fileContent);
        Serial.println("[Memory] File open failed");
        return;
    }
    file.close();
}

void Memory::deleteAllWifiConnections()
{
    this->fileSystem->remove(this->wifiConnectionFile.c_str());
    Serial.println("[Memory] Wifi config deleted");
    // open new again
    File file = this->fileSystem->open(this->wifiConnectionFile.c_str(), "w");
    file.close();
}

// --------------------------------- WIFI CONNECTION ATTEMPTS

void Memory::addNewWifiConnectionAttempt(String datetime, String password)
{
    String wifiConfig = "\"" + datetime + "\":\"" + password + "\"";
    File historyFile = this->fileSystem->open(this->wifiConnectionAttemptsFile.c_str(), "r");
    String history = historyFile.readString();
    if(history.length() > 0){
        wifiConfig = history + "," + wifiConfig;
    }
    historyFile.close();

    File file = this->fileSystem->open(this->wifiConnectionAttemptsFile.c_str(), "w");
    if (file)
    {
        file.print(wifiConfig);
        file.close();
        Serial.printf("[Memory] The attempt has been saved in %s - Size: %d bytes\n", this->wifiConnectionAttemptsFile.c_str(), file.size());
    }
    else
    {
        Serial.println("[Memory] Attempt file open failed");
    }
}

DynamicJsonDocument Memory::getWifiConnectionAttempts()
{
    File file = this->fileSystem->open(this->wifiConnectionAttemptsFile.c_str(), "r");
    if (!file)
    {
        Serial.println("[Memory] File open failed");
        return JsonObject();
    }

    String fileContent = "{" + file.readString() + "}";
    file.close();

    DynamicJsonDocument doc(200);
    DeserializationError error = deserializeJson(doc, fileContent);
    if (error)
    {
        Serial.println("[Memory] Failed to read file, using default configuration");
    }

    return doc;
}

void Memory::deleteWifiConnectionAttempts()
{
    this->fileSystem->remove(this->wifiConnectionAttemptsFile.c_str());
    Serial.println("[Memory] Wifi connection attempts deleted");
    // open new again
    File file = this->fileSystem->open(this->wifiConnectionAttemptsFile.c_str(), "w");
    file.close();
}

// --------------------------------- HTML

String Memory::returnDefaultHtmlPage()
{
    if (this->defaultPage == "") {
        return defaultPageHtml;
    } else {
        return this->readFile(this->defaultPage);
    }
}

String Memory::returnWifiLogoBase64()
{
    return wifiLogoBase64;
}


