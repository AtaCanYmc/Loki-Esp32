//
// Created by Ata Can Yaymacı on 29.12.2023.
//

#include "Voice.h"

Voice::Voice() {
    pinMode(BUZZER_PIN, OUTPUT);
    pinMode(ALARM_BUTTON_PIN, INPUT);
    pinMode(LED_PIN, OUTPUT);
    pinMode(VIBRATION_PIN, OUTPUT);

    digitalWrite(BUZZER_PIN, LOW);
    digitalWrite(LED_PIN, LOW);
    digitalWrite(VIBRATION_PIN, LOW);
}

void Voice::alarm() { // alarm until button pressed
    int alarmCount = 30;
    bool beforeLed = digitalRead(LED_PIN);
    bool beforeVibration = digitalRead(VIBRATION_PIN);
    if(!this->isMute) {
        while((!digitalRead(ALARM_BUTTON_PIN) == LOW) && alarmCount > 0) {
            tone(BUZZER_PIN, 1000);
            this->led(HIGH);
            this->vibration(LOW);
            delay(100);

            tone(BUZZER_PIN, 2000);
            this->led(LOW);
            this->vibration(HIGH);
            delay(100);

            tone(BUZZER_PIN, 3000);
            this->led(HIGH);
            this->vibration(HIGH);
            delay(100);

            tone(BUZZER_PIN, 2000);
            this->led(LOW);
            this->vibration(HIGH);
            delay(100);

            tone(BUZZER_PIN, 1000);
            this->led(HIGH);
            this->vibration(LOW);
            delay(100);

            noTone(BUZZER_PIN);
            this->led(LOW);
            this->vibration(HIGH);
            delay(200);

            alarmCount--;
        }
    }
    this->led(beforeLed);
    this->vibration(beforeVibration);
}

void Voice::emergency() { // alarm until button pressed
    bool beforeLed = digitalRead(LED_PIN);
    bool beforeVibration = digitalRead(VIBRATION_PIN);
    if(!this->isMute) {
        for (int i = 0; i < 7; ++i) {
            tone(BUZZER_PIN, 500);
            this->led(HIGH);
            this->vibration(HIGH);
            delay(600);

            noTone(BUZZER_PIN);
            this->led(LOW);
            this->vibration(LOW);
            delay(200);
        }
    }
    this->led(beforeLed);
    this->vibration(beforeVibration);
}

void Voice::notification() { // alarm until button pressed
    bool beforeLed = digitalRead(LED_PIN);
    bool beforeVibration = digitalRead(VIBRATION_PIN);
    if(!this->isMute) {
        for (int i = 0; i < 2; ++i) {
            tone(BUZZER_PIN, 1000);
            this->led(HIGH);
            this->vibration(HIGH);
            delay(300);

            noTone(BUZZER_PIN);
            this->led(LOW);
            this->vibration(LOW);
            delay(200);
        }
    }
    digitalWrite(LED_PIN, beforeLed);
    digitalWrite(VIBRATION_PIN, beforeVibration);
}

void Voice::led(bool state) {
    if (this->isLedOn) {
        digitalWrite(LED_PIN, state);
    }
}

void Voice::vibration(bool state) {
    if (this->isVibrationOn) {
        digitalWrite(VIBRATION_PIN, state);
    }
}

void Voice::blink(int count) {
    bool beforeLed = digitalRead(LED_PIN);
    for (int i = 0; i < count; ++i) {
        this->led(HIGH);
        delay(300);
        this->led(LOW);
        delay(300);
    }
    this->led(beforeLed);
}

void Voice::blinkVibration(int count) {
    bool beforeVibration = digitalRead(VIBRATION_PIN);
    for (int i = 0; i < count; ++i) {
        this->vibration(HIGH);
        delay(300);

        this->led(LOW);
        delay(300);
    }
    this->vibration(beforeVibration);
}

bool Voice::checkAlarmButton() {
    return digitalRead(ALARM_BUTTON_PIN) == LOW;
}
