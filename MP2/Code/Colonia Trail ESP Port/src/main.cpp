#include <Arduino.h>
#include <Wire.h>
#include <SPI.h>
#include <Adafruit_LIS3DH.h>
#include <Adafruit_Sensor.h>
#include <Shape.hpp>
#include <Tone32.hpp>

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

const int VIBROMOTOR_OUTPUT = 16;
const int PIEZO_OUTPUT = 19;
const int LEFT_BUTTON_INPUT = 21;
const int RIGHT_BUTTON_INPUT = 17;
const int TONE_PWM_CHANNEL = 0;
const int VIBRO_PWM_CHANNEL = 1;

// PWM constants for vibromotor
const int VIBRO_FREQ = 500;
const int VIBRO_RESOLUTION = 8;                                 // Number of bits, 0-255
const int MAX_DUTY_CYCLE = (int)(pow(2, VIBRO_RESOLUTION) - 1); // Maximum duty cycle

// Tone usage copied heavily from https://makeabilitylab.github.io/physcomp/esp32/tone.html#introducing-the-tone32hpp-class
Tone32 piezoTone32(PIEZO_OUTPUT, TONE_PWM_CHANNEL);
Tone32 vibroTone32(VIBROMOTOR_OUTPUT, VIBRO_PWM_CHANNEL);
const int NUM_NOTES_IN_SCALE = 8;
const note_t C_SCALE[NUM_NOTES_IN_SCALE] = {NOTE_C, NOTE_D, NOTE_E, NOTE_F, NOTE_G, NOTE_A, NOTE_B, NOTE_C};
const int C_SCALE_OCTAVES[NUM_NOTES_IN_SCALE] = {4, 4, 4, 4, 4, 4, 4, 5};
const char C_SCALE_CHARS[NUM_NOTES_IN_SCALE] = {'C', 'D', 'E', 'F', 'G', 'A', 'B', 'C'};
note_t lastNote = NOTE_C;

// for tracking fps
unsigned long frameCount = 0;
float fps = 0;
unsigned long fpsStartTimeStamp = 0;

int16_t x, y;
uint16_t textWidth, textHeight;
const char strTravel[] = "Travelling to Colonia";
const char strVictory[] = "Welcome to Colonia!";
const char strProgress[] = "Progress:";
const char strCr[] = "Cr:";
const char strHealth[] = "Hull:";
const char strPoints[] = "Points:";
const char strLy1[] = "You've travelled ";
const char strLy2[] = " lightyears.";
const char strAsteroidBelt[] = "You've hit an asteroid belt! Avoid 15 asteroids to break free.";

const char strTitle[] = "Colonia Trail";

const char strWelcome1[] = "The year is 3307. Colonia, a star system 22,000.46 lightyears from Sol, is experiencing a tritium mining boom.";
const char strWelcome2[] = "You are an independent commander looking to make a fortune in Colonia, but first, you have to get there.";

const char strChooseLoadout[] = "Choose ship with L/R:";

// Ship stats
// Krait Phantom - Manueverable, travels fast, weak hull, expensive
const int kraitSpeed = 5;
const int kraitHull = 100;
const int kraitCredits = 500;

// Type-9 Heavy - Slow, durable, cheap
const int type9Speed = 2;
const int type9Hull = 200;
const int type9Credits = 1000;

class Ship : public Rectangle
{
protected:
  bool isKrait;

public:
  Ship(int x, int y, int width, int height) : Rectangle(x, y, width, height)
  {
    isKrait = false;
  }

  bool getIsKrait()
  {
    return isKrait;
  }

  void setKrait(bool val)
  {
    isKrait = val;
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
int shipSpeed = 0;

Asteroid asteroids[NUM_ASTEROIDS] = {Asteroid(0, 0, 0, false), Asteroid(0, 0, 0, false), Asteroid(0, 0, 0, false), Asteroid(0, 0, 0, false),
                                     Asteroid(0, 0, 0, false), Asteroid(0, 0, 0, false), Asteroid(0, 0, 0, false), Asteroid(0, 0, 0, false),
                                     Asteroid(0, 0, 0, false), Asteroid(0, 0, 0, false)};
int asteroidsOnScreen = 0;

enum GameState
{
  MENU,
  NEW_GAME,
  TRAVEL,
  MINIGAME,
  GAME_OVER
};

GameState state = NEW_GAME;
int points = 0;

void minigame();
void travel();
int convertAccelToMovement();
void travelStatus();
void minigameStatus();
void calcFrameRate();
void newGame();
void titleScreen();
void drawShip(int x, int y, bool isKrait);

void setup()
{
  delay(5000);
  //Serial.begin(9600);
  // SSD1306_SWITCHCAPVCC = generate _display voltage from 3.3V internally
  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3D))
  { // Address 0x3D for 128x64
    //Serial.println("SSD1306 allocation failed");
    for (;;)
      ; // Don't proceed, loop forever
  }
  display.setRotation(2);

