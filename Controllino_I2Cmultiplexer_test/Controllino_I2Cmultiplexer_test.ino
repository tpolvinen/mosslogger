
#include <SPI.h>
#include <Controllino.h> // Usage of CONTROLLINO library allows you to use CONTROLLINO_xx aliases in your sketch.
#include <Wire.h>

extern "C" { 
#include "utility/twi.h"  // from Wire library, so we can do bus scanning
}

#include <Adafruit_ADS1015.h>

Adafruit_ADS1115 ads0(0x48);
Adafruit_ADS1115 ads1(0x48);

#define TCAADDR 0x70

const int8_t MODULE0 = 0;
const int8_t MODULE1 = 1;

int16_t sample = 0;

void tcaselect(uint8_t i) {
  if (i > 7) return;

  Wire.beginTransmission(TCAADDR);
  Wire.write(1 << i);
  Wire.endTransmission();
}


void setup() {
  
  Wire.begin();

  Serial.begin(115200);
    while (!Serial);
  delay(1000);
  
  Serial.println("\nTCAScanner ready!");

  for (uint8_t t = 0; t < 8; t++) {
    tcaselect(t);
    Serial.print("TCA Port #"); Serial.println(t);

    for (uint8_t addr = 0; addr <= 127; addr++) {
      if (addr == TCAADDR) continue;

      uint8_t data;
      if (! twi_writeTo(addr, &data, 0, 1, 1)) {
        Serial.print("Found I2C 0x");  Serial.println(addr, HEX);
      }
    }
  }
  Serial.println("\nScanning done");
  
  Serial.println("\nBegin ads0");
  tcaselect(MODULE0);
  ads0.setGain(GAIN_TWOTHIRDS);
  ads0.begin();
  sample = ads0.readADC_SingleEnded(0);
  Serial.print("\nads0 sample:\t");
  Serial.println(sample);

  Serial.println("\nBegin ads1");
  tcaselect(MODULE1);
  ads1.setGain(GAIN_TWOTHIRDS);
  ads1.begin();
  sample = ads1.readADC_SingleEnded(0);
  Serial.print("\nads1 sample:\t");
  Serial.println(sample);

  Serial.println("\nads begin()s done");
}

void loop()
{
  tcaselect(MODULE0);
  sample = ads0.readADC_SingleEnded(0);
  Serial.print("ads0 sample:\t");
  Serial.print(sample);
    tcaselect(MODULE1);
  sample = ads1.readADC_SingleEnded(0);
  Serial.print("\tads1 sample:\t");
  Serial.println(sample);
  delay(500);
}
