#include <Arduino.h>

const int POT_PIN = A0;
const int LED_PIN = 3;
const int DELAY = 50;

void setup() {
    pinMode(LED_PIN, OUTPUT);
    Serial.begin(9600);
}

void loop() {

    int potVal = analogRead(POT_PIN);
    Serial.println(potVal);
    int ledVal = map(potVal, 0, 1023, 0, 255);
    analogWrite(LED_PIN, ledVal);
    delay(DELAY);


}