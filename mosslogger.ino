// Super simple SD card logging, with relay controlled sensor and ADC on/off.
// With normal SD.h library and one SD card, readings saved only 173.2 kB, from 2019-11-08T20:38:23 to 2019-11-08T21:28:17


#include <SPI.h>
// #include <SD.h>
#include <Controllino.h> // Usage of CONTROLLINO library allows you to use CONTROLLINO_xx aliases in your sketch.
#include <Wire.h>
#include <Adafruit_ADS1015.h>

// include SdFat library w/ Arduino IDE gives these
#include <SdFatConfig.h>
#include <sdios.h>
#include <FreeStack.h>
#include <MinimumSerial.h>
#include <SdFat.h>
#include <BlockDriver.h>
#include <SysCall.h>

Adafruit_ADS1115 ads0(0x48);
Adafruit_ADS1115 ads1(0x49);
Adafruit_ADS1115 ads2(0x4A);
Adafruit_ADS1115 ads3(0x4B);

// these control power to ACDs (ADS1115s), need to shut down to avoid heating the samples with thermistors
// NOTE: for some reason, using just one relay to cut current causes the Controllino/code to get stuck on first reading in measurementRound()
#define currentRelay CONTROLLINO_R0
#define groundRelay CONTROLLINO_R1

SdFat sd1;
const uint8_t SD1_CS = 53;  // chip select for sd1
SdFat sd2;
const uint8_t SD2_CS = 7;   // chip select for sd2 // CONTROLLINO_D5 = pin 7 on Arduino MEGA

//------------------------------------------------------------------------------
// print error msg, any SD error codes, and halt.
// store messages in flash
#define errorExit(msg) errorHalt(F(msg))
#define initError(msg) initErrorHalt(F(msg))
//------------------------------------------------------------------------------

// These are used in getTimeAndDate()
char dateAndTimeData[20]; // space for YYYY-MM-DDTHH-MM-SS, plus the null char terminator
char logfileName[13]; // space for DDHHMMSS.csv, plus the null char terminator
uint8_t thisYear; // Controllino RTC library gives only two digits with Controllino_GetYear(), "2000 + thisYear" used in getTimeAndDate()
uint8_t thisMonth; // = Controllino_GetMonth();
uint8_t thisDay; // = Controllino_GetDay();
uint8_t thisHour; // = Controllino_GetHour();
uint8_t thisMinute; // = Controllino_GetMinute();
uint8_t thisSecond; // = Controllino_GetSecond();

uint8_t logDay; // = Controllino_GetDay();
uint8_t logHour; // = Controllino_GetHour();
uint8_t logMinute; // = Controllino_GetMinute();
uint8_t logSecond; // = Controllino_GetSecond();

File logfile1;
File logfile2;

// uint8_t previousLogFileDay = 0; // Stores the value of RTC day when logFile was started, i.e. current day.

// unsigned long startMillis = 0;
// unsigned long currentMillis = 0;

unsigned long startShutDownPeriod = 0;

//const unsigned long measurementPeriod = 10000; // in milliseconds, how long to keep making measurement rounds
const unsigned long shutDownPeriod = 10000; // in milliseconds, how long to power off ADCs between measurement periods

const unsigned long measurementRoundPeriod = 1000; //  in milliseconds, how long to loop through ADCs reading values in, before calculating the averages and writing the new data to the file on SD card.

//void error(char *str) {
//  Serial.print("error: ");
//  Serial.println(str);
//  while (1);
//}
//------------------------------------------------------------------------------
// print error msg, any SD error codes, and halt.
// store messages in flash
#define errorExit(msg) errorHalt(F(msg))
#define initError(msg) initErrorHalt(F(msg))
//------------------------------------------------------------------------------

void getTimeAndDate() {
  thisYear = 2000 + Controllino_GetYear(); // "2000 +" because Controllino RTC library gives only two digits with Controllino_GetYear()
  thisMonth = Controllino_GetMonth();
  thisDay = Controllino_GetDay();
  thisHour = Controllino_GetHour();
  thisMinute = Controllino_GetMinute();
  thisSecond = Controllino_GetSecond();

  sprintf(dateAndTimeData, ("%4d-%02d-%02dT%02d:%02d:%02d"), thisYear, thisMonth, thisDay, thisHour, thisMinute, thisSecond);
}

void getLogFileName() {
  // char logfileName[13]; // space for DDHHMMSS.csv, plus the null char terminator
  logDay = Controllino_GetDay();
  logHour = Controllino_GetHour();
  logMinute = Controllino_GetMinute();
  logSecond = Controllino_GetSecond();

  sprintf(logfileName, ("%02d%02d%02d%02d.csv"), logDay, logHour, logMinute, logSecond);
}

