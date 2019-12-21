//mosslogger.ino
// From HIH-4000 series sensor datasheet: VOUT=(VSUPPLY)(0.0062(sensor RH)+0.16), typical at25 ºC
// Supplied voltage at ADCs GND-VDD is 4V86 (w/ handheld multimeter)


#include <SPI.h>
#include <SdFat.h>
#include <Controllino.h> // Usage of CONTROLLINO library allows you to use CONTROLLINO_xx aliases in your sketch.
#include <Wire.h>
#include <Adafruit_ADS1015.h>
#include <avr/wdt.h>
#include "Statistic.h"

//------------------------------------------------------------------------------

Adafruit_ADS1115 port0ads0(0x48);
Adafruit_ADS1115 port0ads1(0x49);
Adafruit_ADS1115 port0ads2(0x4A);

Adafruit_ADS1115 port1ads0(0x48);
Adafruit_ADS1115 port1ads1(0x49);
Adafruit_ADS1115 port1ads2(0x4A);
Adafruit_ADS1115 port1ads3(0x4B);

// these control ground connection to voltage dividers in adc channels with thermistors (not RH sensors)
#define port0ads0Relay CONTROLLINO_R6
#define port0ads1Relay CONTROLLINO_R7
#define port0ads2Relay CONTROLLINO_R8
#define port1ads0Relay CONTROLLINO_R9
#define port1ads1Relay CONTROLLINO_R10
#define port1ads2Relay CONTROLLINO_R11
#define port1ads3Relay CONTROLLINO_R12

unsigned long startGetDateAndTimeInterval = 0; // to mark the start of current getDateAndTimeInterval
const unsigned long getDateAndTimeInterval = 1000; // in milliseconds, interval between dateAndTimeData refreshs with getDateAndTime()

unsigned long startRelayTimeBuffer = 0; // to mark the start of current relayTimeBuffer
const unsigned long relayTimeBuffer = 1000; // in milliseconds, interval between turning relays on and starting measurements(), in effect giving ADCs time to start

//------------------------------------------------------------------------------

unsigned long startShutDownPeriod = 0; // to mark the start of current shutDownPeriod
const unsigned long shutDownPeriod = 5000; // in milliseconds, how long to power off ADCs between measurement periods

unsigned long startMeasurementPeriod = 0; // to mark the start of current measurementPeriod
const unsigned long measurementPeriod = 10000; // in milliseconds, how long to keep measuring, looping measurement rounds

const unsigned long measurementRoundPeriod = 1000; //  in milliseconds, how long to loop through ADCs reading values in, before calculating the averages

//------------------------------------------------------------------------------

unsigned long startsdCardInitializeDelay = 0; // to mark the start of current sdCardInitializeDelay
const int16_t sdCardInitializeDelay = 200; // in milliseconds, interval between attempts to read sd card if removed - remember watchdog timer settings!

Statistic port0measurementRoundStatistic00, port0measurementRoundStatistic01, port0measurementRoundStatistic02, port0measurementRoundStatistic03;
Statistic port0measurementRoundStatistic10, port0measurementRoundStatistic11, port0measurementRoundStatistic12, port0measurementRoundStatistic13;
Statistic port0measurementRoundStatistic20, port0measurementRoundStatistic21, port0measurementRoundStatistic22, port0measurementRoundStatistic23;

Statistic port1measurementRoundStatistic00, port1measurementRoundStatistic01, port1measurementRoundStatistic02, port1measurementRoundStatistic03;
Statistic port1measurementRoundStatistic10, port1measurementRoundStatistic11, port1measurementRoundStatistic12, port1measurementRoundStatistic13;
Statistic port1measurementRoundStatistic20, port1measurementRoundStatistic21, port1measurementRoundStatistic22, port1measurementRoundStatistic23;
Statistic port1measurementRoundStatistic30, port1measurementRoundStatistic31, port1measurementRoundStatistic32, port1measurementRoundStatistic33;

int16_t measurementRoundCounter = 0;

signed long port0measurementRoundSum00 = 0, port0measurementRoundSum01 = 0, port0measurementRoundSum02 = 0, port0measurementRoundSum03 = 0;
signed long port0measurementRoundSum10 = 0, port0measurementRoundSum11 = 0, port0measurementRoundSum12 = 0, port0measurementRoundSum13 = 0;
signed long port0measurementRoundSum20 = 0, port0measurementRoundSum21 = 0, port0measurementRoundSum22 = 0, port0measurementRoundSum23 = 0;

signed long port1measurementRoundSum00 = 0, port1measurementRoundSum01 = 0, port1measurementRoundSum02 = 0, port1measurementRoundSum03 = 0;
signed long port1measurementRoundSum10 = 0, port1measurementRoundSum11 = 0, port1measurementRoundSum12 = 0, port1measurementRoundSum13 = 0;
signed long port1measurementRoundSum20 = 0, port1measurementRoundSum21 = 0, port1measurementRoundSum22 = 0, port1measurementRoundSum23 = 0;
signed long port1measurementRoundSum30 = 0, port1measurementRoundSum31 = 0, port1measurementRoundSum32 = 0, port1measurementRoundSum33 = 0;

