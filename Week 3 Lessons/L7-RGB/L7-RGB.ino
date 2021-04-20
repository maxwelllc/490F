const int RGB_BLUE_PIN = 3;
const int RGB_GREEN_PIN = 5;
const int RGB_RED_PIN = 6;
const int DELAY_MS = 20;
const int MAX_COLOR_VALUE = 255;
const boolean COMMON_ANODE = false; 

enum RGB{
  RED,
  GREEN,
  BLUE,
  NUM_COLORS
};

int _rgbLedValues[] = {255, 0, 0}; // Red, Green, Blue
enum RGB _curFadingUpColor = GREEN;
enum RGB _curFadingDownColor = RED;
const int FADE_STEP = 5;  

void setup() {
  // put your setup code here, to run once:
  
  pinMode(RGB_RED_PIN, OUTPUT);
  pinMode(RGB_GREEN_PIN, OUTPUT);
  pinMode(RGB_BLUE_PIN, OUTPUT);

  Serial.begin(9600); 
  Serial.println("Red, Green, Blue");
  setColor(_rgbLedValues[RED], _rgbLedValues[GREEN], _rgbLedValues[BLUE]);
  delay(DELAY_MS);
}

void loop() {
  // put your main code here, to run repeatedly:
  
  _rgbLedValues[_curFadingUpColor] += FADE_STEP;
  _rgbLedValues[_curFadingDownColor] -= FADE_STEP;

  if(_rgbLedValues[_curFadingUpColor] > MAX_COLOR_VALUE) {
    _rgbLedValues[_curFadingUpColor] = MAX_COLOR_VALUE;
    _curFadingUpColor = (RGB) ((int)_curFadingUpColor+1);

    if(_curFadingUpColor > (int)BLUE) {
      _curFadingUpColor = RED;
    }
  }

  if(_rgbLedValues[_curFadingDownColor] < 0) {
    _rgbLedValues[_curFadingDownColor] = 0;
    _curFadingDownColor = (RGB)((int)_curFadingDownColor+1);

    if(_curFadingDownColor > (int)BLUE) {
      _curFadingDownColor = RED;
    }
  }

  setColor(_rgbLedValues[RED], _rgbLedValues[GREEN], _rgbLedValues[BLUE]);
  delay(DELAY_MS);
}

void setColor(int red, int green, int blue)
{
  Serial.print(red);
  Serial.print(", ");
  Serial.print(green);
  Serial.print(", ");
  Serial.println(blue);

  // If a common anode LED, invert values
  if(COMMON_ANODE == true){
    red = MAX_COLOR_VALUE - red;
    green = MAX_COLOR_VALUE - green;
    blue = MAX_COLOR_VALUE - blue;
  }
  analogWrite(RGB_RED_PIN, red);
  analogWrite(RGB_GREEN_PIN, green);
  analogWrite(RGB_BLUE_PIN, blue);  
}
