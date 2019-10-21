# mosslogger
Arduino based temperature data logger for moss experiment, with: Controllino MEGA, ADS1115, Micro SD Card Breakout Board, thermistors
Initial step: using Controllino MEGA, implement Adafruit ADS1115 and get readings over I2C to serial monitor. Instead of thermistors, using 10 kOhm resistors for stable output. No calculations to actual temperature readings at this point.

Secondly, testing two ADS1115s together, using all 8 channels total. One ADS1115 will be connected to Adafruit LM4040 voltage reference in both VDD and voltage dividers in it's channels. The other ADS1115 will be connected to LM4040 in voltage dividers only. We need to calculate the standard deviation of each ADS1115's measurements, taking in to account as many measurements as we can. This should give a sanity check whether we need LM4040 with Controllino MEGA.

Thirdly, implement data logging with Adafruit Micro SD Card Breakout Board.

Fourthly, optimize measurement frequency with data logging, keeping in mind the actual use case of one measurement data point per thermistor per second. Take care not to take too long with logging.

Optionally, test differential readings against single-ended ones, preferably in the final surroundings to determine if the chamber creates any noise in the measurements. Will there be any significant advantages of implementing differential reading between LM4040's 4.096 V and voltage dividers?

Optionally, implement calculations to convert data from ADS1115s to degrees C. Take care not to take too long with each calculation.
