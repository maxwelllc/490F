#include <Arduino.h>

const int PIEZO_PIN = 3;
const int LED_PIN = LED_BUILTIN;
const int DELAY = 500;

void setup() {
    pinMode(PIEZO_PIN, OUTPUT);
    pinMode(LED_PIN, OUTPUT);
}

void loop() {

    tone(PIEZO_PIN, 392);
    digitalWrite(LED_PIN, HIGH);
    delay(DELAY);

    tone(PIEZO_PIN, 262);
    digitalWrite(LED_PIN, LOW);
    delay(DELAY);

}