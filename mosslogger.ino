// Super simple SD card logging 

#include <SPI.h>
#include <SD.h>
#include <Controllino.h>
#include <Wire.h>
#include <Adafruit_ADS1015.h>

Adafruit_ADS1115 ads0(0x48);
Adafruit_ADS1115 ads1(0x49);
Adafruit_ADS1115 ads2(0x4A);
Adafruit_ADS1115 ads3(0x4B);

// the digital pins that connect to the LEDs digitalWrite(CONTROLLINO_D0, HIGH);
#define redLEDpin CONTROLLINO_D0
#define greenLEDpin CONTROLLINO_D1

unsigned long previousMillis = 0;

// for the data logging shield, we use digital pin 10 for the SD cs line
const int chipSelect = 53;

// the logging file
File logfile;

void error(char *str)
{
  Serial.print("error: ");
  Serial.println(str);
  
  // red LED indicates error
  digitalWrite(redLEDpin, HIGH);

  while(1);
}

void setup() {

  Serial.begin(9600);

  pinMode(redLEDpin, OUTPUT);
  pinMode(greenLEDpin, OUTPUT);

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

  // initialize the SD card
  Serial.print("Initializing SD card...");
  // make sure that the default chip select pin is set to
  // output, even if you don't use it:
  pinMode(53, OUTPUT);
  
  // see if the card is present and can be initialized:
  if (!SD.begin(chipSelect)) {
    error("Card failed, or not present");
  }
  Serial.println("card initialized.");
  
  // create a new file
  char filename[] = "LOGGER00.CSV";
  for (uint8_t i = 0; i < 100; i++) {
    filename[6] = i/10 + '0';
    filename[7] = i%10 + '0';
    if (! SD.exists(filename)) {
      // only open a new file if it doesn't exist
      logfile = SD.open(filename, FILE_WRITE); 
      break;  // leave the loop!
    }
  }
  
  if (! logfile) {
    error("couldnt create file");
  }
  
  Serial.print("Logging to: ");
  Serial.println(filename);

}

void loop() {

  digitalWrite(greenLEDpin, HIGH);

  int16_t counter = 0;

  long oneSecondAverage00 = 0, oneSecondAverage01 = 0, oneSecondAverage02 = 0, oneSecondAverage03 = 0;
  long oneSecondAverage10 = 0, oneSecondAverage11 = 0, oneSecondAverage12 = 0, oneSecondAverage13 = 0;
  long oneSecondAverage20 = 0, oneSecondAverage21 = 0, oneSecondAverage22 = 0, oneSecondAverage23 = 0;
  long oneSecondAverage30 = 0, oneSecondAverage31 = 0, oneSecondAverage32 = 0, oneSecondAverage33 = 0;

  int16_t measurement00, measurement01, measurement02, measurement03;
  int16_t measurement10, measurement11, measurement12, measurement13;
  int16_t measurement20, measurement21, measurement22, measurement23;
  int16_t measurement30, measurement31, measurement32, measurement33;

  previousMillis = millis();

  Serial.println("Reading...");
  
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

    counter = counter + 1;
  }

  Serial.println("Calculating...");

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

  Serial.println("Writing...");

  
  
  
  

  logfile.print(oneSecondAverage00);
  Serial.print(oneSecondAverage00);
  logfile.print(",");
  Serial.print(",");
  logfile.print(oneSecondAverage01);
  Serial.print(oneSecondAverage01);
  logfile.print(",");
  Serial.println("...");
  logfile.print(oneSecondAverage02);
  logfile.print(",");
  logfile.print(oneSecondAverage03);
  logfile.print(",");

  logfile.print(oneSecondAverage10);
  logfile.print(",");
  logfile.print(oneSecondAverage11);
  logfile.print(",");
  logfile.print(oneSecondAverage12);
  logfile.print(",");
  logfile.print(oneSecondAverage13);
  logfile.print(",");

  logfile.print(oneSecondAverage20);
  logfile.print(",");
  logfile.print(oneSecondAverage21);
  logfile.print(",");
  logfile.print(oneSecondAverage22);
  logfile.print(",");
  logfile.print(oneSecondAverage23);
  logfile.print(",");

  logfile.print(oneSecondAverage30);
  logfile.print(",");
  logfile.print(oneSecondAverage31);
  logfile.print(",");
  logfile.print(oneSecondAverage32);
  logfile.print(",");
  logfile.println(oneSecondAverage33);

  digitalWrite(greenLEDpin, LOW);

  // blink LED to show we are syncing data to the card & updating FAT!
  digitalWrite(redLEDpin, HIGH);
  logfile.flush();
  digitalWrite(redLEDpin, LOW);

  Serial.println("Loop done.");


  // in one second getting 7 rounds of readings, 128 milliseconds missed in printing and counting

  // when 8 or 16 single-ended readings, 1120 readings in 10000 milliseconds

  // for 8 single-ended readings * 100, time is 7115 milliseconds
  // for 16 single-ended readings * 100, time is 14230 milliseconds

}
