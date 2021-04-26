#include <Arduino.h>
#include <CapacitiveSensor.h>

// Frequencies (in Hz) of our piano keys
// From: https://en.wikipedia.org/wiki/Piano_key_frequencies
#define KEY_C 262  // 261.6256 Hz (middle C)
#define KEY_D 294  // 293.6648 Hz
#define KEY_E 330  // 329.6276 Hz
#define KEY_F 350  // 349.2282 Hz
#define KEY_G 392  // 391.9954 Hz
#define KEY_A 440  // 440.0000 Hz
#define KEY_B 494  // 493.8833 Hz

const int PIEZO_PIN = 7;
const int LED_PIN = LED_BUILTIN;

const int INPUT_BUTTON_C_PIN = 2;
const int INPUT_BUTTON_D_PIN = 3;
const int INPUT_BUTTON_E_PIN = 4;
const int INPUT_BUTTON_F_PIN = 5;
const int INPUT_BUTTON_G_PIN = 6;

const boolean buttonsAreActiveLow = true;

CapacitiveSensor  plantA = CapacitiveSensor(13,10);
CapacitiveSensor plantB = CapacitiveSensor(12, 9);

boolean isButtonPressed(int btnPin){
  int btnVal = digitalRead(btnPin);
  if(buttonsAreActiveLow && btnVal == LOW){
    // button is hooked up with pull-up resistor
    // and is in a pressed state
    digitalWrite(LED_PIN, HIGH);
    return true;
  }else if(!buttonsAreActiveLow && btnVal == HIGH){
    // button is hooked up with a pull-down resistor
    // and is in a pressed state
    digitalWrite(LED_PIN, HIGH);
    return true;
  }

  // button is not pressed
  return false;
}

void setup() {
  Serial.begin(9600);
  pinMode(INPUT_BUTTON_C_PIN, INPUT_PULLUP);
  pinMode(INPUT_BUTTON_D_PIN, INPUT_PULLUP);
  pinMode(INPUT_BUTTON_E_PIN, INPUT_PULLUP);
  pinMode(INPUT_BUTTON_F_PIN, INPUT_PULLUP);
  pinMode(INPUT_BUTTON_G_PIN, INPUT_PULLUP);
  pinMode(PIEZO_PIN, OUTPUT);
  pinMode(LED_PIN, OUTPUT);
}

void loop() {
  long plantATotal = plantA.capacitiveSensor(30);
  long plantBTotal = plantB.capacitiveSensor(30);
  Serial.print(plantATotal);
  Serial.print("\t");
  Serial.println(plantBTotal);
  if(isButtonPressed(INPUT_BUTTON_C_PIN)){
    tone(PIEZO_PIN, KEY_C);
  }else if(isButtonPressed(INPUT_BUTTON_D_PIN)){
    tone(PIEZO_PIN, KEY_D);
  }else if(isButtonPressed(INPUT_BUTTON_E_PIN)){
    tone(PIEZO_PIN, KEY_E);
  }else if(isButtonPressed(INPUT_BUTTON_F_PIN)){
    tone(PIEZO_PIN, KEY_F);
  }else if(isButtonPressed(INPUT_BUTTON_G_PIN)){
    tone(PIEZO_PIN, KEY_G);
  }else if(plantATotal > 300) {
    tone(PIEZO_PIN, KEY_A);
  }else if(plantBTotal > 300) {
    tone(PIEZO_PIN, KEY_B);
  }else{
    noTone(PIEZO_PIN); 
    digitalWrite(LED_PIN, LOW);
  }
  delay(50);
}
