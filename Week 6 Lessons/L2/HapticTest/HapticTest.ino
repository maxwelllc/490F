void setup() {
  // initialize digital pin LED_BUILTIN as an output.
  pinMode(3, OUTPUT);
}

void loop() {
  

  analogWrite(3, 255);
  delay(50);
  
  analogWrite(3, 25);
  delay(100);

  analogWrite(3, 255);
  delay(50);

  analogWrite(3, 0);
  delay(1000);
}
