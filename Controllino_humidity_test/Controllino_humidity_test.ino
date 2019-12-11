/* Datasheet: 
 *  V OUT =(V SUPPLY )(0.0062(sensor RH) + 0.16), typical at 25 oC
 *  True RH = (Sensor RH)/(1.0546 – 0.00216T), T in oC
 *  
 *  Line Guide: http://stevenengineering.com/Tech_Support/PDFs/31G-HS.pdf
*/

void setup() {
  // put your setup code here, to run once:

}

void loop() {
  // put your main code here, to run repeatedly:

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
