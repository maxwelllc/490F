#include <CapacitiveSensor.h>

/*
 * CapitiveSense Library Demo Sketch
 * Paul Badger 2008
 * Uses a high value resistor e.g. 10M between send pin and receive pin
 * Resistor effects sensitivity, experiment with values, 50K - 50M. Larger resistor values yield larger sensor values.
 * Receive pin is the sensor pin - try different amounts of foil/metal on this pin
 */


// Capacitive sensor created between pins 2 (emitter) and 12 (sensor)
CapacitiveSensor cs_1 = CapacitiveSensor(12, 2);

// Capacitive sensor created between pins 4 (emitter) and 9 (sensor)
CapacitiveSensor cs_2 = CapacitiveSensor(4, 9);

void setup()                    
{
   Serial.begin(9600);
}

void loop()                    
{
    long start = millis();
    long total1 =  cs_1.capacitiveSensor(30);
    long total2 = cs_2.capacitiveSensor(30);

    Serial.print(millis() - start);        // check on performance in milliseconds
    Serial.print("\t");                    // tab character for debug windown spacing

    Serial.print(total1);                  // print sensor output 1
    Serial.print("\t");
    Serial.println(total2);
    
    
    delay(10);                             // arbitrary delay to limit data to serial port 
}
