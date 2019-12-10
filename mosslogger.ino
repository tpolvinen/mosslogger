
#include <SPI.h>
#include <SdFat.h>
#include <Controllino.h> // Usage of CONTROLLINO library allows you to use CONTROLLINO_xx aliases in your sketch.
#include <Wire.h>
#include <Adafruit_ADS1015.h>

#include <avr/wdt.h>

//------------------------------------------------------------------------------

Adafruit_ADS1115 ads0(0x48);
Adafruit_ADS1115 ads1(0x49);
Adafruit_ADS1115 ads2(0x4A);
Adafruit_ADS1115 ads3(0x4B);

// these control ground connection to voltage dividers in adc channels (thermistors)
#define ads0Relay CONTROLLINO_R6
#define ads1Relay CONTROLLINO_R7
#define ads2Relay CONTROLLINO_R8
#define ads3Relay CONTROLLINO_R9

unsigned long startGetDateAndTimeInterval = 0; // to mark the start of current getDateAndTimeInterval
const unsigned long getDateAndTimeInterval = 1000; // in milliseconds, interval between dateAndTimeData refreshs with getDateAndTime()

unsigned long startRelayTimeBuffer = 0; // to mark the start of current relayTimeBuffer
const unsigned long relayTimeBuffer = 20; // in milliseconds, interval between turning relays on and starting measurements(), in effect giving ADCs time to start

//------------------------------------------------------------------------------

unsigned long startShutDownPeriod = 0; // to mark the start of current shutDownPeriod
const unsigned long shutDownPeriod = 5000; // in milliseconds, how long to power off ADCs between measurement periods

unsigned long startMeasurementPeriod = 0; // to mark the start of current measurementPeriod
const unsigned long measurementPeriod = 10000; // in milliseconds, how long to keep measuring, looping measurement rounds

const unsigned long measurementRoundPeriod = 1000; //  in milliseconds, how long to loop through ADCs reading values in, before calculating the averages

//------------------------------------------------------------------------------

unsigned long startsdCardInitializeDelay = 0; // to mark the start of current sdCardInitializeDelay
const int16_t sdCardInitializeDelay = 200; // in milliseconds, interval between attempts to read sd card if removed - remember watchdog timer settings!


int16_t measurementRoundCounter = 0;

signed long measurementRoundAverage00 = 0, measurementRoundAverage01 = 0, measurementRoundAverage02 = 0, measurementRoundAverage03 = 0;
signed long measurementRoundAverage10 = 0, measurementRoundAverage11 = 0, measurementRoundAverage12 = 0, measurementRoundAverage13 = 0;
signed long measurementRoundAverage20 = 0, measurementRoundAverage21 = 0, measurementRoundAverage22 = 0, measurementRoundAverage23 = 0;
signed long measurementRoundAverage30 = 0, measurementRoundAverage31 = 0, measurementRoundAverage32 = 0, measurementRoundAverage33 = 0;

float measurementRoundVoltage00 = 0.0, measurementRoundVoltage01 = 0.0, measurementRoundVoltage02 = 0.0, measurementRoundVoltage03 = 0.0;
float measurementRoundVoltage10 = 0.0, measurementRoundVoltage11 = 0.0, measurementRoundVoltage12 = 0.0, measurementRoundVoltage13 = 0.0;
float measurementRoundVoltage20 = 0.0, measurementRoundVoltage21 = 0.0, measurementRoundVoltage22 = 0.0, measurementRoundVoltage23 = 0.0;
float measurementRoundVoltage30 = 0.0, measurementRoundVoltage31 = 0.0, measurementRoundVoltage32 = 0.0, measurementRoundVoltage33 = 0.0;

float adcRange = 25970.8; //25974.4;//25967.16; //25959.96; //25956.36; //25952.76; //25945.52; //25865.87; //25960; // 2665.85; // 32767 / 6.144v * 5v = 2665.85

#define THERMISTORNOMINAL 10000  // resistance at 25 degrees C
#define TEMPERATURENOMINAL 25  // temp. for nominal resistance (almost always 25 C)
#define BCOEFFICIENT 3976  // The beta coefficient of the thermistor (usually 3000-4000)
#define SERIESRESISTOR 10000  // the value of the 'other' resistor

