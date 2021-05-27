#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <Adafruit_LIS3DH.h>

const int DELAY_MS = 5;

const int ANALOG_INPUT_PIN = A0;
const int MAX_ANALOG_INPUT = 1023;

String _lastAnalogVal = "0,0,0";

#define SCREEN_WIDTH 128 // OLED display width, in pixels
#define SCREEN_HEIGHT 64 // OLED display height, in pixels

// Declaration for an SSD1306 display connected to I2C (SDA, SCL pins)
#define OLED_RESET     4 // Reset pin # (or -1 if sharing Arduino reset pin)
Adafruit_SSD1306 _display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);


// Accelerometer, i2c
Adafruit_LIS3DH lis = Adafruit_LIS3DH();

void setup() {
  Serial.begin(115200); // set baud rate to 115200
  // SSD1306_SWITCHCAPVCC = generate display voltage from 3.3V internally
  if(!_display.begin(SSD1306_SWITCHCAPVCC, 0x3D)) { // Address 0x3D for 128x64
    Serial.println(F("SSD1306 allocation failed"));
    for(;;); // Don't proceed, loop forever
  }
  _display.setRotation(2);
  _display.clearDisplay();
  _display.setTextSize(1);      // Normal 1:1 pixel scale
  _display.setTextColor(SSD1306_WHITE); // Draw white text
  _display.setCursor(0, 0);     // Start at top-left corner
  _display.print("Waiting to receive\ndata from serial...");
  _display.display();
  
  // Init accel
  if (!lis.begin(0x18))
  { // change this to 0x19 for alternative i2c address
    while (1)
      yield();
  }
}

void loop() {

  // Get the new analog value
  lis.read();
  int x,y,z;
  x = constrain(map(lis.x, 10000, -10000, 0, 640), 0, 640);
  y = constrain(map(lis.y, -10000, 10000, 0, 480), 0, 480);
  z = constrain(map(lis.z, 16000, 10000, 20, 50), 20, 50);
  String val = String(x) + "," + String(y) + "," + String(z);
  

  // If the analog value has changed, send a new one over serial
  if(_lastAnalogVal != val){
    Serial.println(val); 
  }

    _lastAnalogVal = val;

    _display.clearDisplay();
    int16_t x1, y1;
    uint16_t textWidth, textHeight;

    _display.setTextSize(3);  

    // Measure one line of text     
    _display.getTextBounds("Test", 0, 0, &x1, &y1, &textWidth, &textHeight);
    int oneLineOfTextHeight = textHeight;

    // Measure the height of the received data
    _display.getTextBounds(val, 0, 0, &x1, &y1, &textWidth, &textHeight);

    // Automatically set font size and centering and display received text
    if(textHeight <= oneLineOfTextHeight){
      // Write small amounts of text to center of screen
      uint16_t yText = _display.height() / 2 - textHeight / 2;
      uint16_t xText = _display.width() / 2 - textWidth / 2; 
      _display.setCursor(xText, yText);
      _display.print(val);
      _display.display();
    }else{
      // Once text gets too large, write it smaller and start at 0,0
      _display.setTextSize(1);  
      _display.setCursor(0, 0);
      _display.print(val);
      _display.display();
    }
  
  delay(DELAY_MS);
}
