

// This test file is a sandbox for testing two SD card functionality with my data logging code.

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

char dateAndTimeData[20]; // space for YYYY-MM-DDTHH-MM-SS, plus the null char terminator
char logfileName[10]; // space for MM-DD.csv, plus the null char terminator
char dirName[7]; // space for /YY-MM, plus the null char terminator

uint16_t thisYear; // Controllino RTC library gives only two digits with Controllino_GetYear(), "2000 + thisYear" used in getDateAndTime()
uint8_t thisMonth; // = Controllino_GetMonth();
uint8_t thisDay; // = Controllino_GetDay();
uint8_t thisHour; // = Controllino_GetHour();
uint8_t thisMinute; // = Controllino_GetMinute();
uint8_t thisSecond; // = Controllino_GetSecond();

uint8_t logMonth; // = Controllino_GetMonth();
uint8_t logDay; // = Controllino_GetDay();
//uint8_t logHour; // = Controllino_GetHour();
//uint8_t logMinute; // = Controllino_GetMinute();
//uint8_t logSecond; // = Controllino_GetSecond();

uint8_t dirYear; // Controllino RTC library gives only two digits with Controllino_GetYear(), "2000 + thisYear" used in getDateAndTime()
uint8_t dirMonth; // = Controllino_GetMonth();

SdFile logfile1;
SdFile logfile2;

// uint8_t previousLogFileDay = 0; // Stores the value of RTC day when logFile was started, i.e. current day.

unsigned long startShutDownPeriod = 0;

//const unsigned long measurementPeriod = 10000; // in milliseconds, how long to keep making measurement rounds
const unsigned long shutDownPeriod = 1000; // in milliseconds, how long to power off ADCs between measurement periods

const unsigned long measurementRoundPeriod = 1000; //  in milliseconds, how long to loop through ADCs reading values in, before calculating the averages and writing the new data to the file on SD card.

//------------------------------------------------------------------------------
// print error msg, any SD error codes, and halt.
// store messages in flash
#define errorExit(msg) errorHalt(F(msg))
#define initError(msg) initErrorHalt(F(msg))
//------------------------------------------------------------------------------

void getDateAndTime() {
  thisYear = 2000 + Controllino_GetYear();
  thisMonth = Controllino_GetMonth();
  thisDay = Controllino_GetDay();
  thisHour = Controllino_GetHour();
  thisMinute = Controllino_GetMinute();
  thisSecond = Controllino_GetSecond();

  sprintf(dateAndTimeData, ("%04d-%02d-%02dT%02d:%02d:%02d"), thisYear, thisMonth, thisDay, thisHour, thisMinute, thisSecond);
}

void getLogFileName() {
  logMonth = Controllino_GetMonth();
  logDay = Controllino_GetDay();
  sprintf(logfileName, ("%02d-%02d.csv"), logMonth, logDay);
}

void getDirName() {
  dirYear = Controllino_GetYear();
  dirMonth = Controllino_GetMonth();
  sprintf(dirName, ("%02d-%02d"), dirYear, dirMonth);
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

  getDirName();
  getLogFileName();
  getDateAndTime();

  // disable sd2 while initializing sd1
  digitalWrite(SD2_CS, HIGH);

  // initialize the first card
  if (!sd1.begin(SD1_CS)) {
    sd1.initError("sd1.initialize");
  }

  // create folder dirName on sd1 if it does not exist
  if (!sd1.exists(dirName)) {
    if (!sd1.mkdir(dirName)) {
      sd1.errorExit("sd1.mkdir");
    }
  }

  // initialize the second card
  if (!sd2.begin(SD2_CS)) {
    sd2.initError("sd2.initialize");
  }

  // create folder dirName on sd2 if it does not exist
  if (!sd2.exists(dirName)) {
    if (!sd2.mkdir(dirName)) {
      sd2.errorExit("sd2.mkdir");
    }
  }
  // list root directory on both cards
  Serial.println(F("------sd1 root-------"));
  sd1.ls();
  Serial.println(F("------sd2 root-------"));
  sd2.ls();

  // make /dirName the default directory for sd1
  if (!sd1.chdir(dirName)) {
    sd1.errorExit("sd1.chdir");
  }

  // make /dirName the default directory for sd2
  if (!sd2.chdir(dirName)) {
    sd2.errorExit("sd2.chdir");
  }

  // list current directory on both cards
  Serial.println(F("----sd1 dirName------"));
  sd1.ls();
  Serial.println(F("----sd2 dirName------"));
  sd2.ls();
  Serial.println(F("---------------------"));

  // set the current working directory for open() to sd1
  sd1.chvol();
  // create or open /dirName/test1.bin and go to it's end
  SdFile testfile1;
  if (!testfile1.open("test1.txt", O_RDWR | O_CREAT | O_AT_END)) {
    sd1.errorExit("testfile1");
  }




  // write data to /dirName/test1.bin on sd1
  if ((testfile1.print("Testing file write on ")) && (testfile1.println(dateAndTimeData))) {
    sd1.errorExit("sd1.testwrite");
  }

  // set the current working directory for open() to sd2
  sd2.chvol();
  // create or open /dirName/test2.bin and go to it's end
  SdFile testfile2;
  if (!testfile2.open("test2.bin", O_RDWR | O_CREAT | O_AT_END)) {
    sd2.errorExit("testfile2");
  }
  // write data to /testDir2/test2.bin on sd1
  if ((testfile1.print("Testing file write on ")) && (testfile1.println(dateAndTimeData))) {
    sd2.errorExit("sd2.testwrite");
  }

  testfile1.close();
  testfile2.close();

  Serial.println("Testfiles written and closed.");
  // list current directory on both cards
  Serial.println(F("------sd1 -------"));
  sd1.ls("/", LS_R | LS_DATE | LS_SIZE);
  Serial.println(F("------sd2 -------"));
  sd2.ls("/", LS_R | LS_DATE | LS_SIZE);
  Serial.println(F("---------------------"));


  //  // make dirName the default directory for sd1
  //  if (!sd1.chdir(dirName)) {
  //    sd1.errorExit("sd1.chdir");
  //  }
  //
  //  // make dirName the default directory for sd2
  //  if (!sd2.chdir(dirName)) {
  //    sd2.errorExit("sd2.chdir");
  //  }
}