float measurementRoundTemperatureC00 = 0.0, measurementRoundTemperatureC01 = 0.0, measurementRoundTemperatureC02 = 0.0, measurementRoundTemperatureC03 = 0.0;
float measurementRoundTemperatureC10 = 0.0, measurementRoundTemperatureC11 = 0.0, measurementRoundTemperatureC12 = 0.0, measurementRoundTemperatureC13 = 0.0;
float measurementRoundTemperatureC20 = 0.0, measurementRoundTemperatureC21 = 0.0, measurementRoundTemperatureC22 = 0.0, measurementRoundTemperatureC23 = 0.0;
float measurementRoundTemperatureC30 = 0.0, measurementRoundTemperatureC31 = 0.0, measurementRoundTemperatureC32 = 0.0, measurementRoundTemperatureC33 = 0.0;

//------------------------------------------------------------------------------

SdFat sd1;
const uint8_t SD1_CS = 53;  // chip select for sd1
SdFat sd2;
const uint8_t SD2_CS = 7;   // chip select for sd2 // CONTROLLINO_D5 = pin 7 on Arduino MEGA

SdFile measurementfile1;
SdFile measurementfile2;
SdFile logfile1;
SdFile logfile2;

char logMsg[100];
char measurementfileHeader[132]; // space for YYYY-MM-DDThh:mm:ss,0-0,0-1,0-2,0-3,1-0,1-1,1-2,1-3,2-0,2-1,2-2,2-3,3-0,3-1,3-2,3-3, plus the null char terminator
char dateAndTimeData[20]; // space for YYYY-MM-DDTHH-MM-SS, plus the null char terminator
char measurementfileName[10]; // space for MM-DD.csv, plus the null char terminator
char logfileName[13]; // space for MM-DDlog.csv, plus the null char terminator
char dirName[7]; // space for /YY-MM, plus the null char terminator

uint16_t dateYear; // Controllino RTC library gives only two digits with Controllino_GetYear(), "2000 + thisYear" used in getDateAndTime()
int8_t thisYear; // = Controllino_GetYear();
int8_t thisMonth; // = Controllino_GetMonth();
int8_t thisDay; // = Controllino_GetDay();
int8_t thisHour; // = Controllino_GetHour();
int8_t thisMinute; // = Controllino_GetMinute();
int8_t thisSecond; // = Controllino_GetSecond();

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
  //Serial.println("begin getDateAndTime()");

  thisYear = Controllino_GetYear();
  thisMonth = Controllino_GetMonth();
  thisDay = Controllino_GetDay();
  thisHour = Controllino_GetHour();
  thisMinute = Controllino_GetMinute();
  thisSecond = Controllino_GetSecond();

  dateYear = thisYear + 2000;

  sprintf(dateAndTimeData, ("%04d-%02d-%02dT%02d:%02d:%02d"), dateYear, thisMonth, thisDay, thisHour, thisMinute, thisSecond);
  sprintf(measurementfileName, ("%02d-%02d.csv"), thisMonth, thisDay);
  sprintf(logfileName, ("%02d-%02dlog.csv"), thisMonth, thisDay);
  sprintf(dirName, ("/%02d-%02d"), thisYear, thisMonth);

}

//------------------------------------------------------------------------------