float port0measurementRoundAverage00 = 0.0, port0measurementRoundAverage01 = 0.0, port0measurementRoundAverage02 = 0.0, port0measurementRoundAverage03 = 0.0;
float port0measurementRoundAverage10 = 0.0, port0measurementRoundAverage11 = 0.0, port0measurementRoundAverage12 = 0.0, port0measurementRoundAverage13 = 0.0;
float port0measurementRoundAverage20 = 0.0, port0measurementRoundAverage21 = 0.0, port0measurementRoundAverage22 = 0.0, port0measurementRoundAverage23 = 0.0;

float port1measurementRoundAverage00 = 0.0, port1measurementRoundAverage01 = 0.0, port1measurementRoundAverage02 = 0.0, port1measurementRoundAverage03 = 0.0;
float port1measurementRoundAverage10 = 0.0, port1measurementRoundAverage11 = 0.0, port1measurementRoundAverage12 = 0.0, port1measurementRoundAverage13 = 0.0;
float port1measurementRoundAverage20 = 0.0, port1measurementRoundAverage21 = 0.0, port1measurementRoundAverage22 = 0.0, port1measurementRoundAverage23 = 0.0;
float port1measurementRoundAverage30 = 0.0, port1measurementRoundAverage31 = 0.0, port1measurementRoundAverage32 = 0.0, port1measurementRoundAverage33 = 0.0;

float port0measurementRoundStDev00 = 0.0, port0measurementRoundStDev01 = 0.0, port0measurementRoundStDev02 = 0.0, port0measurementRoundStDev03 = 0.0;
float port0measurementRoundStDev10 = 0.0, port0measurementRoundStDev11 = 0.0, port0measurementRoundStDev12 = 0.0, port0measurementRoundStDev13 = 0.0;
float port0measurementRoundStDev20 = 0.0, port0measurementRoundStDev21 = 0.0, port0measurementRoundStDev22 = 0.0, port0measurementRoundStDev23 = 0.0;

float port1measurementRoundStDev00 = 0.0, port1measurementRoundStDev01 = 0.0, port1measurementRoundStDev02 = 0.0, port1measurementRoundStDev03 = 0.0;
float port1measurementRoundStDev10 = 0.0, port1measurementRoundStDev11 = 0.0, port1measurementRoundStDev12 = 0.0, port1measurementRoundStDev13 = 0.0;
float port1measurementRoundStDev20 = 0.0, port1measurementRoundStDev21 = 0.0, port1measurementRoundStDev22 = 0.0, port1measurementRoundStDev23 = 0.0;
float port1measurementRoundStDev30 = 0.0, port1measurementRoundStDev31 = 0.0, port1measurementRoundStDev32 = 0.0, port1measurementRoundStDev33 = 0.0;

float adcRange = 25970.8; //manually calibrated to 10 kOhm resistors = 25 oC (32767 / 6.144v * 5v = 2665.85)

#define THERMISTORNOMINAL 10000  // resistance at 25 degrees C
#define TEMPERATURENOMINAL 25  // temp. for nominal resistance (almost always 25 oC)
#define BCOEFFICIENT 3976  // The beta coefficient of the thermistor (usually 3000-4000)
#define SERIESRESISTOR 10000  // the value of the 'other' resistor

#define HIH4000_SUPPLY_VOLTAGE 5.0
#define ARDUINO_VCC 4.87

float port0RH00ZeroOffsetV = 0.881115, port0RH00Slope = 0.029501451;
float port0RH02ZeroOffsetV = 0.881115, port0RH02Slope = 0.029501451;
float port0RH10ZeroOffsetV = 0.881115, port0RH10Slope = 0.029501451; 
float port0RH12ZeroOffsetV = 0.881115, port0RH12Slope = 0.029501451;    
float port0RH20ZeroOffsetV = 0.881115, port0RH20Slope = 0.029501451;
float port0RH22ZeroOffsetV = 0.881115, port0RH22Slope = 0.029501451;

float port0measurementRoundTrueRH00 = 0.0, port0measurementRoundTemperatureC01 = 0.0, port0measurementRoundTrueRH02 = 0.0, port0measurementRoundTemperatureC03 = 0.0;
float port0measurementRoundTrueRH10 = 0.0, port0measurementRoundTemperatureC11 = 0.0, port0measurementRoundTrueRH12 = 0.0, port0measurementRoundTemperatureC13 = 0.0;
float port0measurementRoundTrueRH20 = 0.0, port0measurementRoundTemperatureC21 = 0.0, port0measurementRoundTrueRH22 = 0.0, port0measurementRoundTemperatureC23 = 0.0;

float port1measurementRoundTemperatureC00 = 0.0, port1measurementRoundTemperatureC01 = 0.0, port1measurementRoundTemperatureC02 = 0.0, port1measurementRoundTemperatureC03 = 0.0;
float port1measurementRoundTemperatureC10 = 0.0, port1measurementRoundTemperatureC11 = 0.0, port1measurementRoundTemperatureC12 = 0.0, port1measurementRoundTemperatureC13 = 0.0;
float port1measurementRoundTemperatureC20 = 0.0, port1measurementRoundTemperatureC21 = 0.0, port1measurementRoundTemperatureC22 = 0.0, port1measurementRoundTemperatureC23 = 0.0;
float port1measurementRoundTemperatureC30 = 0.0, port1measurementRoundTemperatureC31 = 0.0, port1measurementRoundTemperatureC32 = 0.0, port1measurementRoundTemperatureC33 = 0.0;

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
char measurementfileHeader[888]; // space for YYYY-MM-DDThh:mm:ss,0-0,etc. plus the null char terminator
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

