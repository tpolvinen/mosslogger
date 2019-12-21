
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

Adafruit_ADS1115 ads(0x48); //RH here

#define ADSCHANNEL 0
#define HIH4000_SUPPLY_VOLTAGE 5.0
#define ARDUINO_VCC 4.87
#define SLOPE 0.029501451
#define OFFSET 0.881115

int16_t sensorValue = 0;
float voltage = 0.0;
float sensorRH = 0.0;
float trueRH = 0.0;
float temperature = 20.9;

void setup() {
  Serial.begin(115200);

  ads.setGain(GAIN_TWOTHIRDS);
  ads.begin();

}

void loop() {

  sensorValue = ads.readADC_SingleEnded(ADSCHANNEL);
  voltage = (sensorValue * 0.1875)/1000;
  sensorRH = (voltage - OFFSET) / SLOPE;
  trueRH = sensorRH / (1.0546 - (0.00216 * temperature));

  Serial.print("Sensor Voltage = ");
  Serial.print(voltage);
  Serial.print("V,  Sample = ");
  Serial.print(sensorValue);
  Serial.print("  Relative Humidity = ");
  Serial.print(sensorRH);
  Serial.print("%");
  Serial.print("  Temp-compensated RH = ");
  Serial.print(trueRH);
  Serial.println("%");

  delay(1000);

}

// float HIH4030::getSensorRH() {
//   return ((vout() - zeroOffset) / slope);
// }

// /*
//  * Get temperature-compensated relative humity. From data sheet: 
//  *  (3) trueRH = sensorRH / (1.0546 - 0.00216T)
//  */
// float HIH4030::getTrueRH(float temperature) {  
//   return getSensorRH() / (1.0546 - (0.00216 * temperature));

/* 

from https://forum.arduino.cc/index.php?topic=523949.0 -not sure if good with just constrain()?

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

/*
  * This is from calibration example at: https://www.allaboutcircuits.com/projects/how-to-check-and-calibrate-a-humidity-sensor/?cb=1576591105
  */
// /*
//  *****************************************************
//  ** This software is offered strictly as is with no **
//  ** warranties whatsoever. Use it at your own risk. **
//  *****************************************************
//  *
//  * HIH5030.C
//  *
//  * Read the HIH5030 humidity sensor and display RH
//  *   Set up for ADC5 (GPIO pin 5)
//  *   ADC set to 12 bit resolution
//  *   Sample frequency is EPOCH minutes
//  */

// #include "clk.h"
// #include "qm_adc.h"
// #include "qm_pinmux.h"
// #include "qm_pin_functions.h"

// #define EPOCH (1)		/* minutes between samples */

// /* function prototypes */
// void print_float(float value);
// float get_RH(float Vout);
// void delay_minutes(unsigned int mins);

// /* These should be set as accurately as possible  */
// float Vcc=3.32;    /* out measured Vcc */
// float TempC=24.2;  /* measured temp for correction */


// int main(void)
// {
// 	qm_adc_config_t cfg;
// 	qm_adc_xfer_t xfer;
// 	qm_adc_channel_t channels[] = {QM_ADC_CH_5};

// 	uint16_t sample[1] = {0};
// 	int mcount;
// 	float voltage,pRH;

// 	/* NOTE: we are using polled mode to read the ADC */

// 	/* Enable the ADC and set the clock divisor. */
// 	clk_periph_enable(CLK_PERIPH_CLK | CLK_PERIPH_ADC |
// 			CLK_PERIPH_ADC_REGISTER);
// 	clk_adc_set_div(100); /* ADC clock is 320KHz @ 32MHz. */

// 	/* select the ADC pin function and input enable */
// 	qm_pmux_select(QM_PIN_ID_5, QM_PIN_8_FN_AIN_8);
// 	qm_pmux_input_en(QM_PIN_ID_5, true);

// 	/* Set the mode and calibrate. */
// 	qm_adc_set_mode(QM_ADC_0, QM_ADC_MODE_NORM_CAL);
// 	qm_adc_calibrate(QM_ADC_0);

