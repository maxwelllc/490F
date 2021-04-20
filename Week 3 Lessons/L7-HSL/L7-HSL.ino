#include "src/RGBConverter.h"

const int RGB_BLUE_PIN = 3;
const int RGB_GREEN_PIN = 5;
const int RGB_RED_PIN = 6;
const int DELAY_MS = 5;
const int MAX_COLOR_VALUE = 255;
const boolean COMMON_ANODE = false; 


float _hue = 0; //hue varies between 0 - 1
float _step = 0.001f;

RGBConverter _rgbConverter;

 

void setup() {
  // put your setup code here, to run once:
  
  pinMode(RGB_RED_PIN, OUTPUT);
  pinMode(RGB_GREEN_PIN, OUTPUT);
  pinMode(RGB_BLUE_PIN, OUTPUT);

  // Turn on Serial so we can verify expected colors via Serial Monitor
  Serial.begin(9600);   
}

void loop() {
  // put your main code here, to run repeatedly:
  
  // Convert current hue, saturation, and lightness to RGB
  // The library assumes hue, saturation, and lightness range from 0 - 1
  // and that RGB ranges from 0 - 255
  // If lightness is equal to 1, then the RGB LED will be white
  byte rgb[3];
  _rgbConverter.hslToRgb(_hue, 1, 0.5, rgb);

  Serial.print("hue=");
  Serial.print(_hue);
  Serial.print(" r=");
  Serial.print(rgb[0]);
  Serial.print(" g=");
  Serial.print(rgb[1]);
  Serial.print(" b=");
  Serial.println(rgb[2]);
  
  setColor(rgb[0], rgb[1], rgb[2]); 

  // update hue based on step size
  _hue += _step;

  // hue ranges between 0-1, so if > 1, reset to 0
  if(_hue > 1.0){
    _hue = 0;
  }

  delay(DELAY_MS);
}

void setColor(int red, int green, int blue)
{
  // If a common anode LED, invert values
  if(COMMON_ANODE == true){
    red = MAX_COLOR_VALUE - red;
    green = MAX_COLOR_VALUE - green;
    blue = MAX_COLOR_VALUE  - blue;
  }
  analogWrite(RGB_RED_PIN, red);
  analogWrite(RGB_GREEN_PIN, green);
  analogWrite(RGB_BLUE_PIN, blue);  
}
