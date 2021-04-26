#include <Arduino.h>

const int PIEZO_PIN = 3;
const int LED_PIN = LED_BUILTIN;
const int FSR_PIN = A0;
const int DELAY = 20;

void setup() {
    pinMode(PIEZO_PIN, OUTPUT);
    pinMode(LED_PIN, OUTPUT);
    pinMode(FSR_PIN, INPUT);
    
}

void loop() {
    int fsrVal = analogRead(FSR_PIN);
    int ledVal = map(fsrVal, 0, 1023, 0, 255);
    int freq = map(fsrVal, 0, 1023, 50, 1500);

    if(fsrVal > 0) {
        tone(PIEZO_PIN, freq);
    }else {
        noTone(PIEZO_PIN);
    }
    Serial.print(fsrVal);
    Serial.print("\t");
    Serial.println(freq);
    analogWrite(LED_PIN, ledVal);
    delay(DELAY);

}