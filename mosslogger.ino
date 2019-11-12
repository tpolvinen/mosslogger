// Super simple SD card logging, with relay controlled sensor and ADC on/off.
// With normal SD.h library and one SD card, readings saved only 173.2 kB, from 2019-11-08T20:38:23 to 2019-11-08T21:28:17


#include <SPI.h>
 #include <SD.h>
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

// the digital pins that connect to the LEDs, using e.g. digitalWrite(CONTROLLINO_D0, HIGH);
// CONTROLLINO_D0 is used for SD2_CS (pin 2 on Arduino MEGA)
#define readingLedPin CONTROLLINO_D1
#define writingLedPin CONTROLLINO_D2
#define errorLedPin CONTROLLINO_D3

// these control power to ACDs (ADS1115s), (may) need to shut down to avoid heating the samples with thermistors
// NOTE: for some reason, using just one relay to cut current causes the Controllino/code to get stuck on first reading in measurementRound()
// ...apparently shorting the current and ground through ADCs?!?
#define currentRelay CONTROLLINO_R0
#define groundRelay CONTROLLINO_R1

// const int chipSelect = 53; // digital pin 53 for SD module's CS line

SdFat sd1;
const uint8_t SD1_CS = 53;  // chip select for sd1

SdFat sd2;
const uint8_t SD2_CS = 2;   // chip select for sd2 // CONTROLLINO_D0 = pin 2 on Arduino MEGA

const uint8_t BUF_DIM = 100;
uint8_t buf[BUF_DIM];

const uint32_t FILE_SIZE = 1000000;
const uint16_t NWRITE = FILE_SIZE/BUF_DIM;
//------------------------------------------------------------------------------
// print error msg, any SD error codes, and halt.
// store messages in flash
#define errorExit(msg) errorHalt(F(msg))
#define initError(msg) initErrorHalt(F(msg))
//------------------------------------------------------------------------------

// These are used in getTimeAndDate()
char dateAndTimeData[20]; // space for YYYY-MM-DDTHH-MM-SS, plus the null char terminator
uint8_t thisYear; // Controllino RTC library gives only two digits with Controllino_GetYear(), "2000 + thisYear" used in getTimeAndDate()
uint8_t thisMonth; // = Controllino_GetMonth();
uint8_t thisDay; // = Controllino_GetDay();
uint8_t thisHour; // = Controllino_GetHour();
uint8_t thisMinute; // = Controllino_GetMinute();
uint8_t thisSecond; // = Controllino_GetSecond();

uint8_t loopCounterForLog; // Used to keep track of loop() iterations, to check the need for a new log file at intervals (not sure if sane!)
File logfile;
bool needsNewLogFile; // Used to mark the need for a new log file when the time period for one log file has been used up
uint8_t logFileChangeOverTimeUnit; // when this time int value changes, as many times as logFileKeepCounter, new log file is started
const uint8_t logFileKeepCounter = 1; // How many RTC's time units to use one logfile for (as in year, month, day, hour, minute, second)
uint8_t previousLogFileTime = 0; // Stores the value of current RTC time unit

bool measuring; // This is used with millis() timing to control relay that connects power to ACDs (ADS1115s)

unsigned long startMillis = 0;
unsigned long currentMillis = 0;

const unsigned long hammerTime = 10000; // You can't touch this.
const unsigned long shutDownTime = 10000;

const unsigned long measurementRoundMillis = 1000; // How long to loop through ADCs reading values in, before calculating the averages and flushing the new data to the file on SD card.