void initializeSdCard1() {
  getDirName();
  getLogFileName();
  getDateAndTime();

  // disable sd2 while initializing sd1
  digitalWrite(SD2_CS, HIGH);

  if (!sd1.begin(SD1_CS)) {
    sd1.initError("sd1.initialize");
    return;
  }
  Serial.println("initialization 1 done.");


  if (!sd1.exists(dirName)) {
    if (!sd1.mkdir(dirName)) {
      sd1.errorExit("sd1.mkdir");
    }
  }
  // make /dirName the default directory for sd1
  if (!sd1.chdir(dirName)) {
    sd1.errorExit("sd1.chdir");
  }

  // create or open /dirName/test1.bin and go to it's end
  SdFile testfile1;
  if (!testfile1.open("test1.txt", O_RDWR | O_CREAT | O_AT_END)) {
    sd1.errorExit("testfile1");
  }

  // write data to /dirName/test1.bin on sd1
  if ((testfile1.print("Testing file write on ")) && (testfile1.println(dateAndTimeData))) {
    sd1.errorExit("sd1.testwrite");
  }

  testfile1.close();

  Serial.println("Testfile 1 written and closed.");
  // list current directory on both cards
  Serial.println(F("------sd1 -------"));
  sd1.ls("/", LS_R | LS_DATE | LS_SIZE);
  Serial.println(F("------sd2 -------"));
  sd2.ls("/", LS_R | LS_DATE | LS_SIZE);
  Serial.println(F("---------------------"));
}

