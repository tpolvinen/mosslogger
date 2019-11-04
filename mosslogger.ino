
// Test reading with 3V3 voltage from pin:
// Voltage scale factor used is 0.1875mV
// - because Programmable Gain Amplifier
// is not set, and in default it's +/-6.144V:
// ads.setGain(GAIN_TWOTHIRDS);  // 2/3x gain +/- 6.144V  1 bit = 0.1875mV (default)
// Will try with LS4040 reference voltage of 4.096V and use this gain setting:
// ads.setGain(GAIN_ONE);        // 1x gain   +/- 4.096V  1 bit = 0.125mV


#include <Controllino.h>
#include <Wire.h>
#include <Adafruit_ADS1015.h>


Adafruit_ADS1115 ads0(0x48);
Adafruit_ADS1115 ads1(0x49);
float Voltage = 0.0;

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
  Voltage = (measurement0 * 0.1875)/1000; 

  Serial.print("ads0: ");  
  Serial.print(measurement0);
  Serial.print("  Voltage: ");
  
  Voltage = (measurement0 * 0.1875)/1000;

  Serial.println(Voltage, 7);
  Serial.print("ads1: ");
  Serial.print(measurement1);
  Serial.print("  Voltage: ");
  
  Voltage = (measurement1 * 0.1875)/1000;
  
  Serial.println(Voltage, 7);

  delay(500);


}
