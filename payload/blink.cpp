/* Author: Owen Lyke
 Created: April 9 2019
 License: MIT. See SparkFun Arduino Apollo3 Project for more information

 This example demonstrates usage of:
 pinMode
 delay
*/

//Various boards have status LEDs on different pins
//Most have LED_BUILTIN defined so just use that
//But if you want, you can blink any pin

//#define blinkPin 5
#include "Arduino.h"
#include "Wire.h"

#define blinkPin LED_BUILTIN

void setup()
{
	Serial.begin(115200);
	// while (!Serial) ;

	Serial.println("SparkFun Arduino Apollo3 Wire Example (I2C Scanner)");
	// Serial.printf("Compiled on %s, %s\n\n", __DATE__, __TIME__);

	Wire.begin();
	pinMode(blinkPin, OUTPUT);
}

void loop()
{
	digitalWrite(blinkPin, LOW);
	delay(1000);
	digitalWrite(blinkPin, HIGH);
	delay(5000);
	Serial.println("Hello, world 3!");
}