// 	/* Configure the ADC. */
// 	cfg.window = 20; /* Clock cycles between the start of each sample. */
// 	cfg.resolution = QM_ADC_RES_12_BITS;
// 	qm_adc_set_config(QM_ADC_0, &cfg);

// 	/* Set up xfer. */
// 	xfer.ch = channels;
// 	xfer.ch_len = 1;       /* only using a single channel */
// 	xfer.samples = sample;
// 	xfer.samples_len = 1;  /* one shot */

// 	QM_PUTS("\nHIH5030 Sensor Test\n");
// 	mcount=0;	/* initialize sample counter */
// 	QM_PRINTF("\nSample   RH%      ADC     Vout\n");
// 	while(1){
// 		/* Run the conversion and wait for it to complete. */
// 		if (qm_adc_convert(QM_ADC_0, &xfer, NULL)) {
// 			QM_PUTS("Error: ADC conversion failed!");
// 			return 1; /* quit */
// 		}

// 		/* convert to voltage */
// 		voltage=(float)sample[0]*(Vcc / 4096.0);
// 		pRH=get_RH(voltage);
// 		QM_PRINTF("%d       ",mcount);
// 		print_float(pRH);
// 		/* use this if you want to print the sample value and voltage */
// 		QM_PRINTF("    %d", sample[0]);
// 		QM_PRINTF("    ");
// 		print_float(voltage);
// 		QM_PRINTF("\n");
// 		mcount++;
// 		delay_minutes(EPOCH);
// 	}

// 	return 0;
// }

// void print_float(float value)
// {
// 	/*
// 	 *  simple dedicated floating point print routine
// 	 *  *basically* accurate to two place right of dp
// 	 */
// 	/* prints 2 digits after the decimal point */
// 	int digit,intdigits,fractdigits;

// 	if (value<0)
// 	{
// 		value*=-1;
// 		QM_PRINTF("-");
// 	}

// 	intdigits = (int) value;
// 	fractdigits=(10000 * (value-(float)intdigits));
// 	QM_PRINTF("%d",intdigits);
// 	QM_PRINTF(".");
// 	digit=fractdigits /1000;
// 	QM_PRINTF("%d",digit);
// 	fractdigits=fractdigits % 1000;
// 	digit=fractdigits /100;
// 	QM_PRINTF("%d",digit);
// 	/*
// 	fractdigits=fractdigits % 100;
// 	digit=fractdigits /10;
// 	QM_PRINTF("%d",digit);
// 	fractdigits=fractdigits % 10;
// 	digit=fractdigits /1;
// 	QM_PRINTF("%d",digit);
// 	*/

// }

// float get_RH(float voltage){
// 	/*
// 	 * Get 5050 RH
// 	 *  receive voltage from 12 bit analog to digital converter value
// 	 *  return RH as a float
// 	 */
// 	float percentRH1,percentRH;
// 	percentRH1=((voltage/(0.00636*Vcc))-23.8208);
// 	// assume a correction for temp from the static var TempC
// 	percentRH=percentRH1/(1.0546-(0.00216*TempC));
// 	return (percentRH);
// }

// void delay_minutes(unsigned int mins){
// 	unsigned int count;

// 	for(count=0;count<mins;count++){
// 		clk_sys_udelay(60000000UL);	/* delay 60 seconds */
// 	}
// }

/*
  * This is from: https://github.com/elnerda/HIH4000/blob/master/HIH4000.cpp
  */
/* float HIH4000::getTrueHumidity(float tempC)
{
	float zero_offset = 0.826; //25°
	float slope = 0.031483;    //25°
	int reading = analogRead(_pin);
	float voltage = reading*5.0;
	voltage/=1024.0;
	float RH=(voltage-zero_offset)/slope;
	float trueRH=(RH)/1.0546-0.0026*tempC;

	return trueRH;
}  */
