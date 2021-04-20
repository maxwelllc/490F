const int LED1_OUTPUT_PIN = 3; // Anode faces Pin 3 (cathode connected to 0V)
const int LED2_OUTPUT_PIN = 4; // Cathode faces Pin 4 (anode connected to 5V)
const int DELAY_MS = 1000; // delay for 1 sec between blinks

// The setup function runs once when you press reset or power the board
void setup() {
  // Set our LED pins as output
  pinMode(LED1_OUTPUT_PIN, OUTPUT);
  pinMode(LED2_OUTPUT_PIN, OUTPUT);
}

// The loop function runs over and over again forever
void loop() {
  // Below, you're going to see that driving Pin 3 HIGH will turn on LED1
  // but driving Pin 4 HIGH will actually turn *off* LED2
  digitalWrite(LED1_OUTPUT_PIN, HIGH);  // turns ON LED1
  digitalWrite(LED2_OUTPUT_PIN, HIGH);  // turns OFF LED2
  delay(DELAY_MS);                      // delay is in milliseconds; so wait one second
  
  digitalWrite(LED1_OUTPUT_PIN, LOW);   // turns OFF LED1 (Pin 3 is now 0V and other leg of LED is 0V)
  digitalWrite(LED2_OUTPUT_PIN, LOW);   // turns ON LED2 (Pin 4 is now 0V and other leg of LED is 5V)
  delay(DELAY_MS);                      // wait for a second
}