  // Init accel
  if (!lis.begin(0x18))
  { // change this to 0x19 for alternative i2c address
    //Serial.println("Couldnt start");
    while (1)
      yield();
  }

  ledcSetup(VIBRO_PWM_CHANNEL, VIBRO_FREQ, VIBRO_RESOLUTION);
  ledcAttachPin(VIBROMOTOR_OUTPUT, VIBRO_PWM_CHANNEL);

  pinMode(LEFT_BUTTON_INPUT, INPUT_PULLUP);
  pinMode(RIGHT_BUTTON_INPUT, INPUT_PULLUP);
}

void loop()
{
  // put your main code here, to run repeatedly:
  display.clearDisplay();
  calcFrameRate();
  if (state == MINIGAME)
  {
    //Serial.println("Minigame state");
    minigameStatus();
    minigame();
  }
  else if (state == TRAVEL)
  {
    travelStatus();
    travel();
  }
  else if (state == NEW_GAME)
  {
    newGame();
  }
  else
  {
    //Serial.println("Non minigame state");
  }

  display.display();
  piezoTone32.update();
  vibroTone32.update();
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
    //Serial.println(lyTraveled);
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
  if (points > 15)
  { // TODO end minigame logic
    state = TRAVEL;
    credits = points;
    points = 0;
  }
  int movement = convertAccelToMovement();
  ship.setY(ship.getY() + movement);
  ship.forceInside(0, 0, display.width(), display.height() - 9);
  drawShip(ship.getX(), ship.getY(), ship.getIsKrait());
  //ship.draw(display);

  // For each asteroid currently active, we draw it and check whether it's passed the player
  for (int i = 0; i < NUM_ASTEROIDS; i++)
  {
    if (asteroids[i].getDisplay())
    {
      asteroids[i].setX(asteroids[i].getX() - ASTEROID_SPEED);
      asteroids[i].draw(display);
      //Serial.println(asteroids[i].toString());
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

      // If we've collided with an asteroid, crash and deduct health!
      if (asteroids[i].overlaps(ship))
      {
        asteroids[i].setDisplay(false);
        asteroids[i].setHasPassed(true);
        asteroidsOnScreen--;
        shipHealth -= 10;
        vibroTone32.playNote(C_SCALE[0], C_SCALE_OCTAVES[0], 500);
      }
    }
  }

  if (asteroidsOnScreen < NUM_ASTEROIDS)
  {
    // Spawn new asteroids!
    int randNumber = random(1000);
    int numAsteroids = 0;
    if (randNumber > 950)
    {
      numAsteroids = random(1, 4);
    }
    for (int i = 0; i < NUM_ASTEROIDS && asteroidsOnScreen < NUM_ASTEROIDS && numAsteroids > 0; i++)
    {
      if (asteroids[i].getDisplay() == false)
      {
        int randY = random(SCREEN_HEIGHT - 10);
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
  display.print("  ");
  display.print(strHealth);
  display.print(shipHealth);
  display.print("%");
  display.drawLine(0, SCREEN_HEIGHT - 9, SCREEN_WIDTH, SCREEN_HEIGHT - 9, WHITE);
  display.setCursor(0, 0);
  display.print(fps);
}
int convertAccelToMovement()
{
  lis.read();
  //Serial.print("Raw accel: ");
  //Serial.println(lis.y);
  int rawVal = lis.y;
  int movement = map(rawVal, -7000, 7000, -1 * shipSpeed, shipSpeed); 
  //Serial.print("Mapped accel: ");
  //Serial.println(movement);
  return movement;
}

// Ripped (almost) directly from FlappyBird.ino
// https://github.com/makeabilitylab/arduino/blob/master/OLED/FlappyBird/FlappyBird.ino
void calcFrameRate()
{
  unsigned long elapsedTime = millis() - fpsStartTimeStamp;
  frameCount++;
  if (elapsedTime > 1000)
  {
    fps = frameCount / (elapsedTime / 1000.0);
    fpsStartTimeStamp = millis();
    frameCount = 0;
  }
}

void newGame()
{
  // Displays title with a lil tune
  titleScreen();

  // Display lore primer
  display.setTextSize(1);
  for (int i = SCREEN_HEIGHT; i >= 0; i--)
  {
    display.clearDisplay();
    display.setCursor(0, i);
    display.print(strWelcome1);
    display.display();
    delay(50);
  }
  delay(5000);
  for (int i = SCREEN_HEIGHT; i >= 0; i--)
  {
    display.clearDisplay();
    display.setCursor(0, i);
    display.print(strWelcome2);
    display.display();
    delay(50);
  }
  delay(5000);
  display.clearDisplay();
  // Choose loadout
  int16_t x, y;
  uint16_t textWidth, textHeight;
  display.getTextBounds(strChooseLoadout, 0, 0, &x, &y, &textWidth, &textHeight);
  display.setCursor(SCREEN_WIDTH / 2 - textWidth / 2, 0);
  display.print(strChooseLoadout);
  display.drawLine((SCREEN_WIDTH / 2) - 1, 10, (SCREEN_WIDTH / 2) - 1, SCREEN_HEIGHT, WHITE);

  display.setCursor(0, 10);
  display.print("The Krait");
  display.setCursor(0, 19);
  display.print("Speed:");
  display.print(kraitSpeed);
  display.setCursor(0, 28);
  display.print("Hull:");
  display.print(kraitHull);
  display.setCursor(0, 37);
  display.print("Creds:");
  display.print(kraitCredits);
  drawShip(SCREEN_WIDTH / 4, 50, true);

  display.setCursor((SCREEN_WIDTH / 2) + 1, 10);
  display.print("The Type-9");
  display.setCursor((SCREEN_WIDTH / 2) + 1, 19);
  display.print("Speed:");
  display.print(type9Speed);
  display.setCursor((SCREEN_WIDTH / 2) + 1, 28);
  display.print("Hull:");
  display.print(type9Hull);
  display.setCursor((SCREEN_WIDTH / 2) + 1, 37);
  display.print("Creds:");
  display.print(type9Credits);
  drawShip(3 * (SCREEN_WIDTH / 4), 50, false);

  display.display();
  bool selection = false;
  while (selection == false)
  {
    int left = digitalRead(LEFT_BUTTON_INPUT);
    int right = digitalRead(RIGHT_BUTTON_INPUT);
    if(left == LOW) {
      selection = true;
      ship.setKrait(true);
      ship.setDimensions(5, 5);
      shipHealth = kraitHull;
      shipSpeed = kraitSpeed;
      credits = kraitCredits;
    } else if(right == LOW) {
      selection = true;
      ship.setKrait(false);
      ship.setDimensions(11, 9);
      shipHealth = type9Hull;
      shipSpeed = type9Speed;
      credits = type9Credits;
    }
  }

  state = TRAVEL;
}

void titleScreen()
{
  int16_t x, y;
  uint16_t textWidth, textHeight;

  int draw_width = 0; // How much of the title is revealed
  display.setTextSize(3);
  display.setTextColor(WHITE, BLACK);
  display.getTextBounds(strTitle, 0, 0, &x, &y, &textWidth, &textHeight);

  for (draw_width = 0; draw_width < SCREEN_WIDTH; draw_width++)
  {
    display.setCursor(SCREEN_WIDTH / 2 - textWidth / 2, SCREEN_HEIGHT / 2 - textHeight / 2);
    display.print(strTitle);
    display.fillRect(draw_width, 0, SCREEN_WIDTH - draw_width, SCREEN_HEIGHT, BLACK);
    delay(5);
    display.display();
  }

  piezoTone32.playNote(C_SCALE[4], C_SCALE_OCTAVES[4]);
  delay(500);
  piezoTone32.playNote(C_SCALE[0], C_SCALE_OCTAVES[0]);
  delay(500);
  piezoTone32.playNote(C_SCALE[2], C_SCALE_OCTAVES[2]);
  ledcWriteTone(VIBRO_PWM_CHANNEL, MAX_DUTY_CYCLE);
  delay(1000);
  piezoTone32.stopPlaying();
  ledcWriteTone(VIBRO_PWM_CHANNEL, 0);
  display.clearDisplay();
}

void drawShip(int x, int y, bool isKrait)
{
  if (isKrait)
  {
    display.fillTriangle(x, y, x, y + 4, x + 5, y + 2, WHITE);
  }
  else // Otherwise, it's the T9
  {
    display.fillRoundRect(x - 1, y, 11, 9, 2, WHITE);
  }
}
