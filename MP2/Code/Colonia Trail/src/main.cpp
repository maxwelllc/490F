#include <Arduino.h>
#include <Wire.h>
#include <SPI.h>
#include <Adafruit_LIS3DH.h>
#include <Adafruit_Sensor.h>
#include <Shape.hpp>
#include <ArduinoSTL.h>
#include <vector>

// Accelerometer setup
// Used for software SPI
#define LIS3DH_CLK 13
#define LIS3DH_MISO 12
#define LIS3DH_MOSI 11
// Used for hardware & software SPI
#define LIS3DH_CS 10
Adafruit_LIS3DH lis = Adafruit_LIS3DH(LIS3DH_CS, LIS3DH_MOSI, LIS3DH_MISO, LIS3DH_CLK);

// Screen setup
#define SCREEN_WIDTH 128 // OLED _display width, in pixels
#define SCREEN_HEIGHT 64 // OLED _display height, in pixels

// Declaration for an SSD1306 _display connected to I2C (SDA, SCL pins)
#define OLED_RESET 4 // Reset pin # (or -1 if sharing Arduino reset pin)
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

const int DELAY_LOOP_MS = 50;
const int NUM_ASTEROIDS_PER_LEVEL = 2;
const int MAX_WAVES = 5;
const int ASTEROID_SPEED = 1;

class Ship : public Rectangle
{
public:
  Ship(int x, int y, int width, int height) : Rectangle(x, y, width, height)
  {
  }
};

class Asteroid : public Circle
{
protected:
  bool hasPassedShip = false;
  bool display = false;

public:
  Asteroid(int x, int y, int radius) : Circle(x, y, radius)
  {
  }

  bool getHasPassed()
  {
    return hasPassedShip;
  }

  void setHasPassed(bool newVal)
  {
    hasPassedShip = newVal;
  }

  bool getDisplay()
  {
    return display;
  }

  void setDisplay(bool newVal)
  {
    display = newVal;
  }
};

Ship ship(5, SCREEN_HEIGHT / 2, 5, 5);

std::vector<Asteroid> asteroids;

enum GameState
{
  MENU,
  MINIGAME,
  GAME_OVER
};

GameState state = MINIGAME;
int points = 0;

void minigame();
int convertAccelToMovement();

void setup()
{
  // SSD1306_SWITCHCAPVCC = generate _display voltage from 3.3V internally
  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3D))
  { // Address 0x3D for 128x64
    Serial.println(F("SSD1306 allocation failed"));
    for (;;)
      ; // Don't proceed, loop forever
  }
}

void loop()
{
  // put your main code here, to run repeatedly:
  display.clearDisplay();

  if (state == MINIGAME)
  {
    minigame();
  }
  else
  {
  }

  display.display();
  delay(DELAY_LOOP_MS);
}

void minigame()
{
  int movement = convertAccelToMovement();
  ship.setY(ship.getY() + movement);
  ship.forceInside(0, 0, display.width(), display.height());

  int maxRight = 0;
  for (int i = 0; i < asteroids.size(); i++)
  {
    asteroids[i].setX(asteroids[i].getX() - ASTEROID_SPEED);
    asteroids[i].draw(display);

    Serial.println(asteroids[i].toString());

    if (asteroids[i].getRight() < ship.getLeft() && asteroids[i].getHasPassed() == false)
    {
      points++;
      asteroids[i].setHasPassed(true);
    }
  }
}

int convertAccelToMovement()
{
  lis.read();
  int rawVal = lis.x;
  int movement = map(rawVal, -5000, 5000, -5, 5); // TODO: I have no idea what the input range is
  return movement;
}