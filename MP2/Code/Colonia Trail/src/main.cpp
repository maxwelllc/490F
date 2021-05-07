#include <Arduino.h>
#include <Wire.h>
#include <SPI.h>
#include <Adafruit_LIS3DH.h>
#include <Adafruit_Sensor.h>
#include <Shape.hpp>;

// Used for software SPI
#define LIS3DH_CLK 13
#define LIS3DH_MISO 12
#define LIS3DH_MOSI 11
// Used for hardware & software SPI
#define LIS3DH_CS 10
Adafruit_LIS3DH lis = Adafruit_LIS3DH(LIS3DH_CS, LIS3DH_MOSI, LIS3DH_MISO, LIS3DH_CLK);

#define SCREEN_WIDTH 128 // OLED _display width, in pixels
#define SCREEN_HEIGHT 64 // OLED _display height, in pixels

// Declaration for an SSD1306 _display connected to I2C (SDA, SCL pins)
#define OLED_RESET     4 // Reset pin # (or -1 if sharing Arduino reset pin)
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);


class Ship : public Rectangle {
  public:
    Ship(int x, int y, int width, int height) : Rectangle(x, y, width, height)
    {
    }
};

Ship ship(5, SCREEN_HEIGHT / 2, 5, 5);

enum GameState {
  MENU,
  MINIGAME,
  GAME_OVER
};

GameState state = MINIGAME;

void minigame();

void setup() {
  // SSD1306_SWITCHCAPVCC = generate _display voltage from 3.3V internally
  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3D)) { // Address 0x3D for 128x64
    Serial.println(F("SSD1306 allocation failed"));
    for (;;); // Don't proceed, loop forever
  }


}

void loop() {
  // put your main code here, to run repeatedly:
}

void minigame() {

}

int convertAccelToMovement() {
  lis.read();
  int rawVal = lis.x;
  int movement = map(rawVal, -5000, 5000, -5, 5); // TODO: I have no idea what the input range is
  return movement;
}