void initializeADCs() {
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

  ads0.setGain(GAIN_TWOTHIRDS);
  ads1.setGain(GAIN_TWOTHIRDS);
  ads2.setGain(GAIN_TWOTHIRDS);
  ads3.setGain(GAIN_TWOTHIRDS);

  ads0.begin();
  ads1.begin();
  ads2.begin();
  ads3.begin();
}

void initializeSdCards() {

  // disable sd2 while initializing sd1
  pinMode(SD2_CS, OUTPUT);
  digitalWrite(SD2_CS, HIGH);

  // initialize sd1
  Serial.print("Initializing SD card 1...");
  // make sure that the default chip select pin is set to
  // output, even if you don't use it:
  pinMode(SD1_CS, OUTPUT);

  // see if the card is present and can be initialized:
  if (!sd1.begin(SD1_CS)) {
    sd1.initError("sd1:");
    //error("Card 1 failed, or not present");
  }

  Serial.println("Card 1 initialized.");

  // initialize sd2
  Serial.print("Initializing SD card 2...");

  // see if the card is present and can be initialized:
  if (!sd1.begin(SD2_CS)) {
    sd2.initError("sd2:");
    //error("Card 2 failed, or not present");
  }

  Serial.println("Card 2 initialized.");

  getLogFileName();

  if (! sd1.exists(logfileName)) {
    // only open a new file if it doesn't exist
    logfile1 = sd1.open(logfileName, FILE_WRITE);
  }

  if (! logfile1) {
    sd1.errorExit("logfile1");
    //error("couldnt create file 1");
  }

  Serial.print("Logging on card 1 to: ");
  Serial.println(logfileName);

  if (! sd2.exists(logfileName)) {
    // only open a new file if it doesn't exist
    logfile2 = sd2.open(logfileName, FILE_WRITE);
  }

  if (! logfile2) {
    sd2.errorExit("logfile2");
    //error("couldnt create file 2");
  }

  Serial.print("Logging on card 2 to: ");
  Serial.println(logfileName);
}

void measurementRound() {

  unsigned long measurementRoundStartMillis = 0;
  int16_t measurementRoundCounter = 0;

  long measurementRoundAverage00 = 0, measurementRoundAverage01 = 0, measurementRoundAverage02 = 0, measurementRoundAverage03 = 0;
  long measurementRoundAverage10 = 0, measurementRoundAverage11 = 0, measurementRoundAverage12 = 0, measurementRoundAverage13 = 0;
  long measurementRoundAverage20 = 0, measurementRoundAverage21 = 0, measurementRoundAverage22 = 0, measurementRoundAverage23 = 0;
  long measurementRoundAverage30 = 0, measurementRoundAverage31 = 0, measurementRoundAverage32 = 0, measurementRoundAverage33 = 0;

  int16_t measurement00, measurement01, measurement02, measurement03;
  int16_t measurement10, measurement11, measurement12, measurement13;
  int16_t measurement20, measurement21, measurement22, measurement23;
  int16_t measurement30, measurement31, measurement32, measurement33;

  getTimeAndDate(); //store date and time before taking measurements/reading ACDs, will be printed to logfile along the values measured

  measurementRoundStartMillis = millis();

  while (millis() - measurementRoundStartMillis <= measurementRoundPeriod) {
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

    measurementRoundAverage00 += measurement00;
    measurementRoundAverage01 += measurement01;
    measurementRoundAverage02 += measurement02;
    measurementRoundAverage03 += measurement03;

    measurementRoundAverage10 += measurement10;
    measurementRoundAverage11 += measurement11;
    measurementRoundAverage12 += measurement12;
    measurementRoundAverage13 += measurement13;

    measurementRoundAverage20 += measurement20;
    measurementRoundAverage21 += measurement21;
    measurementRoundAverage22 += measurement22;
    measurementRoundAverage23 += measurement23;

    measurementRoundAverage30 += measurement30;
    measurementRoundAverage31 += measurement31;
    measurementRoundAverage32 += measurement32;
    measurementRoundAverage33 += measurement33;

    measurementRoundCounter++;

  }

  measurementRoundAverage00 /= measurementRoundCounter;
  measurementRoundAverage01 /= measurementRoundCounter;
  measurementRoundAverage02 /= measurementRoundCounter;
  measurementRoundAverage03 /= measurementRoundCounter;

  measurementRoundAverage10 /= measurementRoundCounter;
  measurementRoundAverage11 /= measurementRoundCounter;
  measurementRoundAverage12 /= measurementRoundCounter;
  measurementRoundAverage13 /= measurementRoundCounter;

  measurementRoundAverage20 /= measurementRoundCounter;
  measurementRoundAverage21 /= measurementRoundCounter;
  measurementRoundAverage22 /= measurementRoundCounter;
  measurementRoundAverage23 /= measurementRoundCounter;

  measurementRoundAverage30 /= measurementRoundCounter;
  measurementRoundAverage31 /= measurementRoundCounter;
  measurementRoundAverage32 /= measurementRoundCounter;
  measurementRoundAverage33 /= measurementRoundCounter;

  logfile1.print(dateAndTimeData);

  logfile1.print(",");

  logfile1.print(measurementRoundAverage00);
  logfile1.print(",");
  logfile1.print(measurementRoundAverage01);
  logfile1.print(",");
  logfile1.print(measurementRoundAverage02);
  logfile1.print(",");
  logfile1.print(measurementRoundAverage03);
  logfile1.print(",");

  logfile1.print(measurementRoundAverage10);
  logfile1.print(",");
  logfile1.print(measurementRoundAverage11);
  logfile1.print(",");
  logfile1.print(measurementRoundAverage12);
  logfile1.print(",");
  logfile1.print(measurementRoundAverage13);
  logfile1.print(",");

  logfile1.print(measurementRoundAverage20);
  logfile1.print(",");
  logfile1.print(measurementRoundAverage21);
  logfile1.print(",");
  logfile1.print(measurementRoundAverage22);
  logfile1.print(",");
  logfile1.print(measurementRoundAverage23);
  logfile1.print(",");

  logfile1.print(measurementRoundAverage30);
  logfile1.print(",");
  logfile1.print(measurementRoundAverage31);
  logfile1.print(",");
  logfile1.print(measurementRoundAverage32);
  logfile1.print(",");
  logfile1.println(measurementRoundAverage33);

  logfile1.close();

  logfile2.print(dateAndTimeData);

  logfile2.print(",");

  logfile2.print(measurementRoundAverage00);
  logfile2.print(",");
  logfile2.print(measurementRoundAverage02);
  logfile2.print(",");
  logfile2.print(measurementRoundAverage02);
  logfile2.print(",");
  logfile2.print(measurementRoundAverage03);
  logfile2.print(",");

  logfile2.print(measurementRoundAverage20);
  logfile2.print(",");
  logfile2.print(measurementRoundAverage22);
  logfile2.print(",");
  logfile2.print(measurementRoundAverage22);
  logfile2.print(",");
  logfile2.print(measurementRoundAverage23);
  logfile2.print(",");

  logfile2.print(measurementRoundAverage20);
  logfile2.print(",");
  logfile2.print(measurementRoundAverage22);
  logfile2.print(",");
  logfile2.print(measurementRoundAverage22);
  logfile2.print(",");
  logfile2.print(measurementRoundAverage23);
  logfile2.print(",");

  logfile2.print(measurementRoundAverage30);
  logfile2.print(",");
  logfile2.print(measurementRoundAverage32);
  logfile2.print(",");
  logfile2.print(measurementRoundAverage32);
  logfile2.print(",");
  logfile2.println(measurementRoundAverage33);

  logfile2.close();
}

