# mosslogger
Arduino based temperature data logger for moss experiment, with: Controllino MEGA, ADS1115, Micro SD Card Breakout Board, thermistors

## Step 1: 
Using Controllino MEGA, implement Adafruit ADS1115 and get readings over I2C to serial monitor. Calculate (naive) averages over one second and how many measusement is read over that time. Instead of thermistors, using 10 kOhm resistors for stable output. No calculations to actual temperature readings at this point. Optionally, the calculations to degrees C will be implemented after all other functionalities are in place.

## Step 2: 
Testing four ADS1115s together, using all 16 channels in single-ended reading. Check that the voltage dividers work as expected: calculate Vout by hand, then calculate voltage from numbers produced by ADS1115s. Do they match?

## Step 3: 
Implementation of data logging with Adafruit Micro SD Card Breakout Board, optionally with two cards and SdFat library. When all measurement and data logging features are functional, change 10 kOhm resistors to potentiometers, then thermistors.

## Step 4: 
Optimize measurement frequency with data logging, keeping in mind the actual use case of one measurement data point per thermistor per second. Take care not to take too long with logging.

## Optionally: 
Test differential readings against single-ended ones, preferably in the final surroundings to determine if the chamber creates any noise in the measurements. Will there be any significant advantages of implementing differential reading between LM4040's 4.096 V and voltage dividers?

## Optionally: 
Implementation of calculations to convert data from ADS1115s to degrees C. Take care not to take too long with each calculation.

## Optionally:
Implement statistics library to calculate the standard deviation of each ADS1115's set of per second measurements, taking in as many measurements as we can in time of one second (this appears to be redundant since 4000 one-second-average readings have very small standard deviation, most under 1, reading values being over 12000)..

## Optionally:
Implementation of system log, separate of data log.

# Notes
LM4040 apparently suffers from a voltage drop when connected to several voltage dividers used with 10 kOhm resistors (+thermistors). Therefore, I leave them for "calibration" use that may or may not be implemented. :)
Controllino MEGA, while connected to USB only, gives 4.33 V from X1 pins 1 (5V) and 2 (GND).
In contrast, the same pins X1 1 and 2 give 4.84 V while Controllino MEGA is connected to a 12 V PSU.
It may be that reference voltage is good enough for measurements when 12 V (or 24 V) PSU is connected.

# Questions
How to check if the 5V power from pins is stable enough?
How to turn thermistors on and off by relays so they won't heat up their surroundings?
How to implement ifdef between single-end and differential readings? Same for GAIN setting and measurement period?
How to use two SD cards at once to store the same data on two separate cards?
Can ADS1115 readings be read in as bits instead of integers, and does it speed up the saving to SD card?
How can SD card data logging be made more robust?
How we can save the logs to RasPI periodically so we won't take time from measurements?
