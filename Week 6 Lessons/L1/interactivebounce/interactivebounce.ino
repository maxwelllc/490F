#include <SPI.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

#define SCREEN_WIDTH 128 // OLED display width, in pixels
#define SCREEN_HEIGHT 64 // OLED display height, in pixels

// Declaration for an SSD1306 display connected to I2C (SDA, SCL pins)
#define OLED_RESET     4 // Reset pin # (or -1 if sharing Arduino reset pin)
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

int width = 20;
int height = 10;
int x = SCREEN_WIDTH/2;
int y = SCREEN_HEIGHT/2;
int xSpeed = 0;
int ySpeed = 0;

const int FSR_PIN = A0;

void setup() {
  Serial.begin(9600);

  // SSD1306_SWITCHCAPVCC = generate display voltage from 3.3V internally
  if(!display.begin(SSD1306_SWITCHCAPVCC, 0x3D)) { // Address 0x3D for 128x64
    Serial.println(F("SSD1306 allocation failed"));
    for(;;); // Don't proceed, loop forever
  }

  xSpeed = random(1,3);
  ySpeed = random(1,3);
  
  display.clearDisplay();   

}

void loop() {
  display.clearDisplay();
  int fsrVal = analogRead(FSR_PIN);
  Serial.println(fsrVal);
  int speedScale = map(fsrVal, 0, 1028, 1, 10);
  x += (xSpeed * speedScale);
  y += (ySpeed * speedScale);

  if(x <= 0 || x + width >= SCREEN_WIDTH) {
    xSpeed = xSpeed * -1;
  }
  if(y <= 0 || y + height >= SCREEN_HEIGHT) {
    ySpeed = ySpeed * -1;
  }

  display.fillRect(x, y, width, height, SSD1306_WHITE);
  


  display.display();
}

void drawRandomCircles(int numShapes) {
  for(int i = 0;i<numShapes;i++){
    int x = random(0, SCREEN_WIDTH);
    int y = random(0, SCREEN_HEIGHT);
    int radius = random(1, 15);
    
    display.drawCircle(x, y, radius, SSD1306_WHITE);
  }
  
}
