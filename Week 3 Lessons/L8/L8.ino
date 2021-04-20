
class Blinker{
  private:
    const int _pin;
    const unsigned long _interval;

    int _state;
    unsigned long _lastToggledTimestamp;

  public:
    Blinker(int pin, unsigned long blinkInterval) :
      _pin(pin), _interval(blinkInterval)
    {
      _state = LOW;
      _lastToggledTimestamp = 0;
      pinMode(_pin, OUTPUT);
    }
    
  void update(){
    unsigned long currentTimestampMs = millis();

    if(currentTimestampMs - _lastToggledTimestamp >= _interval) {
      _lastToggledTimestamp = currentTimestampMs;
      _state = !_state;
      digitalWrite(_pin, _state);
    }
  }
};

Blinker _led1Blinker(2, 200);
Blinker _led2Blinker(5, 333);
Blinker _led3Blinker(9, 1111);

void setup() {
  // put your setup code here, to run once:

}

void loop() {
  // put your main code here, to run repeatedly:
  _led1Blinker.update();
  _led2Blinker.update();
  _led3Blinker.update();
}
