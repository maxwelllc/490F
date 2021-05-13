#include <Arduino.h>
#include <Wire.h>
#include <SPI.h>
#include <Adafruit_LIS3DH.h>
#include <Adafruit_Sensor.h>
#include <Shape.hpp>

// Accelerometer, i2c
Adafruit_LIS3DH lis = Adafruit_LIS3DH();

// Screen setup
#define SCREEN_WIDTH 128 // OLED _display width, in pixels
#define SCREEN_HEIGHT 64 // OLED _display height, in pixels

// Declaration for an SSD1306 _display connected to I2C (SDA, SCL pins)
#define OLED_RESET 4 // Reset pin # (or -1 if sharing Arduino reset pin)
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

#define DELAY_LOOP_MS 50
#define NUM_ASTEROIDS 10
#define MAX_WAVES 5
#define ASTEROID_SPEED 2

#define LY_TO_COLONIA 22000

int16_t x, y;
uint16_t textWidth, textHeight;
const char strTravel[] = "Travelling to Colonia";
const char strVictory[] = "Welcome to Colonia!";
const char strProgress[] = "Progress:";
const char strCr[] = "Cr:";
const char strHealth[] = "HP:";
const char strPoints[] = "Points:";
const char strLy1[] = "You've travelled ";
const char strLy2[] = " lightyears.";
const char strAsteroidBelt[] = "You've hit an asteroid belt! Avoid 15 asteroids to break free.";
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
  Asteroid(int x, int y, int radius, bool display) : Circle(x, y, radius)
  {
    display = display;
    setDrawFill(true);
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
int lyTraveled = 0;
int credits = 0;
int shipHealth = 100;

Asteroid asteroids[NUM_ASTEROIDS] = {Asteroid(0, 0, 0, false), Asteroid(0, 0, 0, false), Asteroid(0, 0, 0, false), Asteroid(0, 0, 0, false),
                                     Asteroid(0, 0, 0, false), Asteroid(0, 0, 0, false), Asteroid(0, 0, 0, false), Asteroid(0, 0, 0, false),
                                     Asteroid(0, 0, 0, false), Asteroid(0, 0, 0, false)};
int asteroidsOnScreen = 0;

enum GameState
{
  MENU,
  TRAVEL,
  MINIGAME,
  GAME_OVER
};

GameState state = TRAVEL;
int points = 0;

void minigame();
void travel();
int convertAccelToMovement();
void travelStatus();
void minigameStatus();

void setup()
{
  delay(5000);
  Serial.begin(9600);
  // SSD1306_SWITCHCAPVCC = generate _display voltage from 3.3V internally
  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3D))
  { // Address 0x3D for 128x64
    Serial.println("SSD1306 allocation failed");
    for (;;)
      ; // Don't proceed, loop forever
  }

  // Init accel
  if (!lis.begin(0x18))
  { // change this to 0x19 for alternative i2c address
    Serial.println("Couldnt start");
    while (1)
      yield();
  }
}

void loop()
{
  // put your main code here, to run repeatedly:
  display.clearDisplay();

  if (state == MINIGAME)
  {
    Serial.println("Minigame state");
    minigameStatus();
    minigame();
  }
  else if (state == TRAVEL)
  {
    travelStatus();
    travel();
  }
  else
  {
    Serial.println("Non minigame state");
  }

  display.display();
  delay(DELAY_LOOP_MS);
}

void travel()
{
  display.setTextSize(1);
  display.setTextColor(WHITE, BLACK);
  display.setCursor(0, 0);
  int randomEvent = random(4000);
  if (randomEvent < 10)
  {
    display.print(strAsteroidBelt);
    display.display();
    delay(4000);
    state = MINIGAME;
  }
  else
  {
    lyTraveled += 20;
    Serial.println(lyTraveled);
    if (lyTraveled >= LY_TO_COLONIA)
    {
      display.print(strVictory);
    }
    else
    {
      display.print(strTravel);
      display.print(strLy1);
      display.print(lyTraveled);
      display.print(strLy2);
    }
  }
}

void minigame()
{
  if(points > 15) { // TODO end minigame logic
    state = TRAVEL;
    credits = points;
    points = 0;
  }
  int movement = convertAccelToMovement();
  ship.setY(ship.getY() + movement);
  ship.forceInside(0, 0, display.width(), display.height());
  ship.draw(display);

  int maxRight = 0;
  // For each asteroid currently active, we draw it and check whether it's passed the player
  for (int i = 0; i < NUM_ASTEROIDS; i++)
  {
    if (asteroids[i].getDisplay())
    {
      asteroids[i].setX(asteroids[i].getX() - ASTEROID_SPEED);
      asteroids[i].draw(display);
      Serial.println(asteroids[i].toString());
      if (asteroids[i].getRight() < ship.getLeft() && asteroids[i].getHasPassed() == false)
      {
        points++;
        asteroids[i].setHasPassed(true);
      }
      if (asteroids[i].getRight() < 0)
      {
        asteroids[i].setDisplay(false);
        asteroidsOnScreen--;
      }
    }
  }

  // Spawn new asteroids!
  int randNumber = random(1000);
  int numAsteroids = 0;
  if (randNumber > 950 && asteroidsOnScreen < NUM_ASTEROIDS)
  {
    numAsteroids = random(1, 4);
  }
  for (int i = 0; i < NUM_ASTEROIDS && asteroidsOnScreen < NUM_ASTEROIDS && numAsteroids > 0; i++)
  {
    if (asteroids[i].getDisplay() == false)
    {
      int randY = random(SCREEN_HEIGHT - 1);
      int radius = random(5, 7);
      asteroids[i].setX(SCREEN_WIDTH);
      asteroids[i].setY(randY);
      asteroids[i].setRadius(radius);
      asteroids[i].setDisplay(true);
      asteroids[i].setHasPassed(false);
      asteroidsOnScreen++;
      numAsteroids--;
    }
  }
}

void travelStatus()
{
  display.setTextSize(1);
  display.setTextColor(WHITE, BLACK);
  display.setCursor(0, SCREEN_HEIGHT - 16);
  display.print(strProgress);
  display.setCursor(0, SCREEN_HEIGHT - 8);
  display.print(strHealth);
  display.print(shipHealth);
  display.print('%');
  display.setCursor(SCREEN_WIDTH / 2, SCREEN_HEIGHT - 8);
  display.print(strCr);
  display.print(credits);
  display.drawLine(0, SCREEN_HEIGHT - 17, SCREEN_WIDTH, SCREEN_HEIGHT - 17, WHITE);
  display.drawRect(60, SCREEN_HEIGHT - 16, SCREEN_WIDTH - 70, 8, WHITE);
  int barWidth = map(lyTraveled, 0, LY_TO_COLONIA, 0, SCREEN_WIDTH - 70);
  display.fillRect(60, SCREEN_HEIGHT - 16, barWidth, 8, WHITE);
}

void minigameStatus()
{
  display.setTextSize(1);
  display.setTextColor(WHITE, BLACK);
  display.setCursor(0, SCREEN_HEIGHT - 8);
  display.print(strPoints);
  display.print(points);
  display.drawLine(0, SCREEN_HEIGHT - 9, SCREEN_WIDTH, SCREEN_HEIGHT - 9, WHITE);
}
int convertAccelToMovement()
{
  lis.read();
  Serial.print("Raw accel: ");
  Serial.println(lis.y);
  int rawVal = lis.y;
  int movement = map(rawVal, -7000, 7000, -4, 4); // TODO: I have no idea what the input range is
  Serial.print("Mapped accel: ");
  Serial.println(movement);
  return -movement;
}