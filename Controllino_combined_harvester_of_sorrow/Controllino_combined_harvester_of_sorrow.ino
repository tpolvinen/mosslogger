

// This test file is a sandbox for testing combined ADC timing and two SD card data saving


#include <SPI.h>
#include <SdFat.h>
#include <Controllino.h> // Usage of CONTROLLINO library allows you to use CONTROLLINO_xx aliases in your sketch.
#include <Wire.h>
#include <Adafruit_ADS1015.h>

//------------------------------------------------------------------------------

Adafruit_ADS1115 ads0(0x48);
Adafruit_ADS1115 ads1(0x49);
Adafruit_ADS1115 ads2(0x4A);
Adafruit_ADS1115 ads3(0x4B);

// these control power to ACDs (ADS1115s), need to shut down to avoid heating the samples with thermistors
// NOTE: for some reason, using just one relay to cut current causes the Controllino/code to get stuck on first reading in measurementRound()
#define currentRelay CONTROLLINO_R0
#define groundRelay CONTROLLINO_R1

unsigned long startShutDownPeriod = 0;

const unsigned long shutDownPeriod = 4000; // in milliseconds, how long to power off ADCs between measurement periods

const unsigned long measurementRoundPeriod = 1000; //  in milliseconds, how long to loop through ADCs reading values in, before calculating the averages

int16_t measurementRoundCounter = 0;

long measurementRoundAverage00 = 0, measurementRoundAverage01 = 0, measurementRoundAverage02 = 0, measurementRoundAverage03 = 0;
long measurementRoundAverage10 = 0, measurementRoundAverage11 = 0, measurementRoundAverage12 = 0, measurementRoundAverage13 = 0;
long measurementRoundAverage20 = 0, measurementRoundAverage21 = 0, measurementRoundAverage22 = 0, measurementRoundAverage23 = 0;
long measurementRoundAverage30 = 0, measurementRoundAverage31 = 0, measurementRoundAverage32 = 0, measurementRoundAverage33 = 0;

//------------------------------------------------------------------------------

SdFat sd1;
const uint8_t SD1_CS = 53;  // chip select for sd1
SdFat sd2;
const uint8_t SD2_CS = 7;   // chip select for sd2 // CONTROLLINO_D5 = pin 7 on Arduino MEGA

SdFile measurementfile1;
SdFile measurementfile2;

char dateAndTimeData[20]; // space for YYYY-MM-DDTHH-MM-SS, plus the null char terminator
char measurementfileName[10]; // space for MM-DD.csv, plus the null char terminator
char dirName[7]; // space for /YY-MM, plus the null char terminator

uint16_t thisYear; // Controllino RTC library gives only two digits with Controllino_GetYear(), "2000 + thisYear" used in getDateAndTime()
uint8_t thisMonth; // = Controllino_GetMonth();
uint8_t thisDay; // = Controllino_GetDay();
uint8_t thisHour; // = Controllino_GetHour();
uint8_t thisMinute; // = Controllino_GetMinute();
uint8_t thisSecond; // = Controllino_GetSecond();

//------------------------------------------------------------------------------
// print error msg, any SD error codes, and halt.
// store messages in flash
#define errorExit(msg) errorHalt(F(msg))
#define initError(msg) initErrorHalt(F(msg))
//------------------------------------------------------------------------------

void initializeADCs() {
  ads0.setGain(GAIN_TWOTHIRDS);
  ads1.setGain(GAIN_TWOTHIRDS);
  ads2.setGain(GAIN_TWOTHIRDS);
  ads3.setGain(GAIN_TWOTHIRDS);

  ads0.begin();
  ads1.begin();
  ads2.begin();
  ads3.begin();
}

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

//------------------------------------------------------------------------------

void getMeasurementfileName() {
  thisMonth = Controllino_GetMonth();
  thisDay = Controllino_GetDay();
  sprintf(measurementfileName, ("%02d-%02d.csv"), thisMonth, thisDay);
}

void getDirName() {
  thisYear = Controllino_GetYear();
  thisMonth = Controllino_GetMonth();
  sprintf(dirName, ("/%02d-%02d"), thisYear, thisMonth);
}

//------------------------------------------------------------------------------

