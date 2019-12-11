/* Datasheet: 
 *  V OUT =(V SUPPLY )(0.0062(sensor RH) + 0.16), typical at 25 oC
 *  True RH = (Sensor RH)/(1.0546 – 0.00216T), T in oC
 *  
 *  Line Guide: http://stevenengineering.com/Tech_Support/PDFs/31G-HS.pdf
*/

/* Manufacturer source:
 * https://sensing.honeywell.com/HIH-4000-003-Humidity-Sensors
 */

// PLAN: ads3 channel 0 for HIH-4000-003, channel 1 for thermistor, channels 2 and 3 same.
// Can I use the Sparkfun HIH4030 library for this?
// How to set calibration values?

#include <SPI.h>
#include <Controllino.h> // Usage of CONTROLLINO library allows you to use CONTROLLINO_xx aliases in your sketch.
#include <Wire.h>
#include <Adafruit_ADS1015.h>

Adafruit_ADS1115 ads3(0x4B); //RH here

void setup() {
  Serial.begin(115200);

  ads3.setGain(GAIN_TWOTHIRDS);
  ads3.begin();

}

void loop() {
  int sensorValue = ads3.readADC_SingleEnded(0);
  Serial.println(sensorValue);
  delay(10);        // delay in between reads for stability

}

/* 

from https://forum.arduino.cc/index.php?topic=523949.0

const byte HIH4000_Pin = A0;
const int minimum = 169; // 0% calibration
const int maximum = 814; // 100% calibration
int HIHReading;
int trueRH;

void setup() {
  Serial.begin(9600);
}

void loop() {
  HIHReading = analogRead(HIH4000_Pin);
  Serial.print("RAW HIHReading: ");
  Serial.println(HIHReading);
  // temp correction here
  HIHReading = constrain(HIHReading, minimum, maximum);
  trueRH = map(HIHReading, minimum, maximum, 0, 100);
  Serial.print("Sensor RH: ");
  Serial.print(trueRH);
  Serial.println("%");
  delay(1000); // use millis() timing in final code
}

*/

/*
 * This is from: https://forum.arduino.cc/index.php?topic=143383.0
 */
// float GET_HUMIDITY(byte analogPin){
/*[Voltage output (2nd order curve fit)]  Vout=0.00003(sensor RH)^2+0.0281(sensor RH)+0.820, typical @ 25 ºC and 5VDC supply
the equation was found on http://www.phanderson.com/hih-4000.pdf
and a better / more ledigible typical output curve can be found at http://sensing.honeywell.com/index.php/ci_id/51625/la_id/1/document/1/re_id/0

Solving the above for (sensor RH) = 182.57418583506*(sqrt(Vout + 5.76008333333335) - 2.5651673109825)

Vout = (float)(analogRead(pin)) * vRef / 1023 where vRef = 5 volts
Vout = (float)(analogRead(pin)) * 5 / 1023
Vout = (float)(analogRead(pin)) * 0.004888

(sensor RH) = 182.57418583506*(sqrt(((float)(analogRead(pin)) * 0.004888) + 5.76008333333335) - 2.5651673109825)

as an example, assume Vout = 2.0 volts, (sensor RH) would then equal 40.9002 %
looking at the ledigible typical output curve, it can be seen that at 2.0 volts the best linear fit curve is just a little more than 40% indicating the calculation is correct
*/
//
//  float Percent_RH = 182.57418583506 * (sqrt(((float)(analogRead(analogPin)) * 0.004888) + 5.76008333333335) - 2.5651673109825);
//  return Percent_RH;
//} 
