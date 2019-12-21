/*
 *****************************************************
 ** This software is offered strictly as is with no **
 ** warranties whatsoever. Use it at your own risk. **
 *****************************************************
 *
 * DHT22.C
 *
 * Reads a DHT-22 humidity and temperature sensor.
 */

/* includes */
#include "qm_gpio.h"
#include "clk.h"

/* prototypes */
int getDHT();
uint32_t wait_for_low();
uint32_t wait_for_high();
void print_float(float);
void delay_minutes(unsigned int);

/* defines */
#define DHTPIN (0)	/* DHT data pin connected to GPIO  */
#define EPOCH (1)   /* time between samples            */

/* globals */
qm_gpio_port_config_t GPIOcfg;
qm_gpio_state_t state;
uint8_t bytecounter, bitcounter, acounter;
uint8_t DHTdata[5];
/*
 5 bytes of DHT data are as follows:
 [0] RH integral
 [1] RH decimal
 [2] Temp integral
 [3] Temp decimal
 [4] checksum is the sum of all four bytes AND 255
 */
int finished;

int main() {

	float RH, Temp;
	uint16_t sampleN = 0; /* counter for  samples */

	QM_PUTS("\nDHT-22 Sensor Test\n");

	/* print banner */
	QM_PRINTF("Sample   Temp (C)      Humidity      CKSUM\n");

	while (1) {
		DHTdata[0] = DHTdata[1] = DHTdata[2] = DHTdata[3] = DHTdata[4] = 0;

		if (getDHT() != 0) {
			QM_PRINTF("\nError reading DHT\n");
		} else {
			/* QM_PRINTF("\nDHT read OK"); */
		}

		// Humidity
		RH = DHTdata[0];
		RH *= 256.0;
		RH += DHTdata[1];
		RH /= 10;
		// Temperature
		Temp = DHTdata[2] & 0x7f;
		Temp *= 256.0;
		Temp += DHTdata[3];
		Temp /= 10;
		if (DHTdata[2] & 0x80) {
			Temp *= -1;
		}
		/* Use the following for Temp C to F */
		/* float TempF=Temp * 9 / 5 +32; */
		QM_PRINTF("%d        ",sampleN);
		sampleN++;
		print_float(Temp);
		QM_PRINTF("         ");
		print_float(RH);
		// checksum
		if (((DHTdata[0] + DHTdata[1] + DHTdata[2] + DHTdata[3]) & 255)
				!= DHTdata[4]) {
			QM_PRINTF("         *** BAD CRC! ***  \n");
		} else {
			QM_PRINTF("         OK  \n");
		}
		delay_minutes(EPOCH);
	}
}

