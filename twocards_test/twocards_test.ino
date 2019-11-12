
#include <SPI.h>
#include "SdFat.h"
#include "FreeStack.h"
#include <Controllino.h> // Usage of CONTROLLINO library allows you to use CONTROLLINO_xx aliases in your sketch.
#include <Wire.h>
#include <Adafruit_ADS1015.h>


Adafruit_ADS1115 ads0(0x48);
Adafruit_ADS1115 ads1(0x49);
Adafruit_ADS1115 ads2(0x4A);
Adafruit_ADS1115 ads3(0x4B);

#define currentRelay CONTROLLINO_R0
#define groundRelay CONTROLLINO_R1

SdFat sd1;
const uint8_t SD1_CS = 53;  // chip select for sd1

SdFat sd2;
const uint8_t SD2_CS = 2;   // chip select for sd2

const uint8_t BUF_DIM = 100;
uint8_t buf[BUF_DIM];

const uint32_t FILE_SIZE = 1000000;
const uint16_t NWRITE = FILE_SIZE / BUF_DIM;

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

  //initializeSdCard();

  measuring = true;
  digitalWrite(currentRelay, HIGH);
  digitalWrite(groundRelay, HIGH);
  startMillis = millis();

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

//------------------------------------------------------------------------------

void loop () {
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