void measurements() {
  unsigned long measurementRoundStartMillis = 0;
  measurementRoundCounter = 0; // moved to global

  measurementRoundAverage00 = 0, measurementRoundAverage01 = 0, measurementRoundAverage02 = 0, measurementRoundAverage03 = 0;
  measurementRoundAverage10 = 0, measurementRoundAverage11 = 0, measurementRoundAverage12 = 0, measurementRoundAverage13 = 0;
  measurementRoundAverage20 = 0, measurementRoundAverage21 = 0, measurementRoundAverage22 = 0, measurementRoundAverage23 = 0;
  measurementRoundAverage30 = 0, measurementRoundAverage31 = 0, measurementRoundAverage32 = 0, measurementRoundAverage33 = 0;

  int16_t measurement00, measurement01, measurement02, measurement03;
  int16_t measurement10, measurement11, measurement12, measurement13;
  int16_t measurement20, measurement21, measurement22, measurement23;
  int16_t measurement30, measurement31, measurement32, measurement33;

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
}

//------------------------------------------------------------------------------

void sd1write() {

  if (!sd1.begin(SD1_CS)) {
    sd1.initError("sd1.initialize");
    return;
  }
  if (!sd1.exists(dirName)) {
    if (!sd1.mkdir(dirName)) {
      sd1.errorExit("sd1.mkdir");
    }
  }

  // make /dirName the default directory for sd1
  if (!sd1.chdir(dirName)) {
    sd1.errorExit("sd1.chdir");
  }

  //open file within Folder
  if (!measurementfile1.open(measurementfileName, O_RDWR | O_CREAT | O_AT_END)) {
    sd1.errorExit("open measurementfile1");
  }

  if (! (measurementfile1.print(dateAndTimeData)) ) {
    sd1.errorExit("measurementfile1 writing");
  }

  if (! (measurementfile1.print(",")) ) {
    sd1.errorExit("measurementfile1 writing");
  }

  if (! (measurementfile1.print(measurementRoundAverage00)) ) {
    sd1.errorExit("measurementfile1 writing");
  }
  if (! (measurementfile1.print(",")) ) {
    sd1.errorExit("measurementfile1 writing");
  }
  if (! (measurementfile1.print(measurementRoundAverage01)) ) {
    sd1.errorExit("measurementfile1 writing");
  }
  if (! (measurementfile1.print(",")) ) {
    sd1.errorExit("measurementfile1 writing");
  }
  if (! (measurementfile1.print(measurementRoundAverage01)) ) {
    sd1.errorExit("measurementfile1 writing");
  }
  if (! (measurementfile1.print(",")) ) {
    sd1.errorExit("measurementfile1 writing");
  }
  if (! (measurementfile1.print(measurementRoundAverage03)) ) {
    sd1.errorExit("measurementfile1 writing");
  }
  if (! (measurementfile1.print(",")) ) {
    sd1.errorExit("measurementfile1 writing");
  }

  if (! (measurementfile1.print(measurementRoundAverage10)) ) {
    sd1.errorExit("measurementfile1 writing");
  }
  if (! (measurementfile1.print(",")) ) {
    sd1.errorExit("measurementfile1 writing");
  }
  if (! (measurementfile1.print(measurementRoundAverage11)) ) {
    sd1.errorExit("measurementfile1 writing");
  }
  if (! (measurementfile1.print(",")) ) {
    sd1.errorExit("measurementfile1 writing");
  }
  if (! (measurementfile1.print(measurementRoundAverage11)) ) {
    sd1.errorExit("measurementfile1 writing");
  }
  if (! (measurementfile1.print(",")) ) {
    sd1.errorExit("measurementfile1 writing");
  }
  if (! (measurementfile1.print(measurementRoundAverage13)) ) {
    sd1.errorExit("measurementfile1 writing");
  }
  if (! (measurementfile1.print(",")) ) {
    sd1.errorExit("measurementfile1 writing");
  }

  if (! (measurementfile1.print(measurementRoundAverage10)) ) {
    sd1.errorExit("measurementfile1 writing");
  }
  if (! (measurementfile1.print(",")) ) {
    sd1.errorExit("measurementfile1 writing");
  }
  if (! (measurementfile1.print(measurementRoundAverage11)) ) {
    sd1.errorExit("measurementfile1 writing");
  }
  if (! (measurementfile1.print(",")) ) {
    sd1.errorExit("measurementfile1 writing");
  }
  if (! (measurementfile1.print(measurementRoundAverage11)) ) {
    sd1.errorExit("measurementfile1 writing");
  }
  if (! (measurementfile1.print(",")) ) {
    sd1.errorExit("measurementfile1 writing");
  }
  if (! (measurementfile1.print(measurementRoundAverage13)) ) {
    sd1.errorExit("measurementfile1 writing");
  }
  if (! (measurementfile1.print(",")) ) {
    sd1.errorExit("measurementfile1 writing");
  }

  if (! (measurementfile1.print(measurementRoundAverage30)) ) {
    sd1.errorExit("measurementfile1 writing");
  }
  if (! (measurementfile1.print(",")) ) {
    sd1.errorExit("measurementfile1 writing");
  }
  if (! (measurementfile1.print(measurementRoundAverage31)) ) {
    sd1.errorExit("measurementfile1 writing");
  }
  if (! (measurementfile1.print(",")) ) {
    sd1.errorExit("measurementfile1 writing");
  }
  if (! (measurementfile1.print(measurementRoundAverage31)) ) {
    sd1.errorExit("measurementfile1 writing");
  }
  if (! (measurementfile1.print(",")) ) {
    sd1.errorExit("measurementfile1 writing");
  }
  if (! (measurementfile1.println(measurementRoundAverage33)) ) {
    sd1.errorExit("measurementfile1 writing");
  }

  measurementfile1.close();

}

