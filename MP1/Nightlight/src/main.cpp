#include <Arduino.h>
#include <CapacitiveSensor.h>
#include <RGBConverter.h>

const int RGB_RED_PIN = 3;
const int RGB_GREEN_PIN = 5;
const int RGB_BLUE_PIN = 6;

const int MODE_SWITCH_BUTTON = 7;
const int MODE_SWITCH_LED = -1;
const int AUX_BUTTON = 8;
const int AUX_LED = -1;

const int SOIL_IN = A1;
const int SLIDER_IN = A0;
const int PHOTOCELL_IN = A2;

const int DEBOUNCE_WINDOW = 30; // ms
const int DELAY_INTERVAL = 30;  // ms

const int MAX_RGB_VALUE = 255;
const boolean COMMON_ANODE = false;
const int TIME_TO_FADE = 30;

float hue = 0;
float step = 0.001f;
RGBConverter rgbConverter;
unsigned long lastLEDUpdateMs = 0; // Last time we updated the LED color - For breathing animation
int fadeAmount = 5; // Amount to fade - For breathing animation 
int currFadeBrightness = 0; // Current modifier for RGB - For breathing animation

// Arbitrary thresholds for plantHealth(), on scale 0-1028
// Note that soil moisture DECREASES as sensor value increases
const int lightMax = 900;
const int lightMin = 300;
const int waterMoist = 300;
const int waterDry = 500;

byte savedRGB[3] = {0, 0, 0};
boolean lockRGB = false;

// Capacitive sensor created between pins 2 (emitter) and 12 (sensor)
CapacitiveSensor cs_1 = CapacitiveSensor(2, 12);

// Capacitive sensor created between pins 4 (emitter) and 9 (sensor)
CapacitiveSensor cs_2 = CapacitiveSensor(4, 9);

int cs1Threshold = -1;
int cs2Threshold = -1;

// state = 1: crossfade() mode, press save button to pause the color
// state = 2: rgbSelector() mode, press leaf to toggle color, use slider to adjust brightness
// state = 3: plantHealth() mode, color and brightness will depend on moisture sensor and light
int state = 1;

void crossfade();
void plantHealth();
void rgbSelector();
void setColor(int red, int green, int blue);
void calibrateTouch();