int getDHT() {

	acounter = 0; /* array index counter */
	bytecounter = 5; /* data bytes counter  */
	bitcounter = 8; /* data bits counter   */
	finished = 0; /* mark not finished   */

	/* make GPIO pin connected to DHT data pin output and set it low */
	GPIOcfg.direction |= (1U << DHTPIN);
	qm_gpio_set_config(QM_GPIO_0, &GPIOcfg);
	qm_gpio_clear_pin(QM_GPIO_0, DHTPIN);
	clk_sys_udelay(18000); /* delay 18 ms */
	/* bring data pin high for 30 us */
	qm_gpio_set_pin(QM_GPIO_0, DHTPIN);
	clk_sys_udelay(30); /* delay 30 us */
	/* set data pin to input */
	GPIOcfg.direction &= ~(1U << DHTPIN);
	qm_gpio_set_config(QM_GPIO_0, &GPIOcfg);
	/* DHT should set the line low for 80 us - check and wait for it */
	if (wait_for_low() == 0) {
		QM_PRINTF("\nTimeout error waiting for DHT low 1 \n");
		return (1); /* error */
	}
	/* DHT should set the line high for 80 us - check and wait for it */
	if (wait_for_high() == 0) {
		QM_PRINTF("\nTimeout error waiting for DHT high\n");
		return (1); /* error */
	}
	/* DHT should set the line low again marking the beginning of a start bit */
	if (wait_for_low() == 0) {
		QM_PRINTF("\nTimeout error waiting for DHT low 2\n");
		return (1); /* error */
	}

	/* get all 5 bytes */
	while (finished == 0) {
		/* this is the start bit - a 50 us low on the data line followed
		 by a 26-28 us high if bit='0' or 70 us high if bit='1' */
		/* wait for start bit time to end (a high) */
		if (wait_for_high() == 0) {
			QM_PRINTF("\nTimeout error waiting for DHT high\n");
			return (1); /* error */
		}
		/* delay for 40 us */
		clk_sys_udelay(40);
		/* read the data line to determine if the bit is a 1 or was a 0 */
		qm_gpio_read_pin(QM_GPIO_0, DHTPIN, &state);
		if (state == 0) {
			/* the bit was a 0 and it is over and the DHT is in a start bit time
			 with about 24 us left - process the bit and jump to wait for high */
			DHTdata[acounter] = DHTdata[acounter] << 1;
			DHTdata[acounter] &= 254; /* make sure lsb=0 (probably unnecessary) */
			if (--bitcounter == 0) {
				bitcounter = 8;
				acounter++;
				if (--bytecounter == 0) {
					finished = 1;
				}
			}
		} else {
			/* the bit was a '1' and we are still in the bit time - wait for it to end
			 first then process the bit and check for bumps */
			if (wait_for_low() == 0) {
				QM_PRINTF("\nTimeout error waiting for DHT low 3\n");
				return (1); /* error */
			}
			/* process the bit */
			DHTdata[acounter] = DHTdata[acounter] << 1;
			DHTdata[acounter] |= 1; /* set lsb=1 */
			/* bump bit counter */
			if (--bitcounter == 0) {
				bitcounter = 8;
				acounter++;
				if (--bytecounter == 0) {
					finished = 1;
				}
			}

		}
	}
	return (0);
}

uint32_t wait_for_low() {
	/* wait for the DHT pin to be low - return 0 on failure (count exceeded) */

	uint32_t count = 1;

	qm_gpio_read_pin(QM_GPIO_0, DHTPIN, &state);
	while (state != 0) {
		if (count++ >= 200000) { /* ~ 1-2 millisecond to read time out */
			return 0; /* Exceeded timeout, fail. */
		}
		qm_gpio_read_pin(QM_GPIO_0, DHTPIN, &state);
	}
	return count;
}

uint32_t wait_for_high() {
	/* wait for the DHT pin to be high - return 0 on failure (count exceeded) */
	uint32_t count = 1;
	qm_gpio_read_pin(QM_GPIO_0, DHTPIN, &state);
	while (state != 1) {
		if (count++ >= 200000) { /* ~ 1-2 millisecond to read time out */
			return 0; /* Exceeded timeout, fail. */
		}
		qm_gpio_read_pin(QM_GPIO_0, DHTPIN, &state);
	}
	return count;
}

void print_float(float value) {
	/*
	 *  simple dedicated floating point print routine
	 *  *basically* accurate to two place right of dp
	 */

	/* prints 2 digits after the decimal point */
	int digit, intdigits, fractdigits;

	if (value < 0) {
		value *= -1;
		QM_PRINTF("-");
	}

	intdigits = (int) value;
	fractdigits = (10000 * (value - (float) intdigits));
	QM_PRINTF("%d",intdigits); QM_PRINTF(".");
	digit = fractdigits / 1000;
	QM_PRINTF("%d",digit);
	fractdigits = fractdigits % 1000;
	digit = fractdigits / 100;
	QM_PRINTF("%d",digit);
}

void delay_minutes(unsigned int mins) {
	unsigned int count;

	for (count = 0; count < mins; count++) {
		clk_sys_udelay(60000000UL); /* delay 60 seconds */
	}
}
