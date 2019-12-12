# New Mosslogger Deal

Need to have independently functioning measurement periods and intervals with relay controls. Also, needs continous measurement.

Can this be done with Measurement structs?

	Measurement struct:
		int8_t analogPins[] // which analog pins to read
		int8_t relays[] // which adcs to use = which relays to operate
		int8_t ads0Channels[] = {0,1,2,3}
		int8_t ads1Channels[] = {0,1,2}
		int8_t ads2Channels[] = {}
		int8_t ads3Channels[] = {2}
		bool measuring
		unsigned long startIntervalMs
		unsigned long startMeasurementPeriodMs
		unsigned long startMeasurementRoundMs
		const unsigned long intervalMs
		const unsigned long measurementPeriodMs
		const unsigned long measurementRoundMs
		SdFile sdFiles[] = {measurementfile1,measurementfile2} // could measurementfile1 be used on both cards?