//------------------------------------------------------------------------------

void sd2write() {

  if (!sd2.begin(SD2_CS)) {
    sd2.initError("sd2.initialize");
    return;
  }

  if (!sd2.exists(dirName)) {
    if (!sd2.mkdir(dirName)) {
      sd2.errorExit("sd2.mkdir");
    }
  }

  // make /dirName the default directory for sd2
  if (!sd2.chdir(dirName)) {
    sd2.errorExit("sd2.chdir");
  }

  //open file within Folder
  if (!measurementfile2.open(measurementfileName, O_RDWR | O_CREAT | O_AT_END)) {
    sd2.errorExit("open measurementfile2");
  }

  if (! (measurementfile2.print(dateAndTimeData)) ) {
    sd2.errorExit("measurementfile2 writing");
  }

  if (! (measurementfile2.print(",")) ) {
    sd2.errorExit("measurementfile2 writing");
  }

  if (! (measurementfile2.print(measurementRoundAverage00)) ) {
    sd2.errorExit("measurementfile2 writing");
  }
  if (! (measurementfile2.print(",")) ) {
    sd2.errorExit("measurementfile2 writing");
  }
  if (! (measurementfile2.print(measurementRoundAverage01)) ) {
    sd2.errorExit("measurementfile2 writing");
  }
  if (! (measurementfile2.print(",")) ) {
    sd2.errorExit("measurementfile2 writing");
  }
  if (! (measurementfile2.print(measurementRoundAverage02)) ) {
    sd2.errorExit("measurementfile2 writing");
  }
  if (! (measurementfile2.print(",")) ) {
    sd2.errorExit("measurementfile2 writing");
  }
  if (! (measurementfile2.print(measurementRoundAverage03)) ) {
    sd2.errorExit("measurementfile2 writing");
  }
  if (! (measurementfile2.print(",")) ) {
    sd2.errorExit("measurementfile2 writing");
  }

  if (! (measurementfile2.print(measurementRoundAverage10)) ) {
    sd2.errorExit("measurementfile2 writing");
  }
  if (! (measurementfile2.print(",")) ) {
    sd2.errorExit("measurementfile2 writing");
  }
  if (! (measurementfile2.print(measurementRoundAverage11)) ) {
    sd2.errorExit("measurementfile2 writing");
  }
  if (! (measurementfile2.print(",")) ) {
    sd2.errorExit("measurementfile2 writing");
  }
  if (! (measurementfile2.print(measurementRoundAverage12)) ) {
    sd2.errorExit("measurementfile2 writing");
  }
  if (! (measurementfile2.print(",")) ) {
    sd2.errorExit("measurementfile2 writing");
  }
  if (! (measurementfile2.print(measurementRoundAverage13)) ) {
    sd2.errorExit("measurementfile2 writing");
  }
  if (! (measurementfile2.print(",")) ) {
    sd2.errorExit("measurementfile2 writing");
  }

  if (! (measurementfile2.print(measurementRoundAverage20)) ) {
    sd2.errorExit("measurementfile2 writing");
  }
  if (! (measurementfile2.print(",")) ) {
    sd2.errorExit("measurementfile2 writing");
  }
  if (! (measurementfile2.print(measurementRoundAverage21)) ) {
    sd2.errorExit("measurementfile2 writing");
  }
  if (! (measurementfile2.print(",")) ) {
    sd2.errorExit("measurementfile2 writing");
  }
  if (! (measurementfile2.print(measurementRoundAverage22)) ) {
    sd2.errorExit("measurementfile2 writing");
  }
  if (! (measurementfile2.print(",")) ) {
    sd2.errorExit("measurementfile2 writing");
  }
  if (! (measurementfile2.print(measurementRoundAverage23)) ) {
    sd2.errorExit("measurementfile2 writing");
  }
  if (! (measurementfile2.print(",")) ) {
    sd2.errorExit("measurementfile2 writing");
  }

  if (! (measurementfile2.print(measurementRoundAverage30)) ) {
    sd2.errorExit("measurementfile2 writing");
  }
  if (! (measurementfile2.print(",")) ) {
    sd2.errorExit("measurementfile2 writing");
  }
  if (! (measurementfile2.print(measurementRoundAverage31)) ) {
    sd2.errorExit("measurementfile2 writing");
  }
  if (! (measurementfile2.print(",")) ) {
    sd2.errorExit("measurementfile2 writing");
  }
  if (! (measurementfile2.print(measurementRoundAverage32)) ) {
    sd2.errorExit("measurementfile2 writing");
  }
  if (! (measurementfile2.print(",")) ) {
    sd2.errorExit("measurementfile2 writing");
  }
  if (! (measurementfile2.println(measurementRoundAverage33)) ) {
    sd2.errorExit("measurementfile2 writing");
  }

  measurementfile2.close();

}

