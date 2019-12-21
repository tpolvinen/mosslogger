/*
 *****************************************************
 ** This software is offered strictly as is with no **
 ** warranties whatsoever. Use it at your own risk. **
 *****************************************************
 *
 * HIH8121.C
 *
 * Read an HIH8121 over I2C
 */

/* includes */
#include "qm_common.h"
#include "qm_i2c.h"
#include "qm_pinmux.h"
#include "clk.h"

/* prototypes */
int I2CBegin();
void delay_minutes(unsigned int mins);
void print_float(float value);

/* defines */
#define BYTES2READ (4)
#define HIH8121addr (0x27) 	/* slave I2c address */
#define EPOCH (1)			/* time between reads (mins) */


/* globals */
qm_i2c_config_t I2Ccfg; 	   /* I2C structure variables             */
qm_i2c_status_t I2Cstatus; 	   /* see i2c.h for values                */
uint8_t HIHdata[4]; 		   /* data read from the sensor           */
uint8_t HIHstatus;			   /* sensor status bits		          */
uint16_t RHraw,Tempraw;		   /* raw RH (%) and temp (c) values      */
float RH,TempC;				   /* converted RH (%) and Temp (C)       */
uint8_t dummydata[1] = {0xff}; /* dummy data to complete read request */


int main(void) {

	int rescode, sample=0;

	QM_PUTS("\nHIH8121 Sensor Test\n");
	/* wait past the command mode window */
	clk_sys_udelay(50000); /* 50 ms delay */
	/* configure I2C and quit if error */
	if ( (rescode=I2CBegin()!=0) ) {
		QM_PRINTF("Error: Could not configure I2C (errno=%d)\n",rescode);
		return (1);
	}
	/* print header */
	QM_PRINTF("\nSample     RH          TempC");
	/* read sensor and display result loop*/
	while (1) {
		/* kick the sensor */
		if (qm_i2c_master_write(QM_I2C_0, HIH8121addr, dummydata,
				sizeof(dummydata), true, &I2Cstatus)) {
			return (errno); 	/* error code (errn) */
		}
		clk_sys_udelay(50000);  /* 60 ms delay for measurement (~36.65 ms)*/
		/* get the data */
		rescode=qm_i2c_master_read(QM_I2C_0, HIH8121addr, HIHdata, BYTES2READ, true,
				&I2Cstatus);
		if(rescode) {
			QM_PRINTF("Error: Could not read HIH8121 (errno=%d)\n",rescode);
			return (1);
		}
		HIHstatus=HIHdata[0] & 0b11000000;      /* get status bits */
		if(HIHstatus!=0){
			QM_PRINTF("\nBad Status Bits\n");
		}
		HIHdata[0]=HIHdata[0] & 0b00111111;  /* mask status     */
		Tempraw=(HIHdata[2]* 256) + HIHdata[3];
		Tempraw=Tempraw/4;	/* shift temperature */
		RHraw=(HIHdata[0] * 256) + HIHdata[1];
		RH=((float)RHraw /16382.0) * 100.0;
		TempC=(((float)Tempraw/16382.0)*165.0)- 40;
		QM_PRINTF("\n%d         ",sample);
		print_float(RH);
		QM_PRINTF("       ");
		print_float(TempC);
	    delay_minutes(EPOCH);
	    sample++;
	}
	/* never gets here */
	return (0);
}

int I2CBegin() {
	/*  Enable I2C 0 */
	clk_periph_enable(CLK_PERIPH_CLK | CLK_PERIPH_I2C_M0_REGISTER);
	/* set IO pins for SDA and SCL */
	qm_pmux_select(QM_PIN_ID_6, QM_PMUX_FN_2);
	qm_pmux_select(QM_PIN_ID_7, QM_PMUX_FN_2);

	/* Configure I2C */
	I2Ccfg.address_mode = QM_I2C_7_BIT;
	I2Ccfg.mode = QM_I2C_MASTER;
	I2Ccfg.speed = QM_I2C_SPEED_STD; /* 100 */

	/* set the configuration through the structure and return if failure */
	if (qm_i2c_set_config(QM_I2C_0, &I2Ccfg)) {
		QM_PUTS("Error: I2C_0 config\n");
		return (errno);
	} else {
		return (0);
	}
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
}

void delay_minutes(unsigned int mins){
	unsigned int count;

	for(count=0;count<mins;count++){
		clk_sys_udelay(60000000UL);	/* delay 60 seconds */
	}
}


