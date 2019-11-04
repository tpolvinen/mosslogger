
// Trying readings with LS4040 reference voltage of 4.096V and use this gain setting:
// ads.setGain(GAIN_ONE);        // 1x gain   +/- 4.096V  1 bit = 0.125mV


#include <Controllino.h>
#include <Wire.h>
#include <Adafruit_ADS1015.h>


Adafruit_ADS1115 ads0(0x48);
Adafruit_ADS1115 ads1(0x49);
Adafruit_ADS1115 ads2(0x4A);
Adafruit_ADS1115 ads3(0x4B);
// float Voltage = 0.0;

void setup() {
  Serial.begin(9600);
  ads0.setGain(GAIN_ONE);        // 1x gain   +/- 4.096V  1 bit = 0.125mV
  ads1.setGain(GAIN_ONE);        // 1x gain   +/- 4.096V  1 bit = 0.125mV
  ads2.setGain(GAIN_ONE);        // 1x gain   +/- 4.096V  1 bit = 0.125mV
  ads3.setGain(GAIN_ONE);        // 1x gain   +/- 4.096V  1 bit = 0.125mV
  ads0.begin();
  ads1.begin();
    ads2.begin();
  ads3.begin();
}

void loop() {

  int16_t measurement00;
  int16_t measurement01;
  int16_t measurement02;
  int16_t measurement03;
  
  int16_t measurement10;
  int16_t measurement11;
  int16_t measurement12;
  int16_t measurement13;

  int16_t measurement20;
  int16_t measurement21;
  int16_t measurement22;
  int16_t measurement23;
  
  int16_t measurement30;
  int16_t measurement31;
  int16_t measurement32;
  int16_t measurement33;

  measurement00 = ads0.readADC_SingleEnded(0);
  measurement01 = ads0.readADC_SingleEnded(1);
  measurement02 = ads0.readADC_SingleEnded(2);
  measurement03 = ads0.readADC_SingleEnded(3);
  
  measurement10 = ads1.readADC_SingleEnded(0);
  measurement11 = ads1.readADC_SingleEnded(1);
  measurement12 = ads1.readADC_SingleEnded(2);
  measurement13 = ads1.readADC_SingleEnded(3);
  
  measurement20 = ads2.readADC_SingleEnded(0);
  measurement21 = ads2.readADC_SingleEnded(1);
  measurement22 = ads2.readADC_SingleEnded(2);
  measurement23 = ads2.readADC_SingleEnded(3);
  
  measurement30 = ads3.readADC_SingleEnded(0);
  measurement31 = ads3.readADC_SingleEnded(1);
  measurement32 = ads3.readADC_SingleEnded(2);
  measurement33 = ads3.readADC_SingleEnded(3);

  Serial.print("ads0:   ");
  Serial.print(measurement00);
  Serial.print("  ");
  Serial.print(measurement01);
  Serial.print("  ");
  Serial.print(measurement02);
  Serial.print("  ");
  Serial.println(measurement03);

//  Serial.print("  Voltage 0: ");
//  Voltage = (measurement00 * 0.125) / 1000;
//  Serial.print(Voltage, 4);
//  Serial.print("  ");
//  Voltage = (measurement01 * 0.125) / 1000;
//  Serial.println(Voltage, 4);

  Serial.print("ads1:   ");
  Serial.print(measurement10);
  Serial.print("  ");
  Serial.print(measurement11);
  Serial.print("  ");
  Serial.print(measurement12);
  Serial.print("  ");
  Serial.println(measurement13);

//  Serial.print("  Voltage 1: ");
//  Voltage = (measurement10 * 0.125) / 1000;
//  Serial.print(Voltage, 4);
//  Serial.print("  ");
//  Voltage = (measurement11 * 0.125) / 1000;
//  Serial.println(Voltage, 4);

  Serial.print("ads2:   ");
  Serial.print(measurement20);
  Serial.print("  ");
  Serial.print(measurement21);
  Serial.print("  ");
  Serial.print(measurement22);
  Serial.print("  ");
  Serial.println(measurement23);

  Serial.print("ads3:   ");
  Serial.print(measurement30);
  Serial.print("  ");
  Serial.print(measurement31);
  Serial.print("  ");
  Serial.print(measurement32);
  Serial.print("  ");
  Serial.println(measurement33);

  delay(500);


}
