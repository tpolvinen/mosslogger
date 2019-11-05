# mosslogger
Arduino based temperature data logger for moss experiment, with: Controllino MEGA, ADS1115, Micro SD Card Breakout Board, thermistors

## Step 1: 
Using Controllino MEGA, implement Adafruit ADS1115 with Adafruit LM4040 and get readings over I2C to serial monitor. Calculate (naive) averages over one second and how many measusement is read over that time. Instead of thermistors, using 10 kOhm resistors for stable output. No calculations to actual temperature readings at this point. Optionally, the calculations to degrees C will be implemented after all other functionalities are in place.

## Step 2: 
Testing four ADS1115s and two LM4040s together, using all 16 channels in single-ended reading. Implement statistics library to calculate the standard deviation of each ADS1115's measurements, taking in to account as many measurements as we can over one second period.

## Step 3: 
Implementation of data logging with Adafruit Micro SD Card Breakout Board. When all measurement and data logging features are functional, change 10 kOhm resistors to thermistors.

## Step 4: 
Optimize measurement frequency with data logging, keeping in mind the actual use case of one measurement data point per thermistor per second. Take care not to take too long with logging.

## Optionally: 
Test differential readings against single-ended ones, preferably in the final surroundings to determine if the chamber creates any noise in the measurements. Will there be any significant advantages of implementing differential reading between LM4040's 4.096 V and voltage dividers?

## Optionally: 
Implementation of calculations to convert data from ADS1115s to degrees C. Take care not to take too long with each calculation.

# Notes
Controllino MEGA, while connected to USB only, gives 4.33 V from X1 pins 1 (5V) and 2 (GND).
In contrast, the same pins X1 1 and 2 give 4.84 V while Controllino MEGA is connected to a 12 V PSU.
It may be that reference voltage is good enough for measurements when 12 V (or 24 V) PSU is connected.

