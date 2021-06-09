#include <Arduino.h>
#include <Wire.h>
#include <SPI.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <Adafruit_I2CDevice.h>

void checkStateChange();
void checkButton();
void pressToContinue();
void pose();
void tomatoAlert();
void menu();
void endGame();
void drawLives();

#define SCREEN_WIDTH 128 // OLED display width, in pixels
#define SCREEN_HEIGHT 64 // OLED display height, in pixels

// Declaration for an SSD1306 display connected to I2C (SDA, SCL pins)
#define OLED_RESET 4 // Reset pin # (or -1 if sharing Arduino reset pin)
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

// 0 = no tomato, 1 = top, 2 = right, 3 = bottom, 4 = left
int tomatoDirection = 0;
// 1 = menu, 2 = minigame, 3 = endgame, 4 = pause
int state = 1;
// lives remaining
int livesRemaining = 3;

// Used to prevent double sends
bool lastButtonVal = false;

// Time the tomato alert started
long startTime = millis();
// how long the tomato alert lasts
const long TOMATO_DURATION = 1000;

int16_t x, y;
uint16_t textWidth, textHeight;

// Pinouts and PWM channels
const int VIBROMOTOR_OUTPUT = 26;
const int BUTTON_INPUT = 14;
const int VIBRO_PWM_CHANNEL = 0;

// PWM constants for vibromotor
const int VIBRO_FREQ = 500;
const int VIBRO_RESOLUTION = 8;                                 // Number of bits, 0-255
const int MAX_DUTY_CYCLE = (int)(pow(2, VIBRO_RESOLUTION) - 1); // Maximum duty cycle

const int DEBOUNCE_WINDOW = 40; //ms
const int DELAY = 50;           // ms
void setup()
{
  Serial.begin(115200); // set baud rate to 115200
  // SSD1306_SWITCHCAPVCC = generate display voltage from 3.3V internally
  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3D))
  { // Address 0x3D for 128x64
    Serial.println(F("SSD1306 allocation failed"));
    for (;;)
      ; // Don't proceed, loop forever
  }
  display.setRotation(0);
  display.clearDisplay();
  display.setTextSize(1);              // Normal 1:1 pixel scale
  display.setTextColor(SSD1306_WHITE); // Draw white text
  display.setCursor(0, 0);             // Start at top-left corner
  display.print("Waiting to receive\ndata from serial...");
  display.display();
  pinMode(BUTTON_INPUT, INPUT_PULLUP);
  ledcSetup(VIBRO_PWM_CHANNEL, VIBRO_FREQ, VIBRO_RESOLUTION);
  ledcAttachPin(VIBROMOTOR_OUTPUT, VIBRO_PWM_CHANNEL);
}

void loop()
{
  // put your main code here, to run repeatedly:
  checkStateChange();
  checkButton();

  display.clearDisplay();

  if (state == 1) // menuing!
  {
    menu();
  }
  else if (state == 2) // posing!
  {
    pose();
    drawLives();
  }
  else if (state == 3) // end game-ing!
  {
    endGame();
  }
  else if (state == 4) // pausing!
  {
    pressToContinue();
    drawLives();
  }

  // Quell any tomato alerts if the window is over
  if (millis() > startTime + TOMATO_DURATION)
  {
    ledcWriteTone(VIBRO_PWM_CHANNEL, 0);
  }

  display.display();
  delay(DELAY);
}

/*
 *  Borrowed some tokenizing snippets from https://github.com/makeabilitylab/arduino/blob/master/Serial/DisplayShapeSerialBidirectional/DisplayShapeSerialBidirectional.ino
 * 
 * data should be csv of ints
 * state,tomatoDirection,lives
 */
void checkStateChange()
{
  if (Serial.available() > 0)
  {
    String rcvdSerialData = Serial.readStringUntil('\n');

    int startIndex = 0;
    int endIndex = rcvdSerialData.indexOf(',');
    if (endIndex != -1)
    {
      String stateString = rcvdSerialData.substring(startIndex, endIndex);
      state = stateString.toInt();

      startIndex = endIndex + 1;
      endIndex = rcvdSerialData.indexOf(',', startIndex);
      String tomatoString = rcvdSerialData.substring(startIndex, endIndex);
      int newDirection = tomatoString.toInt();
      if (tomatoDirection == 0 && newDirection != 0)
      {
        tomatoAlert();
      }
      tomatoDirection = newDirection;

      startIndex = endIndex + 1;
      endIndex = rcvdSerialData.indexOf(',', startIndex);
      String livesString = rcvdSerialData.substring(startIndex, endIndex);
      livesRemaining = livesString.toInt();
    }
  }
}

