#include <Arduino.h>
#include <Wire.h>
#include <SPI.h>
#include <Adafruit_LIS3DH.h>
#include <Adafruit_Sensor.h>
#include <Shape.hpp>
#include <Tone32.hpp>
#include <main.h>

// Accelerometer, i2c
Adafruit_LIS3DH lis = Adafruit_LIS3DH();

// Screen setup
#define SCREEN_WIDTH 128 // OLED _display width, in pixels
#define SCREEN_HEIGHT 64 // OLED _display height, in pixels

// Declaration for an SSD1306 _display connected to I2C (SDA, SCL pins)
#define OLED_RESET 4 // Reset pin # (or -1 if sharing Arduino reset pin)
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

// Game constants
#define DELAY_LOOP_MS 30
#define NUM_ASTEROIDS 10
#define ASTEROID_SPEED 3
#define LY_TO_COLONIA 22000
#define FUEL_SCALE 100

// Pinouts and PWM channels
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

int16_t x, y;
uint16_t textWidth, textHeight;

// Lots of text
// Consts probably arent needed now with ESP levels of RAM, and make it a little annoying to read the code, but it's easier to leave these than move them
const char strTitle[] = "Colonia Trail";
const char strWelcome1[] = "The year is 3307. Colonia, a star system  22,000 lightyears from Sol, is experiencing a tritium mining boom.";
const char strWelcome2[] = "You are an independent Commander looking to make your fortune in Colonia. Unfortunately, the trip will not be easy.";
const char strWelcome3[] = "Your trip into the black will involve dangers unimaginable, as well as wonderous sights. Prepare carefully, and you might survive.";
const char strTravel[] = "Travelling to Colonia";
const char strVictory[] = "Welcome to Colonia!";
const char strProgress[] = "Ly:";
const char strCr[] = "Cr:";
const char strHealth[] = "Hull:";
const char strPoints[] = "Points:";
const char strLy1[] = "You've travelled ";
const char strLy2[] = " lightyears.";
const char strAsteroidBelt[] = "You've hit an asteroid belt! Avoid 25 asteroids to break free.";
const char strPassedBelt[] = "You've made it through the asteroid belt. You've been able to salvage ore to sell at the next waypoint.";
const char strSpatialAnomaly[] = "You've noticed an interesting sensor reading. Your computer indicates some type of anomaly.";

const char strUseLR[] = "Use L/R buttons to choose:";
const char strChooseLoadout[] = "Choose ship with L/R:";

// Ship stats
// Krait Phantom - Manueverable, travels fast, weak hull, expensive
const int kraitSpeed = 5;
const int kraitHull = 50;
const int kraitCredits = 500;

// Type-9 Heavy - Slow, durable, cheap
const int type9Speed = 2;
const int type9Hull = 100;
const int type9Credits = 1000;

// Used for asteroid game hitbox
Ship ship(5, SCREEN_HEIGHT / 2, 5, 5);

// Misc stats, cr/health/speed are set in loadout selection
int lyTraveled = 0;
int credits = 0;
int shipHealth = 100;
int shipSpeed = 0;
int fuel = 0;
int nextWaypoint = 0;

// Travel screen data
int shipX = 0;
int shipY = (SCREEN_HEIGHT - 18) / 2;
int shipYDelta = 1;
const int maxYDelta = 10;

// Stores asteroid positions for the minigame
Asteroid asteroids[NUM_ASTEROIDS] = {Asteroid(0, 0, 0, false), Asteroid(0, 0, 0, false), Asteroid(0, 0, 0, false), Asteroid(0, 0, 0, false),
                                     Asteroid(0, 0, 0, false), Asteroid(0, 0, 0, false), Asteroid(0, 0, 0, false), Asteroid(0, 0, 0, false),
                                     Asteroid(0, 0, 0, false), Asteroid(0, 0, 0, false)};
int asteroidsOnScreen = 0;

// Data used to calculate waypoints
const char *waypointNames[5] = {"Amundsen Terminal", "Eagle Sector Secure Facility", "Eudaemon Anchorage", "Gagarin Gate", "Polo Harbour"};
int waypointDistances[5] = {4481, 7006, 7692, 14317, 17589};
bool waypointPassed[5] = {false, false, false, false, false};