void setup() {

  Serial.begin(9600);

  // Wait for USB Serial 
  while (!Serial) {
    SysCall::yield();
  }
  
  twoCardsTest();

  Controllino_RTC_init(0);

  getTimeAndDate();
  logFileChangeOverTimeUnit = thisDay;  // THIS JUST PLAIN DOES NOT WORK! Think again tomorrow. :)

  pinMode(readingLedPin, OUTPUT);
  pinMode(writingLedPin, OUTPUT);
  pinMode(errorLedPin, OUTPUT);

  pinMode(currentRelay, OUTPUT);
  pinMode(groundRelay, OUTPUT);

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

  initializeSdCard();

  measuring = true;
  digitalWrite(currentRelay, HIGH);
  digitalWrite(groundRelay, HIGH);
  startMillis = millis();

}

void error(char *str) {
  Serial.print("error: ");
  Serial.println(str);
  digitalWrite(errorLedPin, HIGH); // error LED indicates error :D
  while (1);
}

void initializeSdCard() {

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
    filename[6] = i / 10 + '0';
    filename[7] = i % 10 + '0';
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

void getTimeAndDate() {
  thisYear = 2000 + Controllino_GetYear(); // "2000 +" because Controllino RTC library gives only two digits with Controllino_GetYear()
  thisMonth = Controllino_GetMonth();
  thisDay = Controllino_GetDay();
  thisHour = Controllino_GetHour();
  thisMinute = Controllino_GetMinute();
  thisSecond = Controllino_GetSecond();

  sprintf(dateAndTimeData, ("%4d-%02d-%02dT%02d:%02d:%02d"), thisYear, thisMonth, thisDay, thisHour, thisMinute, thisSecond);

}

void checkNeedForNewLogFile() {
  
}

void newLogFile() {

}



void loop() {

  currentMillis = millis();

  if (measuring) {
    if (currentMillis - startMillis >= hammerTime) { // check if the time for measurement rounds is elapsed
      digitalWrite(currentRelay, LOW);
      digitalWrite(groundRelay, LOW);
      measuring = false;
      Serial.println("Moving from hammerTime to shutDownTime!");
      startMillis = millis();
    } else {
      measurementRound();
      if (needsNewLogFile) {
        newLogFile();
      }
    }
  } else {
    if (currentMillis - startMillis >= shutDownTime) {
      digitalWrite(currentRelay, HIGH);
      digitalWrite(groundRelay, HIGH);
      measuring = true;
      Serial.println("Moving from shutDownTime to hammerTime!");
      startMillis = millis();
    }
  }

  loopCounterForLog++;

  if (loopCounter <= loopNumberForLog) {
    checkNeedForNewLogFile();
    if (needsNewLogFile) {
      newLogFile();
    }
    loopCounter = 0;
  }

}

void measurementRound() { //Could probably take time and date char array as parameter?

  unsigned long loopMillis = 0;
  unsigned long readingMillis = 0;
  unsigned long calculatingMillis = 0;
  unsigned long loggingMillis = 0;
  unsigned long sdMillis = 0;
  int16_t readingRoundCounter = 0;

  loopMillis = millis();

  long measurementRoundAverage00 = 0, measurementRoundAverage01 = 0, measurementRoundAverage02 = 0, measurementRoundAverage03 = 0;
  long measurementRoundAverage10 = 0, measurementRoundAverage11 = 0, measurementRoundAverage12 = 0, measurementRoundAverage13 = 0;
  long measurementRoundAverage20 = 0, measurementRoundAverage21 = 0, measurementRoundAverage22 = 0, measurementRoundAverage23 = 0;
  long measurementRoundAverage30 = 0, measurementRoundAverage31 = 0, measurementRoundAverage32 = 0, measurementRoundAverage33 = 0;

  int16_t measurement00, measurement01, measurement02, measurement03;
  int16_t measurement10, measurement11, measurement12, measurement13;
  int16_t measurement20, measurement21, measurement22, measurement23;
  int16_t measurement30, measurement31, measurement32, measurement33;

  Serial.print("Reading,");
  digitalWrite(readingLedPin, HIGH);
  readingMillis = millis();

  getTimeAndDate(); //store date and time before taking measurements/reading ACDs, will be printed to logfile along the values measured

  while (millis() - readingMillis <= measurementRoundMillis) {
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

    readingRoundCounter++;

  }

  readingMillis = millis() - readingMillis;
  Serial.print(readingMillis);
  Serial.print(",");

  Serial.print("Calculating,");

  calculatingMillis = millis();

  measurementRoundAverage00 /= readingRoundCounter;
  measurementRoundAverage01 /= readingRoundCounter;
  measurementRoundAverage02 /= readingRoundCounter;
  measurementRoundAverage03 /= readingRoundCounter;

  measurementRoundAverage10 /= readingRoundCounter;
  measurementRoundAverage11 /= readingRoundCounter;
  measurementRoundAverage12 /= readingRoundCounter;
  measurementRoundAverage13 /= readingRoundCounter;

  measurementRoundAverage20 /= readingRoundCounter;
  measurementRoundAverage21 /= readingRoundCounter;
  measurementRoundAverage22 /= readingRoundCounter;
  measurementRoundAverage23 /= readingRoundCounter;

  measurementRoundAverage30 /= readingRoundCounter;
  measurementRoundAverage31 /= readingRoundCounter;
  measurementRoundAverage32 /= readingRoundCounter;
  measurementRoundAverage33 /= readingRoundCounter;

  calculatingMillis = millis() - calculatingMillis;
  Serial.print(calculatingMillis);
  Serial.print(",");

  Serial.print("Logfile writing,");

  loggingMillis = millis();

  logfile.print(dateAndTimeData);

  logfile.print(",");

  logfile.print(measurementRoundAverage00);
  logfile.print(",");
  logfile.print(measurementRoundAverage01);
  logfile.print(",");
  logfile.print(measurementRoundAverage02);
  logfile.print(",");
  logfile.print(measurementRoundAverage03);
  logfile.print(",");

  logfile.print(measurementRoundAverage10);
  logfile.print(",");
  logfile.print(measurementRoundAverage11);
  logfile.print(",");
  logfile.print(measurementRoundAverage12);
  logfile.print(",");
  logfile.print(measurementRoundAverage13);
  logfile.print(",");

  logfile.print(measurementRoundAverage20);
  logfile.print(",");
  logfile.print(measurementRoundAverage21);
  logfile.print(",");
  logfile.print(measurementRoundAverage22);
  logfile.print(",");
  logfile.print(measurementRoundAverage23);
  logfile.print(",");

  logfile.print(measurementRoundAverage30);
  logfile.print(",");
  logfile.print(measurementRoundAverage31);
  logfile.print(",");
  logfile.print(measurementRoundAverage32);
  logfile.print(",");
  logfile.println(measurementRoundAverage33);

  loggingMillis = millis() - loggingMillis;

  digitalWrite(readingLedPin, LOW);

  Serial.print(loggingMillis);
  Serial.print(",");

  Serial.print("SD card writing,");
  digitalWrite(writingLedPin, HIGH);

  sdMillis = millis();

  logfile.flush();

  sdMillis = millis() - sdMillis;

  Serial.print(sdMillis);
  Serial.print(",");
  digitalWrite(writingLedPin, LOW);

  loopMillis = millis() - loopMillis;

  Serial.print("Loop done in,");
  Serial.println(loopMillis);

}





void twoCardsTest() {
  Serial.print(F("FreeStack: "));

  Serial.println(FreeStack());

  // fill buffer with known data
  for (size_t i = 0; i < sizeof(buf); i++) {
    buf[i] = i;
  }

  Serial.println(F("type any character to start"));
  while (!Serial.available()) {
    SysCall::yield();
  }

  // disable sd2 while initializing sd1
  pinMode(SD2_CS, OUTPUT);
  digitalWrite(SD2_CS, HIGH);

  // initialize the first card
  if (!sd1.begin(SD1_CS)) {
    sd1.initError("sd1:");
  }
  // create Dir1 on sd1 if it does not exist
  if (!sd1.exists("/Dir1")) {
    if (!sd1.mkdir("/Dir1")) {
      sd1.errorExit("sd1.mkdir");
    }
  }
  // initialize the second card
  if (!sd2.begin(SD2_CS)) {
    sd2.initError("sd2:");
  }
// create Dir2 on sd2 if it does not exist
  if (!sd2.exists("/Dir2")) {
    if (!sd2.mkdir("/Dir2")) {
      sd2.errorExit("sd2.mkdir");
    }
  }
  // list root directory on both cards
  Serial.println(F("------sd1 root-------"));
  sd1.ls();
  Serial.println(F("------sd2 root-------"));
  sd2.ls();

  // make /Dir1 the default directory for sd1
  if (!sd1.chdir("/Dir1")) {
    sd1.errorExit("sd1.chdir");
  }

  // make /Dir2 the default directory for sd2
  if (!sd2.chdir("/Dir2")) {
    sd2.errorExit("sd2.chdir");
  }

  // list current directory on both cards
  Serial.println(F("------sd1 Dir1-------"));
  sd1.ls();
  Serial.println(F("------sd2 Dir2-------"));
  sd2.ls();
  Serial.println(F("---------------------"));

  // remove rename.bin from /Dir2 directory of sd2
  if (sd2.exists("rename.bin")) {
    if (!sd2.remove("rename.bin")) {
      sd2.errorExit("remove rename.bin");
    }
  }
  // set the current working directory for open() to sd1
  sd1.chvol();

  // create or open /Dir1/test.bin and truncate it to zero length
  SdFile file1;
  if (!file1.open("test.bin", O_RDWR | O_CREAT | O_TRUNC)) {
    sd1.errorExit("file1");
  }
  Serial.println(F("Writing test.bin to sd1"));

  // write data to /Dir1/test.bin on sd1
  for (uint16_t i = 0; i < NWRITE; i++) {
    if (file1.write(buf, sizeof(buf)) != sizeof(buf)) {
      sd1.errorExit("sd1.write");
    }
  }
  // set the current working directory for open() to sd2
  sd2.chvol();

  // create or open /Dir2/copy.bin and truncate it to zero length
  SdFile file2;
  if (!file2.open("copy.bin", O_WRONLY | O_CREAT | O_TRUNC)) {
    sd2.errorExit("file2");
  }
  Serial.println(F("Copying test.bin to copy.bin"));

  // copy file1 to file2
  file1.rewind();
  uint32_t t = millis();

  while (1) {
    int n = file1.read(buf, sizeof(buf));
    if (n < 0) {
      sd1.errorExit("read1");
    }
    if (n == 0) {
      break;
    }
    if ((int)file2.write(buf, n) != n) {
      sd2.errorExit("write2");
    }
  }
  t = millis() - t;
  Serial.print(F("File size: "));
  Serial.println(file2.fileSize());
  Serial.print(F("Copy time: "));
  Serial.print(t);
  Serial.println(F(" millis"));
  // close test.bin
  file1.close();
  file2.close(); 
  // list current directory on both cards
  Serial.println(F("------sd1 -------"));
  sd1.ls("/", LS_R | LS_DATE | LS_SIZE);
  Serial.println(F("------sd2 -------"));
  sd2.ls("/", LS_R | LS_DATE | LS_SIZE);
  Serial.println(F("---------------------"));
  Serial.println(F("Renaming copy.bin"));
  // rename the copy
  if (!sd2.rename("copy.bin", "rename.bin")) {
    sd2.errorExit("sd2.rename");
  }
  // list current directory on both cards
  Serial.println(F("------sd1 -------"));
  sd1.ls("/", LS_R | LS_DATE | LS_SIZE);
  Serial.println(F("------sd2 -------"));
  sd2.ls("/", LS_R | LS_DATE | LS_SIZE);
  Serial.println(F("---------------------"));
  Serial.println(F("Done"));

}