void port0InitializeADCs() {
  port0ads0.setGain(GAIN_TWOTHIRDS);
  port0ads1.setGain(GAIN_TWOTHIRDS);
  port0ads2.setGain(GAIN_TWOTHIRDS);

  port0ads0.begin();
  port0ads1.begin();
  port0ads2.begin();
}

//------------------------------------------------------------------------------

void port1InitializeADCs() {
  port1ads0.setGain(GAIN_TWOTHIRDS);
  port1ads1.setGain(GAIN_TWOTHIRDS);
  port1ads2.setGain(GAIN_TWOTHIRDS);
  port1ads3.setGain(GAIN_TWOTHIRDS);

  port1ads0.begin();
  port1ads1.begin();
  port1ads2.begin();
  port1ads3.begin();
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

  clearRoundVariables();

  unsigned long measurementRoundStartMillis = 0;

  int16_t port0measurement00, port0measurement01, port0measurement02, port0measurement03;
  int16_t port0measurement10, port0measurement11, port0measurement12, port0measurement13;
  int16_t port0measurement20, port0measurement21, port0measurement22, port0measurement23;

  int16_t port1measurement00, port1measurement01, port1measurement02, port1measurement03;
  int16_t port1measurement10, port1measurement11, port1measurement12, port1measurement13;
  int16_t port1measurement20, port1measurement21, port1measurement22, port1measurement23;
  int16_t port1measurement30, port1measurement31, port1measurement32, port1measurement33;

  measurementRoundStartMillis = millis();

  while (millis() - measurementRoundStartMillis <= measurementRoundPeriod) {

    wdt_reset();
    tcaselect(0);

    port0measurement00 = port0ads0.readADC_SingleEnded(0);
    port0measurement01 = port0ads0.readADC_SingleEnded(1);
    port0measurement02 = port0ads0.readADC_SingleEnded(2);
    port0measurement03 = port0ads0.readADC_SingleEnded(3);

    port0measurement10 = port0ads1.readADC_SingleEnded(0);
    port0measurement11 = port0ads1.readADC_SingleEnded(1);
    port0measurement12 = port0ads1.readADC_SingleEnded(2);
    port0measurement13 = port0ads1.readADC_SingleEnded(3);

    port0measurement20 = port0ads2.readADC_SingleEnded(0);
    port0measurement21 = port0ads2.readADC_SingleEnded(1);
    port0measurement22 = port0ads2.readADC_SingleEnded(2);
    port0measurement23 = port0ads2.readADC_SingleEnded(3);

    port1measurement00 = port1ads0.readADC_SingleEnded(0);
    port1measurement01 = port1ads0.readADC_SingleEnded(1);
    port1measurement02 = port1ads0.readADC_SingleEnded(2);
    port1measurement03 = port1ads0.readADC_SingleEnded(3);

    port1measurement10 = port1ads1.readADC_SingleEnded(0);
    port1measurement11 = port1ads1.readADC_SingleEnded(1);
    port1measurement12 = port1ads1.readADC_SingleEnded(2);
    port1measurement13 = port1ads1.readADC_SingleEnded(3);

    port1measurement20 = port1ads2.readADC_SingleEnded(0);
    port1measurement21 = port1ads2.readADC_SingleEnded(1);
    port1measurement22 = port1ads2.readADC_SingleEnded(2);
    port1measurement23 = port1ads2.readADC_SingleEnded(3);

    port1measurement30 = port1ads3.readADC_SingleEnded(0);
    port1measurement31 = port1ads3.readADC_SingleEnded(1);
    port1measurement32 = port1ads3.readADC_SingleEnded(2);
    port1measurement33 = port1ads3.readADC_SingleEnded(3);

    port0measurementRoundStatistic00.add(port0measurement00);
    port0measurementRoundStatistic01.add(port0measurement01);
    port0measurementRoundStatistic02.add(port0measurement02);
    port0measurementRoundStatistic03.add(port0measurement03);
    port0measurementRoundStatistic10.add(port0measurement10);
    port0measurementRoundStatistic11.add(port0measurement11);
    port0measurementRoundStatistic12.add(port0measurement12);
    port0measurementRoundStatistic13.add(port0measurement13);
    port0measurementRoundStatistic20.add(port0measurement20);
    port0measurementRoundStatistic21.add(port0measurement21);
    port0measurementRoundStatistic22.add(port0measurement22);
    port0measurementRoundStatistic23.add(port0measurement23);

    port1measurementRoundStatistic00.add(port1measurement00);
    port1measurementRoundStatistic01.add(port1measurement01);
    port1measurementRoundStatistic02.add(port1measurement02);
    port1measurementRoundStatistic03.add(port1measurement03);
    port1measurementRoundStatistic10.add(port1measurement10);
    port1measurementRoundStatistic11.add(port1measurement11);
    port1measurementRoundStatistic12.add(port1measurement12);
    port1measurementRoundStatistic13.add(port1measurement13);
    port1measurementRoundStatistic20.add(port1measurement20);
    port1measurementRoundStatistic21.add(port1measurement21);
    port1measurementRoundStatistic22.add(port1measurement22);
    port1measurementRoundStatistic23.add(port1measurement23);
    port1measurementRoundStatistic30.add(port1measurement30);
    port1measurementRoundStatistic31.add(port1measurement31);
    port1measurementRoundStatistic32.add(port1measurement32);
    port1measurementRoundStatistic33.add(port1measurement33);

    port0measurementRoundSum00 += port0measurement00;
    port0measurementRoundSum01 += port0measurement01;
    port0measurementRoundSum02 += port0measurement02;
    port0measurementRoundSum03 += port0measurement03;

    port0measurementRoundSum10 += port0measurement10;
    port0measurementRoundSum11 += port0measurement11;
    port0measurementRoundSum12 += port0measurement12;
    port0measurementRoundSum13 += port0measurement13;

    port0measurementRoundSum20 += port0measurement20;
    port0measurementRoundSum21 += port0measurement21;
    port0measurementRoundSum22 += port0measurement22;
    port0measurementRoundSum23 += port0measurement23;

    port1measurementRoundSum00 += port1measurement00;
    port1measurementRoundSum01 += port1measurement01;
    port1measurementRoundSum02 += port1measurement02;
    port1measurementRoundSum03 += port1measurement03;

    port1measurementRoundSum10 += port1measurement10;
    port1measurementRoundSum11 += port1measurement11;
    port1measurementRoundSum12 += port1measurement12;
    port1measurementRoundSum13 += port1measurement13;

    port1measurementRoundSum20 += port1measurement20;
    port1measurementRoundSum21 += port1measurement21;
    port1measurementRoundSum22 += port1measurement22;
    port1measurementRoundSum23 += port1measurement23;

    port1measurementRoundSum30 += port1measurement30;
    port1measurementRoundSum31 += port1measurement31;
    port1measurementRoundSum32 += port1measurement32;
    port1measurementRoundSum33 += port1measurement33;

    measurementRoundCounter++;

    // limit if for measurement protection:
    //    long max. is 2,147,483,647
    //    int16_t max. is 32,767
    //    long max - int16_t max = 2147450880
    //    if (measurementRoundSum00 > 2147450880L) { break; }
    // ...so the measurements stop if the added values become too big and overflow is imminent.
    // 65538 * 32767 = 2,147,483,647 -> 65538 / 7 = 9362 seconds, if seven readings per second
    // -> 156 minutes = 2.6 hours max (measurement period)

    if (
      (port0measurementRoundSum00 > 2147450880L)
      && (port0measurementRoundSum01 > 2147450880L)
      && (port0measurementRoundSum02 > 2147450880L)
      && (port0measurementRoundSum03 > 2147450880L)
      && (port0measurementRoundSum10 > 2147450880L)
      && (port0measurementRoundSum11 > 2147450880L)
      && (port0measurementRoundSum12 > 2147450880L)
      && (port0measurementRoundSum13 > 2147450880L)
      && (port0measurementRoundSum20 > 2147450880L)
      && (port0measurementRoundSum21 > 2147450880L)
      && (port0measurementRoundSum22 > 2147450880L)
      && (port1measurementRoundSum00 > 2147450880L)
      && (port1measurementRoundSum01 > 2147450880L)
      && (port1measurementRoundSum02 > 2147450880L)
      && (port1measurementRoundSum03 > 2147450880L)
      && (port1measurementRoundSum10 > 2147450880L)
      && (port1measurementRoundSum11 > 2147450880L)
      && (port1measurementRoundSum12 > 2147450880L)
      && (port1measurementRoundSum13 > 2147450880L)
      && (port1measurementRoundSum20 > 2147450880L)
      && (port1measurementRoundSum21 > 2147450880L)
      && (port1measurementRoundSum22 > 2147450880L)
      && (port1measurementRoundSum23 > 2147450880L)
      && (port1measurementRoundSum30 > 2147450880L)
      && (port1measurementRoundSum31 > 2147450880L)
      && (port1measurementRoundSum32 > 2147450880L)
      && (port1measurementRoundSum33 > 2147450880L)
    ) {
      break;
    }
  }

  port0measurementRoundAverage00 = port0measurementRoundStatistic00.average();
  port0measurementRoundAverage01 = port0measurementRoundStatistic01.average();
  port0measurementRoundAverage02 = port0measurementRoundStatistic02.average();
  port0measurementRoundAverage03 = port0measurementRoundStatistic03.average();

  port0measurementRoundAverage10 = port0measurementRoundStatistic10.average();
  port0measurementRoundAverage11 = port0measurementRoundStatistic11.average();
  port0measurementRoundAverage12 = port0measurementRoundStatistic12.average();
  port0measurementRoundAverage13 = port0measurementRoundStatistic13.average();

  port0measurementRoundAverage20 = port0measurementRoundStatistic20.average();
  port0measurementRoundAverage21 = port0measurementRoundStatistic21.average();
  port0measurementRoundAverage22 = port0measurementRoundStatistic22.average();
  port0measurementRoundAverage23 = port0measurementRoundStatistic23.average();

  port1measurementRoundAverage00 = port1measurementRoundStatistic00.average();
  port1measurementRoundAverage01 = port1measurementRoundStatistic01.average();
  port1measurementRoundAverage02 = port1measurementRoundStatistic02.average();
  port1measurementRoundAverage03 = port1measurementRoundStatistic03.average();

  port1measurementRoundAverage10 = port1measurementRoundStatistic10.average();
  port1measurementRoundAverage11 = port1measurementRoundStatistic11.average();
  port1measurementRoundAverage12 = port1measurementRoundStatistic12.average();
  port1measurementRoundAverage13 = port1measurementRoundStatistic13.average();

  port1measurementRoundAverage20 = port1measurementRoundStatistic20.average();
  port1measurementRoundAverage21 = port1measurementRoundStatistic21.average();
  port1measurementRoundAverage22 = port1measurementRoundStatistic22.average();
  port1measurementRoundAverage23 = port1measurementRoundStatistic23.average();

  port1measurementRoundAverage30 = port1measurementRoundStatistic30.average();
  port1measurementRoundAverage31 = port1measurementRoundStatistic31.average();
  port1measurementRoundAverage32 = port1measurementRoundStatistic32.average();
  port1measurementRoundAverage33 = port1measurementRoundStatistic33.average();

  port0measurementRoundStDev00 = port0measurementRoundStatistic00.pop_stdev();
  port0measurementRoundStDev01 = port0measurementRoundStatistic01.pop_stdev();
  port0measurementRoundStDev02 = port0measurementRoundStatistic02.pop_stdev();
  port0measurementRoundStDev03 = port0measurementRoundStatistic03.pop_stdev();

  port0measurementRoundStDev10 = port0measurementRoundStatistic10.pop_stdev();
  port0measurementRoundStDev11 = port0measurementRoundStatistic11.pop_stdev();
  port0measurementRoundStDev12 = port0measurementRoundStatistic12.pop_stdev();
  port0measurementRoundStDev13 = port0measurementRoundStatistic13.pop_stdev();

  port0measurementRoundStDev20 = port0measurementRoundStatistic20.pop_stdev();
  port0measurementRoundStDev21 = port0measurementRoundStatistic21.pop_stdev();
  port0measurementRoundStDev22 = port0measurementRoundStatistic22.pop_stdev();
  port0measurementRoundStDev23 = port0measurementRoundStatistic23.pop_stdev();

  port1measurementRoundStDev00 = port1measurementRoundStatistic00.pop_stdev();
  port1measurementRoundStDev01 = port1measurementRoundStatistic01.pop_stdev();
  port1measurementRoundStDev02 = port1measurementRoundStatistic02.pop_stdev();
  port1measurementRoundStDev03 = port1measurementRoundStatistic03.pop_stdev();

  port1measurementRoundStDev10 = port1measurementRoundStatistic10.pop_stdev();
  port1measurementRoundStDev11 = port1measurementRoundStatistic11.pop_stdev();
  port1measurementRoundStDev12 = port1measurementRoundStatistic12.pop_stdev();
  port1measurementRoundStDev13 = port1measurementRoundStatistic13.pop_stdev();

  port1measurementRoundStDev20 = port1measurementRoundStatistic20.pop_stdev();
  port1measurementRoundStDev21 = port1measurementRoundStatistic21.pop_stdev();
  port1measurementRoundStDev22 = port1measurementRoundStatistic22.pop_stdev();
  port1measurementRoundStDev23 = port1measurementRoundStatistic23.pop_stdev();

  port1measurementRoundStDev30 = port1measurementRoundStatistic30.pop_stdev();
  port1measurementRoundStDev31 = port1measurementRoundStatistic31.pop_stdev();
  port1measurementRoundStDev32 = port1measurementRoundStatistic32.pop_stdev();
  port1measurementRoundStDev33 = port1measurementRoundStatistic33.pop_stdev();


  port0measurementRoundTemperatureC01 = steinhartCalculation(port0measurementRoundAverage01);
  port0measurementRoundTrueRH00 = rhCalculation(port0measurementRoundAverage00,port0measurementRoundTemperatureC01);

  port0measurementRoundTemperatureC03 = steinhartCalculation(port0measurementRoundAverage03);
  port0measurementRoundTrueRH02 = rhCalculation(port0measurementRoundAverage02,port0measurementRoundTemperatureC03);

  port0measurementRoundTemperatureC11 = steinhartCalculation(port0measurementRoundAverage11);
  port0measurementRoundTrueRH10 = rhCalculation(port0measurementRoundAverage10,port0measurementRoundTemperatureC11);
  
  port0measurementRoundTemperatureC13 = steinhartCalculation(port0measurementRoundAverage13);
  port0measurementRoundTrueRH12 = rhCalculation(port0measurementRoundAverage12,port0measurementRoundTemperatureC13);

  port0measurementRoundTemperatureC21 = steinhartCalculation(port0measurementRoundAverage21);
  port0measurementRoundTrueRH20 = rhCalculation(port0measurementRoundAverage20,port0measurementRoundTemperatureC21);
  
  port0measurementRoundTemperatureC23 = steinhartCalculation(port0measurementRoundAverage23);
  port0measurementRoundTrueRH22 = rhCalculation(port0measurementRoundAverage22,port0measurementRoundTemperatureC23);

  port1measurementRoundTemperatureC00 = steinhartCalculation(port1measurementRoundAverage00);
  port1measurementRoundTemperatureC01 = steinhartCalculation(port1measurementRoundAverage01);
  port1measurementRoundTemperatureC02 = steinhartCalculation(port1measurementRoundAverage02);
  port1measurementRoundTemperatureC03 = steinhartCalculation(port1measurementRoundAverage03);

  port1measurementRoundTemperatureC10 = steinhartCalculation(port1measurementRoundAverage10);
  port1measurementRoundTemperatureC11 = steinhartCalculation(port1measurementRoundAverage11);
  port1measurementRoundTemperatureC12 = steinhartCalculation(port1measurementRoundAverage12);
  port1measurementRoundTemperatureC13 = steinhartCalculation(port1measurementRoundAverage13);

  port1measurementRoundTemperatureC20 = steinhartCalculation(port1measurementRoundAverage20);
  port1measurementRoundTemperatureC21 = steinhartCalculation(port1measurementRoundAverage21);
  port1measurementRoundTemperatureC22 = steinhartCalculation(port1measurementRoundAverage22);
  port1measurementRoundTemperatureC23 = steinhartCalculation(port1measurementRoundAverage23);

  port1measurementRoundTemperatureC30 = steinhartCalculation(port1measurementRoundAverage30);
  port1measurementRoundTemperatureC31 = steinhartCalculation(port1measurementRoundAverage31);
  port1measurementRoundTemperatureC32 = steinhartCalculation(port1measurementRoundAverage32);
  port1measurementRoundTemperatureC33 = steinhartCalculation(port1measurementRoundAverage33);

}