void initializeSdCard2() {

  // disable sd2 while initializing sd2
  digitalWrite(SD1_CS, HIGH);

  if (!sd2.begin(SD2_CS)) {
    sd2.initError("sd2.initialize");
    return;
  }
  Serial.println("initialization 2 done.");


  if (!sd2.exists(dirName)) {
    if (!sd2.mkdir(dirName)) {
      sd2.errorExit("sd2.mkdir");
    }
  }
  // make /dirName the default directory for sd2
  if (!sd2.chdir(dirName)) {
    sd2.errorExit("sd2.chdir");
  }

  // create or open /dirName/test2.bin and go to it's end
  SdFile testfile2;
  if (!testfile2.open("test2.txt", O_RDWR | O_CREAT | O_AT_END)) {
    sd2.errorExit("testfile2");
  }

  // write data to /dirName/test2.bin on sd2
  if ((testfile2.print("Testing file write on ")) && (testfile2.println(dateAndTimeData))) {
    sd2.errorExit("sd2.testwrite");
  }

  testfile2.close();

  Serial.println("Testfile 2 written and closed.");
  // list current directory on both cards
  Serial.println(F("------sd1 -------"));
  sd1.ls("/", LS_R | LS_DATE | LS_SIZE);
  Serial.println(F("------sd2 -------"));
  sd2.ls("/", LS_R | LS_DATE | LS_SIZE);
  Serial.println(F("---------------------"));
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

  getDateAndTime(); //store date and time before taking measurements/reading ACDs, will be printed to logfile along the values measured

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

  // set the current working directory for open() to sd1
  sd1.chvol();
  // create or open /dirName/logfile1 and go to it's end
  if (!logfile1.open(logfileName, O_RDWR | O_CREAT | O_AT_END)) {
    sd1.errorExit("logfile1 opening");
  }

  if (!
      (logfile1.print(dateAndTimeData))

      && (logfile1.print(","))

      && (logfile1.print(measurementRoundAverage00))
      && (logfile1.print(","))
      && (logfile1.print(measurementRoundAverage01))
      && (logfile1.print(","))
      && (logfile1.print(measurementRoundAverage02))
      && (logfile1.print(","))
      && (logfile1.print(measurementRoundAverage03))
      && (logfile1.print(","))

      && (logfile1.print(measurementRoundAverage10))
      && (logfile1.print(","))
      && (logfile1.print(measurementRoundAverage11))
      && (logfile1.print(","))
      && (logfile1.print(measurementRoundAverage12))
      && (logfile1.print(","))
      && (logfile1.print(measurementRoundAverage13))
      && (logfile1.print(","))

      && (logfile1.print(measurementRoundAverage20))
      && (logfile1.print(","))
      && (logfile1.print(measurementRoundAverage21))
      && (logfile1.print(","))
      && (logfile1.print(measurementRoundAverage22))
      && (logfile1.print(","))
      && (logfile1.print(measurementRoundAverage23))
      && (logfile1.print(","))

      && (logfile1.print(measurementRoundAverage30))
      && (logfile1.print(","))
      && (logfile1.print(measurementRoundAverage31))
      && (logfile1.print(","))
      && (logfile1.print(measurementRoundAverage32))
      && (logfile1.print(","))
      && (logfile1.println(measurementRoundAverage33))
     ) {
    sd1.errorExit("logfile1 writing");
  }

  logfile1.close();

  // set the current working directory for open() to sd2
  sd2.chvol();
  // create or open /dirName/logfile2 and go to it's end
  if (!logfile2.open(logfileName, O_RDWR | O_CREAT | O_AT_END)) {
    sd2.errorExit("logfile2 opening");
  }
  if (!
      (logfile2.print(dateAndTimeData))

      && (logfile2.print(","))

      && (logfile2.print(measurementRoundAverage00))
      && (logfile2.print(","))
      && (logfile2.print(measurementRoundAverage01))
      && (logfile2.print(","))
      && (logfile2.print(measurementRoundAverage02))
      && (logfile2.print(","))
      && (logfile2.print(measurementRoundAverage03))
      && (logfile2.print(","))

      && (logfile2.print(measurementRoundAverage10))
      && (logfile2.print(","))
      && (logfile2.print(measurementRoundAverage11))
      && (logfile2.print(","))
      && (logfile2.print(measurementRoundAverage12))
      && (logfile2.print(","))
      && (logfile2.print(measurementRoundAverage13))
      && (logfile2.print(","))

      && (logfile2.print(measurementRoundAverage20))
      && (logfile2.print(","))
      && (logfile2.print(measurementRoundAverage21))
      && (logfile2.print(","))
      && (logfile2.print(measurementRoundAverage22))
      && (logfile2.print(","))
      && (logfile2.print(measurementRoundAverage23))
      && (logfile2.print(","))

      && (logfile2.print(measurementRoundAverage30))
      && (logfile2.print(","))
      && (logfile2.print(measurementRoundAverage31))
      && (logfile2.print(","))
      && (logfile2.print(measurementRoundAverage32))
      && (logfile2.print(","))
      && (logfile2.println(measurementRoundAverage33))
     ) {
    sd2.errorExit("logfile2 writing");
  }

  logfile2.close();
}

void setup() {

  pinMode(SD2_CS, OUTPUT);
  // RTC goes bonkers if SD2 is available
  digitalWrite(SD2_CS, HIGH);

  Serial.begin(9600);

  // Wait for USB Serial
  while (!Serial) {
    SysCall::yield();
  }

  Serial.println();
  Serial.println();

  Serial.println(F("twocards_test.ino"));

  pinMode(currentRelay, OUTPUT);
  pinMode(groundRelay, OUTPUT);

  Controllino_RTC_init();
  getDateAndTime();
  Serial.println((char*)dateAndTimeData);

  digitalWrite(currentRelay, HIGH);
  digitalWrite(groundRelay, HIGH);
  delay(250); // allows time for ACDs to start

  initializeADCs();
  initializeSdCards();

//  getDirName();
//  getLogFileName();
//  getDateAndTime();
//  initializeSdCard1();
//  initializeSdCard2();

  //measurementRound();

  startShutDownPeriod = millis();

  digitalWrite(currentRelay, LOW);
  digitalWrite(groundRelay, LOW);
}