// Used for the hyperspace effect on the travel screen
int hyperspaceLineX[20] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
int hyperspaceLineY[20] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
int hyperspaceLineLength[20] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};

enum GameState
{
  MENU,
  NEW_GAME,
  TRAVEL,
  MINIGAME,
  SPATIAL_ANOMALY,
  REFUEL,
  VICTORY,
  GAME_OVER
};

GameState state = MENU;

// Points tracking is only for minigame state - End game points calculations are on credits totals
int points = 0;

void setup()
{
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
    while (1)
      yield();
  }

  ledcSetup(VIBRO_PWM_CHANNEL, VIBRO_FREQ, VIBRO_RESOLUTION);
  ledcSetup(TONE_PWM_CHANNEL, VIBRO_FREQ, VIBRO_RESOLUTION);
  ledcAttachPin(VIBROMOTOR_OUTPUT, VIBRO_PWM_CHANNEL);

  pinMode(LEFT_BUTTON_INPUT, INPUT_PULLUP);
  pinMode(RIGHT_BUTTON_INPUT, INPUT_PULLUP);

  // Displays title with a lil tune
  titleScreen();
}

void loop()
{
  display.clearDisplay();
  if (state == MINIGAME)
  {
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
  else if (state == SPATIAL_ANOMALY)
  {
    spatialAnomaly();
  }
  else if (state == GAME_OVER)
  {
    gameOver();
  }
  else if (state == MENU)
  {
    menu();
  }
  else if (state == REFUEL)
  {
    refuel();
  }
  else if (state == VICTORY)
  {
    victory();
  }

  display.display();
  piezoTone32.update();
  vibroTone32.update();
  delay(DELAY_LOOP_MS);
}

// Travelling between waypoints
void travel()
{
  if (shipHealth <= 0)
  {
    state = GAME_OVER;
    return;
  }
  if (fuel <= 0)
  {
    state = REFUEL;
  }
  for (int i = nextWaypoint; i < 5; i++)
  {
    Serial.println(i);
    if (lyTraveled > waypointDistances[i] && waypointPassed[i] != true)
    {
      waypoint(i);
      return;
    }
  }

  display.setTextSize(1);
  display.setTextColor(WHITE, BLACK);
  display.setCursor(0, 0);

  // Roll the dice for a random event
  int randomEvent = random(4000);
  if (randomEvent < 10)
  {
    eventText(strAsteroidBelt);
    awaitInput(true);
    state = MINIGAME;
  }
  else if (randomEvent < 30)
  {
    eventText(strSpatialAnomaly);
    awaitInput(true);
    state = SPATIAL_ANOMALY;
  }
  else
  { // draw travel screen and travel
    lyTraveled += shipSpeed * 2;
    fuel -= shipSpeed * 2;

    if (lyTraveled >= LY_TO_COLONIA)
    {
      state = VICTORY;
    }
    else
    {
      for (int i = 0; i < 20; i++)
      {
        if (hyperspaceLineLength[i] == 0 || hyperspaceLineX[i] < 0)
        {
          hyperspaceLineX[i] = random(SCREEN_WIDTH - 50, SCREEN_WIDTH);
          hyperspaceLineY[i] = random(0, SCREEN_HEIGHT - 18);
          hyperspaceLineLength[i] = random(5, 20);
        }
        hyperspaceLineX[i] -= 10;
        display.drawLine(hyperspaceLineX[i], hyperspaceLineY[i], hyperspaceLineX[i] + hyperspaceLineLength[i], hyperspaceLineY[i], WHITE);
      }

      if (nextWaypoint == 0)
      {
        shipX = map(lyTraveled, 0, waypointDistances[0], 0, SCREEN_WIDTH);
      }
      else
      {
        shipX = map(lyTraveled, waypointDistances[nextWaypoint - 1], waypointDistances[nextWaypoint], 0, SCREEN_WIDTH);
      }
      shipY = shipY + shipYDelta;
      if (shipY > ((SCREEN_HEIGHT - 18) / 2 + maxYDelta) || shipY < ((SCREEN_HEIGHT - 18) / 2 - maxYDelta))
      {
        shipYDelta = -shipYDelta;
      }

      drawShip(shipX, shipY, ship.getIsKrait());

      // Display "next stop"
      int16_t x, y;
      uint16_t textWidth, textHeight;
      char displayText[128];
      strcpy(displayText, "Next waypoint: ");
      strcat(displayText, waypointNames[nextWaypoint]);
      display.getTextBounds(displayText, 0, 0, &x, &y, &textWidth, &textHeight);
      display.setCursor(SCREEN_WIDTH / 2 - textWidth / 2, 0);
      display.print(displayText);
    }
  }
}

// Docked at a station, refuel and repair
void waypoint(int index)
{
  nextWaypoint = index + 1;
  char displayText[512];
  strcpy(displayText, "You've reached ");
  strcat(displayText, waypointNames[index]);
  strcat(displayText, ", a station on the Colonia Connection Highway. It offers refuge for explorers going to Colonia.");
  eventText(displayText);
  awaitInput(true);
  eventText("You can spend credits to refuel or repair your ship, but fewer credits when you arrive in Colonia will make your life much harder.");
  awaitInput(true);

  // Generate status menu
  statusAlert();
  awaitInput(true);

  bool docked = true;
  while (docked)
  {
    choiceText("Explore the station, or undock and continue?", "leave the", "station", "buy fuel", "and supplies");
    bool choice = awaitInput(true);
    if (choice)
    { // staying in the station
      eventText("You wander around the promenade and are able to secure a good deal on supplies considering how far away you are from Sol.");
      awaitInput(true);
      int maxHull;
      if (ship.getIsKrait())
      {
        maxHull = kraitHull;
      }
      else
      {
        maxHull = type9Hull;
      }
      int fuelCostPerTon = 10 + (index * 10);
      int hullCostPerPoint = 100 + (index * 10);
      int hullToFullRepair = maxHull - shipHealth;
      int maximumHullPurchasable = min(hullToFullRepair, credits / hullCostPerPoint);
      int maximumFuelPurchasable = min(5, credits / fuelCostPerTon);
      char fuelAmt[16];
      char fuelCost[16];
      char hullAmt[16];
      char hullCost[16];
      sprintf(fuelAmt, "%dt fuel", maximumFuelPurchasable);
      sprintf(fuelCost, "for %dcr", maximumFuelPurchasable * fuelCostPerTon);
      sprintf(hullAmt, "%d hull", maximumHullPurchasable);
      sprintf(hullCost, "for %dcr", maximumHullPurchasable * hullCostPerPoint);

      choiceText("Buy fuel or repairs?", fuelAmt, fuelCost, hullAmt, hullCost);
      bool choice = awaitInput(true);
      if (choice)
      { //hull
        shipHealth += maximumHullPurchasable;
        credits -= maximumHullPurchasable * hullCostPerPoint;
      }
      else
      { // fuel
        fuel += maximumFuelPurchasable * FUEL_SCALE;
        credits -= maximumFuelPurchasable * fuelCostPerTon;
      }
      statusAlert();
      awaitInput(true);
    }
    else
    { // Leaving
      docked = false;
      eventText("You depart the station and head out into the black. Good luck, Commander.");
      waypointPassed[index] = true;
      awaitInput(true);
    }
  }
}

// Renders the current state
void statusAlert()
{
  char displayText[512];
  strcpy(displayText, "You currently have ");
  char valBuffer[32];
  sprintf(valBuffer, "%d", fuel / FUEL_SCALE);
  strcat(displayText, valBuffer);
  strcat(displayText, " tons of fuel, ");
  sprintf(valBuffer, "%d", shipHealth);
  strcat(displayText, valBuffer);
  strcat(displayText, " out of ");
  int maxHull;
  if (ship.getIsKrait())
  {
    maxHull = kraitHull;
  }
  else
  {
    maxHull = type9Hull;
  }

  sprintf(valBuffer, "%d", maxHull);
  strcat(displayText, valBuffer);
  strcat(displayText, " maximum hull points, and ");
  sprintf(valBuffer, "%d", credits);
  strcat(displayText, valBuffer);
  strcat(displayText, " credits to your name.");
  eventText(displayText);
}

// Asteroid dodging minigame
void minigame()
{
  if (points > 25) // End the game when passed 15 asteroids
  {
    state = TRAVEL;
    credits += random(500);
    points = 0;

    eventText(strPassedBelt);

    piezoTone32.playNote(C_SCALE[4], C_SCALE_OCTAVES[4]);
    delay(500);
    piezoTone32.playNote(C_SCALE[0], C_SCALE_OCTAVES[0]);
    delay(500);
    piezoTone32.playNote(C_SCALE[2], C_SCALE_OCTAVES[2]);
    ledcWriteTone(VIBRO_PWM_CHANNEL, MAX_DUTY_CYCLE);
    delay(1000);
    piezoTone32.stopPlaying();
    ledcWriteTone(VIBRO_PWM_CHANNEL, 0);

    awaitInput(true);
  }

  if (shipHealth <= 0)
  {
    state = GAME_OVER;
    return;
  }

  int movement = convertAccelToMovement();
  ship.setY(ship.getY() + movement);
  ship.forceInside(0, 0, display.width(), display.height() - 9);
  // the ship object is just a hitbox and not actually drawn, we use drawShip() to draw depending on the ship loadout
  drawShip(ship.getX(), ship.getY(), ship.getIsKrait());

  // For each asteroid currently active, we draw it and check whether it's passed the player
  for (int i = 0; i < NUM_ASTEROIDS; i++)
  {
    if (asteroids[i].getDisplay())
    {
      asteroids[i].setX(asteroids[i].getX() - ASTEROID_SPEED);
      asteroids[i].draw(display);
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

// Overlay a status bar on the bottom of the screen during travel
// Lots of ugly magic numbers
// SCREEN_HEIGHT - 17 = where to draw the line, enough room for two lines of text
void travelStatus()
{
  display.setTextSize(1);
  display.setTextColor(WHITE, BLACK);
  display.setCursor(0, SCREEN_HEIGHT - 16);
  display.print(strProgress);
  display.setCursor(65, display.getCursorY());
  display.print("Fuel:");
  display.print(fuel / FUEL_SCALE);

  display.setCursor(0, SCREEN_HEIGHT - 8);
  display.print(strHealth);
  display.print(shipHealth);
  display.print('%');
  display.setCursor(SCREEN_WIDTH / 2, SCREEN_HEIGHT - 8);
  display.print(strCr);
  display.print(credits);
  display.drawLine(0, SCREEN_HEIGHT - 17, SCREEN_WIDTH, SCREEN_HEIGHT - 17, WHITE);
  display.drawRect(20, SCREEN_HEIGHT - 16, 40, 8, WHITE);
  int barWidth = map(lyTraveled, 0, LY_TO_COLONIA, 0, 40);
  display.fillRect(20, SCREEN_HEIGHT - 16, barWidth, 8, WHITE);
}

// Draw a status bar on the asteroid game screen
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
}

// Converts raw accelerometer data to delta-Y depending on ship speed
int convertAccelToMovement()
{
  lis.read();
  int rawVal = lis.y;
  int movement = map(rawVal, -7000, 7000, -1 * shipSpeed, shipSpeed);
  return movement;
}

// Begins a new game, including ship selection
void newGame()
{

  // Display lore primer
  // I shouldve just put the strings in an array oops
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
  for (int i = SCREEN_HEIGHT; i >= 0; i--)
  {
    display.clearDisplay();
    display.setCursor(0, i);
    display.print(strWelcome3);
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
  bool selection = awaitInput(true);
  if (selection == false) // Krait
  {
    ship.setKrait(true);
    ship.setDimensions(5, 5);
    shipHealth = kraitHull;
    shipSpeed = kraitSpeed;
    credits = kraitCredits;
    fuel = 4000;
  }
  else // Type-9 Heavy
  {
    ship.setKrait(false);
    ship.setDimensions(11, 9);
    shipHealth = type9Hull;
    shipSpeed = type9Speed;
    credits = type9Credits;
    fuel = 3000;
  }

  state = TRAVEL;
}

// "Colonia Trail" title display
void titleScreen()
{
  int16_t x, y;
  uint16_t textWidth, textHeight;

  int draw_width = 0; // Determines how much of the title is revealed
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

// Draws the appropriate ship at given coords
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

// Triggers when out of fuel
void refuel()
{
  eventText("You've run out of fuel and are running on fumes! You don't have enough to jump out of this system.");
  awaitInput(true);
  // Roll whether there is a scoopable star
  int roll = random(21);
  if (roll < 10)
  { // Fail
    eventText("Unfortunately, there is no fuel-scoopable star in this system. Your only option is to signal for help and hope a passing traveler saves you.");
    awaitInput(true);
    eventText("You could offer a reward to anyone who could offer you fuel, but that increases the chance that a pirate might engage.");
    awaitInput(true);
    choiceText("Offer a reward?", "not offer", "a reward", "promise", "1000 cr");
    bool choice = awaitInput(true);
    if (choice)
    { // Reward offered for refuel
      roll = random(21);
      eventText("Just as your life support is giving out, your sensors read a new signal approaching you.");
      awaitInput(true);
      if (roll < 5)
      { // Success
        eventText("Your comms panel lights up and a voice cuts through the static - 'Howdy, CMDR! Looks like you're in a pinch. Let me help you out.'");
        awaitInput(true);
        eventText("The sound of fuel limpet drones attaching to your ship is soon followed by your fuel gauge filling. Gain 10t fuel, lose 1000cr.");
        awaitInput(true);
        fuel += 10 * FUEL_SCALE;
        credits -= 1000;
        credits = max(0, credits);
      }
      else
      { // Pirate
        eventText("Your comm panel lights up as a Python deploys it's weapons in front of you - 'Looks like you're in a tough spot, CMDR. I think my fee just went up.'");
        awaitInput(true);
        if (credits < 2000)
        {
          eventText("'Pah, worthless, you barely have any credits to your name. This is doing you a favor.' Suddenly, you see a flash of light.");
          awaitInput(true);
          eventText("It's a pirate! Before you know it, your cockpit has cracked and is venting air. You barely have time to think before the hypoxia sets in.");
          awaitInput(true);
          state = GAME_OVER;
        }
        else
        {
          eventText("'Transfer 2000 credits to me and we'll call it a deal. The way I see it, you don't have much choice.'");
          awaitInput(true);
          choiceText("Pay the fee?", "not pay", "the pirate", "pay 2000", "credits");
          choice = awaitInput(true);
          if (choice)
          { // Paid
            eventText("You transfer the credits and the pirate reluctantly sends over a fuel limpet. Thank Raxxla there's still honor in the black.");
            awaitInput(true);
            eventText("You lost 2000 credits, but gained 15 tons of fuel.");
            awaitInput(true);
            fuel += 15 * FUEL_SCALE;
            credits -= 2000;
            state = TRAVEL;
          }
          else
          { // Not paid
            eventText("'Fine. I'll just cut you up for scrap metal.'");
            awaitInput(true);
            eventText("This far out into the black, no one can hear you scream.");
            awaitInput(true);
            state = GAME_OVER;
          }
        }
      }
    }
    else
    { // No reward offered
      int roll = random(21);
      if (roll > 10)
      { // Someone answers your call
        eventText("Your comms panel lights up and a voice cuts through the static - 'Howdy, CMDR! Looks like you're in a pinch. Let me help you out.'");
        awaitInput(true);
        eventText("The sound of fuel limpet drones attaching to your ship is soon followed by your fuel gauge filling. Gain 10t fuel");
        awaitInput(true);
        fuel += 10 * FUEL_SCALE;
        state = TRAVEL;
      }
      else
      { // A cold death
        eventText("You wait. It's getting cold now. Your primary life support's gone out, you only have minutes of emergency oxygen left.");
        awaitInput(true);
        eventText("You remember learning about hypoxia while studying to become a Commander. Your last thoughts are of the academy as your conciousness slips away.");
        awaitInput(true);
        state = GAME_OVER;
      }
    }
  }
  else
  { // Refuel
    eventText("Thankfully, there's a scoopable star in this system. You engage your fuel scoop and approach the corona with caution.");
    awaitInput(true);
    eventText("Gain 40t fuel, but lose 10 hull from overheating.");
    fuel += 40 * FUEL_SCALE;
    shipHealth -= 10;
    state = TRAVEL;
  }
}

// Sensor reading event
void spatialAnomaly()
{
  int type = random(21);
  enum Type
  {
    BLACK_HOLE,
    NEUTRON,
    EARTHLIKE,
    THARGOID
  };
  Type encounter; // Type of event
  char encounterText[32];
  if (type < 5)
  { // Black hole
    encounter = BLACK_HOLE;
    strcpy(encounterText, "a Black Hole");
  }
  else if (type < 15)
  { // Neutron star
    encounter = NEUTRON;
    strcpy(encounterText, "a Neutron Star");
  }
  else if (type < 18)
  { // Earthlike world
    encounter = EARTHLIKE;
    strcpy(encounterText, "an Earthlike World");
  }
  else
  { // Thargoid homeworld
    encounter = THARGOID;
    strcpy(encounterText, "an Anomalous Ammonia Planet");
  }

  char displayText[512];
  strcpy(displayText, "Sensor readings indicate ");
  strcat(displayText, encounterText);
  strcat(displayText, ". Exploring may put you in danger or waste fuel, but you may find a wonder of the galaxy.");
  eventText(displayText);
  awaitInput(true);

  choiceText(strUseLR, "explore &", "use 2 fuel", "continue", "onwards");
  bool choice = awaitInput(true);
  if (choice)
  { // Continue onward
    eventText("You continue onward. A nagging regret in the back of your head has begun to set in already.");
    state = TRAVEL;
  }
  else
  { // Explore!!
    fuel -= 2 * FUEL_SCALE;
    int eventRoll = random(20);
    state = TRAVEL;
    switch (encounter)
    {
    case BLACK_HOLE:
      if (eventRoll > 5)
      { // Success
        eventText("You've looked into the void, and the void looks back. You are the first to discover this long-dead stellar remnant. You feel uneasy.");
        awaitInput(true);
        eventText("You'll be able to sell your gravitational readings and starcharts for a hefty sum. You've gained 1000 credits.");
        awaitInput(true);
        credits += 1000;
      }
      else
      { // Fail
        eventText("While running your scanners, you fell too close! You've taken massive hull damage as the gravitational forces pull at your ship.");
        ledcWriteTone(VIBRO_PWM_CHANNEL, MAX_DUTY_CYCLE);
        awaitInput(true);
        ledcWriteTone(VIBRO_PWM_CHANNEL, 0);
        eventText("You escape, but just barely. You've lost 25 hull points, but gained a story of valor in the face of the void.");
        awaitInput(true);
        shipHealth -= 25;
      }
      break;
    case NEUTRON:
      if (eventRoll > 5)
      { // Success
        eventText("This neutron star is promising! You can use the neutron-rich exotic matter to supercharge your jump drive.");
        awaitInput(true);
        eventText("You jumped an additional 2000 light years closer to Colonia!");
        awaitInput(true);
        lyTraveled += 2000;
      }
      else
      { //Failure
        eventText("You think you can use the neutron-rich exotic matter around this star to boost your drive, but you fly too close.");
        awaitInput(true);
        eventText("Your ship overheats, damaging your hull and jump drive. Lose 10 hull points.");
        awaitInput(true);
        shipHealth -= 10;
      }
      break;
    case EARTHLIKE:
      if (eventRoll > 2)
      { // Success
        eventText("You're the first to discover this beautiful, terraformable, blue gem of a world. It reminds you a little of home.");
        awaitInput(true);
        eventText("Maybe one day, settlers will come here as they did to Colonia. This cartography data will be worth 500 credits.");
        awaitInput(true);
        credits += 500;
      }
      else
      { // Failure
        eventText("It turns out your sensor was acting up. This is just another unterraformable rock.");
        awaitInput(true);
        eventText("You spent fuel for nothing and lost 50 credits, since you'll need to repair your sensor when you next land.");
        awaitInput(true);
        credits -= 50;
      }
      break;
    case THARGOID:
      if (eventRoll > 10)
      { // Success
        eventText("Ammonia based worlds are candidates for Thargoid alien life, so this data is valuable.");
        awaitInput(true);
        eventText("Wisely, you don't stick around long enough to see if there's anyone home. Gain 1000 credits.");
        awaitInput(true);
        credits += 1000;
      }
      else
      { // Failure
        eventText("Ammonia based worlds might contain Thargoid alien life. This planet is showing anomalous surface structures.");
        awaitInput(true);
        eventText("Despite the hair on the back of your neck standing up, you attempt a landing in the name of exploration.");
        awaitInput(true);
        thargoidEncounter();
      }
      break;
    }
  }
}
void thargoidEncounter()
{
  eventText("You pinpoint the anomaly and carefully guide your ship down through the ammonia clouds.");
  awaitInput(true);
  eventText("The landing is rough. You lose 5 hull.");
  awaitInput(true);
  shipHealth -= 5;
  eventText("You step out, clad in your custom Supratech Artemis EVA suit. You won't have long before life support fails.");
  awaitInput(true);
  eventText("You approach an unsettling organic formation of tendril-like arms arranged as a giant, grotesque flower.");
  awaitInput(true);
  choiceText("Do you power through the fear?", "turn back", "to my ship", "continue", "closer");
  bool choice = awaitInput(false);
  if (choice)
  { // Continue

    eventText("You find what seems to be an entrance into a dimly lit cavity. You cautiously descend into this structure.");
    awaitInput(false);
    vibroTone32.playNote(C_SCALE[2], C_SCALE_OCTAVES[2]);
    piezoTone32.playNote(C_SCALE[2], C_SCALE_OCTAVES[2]);
    eventText("You hear an unsettling rattling noise, soon followed by a scraping.");
    awaitInput(false);
    eventText("It's getting closer. You fumble for your sidearm, but this isn't a combat suit.");
    vibroTone32.playNote(C_SCALE[0], C_SCALE_OCTAVES[0]);
    piezoTone32.playNote(C_SCALE[0], C_SCALE_OCTAVES[0]);
    awaitInput(false);
    vibroTone32.stopPlaying();
    piezoTone32.stopPlaying();
    ledcWriteTone(VIBRO_PWM_CHANNEL, MAX_DUTY_CYCLE);
    ledcWriteTone(TONE_PWM_CHANNEL, MAX_DUTY_CYCLE);
    display.clearDisplay();
    display.display();
    delay(1000);
    ledcWriteTone(VIBRO_PWM_CHANNEL, 0);
    ledcWriteTone(TONE_PWM_CHANNEL, 0);
    eventText("Your last thoughts are of your family as an insectoid alien rips your suit open.");
    awaitInput(false);
    state = GAME_OVER;
  }
  else
  { // Run
    eventText("This feels like the safest choice. This data alone will be valuable. You gain 2000 credits.");
    awaitInput(true);
    credits += 2000;
    state = TRAVEL;
  }
}

// First game state, displays tutorial and lets you start new game
void menu()
{
  eventText("When you see a screenwith borders like    this, press the left or right button to proceed. Press any button now to start the game.");
  awaitInput(true);
  state = NEW_GAME;
}

// Not a very rewarding victory screen tbh
// Displays points as credits + 5000 bonus for winning
void victory()
{
  int16_t x, y;
  uint16_t textWidth, textHeight;

  display.setTextSize(2);
  display.getTextBounds("Welcome to Colonia!", 0, 0, &x, &y, &textWidth, &textHeight);
  display.setCursor(SCREEN_WIDTH / 2 - textWidth / 2, SCREEN_HEIGHT / 2 - textHeight / 2);
  display.print("Welcome to Colonia!");

  display.setTextSize(1);
  display.setCursor(0, display.getCursorY() + 16);
  display.print(credits + 5000);
  display.print("points");
  display.display();
  awaitInput(true);
  if (credits > 3000)
  {
    eventText("You made it to Colonia with ample credits. You're able to outfit a powerful mining ship and strike it rich before the Tritium runs dry.");
  }
  else
  {
    eventText("You made it to Colonia! You don't have many credits to your name, so it takes some time, but eventually, you make a living here in Colonia.");
  }
  awaitInput(true);
  titleScreen();
  state = MENU;
}

// Game over, display credits as points
void gameOver()
{
  int16_t x, y;
  uint16_t textWidth, textHeight;

  display.setTextSize(2);
  display.getTextBounds("Game Over!", 0, 0, &x, &y, &textWidth, &textHeight);
  display.setCursor(SCREEN_WIDTH / 2 - textWidth / 2, SCREEN_HEIGHT / 2 - textHeight / 2);
  display.print("Game over!");

  display.setTextSize(1);
  display.setCursor(0, display.getCursorY() + 16);
  display.print(credits);
  display.print("points");
  display.display();
  awaitInput(true);
  titleScreen();
  state = MENU;
}

// Pop-up text box, text should be at most 150 characters
void eventText(const char *text)
{
  display.clearDisplay();
  display.fillRect(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, WHITE);
  display.fillRect(0, 3, SCREEN_WIDTH, SCREEN_HEIGHT - 6, BLACK);
  int16_t x, y;
  uint16_t textWidth, textHeight;

  display.setTextSize(1);
  display.setTextColor(WHITE, BLACK);
  display.getTextBounds(text, 0, 0, &x, &y, &textWidth, &textHeight);
  display.setCursor(SCREEN_WIDTH / 2 - textWidth / 2, SCREEN_HEIGHT / 2 - textHeight / 2);
  display.print(text);
  display.display();
}

// Choice text template, each choice line should be at most 11 characters
void choiceText(const char *desc, const char *lChoice1, const char *lChoice2, const char *rChoice1, const char *rChoice2)
{
  display.clearDisplay();
  int16_t x, y;
  uint16_t textWidth, textHeight;
  // Draw the description text and the lines
  display.setTextSize(1);
  display.setTextColor(WHITE, BLACK);
  display.getTextBounds(desc, 0, 0, &x, &y, &textWidth, &textHeight);
  display.setCursor(SCREEN_WIDTH / 2 - textWidth / 2, 0);
  display.print(desc);
  display.drawLine(0, display.getCursorY() + 9, SCREEN_WIDTH, display.getCursorY() + 9, WHITE);
  display.drawLine(SCREEN_WIDTH / 2, display.getCursorY() + 9, SCREEN_WIDTH / 2, SCREEN_HEIGHT, WHITE);

  // Choices
  display.setCursor(0, display.getCursorY() + 11);
  display.print("I will");
  display.setCursor((SCREEN_WIDTH / 2) + 1, display.getCursorY());
  display.print("I will");

  display.setCursor(0, display.getCursorY() + 9);
  display.print(lChoice1);
  display.setCursor((SCREEN_WIDTH / 2) + 1, display.getCursorY());
  display.print(rChoice1);

  display.setCursor(0, display.getCursorY() + 9);
  display.print(lChoice2);
  display.setCursor((SCREEN_WIDTH / 2) + 1, display.getCursorY());
  display.print(rChoice2);

  // Button indicators

  display.fillRect(0, SCREEN_HEIGHT - 11, 11, 11, WHITE);
  display.fillCircle(5, SCREEN_HEIGHT - 6, 4, BLACK);
  display.setCursor(4, SCREEN_HEIGHT - 9);
  display.print("L");

  display.fillRect(SCREEN_WIDTH - 11, SCREEN_HEIGHT - 11, 11, 11, WHITE);
  display.fillCircle(SCREEN_WIDTH - 6, SCREEN_HEIGHT - 6, 4, BLACK);
  display.setCursor(SCREEN_WIDTH - 8, SCREEN_HEIGHT - 9);
  display.print("R");

  display.display();
}

// Returns true if the right button is pressed, returns false if the left button is pressed.
boolean awaitInput(bool sound)
{
  while (true)
  {
    int left = digitalRead(LEFT_BUTTON_INPUT);
    int right = digitalRead(RIGHT_BUTTON_INPUT);
    delay(40); // Debounce window
    int left2 = digitalRead(LEFT_BUTTON_INPUT);
    int right2 = digitalRead(RIGHT_BUTTON_INPUT);

    if (left == LOW && left2 == left)
    {
      if (sound)
      {
        piezoTone32.playNote(C_SCALE[8], C_SCALE_OCTAVES[8]);
        delay(200);
        piezoTone32.stopPlaying();
      }
      else
      {
        delay(500);
      }
      return false;
    }
    else if (right == LOW && right2 == right)
    {
      if (sound)
      {
        piezoTone32.playNote(C_SCALE[5], C_SCALE_OCTAVES[5]);
        delay(200);
        piezoTone32.stopPlaying();
      }
      else
      {
        delay(500);
      }
      return true;
    }
  }
}