//------------------------------------------------------------------------------

float rhCalculation(float average, float temperature) {
  // TODO
}

//------------------------------------------------------------------------------

float steinhartCalculation(float average) {

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
  if (! (measurementfile1.print(measurementRoundStDev00)) ) {
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
  if (! (measurementfile1.print(measurementRoundStDev01)) ) {
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
  if (! (measurementfile1.print(measurementRoundStDev02)) ) {
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
  if (! (measurementfile1.print(measurementRoundStDev03)) ) {
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
  if (! (measurementfile1.print(measurementRoundStDev10)) ) {
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
  if (! (measurementfile1.print(measurementRoundStDev11)) ) {
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
  if (! (measurementfile1.print(measurementRoundStDev12)) ) {
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
  if (! (measurementfile1.print(measurementRoundStDev13)) ) {
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
  if (! (measurementfile1.print(measurementRoundStDev20)) ) {
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
  if (! (measurementfile1.print(measurementRoundStDev21)) ) {
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
  if (! (measurementfile1.print(measurementRoundStDev22)) ) {
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
  if (! (measurementfile1.print(measurementRoundStDev23)) ) {
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
  if (! (measurementfile1.print(measurementRoundStDev30)) ) {
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
  if (! (measurementfile1.print(measurementRoundStDev31)) ) {
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
  if (! (measurementfile1.print(measurementRoundStDev32)) ) {
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
  if (! (measurementfile1.print(measurementRoundTemperatureC33)) ) {
    sd1.errorExit("measurementfile1 writing");
  }
  if (! (measurementfile1.print(",")) ) {
    sd1.errorExit("measurementfile1 writing");
  }
  if (! (measurementfile1.println(measurementRoundStDev33)) ) {
    sd1.errorExit("measurementfile1 writing");
  }

  measurementfile1.close();

}

//------------------------------------------------------------------------------

void sd2write() {
  //  Serial.println("begin sd2write()");

  wdt_reset();

  for (; !sd2.begin(SD2_CS) ;) {

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
    sd2.errorExit("measurementfile2 writing");
  }
  if (! (measurementfile2.print(",")) ) {
    sd2.errorExit("measurementfile2 writing");
  }
  if (! (measurementfile2.print(measurementRoundTemperatureC00)) ) {
    sd2.errorExit("measurementfile2 writing");
  }
  if (! (measurementfile2.print(",")) ) {
    sd2.errorExit("measurementfile2 writing");
  }
  if (! (measurementfile2.print(measurementRoundStDev00)) ) {
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
  if (! (measurementfile2.print(measurementRoundTemperatureC01)) ) {
    sd2.errorExit("measurementfile2 writing");
  }
  if (! (measurementfile2.print(",")) ) {
    sd2.errorExit("measurementfile2 writing");
  }
  if (! (measurementfile2.print(measurementRoundStDev01)) ) {
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
  if (! (measurementfile2.print(measurementRoundTemperatureC02)) ) {
    sd2.errorExit("measurementfile2 writing");
  }
  if (! (measurementfile2.print(",")) ) {
    sd2.errorExit("measurementfile2 writing");
  }
  if (! (measurementfile2.print(measurementRoundStDev02)) ) {
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
  if (! (measurementfile2.print(measurementRoundTemperatureC03)) ) {
    sd2.errorExit("measurementfile2 writing");
  }
  if (! (measurementfile2.print(",")) ) {
    sd2.errorExit("measurementfile2 writing");
  }
  if (! (measurementfile2.print(measurementRoundStDev03)) ) {
    sd2.errorExit("measurementfile2 writing");
  }
  if (! (measurementfile2.print(",")) ) {
    sd2.errorExit("measurementfile2 writing");
  }

  //-------------------------------------------------------------

  if (! (measurementfile2.print(measurementRoundAverage10)) ) {
    sd2.errorExit("measurementfile2 writing");
  }
  if (! (measurementfile2.print(",")) ) {
    sd2.errorExit("measurementfile2 writing");
  }
  if (! (measurementfile2.print(measurementRoundTemperatureC10)) ) {
    sd2.errorExit("measurementfile2 writing");
  }
  if (! (measurementfile2.print(",")) ) {
    sd2.errorExit("measurementfile2 writing");
  }
  if (! (measurementfile2.print(measurementRoundStDev10)) ) {
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
  if (! (measurementfile2.print(measurementRoundTemperatureC11)) ) {
    sd2.errorExit("measurementfile2 writing");
  }
  if (! (measurementfile2.print(",")) ) {
    sd2.errorExit("measurementfile2 writing");
  }
  if (! (measurementfile2.print(measurementRoundStDev11)) ) {
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
  if (! (measurementfile2.print(measurementRoundTemperatureC12)) ) {
    sd2.errorExit("measurementfile2 writing");
  }
  if (! (measurementfile2.print(",")) ) {
    sd2.errorExit("measurementfile2 writing");
  }
  if (! (measurementfile2.print(measurementRoundStDev12)) ) {
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
  if (! (measurementfile2.print(measurementRoundTemperatureC13)) ) {
    sd2.errorExit("measurementfile2 writing");
  }
  if (! (measurementfile2.print(",")) ) {
    sd2.errorExit("measurementfile2 writing");
  }
  if (! (measurementfile2.print(measurementRoundStDev13)) ) {
    sd2.errorExit("measurementfile2 writing");
  }
  if (! (measurementfile2.print(",")) ) {
    sd2.errorExit("measurementfile2 writing");
  }

  //-------------------------------------------------------------

  if (! (measurementfile2.print(measurementRoundAverage20)) ) {
    sd2.errorExit("measurementfile2 writing");
  }
  if (! (measurementfile2.print(",")) ) {
    sd2.errorExit("measurementfile2 writing");
  }
  if (! (measurementfile2.print(measurementRoundTemperatureC20)) ) {
    sd2.errorExit("measurementfile2 writing");
  }
  if (! (measurementfile2.print(",")) ) {
    sd2.errorExit("measurementfile2 writing");
  }
  if (! (measurementfile2.print(measurementRoundStDev20)) ) {
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
  if (! (measurementfile2.print(measurementRoundTemperatureC21)) ) {
    sd2.errorExit("measurementfile2 writing");
  }
  if (! (measurementfile2.print(",")) ) {
    sd2.errorExit("measurementfile2 writing");
  }
  if (! (measurementfile2.print(measurementRoundStDev21)) ) {
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
  if (! (measurementfile2.print(measurementRoundTemperatureC22)) ) {
    sd2.errorExit("measurementfile2 writing");
  }
  if (! (measurementfile2.print(",")) ) {
    sd2.errorExit("measurementfile2 writing");
  }
  if (! (measurementfile2.print(measurementRoundStDev22)) ) {
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
  if (! (measurementfile2.print(measurementRoundTemperatureC23)) ) {
    sd2.errorExit("measurementfile2 writing");
  }
  if (! (measurementfile2.print(",")) ) {
    sd2.errorExit("measurementfile2 writing");
  }
  if (! (measurementfile2.print(measurementRoundStDev23)) ) {
    sd2.errorExit("measurementfile2 writing");
  }
  if (! (measurementfile2.print(",")) ) {
    sd2.errorExit("measurementfile2 writing");
  }

  //-------------------------------------------------------------

  if (! (measurementfile2.print(measurementRoundAverage30)) ) {
    sd2.errorExit("measurementfile2 writing");
  }
  if (! (measurementfile2.print(",")) ) {
    sd2.errorExit("measurementfile2 writing");
  }
  if (! (measurementfile2.print(measurementRoundTemperatureC30)) ) {
    sd2.errorExit("measurementfile2 writing");
  }
  if (! (measurementfile2.print(",")) ) {
    sd2.errorExit("measurementfile2 writing");
  }
  if (! (measurementfile2.print(measurementRoundStDev30)) ) {
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
  if (! (measurementfile2.print(measurementRoundTemperatureC31)) ) {
    sd2.errorExit("measurementfile2 writing");
  }
  if (! (measurementfile2.print(",")) ) {
    sd2.errorExit("measurementfile2 writing");
  }
  if (! (measurementfile2.print(measurementRoundStDev31)) ) {
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
  if (! (measurementfile2.print(measurementRoundTemperatureC32)) ) {
    sd2.errorExit("measurementfile2 writing");
  }
  if (! (measurementfile2.print(",")) ) {
    sd2.errorExit("measurementfile2 writing");
  }
  if (! (measurementfile2.print(measurementRoundStDev32)) ) {
    sd2.errorExit("measurementfile2 writing");
  }
  if (! (measurementfile2.print(",")) ) {
    sd2.errorExit("measurementfile2 writing");
  }

  if (! (measurementfile2.print(measurementRoundAverage33)) ) {
    sd2.errorExit("measurementfile2 writing");
  }
  if (! (measurementfile2.print(",")) ) {
    sd2.errorExit("measurementfile2 writing");
  }
  if (! (measurementfile2.print(measurementRoundTemperatureC33)) ) {
    sd2.errorExit("measurementfile2 writing");
  }
  if (! (measurementfile2.print(",")) ) {
    sd2.errorExit("measurementfile2 writing");
  }
  if (! (measurementfile2.println(measurementRoundStDev33)) ) {
    sd2.errorExit("measurementfile2 writing");
  }

  measurementfile2.close();

}

//------------------------------------------------------------------------------

void sd1writeHeader() {

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

void clearRoundVariables() {

  measurementRoundCounter = 0; // moved to global

  measurementRoundAverage00 = 0, measurementRoundAverage01 = 0, measurementRoundAverage02 = 0, measurementRoundAverage03 = 0;
  measurementRoundAverage10 = 0, measurementRoundAverage11 = 0, measurementRoundAverage12 = 0, measurementRoundAverage13 = 0;
  measurementRoundAverage20 = 0, measurementRoundAverage21 = 0, measurementRoundAverage22 = 0, measurementRoundAverage23 = 0;
  measurementRoundAverage30 = 0, measurementRoundAverage31 = 0, measurementRoundAverage32 = 0, measurementRoundAverage33 = 0;

  measurementRoundTemperatureC00 = 0.0, measurementRoundTemperatureC01 = 0.0, measurementRoundTemperatureC02 = 0.0, measurementRoundTemperatureC03 = 0.0;
  measurementRoundTemperatureC10 = 0.0, measurementRoundTemperatureC11 = 0.0, measurementRoundTemperatureC12 = 0.0, measurementRoundTemperatureC13 = 0.0;
  measurementRoundTemperatureC20 = 0.0, measurementRoundTemperatureC21 = 0.0, measurementRoundTemperatureC22 = 0.0, measurementRoundTemperatureC23 = 0.0;
  measurementRoundTemperatureC30 = 0.0, measurementRoundTemperatureC31 = 0.0, measurementRoundTemperatureC32 = 0.0, measurementRoundTemperatureC33 = 0.0;

  measurementRoundStatistic00.clear();
  measurementRoundStatistic01.clear();
  measurementRoundStatistic02.clear();
  measurementRoundStatistic03.clear();
  measurementRoundStatistic10.clear();
  measurementRoundStatistic11.clear();
  measurementRoundStatistic12.clear();
  measurementRoundStatistic13.clear();
  measurementRoundStatistic20.clear();
  measurementRoundStatistic21.clear();
  measurementRoundStatistic22.clear();
  measurementRoundStatistic23.clear();
  measurementRoundStatistic30.clear();
  measurementRoundStatistic31.clear();
  measurementRoundStatistic32.clear();
  measurementRoundStatistic33.clear();

}

//------------------------------------------------------------------------------

void setup() {

  wdt_disable();  // Disable the watchdog and wait for more than 2 seconds
  delay(3000);  // With this the Arduino doesn't keep resetting infinitely in case of wrong configuration
  wdt_enable(WDTO_250MS);

  //Serial.begin(115200);

  // Wait for USB Serial
  //while (!Serial) {
  //  ; // wait for serial port to connect. Needed for native USB port only
  //}
  //
  //    Serial.println(F("setup() begin"));

  clearRoundVariables();

  pinMode(ads0Relay, OUTPUT);
  pinMode(ads1Relay, OUTPUT);
  pinMode(ads2Relay, OUTPUT);
  //pinMode(ads3Relay, OUTPUT);

  digitalWrite(SD2_CS, HIGH);

  Controllino_RTC_init();

  getDateAndTime();
  startGetDateAndTimeInterval = millis();

  sprintf(logMsg, ("Hello world. I start now.,shutDownPeriod,%ld,measurementPeriod,%ld,measurementRoundPeriod,%ld"), shutDownPeriod, measurementPeriod, measurementRoundPeriod);

  sd1writeLog();
  sd2writeLog();

  sprintf(measurementfileHeader, ("YYYY-MM-DDThh:mm:ss,0-0-0 raw,0-0-0 RH,0-0-0 StDev,0-0-1 raw,0-0-1 C*,0-0-1 StDev,0-0-2 raw,0-0-2 RH,0-0-2 StDev,0-0-3 raw,0-0-3 C*,0-0-3 StDev,0-1-0 raw,0-1-0 RH,0-1-0 StDev,0-1-1 raw,0-1-1 C*,0-1-1 StDev,0-1-2 raw,0-1-2 RH,0-1-2 StDev,0-1-3 raw,0-1-3 C*,0-1-3 StDev,0-2-0 raw,0-2-0 RH,0-2-0 StDev,0-2-1 raw,0-2-1 C*,0-2-1 StDev,0-2-2 raw,0-2-2 RH,0-2-2 StDev,0-2-3 raw,0-2-3 C*,0-2-3 StDev,1-0-0 raw,1-0-0 C*,1-0-0 StDev,1-0-1 raw,1-0-1 C*,1-0-1 StDev,1-0-2 raw,1-0-2 C*,1-0-2 StDev,1-0-3 raw,1-0-3 C*,1-0-3 StDev,1-1-0 raw,1-1-0 C*,1-1-0 StDev,1-1-1 raw,1-1-1 C*,1-1-1 StDev,1-1-2 raw,1-1-2 C*,1-1-2 StDev,1-1-3 raw,1-1-3 C*,1-1-3 StDev,1-2-0 raw,1-2-0 C*,1-2-0 StDev,1-2-1 raw,1-2-1 C*,1-2-1 StDev,1-2-2 raw,1-2-2 C*,1-2-2 StDev,1-2-3 raw,1-2-3 C*,1-2-3 StDev,1-3-0 raw,1-3-0 C*,1-3-0 StDev,1-3-1 raw,1-3-1 C*,1-3-1 StDev,1-3-2 raw,1-3-2 C*,1-3-2 StDev,1-3-3 raw,1-3-3 C*,1-3-3 StDev"));

  sd1writeHeader();
  sd2writeHeader();

  tcaselect(0);
  port0InitializeADCs();

  tcaselect(1);
  port1InitializeADSs();

  tcaselect(0);

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
    digitalWrite(ads4Relay, HIGH);
    digitalWrite(ads5Relay, HIGH);
    digitalWrite(ads6Relay, HIGH);

    clearRoundVariables();

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
    //digitalWrite(ads3Relay, LOW);

  }

}
