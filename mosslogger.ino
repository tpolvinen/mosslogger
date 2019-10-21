#include <Controllino.h>
#include <Wire.h>
#include <Adafruit_ADS1015.h>


Adafruit_ADS1115 ads0;
Adafruit_ADS1115 ads1(0x49);


void setup() {
  Serial.begin(9600);
  ads0.begin();
  ads1.begin();
}

void loop() {

  int16_t measurement0;
  int16_t measurement1;
  measurement0 = ads0.readADC_SingleEnded(0);
  measurement1 = ads1.readADC_SingleEnded(0);

  Serial.print("ads0: ");

  Serial.println(measurement0);

  Serial.print("ads1: ");

  Serial.println(measurement1);

  delay(500);


}
