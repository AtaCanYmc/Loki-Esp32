//
// Created by Ata Can Yaymacı on 15.01.2024.
//

#include "Face.h"

Face::Face() {
    Serial.println("[Face] Face object created.");
}

void Face::setDisplay(Adafruit_SSD1306 &display) {
    this->display = &display;
    this->startDisplay();
    this->Welcome();
}

void Face::startDisplay() {
    if(!this->display->begin(SSD1306_SWITCHCAPVCC, 0x3C)) { // Address 0x3D for 128x64
        Serial.println(F("[Screen] SSD1306 allocation failed"));
        for(;;);
    }
    delay(2000);
    display->clearDisplay();
}

void Face::Welcome() {
    display->clearDisplay();
    display->setTextSize(1);
    display->setTextColor(WHITE);
    display->setCursor(55, 2);
    display->println("LOKI");
    display->setTextColor(WHITE);
    display->setCursor(45, 12);
    display->println("Welcome");
    display->display();
    this->clearTime = millis() + 2000;
}

void Face::clearRow(u_int8_t row) {
    display->setTextSize(1);
    display->setTextColor(BLACK);
    display->setCursor(0, row);
    display->println("                    ");
    display->display();
}

void Face::displayNotification(String text) {
    display->clearDisplay();
    display->setTextSize(1);
    display->setTextColor(WHITE);
    display->setCursor(0, 0);
    display->println(text);
    display->display();
    this->clearTime = millis() + 5000;
}

void Face::idle(String mode, String ip, String date, String time) {
    display->clearDisplay();
    display->setTextColor(WHITE);
    display->setTextSize(1);

    if(this->idleIndex%2 == 0) {
        display->drawRect(display->width()/2-13, 3, 10, 2, WHITE);
        display->drawRect(display->width()/2+7, 3, 10, 2, WHITE);
    } else {
        display->fillCircle(display->width()/2-10, 5, 5, WHITE);
        display->fillCircle(display->width()/2+10, 5, 5, WHITE);
    }

    display->fillRect(display->width()/2-10, 15, 21, 3, WHITE);

    display->setCursor(0, 35);
    display->println("Date => " + date);

    display->setCursor(0, 45);
    display->println("Time => " + time);

    display->setCursor(0, 55);
    display->println(mode + " => " + ip);

    display->display();
    this->clearTime = millis() + this->faceMode[idleIndex%2];
    this->idleIndex = (this->idleIndex + 1) % 2;
}