void loop() {


  if (millis() - startShutDownPeriod <= shutDownPeriod) {
    digitalWrite(currentRelay, HIGH);
    digitalWrite(groundRelay, HIGH);
    delay(250); // allows time for ACDs to start

    //measurementRound();

    startShutDownPeriod = millis();

    digitalWrite(currentRelay, LOW);
    digitalWrite(groundRelay, LOW);
  }
}


// Try this instead:
//#include <SPI.h>
//#include <SdFat.h>
//
//const byte chipSelect1 = 53;
//const byte chipSelect2 = 7;
//SdFat sd1;
//SdFat sd2;
//SdFile file1;
//SdFile file2;
//
//void setup()
//{
//  Serial.begin(9600);
//
//  Serial.print("Initializing SD card 1...");
//  //pinMode(53, OUTPUT);
//  pinMode(7, OUTPUT);
//  digitalWrite(7, HIGH);
//
//  sd1run();
//
//  sd2run();
//}
//
//void sd1run() {
//  if (!sd1.begin(53)) {
//    Serial.println("initialization 1 failed!");
//    return;
//  }
//  Serial.println("initialization 1 done.");
//
//  //make Folders
//  sd1.mkdir("Folders");
//  sd1.chdir("/Folders");
//  sd1.mkdir("Folder_A");
//  sd1.mkdir("Folder_B");
//
//  sd1.chdir("/Folders/Folder_A");
//
//  //open file within Folder
//  file1.open ("testA.txt", O_RDWR | O_CREAT | O_AT_END);
//  Serial.print("1 Writing to Folder_A file testA.txt...");
//  file1.println("1 testing file write to Folder_A");
//  // close the file:
//  file1.close();
//  Serial.println("1 done.");
//
//  sd1.chdir("/Folders/Folder_B");
//
//  file1.open("testB.txt", O_RDWR | O_CREAT | O_AT_END);
//  Serial.print("1 Writing to Folder_B file testB.txt...");
//  file1.println("1 testing file write to Folder_B");
//  // close the file:
//  file1.close();
//  Serial.println("1 done, reopening...");
//  Serial.println();
//
//  //reopen the files and read them
//
//  sd1.chdir("/Folders/Folder_A");
//  file1.open ("testA.txt", O_READ);
//  Serial.println("1 Reading testA.txt");
//  while (file1.available()) {
//    Serial.write(file1.read());
//  }
//  // close the file1:
//  Serial.println();
//  file1.close();
//
//  sd1.chdir("/Folders/Folder_B");
//  file1.open("testB.txt", O_READ);
//  Serial.println("1 Reading testB.txt");
//  while (file1.available()) {
//    Serial.write(file1.read());
//  }
//  // close the file:
//  Serial.println("1 done for real.");
//  file1.close();
//}
//
//void sd2run() {
//  if (!sd2.begin(53)) {
//    Serial.println("initialization 2 failed!");
//    return;
//  }
//  Serial.println("initialization 2 done.");
//
//  //make Folders
//  sd2.mkdir("Folders");
//  sd2.chdir("/Folders");
//  sd2.mkdir("Folder_A");
//  sd2.mkdir("Folder_B");
//
//  sd2.chdir("/Folders/Folder_A");
//
//  //open file within Folder
//  file2.open ("testA.txt", O_RDWR | O_CREAT | O_AT_END);
//  Serial.print("2 Writing to Folder_A file testA.txt...");
//  file2.println("2 testing file write to Folder_A");
//  // close the file:
//  file2.close();
//  Serial.println("2 done.");
//
//  sd2.chdir("/Folders/Folder_B");
//
//  file2.open("testB.txt", O_RDWR | O_CREAT | O_AT_END);
//  Serial.print("2 Writing to Folder_B file testB.txt...");
//  file2.println("2 testing file write to Folder_B");
//  // close the file:
//  file2.close();
//  Serial.println("2 done, reopening...");
//  Serial.println();
//
//  //reopen the files and read them
//
//  sd2.chdir("/Folders/Folder_A");
//  file2.open ("testA.txt", O_READ);
//  Serial.println("2 Reading testA.txt");
//  while (file2.available()) {
//    Serial.write(file2.read());
//  }
//  // close the file2:
//  Serial.println();
//  file2.close();
//
//  sd2.chdir("/Folders/Folder_B");
//  file2.open("testB.txt", O_READ);
//  Serial.println("2 Reading testB.txt");
//  while (file2.available()) {
//    Serial.write(file2.read());
//  }
//  // close the file:
//  Serial.println("2 done for real.");
//  file2.close();
//}
//
//void loop()
//{}