void setup() {

  Serial.begin(9600);

  // Wait for USB Serial
  while (!Serial) {
    SysCall::yield();
  }

  pinMode(currentRelay, OUTPUT);
  pinMode(groundRelay, OUTPUT);

  Controllino_RTC_init(0);

  digitalWrite(currentRelay, HIGH);
  digitalWrite(groundRelay, HIGH);
  delay(500); // allows time for ACDs to start

  initializeADCs();
  initializeSdCards();

  measurementRound();
  
  startShutDownPeriod = millis();

  digitalWrite(currentRelay, LOW);
  digitalWrite(groundRelay, LOW);
}

void loop() {

  // REFACTORING loop():
  //startShutDownPeriod = 0;
  //measurementPeriod = 10000;
  //shutDownPeriod = 10000;

  if (millis() - startShutDownPeriod <= shutDownPeriod) {
    digitalWrite(currentRelay, HIGH);
    digitalWrite(groundRelay, HIGH);
    delay(500); // allows time for ACDs to start

    //getTimeAndDate();
    //initializeADCs();
    //initializeSdCards();
    getLogFileName();

    if (! sd1.exists(logfileName)) {
      // only open a new file if it doesn't exist
      logfile1 = sd1.open(logfileName, FILE_WRITE);
    }

    if (! logfile1) {
      sd1.errorExit("logfile1");
      //error("couldnt create file 1");
    }

    Serial.print("Logging on card 1 to: ");
    Serial.println(logfileName);

    getLogFileName();

    if (! sd2.exists(logfileName)) {
      // only open a new file if it doesn't exist
      logfile2 = sd2.open(logfileName, FILE_WRITE);
    }

    if (! logfile2) {
      sd2.errorExit("logfile2");
      //error("couldnt create file 2");
    }

    Serial.print("Logging on card 2 to: ");
    Serial.println(logfileName);

    measurementRound();
    
    startShutDownPeriod = millis();

    digitalWrite(currentRelay, LOW);
    digitalWrite(groundRelay, LOW);
  }
}
