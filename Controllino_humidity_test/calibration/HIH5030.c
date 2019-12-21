/*
 *****************************************************
 ** This software is offered strictly as is with no **
 ** warranties whatsoever. Use it at your own risk. **
 *****************************************************
 *
 * HIH5030.C
 *
 * Read the HIH5030 humidity sensor and display RH
 *   Set up for ADC5 (GPIO pin 5)
 *   ADC set to 12 bit resolution
 *   Sample frequency is EPOCH minutes
 */

#include "clk.h"
#include "qm_adc.h"
#include "qm_pinmux.h"
#include "qm_pin_functions.h"

#define EPOCH (1)		/* minutes between samples */

/* function prototypes */
void print_float(float value);
float get_RH(float Vout);
void delay_minutes(unsigned int mins);

/* These should be set as accurately as possible  */
float Vcc=3.32;    /* out measured Vcc */
float TempC=24.2;  /* measured temp for correction */


int main(void)
{
	qm_adc_config_t cfg;
	qm_adc_xfer_t xfer;
	qm_adc_channel_t channels[] = {QM_ADC_CH_5};

	uint16_t sample[1] = {0};
	int mcount;
	float voltage,pRH;

	/* NOTE: we are using polled mode to read the ADC */

	/* Enable the ADC and set the clock divisor. */
	clk_periph_enable(CLK_PERIPH_CLK | CLK_PERIPH_ADC |
			CLK_PERIPH_ADC_REGISTER);
	clk_adc_set_div(100); /* ADC clock is 320KHz @ 32MHz. */

	/* select the ADC pin function and input enable */
	qm_pmux_select(QM_PIN_ID_5, QM_PIN_8_FN_AIN_8);
	qm_pmux_input_en(QM_PIN_ID_5, true);

	/* Set the mode and calibrate. */
	qm_adc_set_mode(QM_ADC_0, QM_ADC_MODE_NORM_CAL);
	qm_adc_calibrate(QM_ADC_0);

	/* Configure the ADC. */
	cfg.window = 20; /* Clock cycles between the start of each sample. */
	cfg.resolution = QM_ADC_RES_12_BITS;
	qm_adc_set_config(QM_ADC_0, &cfg);

	/* Set up xfer. */
	xfer.ch = channels;
	xfer.ch_len = 1;       /* only using a single channel */
	xfer.samples = sample;
	xfer.samples_len = 1;  /* one shot */

	QM_PUTS("\nHIH5030 Sensor Test\n");
	mcount=0;	/* initialize sample counter */
	QM_PRINTF("\nSample   RH%      ADC     Vout\n");
	while(1){
		/* Run the conversion and wait for it to complete. */
		if (qm_adc_convert(QM_ADC_0, &xfer, NULL)) {
			QM_PUTS("Error: ADC conversion failed!");
			return 1; /* quit */
		}

		/* convert to voltage */
		voltage=(float)sample[0]*(Vcc / 4096.0);
		pRH=get_RH(voltage);
		QM_PRINTF("%d       ",mcount);
		print_float(pRH);
		/* use this if you want to print the sample value and voltage */
		QM_PRINTF("    %d", sample[0]);
		QM_PRINTF("    ");
		print_float(voltage);
		QM_PRINTF("\n");
		mcount++;
		delay_minutes(EPOCH);
	}

	return 0;
}

void print_float(float value)
{
	/*
	 *  simple dedicated floating point print routine
	 *  *basically* accurate to two place right of dp
	 */
	/* prints 2 digits after the decimal point */
	int digit,intdigits,fractdigits;

	if (value<0)
	{
		value*=-1;
		QM_PRINTF("-");
	}

	intdigits = (int) value;
	fractdigits=(10000 * (value-(float)intdigits));
	QM_PRINTF("%d",intdigits);
	QM_PRINTF(".");
	digit=fractdigits /1000;
	QM_PRINTF("%d",digit);
	fractdigits=fractdigits % 1000;
	digit=fractdigits /100;
	QM_PRINTF("%d",digit);
	/*
	fractdigits=fractdigits % 100;
	digit=fractdigits /10;
	QM_PRINTF("%d",digit);
	fractdigits=fractdigits % 10;
	digit=fractdigits /1;
	QM_PRINTF("%d",digit);
	*/

}

float get_RH(float voltage){
	/*
	 * Get 5050 RH
	 *  receive voltage from 12 bit analog to digital converter value
	 *  return RH as a float
	 */
	float percentRH1,percentRH;
	percentRH1=((voltage/(0.00636*Vcc))-23.8208);
	// assume a correction for temp from the static var TempC
	percentRH=percentRH1/(1.0546-(0.00216*TempC));
	return (percentRH);
}

void delay_minutes(unsigned int mins){
	unsigned int count;

	for(count=0;count<mins;count++){
		clk_sys_udelay(60000000UL);	/* delay 60 seconds */
	}
}
