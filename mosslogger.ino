// Getting readings with voltage from MEGA 5V pin

#include <Controllino.h>
#include <Wire.h>
#include <Adafruit_ADS1015.h>

Adafruit_ADS1115 ads0(0x48);
Adafruit_ADS1115 ads1(0x49);
Adafruit_ADS1115 ads2(0x4A);
Adafruit_ADS1115 ads3(0x4B);

unsigned long previousMillis = 0;
unsigned long missedMillis = 0;
//unsigned long measurementMillis = 0;
int16_t counter = 0;

long oneSecondAverage00 = 0, oneSecondAverage01 = 0, oneSecondAverage02 = 0, oneSecondAverage03 = 0;
long oneSecondAverage10 = 0, oneSecondAverage11 = 0, oneSecondAverage12 = 0, oneSecondAverage13 = 0;
long oneSecondAverage20 = 0, oneSecondAverage21 = 0, oneSecondAverage22 = 0, oneSecondAverage23 = 0;
long oneSecondAverage30 = 0, oneSecondAverage31 = 0, oneSecondAverage32 = 0, oneSecondAverage33 = 0;

int16_t measurement00, measurement01, measurement02, measurement03;
int16_t measurement10, measurement11, measurement12, measurement13;
int16_t measurement20, measurement21, measurement22, measurement23;
int16_t measurement30, measurement31, measurement32, measurement33;

void setup()

Serial.begin(9600);

// The ADC input range (or gain) can be changed via the following
// functions, but be careful never to exceed VDD +0.3V max, or to
// exceed the upper and lower limits if you adjust the input range!
// Setting these values incorrectly may destroy your ADC!
//                                                                ADS1115
//                                                                -------
// ads.setGain(GAIN_TWOTHIRDS);  // 2/3x gain +/- 6.144V  1 bit = 0.1875mV (default)
// ads.setGain(GAIN_ONE);        // 1x gain   +/- 4.096V  1 bit = 0.125mV
// ads.setGain(GAIN_TWO);        // 2x gain   +/- 2.048V  1 bit = 0.0625mV
// ads.setGain(GAIN_FOUR);       // 4x gain   +/- 1.024V  1 bit = 0.03125mV
// ads.setGain(GAIN_EIGHT);      // 8x gain   +/- 0.512V  1 bit = 0.015625mV
// ads.setGain(GAIN_SIXTEEN);    // 16x gain  +/- 0.256V  1 bit = 0.0078125mV

ads0.setGain(GAIN_TWOTHIRDS);        // 1x gain   +/- 4.096V  1 bit = 0.125mV
ads1.setGain(GAIN_TWOTHIRDS);        // 1x gain   +/- 4.096V  1 bit = 0.125mV
ads2.setGain(GAIN_TWOTHIRDS);        // 1x gain   +/- 4.096V  1 bit = 0.125mV
ads3.setGain(GAIN_TWOTHIRDS);        // 1x gain   +/- 4.096V  1 bit = 0.125mV

ads0.begin();
ads1.begin();
ads2.begin();
ads3.begin();
}

void loop() {

  previousMillis = millis();
  //  measurementMillis = millis();

  while (millis() - previousMillis <= 1000) {
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

    counter = counter + 1;

    oneSecondAverage00 += measurement00;
    oneSecondAverage01 += measurement01;
    oneSecondAverage02 += measurement02;
    oneSecondAverage03 += measurement03;

    oneSecondAverage10 += measurement10;
    oneSecondAverage11 += measurement11;
    oneSecondAverage12 += measurement12;
    oneSecondAverage13 += measurement13;

    oneSecondAverage20 += measurement20;
    oneSecondAverage21 += measurement21;
    oneSecondAverage22 += measurement22;
    oneSecondAverage23 += measurement23;

    oneSecondAverage30 += measurement30;
    oneSecondAverage31 += measurement31;
    oneSecondAverage32 += measurement32;
    oneSecondAverage33 += measurement33;
  }

  //missedMillis = millis();

  //  Serial.print(counter);
  //  Serial.print(" rounds of readings in 1 sec, total readings: ");

  oneSecondAverage00 /= counter;
  oneSecondAverage01 /= counter;
  oneSecondAverage02 /= counter;
  oneSecondAverage03 /= counter;

  oneSecondAverage10 /= counter;
  oneSecondAverage11 /= counter;
  oneSecondAverage12 /= counter;
  oneSecondAverage13 /= counter;

  oneSecondAverage20 /= counter;
  oneSecondAverage21 /= counter;
  oneSecondAverage22 /= counter;
  oneSecondAverage23 /= counter;

  oneSecondAverage30 /= counter;
  oneSecondAverage31 /= counter;
  oneSecondAverage32 /= counter;
  oneSecondAverage33 /= counter;

  Serial.print(oneSecondAverage00);
  Serial.print(",");
  Serial.print(oneSecondAverage01);
  Serial.print(",");
  Serial.print(oneSecondAverage02);
  Serial.print(",");
  Serial.print(oneSecondAverage03);
  Serial.print(",");

  Serial.print(oneSecondAverage10);
  Serial.print(",");
  Serial.print(oneSecondAverage11);
  Serial.print(",");
  Serial.print(oneSecondAverage12);
  Serial.print(",");
  Serial.print(oneSecondAverage13);
  Serial.print(",");

  Serial.print(",");

  Serial.print(oneSecondAverage20);
  Serial.print(",");
  Serial.print(oneSecondAverage21);
  Serial.print(",");
  Serial.print(oneSecondAverage22);
  Serial.print(",");
  Serial.print(oneSecondAverage23);
  Serial.print(",");

  Serial.print(oneSecondAverage30);
  Serial.print(",");
  Serial.print(oneSecondAverage31);
  Serial.print(",");
  Serial.print(oneSecondAverage32);
  Serial.print(",");
  Serial.print(oneSecondAverage33);

  //  Serial.print(",");
  //  measurementMillis = millis() - measurementMillis;
  //  Serial.print(measurementMillis);

  Serial.println();

  counter = 0;

  oneSecondAverage00 = 0;
  oneSecondAverage01 = 0;
  oneSecondAverage02 = 0;
  oneSecondAverage03 = 0;

  oneSecondAverage10 = 0;
  oneSecondAverage11 = 0;
  oneSecondAverage12 = 0;
  oneSecondAverage13 = 0;

  oneSecondAverage20 = 0;
  oneSecondAverage21 = 0;
  oneSecondAverage22 = 0;
  oneSecondAverage23 = 0;

  oneSecondAverage30 = 0;
  oneSecondAverage31 = 0;
  oneSecondAverage32 = 0;
  oneSecondAverage33 = 0;

  //missedMillis = millis() - missedMillis;

  //  Serial.print("Missed milliseconds while printing and counting: ");
  //  Serial.println(missedMillis);

  //missedMillis = 0;

  // in one second getting 7 rounds of readings, 128 milliseconds missed in printing and counting

  // when 8 or 16 single-ended readings, 1120 readings in 10000 milliseconds

  // for 8 single-ended readings * 100, time is 7115 milliseconds
  // for 16 single-ended readings * 100, time is 14230 milliseconds

}