void measurements() {
  //  Serial.println("begin measurements()");

  wdt_reset();

  unsigned long measurementRoundStartMillis = 0;
  measurementRoundCounter = 0; // moved to global

  measurementRoundAverage00 = 0, measurementRoundAverage01 = 0, measurementRoundAverage02 = 0, measurementRoundAverage03 = 0;
  measurementRoundAverage10 = 0, measurementRoundAverage11 = 0, measurementRoundAverage12 = 0, measurementRoundAverage13 = 0;
  measurementRoundAverage20 = 0, measurementRoundAverage21 = 0, measurementRoundAverage22 = 0, measurementRoundAverage23 = 0;
  measurementRoundAverage30 = 0, measurementRoundAverage31 = 0, measurementRoundAverage32 = 0, measurementRoundAverage33 = 0;

  measurementRoundVoltage00 = 0.0, measurementRoundVoltage01 = 0.0, measurementRoundVoltage02 = 0.0, measurementRoundVoltage03 = 0.0;
  measurementRoundVoltage10 = 0.0, measurementRoundVoltage11 = 0.0, measurementRoundVoltage12 = 0.0, measurementRoundVoltage13 = 0.0;
  measurementRoundVoltage20 = 0.0, measurementRoundVoltage21 = 0.0, measurementRoundVoltage22 = 0.0, measurementRoundVoltage23 = 0.0;
  measurementRoundVoltage30 = 0.0, measurementRoundVoltage31 = 0.0, measurementRoundVoltage32 = 0.0, measurementRoundVoltage33 = 0.0;

  measurementRoundTemperatureC00 = 0.0, measurementRoundTemperatureC01 = 0.0, measurementRoundTemperatureC02 = 0.0, measurementRoundTemperatureC03 = 0.0;
  measurementRoundTemperatureC10 = 0.0, measurementRoundTemperatureC11 = 0.0, measurementRoundTemperatureC12 = 0.0, measurementRoundTemperatureC13 = 0.0;
  measurementRoundTemperatureC20 = 0.0, measurementRoundTemperatureC21 = 0.0, measurementRoundTemperatureC22 = 0.0, measurementRoundTemperatureC23 = 0.0;
  measurementRoundTemperatureC30 = 0.0, measurementRoundTemperatureC31 = 0.0, measurementRoundTemperatureC32 = 0.0, measurementRoundTemperatureC33 = 0.0;

  int16_t measurement00, measurement01, measurement02, measurement03;
  int16_t measurement10, measurement11, measurement12, measurement13;
  int16_t measurement20, measurement21, measurement22, measurement23;
  int16_t measurement30, measurement31, measurement32, measurement33;

  measurementRoundStartMillis = millis();

  while (millis() - measurementRoundStartMillis <= measurementRoundPeriod) {

    wdt_reset();

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

    // limit if for measurement protection:
    //    long max. is 2,147,483,647
    //    int16_t max. is 32,767
    //    long max - int16_t max = 2147450880
    //    if (measurementRoundAverage00 > 2147450880L) { break; }
    // ...so the measurements stop if the added values become too big and overflow is imminent.
    // 65538 * 32767 = 2,147,483,647 -> 65538 / 7 = 9362 seconds, if seven readings per second
    // -> 156 minutes = 2.6 hours max (measurement period)

    if (
      (measurementRoundAverage00 > 2147450880L)
      && (measurementRoundAverage01 > 2147450880L)
      && (measurementRoundAverage02 > 2147450880L)
      && (measurementRoundAverage03 > 2147450880L)
      && (measurementRoundAverage10 > 2147450880L)
      && (measurementRoundAverage11 > 2147450880L)
      && (measurementRoundAverage12 > 2147450880L)
      && (measurementRoundAverage13 > 2147450880L)
      && (measurementRoundAverage20 > 2147450880L)
      && (measurementRoundAverage21 > 2147450880L)
      && (measurementRoundAverage22 > 2147450880L)
      && (measurementRoundAverage23 > 2147450880L)
      && (measurementRoundAverage30 > 2147450880L)
      && (measurementRoundAverage31 > 2147450880L)
      && (measurementRoundAverage32 > 2147450880L)
      && (measurementRoundAverage33 > 2147450880L)
    ) {
      break;
    }
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

  measurementRoundTemperatureC00 = steinhartCalculation(measurementRoundAverage00);
  measurementRoundTemperatureC01 = steinhartCalculation(measurementRoundAverage01);
  measurementRoundTemperatureC02 = steinhartCalculation(measurementRoundAverage02);
  measurementRoundTemperatureC03 = steinhartCalculation(measurementRoundAverage03);

  measurementRoundTemperatureC10 = steinhartCalculation(measurementRoundAverage10);
  measurementRoundTemperatureC11 = steinhartCalculation(measurementRoundAverage11);
  measurementRoundTemperatureC12 = steinhartCalculation(measurementRoundAverage12);
  measurementRoundTemperatureC13 = steinhartCalculation(measurementRoundAverage13);

  measurementRoundTemperatureC20 = steinhartCalculation(measurementRoundAverage20);
  measurementRoundTemperatureC21 = steinhartCalculation(measurementRoundAverage21);
  measurementRoundTemperatureC22 = steinhartCalculation(measurementRoundAverage22);
  measurementRoundTemperatureC23 = steinhartCalculation(measurementRoundAverage23);

  measurementRoundTemperatureC30 = steinhartCalculation(measurementRoundAverage30);
  measurementRoundTemperatureC31 = steinhartCalculation(measurementRoundAverage31);
  measurementRoundTemperatureC32 = steinhartCalculation(measurementRoundAverage32);
  measurementRoundTemperatureC33 = steinhartCalculation(measurementRoundAverage33);

}

//------------------------------------------------------------------------------

float steinhartCalculation(signed long measurementRoundAverage) {

  float average;
  average = (float) measurementRoundAverage; // 13333.00; for 10000 resistance & 25.00 C*
  float steinhart;

  // convert the value to resistance
  average = adcRange / average - 1; //26665.85 / average - 1; //adcRange / average - 1;
  average = 10000 / average; // to get 10kOhm, needs 10000÷(26665.85÷13333−1)
  steinhart = average; // ~6559 not 10000 !!?? --> range 32767 / 6.144v * 5v = 26665.85

  steinhart = average / THERMISTORNOMINAL;     // (R/Ro)
  steinhart = log(steinhart);                  // ln(R/Ro)
  steinhart /= BCOEFFICIENT;                   // 1/B * ln(R/Ro)
  steinhart += 1.0 / (TEMPERATURENOMINAL + 273.15); // + (1/To)
  steinhart = 1.0 / steinhart;                 // Invert
  steinhart -= 273.15;                         // convert to C

  return steinhart;
}

//------------------------------------------------------------------------------

void sd1write() {

  wdt_reset();

  for (; !sd1.begin(SD1_CS) ;) {

    wdt_reset();

    if (millis() > startsdCardInitializeDelay + sdCardInitializeDelay) {
      sd1.begin(SD1_CS);
      startsdCardInitializeDelay = millis();
    }

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

  //-------------------------------------------------------------

  if (! (measurementfile1.print(dateAndTimeData)) ) {
    sd1.errorExit("measurementfile1 writing");
  }

  if (! (measurementfile1.print(",")) ) {
    sd1.errorExit("measurementfile1 writing");
  }

  //-------------------------------------------------------------

  if (! (measurementfile1.print(measurementRoundAverage00)) ) {
    sd1.errorExit("measurementfile1 writing");
  }
  if (! (measurementfile1.print(",")) ) {
    sd1.errorExit("measurementfile1 writing");
  }
  if (! (measurementfile1.print(measurementRoundTemperatureC00)) ) {
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
  if (! (measurementfile1.print(measurementRoundTemperatureC01)) ) {
    sd1.errorExit("measurementfile1 writing");
  }
  if (! (measurementfile1.print(",")) ) {
    sd1.errorExit("measurementfile1 writing");
  }

  if (! (measurementfile1.print(measurementRoundAverage02)) ) {
    sd1.errorExit("measurementfile1 writing");
  }
  if (! (measurementfile1.print(",")) ) {
    sd1.errorExit("measurementfile1 writing");
  }
  if (! (measurementfile1.print(measurementRoundTemperatureC02)) ) {
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
  if (! (measurementfile1.print(measurementRoundTemperatureC03)) ) {
    sd1.errorExit("measurementfile1 writing");
  }
  if (! (measurementfile1.print(",")) ) {
    sd1.errorExit("measurementfile1 writing");
  }

  //-------------------------------------------------------------

  if (! (measurementfile1.print(measurementRoundAverage10)) ) {
    sd1.errorExit("measurementfile1 writing");
  }
  if (! (measurementfile1.print(",")) ) {
    sd1.errorExit("measurementfile1 writing");
  }
  if (! (measurementfile1.print(measurementRoundTemperatureC10)) ) {
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
  if (! (measurementfile1.print(measurementRoundTemperatureC11)) ) {
    sd1.errorExit("measurementfile1 writing");
  }
  if (! (measurementfile1.print(",")) ) {
    sd1.errorExit("measurementfile1 writing");
  }

  if (! (measurementfile1.print(measurementRoundAverage12)) ) {
    sd1.errorExit("measurementfile1 writing");
  }
  if (! (measurementfile1.print(",")) ) {
    sd1.errorExit("measurementfile1 writing");
  }
  if (! (measurementfile1.print(measurementRoundTemperatureC12)) ) {
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
  if (! (measurementfile1.print(measurementRoundTemperatureC13)) ) {
    sd1.errorExit("measurementfile1 writing");
  }
  if (! (measurementfile1.print(",")) ) {
    sd1.errorExit("measurementfile1 writing");
  }

  //-------------------------------------------------------------

  if (! (measurementfile1.print(measurementRoundAverage20)) ) {
    sd1.errorExit("measurementfile1 writing");
  }
  if (! (measurementfile1.print(",")) ) {
    sd1.errorExit("measurementfile1 writing");
  }
  if (! (measurementfile1.print(measurementRoundTemperatureC20)) ) {
    sd1.errorExit("measurementfile1 writing");
  }
  if (! (measurementfile1.print(",")) ) {
    sd1.errorExit("measurementfile1 writing");
  }

  if (! (measurementfile1.print(measurementRoundAverage21)) ) {
    sd1.errorExit("measurementfile1 writing");
  }
  if (! (measurementfile1.print(",")) ) {
    sd1.errorExit("measurementfile1 writing");
  }
  if (! (measurementfile1.print(measurementRoundTemperatureC21)) ) {
    sd1.errorExit("measurementfile1 writing");
  }
  if (! (measurementfile1.print(",")) ) {
    sd1.errorExit("measurementfile1 writing");
  }

  if (! (measurementfile1.print(measurementRoundAverage22)) ) {
    sd1.errorExit("measurementfile1 writing");
  }
  if (! (measurementfile1.print(",")) ) {
    sd1.errorExit("measurementfile1 writing");
  }
  if (! (measurementfile1.print(measurementRoundTemperatureC22)) ) {
    sd1.errorExit("measurementfile1 writing");
  }
  if (! (measurementfile1.print(",")) ) {
    sd1.errorExit("measurementfile1 writing");
  }

  if (! (measurementfile1.print(measurementRoundAverage23)) ) {
    sd1.errorExit("measurementfile1 writing");
  }
  if (! (measurementfile1.print(",")) ) {
    sd1.errorExit("measurementfile1 writing");
  }
  if (! (measurementfile1.print(measurementRoundTemperatureC23)) ) {
    sd1.errorExit("measurementfile1 writing");
  }
  if (! (measurementfile1.print(",")) ) {
    sd1.errorExit("measurementfile1 writing");
  }

  //-------------------------------------------------------------

  if (! (measurementfile1.print(measurementRoundAverage30)) ) {
    sd1.errorExit("measurementfile1 writing");
  }
  if (! (measurementfile1.print(",")) ) {
    sd1.errorExit("measurementfile1 writing");
  }
  if (! (measurementfile1.print(measurementRoundTemperatureC30)) ) {
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
  if (! (measurementfile1.print(measurementRoundTemperatureC31)) ) {
    sd1.errorExit("measurementfile1 writing");
  }
  if (! (measurementfile1.print(",")) ) {
    sd1.errorExit("measurementfile1 writing");
  }

  if (! (measurementfile1.print(measurementRoundAverage32)) ) {
    sd1.errorExit("measurementfile1 writing");
  }
  if (! (measurementfile1.print(",")) ) {
    sd1.errorExit("measurementfile1 writing");
  }
  if (! (measurementfile1.print(measurementRoundTemperatureC32)) ) {
    sd1.errorExit("measurementfile1 writing");
  }
  if (! (measurementfile1.print(",")) ) {
    sd1.errorExit("measurementfile1 writing");
  }

  if (! (measurementfile1.print(measurementRoundAverage33)) ) {
    sd1.errorExit("measurementfile1 writing");
  }
  if (! (measurementfile1.print(",")) ) {
    sd1.errorExit("measurementfile1 writing");
  }
  if (! (measurementfile1.println(measurementRoundTemperatureC33)) ) {
    sd1.errorExit("measurementfile1 writing");
  }

  measurementfile1.close();

}

//------------------------------------------------------------------------------


void sd2write() {
  //  Serial.println("begin sd2write()");

  wdt_reset();

  for (; !sd2.begin(SD2_CS);) {

    wdt_reset();

    if (millis() > startsdCardInitializeDelay + sdCardInitializeDelay) {
      sd2.begin(SD2_CS);
      startsdCardInitializeDelay = millis();
    }

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

  //-------------------------------------------------------------

  if (! (measurementfile2.print(dateAndTimeData)) ) {
    sd2.errorExit("measurementfile2 writing");
  }

  if (! (measurementfile2.print(",")) ) {
    sd2.errorExit("measurementfile2 writing");
  }

  //-------------------------------------------------------------

  if (! (measurementfile2.print(measurementRoundAverage00)) ) {
    sd1.errorExit("measurementfile2 writing");
  }
  if (! (measurementfile2.print(",")) ) {
    sd1.errorExit("measurementfile2 writing");
  }
  if (! (measurementfile2.print(measurementRoundTemperatureC00)) ) {
    sd1.errorExit("measurementfile2 writing");
  }
  if (! (measurementfile2.print(",")) ) {
    sd1.errorExit("measurementfile2 writing");
  }

  if (! (measurementfile2.print(measurementRoundAverage01)) ) {
    sd1.errorExit("measurementfile2 writing");
  }
  if (! (measurementfile2.print(",")) ) {
    sd1.errorExit("measurementfile2 writing");
  }
  if (! (measurementfile2.print(measurementRoundTemperatureC01)) ) {
    sd1.errorExit("measurementfile2 writing");
  }
  if (! (measurementfile2.print(",")) ) {
    sd1.errorExit("measurementfile2 writing");
  }

  if (! (measurementfile2.print(measurementRoundAverage02)) ) {
    sd1.errorExit("measurementfile2 writing");
  }
  if (! (measurementfile2.print(",")) ) {
    sd1.errorExit("measurementfile2 writing");
  }
  if (! (measurementfile2.print(measurementRoundTemperatureC02)) ) {
    sd1.errorExit("measurementfile2 writing");
  }
  if (! (measurementfile2.print(",")) ) {
    sd1.errorExit("measurementfile2 writing");
  }

  if (! (measurementfile2.print(measurementRoundAverage03)) ) {
    sd1.errorExit("measurementfile2 writing");
  }
  if (! (measurementfile2.print(",")) ) {
    sd1.errorExit("measurementfile2 writing");
  }
  if (! (measurementfile2.print(measurementRoundTemperatureC03)) ) {
    sd1.errorExit("measurementfile2 writing");
  }
  if (! (measurementfile2.print(",")) ) {
    sd1.errorExit("measurementfile2 writing");
  }

  //-------------------------------------------------------------

  if (! (measurementfile2.print(measurementRoundAverage10)) ) {
    sd1.errorExit("measurementfile2 writing");
  }
  if (! (measurementfile2.print(",")) ) {
    sd1.errorExit("measurementfile2 writing");
  }
  if (! (measurementfile2.print(measurementRoundTemperatureC10)) ) {
    sd1.errorExit("measurementfile2 writing");
  }
  if (! (measurementfile2.print(",")) ) {
    sd1.errorExit("measurementfile2 writing");
  }

  if (! (measurementfile2.print(measurementRoundAverage11)) ) {
    sd1.errorExit("measurementfile2 writing");
  }
  if (! (measurementfile2.print(",")) ) {
    sd1.errorExit("measurementfile2 writing");
  }
  if (! (measurementfile2.print(measurementRoundTemperatureC11)) ) {
    sd1.errorExit("measurementfile2 writing");
  }
  if (! (measurementfile2.print(",")) ) {
    sd1.errorExit("measurementfile2 writing");
  }

  if (! (measurementfile2.print(measurementRoundAverage12)) ) {
    sd1.errorExit("measurementfile2 writing");
  }
  if (! (measurementfile2.print(",")) ) {
    sd1.errorExit("measurementfile2 writing");
  }
  if (! (measurementfile2.print(measurementRoundTemperatureC12)) ) {
    sd1.errorExit("measurementfile2 writing");
  }
  if (! (measurementfile2.print(",")) ) {
    sd1.errorExit("measurementfile2 writing");
  }

  if (! (measurementfile2.print(measurementRoundAverage13)) ) {
    sd1.errorExit("measurementfile2 writing");
  }
  if (! (measurementfile2.print(",")) ) {
    sd1.errorExit("measurementfile2 writing");
  }
  if (! (measurementfile2.print(measurementRoundTemperatureC13)) ) {
    sd1.errorExit("measurementfile2 writing");
  }
  if (! (measurementfile2.print(",")) ) {
    sd1.errorExit("measurementfile2 writing");
  }

  //-------------------------------------------------------------

  if (! (measurementfile2.print(measurementRoundAverage20)) ) {
    sd1.errorExit("measurementfile2 writing");
  }
  if (! (measurementfile2.print(",")) ) {
    sd1.errorExit("measurementfile2 writing");
  }
  if (! (measurementfile2.print(measurementRoundTemperatureC20)) ) {
    sd1.errorExit("measurementfile2 writing");
  }
  if (! (measurementfile2.print(",")) ) {
    sd1.errorExit("measurementfile2 writing");
  }

  if (! (measurementfile2.print(measurementRoundAverage21)) ) {
    sd1.errorExit("measurementfile2 writing");
  }
  if (! (measurementfile2.print(",")) ) {
    sd1.errorExit("measurementfile2 writing");
  }
  if (! (measurementfile2.print(measurementRoundTemperatureC21)) ) {
    sd1.errorExit("measurementfile2 writing");
  }
  if (! (measurementfile2.print(",")) ) {
    sd1.errorExit("measurementfile2 writing");
  }

  if (! (measurementfile2.print(measurementRoundAverage22)) ) {
    sd1.errorExit("measurementfile2 writing");
  }
  if (! (measurementfile2.print(",")) ) {
    sd1.errorExit("measurementfile2 writing");
  }
  if (! (measurementfile2.print(measurementRoundTemperatureC22)) ) {
    sd1.errorExit("measurementfile2 writing");
  }
  if (! (measurementfile2.print(",")) ) {
    sd1.errorExit("measurementfile2 writing");
  }

  if (! (measurementfile2.print(measurementRoundAverage23)) ) {
    sd1.errorExit("measurementfile2 writing");
  }
  if (! (measurementfile2.print(",")) ) {
    sd1.errorExit("measurementfile2 writing");
  }
  if (! (measurementfile2.print(measurementRoundTemperatureC23)) ) {
    sd1.errorExit("measurementfile2 writing");
  }
  if (! (measurementfile2.print(",")) ) {
    sd1.errorExit("measurementfile2 writing");
  }

  //-------------------------------------------------------------

  if (! (measurementfile2.print(measurementRoundAverage30)) ) {
    sd1.errorExit("measurementfile2 writing");
  }
  if (! (measurementfile2.print(",")) ) {
    sd1.errorExit("measurementfile2 writing");
  }
  if (! (measurementfile2.print(measurementRoundTemperatureC30)) ) {
    sd1.errorExit("measurementfile2 writing");
  }
  if (! (measurementfile2.print(",")) ) {
    sd1.errorExit("measurementfile2 writing");
  }

  if (! (measurementfile2.print(measurementRoundAverage31)) ) {
    sd1.errorExit("measurementfile2 writing");
  }
  if (! (measurementfile2.print(",")) ) {
    sd1.errorExit("measurementfile2 writing");
  }
  if (! (measurementfile2.print(measurementRoundTemperatureC31)) ) {
    sd1.errorExit("measurementfile2 writing");
  }
  if (! (measurementfile2.print(",")) ) {
    sd1.errorExit("measurementfile2 writing");
  }

  if (! (measurementfile2.print(measurementRoundAverage32)) ) {
    sd1.errorExit("measurementfile2 writing");
  }
  if (! (measurementfile2.print(",")) ) {
    sd1.errorExit("measurementfile2 writing");
  }
  if (! (measurementfile2.print(measurementRoundTemperatureC32)) ) {
    sd1.errorExit("measurementfile2 writing");
  }
  if (! (measurementfile2.print(",")) ) {
    sd1.errorExit("measurementfile2 writing");
  }

  if (! (measurementfile2.print(measurementRoundAverage33)) ) {
    sd1.errorExit("measurementfile2 writing");
  }
  if (! (measurementfile2.print(",")) ) {
    sd1.errorExit("measurementfile2 writing");
  }
  if (! (measurementfile2.println(measurementRoundTemperatureC33)) ) {
    sd1.errorExit("measurementfile2 writing");
  }

  measurementfile2.close();

}

//------------------------------------------------------------------------------

void sd1writeHeader() {

  wdt_reset();

  for (; !sd1.begin(SD1_CS);) {

    wdt_reset();

    if (millis() > startsdCardInitializeDelay + sdCardInitializeDelay) {
      sd1.begin(SD1_CS);
      startsdCardInitializeDelay = millis();
    }
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
  if (!measurementfile1.open(measurementfileName, O_RDWR | O_CREAT)) {
    sd1.errorExit("open measurementfile1");
  }

  if (! (measurementfile1.println(measurementfileHeader)) ) {
    sd1.errorExit("measurementfile1 writing");
  }

  measurementfile1.close();

}

//------------------------------------------------------------------------------

void sd2writeHeader() {

  wdt_reset();

  for (; !sd2.begin(SD2_CS);) {

    wdt_reset();

    if (millis() > startsdCardInitializeDelay + sdCardInitializeDelay) {
      sd2.begin(SD2_CS);
      startsdCardInitializeDelay = millis();
    }
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
  if (!measurementfile2.open(measurementfileName, O_RDWR | O_CREAT)) {
    sd2.errorExit("open measurementfile2");
  }

  if (! (measurementfile2.println(measurementfileHeader)) ) {
    sd2.errorExit("measurementfile2 writing");
  }

  measurementfile2.close();

}

//------------------------------------------------------------------------------

void sd1writeLog() {

  wdt_reset();

  for (; !sd1.begin(SD1_CS);) {

    wdt_reset();

    if (millis() > startsdCardInitializeDelay + sdCardInitializeDelay) {
      sd1.begin(SD1_CS);
      startsdCardInitializeDelay = millis();
    }
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
  if (!logfile1.open(logfileName, O_RDWR | O_CREAT | O_AT_END)) {
    sd1.errorExit("open logfile1");
  }

  if (! (logfile1.print(dateAndTimeData)) ) {
    sd1.errorExit("logfile1 writing");
  }

  if (! (logfile1.print(",")) ) {
    sd1.errorExit("logfile1 writing");
  }

  if (! (logfile1.println(logMsg)) ) {
    sd1.errorExit("logfile1 writing");
  }

  logfile1.close();

}

//------------------------------------------------------------------------------

void sd2writeLog() {

  wdt_reset();

  for (; !sd2.begin(SD2_CS);) {

    wdt_reset();

    if (millis() > startsdCardInitializeDelay + sdCardInitializeDelay) {
      sd2.begin(SD2_CS);
      startsdCardInitializeDelay = millis();
    }
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
  if (!logfile2.open(logfileName, O_RDWR | O_CREAT | O_AT_END)) {
    sd2.errorExit("open logfile2");
  }

  if (! (logfile2.print(dateAndTimeData)) ) {
    sd2.errorExit("logfile2 writing");
  }

  if (! (logfile2.print(",")) ) {
    sd2.errorExit("logfile2 writing");
  }

  if (! (logfile2.println(logMsg)) ) {
    sd2.errorExit("logfile2 writing");
  }

  logfile2.close();

}

//------------------------------------------------------------------------------

void relayTimeBufferTimer() {
  startRelayTimeBuffer = millis();

  //  wait approx. [relayTimeBuffer] ms

  while (millis() < startRelayTimeBuffer + relayTimeBuffer) {
    wdt_reset();
  }
}

//------------------------------------------------------------------------------

void setup() {

  wdt_disable();  // Disable the watchdog and wait for more than 2 seconds
  delay(3000);  // With this the Arduino doesn't keep resetting infinitely in case of wrong configuration
  wdt_enable(WDTO_250MS);

  //    Serial.begin(115200);
  //
  //    // Wait for USB Serial
  //    while (!Serial) {
  //      ; // wait for serial port to connect. Needed for native USB port only
  //    }
  //
  //    Serial.println(F("setup() begin"));

  pinMode(ads0Relay, OUTPUT);
  pinMode(ads1Relay, OUTPUT);
  pinMode(ads2Relay, OUTPUT);
  pinMode(ads3Relay, OUTPUT);

  digitalWrite(SD2_CS, HIGH);

  Controllino_RTC_init();

  getDateAndTime();
  startGetDateAndTimeInterval = millis();

  sprintf(logMsg, ("Hello world. I start now.,shutDownPeriod,%ld,measurementPeriod,%ld,measurementRoundPeriod,%ld"), shutDownPeriod, measurementPeriod, measurementRoundPeriod);

  sd1writeLog();
  sd2writeLog();

  sprintf(measurementfileHeader, ("YYYY-MM-DDThh:mm:ss,0-0,C*,0-1,C*,0-2,C*,0-3,C*,1-0,C*,1-1,C*,1-2,C*,1-3,C*,2-0,C*,2-1,C*,2-2,C*,2-3,C*,3-0,C*,3-1,C*,3-2,C*,3-3,C*"));

  sd1writeHeader();
  sd2writeHeader();

  initializeADCs();

  startShutDownPeriod = millis() - shutDownPeriod;// + 1000; // start shutdownperiod, but start measurements in loop() faster

}

//------------------------------------------------------------------------------

void loop() {

  wdt_reset();

  while (millis() - startShutDownPeriod >= shutDownPeriod - relayTimeBuffer) {

    digitalWrite(ads0Relay, HIGH);
    digitalWrite(ads1Relay, HIGH);
    digitalWrite(ads2Relay, HIGH);
    digitalWrite(ads3Relay, HIGH);

    relayTimeBufferTimer();

    startMeasurementPeriod = millis();

    while (millis() - startMeasurementPeriod <= measurementPeriod) {

      if ( millis () - startGetDateAndTimeInterval >= getDateAndTimeInterval) {
        getDateAndTime();
        startGetDateAndTimeInterval = millis();
      }

      measurements(); // Note: looping in measurements() for measurementRoundPeriod

      sd1write();
      sd2write();

    }

    startShutDownPeriod = millis();

    digitalWrite(ads0Relay, LOW);
    digitalWrite(ads1Relay, LOW);
    digitalWrite(ads2Relay, LOW);
    digitalWrite(ads3Relay, LOW);

  }

}