void setup()
{
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

void loop()
{
  // Need to calibrate touch on first run, the <50 check is to ensure the calibration didn't run and
  // set an unreasonably low value
  if (cs1Threshold < 50 || cs2Threshold < 50)
  {
    calibrateTouch();
    return;
  }

  // When we are on the locked mode, light up the button for some feedback!
  // Note that locking doesn't actually do anything in plantHealth() mode
  if (lockRGB)
  {
    digitalWrite(AUX_LED, HIGH);
  }
  else
  {
    digitalWrite(AUX_LED, LOW);
  }

  int modeButtonVal = digitalRead(MODE_SWITCH_BUTTON);
  int saveButtonVal = digitalRead(AUX_BUTTON);
  delay(DEBOUNCE_WINDOW);
  int modeButtonVal2 = digitalRead(MODE_SWITCH_BUTTON);
  int saveButtonVal2 = digitalRead(AUX_BUTTON);

  // Increment mode on mode button press
  if (modeButtonVal == HIGH && modeButtonVal == modeButtonVal2)
  {
    state++;
    if (state > 3)
    {
      state = 1;
    }
  }

  // Toggle lock state on AUX button press
  if (saveButtonVal == HIGH && saveButtonVal == saveButtonVal2)
  {
    lockRGB = !lockRGB;
  }

  if (state == 1)
  {
    crossfade();
  }
  else if (state == 2)
  {
    rgbSelector();
  }
  else
  {
    plantHealth();
  }

  delay(DELAY_INTERVAL);
}

// Crossfades using HSL, brightness is inversely proportional to light level
void crossfade()
{
  int photoCellVal = analogRead(PHOTOCELL_IN);
  int brightness = map(photoCellVal, 0, 1023, 0, 255);

  rgbConverter.hslToRgb(hue, 1, 0.5, savedRGB);
  for (int i = 0; i <= 3; i++)
  {
    savedRGB[i] = savedRGB[i] - brightness;
    if (savedRGB[i] < 10)
    {
      savedRGB[i] = 10;
    }
  }

  setColor(savedRGB[0], savedRGB[1], savedRGB[2], false);

  // If we locked RGB, don't increment hue
  if (lockRGB == false)
  {
    hue += step;
    if (hue > 1.0)
    {
      hue = 0;
    }
  }
}

void rgbSelector()
{

  long total1 = cs_1.capacitiveSensor(30);
  long total2 = cs_2.capacitiveSensor(30);
  Serial.print("Touch: \t");
  Serial.print(total1);
  Serial.print("\t");
  Serial.println(total2);

  int sliderVal = analogRead(SLIDER_IN);
  int brightness = map(sliderVal, 0, 1023, 0, 255);

  if (total1 > cs1Threshold && !lockRGB)
  {
    hue += step;
    if (hue > 1.0)
    {
      hue = 0;
    }
  }
  else if (total2 > cs2Threshold && !lockRGB)
  {
    hue -= step;
    if (hue < 0)
    {
      hue = 1.0;
    }
  }

  rgbConverter.hslToRgb(hue, 1, 0.5, savedRGB);
  for (int i = 0; i <= 3; i++)
  {
    savedRGB[i] = savedRGB[i] - brightness;
    if (savedRGB[i] < 10)
    {
      savedRGB[i] = 10;
    }
  }

  setColor(savedRGB[0], savedRGB[1], savedRGB[2], false);
}

void plantHealth()
{
  int moistureVal = analogRead(SOIL_IN);
  int water = map(moistureVal, 0, 1023, 0, 255);
  int photoCellVal = analogRead(PHOTOCELL_IN);
  int light = map(photoCellVal, 0, 1023, 0, 255);
  int happiness = 0;

  if (moistureVal < waterMoist)
  {
    // If we're overwatered, directly show that with blue
    savedRGB[2] = water;
    savedRGB[0] = 0;
    savedRGB[1] = 0;
  }
  else if (moistureVal > waterDry)
  {
    // If we're underwatered, go red depending on how dry, plus green (to make orange) depending on light
    savedRGB[0] = 255 - water;
    if (light > lightMax)
    {
      savedRGB[1] = 150 - (light / 2);
    }
  }
  else
  { 
    // If water is good, the plant is receptive to touch!
    // Glow slightly green, intensify depending on touch levels
    long total1 = cs_1.capacitiveSensor(30);
    long total2 = cs_2.capacitiveSensor(30);
    Serial.print("Touch: \t");
    Serial.print(total1);
    Serial.print("\t");
    Serial.println(total2);
    
    savedRGB[0] = 0;
    savedRGB[1] = 15;
    savedRGB[2] = 0;

    if(total1 > cs1Threshold) {
      savedRGB[1] += map(total1, cs1Threshold, 6000, 0, 120);
    }
    if(total2 > cs2Threshold) {
      savedRGB[1] += map(total2, cs2Threshold, 6000, 0, 120);
    }
  }

  for (int i = 0; i <= 3; i++)
  {
    if (savedRGB[i] < 0)
    {
      savedRGB[i] = 0;
    }
  }
  setColor(savedRGB[0], savedRGB[1], savedRGB[2], true);
}

// Setting pulse to true will fade up/down by +- 20 per value, creating a breathing effect
void setColor(int red, int green, int blue, boolean pulse = false)
{
  if(pulse) {
    unsigned long currentTime = millis();
    if(currentTime - lastLEDUpdateMs >= TIME_TO_FADE) {
      // If its been long enough, increment fade modifier by fadeAmount
      currFadeBrightness += fadeAmount;
      // If we go past min or max modifier, swap fade direction
      if(currFadeBrightness > 20 || currFadeBrightness < -20) {
        fadeAmount = -fadeAmount;
      }
    }
    lastLEDUpdateMs = currentTime;
  } else {
    currFadeBrightness = 0;
  }
  
  if (COMMON_ANODE == true)
  {
    red = MAX_RGB_VALUE - red;
    green = MAX_RGB_VALUE - green;
    blue = MAX_RGB_VALUE - blue;
  }
  
  // Ensure no negative values or values over 255
  red = min(max(0, red+currFadeBrightness), 255);
  green = min(max(0, green+currFadeBrightness), 255);
  blue = min(max(0, blue+currFadeBrightness), 255);
  
  analogWrite(RGB_RED_PIN, red);
  analogWrite(RGB_GREEN_PIN, green);
  analogWrite(RGB_BLUE_PIN, blue);
}

// Touch one of the leaves and then press the AUX_BUTTON to calibrate that leaf.
// Repeat as needed! Will calibrate depending on whichever has the highest raw signal, since it seems
// the raw value is accurate enough for that.
void calibrateTouch()
{
  long total1 = cs_1.capacitiveSensor(30);
  long total2 = cs_2.capacitiveSensor(30);
  setColor(255, 255, 255, false);
  long auxButtonVal = digitalRead(AUX_BUTTON);
  delay(DEBOUNCE_WINDOW);
  int auxButtonVal2 = digitalRead(AUX_BUTTON);

  if (auxButtonVal == HIGH && auxButtonVal == auxButtonVal2)
  {
    if (total1 > total2)
    {
      cs1Threshold = total1 / 2;
    }
    else
    {
      cs2Threshold = total2 / 2;
    }
  }
  if (cs1Threshold >= 50 && cs1Threshold >= 50)
  {
    setColor(0, 255, 0, false);
    delay(100);
    setColor(0, 0, 0, false);
    delay(100);
    setColor(0, 255, 0, false);
    delay(100);
  }
}