void checkButton()

{
  int buttonPressed = digitalRead(BUTTON_INPUT);
  delay(DEBOUNCE_WINDOW);
  int buttonPressed2 = digitalRead(BUTTON_INPUT);
  // Send button press, only if its a new press
  if (buttonPressed == buttonPressed2)
  {
    if (buttonPressed == LOW && lastButtonVal == false)
    { // pressed
      // send data
      lastButtonVal = true;
      Serial.println("1");
    }
    else if (buttonPressed == HIGH && lastButtonVal == true)
    { // reset bool and send update
      lastButtonVal = false;
      Serial.println("0");
    }
  }
}

void pressToContinue()
{
  display.setTextSize(2);
  display.setTextColor(WHITE, BLACK);
  String pressToContinue = "Press button to begin!";
  display.getTextBounds(pressToContinue, 0, 0, &x, &y, &textWidth, &textHeight);
  display.setCursor(SCREEN_WIDTH / 2 - textWidth / 2, SCREEN_HEIGHT / 2 - textHeight / 2);
  display.print(pressToContinue);
}

void pose()
{

  if (tomatoDirection == 0)
  { // No tomato!
    display.setTextSize(2);
    display.setTextColor(WHITE, BLACK);
    String pressToContinue = "Strike a\n   pose!";
    display.getTextBounds(pressToContinue, 0, 0, &x, &y, &textWidth, &textHeight);
    display.setCursor(SCREEN_WIDTH / 2 - textWidth / 2, SCREEN_HEIGHT / 2 - textHeight / 2);
    display.print(pressToContinue);
  }
  else // incoming tomato!
  {
    String direction;
    if (tomatoDirection == 1) // top
    {
      direction = "ABOVE";
    }
    else if (tomatoDirection == 2) // right
    {
      direction = "RIGHT";
    }
    else if (tomatoDirection == 3) // bottom
    {
      direction = "BELOW";
    }
    else // left
    {
      direction = "LEFT";
    }
    String alert = "Tomato coming from " + direction + "!";
    display.setTextSize(3);
    display.setTextColor(WHITE, BLACK);
    display.getTextBounds(direction, 0, 0, &x, &y, &textWidth, &textHeight);
    display.setCursor(SCREEN_WIDTH / 2 - textWidth / 2, SCREEN_HEIGHT / 2 - textHeight / 2);
    display.print(direction);
  }
}

void tomatoAlert()
{
  ledcWriteTone(VIBRO_PWM_CHANNEL, MAX_DUTY_CYCLE);
  startTime = millis();
}

void menu()
{
  display.setTextSize(2);
  display.setTextColor(WHITE, BLACK);
  String monkeyDance = "ARE YOU\n READY TO\n DANCE,\n MONKEY?";
  display.getTextBounds(monkeyDance, 0, 0, &x, &y, &textWidth, &textHeight);
  display.setCursor(SCREEN_WIDTH / 2 - textWidth / 2, SCREEN_HEIGHT / 2 - textHeight / 2);
  display.print(monkeyDance);
}

void endGame()
{
  String text;
  if(livesRemaining <= 0) {
    text = "You lose,\n  monkey!";
  } else {
    text = "Nice moves\n  monkey!";
  }
  display.setTextSize(2);
  display.setTextColor(WHITE, BLACK);
  display.getTextBounds(text, 0, 0, &x, &y, &textWidth, &textHeight);
  display.setCursor(SCREEN_WIDTH / 2 - textWidth / 2, SCREEN_HEIGHT / 2 - textHeight / 2);
  display.print(text);
}

void drawLives()
{
  display.setTextSize(2);
  for (int i = 0; i < livesRemaining; i++)
  {
    display.setCursor(0 + (12 * i), 0);
    display.write(3);
  }
}