//------------------------------------------------------------------------------

void setup() {
  
  Serial.begin(9600);

  // Wait for USB Serial
  while (!Serial) {
    delay(10);
  }

  pinMode(currentRelay, OUTPUT);
  pinMode(groundRelay, OUTPUT);

  digitalWrite(SD2_CS, HIGH);

  Controllino_RTC_init();
  getDateAndTime();
  getDirName();
  getMeasurementfileName();
  Serial.print("dateAndTimeData char array: "); Serial.println((char*)dateAndTimeData);

  int n;

  Serial.print("Date: "); n = Controllino_GetYear(); Serial.print(n);
  Serial.print("-"); n = Controllino_GetMonth(); Serial.print(n);
  Serial.print("-"); n = Controllino_GetDay(); Serial.println(n);
  Serial.print("Time: "); n = Controllino_GetHour(); Serial.print(n);
  Serial.print(":"); n = Controllino_GetMinute(); Serial.print(n);
  Serial.print(":"); n = Controllino_GetSecond(); Serial.println(n);

  digitalWrite(currentRelay, HIGH);
  digitalWrite(groundRelay, HIGH);
  delay(250); // allows time for ACDs to start

  initializeADCs();

  startShutDownPeriod = millis() - shutDownPeriod + 1000;

  digitalWrite(currentRelay, LOW);
  digitalWrite(groundRelay, LOW);

}

//------------------------------------------------------------------------------

void loop() {

  while (millis() - startShutDownPeriod >= shutDownPeriod) {

    getDateAndTime();
    getDirName();
    getMeasurementfileName();

    digitalWrite(currentRelay, HIGH);
    digitalWrite(groundRelay, HIGH);
    delay(250); // allows time for ACDs to start

    measurements();

    startShutDownPeriod = millis();

    digitalWrite(currentRelay, LOW);
    digitalWrite(groundRelay, LOW);

    sd1write();
    sd2write();

    Serial.println("Measurements done.");
  }
}
