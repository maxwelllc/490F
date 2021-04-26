#include <Arduino.h>
#include <CapacitiveSensor.h>

const int RGB_RED_PIN = 3;
const int RGB_GREEN_PIN =  5;
const int RGB_BLUE_PIN = 6;

const int MODE_SWITCH_BUTTON = 7;
const int MODE_SWITCH_LED = -1;
const int AUX_BUTTON = 8;
const int AUX_LED = -1;

const int SOIL_IN = A1;
const int SLIDER_IN = A0;

CapacitiveSensor cs_1 = CapacitiveSensor(2, 12);

// state = 1: crossfade() mode, press save button to pause the color
// state = 2: rgbSelector() mode, press leaf to toggle color, use slider to adjust brightness
// state = 3: plantHealth() mode, color and brightness will depend on moisture sensor and light
int state = 1; 
void setup() {
  // put your setup code here, to run once:
  Serial.begin(9600);
  pinMode(RGB_RED_PIN, OUTPUT);
  pinMode(RGB_GREEN_PIN, OUTPUT);
  pinMode(RGB_BLUE_PIN, OUTPUT);

  pinMode(MODE_SWITCH_BUTTON, INPUT);
  pinMode(MODE_SWITCH_LED, OUTPUT);
  pinMode(AUX_BUTTON, INPUT);
  pinMode(AUX_LED, OUTPUT);
}

void loop() {
  // put your main code here, to run repeatedly:
}


void crossfade() {

}

void rgbSelector() {

}

void plantHealth() {

}