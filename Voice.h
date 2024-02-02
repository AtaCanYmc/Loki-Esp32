//
// Created by Ata Can Yaymacı on 29.12.2023.
//

#ifndef CYCLOPS_V2_0_VOICE_H
#define CYCLOPS_V2_0_VOICE_H
#include <Arduino.h>

#define BUZZER_PIN 5
#define ALARM_BUTTON_PIN 17
#define LED_PIN 18
#define VIBRATION_PIN 19

class Voice {
public:
    bool isMute = false;
    bool isLedOn = true;
    bool isVibrationOn = true;

    Voice();
    void alarm();
    void emergency();
    void notification();
    void led(bool state);
    void blink(int count);
    void vibration(bool state);
    void blinkVibration(int count);
    bool checkAlarmButton();
};


#endif //CYCLOPS_V2_0_VOICE_H
