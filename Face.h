//
// Created by Ata Can Yaymacı on 15.01.2024.
//
#ifndef LOKI_V1_0_FACE_H
#define LOKI_V1_0_FACE_H

#include <Arduino.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <Wire.h>

class Face {
public:
    Adafruit_SSD1306 *display;
    uint32_t clearTime = 0;
    int faceMode[2] = {200, 800};
    int idleIndex = 0;

    Face();
    void setDisplay(Adafruit_SSD1306 &display);
    void displayNotification(String text);
    void idle(String mode, String ip, String date, String time);
private:
    void startDisplay();
    void Welcome();
    void clearRow(u_int8_t row);

};


#endif //LOKI_V1_0_FACE_H
