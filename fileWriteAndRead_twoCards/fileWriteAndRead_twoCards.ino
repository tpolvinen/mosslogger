// Try this instead:
#include <SPI.h>
#include <SdFat.h>

#include <Controllino.h>

SdFat sd1;
const uint8_t SD1_CS = 53;  // chip select for sd1
SdFat sd2;
const uint8_t SD2_CS = 7;   // chip select for sd2 // CONTROLLINO_D5 = pin 7 on Arduino MEGA

SdFile file1;
SdFile file2;

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

uint8_t dirYear; // Controllino RTC library gives only two digits with Controllino_GetYear(), "2000 + thisYear" used in getDateAndTime()
uint8_t dirMonth; // = Controllino_GetMonth();

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
  sprintf(dirName, ("/%02d-%02d"), dirYear, dirMonth);
}

void sd1run() {
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
  Serial.println(F("-------sd1--------"));
  sd1.ls();
  Serial.println(F("------------------"));
  
  // make /dirName the default directory for sd1
  if (!sd1.chdir(dirName)) {
    sd1.errorExit("sd1.chdir");
  }

  //open file within Folder
  if (!file1.open(logfileName, O_RDWR | O_CREAT | O_AT_END)) {
    sd1.errorExit("open file1");
  }

  Serial.println(F("------dir1-------"));
  sd1.ls();
  Serial.println(F("------------------"));
  
  Serial.print("1 Writing to file1...");
  file1.println(logfileName);

  file1.close();
  Serial.println("1 done.");

  //reopen the files and read them

  file1.open (logfileName, O_READ);
  Serial.println("1 Reading file1");
  while (file1.available()) {
    Serial.write(file1.read());
  }
  
  file1.close();

  Serial.println("1 done, moving on...");

}

void sd2run() {

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
  Serial.println(F("-------sd2--------"));
  sd2.ls();
  Serial.println(F("------------------"));
  
  // make /dirName the default directory for sd2
  if (!sd2.chdir(dirName)) {
    sd2.errorExit("sd2.chdir");
  }

  //open file within Folder
  if (!file2.open(logfileName, O_RDWR | O_CREAT | O_AT_END)) {
    sd2.errorExit("open file2");
  }

  Serial.println(F("------dir2-------"));
  sd2.ls();
  Serial.println(F("------------------"));
  
  Serial.print("2 Writing to file2...");
  file2.println(logfileName);

  file2.close();
  Serial.println("2 done.");

  //reopen the files and read them

  file2.open (logfileName, O_READ);
  Serial.println("2 Reading file2");
  while (file2.available()) {
    Serial.write(file2.read());
  }
  
  file2.close();

  Serial.println("2 done, moving on...");
  
}

void setup() {

  Serial.begin(9600);
  // Wait for USB Serial
  delay(3000);

  Serial.println("Initializing SD cards...");
  //pinMode(SD2_CS, OUTPUT);
  digitalWrite(SD2_CS, HIGH);

  Controllino_RTC_init();
  getDateAndTime();
  getDirName();
  getLogFileName();
  Serial.println((char*)dateAndTimeData);

  int n;

  Serial.print("Date: "); n = Controllino_GetYear(); Serial.print(n);
  Serial.print("-"); n = Controllino_GetMonth(); Serial.print(n);
  Serial.print("-"); n = Controllino_GetDay(); Serial.println(n);
  Serial.print(" Time: "); n = Controllino_GetHour(); Serial.print(n);
  Serial.print(":"); n = Controllino_GetMinute(); Serial.print(n);
  Serial.print(":"); n = Controllino_GetSecond(); Serial.println(n);

  sd1run();

  //digitalWrite(SD1_CS, HIGH);
  //digitalWrite(SD2_CS, LOW);

  sd2run();

  sd1run();

  sd2run();
}

void loop()
{}
